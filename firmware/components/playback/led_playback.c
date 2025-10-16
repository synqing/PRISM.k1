/**
 * @file led_playback.c
 * @brief LED playback engine (built-in effects, 120 FPS)
 */

#include "led_playback.h"
#include "led_driver.h"
#include "prism_temporal_api.h"
#include "pattern_metadata.h"
#include "prism_parser.h"
#include "pattern_storage.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "prism_wave_tables.h"
#include "esp_attr.h"
#include "esp_cpu.h"
#include "perfmon.h"
#include "esp_rom_crc.h"
#include <inttypes.h>
#include "esp_console.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "effect_engine.h"

// Built-in effect IDs (initial set)
#define EFFECT_WAVE_SINGLE      0x0001
#define EFFECT_PALETTE_CYCLE    0x0040

#define PLAYBACK_PATTERN_ID_MAX 64
#define PRISM_FLAG_DELTA        0x01
#define PRISM_FLAG_RLE          0x02
#define PRISM_RLE_MARK          0x80
#define PRISM_RLE_MASK          0x7F
#define PRISM_MAX_PALETTE       64

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

typedef enum {
    PLAYBACK_SOURCE_NONE = 0,
    PLAYBACK_SOURCE_BUILTIN,
    PLAYBACK_SOURCE_PATTERN
} playback_source_t;

typedef struct {
    bool     running;
    playback_source_t source;
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
static int64_t s_last_fx_tick_us = 0;     // Effect engine last tick time

typedef struct {
    bool loaded;
    char id[PLAYBACK_PATTERN_ID_MAX];
    prism_header_v11_t header;
    uint8_t *frames;
    size_t frame_stride;
    uint32_t frame_count;
    uint32_t current_frame;
    uint32_t led_count;
    uint32_t frame_interval_us;
    int64_t last_frame_us;
} pattern_runtime_t;

static pattern_runtime_t s_pattern = {0};

static void playback_free_pattern(void)
{
    if (s_pattern.frames) {
        free(s_pattern.frames);
    }
    memset(&s_pattern, 0, sizeof(s_pattern));
}

void playback_normalize_pattern_id(const char *input, char *output, size_t output_len)
{
    if (!output || output_len == 0) {
        return;
    }

    size_t out_idx = 0;
    bool dot_seen = false;

    const char *src = input;
    if (!src || src[0] == '\0') {
        strlcpy(output, "pattern", output_len);
        return;
    }

    for (size_t i = 0; src[i] != '\0'; ++i) {
        char c = src[i];

        if (c == '.' && !dot_seen) {
            dot_seen = true;
            continue;
        }

        if (c == '/' || c == '\\') {
            continue;
        }

        if (isalnum((unsigned char)c)) {
            output[out_idx++] = (char)tolower((unsigned char)c);
        } else if (c == '-' || c == '_') {
            output[out_idx++] = c;
        } else {
            output[out_idx++] = '_';
        }

        if (out_idx >= output_len - 1) {
            break;
        }
    }

    if (out_idx == 0) {
        strlcpy(output, "pattern", output_len);
    } else {
        output[out_idx] = '\0';
    }
}

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

// Brightness CLI (unconditional)
static int cmd_prism_brightness(int argc, char **argv)
{
    uint32_t target = 255;
    uint32_t dur = 0;
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        if (strncasecmp(a, "target=", 7) == 0) {
            target = (uint32_t)strtoul(a + 7, NULL, 0);
            if (target > 255) target = 255;
        } else if (strncasecmp(a, "dur=", 4) == 0) {
            dur = (uint32_t)strtoul(a + 4, NULL, 0);
        } else if (strncasecmp(a, "ms=", 3) == 0) {
            dur = (uint32_t)strtoul(a + 3, NULL, 0);
        }
    }
    ESP_LOGI(TAG, "CLI brightness: target=%lu dur_ms=%lu", (unsigned long)target, (unsigned long)dur);
    playback_set_brightness((uint8_t)target, dur);
    return 0;
}

