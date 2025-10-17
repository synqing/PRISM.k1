/**
 * @file bench_decode.c
 * @brief ESP32-S3 decode benchmark harness (120 FPS, cycles/us, ≤4KB working set)
 */

#include "bench_decode.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_cpu.h"
#include "esp_heap_caps.h"
#include "esp_rom_sys.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <strings.h>

#include "prism_decode_hooks.h"
#include "bench_generators.h"
#include "decode_stub.h"

#if CONFIG_PRISM_BENCH_REGISTER_CLI
#include "esp_console.h"
#endif
static const char *TAG = "bench_decode";

#ifndef CONFIG_PRISM_BENCH_FRAMES
#define CONFIG_PRISM_BENCH_FRAMES 480
#endif
#ifndef CONFIG_PRISM_BENCH_LED_COUNT
#define CONFIG_PRISM_BENCH_LED_COUNT 160
#endif
#ifndef CONFIG_PRISM_BENCH_PERIOD_US
#define CONFIG_PRISM_BENCH_PERIOD_US 8333
#endif

#if CONFIG_PRISM_BENCH_PATTERN_XOR_DELTA
#define BENCH_DEFAULT_PATTERN BENCH_PATTERN_XOR_DELTA
#elif CONFIG_PRISM_BENCH_PATTERN_RLE
#define BENCH_DEFAULT_PATTERN BENCH_PATTERN_RLE
#else
#define BENCH_DEFAULT_PATTERN BENCH_PATTERN_PALETTE
#endif

typedef struct {
    uint32_t cycles;
    uint32_t us;
    uint32_t bytes;
} bench_sample_t;

typedef struct {
    // configuration
    uint32_t frames_target;
    uint32_t led_count;
    uint32_t period_us;

    // runtime
    esp_timer_handle_t timer;
    volatile uint32_t frame_idx;
    bench_sample_t *samples; // allocated once (non-critical path)

    // decode working set (≤4KB)
    struct {
        uint8_t src[1024];       // synthetic input (1KB)
        uint8_t out_rgb[160*3];  // 480 bytes at 160 LEDs (sized at runtime)
        uint8_t scratch[4096 - 1024 - 480]; // remaining scratch
    } arena;

    bench_generator_state_t gen_state;
    bench_decode_state_t decode_state;
    bench_pattern_t active_pattern;
    bool write_file;

    // heap snapshots
    size_t free_heap_before;
    size_t min_free_before;
    size_t free_heap_after;
    size_t min_free_after;
} bench_ctx_t;

static bench_ctx_t s_ctx;

static int cmp_u32(const void *a, const void *b) {
    uint32_t ua = *(const uint32_t*)a;
    uint32_t ub = *(const uint32_t*)b;
    if (ua < ub) {
        return -1;
    }
    if (ua > ub) {
        return 1;
    }
    return 0;
}

