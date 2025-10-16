/**
 * @file led_driver.c
 * @brief Dual-channel RMT LED driver implementation
 *
 * Task 8: Complete dual-channel LED driver based on proven Emotiscope patterns
 * Implements all 5 subtasks with battle-tested ESP32-S3 code
 */

#include "led_driver.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "led_driver";

/**
 * WS2812B Timing Configuration
 * PROVEN VALUES from Emotiscope - DO NOT MODIFY without hardware verification
 *
 * At 10MHz (0.1us ticks):
 * bit0: {duration0=4 ticks (0.4us high), level0=1, duration1=6 ticks (0.6us low), level1=0}
 * bit1: {duration0=7 ticks (0.7us high), level0=1, duration1=6 ticks (0.6us low), level1=0}
 * reset: {duration0=250 ticks (25us low), level0=0, duration1=250 ticks (25us low), level1=0}
 */

/**
 * Subtask 8.1: WS2812B Encoder (Emotiscope-proven pattern)
 */
typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;

/**
 * Channel State (Subtask 8.2: Double buffers per channel)
 */
typedef struct {
    // RMT hardware (Subtask 8.1)
    rmt_channel_handle_t rmt_channel;
    rmt_encoder_handle_t encoder;

    // Double buffers - static allocation (Emotiscope pattern)
    uint8_t front_buffer[LED_FRAME_SIZE_CH];  // Currently transmitting (GRB order!)
    uint8_t back_buffer[LED_FRAME_SIZE_CH];   // Being filled by effects engine
    bool back_buffer_ready;

    // Statistics (Subtask 8.5)
    led_channel_stats_t stats;
} led_channel_state_t;

/**
 * Driver State (Subtask 8.4: Timing and synchronization)
 */
typedef struct {
    led_channel_state_t channels[LED_CHANNEL_COUNT];

    // Synchronization (Subtask 8.4)
    SemaphoreHandle_t state_mutex;
    esp_timer_handle_t frame_timer;

    // Global stats
    uint32_t total_buffer_swaps;

    // State flags
    bool initialized;
    bool running;
} led_driver_state_t;

static led_driver_state_t s_driver = {0};

// RMT TX configuration (shared by both channels)
static rmt_transmit_config_t s_tx_config = {
    .loop_count = 0,  // No transfer loop
    .flags = {
        .eot_level = 0,
        .queue_nonblocking = 0,
    },
};

/**
 * SUBTASK 8.1: WS2812B Encoder Implementation
 * IRAM_ATTR for performance (Emotiscope pattern)
 */
IRAM_ATTR static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                                               const void *primary_data, size_t data_size,
                                               rmt_encode_state_t *ret_state)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;

    switch (led_encoder->state) {
    case 0: // Send GRB data
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = 1; // Move to reset phase
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state = (rmt_encode_state_t)(state | (uint32_t)RMT_ENCODING_MEM_FULL);
            goto out;
        }
        // fall-through
    case 1: // Send reset code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                                                 sizeof(led_encoder->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = RMT_ENCODING_RESET;
            state = (rmt_encode_state_t)(state | (uint32_t)RMT_ENCODING_COMPLETE);
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state = (rmt_encode_state_t)(state | (uint32_t)RMT_ENCODING_MEM_FULL);
            goto out;
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

/**
 * Subtask 8.1: Create WS2812B encoder with PROVEN timing
 */
