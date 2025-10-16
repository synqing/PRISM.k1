/**
 * @file test_bounds_checking.c
 * @brief Unit tests for secure bounds checking utilities
 */

#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "esp_heap_caps.h"
#include "prism_secure.h"

static const char* TAG = "TEST_BOUNDS";

/**
 * Test 1: safe_memcpy with valid inputs
 */
TEST_CASE("safe_memcpy_valid", "[bounds]")
{
    uint8_t src[64] = {0x01, 0x02, 0x03, 0x04};
    uint8_t dst[64] = {0};

    esp_err_t ret = safe_memcpy(dst, src, 4, sizeof(dst));
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_MEMORY(src, dst, 4);
}

/**
 * Test 2: safe_memcpy overflow detection
 */
TEST_CASE("safe_memcpy_overflow", "[bounds]")
{
    uint8_t src[128];
    uint8_t dst[64];

    // Try to copy 128 bytes into 64-byte buffer
    esp_err_t ret = safe_memcpy(dst, src, 128, sizeof(dst));
    TEST_ASSERT_EQUAL(ESP_ERR_BUFFER_OVERFLOW, ret);
}

/**
 * Test 3: safe_memcpy NULL pointer handling
 */
TEST_CASE("safe_memcpy_null", "[bounds]")
{
    uint8_t buf[64];

    esp_err_t ret = safe_memcpy(NULL, buf, 10, 64);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);

    ret = safe_memcpy(buf, NULL, 10, 64);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

/**
 * Test 4: safe_strncpy with valid inputs
 */
TEST_CASE("safe_strncpy_valid", "[bounds]")
{
    char dst[32];
    const char* src = "Hello, World!";

    esp_err_t ret = safe_strncpy(dst, src, sizeof(dst));
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING(src, dst);
}

/**
 * Test 5: safe_strncpy overflow detection
 */
TEST_CASE("safe_strncpy_overflow", "[bounds]")
{
    char dst[8];
    const char* src = "This string is way too long to fit";

    esp_err_t ret = safe_strncpy(dst, src, sizeof(dst));
    TEST_ASSERT_EQUAL(ESP_ERR_BUFFER_OVERFLOW, ret);
}

/**
 * Test 6: safe_strncpy guaranteed null termination
 */
TEST_CASE("safe_strncpy_null_term", "[bounds]")
{
    char dst[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    const char* src = "Short";

    esp_err_t ret = safe_strncpy(dst, src, sizeof(dst));
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING("Short", dst);
    TEST_ASSERT_EQUAL('\0', dst[5]);  // Ensure null termination
}

/**
 * Test 7: bounds_check valid access
 */
TEST_CASE("bounds_check_valid", "[bounds]")
{
    uint8_t buffer[128];

    // Valid accesses
    TEST_ASSERT_TRUE(bounds_check(buffer, 0, 10, 128));
    TEST_ASSERT_TRUE(bounds_check(buffer, 100, 28, 128));
    TEST_ASSERT_TRUE(bounds_check(buffer, 127, 1, 128));
}

/**
 * Test 8: bounds_check out of bounds
 */
TEST_CASE("bounds_check_out_of_bounds", "[bounds]")
{
    uint8_t buffer[128];

    // Out of bounds accesses
    TEST_ASSERT_FALSE(bounds_check(buffer, 120, 10, 128));  // 120 + 10 = 130 > 128
    TEST_ASSERT_FALSE(bounds_check(buffer, 0, 200, 128));   // 0 + 200 = 200 > 128
    TEST_ASSERT_FALSE(bounds_check(buffer, 128, 1, 128));   // 128 + 1 = 129 > 128
}

/**
 * Test 9: WebSocket frame length validation
 */
TEST_CASE("ws_validate_frame_length", "[bounds][websocket]")
{
    // Valid frame sizes
    TEST_ASSERT_EQUAL(ESP_OK, ws_validate_frame_length(0));
    TEST_ASSERT_EQUAL(ESP_OK, ws_validate_frame_length(1024));
    TEST_ASSERT_EQUAL(ESP_OK, ws_validate_frame_length(8192));

    // Invalid frame sizes
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, ws_validate_frame_length(8193));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, ws_validate_frame_length(16384));
}

