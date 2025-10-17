#include "unity.h"
#include "prism_temporal_api.h"
#include <string.h>
#include "esp_timer.h"

#define LED_COUNT 160

static prism_temporal_ctx_t ctx;
static uint16_t ch1_frame[LED_COUNT];
static uint16_t ch2_frame[LED_COUNT];

void setUp(void) {
    memset(&ctx, 0, sizeof(ctx));
    memset(ch1_frame, 0, sizeof(ch1_frame));
    memset(ch2_frame, 0, sizeof(ch2_frame));
    for (size_t i = 0; i < LED_COUNT; i++) {
        ch1_frame[i] = (uint16_t)(i * 10);
    }
}

void tearDown(void) {}

TEST_CASE("prism_motion_init validates pointers", "[temporal]") {
    esp_err_t err = prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(PRISM_SYNC_SYNC, ctx.sync_mode);
    TEST_ASSERT_EQUAL(0, ctx.frame_index);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, prism_motion_init(NULL, ch1_frame, ch2_frame, LED_COUNT));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, prism_motion_init(&ctx, NULL, ch2_frame, LED_COUNT));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, prism_motion_init(&ctx, ch1_frame, ch2_frame, 0));
}

TEST_CASE("SYNC mode copies ch1 to ch2", "[temporal]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_SYNC;
    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ch1_frame, ch2_frame, LED_COUNT);
}

TEST_CASE("OFFSET zero behaves like SYNC", "[temporal]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_OFFSET;
    ctx.params.delay_ms = 0;
    ctx.frame_index = 10;
    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ch1_frame, ch2_frame, LED_COUNT);
}

TEST_CASE("OFFSET delay window zeros LEDs", "[temporal]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_OFFSET;
    ctx.params.delay_ms = 150;  // 150ms delay
    ctx.frame_index = 5;  // 5 * 16ms = 80ms < 150ms
    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    for (size_t i = 0; i < LED_COUNT; i++) {
        TEST_ASSERT_EQUAL_UINT16(0, ch2_frame[i]);
    }
}

TEST_CASE("CUSTOM per-LED delay applies", "[temporal][custom]") {
    uint16_t delay_map[LED_COUNT];
    for (int i = 0; i < LED_COUNT; i++) {
        delay_map[i] = i * 5;
    }
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_CUSTOM;
    ctx.delay_table = delay_map;
    ctx.frame_time_ms = 50; // active for indices where delay<=50
    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    for (int i = 0; i < LED_COUNT; i++) {
        if (delay_map[i] <= 50) TEST_ASSERT_EQUAL_UINT16(ch1_frame[i], ch2_frame[i]);
        else TEST_ASSERT_EQUAL_UINT16(0, ch2_frame[i]);
    }
}

