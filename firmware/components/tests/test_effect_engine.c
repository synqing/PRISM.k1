/**
 * @file test_effect_engine.c
 * @brief Unit tests for effect engine (Task 9)
 */

#include "unity.h"
#include "effect_engine.h"
#include <string.h>

TEST_CASE("effect_engine brightness ramp", "[effects]")
{
    effect_engine_init();
    effect_chain_clear();
    effect_add_brightness(0);
    effect_brightness_set_target(255, 100); // 100ms ramp

    uint8_t buf[6] = {100, 150, 200, 50, 75, 100};
    // Halfway at 50ms ~= 50% scale
    effect_engine_tick(50);
    uint8_t tmp[6]; memcpy(tmp, buf, sizeof(buf));
    effect_chain_apply(tmp, 2);
    TEST_ASSERT_UINT8_WITHIN(2, (uint8_t)(buf[0]/2), tmp[0]);
    TEST_ASSERT_UINT8_WITHIN(2, (uint8_t)(buf[1]/2), tmp[1]);
    TEST_ASSERT_UINT8_WITHIN(2, (uint8_t)(buf[2]/2), tmp[2]);

    // Finish ramp
    effect_engine_tick(60);
    memcpy(tmp, buf, sizeof(buf));
    effect_chain_apply(tmp, 2);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(buf, tmp, sizeof(buf));
}

TEST_CASE("effect_engine gamma lut updates", "[effects]")
{
    effect_engine_init();
    effect_chain_clear();
    effect_add_brightness(255);
    effect_add_gamma(100); // identity

    uint8_t pixel[3] = { 64, 128, 255 };
    uint8_t tmp[3]; memcpy(tmp, pixel, sizeof(pixel));
    effect_chain_apply(tmp, 1);
    // With gamma 1.0 and full brightness, buffer unchanged
    TEST_ASSERT_EQUAL_UINT8_ARRAY(pixel, tmp, sizeof(pixel));

    // Change gamma to 200 over 0ms (immediate), values should be <= original
    effect_gamma_set_target(200, 0);
    memcpy(tmp, pixel, sizeof(pixel));
    effect_chain_apply(tmp, 1);
    TEST_ASSERT_TRUE(tmp[0] <= pixel[0]);
    TEST_ASSERT_TRUE(tmp[1] <= pixel[1]);
}

