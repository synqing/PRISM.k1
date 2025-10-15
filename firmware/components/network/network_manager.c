/**
 * @file network_manager.c
 * @brief Network management implementation stub
 */

#include "network_manager.h"
#include "esp_log.h"

static const char *TAG = "network";

esp_err_t network_init(void) {
    ESP_LOGI(TAG, "Initializing network subsystem...");
    // TODO: WiFi initialization (task 2)
    // TODO: WebSocket server setup (task 3)
    ESP_LOGI(TAG, "Network subsystem initialized (stub)");
    return ESP_OK;
}

void network_task(void *pvParameters) {
    ESP_LOGI(TAG, "Network task started on core %d", xPortGetCoreID());

    while (1) {
        // TODO: WiFi lifecycle management (task 2)
        // TODO: WebSocket TLV protocol handling (tasks 3, 4)
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGW(TAG, "Network task exiting (unexpected)");
    vTaskDelete(NULL);
}

esp_err_t network_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing network subsystem...");
    // TODO: Cleanup implementation
    return ESP_OK;
}
