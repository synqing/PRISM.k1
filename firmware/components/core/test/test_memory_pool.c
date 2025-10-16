/**
 * @file test_memory_pool.c
 * @brief Unit tests for memory pool manager
 *
 * Tests verify:
 * - Pool initialization
 * - Allocation/deallocation
 * - Bitmap tracking
 * - Statistics accuracy
 * - Thread safety
 * - Fragmentation prevention
 */

#include "unity.h"
#include "prism_memory_pool.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

// Test helper macros
#define TEST_ASSERT_POOL_STATS(free_4k, free_1k, free_256b) do { \
    pool_stats_t stats; \
    TEST_ASSERT_EQUAL(ESP_OK, prism_pool_get_stats(&stats)); \
    TEST_ASSERT_EQUAL_UINT32(free_4k, stats.blocks_free_4k); \
    TEST_ASSERT_EQUAL_UINT32(free_1k, stats.blocks_free_1k); \
    TEST_ASSERT_EQUAL_UINT32(free_256b, stats.blocks_free_256b); \
} while(0)

// Test setup and teardown
void setUp(void) {
    // Each test starts with fresh pools
    prism_pool_reset_stats();
}

void tearDown(void) {
    // Nothing to clean up - pools persist
}

/**
 * Test pool initialization
 */
TEST_CASE("Pool initialization", "[memory_pool]") {
    // First init should succeed
    TEST_ASSERT_EQUAL(ESP_OK, prism_pool_init());

    // Second init should be safe (already initialized)
    TEST_ASSERT_EQUAL(ESP_OK, prism_pool_init());

    // Verify initial stats
    pool_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, prism_pool_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(POOL_COUNT_4K, stats.blocks_free_4k);
    TEST_ASSERT_EQUAL_UINT32(POOL_COUNT_1K, stats.blocks_free_1k);
    TEST_ASSERT_EQUAL_UINT32(POOL_COUNT_256B, stats.blocks_free_256b);
    TEST_ASSERT_EQUAL_UINT32(0, stats.total_allocs);
    TEST_ASSERT_EQUAL_UINT32(0, stats.failed_allocs);
}

/**
 * Test basic allocation and deallocation
 */
TEST_CASE("Basic allocation", "[memory_pool]") {
    // Test 256B allocation
    void* ptr256 = prism_pool_alloc(100);
    TEST_ASSERT_NOT_NULL(ptr256);
    TEST_ASSERT_TRUE(prism_pool_is_pool_memory(ptr256));
    TEST_ASSERT_POOL_STATS(POOL_COUNT_4K, POOL_COUNT_1K, POOL_COUNT_256B - 1);

    // Test 1K allocation
    void* ptr1k = prism_pool_alloc(500);
    TEST_ASSERT_NOT_NULL(ptr1k);
    TEST_ASSERT_TRUE(prism_pool_is_pool_memory(ptr1k));
    TEST_ASSERT_POOL_STATS(POOL_COUNT_4K, POOL_COUNT_1K - 1, POOL_COUNT_256B - 1);

    // Test 4K allocation
    void* ptr4k = prism_pool_alloc(2000);
    TEST_ASSERT_NOT_NULL(ptr4k);
    TEST_ASSERT_TRUE(prism_pool_is_pool_memory(ptr4k));
    TEST_ASSERT_POOL_STATS(POOL_COUNT_4K - 1, POOL_COUNT_1K - 1, POOL_COUNT_256B - 1);

    // Free all
    prism_pool_free(ptr256);
    prism_pool_free(ptr1k);
    prism_pool_free(ptr4k);

    TEST_ASSERT_POOL_STATS(POOL_COUNT_4K, POOL_COUNT_1K, POOL_COUNT_256B);
}

/**
 * Test pool exhaustion
 */
TEST_CASE("Pool exhaustion", "[memory_pool]") {
    // Allocate all 256B blocks
    void* ptrs[POOL_COUNT_256B];
    for (int i = 0; i < POOL_COUNT_256B; i++) {
        ptrs[i] = prism_pool_alloc(200);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    // Next allocation should fall back to 1K pool
    void* fallback = prism_pool_alloc(200);
    TEST_ASSERT_NOT_NULL(fallback);
    TEST_ASSERT_POOL_STATS(POOL_COUNT_4K, POOL_COUNT_1K - 1, 0);

    // Free all
    for (int i = 0; i < POOL_COUNT_256B; i++) {
        prism_pool_free(ptrs[i]);
    }
    prism_pool_free(fallback);
}

/**
 * Test allocation too large
 */
TEST_CASE("Allocation too large", "[memory_pool]") {
    // Request larger than max pool size
    void* ptr = prism_pool_alloc(5000);
    TEST_ASSERT_NULL(ptr);

    pool_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, prism_pool_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1, stats.failed_allocs);
}

/**
 * Test double free detection
 */
TEST_CASE("Double free detection", "[memory_pool]") {
    void* ptr = prism_pool_alloc(100);
    TEST_ASSERT_NOT_NULL(ptr);

    // First free is valid
    prism_pool_free(ptr);

    // Second free should be detected (check logs)
    prism_pool_free(ptr);
}

/**
 * Test non-pool memory free
 */
TEST_CASE("Non-pool memory free", "[memory_pool]") {
    uint8_t stack_buffer[100];

    // Trying to free non-pool memory should be detected
    prism_pool_free(stack_buffer);

    // Should not crash, just log error
}

/**
 * Test statistics accuracy
 */
