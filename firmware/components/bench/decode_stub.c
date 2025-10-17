/**
 * @file decode_stub.c
 * @brief Stub decode logic that mirrors expected packaging behaviour.
 */

#include "decode_stub.h"
#include <string.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static size_t palette_decode(const bench_frame_desc_t *desc,
                             uint8_t *dst,
                             size_t dst_len,
                             uint32_t led_count)
{
    if (!desc->index_buf || !desc->palette_buf || desc->palette_len < 3) {
        memset(dst, 0, dst_len);
        return MIN(dst_len, led_count * 3u);
    }
    const size_t palette_entries = desc->palette_len / 3u;
    const size_t limit = MIN((size_t)led_count, desc->index_len);
    size_t produced = MIN(dst_len, led_count * 3u);
    for (size_t i = 0; i < limit && (i * 3u + 2u) < produced; ++i) {
        const uint8_t idx = desc->index_buf[i] % palette_entries;
        const uint8_t *pal = &desc->palette_buf[idx * 3u];
        dst[i * 3u + 0u] = pal[0];
        dst[i * 3u + 1u] = pal[1];
        dst[i * 3u + 2u] = pal[2];
    }
    return produced;
}

static size_t xor_delta_decode(bench_decode_state_t *state,
                               const bench_frame_desc_t *desc,
                               uint8_t *dst,
                               size_t dst_len,
                               uint32_t led_count)
{
    const size_t bytes_needed = led_count * 3u;
    const size_t limit = MIN(bytes_needed, desc->delta_len);
    size_t produced = MIN(dst_len, bytes_needed);
    for (size_t i = 0; i < produced; ++i) {
        uint8_t base = state->has_last ? state->last_frame[i] : 0;
        uint8_t delta = (i < limit) ? desc->delta_buf[i] : 0;
        dst[i] = base ^ delta;
    }
    return produced;
}

static size_t rle_decode(const bench_frame_desc_t *desc,
                         uint8_t *dst,
                         size_t dst_len,
                         uint32_t led_count)
{
    size_t produced = MIN(dst_len, led_count * 3u);
    size_t out_idx = 0;
    for (size_t i = 0; i + 1 < desc->rle_len && out_idx < produced; i += 2) {
        uint8_t value = desc->rle_buf[i];
        uint8_t count = desc->rle_buf[i + 1];
        for (uint8_t c = 0; c < count && out_idx < produced; ++c) {
            dst[out_idx++] = (uint8_t)(value + c);
        }
    }
    // If we ran short, zero-fill remaining bytes.
    if (out_idx < produced) {
        memset(dst + out_idx, 0, produced - out_idx);
    }
    return produced;
}

void bench_decode_state_init(bench_decode_state_t *state, uint32_t led_count)
{
    if (!state) return;
    state->led_count = led_count;
    state->has_last = 0;
    memset(state->last_frame, 0, sizeof(state->last_frame));
}

size_t bench_decode_apply(bench_decode_state_t *state,
                          const bench_frame_desc_t *desc,
                          uint8_t *dst,
                          size_t dst_len)
{
    if (!state || !desc || !dst) return 0;

    size_t produced = 0;
    switch (desc->pattern) {
    case BENCH_PATTERN_PALETTE:
        produced = palette_decode(desc, dst, dst_len, state->led_count);
        break;
    case BENCH_PATTERN_XOR_DELTA:
        produced = xor_delta_decode(state, desc, dst, dst_len, state->led_count);
        break;
    case BENCH_PATTERN_RLE:
        produced = rle_decode(desc, dst, dst_len, state->led_count);
        break;
    }

    if (produced > sizeof(state->last_frame)) {
        produced = sizeof(state->last_frame);
    }
    memcpy(state->last_frame, dst, produced);
    state->has_last = 1;
    return produced;
}
