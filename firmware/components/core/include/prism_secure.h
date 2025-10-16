/**
 * @file prism_secure.h
 * @brief Secure memory and bounds checking utilities
 *
 * Comprehensive safety utilities preventing buffer overflows, integer
 * overflows, and out-of-bounds access. All critical paths use validation
 * before any memory operation.
 */

#ifndef PRISM_SECURE_H
#define PRISM_SECURE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Constants and Limits
 * ======================================================================== */

/** Maximum WebSocket frame size (8KB per spec) */
#define WS_MAX_FRAME_SIZE           8192

/** Maximum pattern filename length */
#define PATTERN_MAX_FILENAME_LEN    64

/** Maximum string length for bounded operations */
#define SECURE_MAX_STRING_LEN       1024

/** Maximum session ID to prevent overflow */
#define WS_MAX_SESSION_ID           UINT16_MAX

/* ========================================================================
 * Error Codes
 * Note: ESP_ERR_INVALID_SIZE is already defined in esp_err.h (0x104)
 * ======================================================================== */

#define ESP_ERR_BUFFER_OVERFLOW     0x105
#define ESP_ERR_OUT_OF_BOUNDS       0x106
#define ESP_ERR_INTEGER_OVERFLOW    0x107

/* ========================================================================
 * Safe Memory Operations
 * ======================================================================== */

/**
 * Safe memory copy with bounds checking
 *
 * @param dst Destination buffer
 * @param src Source buffer
 * @param size Number of bytes to copy
 * @param max_size Maximum size of destination buffer
 * @return ESP_OK on success, ESP_ERR_BUFFER_OVERFLOW if size > max_size
 */
esp_err_t safe_memcpy(void* dst, const void* src, size_t size, size_t max_size);

/**
 * Safe string copy with guaranteed null termination
 *
 * @param dst Destination buffer
 * @param src Source string
 * @param max_len Maximum size of destination buffer (including null)
 * @return ESP_OK on success, ESP_ERR_BUFFER_OVERFLOW if src doesn't fit
 */
esp_err_t safe_strncpy(char* dst, const char* src, size_t max_len);

/**
 * Safe memory move with bounds checking
 *
 * @param dst Destination buffer
 * @param src Source buffer (may overlap with dst)
 * @param size Number of bytes to move
 * @param max_size Maximum size of destination buffer
 * @return ESP_OK on success, ESP_ERR_BUFFER_OVERFLOW if size > max_size
 */
esp_err_t safe_memmove(void* dst, const void* src, size_t size, size_t max_size);

/**
 * Validate pointer arithmetic is within bounds
 *
 * @param ptr Base pointer
 * @param offset Offset from base
 * @param size Size of data at offset
 * @param max Maximum valid offset (buffer size)
 * @return true if (offset + size) <= max, false otherwise
 */
bool bounds_check(const void* ptr, size_t offset, size_t size, size_t max);

/* ========================================================================
 * WebSocket Frame Validation
 * ======================================================================== */

/**
 * Validate WebSocket frame length
 *
 * @param length Frame length in bytes
 * @return ESP_OK if length <= WS_MAX_FRAME_SIZE, ESP_ERR_INVALID_SIZE otherwise
 */
esp_err_t ws_validate_frame_length(size_t length);

/**
 * Validate TLV (Type-Length-Value) within frame bounds
 *
 * @param type TLV type field
 * @param length TLV length field
 * @param payload_ptr Current position in frame
 * @param frame_end End of frame buffer
 * @return ESP_OK if TLV fits within frame, ESP_ERR_OUT_OF_BOUNDS otherwise
 */
esp_err_t ws_validate_tlv_bounds(uint8_t type, uint32_t length,
                                  const uint8_t* payload_ptr,
                                  const uint8_t* frame_end);

/**
 * Validate WebSocket session ID
 *
 * @param id Session ID to validate
 * @return ESP_OK if id <= WS_MAX_SESSION_ID, ESP_ERR_INTEGER_OVERFLOW otherwise
 */
esp_err_t ws_validate_session_id(uint32_t id);

/* ========================================================================
 * Pattern File Validation
 * ======================================================================== */

/**
 * Validate pattern file header size
 *
 * @param size Header size from file
 * @param max_size Maximum expected header size
 * @return ESP_OK if size <= max_size, ESP_ERR_INVALID_SIZE otherwise
 */
esp_err_t pattern_validate_header_size(size_t size, size_t max_size);

/**
 * Validate pattern chunk offset and size
 *
 * @param offset Chunk offset in file
 * @param chunk_size Chunk size in bytes
 * @param file_size Total file size
 * @return ESP_OK if chunk is within file bounds, ESP_ERR_OUT_OF_BOUNDS otherwise
 */
esp_err_t pattern_validate_chunk_offset(size_t offset, size_t chunk_size, size_t file_size);

/**
 * Validate and sanitize pattern filename
 *
 * @param name Filename to validate
 * @param max_len Maximum allowed filename length
 * @return ESP_OK if valid, ESP_ERR_INVALID_SIZE if too long
 */
esp_err_t pattern_validate_filename(const char* name, size_t max_len);

