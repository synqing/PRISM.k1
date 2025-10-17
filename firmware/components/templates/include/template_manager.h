/**
 * @file template_manager.h
 * @brief Template pattern manager for PRISM K1
 *
 * Manages embedded template patterns for out-of-box functionality.
 * Implements ADR-006 (15 patterns minimum).
 */

#ifndef PRISM_TEMPLATE_MANAGER_H
#define PRISM_TEMPLATE_MANAGER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize template subsystem
 *
 * Loads embedded template catalog (15 patterns minimum, ADR-006).
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t templates_init(void);

/**
 * @brief Template task entry point
 *
 * FreeRTOS task for template generation and metadata management.
 *
 * Stack: 6KB
 * Priority: 3 (low)
 * Core: 0
 *
 * @param pvParameters Task parameters (unused)
 */
void templates_task(void *pvParameters);

/**
 * @brief Deinitialize template subsystem
 *
 * Cleanup template resources.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t templates_deinit(void);

/**
 * @brief List built-in templates, optionally filtered by category.
 *
 * Allocates an array of strings containing template IDs. Caller must free the
 * list and each string using free(). If category is NULL, returns all.
 *
 * @param category Optional category filter: "ambient", "energy", "special"
 * @param out_ids Output array of C strings (allocated)
 * @param out_count Output count of entries
 * @return ESP_OK on success, error otherwise
 */
esp_err_t templates_list(const char* category, char*** out_ids, size_t* out_count);

/**
 * @brief Deploy a template by ID (load and start playback).
 *
 * Attempts to load from RAM cache first; if not present, reads from
 * /littlefs/templates and warms the cache. Then passes the .prism blob to
 * the playback engine.
 *
 * @param template_id Template identifier (e.g., "flow-horizon")
 * @return ESP_OK on success, error otherwise
 */
esp_err_t templates_deploy(const char* template_id);

#ifdef __cplusplus
}
#endif

#endif // PRISM_TEMPLATE_MANAGER_H