static void playback_register_brightness_cli(void)
{
    const esp_console_cmd_t cmd = {
        .command = "prism_brightness",
        .help = "Set global brightness: target=<0-255> dur=<ms>",
        .hint = NULL,
        .func = &cmd_prism_brightness,
        .argtable = NULL,
    };
    (void)esp_console_cmd_register(&cmd);
}

static int cmd_prism_play_pattern(int argc, char **argv)
{
    const char *arg = NULL;
    for (int i = 1; i < argc; ++i) {
        const char *token = argv[i];
        if (token == NULL) {
            continue;
        }
        if (strncasecmp(token, "pattern=", 8) == 0) {
            arg = token + 8;
            break;
        }
        if (strcmp(token, "--pattern") == 0 && (i + 1) < argc) {
            arg = argv[i + 1];
            break;
        }
        if (arg == NULL) {
            arg = token;
        }
    }

    if (!arg || arg[0] == '\0') {
        printf("usage: prism_play <pattern-id>\n");
        return 0;
    }

    char normalized[PLAYBACK_PATTERN_ID_MAX];
    playback_normalize_pattern_id(arg, normalized, sizeof(normalized));

    esp_err_t err = playback_play_pattern_from_storage(normalized);
    if (err == ESP_OK) {
        printf("playing pattern '%s'\n", normalized);
    } else {
        printf("failed to play pattern '%s' (err=%s)\n", normalized, esp_err_to_name(err));
    }
    return 0;
}

static int cmd_prism_stop(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    esp_err_t err = playback_stop();
    if (err != ESP_OK) {
        printf("stop failed: %s\n", esp_err_to_name(err));
    } else {
        printf("playback stopped\n");
    }
    return 0;
}

static void playback_register_pattern_cli(void)
{
    const esp_console_cmd_t play_cmd = {
        .command = "prism_play",
        .help = "Play stored pattern: prism_play <pattern-id>",
        .hint = NULL,
        .func = &cmd_prism_play_pattern,
        .argtable = NULL,
    };
    (void)esp_console_cmd_register(&play_cmd);

    const esp_console_cmd_t stop_cmd = {
        .command = "prism_stop",
        .help = "Stop pattern playback",
        .hint = NULL,
        .func = &cmd_prism_stop,
        .argtable = NULL,
    };
    (void)esp_console_cmd_register(&stop_cmd);
}

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

    // Initialize effect engine
    effect_engine_init();
    effect_chain_clear();
    effect_add_brightness(255); // default full brightness

#ifdef CONFIG_PRISM_METRICS_CLI
    (void)playback_register_cli();
