/**
 * @file led_playback.c
 * @brief LED playback engine (built-in effects, 120 FPS)
 */

#include "led_playback.h"
#include "led_driver.h"
#include "prism_temporal_api.h"
#include "pattern_metadata.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "prism_wave_tables.h"
#include "esp_attr.h"
#include "esp_cpu.h"
#include "perfmon.h"
#include <inttypes.h>
#include "esp_console.h"
#include <string.h>

// Built-in effect IDs (initial set)
#define EFFECT_WAVE_SINGLE      0x0001
#define EFFECT_PALETTE_CYCLE    0x0040

static const char *TAG = "playback";

// Optional performance instrumentation for frame build time (us)
#ifndef PRISM_PERF_INSTRUMENTATION
#define PRISM_PERF_INSTRUMENTATION 0
#endif

#if PRISM_PERF_INSTRUMENTATION
static uint64_t s_build_sum_us = 0;
static uint64_t s_build_max_us = 0;
static uint32_t s_build_samples = 0;
#endif

typedef struct {
    bool     running;
    uint16_t effect_id;
    uint8_t  params[8];
    uint8_t  param_count;
    uint32_t frame_counter;
} playback_state_t;

static playback_state_t s_pb = {0};

// Temporal integration (Task 14)
#define PRISM_LGP_LED_COUNT 160
static uint16_t s_temporal_ch1_u16[PRISM_LGP_LED_COUNT];
static uint16_t s_temporal_ch2_u16[PRISM_LGP_LED_COUNT];
static prism_temporal_ctx_t temporal_ctx;
static int64_t pattern_start_time_us = 0; // Pattern start timestamp (us)

// Update temporal context from loaded pattern metadata
static void update_temporal_context(const prism_pattern_meta_v11_t *meta) {
    if (!meta) {
        ESP_LOGW(TAG, "NULL metadata, using SYNC defaults");
        temporal_ctx.sync_mode = PRISM_SYNC_SYNC;
        temporal_ctx.motion_direction = PRISM_MOTION_STATIC;
        memset(&temporal_ctx.params, 0, sizeof(temporal_ctx.params));
        return;
    }

    // Validate and copy metadata
    if (PRISM_SYNC_IS_VALID(meta->sync_mode)) {
        temporal_ctx.sync_mode = (prism_sync_mode_t)meta->sync_mode;
    } else {
        ESP_LOGW(TAG, "Invalid sync mode %d, defaulting to SYNC", meta->sync_mode);
        temporal_ctx.sync_mode = PRISM_SYNC_SYNC;
    }

    if (PRISM_MOTION_IS_VALID(meta->motion_direction)) {
        temporal_ctx.motion_direction = (prism_motion_t)meta->motion_direction;
    } else {
        ESP_LOGW(TAG, "Invalid motion %d, defaulting to STATIC", meta->motion_direction);
        temporal_ctx.motion_direction = PRISM_MOTION_STATIC;
    }

    // Copy sync parameters
    memcpy(&temporal_ctx.params, &meta->params, sizeof(temporal_ctx.params));

    // Reset frame timing
    temporal_ctx.frame_index = 0;
    temporal_ctx.frame_time_ms = 0;

    ESP_LOGI(TAG, "Temporal context updated: mode=%d, motion=%d",
             temporal_ctx.sync_mode, temporal_ctx.motion_direction);
}

// Start a new pattern with temporal sequencing
static void playback_start_pattern(const prism_pattern_meta_v11_t *meta) {
    // Update temporal context from pattern metadata
    update_temporal_context(meta);

    // Record pattern start time
    pattern_start_time_us = esp_timer_get_time();
    temporal_ctx.frame_time_ms = 0;
    temporal_ctx.frame_index = 0;

    ESP_LOGI(TAG, "Pattern started at %lld us", pattern_start_time_us);
}

// Update frame timing before each frame calculation
static void playback_update_timing(void) {
    // Calculate elapsed time since pattern start
    int64_t now_us = esp_timer_get_time();
    int64_t elapsed_us = now_us - pattern_start_time_us;
    temporal_ctx.frame_time_ms = (uint32_t)(elapsed_us / 1000);
}

