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
#include "pattern_storage.h"
#include "pattern_metadata.h"  // Motion/sync enums and validators (Task 13.2)
#include "led_driver.h"
#include "led_playback.h"
#include "template_manager.h"  // templates_deploy, templates_list
#include "template_patterns.h" // template_catalog_get
#include "esp_log.h"
#include "esp_rom_crc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "network_manager.h"
#include <dirent.h>
#include <sys/stat.h>
#include "esp_app_desc.h"
#include <string.h>
#include <stdlib.h>

static const char* TAG = "protocol";

/* Additional TLV value types for motion/sync within higher-level payloads */
#define PRISM_TLV_MOTION  0x20
#define PRISM_TLV_SYNC    0x21

static bool validate_motion_tlv(uint8_t motion_value)
{
    return PRISM_MOTION_IS_VALID(motion_value);
}

static bool validate_sync_tlv(uint8_t sync_value)
{
    return PRISM_SYNC_IS_VALID(sync_value);
}

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

/**
 * @brief Build and send TLV-encoded response over WebSocket
 *
 * Format: [TYPE:1][LENGTH:2 big-endian][PAYLOAD:N][CRC32:4 big-endian]
 */
static esp_err_t send_tlv_response(int client_fd, uint8_t msg_type, const uint8_t* payload, size_t len)
{
    if (len > TLV_MAX_PAYLOAD_SIZE) {
        ESP_LOGE(TAG, "send_tlv_response: payload too large (%zu)", len);
        return ESP_ERR_INVALID_SIZE;
    }

    size_t frame_len = TLV_HEADER_SIZE + len + TLV_CRC32_SIZE;
    uint8_t* frame = (uint8_t*)malloc(frame_len);
    if (!frame) {
        return ESP_ERR_NO_MEM;
    }

    // Header
    frame[0] = msg_type;
    frame[1] = (len >> 8) & 0xFF;
    frame[2] = (len) & 0xFF;

    // Payload
    if (payload && len > 0) {
        memcpy(&frame[3], payload, len);
    }

    // CRC32 over TYPE+LENGTH+PAYLOAD (big-endian write)
    uint32_t crc = esp_rom_crc32_le(0, frame, TLV_HEADER_SIZE + len);
    size_t crc_off = TLV_HEADER_SIZE + len;
    frame[crc_off + 0] = (crc >> 24) & 0xFF;
    frame[crc_off + 1] = (crc >> 16) & 0xFF;
    frame[crc_off + 2] = (crc >> 8) & 0xFF;
    frame[crc_off + 3] = (crc) & 0xFF;

    esp_err_t ret = ws_send_binary_to_fd(client_fd, frame, frame_len);
    free(frame);
    return ret;
}

/**
 * @brief Convenience error sender (ERROR message with code + text)
 */
static esp_err_t send_error_response(int client_fd, uint8_t error_code, const char* message)
{
    uint8_t buf[256];
    size_t off = 0;
    buf[off++] = error_code;
    if (message && message[0]) {
        size_t ml = strlen(message);
        if (ml > sizeof(buf) - 1) ml = sizeof(buf) - 1;
        memcpy(&buf[off], message, ml);
        off += ml;
    }
    return send_tlv_response(client_fd, MSG_TYPE_ERROR, buf, off);
}

/* ============================================================================
 * Phase 2: Upload Session Management (Subtask 4.2)
 * ============================================================================ */

/**
 * @brief Abort active upload session and cleanup resources
 */
static void abort_upload_session(const char* reason)
{
    ESP_LOGW(TAG, "Aborting upload session: %s", reason);

    if (g_upload_session.upload_buffer != NULL) {
        free(g_upload_session.upload_buffer);
        g_upload_session.upload_buffer = NULL;
    }

    memset(&g_upload_session, 0, sizeof(upload_session_t));
    g_upload_session.state = UPLOAD_STATE_IDLE;
}

