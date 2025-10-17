/**
 * @file prism_heap_monitor.c
 * @brief Heap monitoring system implementation
 *
 * Comprehensive monitoring that detects fragmentation and provides early
 * warning before memory-related failures occur.
 */

#include "prism_heap_monitor.h"
#include "prism_memory_pool.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_system.h"
#include <string.h>

static const char* TAG = "HEAP_MON";

// Global monitoring statistics
static heap_monitor_stats_t g_monitor_stats = {0};
static SemaphoreHandle_t g_monitor_mutex = NULL;
static TaskHandle_t g_monitor_task_handle = NULL;
static bool g_initialized = false;
static bool g_trigger_cycle = false;

// Forward declarations
static void heap_monitor_task(void* param);
static void collect_heap_metrics(heap_metrics_t* metrics);
static void collect_task_stack_info(void);
static void check_thresholds(const heap_metrics_t* metrics);
static void add_to_history(const heap_metrics_t* metrics);

/**
 * Initialize heap monitoring
 */
esp_err_t prism_heap_monitor_init(void) {
    if (g_initialized) {
        ESP_LOGW(TAG, "Heap monitor already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing heap monitor...");

    // Create mutex for thread safety
    g_monitor_mutex = xSemaphoreCreateMutex();
    if (g_monitor_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create monitor mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize statistics structure
    memset(&g_monitor_stats, 0, sizeof(heap_monitor_stats_t));

    // Collect initial metrics
    collect_heap_metrics(&g_monitor_stats.current);

    // Create monitoring task
    // Priority: tskIDLE_PRIORITY + 2 (low priority, runs when idle)
    // Stack: 3KB as specified
    BaseType_t result = xTaskCreate(
        heap_monitor_task,
        "heap_monitor",
        3072,  // 3KB stack
        NULL,
        tskIDLE_PRIORITY + 2,
        &g_monitor_task_handle
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create monitoring task");
        vSemaphoreDelete(g_monitor_mutex);
        return ESP_ERR_NO_MEM;
    }

    g_initialized = true;
    ESP_LOGI(TAG, "Heap monitor initialized successfully");

    return ESP_OK;
}

/**
 * Monitoring task - runs every second
 */
static void heap_monitor_task(void* param) {
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(HEAP_MONITOR_INTERVAL_MS);
    uint32_t integrity_check_counter = 0;

    ESP_LOGI(TAG, "Heap monitoring task started");

    while (1) {
        // Precise 1-second interval using vTaskDelayUntil
        vTaskDelayUntil(&last_wake_time, interval);

        // Check for manual trigger
        if (g_trigger_cycle) {
            g_trigger_cycle = false;
        }

        // Start timing
        uint64_t start_time = esp_timer_get_time();

        // Take mutex
        if (xSemaphoreTake(g_monitor_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "Failed to take mutex");
            continue;
        }

        // Collect current metrics
        heap_metrics_t metrics;
        collect_heap_metrics(&metrics);

        // Store in current
        g_monitor_stats.current = metrics;

        // Add to history
        add_to_history(&metrics);

        // Check thresholds and generate alerts
        check_thresholds(&metrics);

        // Collect task stack information (every cycle)
        collect_task_stack_info();

        // Heap integrity check (every 10 seconds in debug builds)
        #ifdef CONFIG_HEAP_POISONING
        integrity_check_counter++;
        if (integrity_check_counter >= 10) {
            integrity_check_counter = 0;
            if (!heap_caps_check_integrity_all(true)) {
                g_monitor_stats.integrity_check_failures++;
                ESP_LOGE(TAG, "Heap integrity check FAILED!");
            }
        }
        #endif

        // Calculate monitoring overhead
        uint64_t elapsed = esp_timer_get_time() - start_time;
        g_monitor_stats.monitor_time_us =
            (g_monitor_stats.monitor_time_us + elapsed) / 2;  // Running average

        if (elapsed > g_monitor_stats.max_monitor_time_us) {
            g_monitor_stats.max_monitor_time_us = elapsed;
        }

        // Warn if monitoring takes too long
        if (elapsed > 1000) {  // >1ms
            ESP_LOGW(TAG, "Monitoring cycle took %llu us (target <1000)", elapsed);
        }

        xSemaphoreGive(g_monitor_mutex);
    }
}

/**
 * Collect current heap metrics
 */
static void collect_heap_metrics(heap_metrics_t* metrics) {
    metrics->timestamp_ms = esp_timer_get_time() / 1000;
    metrics->free_heap = esp_get_free_heap_size();
    metrics->min_free_heap = esp_get_minimum_free_heap_size();
    metrics->largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

    // Calculate fragmentation percentage
    if (metrics->free_heap > 0) {
        metrics->fragmentation_pct =
            100 - (100 * metrics->largest_block / metrics->free_heap);
    } else {
        metrics->fragmentation_pct = 100;
    }

    // Get pool statistics for allocation counts
    pool_stats_t pool_stats;
    if (prism_pool_get_stats(&pool_stats) == ESP_OK) {
        metrics->alloc_count = pool_stats.total_allocs;
        metrics->free_count = pool_stats.total_frees;
        metrics->failed_allocs = pool_stats.failed_allocs;
    }
}

/**
 * Collect task stack information
 */
static void collect_task_stack_info(void) {
    UBaseType_t task_count = uxTaskGetNumberOfTasks();

    if (task_count > 32) {
        task_count = 32;  // Limit to array size
    }

    TaskStatus_t* task_status = pvPortMalloc(task_count * sizeof(TaskStatus_t));
    if (task_status == NULL) {
        ESP_LOGW(TAG, "Failed to allocate task status array");
        return;
    }

    // Get system state
    UBaseType_t actual_count = uxTaskGetSystemState(task_status, task_count, NULL);

    g_monitor_stats.task_count = actual_count;

    for (UBaseType_t i = 0; i < actual_count && i < 32; i++) {
        task_stack_info_t* info = &g_monitor_stats.tasks[i];

        // Copy task name
        strncpy(info->task_name, task_status[i].pcTaskName, 15);
        info->task_name[15] = '\0';

        // Get stack information
        info->stack_size = task_status[i].usStackHighWaterMark * sizeof(StackType_t);

        // Calculate usage
        UBaseType_t high_water = uxTaskGetStackHighWaterMark(task_status[i].xHandle);
        info->stack_remaining = high_water * sizeof(StackType_t);

        // Note: We can't directly get total stack size, use configured sizes
        // For now, estimate from remaining
        info->stack_used = 0;  // Would need task creation info
        info->usage_pct = 0;

        // Check if critical
        info->critical = (info->stack_remaining < STACK_WARNING_BYTES);

        // Log warning for critically low stack
        if (info->critical) {
            ESP_LOGW(TAG, "Task '%s' stack low: %lu bytes remaining",
                     info->task_name, info->stack_remaining);
        }
    }

    vPortFree(task_status);
}

/**
 * Check thresholds and generate alerts
 */
static void check_thresholds(const heap_metrics_t* metrics) {
    // Check free heap
    if (metrics->free_heap < HEAP_CRITICAL_MIN) {
        g_monitor_stats.low_memory_critical_count++;
        ESP_LOGE(TAG, "CRITICAL: Free heap %lu < %d bytes",
                 metrics->free_heap, HEAP_CRITICAL_MIN);
    } else if (metrics->free_heap < HEAP_WARNING_MIN) {
        g_monitor_stats.low_memory_warnings++;
        ESP_LOGW(TAG, "WARNING: Free heap %lu < %d bytes",
                 metrics->free_heap, HEAP_WARNING_MIN);
    }

    // Check largest block
    if (metrics->largest_block < LARGEST_BLOCK_MIN) {
        ESP_LOGW(TAG, "WARNING: Largest block %lu < %d bytes",
                 metrics->largest_block, LARGEST_BLOCK_MIN);
    }

    // Check fragmentation
    if (metrics->fragmentation_pct >= FRAGMENTATION_CRITICAL) {
        g_monitor_stats.fragmentation_critical_count++;
        ESP_LOGE(TAG, "CRITICAL: Fragmentation %lu%% >= %d%%",
                 metrics->fragmentation_pct, FRAGMENTATION_CRITICAL);
    } else if (metrics->fragmentation_pct >= FRAGMENTATION_WARNING) {
        g_monitor_stats.fragmentation_warnings++;
        ESP_LOGW(TAG, "WARNING: Fragmentation %lu%% >= %d%%",
                 metrics->fragmentation_pct, FRAGMENTATION_WARNING);
    }

    // Check for failed allocations
    if (metrics->failed_allocs > 0) {
        ESP_LOGE(TAG, "CRITICAL: %lu failed allocations detected",
                 metrics->failed_allocs);
    }
}

/**
 * Add metrics to circular history buffer
 */
static void add_to_history(const heap_metrics_t* metrics) {
    g_monitor_stats.history[g_monitor_stats.history_index] = *metrics;

    g_monitor_stats.history_index =
        (g_monitor_stats.history_index + 1) % HEAP_METRICS_HISTORY;

    if (g_monitor_stats.history_count < HEAP_METRICS_HISTORY) {
        g_monitor_stats.history_count++;
    }
}

/**
 * Get monitoring statistics
 */
esp_err_t prism_heap_monitor_get_stats(heap_monitor_stats_t* stats) {
    if (stats == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(g_monitor_mutex, portMAX_DELAY);
    memcpy(stats, &g_monitor_stats, sizeof(heap_monitor_stats_t));
    xSemaphoreGive(g_monitor_mutex);

    return ESP_OK;
}

/**
 * Get current metrics (lightweight)
 */
esp_err_t prism_heap_monitor_get_metrics(heap_metrics_t* metrics) {
    if (metrics == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(g_monitor_mutex, portMAX_DELAY);
    *metrics = g_monitor_stats.current;
    xSemaphoreGive(g_monitor_mutex);

    return ESP_OK;
}

/**
 * Dump detailed statistics
 */
void prism_heap_monitor_dump_stats(void) {
    if (!g_initialized) {
        ESP_LOGW(TAG, "Monitor not initialized");
        return;
    }

    xSemaphoreTake(g_monitor_mutex, portMAX_DELAY);

    ESP_LOGI(TAG, "=== Heap Monitor Statistics ===");

    // Current state
    ESP_LOGI(TAG, "Current State:");
    ESP_LOGI(TAG, "  Free: %lu bytes", g_monitor_stats.current.free_heap);
    ESP_LOGI(TAG, "  Min Free: %lu bytes", g_monitor_stats.current.min_free_heap);
    ESP_LOGI(TAG, "  Largest Block: %lu bytes", g_monitor_stats.current.largest_block);
    ESP_LOGI(TAG, "  Fragmentation: %lu%%", g_monitor_stats.current.fragmentation_pct);
    ESP_LOGI(TAG, "  Allocations: %lu", g_monitor_stats.current.alloc_count);
    ESP_LOGI(TAG, "  Frees: %lu", g_monitor_stats.current.free_count);
    ESP_LOGI(TAG, "  Failed: %lu", g_monitor_stats.current.failed_allocs);

    // Alert counts
    ESP_LOGI(TAG, "Alerts:");
    ESP_LOGI(TAG, "  Fragmentation warnings: %lu", g_monitor_stats.fragmentation_warnings);
    ESP_LOGI(TAG, "  Fragmentation critical: %lu", g_monitor_stats.fragmentation_critical_count);
    ESP_LOGI(TAG, "  Low memory warnings: %lu", g_monitor_stats.low_memory_warnings);
    ESP_LOGI(TAG, "  Low memory critical: %lu", g_monitor_stats.low_memory_critical_count);
    ESP_LOGI(TAG, "  Integrity failures: %lu", g_monitor_stats.integrity_check_failures);

    // Performance
    ESP_LOGI(TAG, "Performance:");
    ESP_LOGI(TAG, "  Avg monitoring time: %lu us", g_monitor_stats.monitor_time_us);
    ESP_LOGI(TAG, "  Max monitoring time: %lu us", g_monitor_stats.max_monitor_time_us);

    // Task stacks
    ESP_LOGI(TAG, "Task Stacks (%u tasks):", g_monitor_stats.task_count);
    for (uint8_t i = 0; i < g_monitor_stats.task_count && i < 10; i++) {
        task_stack_info_t* info = &g_monitor_stats.tasks[i];
        ESP_LOGI(TAG, "  %s: %lu bytes remaining%s",
                 info->task_name,
                 info->stack_remaining,
                 info->critical ? " [CRITICAL]" : "");
    }

    // History trend (last 10 samples)
    if (g_monitor_stats.history_count > 0) {
        ESP_LOGI(TAG, "Recent Trend (last 10s):");
        uint8_t samples = (g_monitor_stats.history_count < 10) ?
                         g_monitor_stats.history_count : 10;

        for (uint8_t i = 0; i < samples; i++) {
            uint8_t idx = (g_monitor_stats.history_index - samples + i + HEAP_METRICS_HISTORY)
                         % HEAP_METRICS_HISTORY;
            heap_metrics_t* m = &g_monitor_stats.history[idx];
            ESP_LOGI(TAG, "  -%ds: free=%lu, frag=%lu%%",
                     samples - i, m->free_heap, m->fragmentation_pct);
        }
    }

    xSemaphoreGive(g_monitor_mutex);
}

/**
 * Crash dump (minimal, safe)
 */
void prism_heap_monitor_crash_dump(void) {
    // Don't take mutex in crash context - just read current values
    ESP_LOGE(TAG, "=== CRASH - Heap State ===");
    ESP_LOGE(TAG, "Free: %lu, Min: %lu, Largest: %lu, Frag: %lu%%",
             g_monitor_stats.current.free_heap,
             g_monitor_stats.current.min_free_heap,
             g_monitor_stats.current.largest_block,
             g_monitor_stats.current.fragmentation_pct);
}

/**
 * Check if heap is critical
 */
bool prism_heap_monitor_is_critical(void) {
    if (!g_initialized) {
        return false;
    }

    heap_metrics_t metrics;
    if (prism_heap_monitor_get_metrics(&metrics) != ESP_OK) {
        return false;
    }

    return (metrics.free_heap < HEAP_CRITICAL_MIN) ||
           (metrics.fragmentation_pct >= FRAGMENTATION_CRITICAL) ||
           (metrics.failed_allocs > 0);
}

/**
 * Reset alert counters
 */
void prism_heap_monitor_reset_alerts(void) {
    xSemaphoreTake(g_monitor_mutex, portMAX_DELAY);
    g_monitor_stats.fragmentation_warnings = 0;
    g_monitor_stats.fragmentation_critical_count = 0;
    g_monitor_stats.low_memory_warnings = 0;
    g_monitor_stats.low_memory_critical_count = 0;
    g_monitor_stats.integrity_check_failures = 0;
    xSemaphoreGive(g_monitor_mutex);
}

/**
 * Trigger immediate monitoring cycle
 */
void prism_heap_monitor_trigger(void) {
    g_trigger_cycle = true;
    if (g_monitor_task_handle != NULL) {
        xTaskNotifyGive(g_monitor_task_handle);
    }
}