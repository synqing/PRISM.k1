/**
 * @file prism_memory_pool.c
 * @brief Memory pool manager implementation
 *
 * Three-tier pool architecture prevents heap fragmentation that causes
 * device failure in 12-48 hours without proper memory management.
 */

#include "prism_memory_pool.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include <string.h>
#include <assert.h>

static const char* TAG = "POOL";

/**
 * Memory pool structure with bitmap allocation tracking
 */
typedef struct {
    // Pool memory blocks (aligned for DMA compatibility)
    uint8_t pool_4k[POOL_COUNT_4K][POOL_SIZE_4K] __attribute__((aligned(4)));
    uint8_t pool_1k[POOL_COUNT_1K][POOL_SIZE_1K] __attribute__((aligned(4)));
    uint8_t pool_256b[POOL_COUNT_256B][POOL_SIZE_256B] __attribute__((aligned(4)));

    // Allocation bitmaps (1 bit per block, 1=allocated, 0=free)
    uint32_t bitmap_4k;      // 8 blocks fit in 32 bits
    uint32_t bitmap_1k;      // 16 blocks fit in 32 bits
    uint32_t bitmap_256b;    // 32 blocks fit in 32 bits

    // Statistics
    pool_stats_t stats;

    // Thread safety
    SemaphoreHandle_t mutex;

    // Initialization flag
    bool initialized;

    // Performance tracking
    uint64_t total_alloc_time;
    uint64_t total_free_time;
} memory_pools_t;

// Single global instance (allocated at init, never freed)
static memory_pools_t* g_pools = NULL;

// Forward declarations for internal functions
static int find_free_bit(uint32_t bitmap, int max_bits);
static void update_peak_usage(void);

/**
 * Initialize memory pools
 */