/**
 * @brief Parse PUT_BEGIN payload: {filename, size, crc}
 *
 * Payload format (PRD line 152):
 * - filename_len: 1 byte (length of filename string)
 * - filename: N bytes (UTF-8 string, not null-terminated)
 * - expected_size: 4 bytes big-endian (total pattern size)
 * - expected_crc: 4 bytes big-endian (expected pattern CRC32)
 */
static esp_err_t parse_put_begin_payload(
    const uint8_t* payload,
    uint16_t payload_len,
    char* out_filename,
    uint32_t* out_size,
    uint32_t* out_crc)
{
    // Minimum: filename_len(1) + filename(1+) + size(4) + crc(4) = 10 bytes
    if (payload_len < 10) {
        ESP_LOGE(TAG, "PUT_BEGIN payload too small: %u bytes (min 10)", payload_len);
        return ESP_ERR_INVALID_ARG;
    }

    // Parse filename length
    uint8_t filename_len = payload[0];
    if (filename_len == 0 || filename_len >= PATTERN_MAX_FILENAME) {
        ESP_LOGE(TAG, "PUT_BEGIN invalid filename length: %u (max %d)",
                 filename_len, PATTERN_MAX_FILENAME - 1);
        return ESP_ERR_INVALID_ARG;
    }

    // Validate payload size matches expected structure
    size_t expected_len = 1 + filename_len + 4 + 4; // len + name + size + crc
    if (payload_len != expected_len) {
        ESP_LOGE(TAG, "PUT_BEGIN payload size mismatch: got %u, expected %zu",
                 payload_len, expected_len);
        return ESP_ERR_INVALID_ARG;
    }

    // Extract filename (copy and sanitize)
    char raw_name[PATTERN_MAX_FILENAME];
    size_t copy_len = filename_len;
    if (copy_len >= sizeof(raw_name)) {
        copy_len = sizeof(raw_name) - 1;
    }
    memcpy(raw_name, &payload[1], copy_len);
    raw_name[copy_len] = '\0';
    playback_normalize_pattern_id(raw_name, out_filename, PATTERN_MAX_FILENAME);
    ESP_LOGI(TAG, "PUT_BEGIN: pattern id '%s' (raw=%s)", out_filename, raw_name);

    // Extract expected size (big-endian)
    size_t offset = 1 + filename_len;
    *out_size = ((uint32_t)payload[offset + 0] << 24) |
                ((uint32_t)payload[offset + 1] << 16) |
                ((uint32_t)payload[offset + 2] << 8) |
                ((uint32_t)payload[offset + 3]);

    // Extract expected CRC32 (big-endian)
    offset += 4;
    *out_crc = ((uint32_t)payload[offset + 0] << 24) |
               ((uint32_t)payload[offset + 1] << 16) |
               ((uint32_t)payload[offset + 2] << 8) |
               ((uint32_t)payload[offset + 3]);

    return ESP_OK;
}

/**
 * @brief Handle PUT_BEGIN: Initiate pattern upload session
 *
 * PRD: 0x10 - PUT_BEGIN {filename, size, crc}
 */
