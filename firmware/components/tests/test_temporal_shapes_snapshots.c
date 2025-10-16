/**
 * @file test_temporal_shapes_snapshots.c
 * @brief Golden snapshot tests for temporal shapes
 */

#include "unity.h"
#include <string.h>
#include "prism_temporal.h"
#include "golden_temporal.h"

TEST_CASE("Golden: ramp 60->150 (16)", "[playback][golden]")
{
    uint16_t tbl[16];
    prism_build_progressive_ramp(60, 150, tbl, 16);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(GOLDEN_RAMP_60_150_16, tbl, 16);
}

TEST_CASE("Golden: triangle 60->120->60 (16)", "[playback][golden]")
{
    uint16_t tbl[16];
    prism_build_triangle(60, 120, 60, tbl, 16, PRISM_MOTION_LEFT);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(GOLDEN_TRIANGLE_60_120_60_16, tbl, 16);
}

TEST_CASE("Golden: wedge 60->140 (16)", "[playback][golden]")
{
    uint16_t tbl[16];
    prism_build_wedge(60, 140, tbl, 16, PRISM_MOTION_LEFT);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(GOLDEN_WEDGE_60_140_16, tbl, 16);
}