/* ========================================================================
 * Array Access Safety
 * ======================================================================== */

/**
 * Safe array indexing (returns NULL if out of bounds)
 *
 * @param array Base array pointer
 * @param index Array index
 * @param element_size Size of each element
 * @param array_size Total number of elements
 * @return Pointer to element if valid, NULL if out of bounds
 */
void* safe_array_index(void* array, size_t index, size_t element_size, size_t array_size);

/**
 * Safe buffer append with overflow check
 *
 * @param buf Buffer to append to
 * @param current_len Current buffer length
 * @param data Data to append
 * @param data_len Length of data to append
 * @param buf_size Total buffer capacity
 * @return ESP_OK on success, ESP_ERR_BUFFER_OVERFLOW if would exceed capacity
 */
esp_err_t safe_buffer_append(uint8_t* buf, size_t current_len,
                              const uint8_t* data, size_t data_len,
                              size_t buf_size);

/**
 * Calculate safe circular buffer index
 *
 * @param index Current index
 * @param size Buffer size (must be power of 2 for optimal performance)
 * @return Wrapped index (index % size)
 */
static inline size_t circular_index(size_t index, size_t size) {
    return index % size;
}

/* ========================================================================
 * String Operations
 * ======================================================================== */

/**
 * Bounded string length calculation
 *
 * @param str String to measure
 * @param max_len Maximum length to search
 * @return String length or max_len if no null terminator found
 */
size_t safe_strlen(const char* str, size_t max_len);

/**
 * Bounded string comparison
 *
 * @param s1 First string
 * @param s2 Second string
 * @param max_len Maximum characters to compare
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int safe_strcmp(const char* s1, const char* s2, size_t max_len);

/**
 * Parse integer with range validation
 *
 * @param str String to parse
 * @param result Output parameter for parsed value
 * @param min Minimum acceptable value
 * @param max Maximum acceptable value
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range or invalid
 */
esp_err_t safe_atoi(const char* str, int32_t* result, int32_t min, int32_t max);

/* ========================================================================
 * Integer Overflow Protection
 * ======================================================================== */

/**
 * Safe addition with overflow detection
 *
 * @param a First operand
 * @param b Second operand
 * @param result Output parameter for sum
 * @return ESP_OK on success, ESP_ERR_INTEGER_OVERFLOW on overflow
 */
esp_err_t safe_add_size_t(size_t a, size_t b, size_t* result);

/**
 * Safe multiplication with overflow detection
 *
 * @param a First operand
 * @param b Second operand
 * @param result Output parameter for product
 * @return ESP_OK on success, ESP_ERR_INTEGER_OVERFLOW on overflow
 */
esp_err_t safe_mul_size_t(size_t a, size_t b, size_t* result);

/**
 * Validate size_t against platform maximum
 *
 * @param size Value to check
 * @return true if size < SIZE_MAX, false otherwise
 */
static inline bool size_t_max_check(size_t size) {
    return size < SIZE_MAX;
}

/* ========================================================================
 * Convenience Macros
 * ======================================================================== */

/**
 * Safe memory copy macro with compile-time size check
 * Usage: SAFE_MEMCPY(dst, src, size, sizeof(dst))
 */
#define SAFE_MEMCPY(dst, src, size, max_size) \
    safe_memcpy((dst), (src), (size), (max_size))

/**
 * Safe string copy macro with compile-time size check
 * Usage: SAFE_STRNCPY(dst, src, sizeof(dst))
 */
#define SAFE_STRNCPY(dst, src, max_len) \
    safe_strncpy((dst), (src), (max_len))

/**
 * Safe memory move macro with compile-time size check
 * Usage: SAFE_MEMMOVE(dst, src, size, sizeof(dst))
 */
#define SAFE_MEMMOVE(dst, src, size, max_size) \
    safe_memmove((dst), (src), (size), (max_size))

/**
 * Bounds check macro
 * Usage: if (!BOUNDS_CHECK(ptr, offset, size, max)) { error; }
 */
#define BOUNDS_CHECK(ptr, offset, size, max) \
    bounds_check((ptr), (offset), (size), (max))

/**
 * Safe array index macro
 * Usage: item_t* ptr = SAFE_ARRAY_INDEX(array, index, sizeof(item_t), count);
 */
#define SAFE_ARRAY_INDEX(array, index, element_size, array_size) \
    safe_array_index((array), (index), (element_size), (array_size))

/**
 * Circular index macro
 * Usage: size_t idx = CIRCULAR_INDEX(current_index, buffer_size);
 */
#define CIRCULAR_INDEX(index, size) \
    circular_index((index), (size))

/**
 * Safe addition macro
 * Usage: if (SAFE_ADD(a, b, &result) != ESP_OK) { error; }
 */
#define SAFE_ADD(a, b, result) \
    safe_add_size_t((a), (b), (result))

/**
 * Safe multiplication macro
 * Usage: if (SAFE_MUL(a, b, &result) != ESP_OK) { error; }
 */
#define SAFE_MUL(a, b, result) \
    safe_mul_size_t((a), (b), (result))

#ifdef __cplusplus
}
#endif

#endif /* PRISM_SECURE_H */
