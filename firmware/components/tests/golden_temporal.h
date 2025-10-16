/**
 * @file golden_temporal.h
 * @brief Golden snapshot arrays for PROGRESSIVE temporal shapes
 */

#ifndef GOLDEN_TEMPORAL_H
#define GOLDEN_TEMPORAL_H

#include <stdint.h>

// Golden for ramp(60->150, count=16)
static const uint16_t GOLDEN_RAMP_60_150_16[16] = {
    60, 66, 72, 78, 84, 90, 96, 102, 108, 114, 120, 126, 132, 138, 144, 150
};

// Golden for triangle(60->120->60, count=16)
static const uint16_t GOLDEN_TRIANGLE_60_120_60_16[16] = {
    60, 67, 75, 82, 90, 97, 105, 112,
    120, 112, 103, 95, 86, 78, 69, 60
};

// Golden for wedge(60->140, count=16, plateau last 4)
static const uint16_t GOLDEN_WEDGE_60_140_16[16] = {
    60, 67, 74, 81, 89, 96, 103, 110, 118, 125, 132, 140, 140, 140, 140, 140
};

#endif // GOLDEN_TEMPORAL_H

