/**
 * @file storage_protocol.c
 * @brief Storage protocol integration layer
 *
 * Bridges WebSocket TLV protocol with storage APIs.
 * Handles pattern upload/download/delete/list commands.
 */

#include "storage_protocol.h"
#include "pattern_storage.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "storage_protocol";

// Protocol command types (from ADR-002 WebSocket spec)
#define CMD_PUT_BEGIN       0x10
#define CMD_PUT_CHUNK       0x11
#define CMD_PUT_END         0x12
#define CMD_DELETE          0x20
#define CMD_LIST            0x21
#define CMD_STATUS          0x30
#define CMD_ERROR           0x40

// Protocol error codes (from ADR-002)
#define ERR_MAX_CLIENTS     0x01
#define ERR_BUFFER_OVERFLOW 0x02
#define ERR_INVALID_TLV     0x03
#define ERR_STORAGE_FULL    0x04
#define ERR_NOT_FOUND       0x05

// Upload session state
typedef struct {
    bool active;
    char pattern_id[32];
    size_t bytes_received;
    uint32_t start_time_ms;
} upload_session_t;

static upload_session_t g_upload_session = {0};

/**
 * @brief Parse TLV header
 * @param data Input data
 * @param len Data length
 * @param type Output: command type
 * @param value_len Output: value length
 * @return ESP_OK on success, ESP_ERR_INVALID_SIZE if malformed
 */
static esp_err_t parse_tlv_header(const uint8_t *data, size_t len,
                                   uint8_t *type, uint16_t *value_len) {
    if (len < 3) {
        ESP_LOGE(TAG, "TLV frame too short: %zu bytes (min 3)", len);
        return ESP_ERR_INVALID_SIZE;
    }

    *type = data[0];
    *value_len = (data[1] << 8) | data[2];

    // Validate value length
    if (len < (size_t)(3 + *value_len)) {
        ESP_LOGE(TAG, "TLV length mismatch: header=%u, available=%zu",
                 *value_len, len - 3);
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

/**
 * @brief Handle PUT_BEGIN command
 * @param value TLV value (pattern_id)
 * @param value_len Value length
 * @param response Output response buffer
 * @param response_len Output response length
 * @return ESP_OK on success
 */
static esp_err_t handle_put_begin(const uint8_t *value, uint16_t value_len,
                                   uint8_t *response, size_t *response_len) {
    // Validate pattern ID length
    if (value_len == 0 || value_len >= sizeof(g_upload_session.pattern_id)) {
        ESP_LOGE(TAG, "Invalid pattern ID length: %u", value_len);
        response[0] = CMD_ERROR;
        response[1] = ERR_INVALID_TLV;
        *response_len = 2;
        return ESP_ERR_INVALID_ARG;
    }

    // Check if upload already in progress
    if (g_upload_session.active) {
        ESP_LOGW(TAG, "Upload already in progress: %s", g_upload_session.pattern_id);
        response[0] = CMD_ERROR;
        response[1] = ERR_BUFFER_OVERFLOW;  // Reuse for "busy"
        *response_len = 2;
        return ESP_FAIL;
    }

    // Initialize upload session
    memcpy(g_upload_session.pattern_id, value, value_len);
    g_upload_session.pattern_id[value_len] = '\0';
    g_upload_session.active = true;
    g_upload_session.bytes_received = 0;
    g_upload_session.start_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    ESP_LOGI(TAG, "Upload session started: %s", g_upload_session.pattern_id);

    // Send STATUS response
    response[0] = CMD_STATUS;
    response[1] = 0x00;  // OK
    *response_len = 2;

    return ESP_OK;
}

/**
 * @brief Handle PUT_CHUNK command
 * @param value TLV value (pattern data chunk)
 * @param value_len Value length
 * @param response Output response buffer
 * @param response_len Output response length
 * @return ESP_OK on success
 */
static esp_err_t handle_put_chunk(const uint8_t *value, uint16_t value_len,
                                   uint8_t *response, size_t *response_len) {
    // Check if upload session active
    if (!g_upload_session.active) {
        ESP_LOGE(TAG, "No active upload session for PUT_CHUNK");
        response[0] = CMD_ERROR;
        response[1] = ERR_INVALID_TLV;
        *response_len = 2;
        return ESP_FAIL;
    }

    // Accumulate bytes (actual storage happens in PUT_END)
    g_upload_session.bytes_received += value_len;

    // Check size limit
    if (g_upload_session.bytes_received > PATTERN_SIZE_MAX) {
        ESP_LOGE(TAG, "Pattern too large: %zu > %d bytes",
                 g_upload_session.bytes_received, PATTERN_SIZE_MAX);
        g_upload_session.active = false;
        response[0] = CMD_ERROR;
        response[1] = ERR_BUFFER_OVERFLOW;
        *response_len = 2;
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGD(TAG, "Chunk received: %u bytes (total: %zu)",
             value_len, g_upload_session.bytes_received);

    // Send STATUS response
    response[0] = CMD_STATUS;
    response[1] = 0x00;  // OK
    *response_len = 2;

    return ESP_OK;
}

/**
 * @brief Handle PUT_END command
 * @param value TLV value (final pattern data)
 * @param value_len Value length
 * @param response Output response buffer
 * @param response_len Output response length
 * @return ESP_OK on success
 */
static esp_err_t handle_put_end(const uint8_t *value, uint16_t value_len,
                                 uint8_t *response, size_t *response_len) {
    // Check if upload session active
    if (!g_upload_session.active) {
        ESP_LOGE(TAG, "No active upload session for PUT_END");
        response[0] = CMD_ERROR;
        response[1] = ERR_INVALID_TLV;
        *response_len = 2;
        return ESP_FAIL;
    }

    // Note: In a real implementation, PUT_CHUNK would accumulate data in a buffer,
    // and PUT_END would write the complete pattern. For now, this is a simplified
    // stub that demonstrates the error handling flow.

    // Simulate storage operation (placeholder)
    // In real implementation: storage_pattern_create(g_upload_session.pattern_id, accumulated_data, total_size)

    // For now, just check quota
    size_t pattern_count = 0;
    esp_err_t ret = storage_pattern_count(&pattern_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to check pattern count");
        g_upload_session.active = false;
        response[0] = CMD_ERROR;
        response[1] = ERR_STORAGE_FULL;
        *response_len = 2;
        return ret;
    }

    if (pattern_count >= PATTERN_IDEAL_COUNT) {
        ESP_LOGW(TAG, "Storage full: %zu/%d patterns", pattern_count, PATTERN_IDEAL_COUNT);
        g_upload_session.active = false;
        response[0] = CMD_ERROR;
        response[1] = ERR_STORAGE_FULL;
        *response_len = 2;
        return ESP_ERR_NO_MEM;
    }

    uint32_t duration_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS) - g_upload_session.start_time_ms;
    ESP_LOGI(TAG, "Upload complete: %s (%zu bytes in %lu ms)",
             g_upload_session.pattern_id, g_upload_session.bytes_received, duration_ms);

    // Clear session
    g_upload_session.active = false;

    // Send STATUS response
    response[0] = CMD_STATUS;
    response[1] = 0x00;  // OK
    *response_len = 2;

    return ESP_OK;
}

/**
 * @brief Handle DELETE command
 * @param value TLV value (pattern_id)
 * @param value_len Value length
 * @param response Output response buffer
 * @param response_len Output response length
 * @return ESP_OK on success
 */
static esp_err_t handle_delete(const uint8_t *value, uint16_t value_len,
                                uint8_t *response, size_t *response_len) {
    if (value_len == 0 || value_len >= 64) {
        ESP_LOGE(TAG, "Invalid pattern ID length for delete: %u", value_len);
        response[0] = CMD_ERROR;
        response[1] = ERR_INVALID_TLV;
        *response_len = 2;
        return ESP_ERR_INVALID_ARG;
    }

    // Extract pattern ID
    char pattern_id[64];
    memcpy(pattern_id, value, value_len);
    pattern_id[value_len] = '\0';

    // Call storage API
    esp_err_t ret = storage_pattern_delete(pattern_id);
    if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "Pattern not found for delete: %s", pattern_id);
        response[0] = CMD_ERROR;
        response[1] = ERR_NOT_FOUND;
        *response_len = 2;
        return ret;
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to delete pattern: %s", pattern_id);
        response[0] = CMD_ERROR;
        response[1] = ERR_STORAGE_FULL;  // Generic error
        *response_len = 2;
        return ret;
    }

    ESP_LOGI(TAG, "Pattern deleted: %s", pattern_id);

    // Send STATUS response
    response[0] = CMD_STATUS;
    response[1] = 0x00;  // OK
    *response_len = 2;

    return ESP_OK;
}