// Calculate CH2 frame using temporal logic (for future RMT dual-channel)
static void playback_frame_temporal(const uint16_t *ch1_frame_u16, size_t led_count) {
    // Update timing before calculation
    playback_update_timing();

    // Calculate CH2 frame using temporal logic
    calculate_ch2_frame(&temporal_ctx, ch1_frame_u16, s_temporal_ch2_u16, led_count);

    // Increment frame counter
    temporal_ctx.frame_index++;
}
// Precomputed spatial phase per LED (0-255 over LED_COUNT_PER_CH)
static uint8_t s_phase_per_led[LED_COUNT_PER_CH];

#ifdef CONFIG_PRISM_PROFILE_TEMPORAL
typedef struct {
    uint32_t total_cycles;
    uint32_t min_cycles;
    uint32_t max_cycles;
    uint32_t samples;
    // D$
    uint64_t dcache_hits;
    uint64_t dcache_misses;
    // I$
    uint64_t icache_hits;
    uint64_t icache_misses;
    // Instructions
    uint64_t insn_count;
} wave_prof_accum_t;

static wave_prof_accum_t s_prof_wave = {0};

static inline uint32_t wave_prof_begin(void) {
    // Configure PMU counters (up to 4) based on Kconfig toggles
    xtensa_perfmon_stop();
    int cid = 0;

    // DCache: use D_ACCESS_U1 with hits = Shared|Exclusive|Modified; misses = Misses
#if CONFIG_PRISM_PROFILE_COUNT_DCACHE
    if (cid <= 1) {
        xtensa_perfmon_init(cid++, XTPERF_CNT_D_ACCESS_U1,
                            XTPERF_MASK_D_ACCESS_HITS_SHARED |
                            XTPERF_MASK_D_ACCESS_HITS_EXCLUSIVE |
                            XTPERF_MASK_D_ACCESS_HITS_MODIFIED, 0, -1);
        xtensa_perfmon_init(cid++, XTPERF_CNT_D_ACCESS_U1,
                            XTPERF_MASK_D_ACCESS_CACHE_MISSES, 0, -1);
    }
#endif

    // ICache: use I_MEM with CACHE_HITS and CACHE_MISSES
#if CONFIG_PRISM_PROFILE_COUNT_ICACHE
    if (cid <= 2) {
        xtensa_perfmon_init(cid++, XTPERF_CNT_I_MEM, XTPERF_MASK_I_MEM_CACHE_HITS,   0, -1);
        xtensa_perfmon_init(cid++, XTPERF_CNT_I_MEM, XTPERF_MASK_I_MEM_CACHE_MISSES, 0, -1);
    }
#endif

    // Instruction count (only if counters remain)
#if CONFIG_PRISM_PROFILE_COUNT_INSN
    if (cid <= 3) {
        xtensa_perfmon_init(cid++, XTPERF_CNT_INSN, XTPERF_MASK_INSN_ALL, 0, -1);
    }
#endif

    // Reset active counters 0..cid-1
    for (int i = 0; i < cid; ++i) {
        xtensa_perfmon_reset(i);
    }
    xtensa_perfmon_start();
    return esp_cpu_get_cycle_count();
}

