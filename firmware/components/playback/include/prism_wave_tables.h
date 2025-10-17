/**
 * @file prism_wave_tables.h
 * @brief Precomputed wave lookup tables and helpers for WAVE effects
 */

#ifndef PRISM_WAVE_TABLES_H
#define PRISM_WAVE_TABLES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 256-entry sine table (0-255 mapped to 0-2π)
// Output: 0-255 representing 0.0-1.0 range (midpoint = 128)
extern const uint8_t sin8_table[256] __attribute__((aligned(64)));

// Fast triangle wave (branchless)
static inline uint8_t triangle8(uint8_t phase) {
    uint8_t folded = phase ^ ((phase & 0x80) ? 0xFF : 0x00);
    return (uint8_t)(folded << 1);
}

// Fast sawtooth wave
static inline uint8_t sawtooth8(uint8_t phase) {
    return phase;
}

#ifdef __cplusplus
}
#endif

#endif // PRISM_WAVE_TABLES_H

