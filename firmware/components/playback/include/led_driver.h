/**
 * @file led_driver.h
 * @brief Dual-channel RMT LED driver for PRISM K1
 *
 * Task 8: RMT LED driver for 2×160 WS2812B LEDs at 120 FPS
 * Based on proven Emotiscope ESP32-S3 reference implementation
 *
 * Hardware Configuration:
 * - Channel 1: GPIO 9  → 160 WS2812B LEDs (independent strip)
 * - Channel 2: GPIO 10 → 160 WS2812B LEDs (independent strip)
 * - Total: 320 LEDs across 2 physically separate strips
 *
 * Architecture:
 * - Dual independent RMT TX channels with separate encoders
 * - Asynchronous parallel transmission for performance
 * - Double-buffered per channel (front/back buffers)
 * - Hardware timer-driven 120 FPS frame submission
 * - GRB color order (not RGB!)
 *
 * Memory footprint: ~6.5KB (4×480 byte buffers + overhead)
 */

#ifndef PRISM_LED_DRIVER_H
#define PRISM_LED_DRIVER_H

#include "esp_err.h"
#include "prism_config.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Hardware Configuration (Dual Independent Channels)
 */
#define LED_GPIO_CH1        9       ///< GPIO pin for Channel 1 data output
#define LED_GPIO_CH2        10      ///< GPIO pin for Channel 2 data output
#define LED_COUNT_PER_CH    160     ///< LEDs per channel
#define LED_TOTAL_COUNT     320     ///< Total LEDs (2 channels)

/**
 * Color Format (GRB order for WS2812B)
 */
#define LED_BYTES_PER_LED   3       ///< GRB bytes per LED
#define LED_FRAME_SIZE_CH   (LED_COUNT_PER_CH * LED_BYTES_PER_LED)  ///< 480 bytes per channel

/**
 * Timing Configuration
 */
// Use global LED_FPS_TARGET from prism_config.h (generated from CANON)
#define LED_FRAME_TIME_MS   (1000 / LED_FPS_TARGET)

/**
 * RMT Configuration (proven values from Emotiscope)
 */
#define LED_RMT_RESOLUTION_HZ   10000000    ///< 10MHz tick resolution (0.1us per tick)
#define LED_RMT_MEM_BLOCKS      128         ///< RMT memory blocks per channel

/**
 * Channel identifier
 */
typedef enum {
    LED_CHANNEL_1 = 0,  ///< First independent channel (GPIO 9)
    LED_CHANNEL_2 = 1,  ///< Second independent channel (GPIO 10)
    LED_CHANNEL_COUNT = 2
} led_channel_t;

/**
 * Driver statistics per channel
 */
typedef struct {
    uint32_t frames_transmitted;    ///< Total frames sent to LEDs
    uint32_t underruns;              ///< Frames that exceeded deadline
    uint32_t max_frame_time_us;     ///< Maximum observed frame time
    uint32_t avg_frame_time_us;     ///< Average frame time
} led_channel_stats_t;

/**
 * Global driver statistics
 */
typedef struct {
    led_channel_stats_t ch1;        ///< Channel 1 statistics
    led_channel_stats_t ch2;        ///< Channel 2 statistics
    uint32_t total_buffer_swaps;    ///< Total buffer swaps across both channels
    bool is_running;                 ///< Driver active status
} led_driver_stats_t;

/**
 * @brief Initialize the dual-channel LED driver
 *
 * Configures both RMT channels, allocates buffers, prepares for 120 FPS operation.
 * Based on proven Emotiscope initialization pattern.
 *
 * @return ESP_OK on success
 *         ESP_ERR_NO_MEM if buffer allocation fails
 *         ESP_ERR_INVALID_STATE if already initialized
 */
esp_err_t led_driver_init(void);

/**
 * @brief Start LED output transmission on both channels
 *
 * Begins 120 FPS refresh task with asynchronous parallel transmission.
 * Must be called after led_driver_init().
 *
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_STATE if not initialized
 */
esp_err_t led_driver_start(void);

/**
 * @brief Submit frame data to a specific channel
 *
 * Copies GRB24 frame data to the back buffer of specified channel.
 * Channels operate independently - submit to each channel separately.
 *
 * @param channel Channel to submit to (LED_CHANNEL_1 or LED_CHANNEL_2)
 * @param frame Pointer to GRB24 frame data (LED_FRAME_SIZE_CH bytes)
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if frame is NULL or channel invalid
 *         ESP_ERR_INVALID_STATE if driver not initialized
 */
esp_err_t led_driver_submit_frame(led_channel_t channel, const uint8_t *frame);

/**
 * @brief Submit frames to both channels simultaneously
 *
 * Convenience function to update both channels at once.
 * Both frames will be swapped together on next refresh.
 *
 * @param frame_ch1 Pointer to Channel 1 GRB24 data (480 bytes)
 * @param frame_ch2 Pointer to Channel 2 GRB24 data (480 bytes)
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if either frame is NULL
 *         ESP_ERR_INVALID_STATE if driver not initialized
 */
esp_err_t led_driver_submit_frames(const uint8_t *frame_ch1, const uint8_t *frame_ch2);

/**
 * @brief Get current driver statistics
 *
 * @param stats Pointer to statistics structure to fill
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if stats is NULL
 */
esp_err_t led_driver_get_stats(led_driver_stats_t *stats);

/**
 * @brief Reset driver statistics
 *
 * Clears all counters and timing measurements for both channels.
 *
 * @return ESP_OK on success
 */
esp_err_t led_driver_reset_stats(void);

/**
 * @brief Stop LED output transmission
 *
 * Stops refresh task and clears all LEDs on both channels.
 *
 * @return ESP_OK on success
 */
esp_err_t led_driver_stop(void);

/**
 * @brief Deinitialize LED driver
 *
 * Releases all resources for both channels.
 * Driver must be stopped before calling this.
 *
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_STATE if driver is still running
 */
esp_err_t led_driver_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // PRISM_LED_DRIVER_H
