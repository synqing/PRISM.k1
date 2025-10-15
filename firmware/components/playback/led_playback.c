/**
 * @file led_playback.c
 * @brief LED playback implementation stub
 */

#include "led_playback.h"
#include "esp_log.h"

static const char *TAG = "playback";

esp_err_t playback_init(void) {
    ESP_LOGI(TAG, "Initializing playback subsystem...");
    // TODO: RMT peripheral configuration (task 8, ADR-003: 320 LEDs)
    // TODO: Effect engine setup (task 9)
    ESP_LOGI(TAG, "Playback subsystem initialized (stub)");
    return ESP_OK;
}

void playback_task(void *pvParameters) {
    ESP_LOGI(TAG, "Playback task started on core %d (HIGHEST priority)", xPortGetCoreID());

    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        // TODO: 60 FPS frame generation (task 8)
        // TODO: Effect engine execution (task 9)
        // TODO: RMT output (task 8)

        // 60 FPS = 16.67ms frame time
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(16));
    }

    ESP_LOGW(TAG, "Playback task exiting (unexpected)");
    vTaskDelete(NULL);
}

esp_err_t playback_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing playback subsystem...");
    // TODO: RMT cleanup
    return ESP_OK;
}
