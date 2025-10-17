/**
 * @file prism_temporal.h
 * @brief Temporal shape builders for PROGRESSIVE mode
 */

#ifndef PRISM_TEMPORAL_H
#define PRISM_TEMPORAL_H

#include <stddef.h>
#include <stdint.h>
#include "prism_motion.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build linear delay ramp over [0..count-1]
 */
void prism_build_progressive_ramp(uint16_t start_ms, uint16_t end_ms,
                                  uint16_t *table, size_t count);

/**
 * @brief Apply motion direction mapping to a logical index
 */
size_t prism_apply_motion_index(size_t index, size_t count, prism_motion_t direction);

/**
 * @brief Build triangle temporal shape (ramp up then ramp down)
 */
void prism_build_triangle(uint16_t start_ms, uint16_t peak_ms, uint16_t end_ms,
                          uint16_t *table, size_t count, prism_motion_t direction);

/**
 * @brief Build wedge temporal shape (ramp then hold)
 */
void prism_build_wedge(uint16_t start_ms, uint16_t peak_ms,
                       uint16_t *table, size_t count, prism_motion_t direction);

#ifdef __cplusplus
}
#endif

#endif // PRISM_TEMPORAL_H
