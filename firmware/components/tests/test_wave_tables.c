/**
 * @file test_wave_tables.c
 * @brief Unity tests for WAVE LUT helpers
 */

#include "unity.h"
#include "prism_wave_tables.h"

TEST_CASE("sin8_table midpoint is 128", "[wave][lut]")
{
    // 0 phase and pi phase should be symmetric around 128
    TEST_ASSERT_EQUAL_UINT8(128, sin8_table[0]);
}

TEST_CASE("triangle8 first half ramps up", "[wave][triangle]")
{
    // Verify monotonic increase from 0..127
    uint8_t prev = triangle8(0);
    for (int i = 1; i < 128; i++) {
        uint8_t cur = triangle8((uint8_t)i);
        TEST_ASSERT_TRUE(cur > prev);
        prev = cur;
    }
}

