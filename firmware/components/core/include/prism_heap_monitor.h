/**
 * @file prism_heap_monitor.h
 * @brief Comprehensive heap monitoring system with fragmentation detection
 *
 * Tracks memory health every second, detects fragmentation, and provides
 * early warning for memory-related failures.
 *
 * Based on research showing fragmentation causes device failure within
 * 12-48 hours without proper monitoring and intervention.
 */

#ifndef PRISM_HEAP_MONITOR_H
#define PRISM_HEAP_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Heap health thresholds (calibrated from research)
 */
#define HEAP_CRITICAL_MIN           50000    // 50KB minimum free
#define HEAP_WARNING_MIN            75000    // 75KB warning threshold
#define LARGEST_BLOCK_MIN           20000    // 20KB largest block minimum
#define FRAGMENTATION_WARNING       30       // 30% fragmentation warning
#define FRAGMENTATION_CRITICAL      50       // 50% fragmentation critical
#define STACK_WARNING_BYTES         512      // Task stack warning threshold

/**
 * Monitoring configuration
 */
#define HEAP_MONITOR_INTERVAL_MS    1000     // Monitor every second
#define HEAP_METRICS_HISTORY        60       // Keep 60 samples (1 minute)
#define HEAP_INTEGRITY_CHECK_MS     10000    // Integrity check every 10s

/**
 * Heap health metrics snapshot
 */
typedef struct {
    uint32_t timestamp_ms;           // Milliseconds since boot
    uint32_t free_heap;              // Current free heap
    uint32_t min_free_heap;          // Minimum free since boot
    uint32_t largest_block;          // Largest contiguous block
    uint32_t fragmentation_pct;      // Fragmentation percentage
    uint32_t alloc_count;            // Total allocations
    uint32_t free_count;             // Total frees
    uint32_t failed_allocs;          // Failed allocation attempts
} heap_metrics_t;

/**
 * Task stack usage information
 */
typedef struct {
    char task_name[16];              // Task name
    uint32_t stack_size;             // Total stack size
    uint32_t stack_used;             // Bytes used
    uint32_t stack_remaining;        // Bytes remaining (high water mark)
    uint8_t usage_pct;               // Usage percentage
    bool critical;                   // True if < STACK_WARNING_BYTES remaining
} task_stack_info_t;

/**
 * Comprehensive heap monitoring statistics
 */
typedef struct {
    // Current snapshot
    heap_metrics_t current;

    // Historical data
    heap_metrics_t history[HEAP_METRICS_HISTORY];
    uint8_t history_index;           // Circular buffer index
    uint8_t history_count;           // Number of valid samples

    // Alert counts
    uint32_t fragmentation_warnings;
    uint32_t fragmentation_critical_count;
    uint32_t low_memory_warnings;
    uint32_t low_memory_critical_count;
    uint32_t integrity_check_failures;

    // Performance metrics
    uint32_t monitor_time_us;        // Time spent in monitoring (avg)
    uint32_t max_monitor_time_us;    // Maximum monitoring time

    // Task stack tracking
    uint8_t task_count;
    task_stack_info_t tasks[32];     // Up to 32 tasks
} heap_monitor_stats_t;

/**
 * Initialize heap monitoring system
 * Creates monitoring task and initializes data structures
 *
 * @return ESP_OK on success, error code on failure
 */
esp_err_t prism_heap_monitor_init(void);

/**
 * Get current heap monitoring statistics
 *
 * @param stats Pointer to structure to fill with current stats
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if stats is NULL
 */
esp_err_t prism_heap_monitor_get_stats(heap_monitor_stats_t* stats);

/**
 * Get current heap metrics (lightweight, fast)
 *
 * @param metrics Pointer to structure to fill with current metrics
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if metrics is NULL
 */
esp_err_t prism_heap_monitor_get_metrics(heap_metrics_t* metrics);

/**
 * Dump detailed heap statistics to console
 * Includes current state, historical trends, and per-task stack usage
 */
void prism_heap_monitor_dump_stats(void);

/**
 * Dump heap statistics on crash (called from error handler)
 * Minimal, safe output suitable for crash context
 */
void prism_heap_monitor_crash_dump(void);

/**
 * Check if heap is in critical state
 *
 * @return true if heap is critical (low memory or high fragmentation)
 */
bool prism_heap_monitor_is_critical(void);

/**
 * Reset alert counters (for testing)
 */
void prism_heap_monitor_reset_alerts(void);

/**
 * Trigger immediate monitoring cycle (for testing)
 */
void prism_heap_monitor_trigger(void);

#ifdef __cplusplus
}
#endif

#endif // PRISM_HEAP_MONITOR_H