static esp_err_t handle_put_begin(const tlv_frame_t* frame, int client_fd)
{
    char filename[PATTERN_MAX_FILENAME];
    uint32_t expected_size;
    uint32_t expected_crc;

    // Parse payload
    esp_err_t ret = parse_put_begin_payload(
        frame->payload,
        frame->length,
        filename,
        &expected_size,
        &expected_crc
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PUT_BEGIN: Failed to parse payload");
        return ret;
    }

    // Validate pattern size (256KB limit per ADR-004)
    if (expected_size == 0 || expected_size > PATTERN_MAX_SIZE) {
        ESP_LOGE(TAG, "PUT_BEGIN: Invalid size %lu (max %lu)",
                 (unsigned long)expected_size, (unsigned long)PATTERN_MAX_SIZE);
        return ESP_ERR_INVALID_ARG;
    }

    // Acquire mutex for session state
    xSemaphoreTake(g_upload_mutex, portMAX_DELAY);

    // Check for existing active session
    if (g_upload_session.state != UPLOAD_STATE_IDLE) {
        xSemaphoreGive(g_upload_mutex);
        ESP_LOGE(TAG, "PUT_BEGIN: Upload already in progress (state=%d)",
                 g_upload_session.state);
        return ESP_ERR_INVALID_STATE;
    }

    // Allocate upload buffer (will be freed in PUT_END or on error/timeout)
    uint8_t* buffer = (uint8_t*)malloc(expected_size);
    if (buffer == NULL) {
        xSemaphoreGive(g_upload_mutex);
        ESP_LOGE(TAG, "PUT_BEGIN: Failed to allocate %lu bytes",
                 (unsigned long)expected_size);
        return ESP_ERR_NO_MEM;
    }

    // Initialize upload session
    g_upload_session.state = UPLOAD_STATE_RECEIVING;
    strncpy(g_upload_session.filename, filename, PATTERN_MAX_FILENAME - 1);
    g_upload_session.filename[PATTERN_MAX_FILENAME - 1] = '\0';
    g_upload_session.expected_size = expected_size;
    g_upload_session.expected_crc = expected_crc;
    g_upload_session.bytes_received = 0;
    g_upload_session.crc_accumulator = 0; // Will start accumulation on first PUT_DATA
    g_upload_session.upload_buffer = buffer;
    g_upload_session.last_activity_ms = get_time_ms();
    g_upload_session.client_fd = client_fd;

    xSemaphoreGive(g_upload_mutex);

    ESP_LOGI(TAG, "PUT_BEGIN: filename='%s' size=%lu crc=0x%08lX",
             filename, (unsigned long)expected_size, (unsigned long)expected_crc);

    return ESP_OK;
}

/**
 * @brief Handle PUT_DATA: Stream pattern data chunk
 *
 * PRD: 0x11 - PUT_DATA {offset, data}
 */
static esp_err_t handle_put_data(const tlv_frame_t* frame, int client_fd)
{
    if (frame->payload == NULL || frame->length == 0) {
        ESP_LOGE(TAG, "PUT_DATA: Empty payload");
        return ESP_ERR_INVALID_ARG;
    }

    // Payload format: offset(4 bytes big-endian) + data(N bytes)
    if (frame->length < 4) {
        ESP_LOGE(TAG, "PUT_DATA: Payload too small (%u bytes, min 4)", frame->length);
        return ESP_ERR_INVALID_ARG;
    }

    // Parse offset (big-endian)
    uint32_t offset = ((uint32_t)frame->payload[0] << 24) |
                      ((uint32_t)frame->payload[1] << 16) |
                      ((uint32_t)frame->payload[2] << 8) |
                      ((uint32_t)frame->payload[3]);

    const uint8_t* data = &frame->payload[4];
    size_t data_len = frame->length - 4;

    // Acquire mutex
    xSemaphoreTake(g_upload_mutex, portMAX_DELAY);

    // Validate session state
    if (g_upload_session.state != UPLOAD_STATE_RECEIVING) {
        xSemaphoreGive(g_upload_mutex);
        ESP_LOGE(TAG, "PUT_DATA: No active upload session (state=%d)",
                 g_upload_session.state);
        return ESP_ERR_INVALID_STATE;
    }

    // Validate client ownership
    if (g_upload_session.client_fd != client_fd) {
        xSemaphoreGive(g_upload_mutex);
        ESP_LOGE(TAG, "PUT_DATA: Session owned by different client");
        return ESP_ERR_INVALID_STATE;
    }

    // Validate offset and length
    if (offset + data_len > g_upload_session.expected_size) {
        ESP_LOGE(TAG, "PUT_DATA: Data exceeds expected size (offset=%lu + len=%zu > total=%lu)",
                 (unsigned long)offset, data_len, (unsigned long)g_upload_session.expected_size);
        abort_upload_session("Size overflow");
        xSemaphoreGive(g_upload_mutex);
        return ESP_ERR_INVALID_SIZE;
    }

    // Copy data to upload buffer
    memcpy(&g_upload_session.upload_buffer[offset], data, data_len);

    // Update bytes received counter
    if (offset + data_len > g_upload_session.bytes_received) {
        g_upload_session.bytes_received = offset + data_len;
    }

    // Accumulate CRC32 over received data
    // Note: This assumes sequential uploads. For out-of-order uploads, CRC accumulation
    // would need to be done in PUT_END over the complete buffer.
    if (g_upload_session.crc_accumulator == 0 && offset == 0) {
        // First chunk - initialize CRC
        g_upload_session.crc_accumulator = esp_rom_crc32_le(0, data, data_len);
    } else {
        // Subsequent chunks - accumulate (only works for sequential uploads)
        // For robustness, we'll recalculate CRC over entire buffer in PUT_END
        g_upload_session.crc_accumulator = esp_rom_crc32_le(
            g_upload_session.crc_accumulator,
            data,
            data_len
        );
    }

    // Update activity timestamp
    g_upload_session.last_activity_ms = get_time_ms();

    float progress = (g_upload_session.bytes_received * 100.0f) / g_upload_session.expected_size;
    ESP_LOGD(TAG, "PUT_DATA: offset=%lu len=%zu progress=%.1f%% (%lu/%lu bytes)",
             (unsigned long)offset, data_len, progress,
             (unsigned long)g_upload_session.bytes_received,
             (unsigned long)g_upload_session.expected_size);

    xSemaphoreGive(g_upload_mutex);
    return ESP_OK;
}