esp_err_t prism_pool_init(void) {
    if (g_pools != NULL) {
        ESP_LOGW(TAG, "Memory pools already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing memory pools...");

    // Calculate total size
    size_t total_size = sizeof(memory_pools_t);
    ESP_LOGI(TAG, "Allocating %zu bytes for memory pools", total_size);

    // Allocate from internal RAM with 32-bit alignment
    g_pools = (memory_pools_t*)heap_caps_malloc(total_size,
                                                MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT);

    if (g_pools == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory pools!");
        return ESP_ERR_NO_MEM;
    }

    // Clear all memory
    memset(g_pools, 0, sizeof(memory_pools_t));

    // Create mutex for thread safety
    g_pools->mutex = xSemaphoreCreateMutex();
    if (g_pools->mutex == NULL) {
        heap_caps_free(g_pools);
        g_pools = NULL;
        ESP_LOGE(TAG, "Failed to create mutex!");
        return ESP_ERR_NO_MEM;
    }

    // Initialize statistics
    g_pools->stats.blocks_free_4k = POOL_COUNT_4K;
    g_pools->stats.blocks_free_1k = POOL_COUNT_1K;
    g_pools->stats.blocks_free_256b = POOL_COUNT_256B;

    g_pools->initialized = true;

    // Log pool addresses for debugging
    ESP_LOGI(TAG, "Pool base address: %p", g_pools);
    ESP_LOGI(TAG, "4K pool: %p - %p", g_pools->pool_4k,
             (uint8_t*)g_pools->pool_4k + sizeof(g_pools->pool_4k));
    ESP_LOGI(TAG, "1K pool: %p - %p", g_pools->pool_1k,
             (uint8_t*)g_pools->pool_1k + sizeof(g_pools->pool_1k));
    ESP_LOGI(TAG, "256B pool: %p - %p", g_pools->pool_256b,
             (uint8_t*)g_pools->pool_256b + sizeof(g_pools->pool_256b));

    ESP_LOGI(TAG, "Memory pools initialized successfully");
    ESP_LOGI(TAG, "Total pool memory: %d KB",
             (POOL_COUNT_4K * POOL_SIZE_4K +
              POOL_COUNT_1K * POOL_SIZE_1K +
              POOL_COUNT_256B * POOL_SIZE_256B) / 1024);

    return ESP_OK;
}

/**
 * Find first free bit in bitmap
 */
static int find_free_bit(uint32_t bitmap, int max_bits) {
    for (int i = 0; i < max_bits; i++) {
        if ((bitmap & (1U << i)) == 0) {
            return i;
        }
    }
    return -1;  // No free bits
}

/**
 * Update peak usage statistics
 */
static void update_peak_usage(void) {
    uint32_t used_4k = POOL_COUNT_4K - g_pools->stats.blocks_free_4k;
    uint32_t used_1k = POOL_COUNT_1K - g_pools->stats.blocks_free_1k;
    uint32_t used_256b = POOL_COUNT_256B - g_pools->stats.blocks_free_256b;

    if (used_4k > g_pools->stats.peak_usage_4k) {
        g_pools->stats.peak_usage_4k = used_4k;
    }
    if (used_1k > g_pools->stats.peak_usage_1k) {
        g_pools->stats.peak_usage_1k = used_1k;
    }
    if (used_256b > g_pools->stats.peak_usage_256b) {
        g_pools->stats.peak_usage_256b = used_256b;
    }
}

/**
 * Allocate memory from pool
 */
void* prism_pool_alloc(size_t size) {
    if (g_pools == NULL || !g_pools->initialized) {
        ESP_LOGE(TAG, "Memory pools not initialized!");
        return NULL;
    }

    if (size == 0) {
        return NULL;
    }

    if (size > POOL_SIZE_4K) {
        ESP_LOGE(TAG, "Allocation size %zu exceeds maximum pool size", size);
        g_pools->stats.failed_allocs++;
        return NULL;
    }

    uint64_t start_time = esp_timer_get_time();
    void* result = NULL;

    xSemaphoreTake(g_pools->mutex, portMAX_DELAY);

    // Determine pool to use based on size
    if (size <= POOL_SIZE_256B) {
        // Try 256B pool first
        int idx = find_free_bit(g_pools->bitmap_256b, POOL_COUNT_256B);
        if (idx >= 0) {
            g_pools->bitmap_256b |= (1U << idx);
            g_pools->stats.blocks_free_256b--;
            result = g_pools->pool_256b[idx];
            ESP_LOGD(TAG, "Allocated 256B block %d at %p", idx, result);
        } else if (size <= POOL_SIZE_1K) {
            // Fall back to 1K pool
            idx = find_free_bit(g_pools->bitmap_1k, POOL_COUNT_1K);
            if (idx >= 0) {
                g_pools->bitmap_1k |= (1U << idx);
                g_pools->stats.blocks_free_1k--;
                result = g_pools->pool_1k[idx];
                ESP_LOGD(TAG, "Allocated 1K block %d at %p (256B fallback)", idx, result);
            }
        }
    } else if (size <= POOL_SIZE_1K) {
        // Try 1K pool
        int idx = find_free_bit(g_pools->bitmap_1k, POOL_COUNT_1K);
        if (idx >= 0) {
            g_pools->bitmap_1k |= (1U << idx);
            g_pools->stats.blocks_free_1k--;
            result = g_pools->pool_1k[idx];
            ESP_LOGD(TAG, "Allocated 1K block %d at %p", idx, result);
        } else {
            // Fall back to 4K pool
            idx = find_free_bit(g_pools->bitmap_4k, POOL_COUNT_4K);
            if (idx >= 0) {
                g_pools->bitmap_4k |= (1U << idx);
                g_pools->stats.blocks_free_4k--;
                result = g_pools->pool_4k[idx];
                ESP_LOGD(TAG, "Allocated 4K block %d at %p (1K fallback)", idx, result);
            }
        }
    } else {
        // Need 4K pool
        int idx = find_free_bit(g_pools->bitmap_4k, POOL_COUNT_4K);
        if (idx >= 0) {
            g_pools->bitmap_4k |= (1U << idx);
            g_pools->stats.blocks_free_4k--;
            result = g_pools->pool_4k[idx];
            ESP_LOGD(TAG, "Allocated 4K block %d at %p", idx, result);
        }
    }

    if (result != NULL) {
        g_pools->stats.total_allocs++;
        update_peak_usage();

        // Clear allocated memory for safety
        memset(result, 0, size);

        // Update timing statistics
        uint64_t alloc_time = esp_timer_get_time() - start_time;
        g_pools->total_alloc_time += alloc_time;
        g_pools->stats.alloc_time_us = g_pools->total_alloc_time / g_pools->stats.total_allocs;
    } else {
        g_pools->stats.failed_allocs++;
        ESP_LOGW(TAG, "No free blocks for size %zu", size);
    }

    xSemaphoreGive(g_pools->mutex);
    return result;
}

/**
 * Free memory back to pool
 */
void prism_pool_free(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    if (g_pools == NULL || !g_pools->initialized) {
        ESP_LOGE(TAG, "Memory pools not initialized!");
        return;
    }

    uint64_t start_time = esp_timer_get_time();
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t base = (uintptr_t)g_pools;
    bool found = false;

    xSemaphoreTake(g_pools->mutex, portMAX_DELAY);

    // Check 4K pool
    if (addr >= (uintptr_t)g_pools->pool_4k &&
        addr < (uintptr_t)g_pools->pool_4k + sizeof(g_pools->pool_4k)) {
        int idx = (addr - (uintptr_t)g_pools->pool_4k) / POOL_SIZE_4K;
        if (idx >= 0 && idx < POOL_COUNT_4K) {
            if (g_pools->bitmap_4k & (1U << idx)) {
                g_pools->bitmap_4k &= ~(1U << idx);
                g_pools->stats.blocks_free_4k++;
                found = true;
                ESP_LOGD(TAG, "Freed 4K block %d at %p", idx, ptr);
            } else {
                ESP_LOGE(TAG, "Double free detected for 4K block %d", idx);
            }
        }
    }
    // Check 1K pool
    else if (addr >= (uintptr_t)g_pools->pool_1k &&
             addr < (uintptr_t)g_pools->pool_1k + sizeof(g_pools->pool_1k)) {
        int idx = (addr - (uintptr_t)g_pools->pool_1k) / POOL_SIZE_1K;
        if (idx >= 0 && idx < POOL_COUNT_1K) {
            if (g_pools->bitmap_1k & (1U << idx)) {
                g_pools->bitmap_1k &= ~(1U << idx);
                g_pools->stats.blocks_free_1k++;
                found = true;
                ESP_LOGD(TAG, "Freed 1K block %d at %p", idx, ptr);
            } else {
                ESP_LOGE(TAG, "Double free detected for 1K block %d", idx);
            }
        }
    }
    // Check 256B pool
    else if (addr >= (uintptr_t)g_pools->pool_256b &&
             addr < (uintptr_t)g_pools->pool_256b + sizeof(g_pools->pool_256b)) {
        int idx = (addr - (uintptr_t)g_pools->pool_256b) / POOL_SIZE_256B;
        if (idx >= 0 && idx < POOL_COUNT_256B) {
            if (g_pools->bitmap_256b & (1U << idx)) {
                g_pools->bitmap_256b &= ~(1U << idx);
                g_pools->stats.blocks_free_256b++;
                found = true;
                ESP_LOGD(TAG, "Freed 256B block %d at %p", idx, ptr);
            } else {
                ESP_LOGE(TAG, "Double free detected for 256B block %d", idx);
            }
        }
    }

    if (found) {
        g_pools->stats.total_frees++;

        // Update timing statistics
        uint64_t free_time = esp_timer_get_time() - start_time;
        g_pools->total_free_time += free_time;
        g_pools->stats.free_time_us = g_pools->total_free_time / g_pools->stats.total_frees;
    } else {
        ESP_LOGE(TAG, "Attempt to free non-pool memory: %p", ptr);
    }

    xSemaphoreGive(g_pools->mutex);
}

/**
 * Check if pointer is from pool
 */
bool prism_pool_is_pool_memory(const void* ptr) {
    if (ptr == NULL || g_pools == NULL) {
        return false;
    }

    uintptr_t addr = (uintptr_t)ptr;

    return (addr >= (uintptr_t)g_pools->pool_4k &&
            addr < (uintptr_t)g_pools->pool_4k + sizeof(g_pools->pool_4k)) ||
           (addr >= (uintptr_t)g_pools->pool_1k &&
            addr < (uintptr_t)g_pools->pool_1k + sizeof(g_pools->pool_1k)) ||
           (addr >= (uintptr_t)g_pools->pool_256b &&
            addr < (uintptr_t)g_pools->pool_256b + sizeof(g_pools->pool_256b));
}

/**
 * Get pool statistics
 */
esp_err_t prism_pool_get_stats(pool_stats_t* stats) {
    if (stats == NULL || g_pools == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(g_pools->mutex, portMAX_DELAY);
    memcpy(stats, &g_pools->stats, sizeof(pool_stats_t));
    xSemaphoreGive(g_pools->mutex);

    return ESP_OK;
}

/**
 * Reset statistics (for testing)
 */
void prism_pool_reset_stats(void) {
    if (g_pools == NULL) {
        return;
    }

    xSemaphoreTake(g_pools->mutex, portMAX_DELAY);

    g_pools->stats.total_allocs = 0;
    g_pools->stats.total_frees = 0;
    g_pools->stats.failed_allocs = 0;
    g_pools->stats.peak_usage_4k = 0;
    g_pools->stats.peak_usage_1k = 0;
    g_pools->stats.peak_usage_256b = 0;
    g_pools->stats.alloc_time_us = 0;
    g_pools->stats.free_time_us = 0;
    g_pools->total_alloc_time = 0;
    g_pools->total_free_time = 0;

    xSemaphoreGive(g_pools->mutex);
}

/**
 * Dump pool state for debugging
 */
void prism_pool_dump_state(void) {
    if (g_pools == NULL) {
        ESP_LOGW(TAG, "Memory pools not initialized");
        return;
    }

    xSemaphoreTake(g_pools->mutex, portMAX_DELAY);

    ESP_LOGI(TAG, "=== Memory Pool State ===");
    ESP_LOGI(TAG, "4K Pool: %d/%d free (peak usage: %d)",
             g_pools->stats.blocks_free_4k, POOL_COUNT_4K,
             g_pools->stats.peak_usage_4k);
    ESP_LOGI(TAG, "1K Pool: %d/%d free (peak usage: %d)",
             g_pools->stats.blocks_free_1k, POOL_COUNT_1K,
             g_pools->stats.peak_usage_1k);
    ESP_LOGI(TAG, "256B Pool: %d/%d free (peak usage: %d)",
             g_pools->stats.blocks_free_256b, POOL_COUNT_256B,
             g_pools->stats.peak_usage_256b);

    ESP_LOGI(TAG, "Lifetime stats: %d allocs, %d frees, %d failed",
             g_pools->stats.total_allocs,
             g_pools->stats.total_frees,
             g_pools->stats.failed_allocs);

    ESP_LOGI(TAG, "Performance: alloc avg %d us, free avg %d us",
             g_pools->stats.alloc_time_us,
             g_pools->stats.free_time_us);

    // Show bitmap states
    ESP_LOGD(TAG, "Bitmaps: 4K=0x%08X, 1K=0x%08X, 256B=0x%08X",
             g_pools->bitmap_4k, g_pools->bitmap_1k, g_pools->bitmap_256b);

    xSemaphoreGive(g_pools->mutex);
}

#ifdef CONFIG_PRISM_POOL_MALLOC_WRAPPER
/**
 * Wrapper to catch malloc calls after initialization
 * This will panic to catch any code using malloc instead of pools
 */
void* __wrap_malloc(size_t size) {
    if (g_pools != NULL && g_pools->initialized) {
        ESP_LOGE(TAG, "malloc() called after pool init! Size: %zu", size);
        ESP_LOGE(TAG, "Use prism_pool_alloc() instead!");
        assert(0);  // Panic in debug builds
    }

    // Before init, allow malloc
    extern void* __real_malloc(size_t size);
    return __real_malloc(size);
}

void __wrap_free(void* ptr) {
    if (g_pools != NULL && g_pools->initialized) {
        if (prism_pool_is_pool_memory(ptr)) {
            // Redirect to pool free
            prism_pool_free(ptr);
            return;
        }
        ESP_LOGE(TAG, "free() called after pool init! Ptr: %p", ptr);
        ESP_LOGE(TAG, "Use prism_pool_free() instead!");
        assert(0);  // Panic in debug builds
    }

    // Before init, allow free
    extern void __real_free(void* ptr);
    __real_free(ptr);
}

void* __wrap_calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;

    if (g_pools != NULL && g_pools->initialized) {
        // Use pool allocation and clear
        void* ptr = prism_pool_alloc(total);
        if (ptr != NULL) {
            memset(ptr, 0, total);
        }
        return ptr;
    }

    // Before init, allow calloc
    extern void* __real_calloc(size_t nmemb, size_t size);
    return __real_calloc(nmemb, size);
}

void* __wrap_realloc(void* ptr, size_t size) {
    if (g_pools != NULL && g_pools->initialized) {
        ESP_LOGE(TAG, "realloc() not supported with memory pools!");
        assert(0);  // Panic - realloc is not supported
    }

    // Before init, allow realloc
    extern void* __real_realloc(void* ptr, size_t size);
    return __real_realloc(ptr, size);
}
#endif // CONFIG_PRISM_POOL_MALLOC_WRAPPER