static void compute_and_emit_summary(void)
{
    uint32_t n = s_ctx.frame_idx;
    if (n == 0) n = 1; // avoid div-by-zero

    // Collect arrays for us and cycles
    uint32_t *usv = (uint32_t*)heap_caps_malloc(n * sizeof(uint32_t), MALLOC_CAP_8BIT);
    uint32_t *cycv = (uint32_t*)heap_caps_malloc(n * sizeof(uint32_t), MALLOC_CAP_8BIT);
    uint64_t sum_us = 0, sum_cycles = 0, sum_bytes = 0;
    for (uint32_t i = 0; i < n; ++i) {
        usv[i] = s_ctx.samples[i].us;
        cycv[i] = s_ctx.samples[i].cycles;
        sum_us += s_ctx.samples[i].us;
        sum_cycles += s_ctx.samples[i].cycles;
        sum_bytes += s_ctx.samples[i].bytes;
    }
    qsort(usv, n, sizeof(uint32_t), cmp_u32);
    qsort(cycv, n, sizeof(uint32_t), cmp_u32);
    uint32_t p99_idx = (n >= 100) ? (n * 99 / 100) : (n - 1);
    uint32_t avg_us = (uint32_t)(sum_us / n);
    uint32_t avg_cycles = (uint32_t)(sum_cycles / n);
    uint32_t p99_us = usv[p99_idx];
    uint32_t p99_cycles = cycv[p99_idx];
    uint32_t max_us = usv[n - 1];

    s_ctx.free_heap_after = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    s_ctx.min_free_after  = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);

    // JSON to UART (line-delimited)
    ESP_LOGI(TAG,
             "{\"bench\":\"decode\",\"frames\":%u,\"leds\":%u,\"period_us\":%u,"
             "\"avg_us\":%u,\"p99_us\":%u,\"max_us\":%u,\"avg_cycles\":%u,\"p99_cycles\":%u,"
             "\"bytes_total\":%llu,\"workset_bytes\":%u,\"heap_free_before\":%u,\"heap_free_after\":%u,"
             "\"heap_min_before\":%u,\"heap_min_after\":%u}",
             n, s_ctx.led_count, s_ctx.period_us,
             avg_us, p99_us, max_us, avg_cycles, p99_cycles,
             (unsigned long long)sum_bytes, (unsigned)sizeof(s_ctx.arena),
             (unsigned)s_ctx.free_heap_before, (unsigned)s_ctx.free_heap_after,
             (unsigned)s_ctx.min_free_before, (unsigned)s_ctx.min_free_after);

#if CONFIG_PRISM_BENCH_ENABLE_FILE
    if (s_ctx.write_file) {
        const char *path = CONFIG_PRISM_BENCH_FILE_PATH;
        FILE *f = fopen(path, "a");
        if (f) {
            fprintf(f,
                    "{\"bench\":\"decode\",\"frames\":%u,\"leds\":%u,\"period_us\":%u,"
                    "\"avg_us\":%u,\"p99_us\":%u,\"max_us\":%u,\"avg_cycles\":%u,\"p99_cycles\":%u,"
                    "\"bytes_total\":%llu,\"workset_bytes\":%u,\"heap_free_before\":%u,\"heap_free_after\":%u,"
                    "\"heap_min_before\":%u,\"heap_min_after\":%u}\n",
                    n, s_ctx.led_count, s_ctx.period_us,
                    avg_us, p99_us, max_us, avg_cycles, p99_cycles,
                    (unsigned long long)sum_bytes, (unsigned)sizeof(s_ctx.arena),
                    (unsigned)s_ctx.free_heap_before, (unsigned)s_ctx.free_heap_after,
                    (unsigned)s_ctx.min_free_before, (unsigned)s_ctx.min_free_after);
            fclose(f);
        } else {
            ESP_LOGW(TAG, "Failed to open metrics file: %s", path);
        }
    }
#endif

    free(usv);
    free(cycv);
}

static void IRAM_ATTR timer_cb(void *arg)
{
    (void)arg;
    // Capture per-frame heap (read-only) to detect heap drift in hot path
    size_t heap_before = heap_caps_get_free_size(MALLOC_CAP_8BIT);

    bench_frame_desc_t desc = {0};
    bench_generator_emit(&s_ctx.gen_state,
                         s_ctx.arena.src, sizeof(s_ctx.arena.src),
                         s_ctx.arena.scratch, sizeof(s_ctx.arena.scratch),
                         &desc);

    prism_decode_hook_ctx_t hctx;
    prism_decode_begin(&hctx);

    size_t nbytes = (size_t)s_ctx.led_count * 3u;
    size_t produced = bench_decode_apply(&s_ctx.decode_state, &desc,
                                         s_ctx.arena.out_rgb, nbytes);

    uint32_t cycles = 0, us = 0;
    prism_decode_end(&hctx, &cycles, &us);

    // Assert no per-frame heap drift
    size_t heap_after = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    if (heap_before != heap_after) {
        // Log minimal info inside callback; full summary prints at end
        ESP_EARLY_LOGE(TAG, "Heap drift detected in callback: before=%u after=%u",
                       (unsigned)heap_before, (unsigned)heap_after);
    }

    uint32_t i = s_ctx.frame_idx;
    if (i < s_ctx.frames_target) {
        s_ctx.samples[i].cycles = cycles;
        s_ctx.samples[i].us     = us;
        s_ctx.samples[i].bytes  = (uint32_t)(desc.bytes_touched + produced);
        s_ctx.frame_idx = i + 1;
    }

    // Stop when done
    if (s_ctx.frame_idx >= s_ctx.frames_target) {
        esp_timer_stop(s_ctx.timer);
    }
}

