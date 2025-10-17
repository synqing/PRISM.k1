/**
 * @file prism_temporal.c
 * @brief PROGRESSIVE mode temporal shape builders
 */

#include "prism_temporal.h"

void prism_build_progressive_ramp(uint16_t start_ms, uint16_t end_ms,
                                  uint16_t *table, size_t count)
{
    if (!table || count == 0) {
        return;
    }

    const int32_t span = (int32_t)end_ms - (int32_t)start_ms;
    if (count == 1) {
        table[0] = start_ms;
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        const int32_t scaled = (span * (int32_t)i) / (int32_t)(count - 1);
        table[i] = (uint16_t)(start_ms + scaled);
    }
}

size_t prism_apply_motion_index(size_t index, size_t count, prism_motion_t direction)
{
    if (count == 0) return 0;

    switch (direction) {
        case PRISM_MOTION_LEFT:
            return index; // 0 -> count-1
        case PRISM_MOTION_RIGHT:
            return (count - 1) - index; // reversed
        case PRISM_MOTION_EDGE:
        case PRISM_MOTION_CENTER: {
            // Mirror mapping around center (simple symmetric mapping)
            size_t mirrored = (index < count / 2) ? index : (count - 1) - index;
            return mirrored;
        }
        case PRISM_MOTION_STATIC:
        default:
            return index;
    }
}

void prism_build_triangle(uint16_t start_ms, uint16_t peak_ms, uint16_t end_ms,
                          uint16_t *table, size_t count, prism_motion_t direction)
{
    if (!table || count == 0) return;
    if (count == 1) { table[0] = start_ms; return; }

    size_t half = count / 2;
    // Up ramp [0..half]
    prism_build_progressive_ramp(start_ms, peak_ms, table, (half == 0) ? 1 : (half + 1));
    // Down ramp [half..count-1]
    if (count > 1) {
        prism_build_progressive_ramp(peak_ms, end_ms, &table[half], count - half);
    }

    // Apply motion mapping in-place by reindexing into a temp buffer if needed
    if (direction != PRISM_MOTION_LEFT) {
        uint16_t tmp[320]; // sufficient for expected LED counts (<=320)
        size_t n = count;
        if (n > sizeof(tmp) / sizeof(tmp[0])) n = sizeof(tmp) / sizeof(tmp[0]);
        for (size_t i = 0; i < n; ++i) {
            size_t dst = prism_apply_motion_index(i, n, direction);
            tmp[dst] = table[i];
        }
        for (size_t i = 0; i < n; ++i) {
            table[i] = tmp[i];
        }
    }
}

void prism_build_wedge(uint16_t start_ms, uint16_t peak_ms,
                       uint16_t *table, size_t count, prism_motion_t direction)
{
    if (!table || count == 0) return;
    if (count == 1) { table[0] = start_ms; return; }

    // Ramp up over first ~75%, then hold
    size_t ramp_len = (count * 3) / 4;
    if (ramp_len < 2) ramp_len = 2;
    if (ramp_len > count) ramp_len = count;

    prism_build_progressive_ramp(start_ms, peak_ms, table, ramp_len);

    // Plateau
    for (size_t i = ramp_len; i < count; ++i) {
        table[i] = peak_ms;
    }

    // Apply motion mapping as above
    if (direction != PRISM_MOTION_LEFT) {
        uint16_t tmp[320];
        size_t n = count;
        if (n > sizeof(tmp) / sizeof(tmp[0])) n = sizeof(tmp) / sizeof(tmp[0]);
        for (size_t i = 0; i < n; ++i) {
            size_t dst = prism_apply_motion_index(i, n, direction);
            tmp[dst] = table[i];
        }
        for (size_t i = 0; i < n; ++i) {
            table[i] = tmp[i];
        }
    }
}

