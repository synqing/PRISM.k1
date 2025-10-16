/**
 * @file test_component_init.c
 * @brief Unity smoke tests for Task 1 component initialization
 *
 * Tests that all component *_init() functions execute without errors.
 * This validates the scaffolding from Task 1.
 */

#include "unity.h"
#include "esp_log.h"
#include "network_manager.h"
// #include "pattern_storage.h"  // Temporarily disabled - Agent 3 working on Task 5
#include "led_playback.h"
#include "template_manager.h"

static const char *TAG = "test_init";

/**
 * Setup function called before each test
 */
void setUp(void) {
    ESP_LOGI(TAG, "Test setup");
}

/**
 * Teardown function called after each test
 */
void tearDown(void) {
    ESP_LOGI(TAG, "Test teardown");
}

/**
 * Test: network_init() returns ESP_OK
 */
TEST_CASE("network_init returns ESP_OK", "[task1][network]")
{
    esp_err_t ret = network_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Cleanup
    network_deinit();
}

/**
 * Test: storage_init() returns ESP_OK
 * TODO(Task 5): Re-enable when Agent 3 completes storage implementation
 */
// TEST_CASE("storage_init returns ESP_OK", "[task1][storage]")
// {
//     esp_err_t ret = storage_init();
//     TEST_ASSERT_EQUAL(ESP_OK, ret);
//
//     // Cleanup
//     storage_deinit();
// }

/**
 * Test: playback_init() returns ESP_OK
 */
TEST_CASE("playback_init returns ESP_OK", "[task1][playback]")
{
    esp_err_t ret = playback_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Cleanup
    playback_deinit();
}

/**
 * Test: templates_init() returns ESP_OK
 */
TEST_CASE("templates_init returns ESP_OK", "[task1][templates]")
{
    esp_err_t ret = templates_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Cleanup
    templates_deinit();
}

/**
 * Test: All components initialize in sequence (integration test)
 * TODO(Task 5): Re-add storage_init() when Agent 3 completes implementation
 */
TEST_CASE("all components initialize in sequence", "[task1][integration]")
{
    ESP_LOGI(TAG, "Testing sequential component initialization");

    // Initialize in the order used by main.c
    TEST_ASSERT_EQUAL(ESP_OK, network_init());
    // TEST_ASSERT_EQUAL(ESP_OK, storage_init());  // Temporarily disabled
    TEST_ASSERT_EQUAL(ESP_OK, playback_init());
    TEST_ASSERT_EQUAL(ESP_OK, templates_init());

    ESP_LOGI(TAG, "All components initialized successfully");

    // Cleanup in reverse order
    templates_deinit();
    playback_deinit();
    // storage_deinit();  // Temporarily disabled
    network_deinit();
}

/**
 * Test: Components can be deinitialized without errors
 * TODO(Task 5): Re-add storage when Agent 3 completes implementation
 */
TEST_CASE("components deinitialize cleanly", "[task1][cleanup]")
{
    // Initialize first
    network_init();
    // storage_init();  // Temporarily disabled
    playback_init();
    templates_init();

    // Deinitialize - should not crash
    TEST_ASSERT_EQUAL(ESP_OK, network_deinit());
    // TEST_ASSERT_EQUAL(ESP_OK, storage_deinit());  // Temporarily disabled
    TEST_ASSERT_EQUAL(ESP_OK, playback_deinit());
    TEST_ASSERT_EQUAL(ESP_OK, templates_deinit());
}

/**
 * Test: Repeated init/deinit cycles work correctly
 * TODO(Task 5): Re-add storage when Agent 3 completes implementation
 */
TEST_CASE("components handle repeated init/deinit cycles", "[task1][robustness]")
{
    for (int i = 0; i < 3; i++) {
        ESP_LOGI(TAG, "Cycle %d: Initializing components", i + 1);

        TEST_ASSERT_EQUAL(ESP_OK, network_init());
        // TEST_ASSERT_EQUAL(ESP_OK, storage_init());  // Temporarily disabled
        TEST_ASSERT_EQUAL(ESP_OK, playback_init());
        TEST_ASSERT_EQUAL(ESP_OK, templates_init());

        ESP_LOGI(TAG, "Cycle %d: Deinitializing components", i + 1);

        TEST_ASSERT_EQUAL(ESP_OK, network_deinit());
        // TEST_ASSERT_EQUAL(ESP_OK, storage_deinit());  // Temporarily disabled
        TEST_ASSERT_EQUAL(ESP_OK, playback_deinit());
        TEST_ASSERT_EQUAL(ESP_OK, templates_deinit());
    }

    ESP_LOGI(TAG, "3 init/deinit cycles completed successfully");
}
