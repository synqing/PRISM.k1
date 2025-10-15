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

#ifdef __cplusplus
}
#endif

#endif // PRISM_TEMPLATE_MANAGER_H