static esp_err_t bench_setup(uint32_t frames,
                             uint32_t leds,
                             uint32_t period_us,
                             bench_pattern_t pattern,
                             bool enable_file)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.frames_target = frames;
    if (leds > 160) {
        ESP_LOGW(TAG, "LED count %u exceeds arena capacity (160); clamping.", (unsigned)leds);
        leds = 160;
    }
    s_ctx.led_count = leds;
    s_ctx.period_us = period_us;
    s_ctx.active_pattern = pattern;
    s_ctx.write_file = enable_file;

    // Fill synthetic src with deterministic pattern
    for (size_t i = 0; i < sizeof(s_ctx.arena.src); ++i) {
        s_ctx.arena.src[i] = (uint8_t)((i * 1103515245u) >> 24);
    }

    s_ctx.samples = (bench_sample_t*)heap_caps_malloc(sizeof(bench_sample_t) * frames, MALLOC_CAP_8BIT);
    if (!s_ctx.samples) return ESP_ERR_NO_MEM;

    bench_generator_init(&s_ctx.gen_state, pattern, leds);
    bench_decode_state_init(&s_ctx.decode_state, leds);

    const esp_timer_create_args_t args = {
        .callback = &timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "bench_dec"
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &s_ctx.timer));

    s_ctx.free_heap_before = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    s_ctx.min_free_before  = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    return ESP_OK;
}

static void bench_teardown(void)
{
    if (s_ctx.timer) {
        esp_timer_stop(s_ctx.timer);
        esp_timer_delete(s_ctx.timer);
        s_ctx.timer = NULL;
    }
    if (s_ctx.samples) {
        free(s_ctx.samples);
        s_ctx.samples = NULL;
    }
}

#if CONFIG_PRISM_BENCH_REGISTER_CLI
static const char *bench_pattern_name(bench_pattern_t pattern)
{
    switch (pattern) {
    case BENCH_PATTERN_PALETTE: return "palette";
    case BENCH_PATTERN_XOR_DELTA: return "xor";
    case BENCH_PATTERN_RLE: return "rle";
    default: return "unknown";
    }
}

static bench_pattern_t bench_parse_pattern(const char *value, bench_pattern_t fallback)
{
    if (!value) return fallback;
    if (strcasecmp(value, "palette") == 0) return BENCH_PATTERN_PALETTE;
    if (strcasecmp(value, "xor") == 0 || strcasecmp(value, "xor_delta") == 0) return BENCH_PATTERN_XOR_DELTA;
    if (strcasecmp(value, "rle") == 0) return BENCH_PATTERN_RLE;
    return fallback;
}
#else
static const char *bench_pattern_name(bench_pattern_t pattern)
{
    switch (pattern) {
    case BENCH_PATTERN_PALETTE: return "palette";
    case BENCH_PATTERN_XOR_DELTA: return "xor";
    case BENCH_PATTERN_RLE: return "rle";
    default: return "unknown";
    }
}
#endif