/**
 * Test 10: WebSocket TLV bounds validation
 */
TEST_CASE("ws_validate_tlv_bounds", "[bounds][websocket]")
{
    uint8_t frame[100];
    uint8_t* frame_end = frame + sizeof(frame);

    // Valid TLV (type=1, length=10, payload fits)
    esp_err_t ret = ws_validate_tlv_bounds(1, 10, frame, frame_end);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Invalid TLV (payload too large)
    ret = ws_validate_tlv_bounds(1, 100, frame, frame_end);
    TEST_ASSERT_EQUAL(ESP_ERR_OUT_OF_BOUNDS, ret);

    // Invalid TLV (pointer beyond frame)
    ret = ws_validate_tlv_bounds(1, 10, frame_end + 1, frame_end);
    TEST_ASSERT_EQUAL(ESP_ERR_OUT_OF_BOUNDS, ret);
}

/**
 * Test 11: WebSocket session ID validation
 */
TEST_CASE("ws_validate_session_id", "[bounds][websocket]")
{
    // Valid session IDs
    TEST_ASSERT_EQUAL(ESP_OK, ws_validate_session_id(0));
    TEST_ASSERT_EQUAL(ESP_OK, ws_validate_session_id(100));
    TEST_ASSERT_EQUAL(ESP_OK, ws_validate_session_id(UINT16_MAX));

    // Invalid session IDs
    TEST_ASSERT_EQUAL(ESP_ERR_INTEGER_OVERFLOW, ws_validate_session_id(UINT16_MAX + 1));
    TEST_ASSERT_EQUAL(ESP_ERR_INTEGER_OVERFLOW, ws_validate_session_id(UINT32_MAX));
}

/**
 * Test 12: Pattern file validation
 */
TEST_CASE("pattern_validate_header_size", "[bounds][pattern]")
{
    // Valid header sizes
    TEST_ASSERT_EQUAL(ESP_OK, pattern_validate_header_size(64, 128));
    TEST_ASSERT_EQUAL(ESP_OK, pattern_validate_header_size(128, 128));

    // Invalid header sizes
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, pattern_validate_header_size(129, 128));
}

/**
 * Test 13: Pattern chunk offset validation
 */
TEST_CASE("pattern_validate_chunk_offset", "[bounds][pattern]")
{
    size_t file_size = 10000;

    // Valid chunks
    TEST_ASSERT_EQUAL(ESP_OK, pattern_validate_chunk_offset(0, 1000, file_size));
    TEST_ASSERT_EQUAL(ESP_OK, pattern_validate_chunk_offset(5000, 5000, file_size));
    TEST_ASSERT_EQUAL(ESP_OK, pattern_validate_chunk_offset(9999, 1, file_size));

    // Invalid chunks
    TEST_ASSERT_EQUAL(ESP_ERR_OUT_OF_BOUNDS, pattern_validate_chunk_offset(9000, 2000, file_size));
    TEST_ASSERT_EQUAL(ESP_ERR_OUT_OF_BOUNDS, pattern_validate_chunk_offset(10000, 1, file_size));
}

/**
 * Test 14: Pattern filename validation
 */
TEST_CASE("pattern_validate_filename", "[bounds][pattern]")
{
    // Valid filenames
    TEST_ASSERT_EQUAL(ESP_OK, pattern_validate_filename("pattern1.bin", 64));
    TEST_ASSERT_EQUAL(ESP_OK, pattern_validate_filename("test-file_v2.dat", 64));
    TEST_ASSERT_EQUAL(ESP_OK, pattern_validate_filename("file.txt", 64));

    // Invalid filenames
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, pattern_validate_filename("../escape.bin", 64));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, pattern_validate_filename("bad/slash.bin", 64));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, pattern_validate_filename("space file.bin", 64));

    // Too long
    char long_name[100];
    memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, pattern_validate_filename(long_name, 64));
}

/**
 * Test 15: safe_array_index valid access
 */
