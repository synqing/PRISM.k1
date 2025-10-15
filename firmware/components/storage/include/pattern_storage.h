/**
 * @file pattern_storage.h
 * @brief Pattern storage manager for PRISM K1
 *
 * Manages LittleFS filesystem for pattern persistence.
 * Implements ADR-005 (/littlefs mount path) and ADR-004 (256KB pattern max).
 */

#ifndef PRISM_PATTERN_STORAGE_H
#define PRISM_PATTERN_STORAGE_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize storage subsystem
 *
 * Mounts LittleFS at /littlefs (ADR-005) and validates partition.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_init(void);

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

/**
 * @brief Deinitialize storage subsystem
 *
 * Unmounts filesystem and cleanup.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // PRISM_PATTERN_STORAGE_H
