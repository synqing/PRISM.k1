/**
 * @file test_heap_monitor.c
 * @brief Unit tests for heap monitoring system
 */

#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "prism_heap_monitor.h"
#include "prism_memory_pool.h"

static const char* TAG = "TEST_HEAP_MON";

/**
 * Test 1: Monitor initialization
 */
TEST_CASE("heap_monitor_init_success", "[heap_monitor]")
{
    esp_err_t ret = prism_heap_monitor_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Second init should warn but succeed
    ret = prism_heap_monitor_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

/**
 * Test 2: Get current metrics
 */
TEST_CASE("heap_monitor_get_metrics", "[heap_monitor]")
{
    // Initialize first
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_init());

    // Wait for first monitoring cycle
    vTaskDelay(pdMS_TO_TICKS(1500));

    heap_metrics_t metrics;
    esp_err_t ret = prism_heap_monitor_get_metrics(&metrics);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify metrics are reasonable
    TEST_ASSERT_GREATER_THAN(0, metrics.free_heap);
    TEST_ASSERT_GREATER_THAN(0, metrics.min_free_heap);
    TEST_ASSERT_GREATER_THAN(0, metrics.largest_block);
    TEST_ASSERT_LESS_THAN(100, metrics.fragmentation_pct);
    TEST_ASSERT_GREATER_THAN(0, metrics.timestamp_ms);
}

/**
 * Test 3: Get full statistics
 */
TEST_CASE("heap_monitor_get_stats", "[heap_monitor]")
{
    // Initialize first
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_init());

    // Wait for several monitoring cycles
    vTaskDelay(pdMS_TO_TICKS(3000));

    heap_monitor_stats_t stats;
    esp_err_t ret = prism_heap_monitor_get_stats(&stats);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify current metrics
    TEST_ASSERT_GREATER_THAN(0, stats.current.free_heap);
    TEST_ASSERT_GREATER_THAN(0, stats.current.largest_block);

    // Verify history is being collected
    TEST_ASSERT_GREATER_THAN(0, stats.history_count);
    TEST_ASSERT_LESS_OR_EQUAL(3, stats.history_count);

    // Verify monitoring overhead is reasonable
    TEST_ASSERT_LESS_THAN(10000, stats.monitor_time_us);  // <10ms
}

/**
 * Test 4: Dump statistics (visual inspection)
 */
TEST_CASE("heap_monitor_dump_stats", "[heap_monitor]")
{
    // Initialize first
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_init());

    // Wait for several cycles
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Should produce detailed output
    printf("\n=== Heap Monitor Stats Dump ===\n");
    prism_heap_monitor_dump_stats();
    printf("================================\n");

    // No assert - just verify it doesn't crash
}

/**
 * Test 5: Fragmentation detection
 */
TEST_CASE("heap_monitor_fragmentation_detection", "[heap_monitor]")
{
    // Initialize monitor
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_init());

    // Create fragmentation by allocating and freeing varying sizes
    void* ptrs[100];
    for (int i = 0; i < 100; i++) {
        size_t size = (i % 3 == 0) ? 256 : ((i % 3 == 1) ? 1024 : 4096);
        ptrs[i] = malloc(size);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    // Free every other allocation to create holes
    for (int i = 0; i < 100; i += 2) {
        free(ptrs[i]);
        ptrs[i] = NULL;
    }

    // Wait for monitor to detect
    vTaskDelay(pdMS_TO_TICKS(2000));

    heap_metrics_t metrics;
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_get_metrics(&metrics));

    printf("Fragmentation after pattern: %lu%%\n", metrics.fragmentation_pct);

    // Should have some fragmentation now
    TEST_ASSERT_GREATER_THAN(0, metrics.fragmentation_pct);

    // Cleanup
    for (int i = 1; i < 100; i += 2) {
        if (ptrs[i]) free(ptrs[i]);
    }
}

/**
 * Test 6: Low memory detection
 */
