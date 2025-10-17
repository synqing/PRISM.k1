// Unity tests for temporal runtime (SYNC/OFFSET)
#include "unity.h"
#include "prism_temporal_api.h"
#include "esp_timer.h"
#include <string.h>

#define LED_COUNT 160
#define FRAME_PERIOD_MS 16  // 60 FPS

static prism_temporal_ctx_t ctx;
static uint16_t ch1_frame[LED_COUNT];
static uint16_t ch2_frame[LED_COUNT];

void setUp(void) {
    memset(&ctx, 0, sizeof(ctx));
    memset(ch1_frame, 0, sizeof(ch1_frame));
    memset(ch2_frame, 0, sizeof(ch2_frame));

    // Fill ch1 with test pattern
    for (size_t i = 0; i < LED_COUNT; i++) {
        ch1_frame[i] = (uint16_t)(i * 10);
    }
}

void tearDown(void) {
}

TEST_CASE("prism_motion_init validates pointers", "[temporal]") {
    esp_err_t err;

    // Valid init
    err = prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(PRISM_SYNC_SYNC, ctx.sync_mode);
    TEST_ASSERT_EQUAL(0, ctx.frame_index);

    // NULL ctx
    err = prism_motion_init(NULL, ch1_frame, ch2_frame, LED_COUNT);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);

    // NULL ch1_frame
    err = prism_motion_init(&ctx, NULL, ch2_frame, LED_COUNT);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);

    // Zero LED count
    err = prism_motion_init(&ctx, ch1_frame, ch2_frame, 0);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

TEST_CASE("SYNC mode copies ch1 to ch2", "[temporal]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_SYNC;

    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);

    // Verify exact copy
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ch1_frame, ch2_frame, LED_COUNT);
}

TEST_CASE("OFFSET mode with zero delay behaves like SYNC", "[temporal]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_OFFSET;
    ctx.params.delay_ms = 0;
    ctx.frame_index = 10;

    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);

    // Zero delay should behave like SYNC
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ch1_frame, ch2_frame, LED_COUNT);
}

TEST_CASE("OFFSET mode zeros LEDs during delay window", "[temporal]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_OFFSET;
    ctx.params.delay_ms = 150;  // 150ms delay
    ctx.frame_index = 5;  // 5 frames * 16ms = 80ms < 150ms

    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);

    // All LEDs should be zero (still in delay window)
    for (size_t i = 0; i < LED_COUNT; i++) {
        TEST_ASSERT_EQUAL_UINT16(0, ch2_frame[i]);
    }
}

TEST_CASE("OFFSET mode activates after delay threshold", "[temporal]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_OFFSET;
    ctx.params.delay_ms = 150;
    ctx.frame_index = 10;  // 10 * 16 = 160ms > 150ms

    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);

    // Should now match ch1
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ch1_frame, ch2_frame, LED_COUNT);
}

TEST_CASE("WAVE mode with zero amplitude behaves like OFFSET base", "[temporal][wave]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_WAVE;
    ctx.params.delay_ms = 100;      // base delay
    ctx.params.wave_amplitude_ms = 0; // zero amplitude

    // Before threshold
    ctx.frame_index = 5; // 80ms
    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    for (size_t i = 0; i < LED_COUNT; i++) {
        TEST_ASSERT_EQUAL_UINT16(0, ch2_frame[i]);
    }

    // After threshold
    ctx.frame_index = 10; // 160ms
    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ch1_frame, ch2_frame, LED_COUNT);
}

TEST_CASE("WAVE mode amplitude creates mixed on/off at intermediate times", "[temporal][wave]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_WAVE;
    ctx.params.delay_ms = 100;        // base
    ctx.params.wave_amplitude_ms = 50; // delays range ~50..150ms

    // Pick 96ms so some LEDs are on and some still off
    ctx.frame_index = 6; // 6 * 16 = 96ms
    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);

    bool any_on = false, any_off = false;
    for (size_t i = 0; i < LED_COUNT; i++) {
        if (ch2_frame[i] == 0) any_off = true;
        if (ch2_frame[i] != 0) any_on = true;
    }
    TEST_ASSERT_TRUE(any_on);
    TEST_ASSERT_TRUE(any_off);
}

TEST_CASE("calculate_ch2_frame meets performance budget", "[temporal]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_OFFSET;
    ctx.params.delay_ms = 100;
    ctx.frame_index = 20;

    int64_t start = esp_timer_get_time();
    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    int64_t end = esp_timer_get_time();

    int64_t elapsed_us = end - start;

    // Must complete in <3380 microseconds (3.38ms budget)
    TEST_ASSERT_LESS_THAN(3380, elapsed_us);
}