static inline void wave_prof_end(uint32_t start)
{
    uint32_t end = esp_cpu_get_cycle_count();
    uint32_t cycles = end - start;
    // Stop PMU and read counters in configured order
    xtensa_perfmon_stop();
    int rid = 0;
#if CONFIG_PRISM_PROFILE_COUNT_DCACHE
    uint32_t d_hits  = xtensa_perfmon_value(rid++);
    uint32_t d_miss  = xtensa_perfmon_value(rid++);
#endif
#if CONFIG_PRISM_PROFILE_COUNT_ICACHE
    uint32_t i_hits  = xtensa_perfmon_value(rid++);
    uint32_t i_miss  = xtensa_perfmon_value(rid++);
#endif
#if CONFIG_PRISM_PROFILE_COUNT_INSN
    uint32_t insn    = xtensa_perfmon_value(rid++);
#endif
    if (s_prof_wave.samples == 0 || cycles < s_prof_wave.min_cycles) {
        s_prof_wave.min_cycles = cycles;
    }
    if (cycles > s_prof_wave.max_cycles) {
        s_prof_wave.max_cycles = cycles;
    }
    s_prof_wave.total_cycles += cycles;
#if CONFIG_PRISM_PROFILE_COUNT_DCACHE
    s_prof_wave.dcache_hits   += d_hits;
    s_prof_wave.dcache_misses += d_miss;
#endif
#if CONFIG_PRISM_PROFILE_COUNT_ICACHE
    s_prof_wave.icache_hits   += i_hits;
    s_prof_wave.icache_misses += i_miss;
#endif
#if CONFIG_PRISM_PROFILE_COUNT_INSN
    s_prof_wave.insn_count    += insn;
#endif
    s_prof_wave.samples++;
}
#else
#define wave_prof_begin() (0)
#define wave_prof_end(start) do { (void)(start); } while(0)
#endif

esp_err_t playback_get_wave_metrics(prism_wave_metrics_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
#ifndef CONFIG_PRISM_PROFILE_TEMPORAL
    (void)out;
    return ESP_ERR_INVALID_STATE;
#else
    // Compute averages and percentages from current accumulators
    uint32_t avg = (s_prof_wave.samples) ? (s_prof_wave.total_cycles / s_prof_wave.samples) : 0;
    unsigned long long d_hits = s_prof_wave.dcache_hits;
    unsigned long long d_miss = s_prof_wave.dcache_misses;
    unsigned long long d_tot  = d_hits + d_miss;
    unsigned long d_hit_pct = d_tot ? (unsigned long)((d_hits * 100ULL) / d_tot) : 0;
    unsigned long long i_hits = s_prof_wave.icache_hits;
    unsigned long long i_miss = s_prof_wave.icache_misses;
    unsigned long long i_tot  = i_hits + i_miss;
    unsigned long i_hit_pct = i_tot ? (unsigned long)((i_hits * 100ULL) / i_tot) : 0;
    unsigned long ipc_x100 = 0;
#if CONFIG_PRISM_PROFILE_COUNT_INSN
    if (avg) ipc_x100 = (unsigned long)((s_prof_wave.insn_count * 100ULL) / avg);
#endif

    out->samples = s_prof_wave.samples;
    out->min_cycles = s_prof_wave.min_cycles;
    out->max_cycles = s_prof_wave.max_cycles;
    out->avg_cycles = avg;
    out->dcache_hits = d_hits;
    out->dcache_misses = d_miss;
    out->dcache_hit_pct = d_hit_pct;
    out->icache_hits = i_hits;
    out->icache_misses = i_miss;
    out->icache_hit_pct = i_hit_pct;
    out->insn_count = s_prof_wave.insn_count;
    out->ipc_x100 = ipc_x100;
    return ESP_OK;
#endif
}

#ifdef CONFIG_PRISM_METRICS_CLI
static int cmd_prism_metrics(int argc, char **argv)
{
    (void)argc; (void)argv;
    prism_wave_metrics_t m = {0};
    esp_err_t err = playback_get_wave_metrics(&m);
    if (err != ESP_OK) {
        printf("profiling disabled or unavailable (err=%d)\n", err);
        return 0;
    }
    printf("samples=%" PRIu32 " min=%" PRIu32 " max=%" PRIu32 " avg=%" PRIu32 " cycles\n", m.samples, m.min_cycles, m.max_cycles, m.avg_cycles);
    printf("D$ hits=%llu misses=%llu hit%%=%" PRIu32 "\n", (unsigned long long)m.dcache_hits, (unsigned long long)m.dcache_misses, m.dcache_hit_pct);
    printf("I$ hits=%llu misses=%llu hit%%=%" PRIu32 "\n", (unsigned long long)m.icache_hits, (unsigned long long)m.icache_misses, m.icache_hit_pct);
    printf("INSN=%llu IPC(x100)=%" PRIu32 "\n", (unsigned long long)m.insn_count, m.ipc_x100);
    return 0;
}

