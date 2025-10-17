// Unity tests for CUSTOM mode delay map validation
#include "unity.h"
#include "prism_temporal_api.h"

TEST_CASE("CUSTOM mode delay map validation", "[temporal][custom]") {
    uint16_t valid_map[160];
    for (int i = 0; i < 160; i++) {
        valid_map[i] = i * 10; // 0ms to 1590ms
    }

    esp_err_t err = prism_temporal_validate_delay_map(valid_map, 160);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

TEST_CASE("CUSTOM mode rejects NULL delay map", "[temporal][custom]") {
    esp_err_t err = prism_temporal_validate_delay_map(NULL, 160);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

TEST_CASE("CUSTOM mode rejects invalid size", "[temporal][custom]") {
    uint16_t map[100];
    esp_err_t err = prism_temporal_validate_delay_map(map, 100);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, err);
}

TEST_CASE("CUSTOM mode applies per-LED delays (sanity)", "[temporal][custom]") {
    prism_temporal_ctx_t ctx;
    uint16_t ch1[160], ch2[160];
    uint16_t delay_map[160];

    for (int i = 0; i < 160; i++) {
        ch1[i] = 1000;
        delay_map[i] = i * 5; // 0, 5, 10, 15... ms
    }

    // Init context
    prism_motion_init(&ctx, ch1, ch2, 160);
    ctx.sync_mode = PRISM_SYNC_CUSTOM;
    ctx.delay_table = delay_map;

    // At frame_time=50ms, only LEDs with delay <= 50 should be active
    ctx.frame_time_ms = 50;
    calculate_ch2_frame(&ctx, ch1, ch2, 160);

    for (int i = 0; i < 160; i++) {
        if (delay_map[i] <= 50) {
            TEST_ASSERT_EQUAL_UINT16(1000, ch2[i]);
        } else {
            TEST_ASSERT_EQUAL_UINT16(0, ch2[i]);
        }
    }
}

