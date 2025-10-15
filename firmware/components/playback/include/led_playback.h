/**
 * @file led_playback.h
 * @brief LED playback engine for PRISM K1
 *
 * Real-time LED output using RMT peripheral.
 * Implements ADR-003 (320 LEDs) with 60 FPS target.
 */

#ifndef PRISM_LED_PLAYBACK_H
#define PRISM_LED_PLAYBACK_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize playback subsystem
 *
 * Configures RMT peripheral for 320 WS2812B LEDs (ADR-003).
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t playback_init(void);

/**
 * @brief Playback task entry point
 *
 * FreeRTOS task for 60 FPS LED frame generation and RMT output.
 * HIGHEST priority - must never block for >16ms.
 *
 * Stack: 8KB
 * Priority: 10 (HIGHEST)
 * Core: 0 (dedicated for real-time)
 *
 * @param pvParameters Task parameters (unused)
 */
void playback_task(void *pvParameters);

/**
 * @brief Deinitialize playback subsystem
 *
 * Cleanup RMT resources.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t playback_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // PRISM_LED_PLAYBACK_H