/**
 * @brief Handle PUT_END: Finalize pattern upload
 *
 * PRD: 0x12 - PUT_END {success}
 */
static esp_err_t handle_put_end(const tlv_frame_t* frame, int client_fd)
{
    // Acquire mutex
    xSemaphoreTake(g_upload_mutex, portMAX_DELAY);

    // Validate session state
    if (g_upload_session.state != UPLOAD_STATE_RECEIVING) {
        xSemaphoreGive(g_upload_mutex);
        ESP_LOGE(TAG, "PUT_END: No active upload session (state=%d)",
                 g_upload_session.state);
        return ESP_ERR_INVALID_STATE;
    }

    // Validate client ownership
    if (g_upload_session.client_fd != client_fd) {
        xSemaphoreGive(g_upload_mutex);
        ESP_LOGE(TAG, "PUT_END: Session owned by different client");
        return ESP_ERR_INVALID_STATE;
    }

    // Validate completeness (all bytes received)
    if (g_upload_session.bytes_received != g_upload_session.expected_size) {
        ESP_LOGE(TAG, "PUT_END: Incomplete upload (received=%lu expected=%lu)",
                 (unsigned long)g_upload_session.bytes_received,
                 (unsigned long)g_upload_session.expected_size);
        abort_upload_session("Incomplete upload");
        xSemaphoreGive(g_upload_mutex);
        return ESP_ERR_INVALID_SIZE;
    }

    // Transition to VALIDATING state
    g_upload_session.state = UPLOAD_STATE_VALIDATING;

    // Recalculate CRC32 over entire buffer (handles out-of-order uploads)
    uint32_t calculated_crc = esp_rom_crc32_le(
        0,
        g_upload_session.upload_buffer,
        g_upload_session.expected_size
    );

    ESP_LOGI(TAG, "PUT_END: CRC32 validation - expected=0x%08lX calculated=0x%08lX",
             (unsigned long)g_upload_session.expected_crc,
             (unsigned long)calculated_crc);

    // Validate CRC32
    if (calculated_crc != g_upload_session.expected_crc) {
        ESP_LOGE(TAG, "PUT_END: CRC32 mismatch!");
        abort_upload_session("CRC mismatch");
        xSemaphoreGive(g_upload_mutex);
        return ESP_ERR_INVALID_CRC;
    }

    // Transition to STORING state
    g_upload_session.state = UPLOAD_STATE_STORING;

    char stored_id[PATTERN_MAX_FILENAME];
    strlcpy(stored_id, g_upload_session.filename, sizeof(stored_id));

    // Remove any existing pattern with the same id (best-effort)
    esp_err_t ret = storage_pattern_delete(stored_id);
    if (ret != ESP_OK && ret != ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "PUT_END: could not remove existing pattern '%s' (%s)",
                 stored_id, esp_err_to_name(ret));
    }

    // Write pattern to persistent storage
    ret = storage_pattern_create(
        stored_id,
        g_upload_session.upload_buffer,
        g_upload_session.expected_size
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PUT_END: storage_pattern_create failed (%s)", esp_err_to_name(ret));
        abort_upload_session("Storage write failed");
        xSemaphoreGive(g_upload_mutex);
        return ret;
    }

    ESP_LOGI(TAG, "PUT_END: Pattern '%s' uploaded successfully (%lu bytes)",
             stored_id,
             (unsigned long)g_upload_session.expected_size);

    // Cleanup and transition to IDLE
    free(g_upload_session.upload_buffer);
    memset(&g_upload_session, 0, sizeof(upload_session_t));
    g_upload_session.state = UPLOAD_STATE_IDLE;

    xSemaphoreGive(g_upload_mutex);

    uint8_t ack[PATTERN_MAX_FILENAME + 1] = {0};
    size_t name_len = strnlen(stored_id, PATTERN_MAX_FILENAME - 1);
    ack[0] = (uint8_t)name_len;
    if (name_len > 0) {
        memcpy(&ack[1], stored_id, name_len);
    }

    return send_tlv_response(client_fd, MSG_TYPE_STATUS, ack, name_len + 1);
}

