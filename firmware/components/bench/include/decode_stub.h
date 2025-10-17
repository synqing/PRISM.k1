/**
 * @file decode_stub.h
 * @brief Stubbed decode implementation used by the benchmark.
 */

#ifndef PRISM_BENCH_DECODE_STUB_H
#define PRISM_BENCH_DECODE_STUB_H

#include "bench_generators.h"

typedef struct {
    uint32_t led_count;
    uint8_t last_frame[160u * 3u];
    int has_last;
} bench_decode_state_t;

void bench_decode_state_init(bench_decode_state_t *state, uint32_t led_count);

size_t bench_decode_apply(bench_decode_state_t *state,
                          const bench_frame_desc_t *desc,
                          uint8_t *dst,
                          size_t dst_len);

#endif // PRISM_BENCH_DECODE_STUB_H