#endif
    playback_register_brightness_cli();
    playback_register_pattern_cli();

    // Initialize temporal context (use temporary u16 CH1/CH2 buffers)
    esp_err_t terr = prism_motion_init(&temporal_ctx, s_temporal_ch1_u16, s_temporal_ch2_u16, PRISM_LGP_LED_COUNT);
    if (terr != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init temporal context: %s", esp_err_to_name(terr));
        return terr;
    }

    ESP_LOGI(TAG, "Playback subsystem ready");
    s_pb.source = PLAYBACK_SOURCE_NONE;
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
            if (s_pb.source == PLAYBACK_SOURCE_PATTERN) {
                if (s_pattern.loaded && s_pattern.frames && s_pattern.frame_count > 0) {
                    int64_t now_us = esp_timer_get_time();
                    if (s_pattern.last_frame_us == 0 || now_us < s_pattern.last_frame_us) {
                        s_pattern.last_frame_us = now_us;
                    }
                    uint32_t interval_us = s_pattern.frame_interval_us ? s_pattern.frame_interval_us : (1000000 / LED_FPS_TARGET);
                    if ((uint64_t)(now_us - s_pattern.last_frame_us) >= interval_us) {
                        uint32_t advance = (uint32_t)((now_us - s_pattern.last_frame_us) / interval_us);
                        if (advance == 0) {
                            advance = 1;
                        }
                        s_pattern.last_frame_us += (int64_t)advance * interval_us;
                        s_pattern.current_frame = (s_pattern.current_frame + advance) % s_pattern.frame_count;
                    }

                    const uint8_t *frame_ptr = s_pattern.frames + s_pattern.current_frame * s_pattern.frame_stride;
                    memcpy(frame_ch1, frame_ptr, LED_FRAME_SIZE_CH);
                    memcpy(frame_ch2, frame_ptr, LED_FRAME_SIZE_CH);

                    int64_t now_us_fx = now_us;
                    uint32_t elapsed_ms = 0;
                    if (s_last_fx_tick_us == 0) {
                        s_last_fx_tick_us = now_us_fx;
                    } else {
                        int64_t dt = now_us_fx - s_last_fx_tick_us;
                        if (dt < 0) dt = 0;
                        elapsed_ms = (uint32_t)(dt / 1000);
                        s_last_fx_tick_us = now_us_fx;
                    }
                    if (elapsed_ms) {
                        effect_engine_tick(elapsed_ms);
                    }
                    effect_chain_apply(frame_ch1, LED_COUNT_PER_CH);
                    effect_chain_apply(frame_ch2, LED_COUNT_PER_CH);

                    (void)led_driver_submit_frames(frame_ch1, frame_ch2);
                    s_pb.frame_counter++;
                }
            } else {
#if PRISM_PERF_INSTRUMENTATION
                int64_t build_t0 = esp_timer_get_time();
#endif
                // Minimal built-in effects (fast, integer math)
                switch (s_pb.effect_id) {
                    case EFFECT_WAVE_SINGLE: {
                        uint32_t t0 = wave_prof_begin();
                        uint8_t amp = (s_pb.param_count >= 1) ? s_pb.params[0] : 255;
                        uint8_t spd = (s_pb.param_count >= 2) ? s_pb.params[1] : 2;
                        uint8_t tphase = (uint8_t)(s_pb.frame_counter * spd);
                        for (int i = 0; i < LED_COUNT_PER_CH; i++) {
                            uint8_t phase = (uint8_t)(s_phase_per_led[i] + tphase);
                            uint8_t s = sin8_table[phase];
                            uint8_t val = (uint8_t)((uint16_t)s * amp / 255);
                            frame_ch1[i * 3 + 0] = val;
                            frame_ch1[i * 3 + 1] = 0;
                            frame_ch1[i * 3 + 2] = 0;
                            frame_ch2[i * 3 + 0] = val;
                            frame_ch2[i * 3 + 1] = 0;
                            frame_ch2[i * 3 + 2] = 0;
                        }
                        wave_prof_end(t0);
#ifdef CONFIG_PRISM_PROFILE_TEMPORAL
                        if ((s_pb.frame_counter % 120) == 0 && s_prof_wave.samples) {
                            uint32_t avg = s_prof_wave.total_cycles / s_prof_wave.samples;
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
                        uint8_t t = (uint8_t)(s_pb.frame_counter & 0xFF);
                        uint8_t r = t;
                        uint8_t g = 255 - t;
                        uint8_t b = (t >> 1) ^ 0x7F;
                        for (int i = 0; i < LED_COUNT_PER_CH; i++) {
                            uint8_t o = (uint8_t)((i * 2 + t) & 0xFF);
                            frame_ch1[i * 3 + 0] = g ^ o;
                            frame_ch1[i * 3 + 1] = r ^ (o >> 1);
                            frame_ch1[i * 3 + 2] = b ^ (o << 1);
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
                for (int i = 0; i < LED_COUNT_PER_CH; ++i) {
                    uint8_t g = frame_ch1[i * 3 + 0];
                    uint8_t r = frame_ch1[i * 3 + 1];
                    uint8_t b = frame_ch1[i * 3 + 2];
                    uint8_t maxc = g;
                    if (r > maxc) maxc = r;
                    if (b > maxc) maxc = b;
                    s_temporal_ch1_u16[i] = (uint16_t)(maxc * 257u);
                }

                playback_update_timing();
                calculate_ch2_frame(&temporal_ctx, s_temporal_ch1_u16, s_temporal_ch2_u16, PRISM_LGP_LED_COUNT);
                temporal_ctx.frame_index++;

                for (int i = 0; i < LED_COUNT_PER_CH; ++i) {
                    if (s_temporal_ch2_u16[i] == 0) {
                        frame_ch2[i * 3 + 0] = 0;
                        frame_ch2[i * 3 + 1] = 0;
                        frame_ch2[i * 3 + 2] = 0;
                    } else {
                        frame_ch2[i * 3 + 0] = frame_ch1[i * 3 + 0];
                        frame_ch2[i * 3 + 1] = frame_ch1[i * 3 + 1];
                        frame_ch2[i * 3 + 2] = frame_ch1[i * 3 + 2];
                    }
                }

                int64_t now_us_fx = esp_timer_get_time();
                uint32_t elapsed_ms = 0;
                if (s_last_fx_tick_us == 0) {
                    s_last_fx_tick_us = now_us_fx;
                } else {
                    int64_t dt = now_us_fx - s_last_fx_tick_us;
                    if (dt < 0) dt = 0;
                    elapsed_ms = (uint32_t)(dt / 1000);
                    s_last_fx_tick_us = now_us_fx;
                }
                if (elapsed_ms) {
                    effect_engine_tick(elapsed_ms);
                }
                effect_chain_apply(frame_ch1, LED_COUNT_PER_CH);
                effect_chain_apply(frame_ch2, LED_COUNT_PER_CH);

                (void)led_driver_submit_frames(frame_ch1, frame_ch2);
                s_pb.frame_counter++;
            }
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
    playback_free_pattern();
    s_pb.effect_id = effect_id;
    s_pb.param_count = (param_count > 8) ? 8 : param_count;
    if (params && s_pb.param_count) {
        memcpy(s_pb.params, params, s_pb.param_count);
    }
    s_pb.frame_counter = 0;
    s_pb.running = true;
    s_pb.source = PLAYBACK_SOURCE_BUILTIN;
    // Initialize temporal timing baseline for built-in effects
    pattern_start_time_us = esp_timer_get_time();
    temporal_ctx.frame_index = 0;
    temporal_ctx.frame_time_ms = 0;
    s_last_fx_tick_us = esp_timer_get_time();
    ESP_LOGI(TAG, "Playback started: effect=0x%04X params=%u fps=%d", effect_id, s_pb.param_count, LED_FPS_TARGET);
    return ESP_OK;
}

esp_err_t playback_play_prism_blob(const char *pattern_id, const uint8_t *blob, size_t blob_size)
{
    if (blob == NULL || blob_size < sizeof(prism_header_v10_t)) {
        return ESP_ERR_INVALID_ARG;
    }

    // Stop any existing playback and clear state
    (void)playback_stop();

    prism_header_v11_t header = {0};
    esp_err_t ret = parse_prism_header(blob, blob_size, &header);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to parse .prism header (%s)", esp_err_to_name(ret));
        return ret;
    }

    size_t offset = sizeof(prism_header_v10_t);
    if (header.base.version == 0x0101) {
        offset += sizeof(prism_pattern_meta_v11_t);
    }

    if (offset + 2 > blob_size) {
        return ESP_ERR_INVALID_SIZE;
    }
    uint16_t extra_len = (uint16_t)(blob[offset] | ((uint16_t)blob[offset + 1] << 8));
    offset += 2;
    if (offset + extra_len > blob_size) {
        return ESP_ERR_INVALID_SIZE;
    }
    offset += extra_len;

    if (offset + 4 > blob_size) {
        return ESP_ERR_INVALID_SIZE;
    }

    size_t payload_len = blob_size - offset - 4;
    const uint8_t *payload = blob + offset;
    const uint8_t *crc_ptr = payload + payload_len;

    uint32_t expected_crc = (uint32_t)crc_ptr[0] |
                            ((uint32_t)crc_ptr[1] << 8) |
                            ((uint32_t)crc_ptr[2] << 16) |
                            ((uint32_t)crc_ptr[3] << 24);
    uint32_t calc_crc = esp_rom_crc32_le(0, payload, payload_len);
    if (calc_crc != expected_crc) {
        ESP_LOGE(TAG, "Payload CRC mismatch (expected=0x%08" PRIX32 " got=0x%08" PRIX32 ")",
                 expected_crc, calc_crc);
        return ESP_ERR_INVALID_CRC;
    }

    uint32_t led_count = header.base.led_count;
    if (led_count != LED_COUNT_PER_CH) {
        ESP_LOGE(TAG, "Unsupported LED count %u (expected %d)", led_count, LED_COUNT_PER_CH);
        return ESP_ERR_INVALID_SIZE;
    }

    uint32_t frame_count = header.base.frame_count;
    if (frame_count == 0) {
        ESP_LOGE(TAG, "Pattern has zero frames");
        return ESP_ERR_INVALID_SIZE;
    }

    size_t frame_stride = LED_FRAME_SIZE_CH;
    size_t frames_size = (size_t)frame_count * frame_stride;
    uint8_t *frames_buf = (uint8_t *)malloc(frames_size);
    if (!frames_buf) {
        ESP_LOGE(TAG, "Failed to allocate frame buffer (%zu bytes)", frames_size);
        return ESP_ERR_NO_MEM;
    }

    uint8_t *decoded = (uint8_t *)malloc(led_count);
    uint8_t *prev = (uint8_t *)calloc(led_count, 1);
    uint8_t *rle_buf = (uint8_t *)malloc(led_count);
    if (!decoded || !prev || !rle_buf) {
        free(frames_buf);
        if (decoded) free(decoded);
        if (prev) free(prev);
        if (rle_buf) free(rle_buf);
        return ESP_ERR_NO_MEM;
    }

    bool prev_valid = false;
    const uint8_t *cursor = payload;
    const uint8_t *end = payload + payload_len;

    if (cursor + 2 > end) {
        free(frames_buf);
        free(decoded);
        free(prev);
        free(rle_buf);
        return ESP_ERR_INVALID_SIZE;
    }

    uint16_t palette_entries = (uint16_t)(cursor[0] | (cursor[1] << 8));
    cursor += 2;
    if (palette_entries == 0 || palette_entries > PRISM_MAX_PALETTE) {
        free(frames_buf);
        free(decoded);
        free(prev);
        free(rle_buf);
        ESP_LOGE(TAG, "Invalid palette size %u", palette_entries);
        return ESP_ERR_INVALID_SIZE;
    }

    if (cursor + palette_entries * 3 > end) {
        free(frames_buf);
        free(decoded);
        free(prev);
        free(rle_buf);
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t palette_grb[PRISM_MAX_PALETTE * 3];
    for (uint16_t i = 0; i < palette_entries; ++i) {
        uint8_t r = cursor[i * 3 + 0];
        uint8_t g = cursor[i * 3 + 1];
        uint8_t b = cursor[i * 3 + 2];
        palette_grb[i * 3 + 0] = g;
        palette_grb[i * 3 + 1] = r;
        palette_grb[i * 3 + 2] = b;
    }
    cursor += palette_entries * 3;

    for (uint32_t frame_idx = 0; frame_idx < frame_count; ++frame_idx) {
        if (cursor + 3 > end) {
            ret = ESP_ERR_INVALID_SIZE;
            break;
        }

        uint8_t flags = cursor[0];
        uint16_t segment_len = (uint16_t)(cursor[1] | (cursor[2] << 8));
        cursor += 3;

        if (cursor + segment_len > end) {
            ret = ESP_ERR_INVALID_SIZE;
            break;
        }

        const uint8_t *segment = cursor;
        cursor += segment_len;

        size_t produced = 0;
        if (flags & PRISM_FLAG_RLE) {
            size_t out_idx = 0;
            size_t pos = 0;
            while (pos < segment_len && out_idx < led_count) {
                uint8_t value = segment[pos++];
                if (value & PRISM_RLE_MARK) {
                    uint8_t run_len = value & PRISM_RLE_MASK;
                    if (pos >= segment_len) {
                        ret = ESP_ERR_INVALID_SIZE;
                        break;
                    }
                    uint8_t run_val = segment[pos++];
                    for (uint8_t c = 0; c < run_len && out_idx < led_count; ++c) {
                        rle_buf[out_idx++] = run_val;
                    }
                } else {
                    rle_buf[out_idx++] = value;
                }
            }
            if (ret != ESP_OK) {
                break;
            }
            if (out_idx != led_count) {
                ret = ESP_ERR_INVALID_SIZE;
                break;
            }
            memcpy(decoded, rle_buf, led_count);
            produced = out_idx;
        } else {
            if (segment_len < led_count) {
                ret = ESP_ERR_INVALID_SIZE;
                break;
            }
            memcpy(decoded, segment, led_count);
            produced = led_count;
        }

        if (produced != led_count) {
            ret = ESP_ERR_INVALID_SIZE;
            break;
        }

        if (flags & PRISM_FLAG_DELTA) {
            if (!prev_valid) {
                ret = ESP_ERR_INVALID_STATE;
                break;
            }
            for (uint32_t i = 0; i < led_count; ++i) {
                decoded[i] ^= prev[i];
            }
        }

        memcpy(prev, decoded, led_count);
        prev_valid = true;

        uint8_t *dst = frames_buf + frame_idx * frame_stride;
        for (uint32_t i = 0; i < led_count; ++i) {
            uint8_t idx = decoded[i];
            if (idx >= palette_entries) {
                ret = ESP_ERR_INVALID_SIZE;
                break;
            }
            const uint8_t *grb = &palette_grb[idx * 3];
            dst[i * 3 + 0] = grb[0];
            dst[i * 3 + 1] = grb[1];
            dst[i * 3 + 2] = grb[2];
        }
        if (ret != ESP_OK) {
            break;
        }
    }

    free(decoded);
    free(prev);
    free(rle_buf);

    if (ret != ESP_OK) {
        free(frames_buf);
        return ret;
    }

    ret = led_driver_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        free(frames_buf);
        ESP_LOGE(TAG, "LED driver init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = led_driver_start();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        free(frames_buf);
        ESP_LOGE(TAG, "LED driver start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    playback_free_pattern();
    s_pattern.frames = frames_buf;
    s_pattern.frame_stride = frame_stride;
    s_pattern.frame_count = frame_count;
    s_pattern.current_frame = 0;
    s_pattern.led_count = led_count;
    s_pattern.last_frame_us = 0;
    s_pattern.header = header;
    s_pattern.loaded = true;
    if (pattern_id && pattern_id[0]) {
        strlcpy(s_pattern.id, pattern_id, sizeof(s_pattern.id));
    } else {
        s_pattern.id[0] = '\0';
    }

    double fps = (header.base.fps > 0) ? ((double)header.base.fps / 256.0) : LED_FPS_TARGET;
    if (fps <= 0.0) {
        fps = LED_FPS_TARGET;
    }
    uint32_t interval_us = (uint32_t)(1000000.0 / fps);
    if (interval_us == 0) {
        interval_us = 1000000 / LED_FPS_TARGET;
    }
    s_pattern.frame_interval_us = interval_us;

    playback_start_pattern(&header.meta);
    s_pb.running = true;
    s_pb.source = PLAYBACK_SOURCE_PATTERN;
    s_pb.frame_counter = 0;
    s_last_fx_tick_us = 0;

    ESP_LOGI(TAG, "Pattern playback started: id='%s' frames=%u fps=%.2f interval_us=%u",
             s_pattern.id, frame_count, fps, s_pattern.frame_interval_us);
    return ESP_OK;
}

esp_err_t playback_play_pattern_from_storage(const char *pattern_id)
{
    if (!pattern_id || pattern_id[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t *buffer = (uint8_t *)malloc(PATTERN_MAX_SIZE);
    if (!buffer) {
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read = 0;
    esp_err_t ret = storage_pattern_read(pattern_id, buffer, PATTERN_MAX_SIZE, &bytes_read);
    if (ret != ESP_OK) {
        free(buffer);
        return ret;
    }

    ret = playback_play_prism_blob(pattern_id, buffer, bytes_read);
    free(buffer);
    return ret;
}

esp_err_t playback_stop(void)
{
    if (!s_pb.running) {
        return ESP_OK;
    }
    playback_free_pattern();
    s_pb.running = false;
    s_pb.source = PLAYBACK_SOURCE_NONE;
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

esp_err_t playback_set_brightness(uint8_t target, uint32_t duration_ms)
{
    // Ensure engine initialized
    effect_engine_init();
    // If brightness not yet added, add it with current=target then no-op ramp
    effect_add_brightness(target);
    effect_brightness_set_target(target, duration_ms);
    return ESP_OK;
}