static esp_err_t bench_run(uint32_t frames,
                           uint32_t leds,
                           uint32_t period_us,
                           bench_pattern_t pattern,
                           bool enable_file)
{
    ESP_LOGI(TAG, "Decode bench: pattern=%s frames=%u leds=%u period_us=%u file=%s",
             bench_pattern_name(pattern), (unsigned)frames, (unsigned)leds,
             (unsigned)period_us, enable_file ? "on" : "off");

    esp_err_t err = bench_setup(frames, leds, period_us, pattern, enable_file);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bench_setup failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_timer_start_periodic(s_ctx.timer, s_ctx.period_us);
    if (err != ESP_OK) {
        bench_teardown();
        ESP_LOGE(TAG, "Failed to start timer: %s", esp_err_to_name(err));
        return err;
    }

    while (s_ctx.frame_idx < s_ctx.frames_target) {
        esp_rom_delay_us(1000); // 1 ms sleep
    }

    compute_and_emit_summary();
    bench_teardown();
    return ESP_OK;
}

#if CONFIG_PRISM_BENCH_REGISTER_CLI
static int bench_cli_cmd(int argc, char **argv)
{
    uint32_t frames = CONFIG_PRISM_BENCH_FRAMES;
    uint32_t leds = CONFIG_PRISM_BENCH_LED_COUNT;
    uint32_t period_us = CONFIG_PRISM_BENCH_PERIOD_US;
    bench_pattern_t pattern = BENCH_DEFAULT_PATTERN;
#if CONFIG_PRISM_BENCH_ENABLE_FILE
    bool enable_file = true;
#else
    bool enable_file = false;
#endif

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (strncasecmp(arg, "pattern=", 8) == 0) {
            pattern = bench_parse_pattern(arg + 8, pattern);
        } else if (strncasecmp(arg, "frames=", 7) == 0) {
            frames = (uint32_t)strtoul(arg + 7, NULL, 0);
        } else if (strncasecmp(arg, "leds=", 5) == 0) {
            leds = (uint32_t)strtoul(arg + 5, NULL, 0);
        } else if (strncasecmp(arg, "period_us=", 10) == 0) {
            period_us = (uint32_t)strtoul(arg + 10, NULL, 0);
#if CONFIG_PRISM_BENCH_ENABLE_FILE
        } else if (strncasecmp(arg, "file=", 5) == 0) {
            if (strcasecmp(arg + 5, "on") == 0) {
                enable_file = true;
            } else if (strcasecmp(arg + 5, "off") == 0) {
                enable_file = false;
            }
#endif
        }
    }

    esp_err_t err = bench_run(frames, leds, period_us, pattern, enable_file);
    return (err == ESP_OK) ? 0 : -1;
}

static void bench_register_cli(void)
{
    static bool registered = false;
    if (registered) {
        return;
    }
    const esp_console_cmd_t cmd = {
        .command = "bench_decode",
        .help = "Run PRISM decode benchmark. Args: pattern=<palette|xor|rle> frames=<n> leds=<n> period_us=<us> [file=on|off]",
        .hint = NULL,
        .func = &bench_cli_cmd,
    };
    if (esp_console_cmd_register(&cmd) == ESP_OK) {
        registered = true;
        ESP_LOGI(TAG, "Registered 'bench_decode' CLI command");
    }
}
#endif

#if CONFIG_PRISM_BENCH_REGISTER_CLI
#define BENCH_AUTO_REGISTER_CLI() bench_register_cli()
#else
#define BENCH_AUTO_REGISTER_CLI() ((void)0)
#endif

#if CONFIG_PRISM_BENCH_ENABLE_FILE
#define BENCH_DEFAULT_FILE true
#else
#define BENCH_DEFAULT_FILE false
#endif

esp_err_t bench_decode_run(void)
{
    BENCH_AUTO_REGISTER_CLI();
    return bench_run(CONFIG_PRISM_BENCH_FRAMES,
                     CONFIG_PRISM_BENCH_LED_COUNT,
                     CONFIG_PRISM_BENCH_PERIOD_US,
                     BENCH_DEFAULT_PATTERN,
                     BENCH_DEFAULT_FILE);
}

/* Autorun handled from app_main() under CONFIG_PRISM_BENCH_AUTORUN */
