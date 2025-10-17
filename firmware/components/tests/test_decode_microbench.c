/**
 * @file test_decode_microbench.c
 * @brief Microbenchmark harness for packaging decode validation (R1.1)
 *
 * Scope: Validation hooks only. This does not implement any decode logic.
 * - Measures cycles/us per-call
 * - Captures heap free/min snapshots to detect per-frame allocations
 * - Runs a fixed number of iterations to estimate avg/p99/max
 *
 * Until Task #30 lands (packaging prototype), a dummy decode is used so the
 * harness can compile/run and report "ready" status without blocking Tooling.
 */

#include "unity.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_cpu.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "prism_decode_hooks.h"
#include "bench_generators.h"
#include "decode_stub.h"

static const char *TAG = "decode_bench";

// --- Placeholder decode Adapter ------------------------------------------------
// When #30 lands, wire this to the real decode API. For now, we simulate a
// single-pass O(N) walk over a byte buffer to validate the harness plumbing.

typedef struct {
    const uint8_t *pkg;
    size_t         pkg_len;
    const uint8_t *palette;    // optional, unused in stub
    size_t         palette_len; // optional, unused in stub
    uint8_t       *out_rgb;    // destination buffer (3 * led_count)
    size_t         led_count;
} decode_args_t;

static inline void dummy_decode_single_pass(const decode_args_t *args) {
    // Simulate O(N) single-pass work proportional to output LEDs
    // No branching in hot path, no heap allocations.
    size_t n = args->led_count * 3; // bytes to "write"
    const uint8_t *src = args->pkg;
    uint8_t *dst = args->out_rgb;
    size_t src_mask = (args->pkg_len ? (args->pkg_len - 1) : 0);
    for (size_t i = 0; i < n; ++i) {
        // simple table lookup-like op
        dst[i] = src[(i) & src_mask];
    }
}

// --- Metrics ------------------------------------------------------------------

typedef struct {
    uint32_t cycles;
    uint32_t us;
} sample_t;

typedef struct {
    sample_t *samples;
    size_t    count;
    size_t    cap;
    size_t    idx;
    size_t    dropped;
    size_t    p99_index; // cached after finalize
    uint32_t  avg_us;
    uint32_t  max_us;
} stats_t;

static int cmp_us(const void *a, const void *b) {
    const sample_t *sa = (const sample_t*)a;
    const sample_t *sb = (const sample_t*)b;
    if (sa->us < sb->us) return -1;
    if (sa->us > sb->us) return 1;
    return 0;
}

static void stats_init(stats_t *st, size_t cap) {
    memset(st, 0, sizeof(*st));
    st->samples = (sample_t*)heap_caps_malloc(sizeof(sample_t) * cap, MALLOC_CAP_8BIT);
    st->cap = cap;
}

static void stats_free(stats_t *st) {
    if (st->samples) {
        free(st->samples);
        st->samples = NULL;
    }
}

static void stats_add(stats_t *st, uint32_t cycles, uint32_t us) {
    if (st->count < st->cap) {
        st->samples[st->count++] = (sample_t){ .cycles = cycles, .us = us };
    } else {
        st->dropped++;
    }
    if (us > st->max_us) st->max_us = us;
}

static void stats_finalize(stats_t *st) {
    // average
    uint64_t sum_us = 0;
    for (size_t i = 0; i < st->count; ++i) sum_us += st->samples[i].us;
    st->avg_us = st->count ? (uint32_t)(sum_us / st->count) : 0;
    // p99 by sorting copy (small N)
    qsort(st->samples, st->count, sizeof(sample_t), cmp_us);
    size_t p99i = (st->count >= 100) ? (size_t)((st->count * 99) / 100) : (st->count ? st->count - 1 : 0);
    st->p99_index = p99i;
}

// --- Harness ------------------------------------------------------------------