/* ============================================================================
 * Phase 4: Playback Integration (Subtask 4.4)
 * ============================================================================ */

/** Control command codes */
#define CONTROL_CMD_PLAY        0x01  /**< Start pattern playback: {pattern_name} */
#define CONTROL_CMD_STOP        0x02  /**< Stop pattern playback: {} */
#define CONTROL_CMD_PAUSE       0x03  /**< Pause playback: {} */
#define CONTROL_CMD_RESUME      0x04  /**< Resume playback: {} */
#define CONTROL_CMD_DEPLOY_TPL  0x12  /**< Deploy built-in template: {command(1), len(1), id(N)} */
#define CONTROL_CMD_BRIGHTNESS  0x10  /**< Set global brightness: {command(1), target(1), duration_ms(2)} */

/**
 * @brief Handle CONTROL command: Playback control
 *
 * PRD: 0x20 - CONTROL {command, params}
 *
 * Control Commands:
 * - 0x01 PLAY: Start pattern playback
 *   Payload: command(1) + pattern_name_len(1) + pattern_name(N)
 * - 0x02 STOP: Stop pattern playback
 *   Payload: command(1)
 * - 0x03 PAUSE: Pause current playback
 *   Payload: command(1)
 * - 0x04 RESUME: Resume paused playback
 *   Payload: command(1)
 */