TEST_CASE("heap_monitor_low_memory_detection", "[heap_monitor]")
{
    // Initialize monitor
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_init());

    // Allocate large chunk to reduce free heap
    size_t target_reduction = 100000;  // Try to reduce by 100KB
    void* large_block = malloc(target_reduction);
    TEST_ASSERT_NOT_NULL(large_block);

    // Wait for monitor to detect
    vTaskDelay(pdMS_TO_TICKS(2000));

    heap_metrics_t metrics;
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_get_metrics(&metrics));

    printf("Free heap after allocation: %lu bytes\n", metrics.free_heap);

    heap_monitor_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_get_stats(&stats));

    // May have generated warnings (depends on initial heap size)
    printf("Low memory warnings: %lu\n", stats.low_memory_warnings);

    // Cleanup
    free(large_block);
}

/**
 * Test 7: Critical heap check
 */
TEST_CASE("heap_monitor_is_critical", "[heap_monitor]")
{
    // Initialize monitor
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_init());

    // Wait for first cycle
    vTaskDelay(pdMS_TO_TICKS(1500));

    // Normal state should not be critical
    bool is_critical = prism_heap_monitor_is_critical();
    TEST_ASSERT_FALSE(is_critical);

    printf("Heap critical state: %s\n", is_critical ? "CRITICAL" : "OK");
}

/**
 * Test 8: History tracking
 */
TEST_CASE("heap_monitor_history_tracking", "[heap_monitor]")
{
    // Initialize monitor
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_init());

    // Wait for 10 seconds to collect history
    vTaskDelay(pdMS_TO_TICKS(10000));

    heap_monitor_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_get_stats(&stats));

    // Should have 10 samples (1/second)
    TEST_ASSERT_GREATER_OR_EQUAL(9, stats.history_count);
    TEST_ASSERT_LESS_OR_EQUAL(10, stats.history_count);

    // Verify history contains valid data
    for (uint8_t i = 0; i < stats.history_count; i++) {
        TEST_ASSERT_GREATER_THAN(0, stats.history[i].free_heap);
        TEST_ASSERT_LESS_THAN(100, stats.history[i].fragmentation_pct);
    }

    printf("History samples collected: %u\n", stats.history_count);
}

/**
 * Test 9: Alert counter reset
 */
TEST_CASE("heap_monitor_reset_alerts", "[heap_monitor]")
{
    // Initialize monitor
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_init());

    // Wait for some data
    vTaskDelay(pdMS_TO_TICKS(2000));

    heap_monitor_stats_t stats_before;
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_get_stats(&stats_before));

    // Reset alerts
    prism_heap_monitor_reset_alerts();

    heap_monitor_stats_t stats_after;
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_get_stats(&stats_after));

    // Verify alerts were reset
    TEST_ASSERT_EQUAL(0, stats_after.fragmentation_warnings);
    TEST_ASSERT_EQUAL(0, stats_after.fragmentation_critical_count);
    TEST_ASSERT_EQUAL(0, stats_after.low_memory_warnings);
    TEST_ASSERT_EQUAL(0, stats_after.low_memory_critical_count);

    printf("Alerts reset successfully\n");
}

/**
 * Test 10: Task stack monitoring
 */
TEST_CASE("heap_monitor_task_stack_info", "[heap_monitor]")
{
    // Initialize monitor
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_init());

    // Wait for task info collection
    vTaskDelay(pdMS_TO_TICKS(2000));

    heap_monitor_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_get_stats(&stats));

    // Should have detected some tasks
    TEST_ASSERT_GREATER_THAN(0, stats.task_count);

    printf("Task count: %u\n", stats.task_count);

    // Check first few tasks
    for (uint8_t i = 0; i < stats.task_count && i < 5; i++) {
        task_stack_info_t* info = &stats.tasks[i];
        printf("  Task[%u]: %s - %lu bytes remaining\n",
               i, info->task_name, info->stack_remaining);

        // Task name should not be empty
        TEST_ASSERT_GREATER_THAN(0, strlen(info->task_name));
    }
}