/**
 * @brief Handle LIST command
 * @param response Output response buffer
 * @param response_len Output response length
 * @return ESP_OK on success
 */
static esp_err_t handle_list(uint8_t *response, size_t *response_len) {
    // Note: Real implementation would encode pattern list as TLV
    // For now, just return count

    size_t count = 0;
    esp_err_t ret = storage_pattern_count(&count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get pattern count");
        response[0] = CMD_ERROR;
        response[1] = ERR_STORAGE_FULL;
        *response_len = 2;
        return ret;
    }

    ESP_LOGI(TAG, "Pattern list requested: %zu patterns", count);

    // Send simplified STATUS response with count
    response[0] = CMD_STATUS;
    response[1] = (uint8_t)count;
    *response_len = 2;

    return ESP_OK;
}

esp_err_t storage_protocol_dispatch(const uint8_t *data, size_t len,
                                     uint8_t *response, size_t *response_len) {
    if (!data || !response || !response_len) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }

    // Parse TLV header
    uint8_t cmd_type;
    uint16_t value_len;
    esp_err_t ret = parse_tlv_header(data, len, &cmd_type, &value_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to parse TLV header");
        response[0] = CMD_ERROR;
        response[1] = ERR_INVALID_TLV;
        *response_len = 2;
        return ret;
    }

    const uint8_t *value = (len > 3) ? &data[3] : NULL;

    // Dispatch to command handler
    switch (cmd_type) {
        case CMD_PUT_BEGIN:
            return handle_put_begin(value, value_len, response, response_len);

        case CMD_PUT_CHUNK:
            return handle_put_chunk(value, value_len, response, response_len);

        case CMD_PUT_END:
            return handle_put_end(value, value_len, response, response_len);

        case CMD_DELETE:
            return handle_delete(value, value_len, response, response_len);

        case CMD_LIST:
            return handle_list(response, response_len);

        default:
            ESP_LOGW(TAG, "Unknown command type: 0x%02X", cmd_type);
            response[0] = CMD_ERROR;
            response[1] = ERR_INVALID_TLV;
            *response_len = 2;
            return ESP_ERR_NOT_SUPPORTED;
    }
}

bool storage_protocol_is_upload_active(void) {
    return g_upload_session.active;
}

void storage_protocol_abort_upload(void) {
    if (g_upload_session.active) {
        ESP_LOGW(TAG, "Aborting active upload session: %s", g_upload_session.pattern_id);
        g_upload_session.active = false;
    }
}