TEST_CASE("Statistics accuracy", "[memory_pool]") {
    prism_pool_reset_stats();

    // Do some allocations
    void* ptr1 = prism_pool_alloc(100);
    void* ptr2 = prism_pool_alloc(500);
    void* ptr3 = prism_pool_alloc(2000);

    pool_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, prism_pool_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(3, stats.total_allocs);

    // Free some
    prism_pool_free(ptr1);
    prism_pool_free(ptr2);

    TEST_ASSERT_EQUAL(ESP_OK, prism_pool_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(2, stats.total_frees);

    // Check peak usage
    TEST_ASSERT_EQUAL_UINT32(1, stats.peak_usage_4k);
    TEST_ASSERT_EQUAL_UINT32(1, stats.peak_usage_1k);
    TEST_ASSERT_EQUAL_UINT32(1, stats.peak_usage_256b);

    prism_pool_free(ptr3);
}

/**
 * Test thread safety with concurrent allocations
 */
static void allocation_task(void* param) {
    int task_id = (int)param;

    for (int i = 0; i < 100; i++) {
        // Allocate random sizes
        size_t size = (esp_random() % 3 == 0) ? 100 :
                     (esp_random() % 3 == 1) ? 500 : 2000;

        void* ptr = prism_pool_alloc(size);
        if (ptr != NULL) {
            // Write pattern to detect corruption
            memset(ptr, task_id, size);

            // Small delay
            vTaskDelay(pdMS_TO_TICKS(1));

            // Verify pattern
            uint8_t* bytes = (uint8_t*)ptr;
            for (size_t j = 0; j < size; j++) {
                if (bytes[j] != task_id) {
                    TEST_FAIL_MESSAGE("Memory corruption detected!");
                }
            }

            prism_pool_free(ptr);
        }

        // Yield to other tasks
        taskYIELD();
    }

    vTaskDelete(NULL);
}

TEST_CASE("Thread safety", "[memory_pool]") {
    #define NUM_TASKS 4
    TaskHandle_t tasks[NUM_TASKS];

    // Create multiple tasks doing concurrent allocations
    for (int i = 0; i < NUM_TASKS; i++) {
        xTaskCreate(allocation_task, "alloc_task", 2048, (void*)i, 5, &tasks[i]);
    }

    // Wait for all tasks to complete
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Verify pool integrity
    pool_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, prism_pool_get_stats(&stats));

    // All blocks should be free after tasks complete
    TEST_ASSERT_EQUAL_UINT32(POOL_COUNT_4K, stats.blocks_free_4k);
    TEST_ASSERT_EQUAL_UINT32(POOL_COUNT_1K, stats.blocks_free_1k);
    TEST_ASSERT_EQUAL_UINT32(POOL_COUNT_256B, stats.blocks_free_256b);
}

/**
 * Test fragmentation resistance over time
 */
TEST_CASE("Fragmentation resistance", "[memory_pool][long]") {
    size_t initial_free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t initial_largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

    // Simulate typical allocation pattern for 1000 iterations
    for (int iter = 0; iter < 1000; iter++) {
        void* ptrs[10];

        // Allocate mix of sizes
        for (int i = 0; i < 10; i++) {
            size_t size = (i % 3 == 0) ? 256 : (i % 3 == 1) ? 1024 : 4096;
            ptrs[i] = prism_pool_alloc(size);
        }

        // Free in different order (worst case for fragmentation)
        for (int i = 0; i < 10; i += 2) {
            prism_pool_free(ptrs[i]);
        }
        for (int i = 1; i < 10; i += 2) {
            prism_pool_free(ptrs[i]);
        }

        // Every 100 iterations, check heap health
        if (iter % 100 == 0) {
            size_t current_free = heap_caps_get_free_size(MALLOC_CAP_8BIT);
            size_t current_largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

            // Heap should not fragment significantly
            TEST_ASSERT_INT_WITHIN(1000, initial_free_heap, current_free);
            TEST_ASSERT_INT_WITHIN(1000, initial_largest_block, current_largest);
        }
    }

    // Final check - all pools should be free
    TEST_ASSERT_POOL_STATS(POOL_COUNT_4K, POOL_COUNT_1K, POOL_COUNT_256B);
}

/**
 * Test performance metrics
 */
TEST_CASE("Performance metrics", "[memory_pool]") {
    prism_pool_reset_stats();

    // Do many allocations to get average times
    for (int i = 0; i < 100; i++) {
        void* ptr = prism_pool_alloc(256);
        TEST_ASSERT_NOT_NULL(ptr);
        prism_pool_free(ptr);
    }

    pool_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, prism_pool_get_stats(&stats));

    // Check that timing stats are reasonable (< 100us average)
    TEST_ASSERT_LESS_THAN_UINT32(100, stats.alloc_time_us);
    TEST_ASSERT_LESS_THAN_UINT32(100, stats.free_time_us);

    printf("Average alloc time: %lu us\n", stats.alloc_time_us);
    printf("Average free time: %lu us\n", stats.free_time_us);
}

/**
 * Test state dump functionality
 */
TEST_CASE("State dump", "[memory_pool]") {
    // Allocate some blocks
    void* ptr1 = prism_pool_alloc(100);
    void* ptr2 = prism_pool_alloc(500);
    void* ptr3 = prism_pool_alloc(2000);

    // Dump state (check console output)
    prism_pool_dump_state();

    // Free all
    prism_pool_free(ptr1);
    prism_pool_free(ptr2);
    prism_pool_free(ptr3);
}