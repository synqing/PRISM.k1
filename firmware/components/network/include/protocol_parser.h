/**
 * @file protocol_parser.h
 * @brief TLV Protocol Parser for PRISM.K1 WebSocket Binary Protocol
 *
 * Implements binary TLV (Type-Length-Value) protocol parsing per PRD specification.
 *
 * Protocol Specification (PRD Lines 148-157):
 * - Frame Format: [TYPE:1][LENGTH:2][PAYLOAD:N][CRC:4]
 * - Endianness: Big-endian (network byte order)
 * - Max Frame Size: 4096 bytes (WebSocket buffer, ADR-002)
 * - Max Pattern Size: 256KB (ADR-004)
 *
 * Message Types (PRD Authority):
 * - 0x10: PUT_BEGIN {filename, size, crc}
 * - 0x11: PUT_DATA {offset, data}
 * - 0x12: PUT_END {success}
 * - 0x20: CONTROL {command, params}
 * - 0x30: STATUS {heap, patterns, uptime}
 * - 0xFF: ERROR {error_code, message} (extension)
 *
 * Integration:
 * - Task 3 (WebSocket): Entry point via protocol_dispatch_command()
 * - Task 5 (Storage): Pattern write/read operations (filename-based)
 * - Task 8 (Playback): LED playback control
 *
 * @author Agent 2
 * @date 2025-10-16
 */

#ifndef PROTOCOL_PARSER_H
#define PROTOCOL_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Message Type Definitions (PRD-Compliant)
 * ============================================================================ */

/** Upload message types (PRD lines 152-154) */
#define MSG_TYPE_PUT_BEGIN      0x10  /**< Initiate pattern upload: {filename, size, crc} */
#define MSG_TYPE_PUT_DATA       0x11  /**< Stream pattern data: {offset, data} */
#define MSG_TYPE_PUT_END        0x12  /**< Finalize upload: {success} */

/** Control message types (PRD line 155) */
#define MSG_TYPE_CONTROL        0x20  /**< Playback control: {command, params} */

/** Status message types (PRD line 156) */
#define MSG_TYPE_STATUS         0x30  /**< Status response: {heap, patterns, uptime} */

/** Extension message types (not in PRD) */
#define MSG_TYPE_DELETE         0x21  /**< Delete pattern: {filename} */
#define MSG_TYPE_LIST           0x22  /**< List patterns: {} */
#define MSG_TYPE_ERROR          0xFF  /**< Error response: {error_code, message} */

/* ============================================================================
 * Error Code Definitions
 * ============================================================================ */

#define ERR_INVALID_FRAME       0x01  /**< Invalid frame format or state violation */
#define ERR_CRC_MISMATCH        0x02  /**< CRC validation failed */
#define ERR_SIZE_EXCEEDED       0x03  /**< Pattern size exceeded 256KB */
#define ERR_STORAGE_FULL        0x04  /**< Memory allocation or storage full */
#define ERR_NOT_FOUND           0x05  /**< Pattern not found */

/* ============================================================================
 * Protocol Constants
 * ============================================================================ */

/** TLV frame structure sizes */
#define TLV_FRAME_MIN_SIZE      7     /**< Minimum: TYPE(1) + LENGTH(2) + CRC32(4) */
#define TLV_HEADER_SIZE         3     /**< TYPE(1) + LENGTH(2) */
#define TLV_CRC32_SIZE          4     /**< CRC32 checksum size */

/** Maximum sizes per ADR-002 and ADR-004 */
#define TLV_MAX_PAYLOAD_SIZE    (4096 - 7)   /**< Max payload: WS buffer - overhead */
#define PATTERN_MAX_SIZE        262144        /**< 256KB maximum pattern size */
#define PATTERN_MAX_FILENAME    64            /**< Maximum filename length */

/** Session timeouts */
#define UPLOAD_TIMEOUT_MS       5000  /**< Upload session idle timeout */

/* ============================================================================
 * Data Structures
 * ============================================================================ */

/**
 * @brief Parsed TLV frame structure
 *
 * Note: payload pointer is NOT owned (points into original buffer).
 * Caller must ensure buffer lifetime exceeds frame usage.
 */