TEST_CASE("safe_array_index_valid", "[bounds]")
{
    uint32_t array[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    uint32_t* ptr = (uint32_t*)safe_array_index(array, 0, sizeof(uint32_t), 10);
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_EQUAL(0, *ptr);

    ptr = (uint32_t*)safe_array_index(array, 5, sizeof(uint32_t), 10);
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_EQUAL(5, *ptr);

    ptr = (uint32_t*)safe_array_index(array, 9, sizeof(uint32_t), 10);
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_EQUAL(9, *ptr);
}

/**
 * Test 16: safe_array_index out of bounds
 */
TEST_CASE("safe_array_index_out_of_bounds", "[bounds]")
{
    uint32_t array[10];

    uint32_t* ptr = (uint32_t*)safe_array_index(array, 10, sizeof(uint32_t), 10);
    TEST_ASSERT_NULL(ptr);

    ptr = (uint32_t*)safe_array_index(array, 100, sizeof(uint32_t), 10);
    TEST_ASSERT_NULL(ptr);
}

/**
 * Test 17: safe_buffer_append valid
 */
TEST_CASE("safe_buffer_append_valid", "[bounds]")
{
    uint8_t buffer[128] = {0};
    uint8_t data1[] = {0x01, 0x02, 0x03};
    uint8_t data2[] = {0x04, 0x05};

    size_t len = 0;

    esp_err_t ret = safe_buffer_append(buffer, len, data1, sizeof(data1), sizeof(buffer));
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    len += sizeof(data1);

    ret = safe_buffer_append(buffer, len, data2, sizeof(data2), sizeof(buffer));
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    len += sizeof(data2);

    // Verify data
    uint8_t expected[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    TEST_ASSERT_EQUAL_MEMORY(expected, buffer, len);
}

/**
 * Test 18: safe_buffer_append overflow
 */
TEST_CASE("safe_buffer_append_overflow", "[bounds]")
{
    uint8_t buffer[16];
    uint8_t data[32];

    esp_err_t ret = safe_buffer_append(buffer, 0, data, sizeof(data), sizeof(buffer));
    TEST_ASSERT_EQUAL(ESP_ERR_BUFFER_OVERFLOW, ret);

    ret = safe_buffer_append(buffer, 10, data, 20, sizeof(buffer));
    TEST_ASSERT_EQUAL(ESP_ERR_BUFFER_OVERFLOW, ret);
}

/**
 * Test 19: circular_index calculation
 */
TEST_CASE("circular_index", "[bounds]")
{
    size_t size = 10;

    TEST_ASSERT_EQUAL(0, circular_index(0, size));
    TEST_ASSERT_EQUAL(5, circular_index(5, size));
    TEST_ASSERT_EQUAL(9, circular_index(9, size));
    TEST_ASSERT_EQUAL(0, circular_index(10, size));
    TEST_ASSERT_EQUAL(5, circular_index(15, size));
    TEST_ASSERT_EQUAL(3, circular_index(103, size));
}

/**
 * Test 20: safe_strlen bounded
 */
TEST_CASE("safe_strlen", "[bounds]")
{
    const char* str1 = "Hello";
    TEST_ASSERT_EQUAL(5, safe_strlen(str1, 100));
    TEST_ASSERT_EQUAL(5, safe_strlen(str1, 10));
    TEST_ASSERT_EQUAL(5, safe_strlen(str1, 5));

    // String without null terminator
    char str2[10] = {'H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd'};
    TEST_ASSERT_EQUAL(10, safe_strlen(str2, 10));  // Stops at max_len
}

/**
 * Test 21: safe_strcmp bounded
 */
TEST_CASE("safe_strcmp", "[bounds]")
{
    TEST_ASSERT_EQUAL(0, safe_strcmp("Hello", "Hello", 10));
    TEST_ASSERT_LESS_THAN(0, safe_strcmp("Apple", "Banana", 10));
    TEST_ASSERT_GREATER_THAN(0, safe_strcmp("Zebra", "Apple", 10));

    // Compare only first N characters
    TEST_ASSERT_EQUAL(0, safe_strcmp("HelloWorld", "HelloThere", 5));
}

/**
 * Test 22: safe_atoi valid parsing
 */
TEST_CASE("safe_atoi_valid", "[bounds]")
{
    int32_t result;

    TEST_ASSERT_EQUAL(ESP_OK, safe_atoi("123", &result, 0, 1000));
    TEST_ASSERT_EQUAL(123, result);

    TEST_ASSERT_EQUAL(ESP_OK, safe_atoi("-456", &result, -1000, 1000));
    TEST_ASSERT_EQUAL(-456, result);

    TEST_ASSERT_EQUAL(ESP_OK, safe_atoi("  789  ", &result, 0, 1000));
    TEST_ASSERT_EQUAL(789, result);

    TEST_ASSERT_EQUAL(ESP_OK, safe_atoi("0", &result, 0, 1000));
    TEST_ASSERT_EQUAL(0, result);
}

/**
 * Test 23: safe_atoi range validation
 */
TEST_CASE("safe_atoi_range", "[bounds]")
{
    int32_t result;

    // Out of range (too large)
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, safe_atoi("1001", &result, 0, 1000));

    // Out of range (too small)
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, safe_atoi("-1", &result, 0, 1000));

    // Invalid format
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, safe_atoi("abc", &result, 0, 1000));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, safe_atoi("", &result, 0, 1000));
}