static esp_err_t rmt_new_led_strip_encoder(rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_led_strip_encoder_t *led_encoder = NULL;

    ESP_GOTO_ON_FALSE(ret_encoder, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");

    led_encoder = calloc(1, sizeof(rmt_led_strip_encoder_t));
    ESP_GOTO_ON_FALSE(led_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for led strip encoder");

    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    led_encoder->base.reset = rmt_led_strip_encoder_reset;

    // PROVEN WS2812B timing from Emotiscope (10MHz resolution)
    // DO NOT MODIFY without hardware testing
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 4,  // 0.4us high
            .level1 = 0,
            .duration1 = 6,  // 0.6us low
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 7,  // 0.7us high
            .level1 = 0,
            .duration1 = 6,  // 0.6us low
        },
        .flags.msb_first = 1,
    };
    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder),
                      err, TAG, "create bytes encoder failed");

    // Copy encoder for reset code
    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder),
                      err, TAG, "create copy encoder failed");

    // Reset code: >50us low (WS2812B spec)
    led_encoder->reset_code = (rmt_symbol_word_t){
        .level0 = 0,
        .duration0 = 250,  // 25us
        .level1 = 0,
        .duration1 = 250,  // 25us (total 50us)
    };

    *ret_encoder = &led_encoder->base;
    return ESP_OK;

err:
    if (led_encoder) {
        if (led_encoder->bytes_encoder) {
            rmt_del_encoder(led_encoder->bytes_encoder);
        }
        if (led_encoder->copy_encoder) {
            rmt_del_encoder(led_encoder->copy_encoder);
        }
        free(led_encoder);
    }
    return ret;
}

/**
 * SUBTASK 8.4: Frame Timer Callback (ISR Context)
 * Triggers frame transmission at target FPS
 */
