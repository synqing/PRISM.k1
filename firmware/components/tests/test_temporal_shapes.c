/**
 * @file test_temporal_shapes.c
 * @brief Unit tests for PROGRESSIVE temporal shape builders
 */

#include "unity.h"
#include <stdbool.h>
#include "prism_temporal.h"

static bool is_monotonic_non_decreasing(const uint16_t *t, size_t n)
{
    if (!t || n < 2) return true;
    for (size_t i = 1; i < n; ++i) {
        if (t[i] < t[i-1]) return false;
    }
    return true;
}

TEST_CASE("Progressive ramp endpoints and monotonicity", "[playback]")
{
    uint16_t tbl[160];
    prism_build_progressive_ramp(60, 150, tbl, 160);
    TEST_ASSERT_EQUAL_UINT16(60, tbl[0]);
    TEST_ASSERT_EQUAL_UINT16(150, tbl[159]);
    TEST_ASSERT_TRUE(is_monotonic_non_decreasing(tbl, 160));
}

TEST_CASE("Motion direction RIGHT reverses mapping", "[playback]")
{
    uint16_t tbl[10];
    for (size_t i = 0; i < 10; ++i) tbl[i] = (uint16_t)i;

    // Re-map indices through motion direction helper
    uint16_t remap[10];
    for (size_t i = 0; i < 10; ++i) {
        size_t dst = prism_apply_motion_index(i, 10, PRISM_MOTION_RIGHT);
        remap[dst] = tbl[i];
    }
    TEST_ASSERT_EQUAL_UINT16(9, remap[0]);
    TEST_ASSERT_EQUAL_UINT16(0, remap[9]);
}

TEST_CASE("Triangle shape produces peak then descent", "[playback]")
{
    uint16_t tbl[8];
    prism_build_triangle(60, 120, 60, tbl, 8, PRISM_MOTION_LEFT);
    // Expect non-decreasing then non-increasing
    TEST_ASSERT_TRUE(tbl[0] <= tbl[1] && tbl[1] <= tbl[2]);
    TEST_ASSERT_TRUE(tbl[3] >= tbl[4] && tbl[4] >= tbl[5]);
}

TEST_CASE("Wedge shape ramps then holds plateau", "[playback]")
{
    uint16_t tbl[16];
    prism_build_wedge(60, 140, tbl, 16, PRISM_MOTION_LEFT);
    // Expect last quarter to be flat at peak
    uint16_t last = tbl[12];
    for (size_t i = 12; i < 16; ++i) {
        TEST_ASSERT_EQUAL_UINT16(last, tbl[i]);
    }
}