esp_err_t playback_register_cli(void)
{
    const esp_console_cmd_t cmd = {
        .command = "prism_metrics",
        .help = "Print WAVE profiling metrics",
        .hint = NULL,
        .func = &cmd_prism_metrics,
        .argtable = NULL,
    };
    return esp_console_cmd_register(&cmd);
}
#else
esp_err_t playback_register_cli(void) { return ESP_ERR_NOT_SUPPORTED; }
#endif

esp_err_t playback_init(void) {
    ESP_LOGI(TAG, "Initializing playback subsystem (120 FPS target)...");
    // Defer LED driver init until first play request
    // Precompute spatial phase offsets for WAVE effects
    for (int i = 0; i < LED_COUNT_PER_CH; i++) {
        s_phase_per_led[i] = (uint8_t)((i * 256) / LED_COUNT_PER_CH);
    }
    // Initialize temporal buffers
    memset(s_temporal_ch1_u16, 0, sizeof(s_temporal_ch1_u16));
    memset(s_temporal_ch2_u16, 0, sizeof(s_temporal_ch2_u16));

#ifdef CONFIG_PRISM_METRICS_CLI
    (void)playback_register_cli();
#endif

    // Initialize temporal context (use temporary u16 CH1/CH2 buffers)
    esp_err_t terr = prism_motion_init(&temporal_ctx, s_temporal_ch1_u16, s_temporal_ch2_u16, PRISM_LGP_LED_COUNT);
    if (terr != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init temporal context: %s", esp_err_to_name(terr));
        return terr;
    }

    ESP_LOGI(TAG, "Playback subsystem ready");
    return ESP_OK;
}

