/**
 * @file frame_generators.c
 * @brief Synthetic frame generators used by the decode benchmark.
 */

#include "bench_generators.h"
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void fill_palette(uint8_t *palette_buf, size_t palette_len)
{
    // Generate a simple gradient palette (RGB triplets).
    size_t entries = palette_len / 3;
    for (size_t i = 0; i < entries; ++i) {
        palette_buf[i * 3 + 0] = (uint8_t)((i * 13u) & 0xFF);
        palette_buf[i * 3 + 1] = (uint8_t)((i * 29u) & 0xFF);
        palette_buf[i * 3 + 2] = (uint8_t)((i * 47u) & 0xFF);
    }
}

static void emit_palette_indices(bench_generator_state_t *state,
                                 uint8_t *src_buf,
                                 size_t src_size,
                                 uint8_t *scratch_buf,
                                 size_t scratch_size,
                                 bench_frame_desc_t *desc)
{
    const size_t palette_entries = MIN(32u, scratch_size / 3u);
    const size_t led_count = state->led_count;
    if (palette_entries == 0 || src_size < led_count) {
        memset(desc, 0, sizeof(*desc));
        return;
    }

    for (size_t i = 0; i < led_count; ++i) {
        src_buf[i] = (uint8_t)((i + state->frame_counter) % palette_entries);
    }

    fill_palette(scratch_buf, palette_entries * 3u);

    desc->pattern = BENCH_PATTERN_PALETTE;
    desc->index_buf = src_buf;
    desc->index_len = led_count;
    desc->palette_buf = scratch_buf;
    desc->palette_len = palette_entries * 3u;
    desc->delta_buf = NULL;
    desc->delta_len = 0;
    desc->rle_buf = NULL;
    desc->rle_len = 0;
    desc->bytes_touched = desc->index_len + desc->palette_len;
}

static void emit_xor_delta(bench_generator_state_t *state,
                           uint8_t *src_buf,
                           size_t src_size,
                           bench_frame_desc_t *desc)
{
    const size_t bytes_needed = state->led_count * 3u;
    if (src_size < bytes_needed) {
        memset(desc, 0, sizeof(*desc));
        return;
    }

    // Produce a deterministic XOR mask (high-entropy regions toggled).
    uint8_t seed = state->prev_mask_seed + 0x3D;
    for (size_t i = 0; i < bytes_needed; ++i) {
        seed ^= (uint8_t)(i * 17u + state->frame_counter);
        src_buf[i] = (uint8_t)(seed & 0x0F); // small delta to stay within realistic bounds
    }
    state->prev_mask_seed = seed;

    desc->pattern = BENCH_PATTERN_XOR_DELTA;
    desc->index_buf = NULL;
    desc->index_len = 0;
    desc->palette_buf = NULL;
    desc->palette_len = 0;
    desc->delta_buf = src_buf;
    desc->delta_len = bytes_needed;
    desc->rle_buf = NULL;
    desc->rle_len = 0;
    desc->bytes_touched = desc->delta_len;
}

static void emit_rle_stream(bench_generator_state_t *state,
                            uint8_t *scratch_buf,
                            size_t scratch_size,
                            bench_frame_desc_t *desc)
{
    const size_t required_pairs = (state->led_count * 3u + 7u) / 8u; // runs of 8 bytes
    const size_t bytes_needed = required_pairs * 2u;
    if (scratch_size < bytes_needed) {
        memset(desc, 0, sizeof(*desc));
        return;
    }

    uint8_t value = (uint8_t)(state->frame_counter & 0xFF);
    for (size_t i = 0; i < required_pairs; ++i) {
        scratch_buf[i * 2u + 0] = value;
        scratch_buf[i * 2u + 1] = 8; // fixed run length
        value = (uint8_t)(value + 23u);
    }

    desc->pattern = BENCH_PATTERN_RLE;
    desc->index_buf = NULL;
    desc->index_len = 0;
    desc->palette_buf = NULL;
    desc->palette_len = 0;
    desc->delta_buf = NULL;
    desc->delta_len = 0;
    desc->rle_buf = scratch_buf;
    desc->rle_len = bytes_needed;
    desc->bytes_touched = desc->rle_len;
}

void bench_generator_init(bench_generator_state_t *state,
                          bench_pattern_t pattern,
                          uint32_t led_count)
{
    if (!state) return;
    state->pattern = pattern;
    state->frame_counter = 0;
    state->led_count = led_count;
    state->prev_mask_seed = 0x5A;
}

void bench_generator_emit(bench_generator_state_t *state,
                          uint8_t *src_buf, size_t src_size,
                          uint8_t *scratch_buf, size_t scratch_size,
                          bench_frame_desc_t *out_desc)
{
    if (!state || !out_desc) return;

    bench_frame_desc_t tmp = {0};
    switch (state->pattern) {
    case BENCH_PATTERN_PALETTE:
        emit_palette_indices(state, src_buf, src_size, scratch_buf, scratch_size, &tmp);
        break;
    case BENCH_PATTERN_XOR_DELTA:
        emit_xor_delta(state, src_buf, src_size, &tmp);
        break;
    case BENCH_PATTERN_RLE:
        emit_rle_stream(state, scratch_buf, scratch_size, &tmp);
        break;
    }

    *out_desc = tmp;
    state->frame_counter++;
}