typedef struct {
    uint8_t type;                       /**< Message type (0x10-0xFF) */
    uint16_t length;                    /**< Payload length in bytes (big-endian) */
    const uint8_t* payload;             /**< Pointer to payload data (NOT copied) */
    uint32_t crc32;                     /**< Received CRC32 checksum (big-endian) */
} tlv_frame_t;

/**
 * @brief Upload session state machine
 */
typedef enum {
    UPLOAD_STATE_IDLE = 0,              /**< No active upload session */
    UPLOAD_STATE_RECEIVING,             /**< Receiving PUT_DATA chunks */
    UPLOAD_STATE_VALIDATING,            /**< Validating final CRC */
    UPLOAD_STATE_STORING,               /**< Writing to storage */
    UPLOAD_STATE_ERROR                  /**< Error occurred, cleanup pending */
} upload_state_t;

/**
 * @brief Upload session context
 *
 * Tracks state for single global upload session (mutual exclusion).
 * Only one pattern upload allowed at a time.
 */
typedef struct {
    upload_state_t state;               /**< Current state machine state */
    char filename[PATTERN_MAX_FILENAME];/**< Pattern filename from PUT_BEGIN */
    uint32_t expected_size;             /**< Total size from PUT_BEGIN */
    uint32_t expected_crc;              /**< Expected CRC from PUT_BEGIN */
    uint32_t bytes_received;            /**< Bytes accumulated from PUT_DATA */
    uint32_t crc_accumulator;           /**< Running CRC32 of pattern data */
    uint8_t* upload_buffer;             /**< Heap-allocated pattern buffer */
    uint32_t last_activity_ms;          /**< Timestamp for timeout detection */
    int client_fd;                      /**< WebSocket client owning session */
} upload_session_t;

/* ============================================================================
 * Public API Functions
 * ============================================================================ */

/**
 * @brief Initialize the protocol parser subsystem
 *
 * Creates upload session mutex and initializes state.
 * Must be called once at startup.
 *
 * @return ESP_OK on success
 * @return ESP_ERR_NO_MEM if mutex creation fails
 */
esp_err_t protocol_parser_init(void);

/**
 * @brief Deinitialize the protocol parser subsystem
 *
 * Cleans up active sessions, frees resources, deletes mutex.
 */
void protocol_parser_deinit(void);

/**
 * @brief Main entry point for protocol command dispatching
 *
 * Called by Task 3 WebSocket handler (network_manager.c line 1238).
 * Parses TLV frame, validates CRC, dispatches to appropriate handler.
 *
 * Thread-safe: Uses internal mutex for session protection.
 *
 * Processing Flow:
 * 1. Parse TLV structure (TYPE, LENGTH, PAYLOAD, CRC32)
 * 2. Validate frame CRC32 checksum
 * 3. Dispatch to message type handler
 * 4. Send response via WebSocket
 *
 * @param frame_data Raw binary frame from WebSocket (<=4096 bytes)
 * @param frame_len Frame length in bytes (minimum 7 bytes)
 * @param client_fd WebSocket client file descriptor
 *
 * @return ESP_OK if frame processed successfully
 * @return ESP_ERR_INVALID_ARG if frame_data is NULL or frame_len < 7
 * @return ESP_ERR_INVALID_CRC if CRC32 validation fails
 * @return ESP_FAIL if command processing fails
 *
 * @note Called from httpd task context
 */
esp_err_t protocol_dispatch_command(
    const uint8_t* frame_data,
    size_t frame_len,
    int client_fd
);

/**
 * @brief Check for upload session timeout
 *
 * Should be called from network_task() every 1 second.
 * Aborts upload if no PUT_DATA received for UPLOAD_TIMEOUT_MS (5s).
 *
 * Thread-safe: Uses internal mutex.
 */
void protocol_check_upload_timeout(void);

/**
 * @brief Get current upload session status
 *
 * @param out_filename Pointer to store filename (optional, can be NULL)
 * @param out_bytes_received Pointer to store bytes received (optional)
 * @param out_total_size Pointer to store expected size (optional)
 *
 * @return true if upload session is active
 * @return false if session is IDLE
 */
bool protocol_get_upload_status(
    char* out_filename,
    uint32_t* out_bytes_received,
    uint32_t* out_total_size
);

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_PARSER_H
