/**
 * @file pattern_storage.h
 * @brief Pattern storage manager for PRISM K1
 *
 * Manages LittleFS filesystem for pattern persistence.
 * Implements ADR-005 (/littlefs mount path), ADR-006 (15-25 patterns),
 * and ADR-007 (partition at 0x320000, 1.5MB).
 */

#ifndef PRISM_PATTERN_STORAGE_H
#define PRISM_PATTERN_STORAGE_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ADR-005: Storage mount path
#define STORAGE_MOUNT_PATH  "/littlefs"
#define STORAGE_PARTITION   "littlefs"

// ADR-006: Pattern storage bounds (simplified for recovery)
#define PATTERN_SIZE_MAX    102400      // 100KB per pattern (simplified)
#define PATTERN_MIN_COUNT   15          // Must fit 15 patterns minimum
#define PATTERN_IDEAL_COUNT 25          // Target capacity

/**
 * @brief Initialize storage subsystem
 *
 * Mounts LittleFS at /littlefs (ADR-005) and validates partition (ADR-007).
 * Auto-formats filesystem on first boot.
 *
 * @return ESP_OK on success
 * @return ESP_ERR_NOT_FOUND if partition not found
 * @return ESP_ERR_INVALID_SIZE if partition size mismatch
 */
esp_err_t storage_init(void);

/**
 * @brief Deinitialize storage subsystem
 *
 * Unmounts filesystem and cleanup.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_deinit(void);

/**
 * @brief Storage task entry point
 *
 * FreeRTOS task for pattern file operations and cache management.
 *
 * Stack: 6KB
 * Priority: 4 (medium-low)
 * Core: 0
 *
 * @param pvParameters Task parameters (unused)
 */
void storage_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // PRISM_PATTERN_STORAGE_H
