/**
 * @file bench_generators.h
 * @brief Synthetic frame generators for the decode benchmark.
 */

#ifndef PRISM_BENCH_GENERATORS_H
#define PRISM_BENCH_GENERATORS_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    BENCH_PATTERN_PALETTE = 0,
    BENCH_PATTERN_XOR_DELTA = 1,
    BENCH_PATTERN_RLE = 2,
} bench_pattern_t;

typedef struct {
    bench_pattern_t pattern;
    const uint8_t *index_buf;
    size_t index_len;
    const uint8_t *palette_buf;
    size_t palette_len;
    const uint8_t *delta_buf;
    size_t delta_len;
    const uint8_t *rle_buf;
    size_t rle_len;
    size_t bytes_touched; /**< Total source bytes consumed (excluding output). */
} bench_frame_desc_t;

typedef struct {
    bench_pattern_t pattern;
    uint32_t frame_counter;
    uint32_t led_count;
    uint8_t prev_mask_seed;
} bench_generator_state_t;

void bench_generator_init(bench_generator_state_t *state,
                          bench_pattern_t pattern,
                          uint32_t led_count);

void bench_generator_emit(bench_generator_state_t *state,
                          uint8_t *src_buf, size_t src_size,
                          uint8_t *scratch_buf, size_t scratch_size,
                          bench_frame_desc_t *out_desc);

#endif // PRISM_BENCH_GENERATORS_H

