/**
 * @file led_playback.h
 * @brief LED playback engine for PRISM K1
 *
 * Real-time LED output using RMT peripheral.
 * Implements ADR-003 (320 LEDs) with 120 FPS target (ADR-008 update).
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
 * FreeRTOS task for 120 FPS LED frame generation and RMT output.
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

/**
 * Playback control API (built-in effects)
 */

/**
 * @brief Start playing a built-in effect with optional parameters.
 *
 * If the playback subsystem and LED driver are not yet running, they will be
 * initialized and started. Parameters are normalized 0-255 values; up to 8
 * parameters are supported, excess are ignored.
 *
 * @param effect_id Built-in effect ID (e.g., 0x0001 WAVE_SINGLE)
 * @param params Optional pointer to parameter array (can be NULL)
 * @param param_count Number of parameters provided (0-8)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t playback_play_builtin(uint16_t effect_id, const uint8_t* params, uint8_t param_count);

/**
 * @brief Stop current playback (keeps LED driver running, clears frame).
 * @return ESP_OK on success
 */
esp_err_t playback_stop(void);

/**
 * @brief Query whether playback is active.
 * @return true if an effect is currently running
 */
bool playback_is_running(void);

// Profiling metrics accessors (available when CONFIG_PRISM_PROFILE_TEMPORAL=y)
typedef struct {
    uint32_t samples;
    uint32_t min_cycles;
    uint32_t max_cycles;
    uint32_t avg_cycles;
    // Data cache
    uint64_t dcache_hits;
    uint64_t dcache_misses;
    uint32_t dcache_hit_pct;
    // Instruction cache
    uint64_t icache_hits;
    uint64_t icache_misses;
    uint32_t icache_hit_pct;
    // Instructions
    uint64_t insn_count;
    uint32_t ipc_x100; // scaled IPC (instructions per cycle * 100)
} prism_wave_metrics_t;

// Snapshot current metrics into out. Returns ESP_ERR_INVALID_STATE if profiling disabled.
esp_err_t playback_get_wave_metrics(prism_wave_metrics_t *out);

// Register CLI command `prism_metrics` to print current metrics
esp_err_t playback_register_cli(void);

#ifdef __cplusplus
}
#endif

#endif // PRISM_LED_PLAYBACK_H