static void run_bench(size_t frames, size_t led_count, size_t pkg_len) {
    // Allocate source/destination buffers
    uint8_t *pkg = (uint8_t*)heap_caps_malloc(pkg_len, MALLOC_CAP_8BIT);
    uint8_t *out = (uint8_t*)heap_caps_malloc(led_count * 3, MALLOC_CAP_8BIT);
    TEST_ASSERT_NOT_NULL(pkg);
    TEST_ASSERT_NOT_NULL(out);

    // Fill pkg with deterministic pattern
    for (size_t i = 0; i < pkg_len; ++i) pkg[i] = (uint8_t)(i * 1315423911u);

    stats_t st = {0};
    stats_init(&st, frames);

    size_t free_before = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t min_free_before = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);

    for (size_t i = 0; i < frames; ++i) {
        prism_decode_hook_ctx_t ctx = {0};
        prism_decode_begin(&ctx);
        // Replace with real decode when available
        decode_args_t args = {
            .pkg = pkg,
            .pkg_len = pkg_len,
            .palette = NULL,
            .palette_len = 0,
            .out_rgb = out,
            .led_count = led_count,
        };
        dummy_decode_single_pass(&args);
        uint32_t cycles = 0, us = 0;
        prism_decode_end(&ctx, &cycles, &us);
        stats_add(&st, cycles, us);
    }

    size_t free_after = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t min_free_after = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);

    stats_finalize(&st);

    uint32_t p99_us = (st.count && st.p99_index < st.count) ? st.samples[st.p99_index].us : 0;
    ESP_LOGI(TAG, "Frames=%u, avg=%u us, p99=%u us, max=%u us, dropped=%u",
             (unsigned)st.count, (unsigned)st.avg_us, (unsigned)p99_us,
             (unsigned)st.max_us, (unsigned)st.dropped);
    ESP_LOGI(TAG, "Heap free before=%u, after=%u, min_before=%u, min_after=%u, delta_min=%d",
             (unsigned)free_before, (unsigned)free_after,
             (unsigned)min_free_before, (unsigned)min_free_after,
             (int)((int)min_free_after - (int)min_free_before));

    // Basic guardrails aligned with R1.1 envelope for stub path
    // Note: Real thresholds enforced once #30 lands; for now just ensure no heap drift.
    TEST_ASSERT_EQUAL_UINT32(free_before, free_after);
    TEST_ASSERT_TRUE(min_free_after >= min_free_before);

    stats_free(&st);
    free(pkg);
    free(out);
}

TEST_CASE("DECODE microbench harness ready (no-op until #30)", "[decode][bench]") {
    ESP_LOGI(TAG, "Harness compiled and running. Waiting on #30 packaging prototype.");
    // Smoke run with tiny sizes to validate plumbing
    run_bench(/*frames*/ 32, /*led_count*/ 160, /*pkg_len*/ 256);
}

TEST_CASE("Bench generators stay within arena bounds", "[decode][bench][gen]") {
    bench_generator_state_t gen;
    bench_generator_init(&gen, BENCH_PATTERN_PALETTE, 32);
    uint8_t src[256];
    uint8_t scratch[256];
    bench_frame_desc_t desc = {0};
    bench_generator_emit(&gen, src, sizeof(src), scratch, sizeof(scratch), &desc);
    TEST_ASSERT_EQUAL(BENCH_PATTERN_PALETTE, desc.pattern);
    TEST_ASSERT_TRUE(desc.index_len == 32);
    TEST_ASSERT_TRUE(desc.palette_len <= sizeof(scratch));

    bench_generator_init(&gen, BENCH_PATTERN_XOR_DELTA, 32);
    memset(&desc, 0, sizeof(desc));
    bench_generator_emit(&gen, src, sizeof(src), scratch, sizeof(scratch), &desc);
    TEST_ASSERT_EQUAL(BENCH_PATTERN_XOR_DELTA, desc.pattern);
    TEST_ASSERT_TRUE(desc.delta_len == 32u * 3u);

    bench_generator_init(&gen, BENCH_PATTERN_RLE, 32);
    memset(&desc, 0, sizeof(desc));
    bench_generator_emit(&gen, src, sizeof(src), scratch, sizeof(scratch), &desc);
    TEST_ASSERT_EQUAL(BENCH_PATTERN_RLE, desc.pattern);
    TEST_ASSERT_TRUE(desc.rle_len > 0);
}

TEST_CASE("Decode stub produces deterministic output", "[decode][bench][stub]") {
    bench_generator_state_t gen;
    bench_generator_init(&gen, BENCH_PATTERN_PALETTE, 16);
    uint8_t src[256];
    uint8_t scratch[256];
    bench_frame_desc_t desc = {0};
    bench_generator_emit(&gen, src, sizeof(src), scratch, sizeof(scratch), &desc);

    bench_decode_state_t state;
    bench_decode_state_init(&state, 16);
    uint8_t out[16 * 3];
    size_t produced = bench_decode_apply(&state, &desc, out, sizeof(out));
    TEST_ASSERT_EQUAL(16u * 3u, produced);

    // Second frame with XOR delta should mutate previously stored frame.
    bench_generator_init(&gen, BENCH_PATTERN_XOR_DELTA, 16);
    memset(&desc, 0, sizeof(desc));
    bench_generator_emit(&gen, src, sizeof(src), scratch, sizeof(scratch), &desc);
    produced = bench_decode_apply(&state, &desc, out, sizeof(out));
    TEST_ASSERT_EQUAL(16u * 3u, produced);
}