static void frame_timer_callback(void *arg)
{
    // Signal refresh task that it's time for next frame
    // Using task notification instead of semaphore (lighter weight)
    BaseType_t higher_priority_task_woken = pdFALSE;
    TaskHandle_t *refresh_task_handle = (TaskHandle_t *)arg;
    vTaskNotifyGiveFromISR(*refresh_task_handle, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

/**
 * SUBTASK 8.4: LED Refresh Task
 * Asynchronously transmits frames to both channels at target FPS
 * IRAM_ATTR for performance
 */
IRAM_ATTR static void led_refresh_task(void *arg)
{
    ESP_LOGI(TAG, "LED refresh task started on core %d", xPortGetCoreID());

    while (s_driver.running) {
        // Wait for frame timer signal (Subtask 8.4) - cadence per LED_FRAME_TIME_MS
        uint32_t notification_value = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(LED_FRAME_TIME_MS * 2));

        if (notification_value == 0) {
            // Timeout - no timer signal
            continue;
        }

        uint64_t frame_start_us = esp_timer_get_time();

        // Wait for previous frame transmission to complete on both channels
        rmt_tx_wait_all_done(s_driver.channels[LED_CHANNEL_1].rmt_channel, -1);  // Infinite wait
        rmt_tx_wait_all_done(s_driver.channels[LED_CHANNEL_2].rmt_channel, -1);

        // Swap buffers if new frames are ready (Subtask 8.3)
        xSemaphoreTake(s_driver.state_mutex, portMAX_DELAY);

        for (int ch = 0; ch < LED_CHANNEL_COUNT; ch++) {
            led_channel_state_t *channel = &s_driver.channels[ch];
            if (channel->back_buffer_ready) {
                // Atomic swap
                uint8_t temp[LED_FRAME_SIZE_CH];
                memcpy(temp, channel->front_buffer, LED_FRAME_SIZE_CH);
                memcpy(channel->front_buffer, channel->back_buffer, LED_FRAME_SIZE_CH);
                memcpy(channel->back_buffer, temp, LED_FRAME_SIZE_CH);
                channel->back_buffer_ready = false;
                s_driver.total_buffer_swaps++;
            }
        }

        xSemaphoreGive(s_driver.state_mutex);

        // Transmit both channels in parallel (asynchronous) - Subtask 8.4
        ESP_ERROR_CHECK(rmt_transmit(s_driver.channels[LED_CHANNEL_1].rmt_channel,
                                     s_driver.channels[LED_CHANNEL_1].encoder,
                                     s_driver.channels[LED_CHANNEL_1].front_buffer,
                                     LED_FRAME_SIZE_CH,
                                     &s_tx_config));

        ESP_ERROR_CHECK(rmt_transmit(s_driver.channels[LED_CHANNEL_2].rmt_channel,
                                     s_driver.channels[LED_CHANNEL_2].encoder,
                                     s_driver.channels[LED_CHANNEL_2].front_buffer,
                                     LED_FRAME_SIZE_CH,
                                     &s_tx_config));

        // Update statistics (Subtask 8.5)
        uint64_t frame_time_us = esp_timer_get_time() - frame_start_us;

        xSemaphoreTake(s_driver.state_mutex, portMAX_DELAY);

        for (int ch = 0; ch < LED_CHANNEL_COUNT; ch++) {
            led_channel_stats_t *stats = &s_driver.channels[ch].stats;
            stats->frames_transmitted++;

            if (frame_time_us > LED_FRAME_TIME_MS * 1000) {
                stats->underruns++;
            }

            if (frame_time_us > stats->max_frame_time_us) {
                stats->max_frame_time_us = (uint32_t)frame_time_us;
            }

            // Running average
            if (stats->frames_transmitted > 0) {
                stats->avg_frame_time_us =
                    (stats->avg_frame_time_us * (stats->frames_transmitted - 1) + frame_time_us)
                    / stats->frames_transmitted;
            }
        }

        xSemaphoreGive(s_driver.state_mutex);

        // Log underruns (Subtask 8.5)
        if (frame_time_us > LED_FRAME_TIME_MS * 1000) {
            ESP_LOGW(TAG, "Frame underrun: %llu us (target: %d ms)", frame_time_us, LED_FRAME_TIME_MS);
        }
    }

    ESP_LOGI(TAG, "LED refresh task exiting");
    vTaskDelete(NULL);
}

/**
 * SUBTASK 8.1 & 8.2: Initialize Dual-Channel LED Driver
 */
esp_err_t led_driver_init(void)
{
    if (s_driver.initialized) {
        ESP_LOGW(TAG, "LED driver already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Initializing dual-channel LED driver:");
    ESP_LOGI(TAG, "  Channel 1: GPIO %d → %d LEDs", LED_GPIO_CH1, LED_COUNT_PER_CH);
    ESP_LOGI(TAG, "  Channel 2: GPIO %d → %d LEDs", LED_GPIO_CH2, LED_COUNT_PER_CH);
    ESP_LOGI(TAG, "  Total: %d LEDs @ %d FPS", LED_TOTAL_COUNT, LED_FPS_TARGET);

    // Subtask 8.4: Create synchronization primitives
    s_driver.state_mutex = xSemaphoreCreateMutex();
    if (!s_driver.state_mutex) {
        ESP_LOGE(TAG, "Failed to create state mutex");
        return ESP_ERR_NO_MEM;
    }

    // Subtask 8.1: Configure both RMT channels with PROVEN settings
    const gpio_num_t channel_gpios[LED_CHANNEL_COUNT] = {LED_GPIO_CH1, LED_GPIO_CH2};

    for (int ch = 0; ch < LED_CHANNEL_COUNT; ch++) {
        led_channel_state_t *channel = &s_driver.channels[ch];

        // RMT TX channel config (Emotiscope-proven)
        rmt_tx_channel_config_t tx_config = {
            .gpio_num = channel_gpios[ch],
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .resolution_hz = LED_RMT_RESOLUTION_HZ,  // 10MHz
            .mem_block_symbols = LED_RMT_MEM_BLOCKS,  // 128 blocks
            .trans_queue_depth = 4,
            .intr_priority = 99,  // Emotiscope pattern
            .flags = {
                .invert_out = 0,
                .with_dma = 0,  // NO DMA (Emotiscope pattern)
                // Note: .io_loop_back and .io_od_mode removed in ESP-IDF v5.x
                //       Use gpio_set_pull_mode() or gpio_set_drive_capability() if needed
            },
        };

        ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&tx_config, &channel->rmt_channel),
                           TAG, "create RMT TX channel %d failed", ch);

        // Create WS2812B encoder
        ESP_RETURN_ON_ERROR(rmt_new_led_strip_encoder(&channel->encoder),
                           TAG, "create LED encoder %d failed", ch);

        // Enable RMT channel
        ESP_RETURN_ON_ERROR(rmt_enable(channel->rmt_channel),
                           TAG, "enable RMT channel %d failed", ch);

        // Subtask 8.2: Initialize buffers (static, no allocation needed!)
        memset(channel->front_buffer, 0, LED_FRAME_SIZE_CH);
        memset(channel->back_buffer, 0, LED_FRAME_SIZE_CH);
        channel->back_buffer_ready = false;

        // Subtask 8.5: Initialize statistics
        memset(&channel->stats, 0, sizeof(led_channel_stats_t));

        ESP_LOGI(TAG, "Channel %d initialized: GPIO %d, %d LEDs (%d bytes)",
                 ch, channel_gpios[ch], LED_COUNT_PER_CH, LED_FRAME_SIZE_CH);
    }

    s_driver.total_buffer_swaps = 0;
    s_driver.initialized = true;

    ESP_LOGI(TAG, "Dual-channel LED driver initialized successfully");
    ESP_LOGI(TAG, "Total memory: %d bytes (4×%d byte buffers)",
             4 * LED_FRAME_SIZE_CH, LED_FRAME_SIZE_CH);

    return ESP_OK;
}