/**
 * Test 11: Manual trigger
 */
TEST_CASE("heap_monitor_manual_trigger", "[heap_monitor]")
{
    // Initialize monitor
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_init());

    // Trigger immediately
    prism_heap_monitor_trigger();

    // Give it time to process
    vTaskDelay(pdMS_TO_TICKS(200));

    heap_metrics_t metrics;
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_get_metrics(&metrics));

    // Should have fresh data
    TEST_ASSERT_GREATER_THAN(0, metrics.free_heap);

    printf("Manual trigger successful - free heap: %lu\n", metrics.free_heap);
}

/**
 * Test 12: Crash dump (safe to call)
 */
TEST_CASE("heap_monitor_crash_dump", "[heap_monitor]")
{
    // Initialize monitor
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_init());

    // Wait for data
    vTaskDelay(pdMS_TO_TICKS(1500));

    // Call crash dump (should not take mutex)
    printf("\n=== Simulated Crash Dump ===\n");
    prism_heap_monitor_crash_dump();
    printf("============================\n");

    // Verify system is still functional
    heap_metrics_t metrics;
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_get_metrics(&metrics));
}

/**
 * Test 13: Invalid parameters
 */
TEST_CASE("heap_monitor_invalid_params", "[heap_monitor]")
{
    // Initialize monitor
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_init());

    // NULL pointer to get_metrics
    esp_err_t ret = prism_heap_monitor_get_metrics(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);

    // NULL pointer to get_stats
    ret = prism_heap_monitor_get_stats(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

/**
 * Test 14: Integration with memory pool
 */
TEST_CASE("heap_monitor_pool_integration", "[heap_monitor]")
{
    // Initialize both systems
    TEST_ASSERT_EQUAL(ESP_OK, prism_pool_init());
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_init());

    // Wait for initial metrics
    vTaskDelay(pdMS_TO_TICKS(1500));

    // Do some pool allocations
    void* p1 = prism_pool_alloc(256);
    void* p2 = prism_pool_alloc(1024);
    void* p3 = prism_pool_alloc(4096);
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_NOT_NULL(p3);

    // Wait for monitor to capture
    vTaskDelay(pdMS_TO_TICKS(1500));

    heap_metrics_t metrics;
    TEST_ASSERT_EQUAL(ESP_OK, prism_heap_monitor_get_metrics(&metrics));

    // Pool statistics should be reflected
    TEST_ASSERT_GREATER_THAN(0, metrics.alloc_count);

    printf("After pool allocations:\n");
    printf("  Alloc count: %lu\n", metrics.alloc_count);
    printf("  Free count: %lu\n", metrics.free_count);

    // Cleanup
    prism_pool_free(p1);
    prism_pool_free(p2);
    prism_pool_free(p3);
}

/**
 * Test runner
 */
void app_main(void)
{
    printf("\n=== PRISM Heap Monitor Unit Tests ===\n");

    UNITY_BEGIN();

    RUN_TEST(heap_monitor_init_success);
    RUN_TEST(heap_monitor_get_metrics);
    RUN_TEST(heap_monitor_get_stats);
    RUN_TEST(heap_monitor_dump_stats);
    RUN_TEST(heap_monitor_fragmentation_detection);
    RUN_TEST(heap_monitor_low_memory_detection);
    RUN_TEST(heap_monitor_is_critical);
    RUN_TEST(heap_monitor_history_tracking);
    RUN_TEST(heap_monitor_reset_alerts);
    RUN_TEST(heap_monitor_task_stack_info);
    RUN_TEST(heap_monitor_manual_trigger);
    RUN_TEST(heap_monitor_crash_dump);
    RUN_TEST(heap_monitor_invalid_params);
    RUN_TEST(heap_monitor_pool_integration);

    UNITY_END();

    printf("\n=== All Tests Complete ===\n");
}