/**
 * Test 24: safe_add_size_t valid
 */
TEST_CASE("safe_add_size_t_valid", "[bounds]")
{
    size_t result;

    TEST_ASSERT_EQUAL(ESP_OK, safe_add_size_t(100, 200, &result));
    TEST_ASSERT_EQUAL(300, result);

    TEST_ASSERT_EQUAL(ESP_OK, safe_add_size_t(0, 100, &result));
    TEST_ASSERT_EQUAL(100, result);

    TEST_ASSERT_EQUAL(ESP_OK, safe_add_size_t(SIZE_MAX - 1, 1, &result));
    TEST_ASSERT_EQUAL(SIZE_MAX, result);
}

/**
 * Test 25: safe_add_size_t overflow
 */
TEST_CASE("safe_add_size_t_overflow", "[bounds]")
{
    size_t result;

    TEST_ASSERT_EQUAL(ESP_ERR_INTEGER_OVERFLOW,
                     safe_add_size_t(SIZE_MAX, 1, &result));

    TEST_ASSERT_EQUAL(ESP_ERR_INTEGER_OVERFLOW,
                     safe_add_size_t(SIZE_MAX / 2 + 1, SIZE_MAX / 2 + 1, &result));
}

/**
 * Test 26: safe_mul_size_t valid
 */
TEST_CASE("safe_mul_size_t_valid", "[bounds]")
{
    size_t result;

    TEST_ASSERT_EQUAL(ESP_OK, safe_mul_size_t(10, 20, &result));
    TEST_ASSERT_EQUAL(200, result);

    TEST_ASSERT_EQUAL(ESP_OK, safe_mul_size_t(0, 1000, &result));
    TEST_ASSERT_EQUAL(0, result);

    TEST_ASSERT_EQUAL(ESP_OK, safe_mul_size_t(1, SIZE_MAX, &result));
    TEST_ASSERT_EQUAL(SIZE_MAX, result);
}

/**
 * Test 27: safe_mul_size_t overflow
 */
TEST_CASE("safe_mul_size_t_overflow", "[bounds]")
{
    size_t result;

    TEST_ASSERT_EQUAL(ESP_ERR_INTEGER_OVERFLOW,
                     safe_mul_size_t(SIZE_MAX, 2, &result));

    TEST_ASSERT_EQUAL(ESP_ERR_INTEGER_OVERFLOW,
                     safe_mul_size_t(SIZE_MAX / 2 + 1, 2, &result));
}

/**
 * Test 28: Macro usage convenience
 */
TEST_CASE("macro_usage", "[bounds]")
{
    uint8_t src[32] = {1, 2, 3, 4};
    uint8_t dst[64];

    // SAFE_MEMCPY macro
    TEST_ASSERT_EQUAL(ESP_OK, SAFE_MEMCPY(dst, src, 4, sizeof(dst)));

    // SAFE_STRNCPY macro
    char str_dst[32];
    TEST_ASSERT_EQUAL(ESP_OK, SAFE_STRNCPY(str_dst, "Test", sizeof(str_dst)));

    // BOUNDS_CHECK macro
    uint8_t buffer[128];
    TEST_ASSERT_TRUE(BOUNDS_CHECK(buffer, 0, 10, 128));
    TEST_ASSERT_FALSE(BOUNDS_CHECK(buffer, 120, 20, 128));

    // CIRCULAR_INDEX macro
    TEST_ASSERT_EQUAL(3, CIRCULAR_INDEX(13, 10));

    // SAFE_ADD macro
    size_t result;
    TEST_ASSERT_EQUAL(ESP_OK, SAFE_ADD(100, 200, &result));
    TEST_ASSERT_EQUAL(300, result);
}