static esp_err_t handle_control(const tlv_frame_t* frame, int client_fd)
{
    if (frame->payload == NULL || frame->length < 1) {
        ESP_LOGE(TAG, "CONTROL: Empty payload (need at least command byte)");
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t command = frame->payload[0];
    ESP_LOGI(TAG, "CONTROL: command=0x%02X length=%u", command, frame->length);

    switch (command) {
        case CONTROL_CMD_PLAY: {
            if (frame->length < 3) {
                (void)send_error_response(client_fd, ERR_INVALID_FRAME, "play invalid");
                return ESP_ERR_INVALID_ARG;
            }

            uint8_t name_len = frame->payload[1];
            if (name_len == 0 || name_len >= PATTERN_MAX_FILENAME) {
                (void)send_error_response(client_fd, ERR_INVALID_FRAME, "name invalid");
                return ESP_ERR_INVALID_ARG;
            }

            if (frame->length != 2 + name_len) {
                (void)send_error_response(client_fd, ERR_INVALID_FRAME, "payload mismatch");
                return ESP_ERR_INVALID_ARG;
            }

            char raw_name[PATTERN_MAX_FILENAME];
            memcpy(raw_name, &frame->payload[2], name_len);
            raw_name[name_len] = '\0';

            char pattern_name[PATTERN_MAX_FILENAME];
            playback_normalize_pattern_id(raw_name, pattern_name, sizeof(pattern_name));

            esp_err_t ret = playback_play_pattern_from_storage(pattern_name);
            if (ret != ESP_OK) {
                uint8_t code = ERR_STORAGE_FULL;
                if (ret == ESP_ERR_NOT_FOUND) {
                    code = ERR_NOT_FOUND;
                } else if (ret == ESP_ERR_INVALID_ARG) {
                    code = ERR_INVALID_FRAME;
                }
                (void)send_error_response(client_fd, code, "play failed");
                return ret;
            }

            uint8_t payload[PATTERN_MAX_FILENAME + 2];
            size_t off = 0;
            payload[off++] = 0x00;
            size_t nl = strnlen(pattern_name, sizeof(pattern_name) - 1);
            memcpy(&payload[off], pattern_name, nl);
            off += nl;
            return send_tlv_response(client_fd, MSG_TYPE_STATUS, payload, off);
        }
        case CONTROL_CMD_STOP: {
            esp_err_t ret = playback_stop();
            if (ret != ESP_OK) {
                (void)send_error_response(client_fd, ERR_INVALID_FRAME, "stop failed");
                return ret;
            }
            uint8_t payload[1] = {0x00};
            return send_tlv_response(client_fd, MSG_TYPE_STATUS, payload, sizeof(payload));
        }
        case CONTROL_CMD_BRIGHTNESS: {
            if (frame->length != 4) {
                (void)send_error_response(client_fd, ERR_INVALID_FRAME, "brightness invalid");
                return ESP_ERR_INVALID_ARG;
            }
            uint8_t target = frame->payload[1];
            uint16_t dur = ((uint16_t)frame->payload[2] << 8) | frame->payload[3];
            extern esp_err_t playback_set_brightness(uint8_t, uint32_t);
            esp_err_t ret = playback_set_brightness(target, (uint32_t)dur);
            if (ret != ESP_OK) {
                (void)send_error_response(client_fd, ERR_INVALID_FRAME, "brightness failed");
                return ret;
            }
            char msg[48];
            int n = snprintf(msg, sizeof(msg), "brightness=%u dur_ms=%u", (unsigned)target, (unsigned)dur);
            if (n < 0) {
                n = 0;
            }
            if ((size_t)n > sizeof(msg)) {
                n = sizeof(msg);
            }
            uint8_t payload[64]; size_t off = 0; payload[off++] = 0x00;
            memcpy(&payload[off], msg, (size_t)n); off += (size_t)n;
            return send_tlv_response(client_fd, MSG_TYPE_STATUS, payload, off);
        }
        case CONTROL_CMD_DEPLOY_TPL: {
            if (frame->length < 3) {
                (void)send_error_response(client_fd, ERR_INVALID_FRAME, "deploy invalid");
                return ESP_ERR_INVALID_ARG;
            }
            uint8_t name_len = frame->payload[1];
            if (name_len == 0 || name_len >= PATTERN_MAX_FILENAME) {
                (void)send_error_response(client_fd, ERR_INVALID_FRAME, "name invalid");
                return ESP_ERR_INVALID_ARG;
            }
            if (frame->length != 2 + name_len) {
                (void)send_error_response(client_fd, ERR_INVALID_FRAME, "payload mismatch");
                return ESP_ERR_INVALID_ARG;
            }
            char tpl_id[PATTERN_MAX_FILENAME];
            memcpy(tpl_id, &frame->payload[2], name_len);
            tpl_id[name_len] = '\0';

            esp_err_t ret = templates_deploy(tpl_id);
            if (ret != ESP_OK) {
                uint8_t code = ERR_INVALID_FRAME;
                if (ret == ESP_ERR_NOT_FOUND) code = ERR_NOT_FOUND;
                (void)send_error_response(client_fd, code, "deploy failed");
                return ret;
            }

            uint8_t payload[PATTERN_MAX_FILENAME + 2];
            size_t off = 0; payload[off++] = 0x00;
            memcpy(&payload[off], tpl_id, name_len); off += name_len;
            return send_tlv_response(client_fd, MSG_TYPE_STATUS, payload, off);
        }
        case 0x11: {
            if (frame->length != 5) {
                (void)send_error_response(client_fd, ERR_INVALID_FRAME, "gamma invalid");
                return ESP_ERR_INVALID_ARG;
            }
            uint16_t gamma_x100 = ((uint16_t)frame->payload[1] << 8) | frame->payload[2];
            uint16_t dur = ((uint16_t)frame->payload[3] << 8) | frame->payload[4];
            extern void effect_add_gamma(uint16_t); extern void effect_gamma_set_target(uint16_t, uint32_t);
            effect_add_gamma(gamma_x100);
            effect_gamma_set_target(gamma_x100, dur);
            char msg[64];
            int n = snprintf(msg, sizeof(msg), "gamma_x100=%u dur_ms=%u", (unsigned)gamma_x100, (unsigned)dur);
            if (n < 0) {
                n = 0;
            }
            if ((size_t)n > sizeof(msg)) {
                n = sizeof(msg);
            }
            uint8_t payload[80]; size_t off = 0; payload[off++] = 0x00;
            memcpy(&payload[off], msg, (size_t)n); off += (size_t)n;
            return send_tlv_response(client_fd, MSG_TYPE_STATUS, payload, off);
        }
        case CONTROL_CMD_PAUSE:
        case CONTROL_CMD_RESUME:
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }
}
static esp_err_t handle_delete(const tlv_frame_t* frame, int client_fd)
{
    if (frame->length == 0 || frame->length > PATTERN_MAX_FILENAME) {
        return send_error_response(client_fd, ERR_INVALID_FRAME, "Empty or oversized filename");
    }

    // Extract filename
    char name[PATTERN_MAX_FILENAME + 1];
    memcpy(name, frame->payload, frame->length);
    name[frame->length] = '\0';

    // Strip known extensions (.prism or .bin)
    char* dot = strrchr(name, '.');
    if (dot && (strcmp(dot, ".prism") == 0 || strcmp(dot, ".bin") == 0)) {
        *dot = '\0';
    }

    // Reject path traversal
    if (strstr(name, "..") || strchr(name, '/')) {
        return send_error_response(client_fd, ERR_INVALID_FRAME, "Invalid filename");
    }

    esp_err_t ret = storage_pattern_delete(name);
    if (ret == ESP_OK) {
        uint8_t payload[PATTERN_MAX_FILENAME + 1];
        size_t off = 0;
        payload[off++] = 0x00; // success code
        size_t nl = strlen(name);
        memcpy(&payload[off], name, nl);
        off += nl;
        return send_tlv_response(client_fd, MSG_TYPE_STATUS, payload, off);
    }
    if (ret == ESP_ERR_NOT_FOUND) {
        return send_error_response(client_fd, ERR_NOT_FOUND, "Pattern not found");
    }
    return send_error_response(client_fd, ERR_STORAGE_FULL, "Delete failed");
}

static esp_err_t handle_list(const tlv_frame_t* frame, int client_fd)
{
    (void)frame; // no payload expected

    DIR* dir = opendir("/littlefs/patterns");
    if (!dir) {
        return send_error_response(client_fd, ERR_STORAGE_FULL, "Cannot open patterns dir");
    }

    uint8_t resp[TLV_MAX_PAYLOAD_SIZE];
    size_t off = 2; // reserve for count
    uint16_t count = 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        // consider only .bin files
        const char* ext = strrchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".bin") != 0) continue;

        // get file stats (build path without format truncation warnings)
        char path[256];
        size_t base_len = strlcpy(path, "/littlefs/patterns/", sizeof(path));
        if (base_len < sizeof(path)) {
            strlcpy(path + base_len, entry->d_name, sizeof(path) - base_len);
        }
        struct stat st;
        if (stat(path, &st) != 0) continue;

        // prepare name without extension
        char id[PATTERN_MAX_FILENAME];
        size_t name_len = (size_t)(ext - entry->d_name);
        if (name_len == 0 || name_len >= sizeof(id)) continue;
        memcpy(id, entry->d_name, name_len);
        id[name_len] = '\0';

        // space check: name_len(2) + name + size(4) + mtime(4)
        size_t need = 2 + name_len + 4 + 4;
        if (off + need > sizeof(resp)) {
            ESP_LOGW(TAG, "LIST truncated due to buffer");
            break;
        }

        // write name length (uint16)
        resp[off++] = (name_len >> 8) & 0xFF;
        resp[off++] = (name_len) & 0xFF;
        // write name
        memcpy(&resp[off], id, name_len);
        off += name_len;
        // write size (uint32)
        uint32_t fsz = (uint32_t)st.st_size;
        memcpy(&resp[off], &fsz, 4);
        off += 4;
        // write timestamp (uint32)
        uint32_t mtime = (uint32_t)st.st_mtime;
        memcpy(&resp[off], &mtime, 4);
        off += 4;

        count++;
    }
    closedir(dir);

    // write count at start (uint16)
    resp[0] = (count >> 8) & 0xFF;
    resp[1] = (count) & 0xFF;

    return send_tlv_response(client_fd, MSG_TYPE_STATUS, resp, off);
}

