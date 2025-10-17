/**
 * @file storage_protocol.h
 * @brief Storage protocol integration layer
 *
 * Bridges WebSocket TLV protocol with storage APIs.
 * Provides command dispatch for pattern upload/download/delete operations.
 */

#ifndef PRISM_STORAGE_PROTOCOL_H
#define PRISM_STORAGE_PROTOCOL_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dispatch storage protocol command
 *
 * Parses TLV command and calls appropriate storage API.
 * Handles: PUT_BEGIN (0x10), PUT_CHUNK (0x11), PUT_END (0x12),
 *          DELETE (0x20), LIST (0x21)
 *
 * Protocol format:
 *   [Type: 1 byte][Length: 2 bytes][Value: N bytes]
 *
 * Error responses:
 *   [0x40][error_code]
 *   - 0x01: MAX_CLIENTS_REACHED
 *   - 0x02: BUFFER_OVERFLOW
 *   - 0x03: INVALID_TLV
 *   - 0x04: STORAGE_FULL
 *   - 0x05: PATTERN_NOT_FOUND
 *
 * Status responses:
 *   [0x30][status_code]
 *   - 0x00: OK
 *
 * @param data Input TLV frame data
 * @param len Frame length
 * @param response Output response buffer (caller allocates, min 64 bytes)
 * @param response_len Output response length
 * @return ESP_OK on success (check response for status)
 * @return ESP_ERR_INVALID_ARG if parameters invalid
 * @return ESP_ERR_INVALID_SIZE if TLV malformed
 * @return ESP_ERR_NO_MEM if storage quota exceeded
 * @return ESP_ERR_NOT_FOUND if pattern doesn't exist
 */
esp_err_t storage_protocol_dispatch(const uint8_t *data, size_t len,
                                     uint8_t *response, size_t *response_len);

/**
 * @brief Check if upload session is active
 *
 * Used by WebSocket layer to track upload state.
 *
 * @return true if upload in progress, false otherwise
 */
bool storage_protocol_is_upload_active(void);

/**
 * @brief Abort active upload session
 *
 * Called on WebSocket disconnect or timeout to clean up session state.
 */
void storage_protocol_abort_upload(void);

#ifdef __cplusplus
}
#endif

#endif // PRISM_STORAGE_PROTOCOL_H
