/**
 * @file prism_secure.c
 * @brief Secure memory and bounds checking utilities implementation
 */

#include "prism_secure.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static const char* TAG = "SECURE";

/* ========================================================================
 * Safe Memory Operations
 * ======================================================================== */

esp_err_t safe_memcpy(void* dst, const void* src, size_t size, size_t max_size)
{
    if (dst == NULL || src == NULL) {
        ESP_LOGE(TAG, "NULL pointer in safe_memcpy");
        return ESP_ERR_INVALID_ARG;
    }

    if (size > max_size) {
        ESP_LOGE(TAG, "Buffer overflow in safe_memcpy: size %u > max %u",
                 size, max_size);
        return ESP_ERR_BUFFER_OVERFLOW;
    }

    if (size == 0) {
        return ESP_OK;  // Nothing to copy
    }

    memcpy(dst, src, size);
    return ESP_OK;
}

esp_err_t safe_strncpy(char* dst, const char* src, size_t max_len)
{
    if (dst == NULL || src == NULL) {
        ESP_LOGE(TAG, "NULL pointer in safe_strncpy");
        return ESP_ERR_INVALID_ARG;
    }

    if (max_len == 0) {
        ESP_LOGE(TAG, "Zero-length buffer in safe_strncpy");
        return ESP_ERR_INVALID_SIZE;
    }

    // Check if source fits (including null terminator)
    size_t src_len = safe_strlen(src, max_len);
    if (src_len >= max_len) {
        ESP_LOGE(TAG, "String overflow in safe_strncpy: len %u >= max %u",
                 src_len, max_len);
        return ESP_ERR_BUFFER_OVERFLOW;
    }

    // Copy and ensure null termination
    strncpy(dst, src, max_len - 1);
    dst[max_len - 1] = '\0';

    return ESP_OK;
}

esp_err_t safe_memmove(void* dst, const void* src, size_t size, size_t max_size)
{
    if (dst == NULL || src == NULL) {
        ESP_LOGE(TAG, "NULL pointer in safe_memmove");
        return ESP_ERR_INVALID_ARG;
    }

    if (size > max_size) {
        ESP_LOGE(TAG, "Buffer overflow in safe_memmove: size %u > max %u",
                 size, max_size);
        return ESP_ERR_BUFFER_OVERFLOW;
    }

    if (size == 0) {
        return ESP_OK;  // Nothing to move
    }

    memmove(dst, src, size);
    return ESP_OK;
}

bool bounds_check(const void* ptr, size_t offset, size_t size, size_t max)
{
    if (ptr == NULL) {
        ESP_LOGE(TAG, "NULL pointer in bounds_check");
        return false;
    }

    // Check for integer overflow in offset + size
    size_t end;
    if (safe_add_size_t(offset, size, &end) != ESP_OK) {
        ESP_LOGE(TAG, "Integer overflow in bounds_check: offset %u + size %u",
                 offset, size);
        return false;
    }

    if (end > max) {
        ESP_LOGE(TAG, "Out of bounds: offset %u + size %u = %u > max %u",
                 offset, size, end, max);
        return false;
    }

    return true;
}

/* ========================================================================
 * WebSocket Frame Validation
 * ======================================================================== */