/**
 * SUBTASK 8.4: Start LED Output
 */
esp_err_t led_driver_start(void)
{
    if (!s_driver.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_driver.running) {
        ESP_LOGW(TAG, "LED driver already running");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting dual-channel LED output @ %d FPS", LED_FPS_TARGET);

    s_driver.running = true;

    // Create refresh task (Subtask 8.4)
    static TaskHandle_t refresh_task_handle;
    BaseType_t result = xTaskCreatePinnedToCore(
        led_refresh_task,
        "led_refresh",
        4096,
        NULL,
        10,  // HIGHEST priority for real-time
        &refresh_task_handle,
        0    // Core 0 for real-time tasks
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LED refresh task");
        s_driver.running = false;
        return ESP_FAIL;
    }

    // Start frame timer (Subtask 8.4)
    esp_timer_create_args_t timer_args = {
        .callback = frame_timer_callback,
        .arg = &refresh_task_handle,
        .name = "led_frame_timer",
        .skip_unhandled_events = true,
    };

    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_args, &s_driver.frame_timer),
                       TAG, "create frame timer failed");

    // Start periodic timer at 60 FPS
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(s_driver.frame_timer, LED_FRAME_TIME_MS * 1000),
                       TAG, "start frame timer failed");

    ESP_LOGI(TAG, "Dual-channel LED driver started (frame period: %d ms)", LED_FRAME_TIME_MS);

    return ESP_OK;
}

/**
 * SUBTASK 8.3: Submit Frame to Specific Channel
 */