void playback_task(void *pvParameters) {
    ESP_LOGI(TAG, "Playback task started on core %d (HIGHEST priority)", xPortGetCoreID());

    TickType_t last_wake = xTaskGetTickCount();

    // Simple frame buffers for both channels (GRB)
    static uint8_t frame_ch1[LED_FRAME_SIZE_CH];
    static uint8_t frame_ch2[LED_FRAME_SIZE_CH];

    // Main render loop at LED_FPS_TARGET
    while (1) {
        if (s_pb.running) {
            #if PRISM_PERF_INSTRUMENTATION
            int64_t build_t0 = esp_timer_get_time();
            #endif
            // Minimal built-in effects (fast, integer math)
            switch (s_pb.effect_id) {
                case EFFECT_WAVE_SINGLE: {
                    uint32_t t0 = wave_prof_begin();
                    // LUT-based sinusoidal green wave across strip using sin8_table
                    // Params: [0]=amplitude (0-255), [1]=speed (phase inc per frame)
                    uint8_t amp = (s_pb.param_count >= 1) ? s_pb.params[0] : 255;
                    uint8_t spd = (s_pb.param_count >= 2) ? s_pb.params[1] : 2;
                    uint8_t tphase = (uint8_t)(s_pb.frame_counter * spd);
                    for (int i = 0; i < LED_COUNT_PER_CH; i++) {
                        uint8_t phase = (uint8_t)(s_phase_per_led[i] + tphase);
                        uint8_t s = sin8_table[phase];
                        // Scale sine by amplitude
                        uint8_t val = (uint8_t)((uint16_t)s * amp / 255);
                        // GRB: set G=val, R=0, B=0
                        frame_ch1[i * 3 + 0] = val; // G
                        frame_ch1[i * 3 + 1] = 0;   // R
                        frame_ch1[i * 3 + 2] = 0;   // B
                        frame_ch2[i * 3 + 0] = val; // G
                        frame_ch2[i * 3 + 1] = 0;   // R
                        frame_ch2[i * 3 + 2] = 0;   // B
                    }
                    wave_prof_end(t0);
#ifdef CONFIG_PRISM_PROFILE_TEMPORAL
                    if ((s_pb.frame_counter % 120) == 0 && s_prof_wave.samples) {
                        uint32_t avg = s_prof_wave.total_cycles / s_prof_wave.samples;
                        // Derive rates (guard div-by-zero)
                        unsigned long long d_hits = s_prof_wave.dcache_hits;
                        unsigned long long d_miss = s_prof_wave.dcache_misses;
                        unsigned long long d_tot  = d_hits + d_miss;
                        unsigned long d_hit_pct = d_tot ? (unsigned long)((d_hits * 100ULL) / d_tot) : 0;
                        unsigned long d_miss_pct = d_tot ? (unsigned long)((d_miss * 100ULL) / d_tot) : 0;

                        unsigned long long i_hits = s_prof_wave.icache_hits;
                        unsigned long long i_miss = s_prof_wave.icache_misses;
                        unsigned long long i_tot  = i_hits + i_miss;
                        unsigned long i_hit_pct = i_tot ? (unsigned long)((i_hits * 100ULL) / i_tot) : 0;
                        unsigned long i_miss_pct = i_tot ? (unsigned long)((i_miss * 100ULL) / i_tot) : 0;

                        // Optional IPC
                        unsigned long ipc_x100 = 0;
#if CONFIG_PRISM_PROFILE_COUNT_INSN
                        if (avg) ipc_x100 = (unsigned long)((s_prof_wave.insn_count * 100ULL) / avg);
#endif

                        ESP_LOGI(TAG, "WAVE prof: samples=%lu min=%lu max=%lu avg=%lu cycles | D$ hit/miss=%llu/%llu (%lu%%/%lu%%) | I$ hit/miss=%llu/%llu (%lu%%/%lu%%)%s%lu",
                                 (unsigned long)s_prof_wave.samples,
                                 (unsigned long)s_prof_wave.min_cycles,
                                 (unsigned long)s_prof_wave.max_cycles,
                                 (unsigned long)avg,
                                 d_hits, d_miss, d_hit_pct, d_miss_pct,
                                 i_hits, i_miss, i_hit_pct, i_miss_pct,
                                 #if CONFIG_PRISM_PROFILE_COUNT_INSN
                                 " | IPC(x100)=",
                                 #else
                                 " | IPC(x100)=",
                                 #endif
                                 ipc_x100);
                        // Reset accumulators each second window
                        s_prof_wave.total_cycles = 0;
                        s_prof_wave.min_cycles = 0;
                        s_prof_wave.max_cycles = 0;
                        s_prof_wave.dcache_hits = 0;
                        s_prof_wave.dcache_misses = 0;
                        s_prof_wave.icache_hits = 0;
                        s_prof_wave.icache_misses = 0;
                        s_prof_wave.insn_count = 0;
                        s_prof_wave.samples = 0;
                    }
#endif
                    break;
                }
                case EFFECT_PALETTE_CYCLE:
                default: {
                    // Cycle through simple GRB palette
                    uint8_t t = (uint8_t)(s_pb.frame_counter & 0xFF);
                    uint8_t r = t;
                    uint8_t g = 255 - t;
                    uint8_t b = (t >> 1) ^ 0x7F;
                    for (int i = 0; i < LED_COUNT_PER_CH; i++) {
                        // Slight spatial offset per LED to create gradient
                        uint8_t o = (uint8_t)((i * 2 + t) & 0xFF);
                        frame_ch1[i * 3 + 0] = g ^ o;  // G
                        frame_ch1[i * 3 + 1] = r ^ (o >> 1);  // R
                        frame_ch1[i * 3 + 2] = b ^ (o << 1);  // B
                        frame_ch2[i * 3 + 0] = frame_ch1[i * 3 + 0];
                        frame_ch2[i * 3 + 1] = frame_ch1[i * 3 + 1];
                        frame_ch2[i * 3 + 2] = frame_ch1[i * 3 + 2];
                    }
                    break;
                }
            }
            #if PRISM_PERF_INSTRUMENTATION
            int64_t build_dt = esp_timer_get_time() - build_t0;
            s_build_sum_us += (uint64_t)build_dt;
            if ((uint64_t)build_dt > s_build_max_us) s_build_max_us = (uint64_t)build_dt;
            s_build_samples++;
            if ((s_pb.frame_counter % 120) == 0 && s_build_samples) {
                uint64_t avg = s_build_sum_us / s_build_samples;
                ESP_LOGI(TAG, "Frame build: samples=%lu max=%luus avg=%luus",
                         (unsigned long)s_build_samples, (unsigned long)s_build_max_us, (unsigned long)avg);
                s_build_sum_us = 0; s_build_max_us = 0; s_build_samples = 0;
            }
            #endif
            // Temporal integration: derive u16 CH1 intensities from current frame_ch1
            for (int i = 0; i < LED_COUNT_PER_CH; ++i) {
                // Use max channel as brightness proxy (GRB order)
                uint8_t g = frame_ch1[i * 3 + 0];
                uint8_t r = frame_ch1[i * 3 + 1];
                uint8_t b = frame_ch1[i * 3 + 2];
                uint8_t maxc = g;
                if (r > maxc) maxc = r;
                if (b > maxc) maxc = b;
                s_temporal_ch1_u16[i] = (uint16_t)(maxc * 257u); // scale 8-bit to 16-bit
            }

            // Update monotonic timing and compute CH2 u16 intensities
            playback_update_timing();
            calculate_ch2_frame(&temporal_ctx, s_temporal_ch1_u16, s_temporal_ch2_u16, PRISM_LGP_LED_COUNT);
            temporal_ctx.frame_index++;

            // Apply temporal CH2 to GRB frame: zero-out LEDs that are not yet active
            for (int i = 0; i < LED_COUNT_PER_CH; ++i) {
                if (s_temporal_ch2_u16[i] == 0) {
                    frame_ch2[i * 3 + 0] = 0;
                    frame_ch2[i * 3 + 1] = 0;
                    frame_ch2[i * 3 + 2] = 0;
                } else {
                    // Active: mirror CH1 pixel
                    frame_ch2[i * 3 + 0] = frame_ch1[i * 3 + 0];
                    frame_ch2[i * 3 + 1] = frame_ch1[i * 3 + 1];
                    frame_ch2[i * 3 + 2] = frame_ch1[i * 3 + 2];
                }
            }

            (void)led_driver_submit_frames(frame_ch1, frame_ch2);
            s_pb.frame_counter++;
        }

        // 120 FPS frame time
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(LED_FRAME_TIME_MS));
    }

    ESP_LOGW(TAG, "Playback task exiting (unexpected)");
    vTaskDelete(NULL);
}

