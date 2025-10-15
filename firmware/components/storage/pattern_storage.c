/**
 * @file pattern_storage.c
 * @brief Pattern storage implementation - LittleFS mount and lifecycle
 *
 * Implements ADR-005 (mount path /littlefs), ADR-006 (15-25 patterns),
 * and ADR-007 (partition at 0x320000, 1.5MB).
 */

#include "pattern_storage.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_partition.h"

static const char *TAG = "storage";
static bool storage_initialized = false;

// ADR-005: Storage mount path
#define STORAGE_MOUNT_PATH  "/littlefs"
#define STORAGE_PARTITION   "littlefs"

// ADR-007: Partition configuration
#define EXPECTED_PARTITION_SIZE   0x180000  // 1.5MB
#define EXPECTED_PARTITION_OFFSET 0x320000  // 64KB aligned

esp_err_t storage_init(void) {
    if (storage_initialized) {
        ESP_LOGW(TAG, "Storage already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing LittleFS at %s", STORAGE_MOUNT_PATH);

    // Validate partition exists (ADR-007)
    const esp_partition_t *partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_SPIFFS,  // LittleFS uses SPIFFS subtype (0x82)
        STORAGE_PARTITION
    );

    if (!partition) {
        ESP_LOGE(TAG, "LittleFS partition '%s' not found!", STORAGE_PARTITION);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Found partition: offset=0x%lX size=0x%lX",
             (unsigned long)partition->address, (unsigned long)partition->size);

    // Verify partition size matches ADR-007
    if (partition->size != EXPECTED_PARTITION_SIZE) {
        ESP_LOGE(TAG, "Partition size mismatch! Expected 0x%X, got 0x%lX",
                 EXPECTED_PARTITION_SIZE, (unsigned long)partition->size);
        return ESP_ERR_INVALID_SIZE;
    }

    // Verify partition offset matches ADR-007 (64KB aligned)
    if (partition->address != EXPECTED_PARTITION_OFFSET) {
        ESP_LOGW(TAG, "Partition offset mismatch: Expected 0x%X, got 0x%lX",
                 EXPECTED_PARTITION_OFFSET, (unsigned long)partition->address);
        // Continue anyway - offset warning only
    }

    // Configure LittleFS
    esp_vfs_littlefs_conf_t conf = {
        .base_path = STORAGE_MOUNT_PATH,
        .partition_label = STORAGE_PARTITION,
        .format_if_mount_failed = true,  // Auto-format on first boot
        .dont_mount = false,
    };

    // Mount filesystem
    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS: %s", esp_err_to_name(ret));
        return ret;
    }

    // Get filesystem info
    size_t total = 0, used = 0;
    ret = esp_littlefs_info(STORAGE_PARTITION, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "LittleFS: total=%zu KB, used=%zu KB, free=%zu KB",
                 total / 1024, used / 1024, (total - used) / 1024);
    } else {
        ESP_LOGW(TAG, "Could not get filesystem info: %s", esp_err_to_name(ret));
    }

    storage_initialized = true;
    ESP_LOGI(TAG, "Storage subsystem initialized successfully");
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
    if (!storage_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Unmounting LittleFS");
    esp_err_t ret = esp_vfs_littlefs_unregister(STORAGE_PARTITION);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount LittleFS: %s", esp_err_to_name(ret));
        return ret;
    }

    storage_initialized = false;
    ESP_LOGI(TAG, "Storage subsystem deinitialized");
    return ESP_OK;
}
