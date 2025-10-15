/**
 * @file protocol_parser.c
 * @brief TLV Protocol Parser Implementation for PRISM.K1
 *
 * Task 4: WebSocket protocol parser and command dispatcher
 * Phase 1: Core TLV parsing and CRC32 validation (Subtask 4.1)
 *
 * PRD Compliance:
 * - Frame format: [TYPE:1][LENGTH:2][PAYLOAD:N][CRC:4]
 * - Big-endian LENGTH and CRC32 (network byte order)
 * - Message types: 0x10, 0x11, 0x12, 0x20, 0x30 (PRD authority)
 *
 * @author Agent 2
 * @date 2025-10-16
 */

#include "protocol_parser.h"
#include "esp_log.h"
#include "esp_rom_crc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

static const char* TAG = "protocol";

/* ============================================================================
 * Module State
 * ============================================================================ */

/** Global upload session (single session, mutual exclusion) */
static upload_session_t g_upload_session = {0};

/** Upload session mutex (protects g_upload_session) */
static SemaphoreHandle_t g_upload_mutex = NULL;

/** Module initialization flag */
static bool g_initialized = false;

/* ============================================================================
 * Initialization / Deinitialization
 * ============================================================================ */

esp_err_t protocol_parser_init(void)
{
    if (g_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    // Create mutex for upload session protection
    g_upload_mutex = xSemaphoreCreateMutex();
    if (g_upload_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create upload mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize upload session to IDLE
    memset(&g_upload_session, 0, sizeof(upload_session_t));
    g_upload_session.state = UPLOAD_STATE_IDLE;

    g_initialized = true;
    ESP_LOGI(TAG, "Protocol parser initialized");

    return ESP_OK;
}

void protocol_parser_deinit(void)
{
    if (!g_initialized) {
        return;
    }

    // Cleanup active session if any
    if (g_upload_mutex != NULL) {
        xSemaphoreTake(g_upload_mutex, portMAX_DELAY);

        if (g_upload_session.state != UPLOAD_STATE_IDLE) {
            ESP_LOGW(TAG, "Cleaning up active upload session during deinit");
            if (g_upload_session.upload_buffer != NULL) {
                free(g_upload_session.upload_buffer);
            }
            memset(&g_upload_session, 0, sizeof(upload_session_t));
        }

        xSemaphoreGive(g_upload_mutex);
        vSemaphoreDelete(g_upload_mutex);
        g_upload_mutex = NULL;
    }

    g_initialized = false;
    ESP_LOGI(TAG, "Protocol parser deinitialized");
}

/* ============================================================================
 * Phase 1: Core TLV Parser (Subtask 4.1)
 * ============================================================================ */

/**
 * @brief Parse TLV frame structure with CRC32 validation
 *
 * Frame format (PRD): [TYPE:1][LENGTH:2][PAYLOAD:N][CRC:4]
 * - TYPE: 1 byte message type identifier
 * - LENGTH: 2 bytes big-endian payload length
 * - PAYLOAD: N bytes variable-length data
 * - CRC32: 4 bytes big-endian checksum over TYPE+LENGTH+PAYLOAD
 *
 * @param data Raw frame data
 * @param len Total frame length
 * @param out_frame Parsed frame structure (payload points into data)
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if data/out_frame is NULL or len < 7
 * @return ESP_ERR_INVALID_SIZE if LENGTH field exceeds remaining data
 * @return ESP_ERR_INVALID_CRC if CRC32 validation fails
 */
static esp_err_t parse_tlv_frame(const uint8_t* data, size_t len, tlv_frame_t* out_frame)
{
    // Validate inputs
    if (data == NULL || out_frame == NULL) {
        ESP_LOGE(TAG, "parse_tlv_frame: NULL arguments");
        return ESP_ERR_INVALID_ARG;
    }

    // Minimum frame size: TYPE(1) + LENGTH(2) + CRC32(4) = 7 bytes
    if (len < TLV_FRAME_MIN_SIZE) {
        ESP_LOGE(TAG, "parse_tlv_frame: frame too small (%zu bytes, min %d)",
                 len, TLV_FRAME_MIN_SIZE);
        return ESP_ERR_INVALID_ARG;
    }

    // Parse header: TYPE (1 byte) + LENGTH (2 bytes big-endian)
    out_frame->type = data[0];
    out_frame->length = ((uint16_t)data[1] << 8) | data[2];

    ESP_LOGD(TAG, "parse_tlv_frame: TYPE=0x%02X LENGTH=%u", out_frame->type, out_frame->length);

    // Validate LENGTH field consistency
    // Expected frame size: TYPE(1) + LENGTH(2) + PAYLOAD(length) + CRC32(4)
    size_t expected_frame_size = TLV_HEADER_SIZE + out_frame->length + TLV_CRC32_SIZE;
    if (len != expected_frame_size) {
        ESP_LOGE(TAG, "parse_tlv_frame: length mismatch (got %zu bytes, expected %zu)",
                 len, expected_frame_size);
        return ESP_ERR_INVALID_SIZE;
    }

    // Validate payload size doesn't exceed WebSocket buffer limit
    if (out_frame->length > TLV_MAX_PAYLOAD_SIZE) {
        ESP_LOGE(TAG, "parse_tlv_frame: payload too large (%u bytes, max %d)",
                 out_frame->length, TLV_MAX_PAYLOAD_SIZE);
        return ESP_ERR_INVALID_SIZE;
    }

    // Extract payload pointer (points into original buffer, NOT copied)
    if (out_frame->length > 0) {
        out_frame->payload = &data[TLV_HEADER_SIZE];
    } else {
        out_frame->payload = NULL;
    }

    // Extract CRC32 from last 4 bytes (big-endian)
    size_t crc_offset = TLV_HEADER_SIZE + out_frame->length;
    out_frame->crc32 = ((uint32_t)data[crc_offset + 0] << 24) |
                       ((uint32_t)data[crc_offset + 1] << 16) |
                       ((uint32_t)data[crc_offset + 2] << 8) |
                       ((uint32_t)data[crc_offset + 3]);

    // Calculate CRC32 over TYPE + LENGTH + PAYLOAD
    // Use esp_rom_crc32_le for hardware-accelerated CRC32
    size_t crc_data_len = TLV_HEADER_SIZE + out_frame->length;
    uint32_t calculated_crc = esp_rom_crc32_le(0, data, crc_data_len);

    ESP_LOGD(TAG, "parse_tlv_frame: CRC32 received=0x%08lX calculated=0x%08lX",
             (unsigned long)out_frame->crc32, (unsigned long)calculated_crc);

    // Validate CRC32
    if (calculated_crc != out_frame->crc32) {
        ESP_LOGE(TAG, "parse_tlv_frame: CRC32 mismatch (received=0x%08lX calculated=0x%08lX)",
                 (unsigned long)out_frame->crc32, (unsigned long)calculated_crc);
        return ESP_ERR_INVALID_CRC;
    }

    ESP_LOGI(TAG, "parse_tlv_frame: valid frame TYPE=0x%02X LENGTH=%u CRC32=0x%08lX",
             out_frame->type, out_frame->length, (unsigned long)out_frame->crc32);

    return ESP_OK;
}

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * @brief Get current FreeRTOS tick count in milliseconds
 */
static uint32_t get_time_ms(void)
{
    return (xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/* ============================================================================
 * Phase 2: Upload Session Management (Subtask 4.2) - STUBS
 * ============================================================================ */

// TODO: Implement in Phase 2
static esp_err_t handle_put_begin(const tlv_frame_t* frame, int client_fd)
{
    ESP_LOGW(TAG, "handle_put_begin: NOT YET IMPLEMENTED (Phase 2)");
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t handle_put_data(const tlv_frame_t* frame, int client_fd)
{
    ESP_LOGW(TAG, "handle_put_data: NOT YET IMPLEMENTED (Phase 2)");
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t handle_put_end(const tlv_frame_t* frame, int client_fd)
{
    ESP_LOGW(TAG, "handle_put_end: NOT YET IMPLEMENTED (Phase 2)");
    return ESP_ERR_NOT_SUPPORTED;
}

/* ============================================================================
 * Phase 3: Storage Integration (Subtask 4.3) - STUBS
 * ============================================================================ */

// TODO: Implement in Phase 3
static esp_err_t handle_control(const tlv_frame_t* frame, int client_fd)
{
    ESP_LOGW(TAG, "handle_control: NOT YET IMPLEMENTED (Phase 3)");
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t handle_delete(const tlv_frame_t* frame, int client_fd)
{
    ESP_LOGW(TAG, "handle_delete: NOT YET IMPLEMENTED (Phase 3)");
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t handle_list(const tlv_frame_t* frame, int client_fd)
{
    ESP_LOGW(TAG, "handle_list: NOT YET IMPLEMENTED (Phase 3)");
    return ESP_ERR_NOT_SUPPORTED;
}

/* ============================================================================
 * Command Dispatcher
 * ============================================================================ */

esp_err_t protocol_dispatch_command(
    const uint8_t* frame_data,
    size_t frame_len,
    int client_fd)
{
    if (!g_initialized) {
        ESP_LOGE(TAG, "Protocol parser not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (frame_data == NULL) {
        ESP_LOGE(TAG, "dispatch_command: NULL frame_data");
        return ESP_ERR_INVALID_ARG;
    }

    // Parse TLV frame
    tlv_frame_t frame;
    esp_err_t ret = parse_tlv_frame(frame_data, frame_len, &frame);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "dispatch_command: frame parsing failed (%s)", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "dispatch_command: client_fd=%d TYPE=0x%02X LENGTH=%u",
             client_fd, frame.type, frame.length);

    // Dispatch based on message type
    switch (frame.type) {
        // Upload commands (PRD 0x10-0x12)
        case MSG_TYPE_PUT_BEGIN:
            ret = handle_put_begin(&frame, client_fd);
            break;

        case MSG_TYPE_PUT_DATA:
            ret = handle_put_data(&frame, client_fd);
            break;

        case MSG_TYPE_PUT_END:
            ret = handle_put_end(&frame, client_fd);
            break;

        // Control commands (PRD 0x20)
        case MSG_TYPE_CONTROL:
            ret = handle_control(&frame, client_fd);
            break;

        // Extension commands (not in PRD)
        case MSG_TYPE_DELETE:
            ret = handle_delete(&frame, client_fd);
            break;

        case MSG_TYPE_LIST:
            ret = handle_list(&frame, client_fd);
            break;

        // Invalid message types
        default:
            ESP_LOGE(TAG, "dispatch_command: unknown message type 0x%02X", frame.type);
            ret = ESP_ERR_NOT_SUPPORTED;
            break;
    }

    if (ret != ESP_OK && ret != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGE(TAG, "dispatch_command: handler failed (TYPE=0x%02X ret=%s)",
                 frame.type, esp_err_to_name(ret));
    }

    return ret;
}

/* ============================================================================
 * Timeout Handling
 * ============================================================================ */

void protocol_check_upload_timeout(void)
{
    if (!g_initialized || g_upload_mutex == NULL) {
        return;
    }

    xSemaphoreTake(g_upload_mutex, portMAX_DELAY);

    if (g_upload_session.state == UPLOAD_STATE_RECEIVING) {
        uint32_t now_ms = get_time_ms();
        uint32_t idle_ms = now_ms - g_upload_session.last_activity_ms;

        if (idle_ms > UPLOAD_TIMEOUT_MS) {
            ESP_LOGW(TAG, "Upload timeout: %lu ms idle (max %d ms)",
                     (unsigned long)idle_ms, UPLOAD_TIMEOUT_MS);

            // Abort session (Phase 2 will implement cleanup)
            if (g_upload_session.upload_buffer != NULL) {
                free(g_upload_session.upload_buffer);
            }
            memset(&g_upload_session, 0, sizeof(upload_session_t));
            g_upload_session.state = UPLOAD_STATE_IDLE;
        }
    }

    xSemaphoreGive(g_upload_mutex);
}

/* ============================================================================
 * Status Query
 * ============================================================================ */

bool protocol_get_upload_status(
    char* out_filename,
    uint32_t* out_bytes_received,
    uint32_t* out_total_size)
{
    if (!g_initialized || g_upload_mutex == NULL) {
        return false;
    }

    bool active = false;

    xSemaphoreTake(g_upload_mutex, portMAX_DELAY);

    if (g_upload_session.state != UPLOAD_STATE_IDLE) {
        active = true;

        if (out_filename != NULL) {
            strncpy(out_filename, g_upload_session.filename, PATTERN_MAX_FILENAME - 1);
            out_filename[PATTERN_MAX_FILENAME - 1] = '\0';
        }

        if (out_bytes_received != NULL) {
            *out_bytes_received = g_upload_session.bytes_received;
        }

        if (out_total_size != NULL) {
            *out_total_size = g_upload_session.expected_size;
        }
    }

    xSemaphoreGive(g_upload_mutex);

    return active;
}