esp_err_t playback_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing playback subsystem...");
    // No explicit resources; LED driver lifecycle is separate
    return ESP_OK;
}

esp_err_t playback_play_builtin(uint16_t effect_id, const uint8_t* params, uint8_t param_count)
{
    // Ensure LED driver is initialized and running
    esp_err_t ret = led_driver_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "LED driver init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = led_driver_start();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "LED driver start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set playback state
    s_pb.effect_id = effect_id;
    s_pb.param_count = (param_count > 8) ? 8 : param_count;
    if (params && s_pb.param_count) {
        memcpy(s_pb.params, params, s_pb.param_count);
    }
    s_pb.frame_counter = 0;
    s_pb.running = true;
    // Initialize temporal timing baseline for built-in effects
    pattern_start_time_us = esp_timer_get_time();
    temporal_ctx.frame_index = 0;
    temporal_ctx.frame_time_ms = 0;
    ESP_LOGI(TAG, "Playback started: effect=0x%04X params=%u fps=%d", effect_id, s_pb.param_count, LED_FPS_TARGET);
    return ESP_OK;
}

esp_err_t playback_stop(void)
{
    if (!s_pb.running) {
        return ESP_OK;
    }
    s_pb.running = false;
    // Clear LEDs once
    static uint8_t black[LED_FRAME_SIZE_CH] = {0};
    (void)led_driver_submit_frames(black, black);
    ESP_LOGI(TAG, "Playback stopped (driver remains running)");
    return ESP_OK;
}

bool playback_is_running(void)
{
    return s_pb.running;
}
