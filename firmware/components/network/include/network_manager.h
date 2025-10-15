/**
 * @file network_manager.h
 * @brief Network management for PRISM K1 - WiFi & WebSocket TLV server
 *
 * Handles WiFi station mode connectivity and binary TLV WebSocket protocol.
 * Implements ADR-002 (4KB buffer, 2 max clients).
 */

#ifndef PRISM_NETWORK_MANAGER_H
#define PRISM_NETWORK_MANAGER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize network subsystem
 *
 * Sets up WiFi STA mode and prepares HTTPS/WebSocket server infrastructure.
 * Must be called before network_task().
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t network_init(void);

/**
 * @brief Network task entry point
 *
 * FreeRTOS task for WiFi management and WebSocket TLV server.
 * Handles client sessions, heartbeats, and binary protocol framing.
 *
 * Stack: 8KB
 * Priority: 5 (medium)
 * Core: 1 (network stack core)
 *
 * @param pvParameters Task parameters (unused)
 */
void network_task(void *pvParameters);

/**
 * @brief Deinitialize network subsystem
 *
 * Cleanup for testing/shutdown scenarios.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t network_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // PRISM_NETWORK_MANAGER_H