/**
 * @brief STATUS/HELLO handler: returns device info block
 * Payload format:
 *  [0-3]  version length (uint32)
 *  [..]   version string (ASCII)
 *  [..]   LED count (uint16)
 *  [..]   storage available bytes (uint32)
 *  [..]   max chunk size (uint16)
 */
static esp_err_t handle_status(const tlv_frame_t* frame, int client_fd)
{
    (void)frame; // no payload expected

    uint8_t resp[256];
    size_t off = 0;

    // Firmware version from app description
    const esp_app_desc_t* ad = esp_app_get_description();
    const char* ver = ad ? ad->version : "unknown";
    uint32_t vlen = (uint32_t)strlen(ver);
    memcpy(&resp[off], &vlen, 4); off += 4;
    memcpy(&resp[off], ver, vlen); off += vlen;

    // LED count
    uint16_t led_count = 320;
    memcpy(&resp[off], &led_count, 2); off += 2;

    // LittleFS free space
    size_t total = 0, used = 0;
    if (storage_get_space(&total, &used) != ESP_OK) {
        total = used = 0;
    }
    uint32_t avail = (uint32_t)(total - used);
    memcpy(&resp[off], &avail, 4); off += 4;

    // Max chunk size (ADR-002: 4096 - 7)
    uint16_t max_chunk = TLV_MAX_PAYLOAD_SIZE;
    memcpy(&resp[off], &max_chunk, 2); off += 2;

    // Template count (from built-in catalog)
    size_t tpl_count = 0; (void)template_catalog_get(&tpl_count);
    uint8_t tplc = (uint8_t)(tpl_count & 0xFF);
    memcpy(&resp[off], &tplc, 1); off += 1;

    return send_tlv_response(client_fd, MSG_TYPE_STATUS, resp, off);
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

        case MSG_TYPE_STATUS:
            ret = handle_status(&frame, client_fd);
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
