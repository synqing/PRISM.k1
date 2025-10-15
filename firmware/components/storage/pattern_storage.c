/**
 * @file pattern_storage.c
 * @brief Pattern storage implementation stub
 */

#include "pattern_storage.h"
#include "esp_log.h"

static const char *TAG = "storage";

esp_err_t storage_init(void) {
    ESP_LOGI(TAG, "Initializing storage subsystem...");
    // TODO: LittleFS mount at /littlefs (task 5, ADR-005)
    // TODO: Partition validation (task 5)
    ESP_LOGI(TAG, "Storage subsystem initialized (stub)");
    return ESP_OK;
}

void storage_task(void *pvParameters) {
    ESP_LOGI(TAG, "Storage task started on core %d", xPortGetCoreID());

    while (1) {
        // TODO: Pattern file operations (task 5)
        // TODO: Pattern format validation (task 6)
        // TODO: Cache management (task 7)
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGW(TAG, "Storage task exiting (unexpected)");
    vTaskDelete(NULL);
}

esp_err_t storage_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing storage subsystem...");
    // TODO: Unmount filesystem
    return ESP_OK;
}