esp_err_t ws_validate_frame_length(size_t length)
{
    if (length > WS_MAX_FRAME_SIZE) {
        ESP_LOGE(TAG, "WebSocket frame too large: %u > %d",
                 length, WS_MAX_FRAME_SIZE);
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

esp_err_t ws_validate_tlv_bounds(uint8_t type, uint32_t length,
                                  const uint8_t* payload_ptr,
                                  const uint8_t* frame_end)
{
    if (payload_ptr == NULL || frame_end == NULL) {
        ESP_LOGE(TAG, "NULL pointer in ws_validate_tlv_bounds");
        return ESP_ERR_INVALID_ARG;
    }

    if (payload_ptr > frame_end) {
        ESP_LOGE(TAG, "Payload pointer beyond frame end");
        return ESP_ERR_OUT_OF_BOUNDS;
    }

    // Calculate remaining space in frame
    size_t remaining = frame_end - payload_ptr;

    // TLV requires: 1 byte type + 4 bytes length + length bytes payload
    const size_t tlv_header_size = 5;
    if (remaining < tlv_header_size) {
        ESP_LOGE(TAG, "TLV header exceeds frame: remaining %u < %u",
                 remaining, tlv_header_size);
        return ESP_ERR_OUT_OF_BOUNDS;
    }

    // Check if payload fits
    if (length > (remaining - tlv_header_size)) {
        ESP_LOGE(TAG, "TLV payload exceeds frame: type %u, length %u, remaining %u",
                 type, length, remaining - tlv_header_size);
        return ESP_ERR_OUT_OF_BOUNDS;
    }

    return ESP_OK;
}

esp_err_t ws_validate_session_id(uint32_t id)
{
    if (id > WS_MAX_SESSION_ID) {
        ESP_LOGE(TAG, "Session ID overflow: %u > %d", id, WS_MAX_SESSION_ID);
        return ESP_ERR_INTEGER_OVERFLOW;
    }
    return ESP_OK;
}

/* ========================================================================
 * Pattern File Validation
 * ======================================================================== */

esp_err_t pattern_validate_header_size(size_t size, size_t max_size)
{
    if (size > max_size) {
        ESP_LOGE(TAG, "Pattern header too large: %u > %u", size, max_size);
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

esp_err_t pattern_validate_chunk_offset(size_t offset, size_t chunk_size, size_t file_size)
{
    // Check for integer overflow
    size_t chunk_end;
    if (safe_add_size_t(offset, chunk_size, &chunk_end) != ESP_OK) {
        ESP_LOGE(TAG, "Chunk offset overflow: %u + %u", offset, chunk_size);
        return ESP_ERR_INTEGER_OVERFLOW;
    }

    if (chunk_end > file_size) {
        ESP_LOGE(TAG, "Chunk exceeds file: offset %u + size %u = %u > file %u",
                 offset, chunk_size, chunk_end, file_size);
        return ESP_ERR_OUT_OF_BOUNDS;
    }

    return ESP_OK;
}

esp_err_t pattern_validate_filename(const char* name, size_t max_len)
{
    if (name == NULL) {
        ESP_LOGE(TAG, "NULL filename in pattern_validate_filename");
        return ESP_ERR_INVALID_ARG;
    }

    size_t len = safe_strlen(name, max_len);
    if (len >= max_len) {
        ESP_LOGE(TAG, "Filename too long: %u >= %u", len, max_len);
        return ESP_ERR_INVALID_SIZE;
    }

    // Check for invalid characters (basic sanitization)
    for (size_t i = 0; i < len; i++) {
        char c = name[i];
        // Allow: alphanumeric, dash, underscore, period
        if (!isalnum((unsigned char)c) && c != '-' && c != '_' && c != '.') {
            ESP_LOGE(TAG, "Invalid character in filename: '%c' at position %u", c, i);
            return ESP_ERR_INVALID_ARG;
        }
    }

    // Prevent directory traversal
    if (strstr(name, "..") != NULL) {
        ESP_LOGE(TAG, "Directory traversal attempt in filename: %s", name);
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

/* ========================================================================
 * Array Access Safety
 * ======================================================================== */

void* safe_array_index(void* array, size_t index, size_t element_size, size_t array_size)
{
    if (array == NULL) {
        ESP_LOGE(TAG, "NULL array in safe_array_index");
        return NULL;
    }

    if (index >= array_size) {
        ESP_LOGE(TAG, "Array index out of bounds: %u >= %u", index, array_size);
        return NULL;
    }

    // Calculate offset with overflow check
    size_t offset;
    if (safe_mul_size_t(index, element_size, &offset) != ESP_OK) {
        ESP_LOGE(TAG, "Offset overflow: index %u * element_size %u",
                 index, element_size);
        return NULL;
    }

    return (uint8_t*)array + offset;
}

esp_err_t safe_buffer_append(uint8_t* buf, size_t current_len,
                              const uint8_t* data, size_t data_len,
                              size_t buf_size)
{
    if (buf == NULL || data == NULL) {
        ESP_LOGE(TAG, "NULL pointer in safe_buffer_append");
        return ESP_ERR_INVALID_ARG;
    }

    // Check current position is valid
    if (current_len > buf_size) {
        ESP_LOGE(TAG, "Current length exceeds buffer: %u > %u",
                 current_len, buf_size);
        return ESP_ERR_INVALID_ARG;
    }

    // Check if append would overflow
    size_t new_len;
    if (safe_add_size_t(current_len, data_len, &new_len) != ESP_OK) {
        ESP_LOGE(TAG, "Append length overflow: %u + %u",
                 current_len, data_len);
        return ESP_ERR_INTEGER_OVERFLOW;
    }

    if (new_len > buf_size) {
        ESP_LOGE(TAG, "Append would overflow buffer: %u + %u = %u > %u",
                 current_len, data_len, new_len, buf_size);
        return ESP_ERR_BUFFER_OVERFLOW;
    }

    // Safe to append
    memcpy(buf + current_len, data, data_len);
    return ESP_OK;
}

/* ========================================================================
 * String Operations
 * ======================================================================== */

size_t safe_strlen(const char* str, size_t max_len)
{
    if (str == NULL) {
        return 0;
    }

    size_t len = 0;
    while (len < max_len && str[len] != '\0') {
        len++;
    }

    return len;
}

int safe_strcmp(const char* s1, const char* s2, size_t max_len)
{
    if (s1 == NULL || s2 == NULL) {
        ESP_LOGE(TAG, "NULL pointer in safe_strcmp");
        return 0;  // Treat as equal to avoid undefined behavior
    }

    for (size_t i = 0; i < max_len; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;  // Both strings ended
        }
    }

    return 0;  // Equal up to max_len
}

esp_err_t safe_atoi(const char* str, int32_t* result, int32_t min, int32_t max)
{
    if (str == NULL || result == NULL) {
        ESP_LOGE(TAG, "NULL pointer in safe_atoi");
        return ESP_ERR_INVALID_ARG;
    }

    // Skip leading whitespace
    while (isspace((unsigned char)*str)) {
        str++;
    }

    // Parse sign
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Parse digits
    int32_t value = 0;
    bool has_digits = false;

    while (isdigit((unsigned char)*str)) {
        has_digits = true;

        int digit = *str - '0';

        // Check for overflow before multiplication
        if (value > (INT32_MAX / 10)) {
            ESP_LOGE(TAG, "Integer overflow in safe_atoi");
            return ESP_ERR_INTEGER_OVERFLOW;
        }

        value *= 10;

        // Check for overflow before addition
        if (value > (INT32_MAX - digit)) {
            ESP_LOGE(TAG, "Integer overflow in safe_atoi");
            return ESP_ERR_INTEGER_OVERFLOW;
        }

        value += digit;
        str++;
    }

    if (!has_digits) {
        ESP_LOGE(TAG, "No digits in safe_atoi");
        return ESP_ERR_INVALID_ARG;
    }

    value *= sign;

    // Range check
    if (value < min || value > max) {
        ESP_LOGE(TAG, "Value out of range in safe_atoi: %d not in [%d, %d]",
                 value, min, max);
        return ESP_ERR_INVALID_ARG;
    }

    *result = value;
    return ESP_OK;
}

/* ========================================================================
 * Integer Overflow Protection
 * ======================================================================== */

esp_err_t safe_add_size_t(size_t a, size_t b, size_t* result)
{
    if (result == NULL) {
        ESP_LOGE(TAG, "NULL result pointer in safe_add_size_t");
        return ESP_ERR_INVALID_ARG;
    }

    // Check for overflow: a + b > SIZE_MAX
    // Rearrange to avoid overflow: a > SIZE_MAX - b
    if (a > (SIZE_MAX - b)) {
        ESP_LOGE(TAG, "Addition overflow: %u + %u > SIZE_MAX", a, b);
        return ESP_ERR_INTEGER_OVERFLOW;
    }

    *result = a + b;
    return ESP_OK;
}

esp_err_t safe_mul_size_t(size_t a, size_t b, size_t* result)
{
    if (result == NULL) {
        ESP_LOGE(TAG, "NULL result pointer in safe_mul_size_t");
        return ESP_ERR_INVALID_ARG;
    }

    // Handle zero cases
    if (a == 0 || b == 0) {
        *result = 0;
        return ESP_OK;
    }

    // Check for overflow: a * b > SIZE_MAX
    // Rearrange to avoid overflow: a > SIZE_MAX / b
    if (a > (SIZE_MAX / b)) {
        ESP_LOGE(TAG, "Multiplication overflow: %u * %u > SIZE_MAX", a, b);
        return ESP_ERR_INTEGER_OVERFLOW;
    }

    *result = a * b;
    return ESP_OK;
}