esp_err_t led_driver_submit_frame(led_channel_t channel, const uint8_t *frame)
{
    if (!s_driver.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (frame == NULL || channel >= LED_CHANNEL_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    // Copy frame to back buffer (Subtask 8.3)
    xSemaphoreTake(s_driver.state_mutex, portMAX_DELAY);
    memcpy(s_driver.channels[channel].back_buffer, frame, LED_FRAME_SIZE_CH);
    s_driver.channels[channel].back_buffer_ready = true;
    xSemaphoreGive(s_driver.state_mutex);

    return ESP_OK;
}

/**
 * SUBTASK 8.3: Submit Frames to Both Channels
 */
esp_err_t led_driver_submit_frames(const uint8_t *frame_ch1, const uint8_t *frame_ch2)
{
    if (!s_driver.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (frame_ch1 == NULL || frame_ch2 == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Copy both frames atomically
    xSemaphoreTake(s_driver.state_mutex, portMAX_DELAY);

    memcpy(s_driver.channels[LED_CHANNEL_1].back_buffer, frame_ch1, LED_FRAME_SIZE_CH);
    s_driver.channels[LED_CHANNEL_1].back_buffer_ready = true;

    memcpy(s_driver.channels[LED_CHANNEL_2].back_buffer, frame_ch2, LED_FRAME_SIZE_CH);
    s_driver.channels[LED_CHANNEL_2].back_buffer_ready = true;

    xSemaphoreGive(s_driver.state_mutex);

    return ESP_OK;
}

/**
 * SUBTASK 8.5: Get Driver Statistics
 */
esp_err_t led_driver_get_stats(led_driver_stats_t *stats)
{
    if (stats == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_driver.state_mutex, portMAX_DELAY);

    stats->ch1 = s_driver.channels[LED_CHANNEL_1].stats;
    stats->ch2 = s_driver.channels[LED_CHANNEL_2].stats;
    stats->total_buffer_swaps = s_driver.total_buffer_swaps;
    stats->is_running = s_driver.running;

    xSemaphoreGive(s_driver.state_mutex);

    return ESP_OK;
}

/**
 * SUBTASK 8.5: Reset Statistics
 */
esp_err_t led_driver_reset_stats(void)
{
    xSemaphoreTake(s_driver.state_mutex, portMAX_DELAY);

    for (int ch = 0; ch < LED_CHANNEL_COUNT; ch++) {
        memset(&s_driver.channels[ch].stats, 0, sizeof(led_channel_stats_t));
    }
    s_driver.total_buffer_swaps = 0;

    xSemaphoreGive(s_driver.state_mutex);

    ESP_LOGI(TAG, "Driver statistics reset");
    return ESP_OK;
}

/**
 * Stop LED Output
 */
esp_err_t led_driver_stop(void)
{
    if (!s_driver.running) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping dual-channel LED driver");

    // Stop timer
    if (s_driver.frame_timer) {
        esp_timer_stop(s_driver.frame_timer);
        esp_timer_delete(s_driver.frame_timer);
        s_driver.frame_timer = NULL;
    }

    // Signal task to exit
    s_driver.running = false;

    // Wait for task to exit
    vTaskDelay(pdMS_TO_TICKS(100));

    // Clear all LEDs on both channels
    for (int ch = 0; ch < LED_CHANNEL_COUNT; ch++) {
        memset(s_driver.channels[ch].front_buffer, 0, LED_FRAME_SIZE_CH);
        rmt_transmit(s_driver.channels[ch].rmt_channel,
                    s_driver.channels[ch].encoder,
                    s_driver.channels[ch].front_buffer,
                    LED_FRAME_SIZE_CH,
                    &s_tx_config);
        rmt_tx_wait_all_done(s_driver.channels[ch].rmt_channel, LED_FRAME_TIME_MS);
    }

    ESP_LOGI(TAG, "Dual-channel LED driver stopped");
    return ESP_OK;
}

/**
 * Deinitialize LED Driver
 */
esp_err_t led_driver_deinit(void)
{
    if (s_driver.running) {
        ESP_LOGE(TAG, "Cannot deinit while running. Call led_driver_stop() first");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Deinitializing dual-channel LED driver");

    // Destroy synchronization primitives
    if (s_driver.state_mutex) {
        vSemaphoreDelete(s_driver.state_mutex);
        s_driver.state_mutex = NULL;
    }

    // Disable and delete RMT resources for both channels
    for (int ch = 0; ch < LED_CHANNEL_COUNT; ch++) {
        led_channel_state_t *channel = &s_driver.channels[ch];

        if (channel->rmt_channel) {
            rmt_disable(channel->rmt_channel);
            rmt_del_channel(channel->rmt_channel);
            channel->rmt_channel = NULL;
        }

        if (channel->encoder) {
            rmt_del_encoder(channel->encoder);
            channel->encoder = NULL;
        }
    }

    s_driver.initialized = false;
    ESP_LOGI(TAG, "Dual-channel LED driver deinitialized");

    return ESP_OK;
}
