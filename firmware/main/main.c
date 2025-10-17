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
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_heap_caps.h"
#include "prism_memory_pool.h"
#include "prism_heap_monitor.h"
#include "network_manager.h"
#include "pattern_storage.h"
#include "led_playback.h"
#include "template_manager.h"
#include "bench_decode.h"
// UART test mode
void uart_test_start(void);

static const char *TAG = "PRISM-K1";

/* Firmware version */
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_BUILD   __DATE__ " " __TIME__

/* Task priorities (ADR: Playback is HIGHEST for 60 FPS real-time) */
#define PRIORITY_PLAYBACK   10  /* HIGHEST - Real-time LED output */
#define PRIORITY_NETWORK    5   /* Medium - WiFi & WebSocket */
#define PRIORITY_STORAGE    4   /* Medium-low - File operations */
#define PRIORITY_TEMPLATES  3   /* Low - Template generation */

/* Task stack sizes (KB) */
#define STACK_PLAYBACK      (8 * 1024)  /* 8KB - Frame generation */
#define STACK_NETWORK       (8 * 1024)  /* 8KB - Network stack */
#define STACK_STORAGE       (6 * 1024)  /* 6KB - File operations */
#define STACK_TEMPLATES     (6 * 1024)  /* 6KB - Template work */

/**
 * System initialization
 */
static esp_err_t system_init(void)
{
    esp_err_t ret;

    /* CRITICAL: Initialize memory pools FIRST before any other allocations */
    ESP_LOGI(TAG, "Initializing memory pools...");
    ret = prism_pool_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize memory pools!");
        return ret;
    }

    /* Dump initial pool state */
    prism_pool_dump_state();

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

    /* Initialize heap monitoring system */
    ESP_LOGI(TAG, "Initializing heap monitor...");
    ret = prism_heap_monitor_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize heap monitor!");
        return ret;
    }

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

    /* Flash info */
    uint32_t flash_size;
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        ESP_LOGI(TAG, "Flash size: %lu MB %s",
                 flash_size / (1024 * 1024),
                 (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "(embedded)" : "(external)");
    }

    ESP_LOGI(TAG, "Free heap: %ld bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Min free heap: %ld bytes", esp_get_minimum_free_heap_size());

    ESP_LOGI(TAG, "========================================");
}

/**
 * Statistics reporting task
 * Dumps detailed heap statistics every 30 seconds
 */
static void stats_reporting_task(void *pvParameters)
{
    const TickType_t delay = pdMS_TO_TICKS(30000);  // Report every 30 seconds

    while (1) {
        vTaskDelay(delay);

        ESP_LOGI(TAG, "========== System Statistics ==========");

        /* Dump comprehensive heap monitor statistics */
        prism_heap_monitor_dump_stats();

        /* Dump memory pool state */
        prism_pool_dump_state();

        ESP_LOGI(TAG, "=====================================");
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

#if CONFIG_PRISM_BENCH_AUTORUN
    ESP_LOGI(TAG, "PRISM decode bench autorun enabled; running before startup...");
    (void)bench_decode_run();
#endif

    /* Initialize components (Task 1: Component Scaffolding) */
    ESP_LOGI(TAG, "Initializing firmware components...");
    ESP_ERROR_CHECK(network_init());    /* WiFi & WebSocket (Task 2 complete) */
    ESP_ERROR_CHECK(storage_init());    /* LittleFS (Task 5 subtasks 5.1-5.4 complete) */
    ESP_ERROR_CHECK(playback_init());   /* RMT LED Driver (Task 8 complete) */
    ESP_ERROR_CHECK(templates_init());  /* Template catalog (stub) */
    ESP_LOGI(TAG, "All components initialized");

#if CONFIG_PRISM_UART_TEST
    ESP_LOGI(TAG, "Starting UART test mode...");
    uart_test_start();
#endif

    /* Create statistics reporting task (heap monitor runs automatically) */
    xTaskCreate(stats_reporting_task, "stats_report", 2048, NULL, 1, NULL);

    /* Create main FreeRTOS tasks (Task 1: Runtime Orchestration) */
    ESP_LOGI(TAG, "Creating FreeRTOS tasks...");

    /* Playback task - HIGHEST priority (Core 0, Priority 10) */
    xTaskCreatePinnedToCore(playback_task, "playback", STACK_PLAYBACK,
                            NULL, PRIORITY_PLAYBACK, NULL, 0);

    /* Network task - Medium priority (Core 1, Priority 5) */
    xTaskCreatePinnedToCore(network_task, "network", STACK_NETWORK,
                            NULL, PRIORITY_NETWORK, NULL, 1);

    /* Storage task - Medium-low priority (Core 0, Priority 4) */
    xTaskCreatePinnedToCore(storage_task, "storage", STACK_STORAGE,
                            NULL, PRIORITY_STORAGE, NULL, 0);

    /* Templates task - Low priority (Core 0, Priority 3) */
    xTaskCreatePinnedToCore(templates_task, "templates", STACK_TEMPLATES,
                            NULL, PRIORITY_TEMPLATES, NULL, 0);

    ESP_LOGI(TAG, "All tasks created");

    ESP_LOGI(TAG, "PRISM K1 started successfully!");

    /* Main loop - currently just prevents task deletion */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
