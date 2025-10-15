/**
 * PRISM K1 Firmware - Main Entry Point
 * ESP32-S3 LED Controller
 *
 * Copyright (c) 2025 PRISM
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

static const char *TAG = "PRISM-K1";

/* Firmware version */
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_BUILD   __DATE__ " " __TIME__

/* Task priorities */
#define PRIORITY_NETWORK    5
#define PRIORITY_STORAGE    4
#define PRIORITY_PLAYBACK   6
#define PRIORITY_WEBSOCKET  5

/* Task stack sizes */
#define STACK_NETWORK       4096
#define STACK_STORAGE       3072
#define STACK_PLAYBACK      4096
#define STACK_WEBSOCKET     6144

/**
 * System initialization
 */
static esp_err_t system_init(void)
{
    esp_err_t ret;

    /* Initialize NVS */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Initialize TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Create default event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "System initialized");
    return ESP_OK;
}

/**
 * Print system information
 */
static void print_system_info(void)
{
    /* Print header */
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "PRISM K1 LED Controller");
    ESP_LOGI(TAG, "Firmware: v%s", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "Build: %s", FIRMWARE_BUILD);
    ESP_LOGI(TAG, "========================================");

    /* Chip info */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    ESP_LOGI(TAG, "Chip: ESP32-S3");
    ESP_LOGI(TAG, "Cores: %d", chip_info.cores);
    ESP_LOGI(TAG, "Features:%s%s%s%s",
             (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? " WiFi" : "",
             (chip_info.features & CHIP_FEATURE_BT) ? " BT" : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? " BLE" : "",
             (chip_info.features & CHIP_FEATURE_IEEE802154) ? " 802.15.4" : "");

    /* Memory info */
    ESP_LOGI(TAG, "Flash size: %dMB %s",
             spi_flash_get_chip_size() / (1024 * 1024),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "(embedded)" : "(external)");

    ESP_LOGI(TAG, "Free heap: %ld bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Min free heap: %ld bytes", esp_get_minimum_free_heap_size());

    ESP_LOGI(TAG, "========================================");
}

/**
 * Heap monitoring task
 */
static void heap_monitor_task(void *pvParameters)
{
    const TickType_t delay = pdMS_TO_TICKS(10000);  // Log every 10 seconds

    while (1) {
        size_t free_heap = esp_get_free_heap_size();
        size_t min_heap = esp_get_minimum_free_heap_size();
        size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

        ESP_LOGI(TAG, "Heap - Free: %d, Min: %d, Largest: %d",
                 free_heap, min_heap, largest_block);

        /* Check for low memory */
        if (free_heap < 50000) {
            ESP_LOGW(TAG, "Low memory warning! Free: %d bytes", free_heap);
        }

        vTaskDelay(delay);
    }
}

/**
 * Main application entry point
 */
void app_main(void)
{
    /* Print system info */
    print_system_info();

    /* Initialize system */
    ESP_ERROR_CHECK(system_init());

    /* TODO: Initialize components */
    // ESP_ERROR_CHECK(network_init());
    // ESP_ERROR_CHECK(storage_init());
    // ESP_ERROR_CHECK(playback_init());
    // ESP_ERROR_CHECK(websocket_init());
    // ESP_ERROR_CHECK(templates_init());

    /* Create heap monitoring task */
    xTaskCreate(heap_monitor_task, "heap_monitor", 2048, NULL, 1, NULL);

    /* TODO: Create main tasks */
    // xTaskCreate(network_task, "network", STACK_NETWORK, NULL, PRIORITY_NETWORK, NULL);
    // xTaskCreate(storage_task, "storage", STACK_STORAGE, NULL, PRIORITY_STORAGE, NULL);
    // xTaskCreate(playback_task, "playback", STACK_PLAYBACK, NULL, PRIORITY_PLAYBACK, NULL);
    // xTaskCreate(websocket_task, "websocket", STACK_WEBSOCKET, NULL, PRIORITY_WEBSOCKET, NULL);

    ESP_LOGI(TAG, "PRISM K1 started successfully!");

    /* Main loop - currently just prevents task deletion */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}