/**
 * Test 29: Stress test with random invalid inputs
 */
TEST_CASE("stress_test_random_inputs", "[bounds][stress]")
{
    uint8_t buffer[256];

    // Try various invalid operations (none should crash)
    for (int i = 0; i < 100; i++) {
        size_t random_size = (i * 137) % 512;  // Pseudo-random sizes
        size_t random_offset = (i * 251) % 512;

        // Should safely reject invalid operations
        safe_memcpy(buffer, buffer, random_size, sizeof(buffer));
        bounds_check(buffer, random_offset, random_size, sizeof(buffer));

        ws_validate_frame_length(random_size * 100);
        pattern_validate_chunk_offset(random_offset, random_size, 1000);
    }

    printf("Stress test completed: 100 iterations of random invalid inputs\n");
}

/**
 * Test 30: Memory leak check
 */
TEST_CASE("memory_leak_check", "[bounds]")
{
    size_t free_before = esp_get_free_heap_size();

    // Perform many operations
    for (int i = 0; i < 1000; i++) {
        uint8_t src[64], dst[64];
        safe_memcpy(dst, src, 32, sizeof(dst));

        char str1[32], str2[32] = "Test";
        safe_strncpy(str1, str2, sizeof(str1));

        size_t result;
        safe_add_size_t(i, i * 2, &result);
        safe_mul_size_t(i, 3, &result);
    }

    size_t free_after = esp_get_free_heap_size();

    // Should not leak memory (allow 100 bytes tolerance for stack variance)
    TEST_ASSERT_INT_WITHIN(100, free_before, free_after);

    printf("Memory leak check: before=%u, after=%u (diff=%d)\n",
           free_before, free_after, (int)(free_before - free_after));
}

/**
 * Test runner
 */
void app_main(void)
{
    printf("\n=== PRISM Bounds Checking Unit Tests ===\n");

    UNITY_BEGIN();

    // Basic memory operations
    RUN_TEST(safe_memcpy_valid);
    RUN_TEST(safe_memcpy_overflow);
    RUN_TEST(safe_memcpy_null);
    RUN_TEST(safe_strncpy_valid);
    RUN_TEST(safe_strncpy_overflow);
    RUN_TEST(safe_strncpy_null_term);

    // Bounds checking
    RUN_TEST(bounds_check_valid);
    RUN_TEST(bounds_check_out_of_bounds);

    // WebSocket validation
    RUN_TEST(ws_validate_frame_length);
    RUN_TEST(ws_validate_tlv_bounds);
    RUN_TEST(ws_validate_session_id);

    // Pattern file validation
    RUN_TEST(pattern_validate_header_size);
    RUN_TEST(pattern_validate_chunk_offset);
    RUN_TEST(pattern_validate_filename);

    // Array and buffer operations
    RUN_TEST(safe_array_index_valid);
    RUN_TEST(safe_array_index_out_of_bounds);
    RUN_TEST(safe_buffer_append_valid);
    RUN_TEST(safe_buffer_append_overflow);
    RUN_TEST(circular_index);

    // String operations
    RUN_TEST(safe_strlen);
    RUN_TEST(safe_strcmp);
    RUN_TEST(safe_atoi_valid);
    RUN_TEST(safe_atoi_range);

    // Integer overflow protection
    RUN_TEST(safe_add_size_t_valid);
    RUN_TEST(safe_add_size_t_overflow);
    RUN_TEST(safe_mul_size_t_valid);
    RUN_TEST(safe_mul_size_t_overflow);

    // Convenience and stress
    RUN_TEST(macro_usage);
    RUN_TEST(stress_test_random_inputs);
    RUN_TEST(memory_leak_check);

    UNITY_END();

    printf("\n=== All Tests Complete ===\n");
}
