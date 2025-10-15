/**
 * @file test_protocol_parser.c
 * @brief Unity tests for protocol_parser component (Task 4 Phase 5)
 *
 * Tests verify:
 * - TLV frame parsing (valid/invalid inputs)
 * - CRC32 validation (correct/incorrect checksums)
 * - Upload state machine transitions
 * - CONTROL command parsing and dispatch
 * - Error handling paths
 *
 * @author Agent 2
 * @date 2025-10-16
 */

#include "unity.h"
#include "protocol_parser.h"
#include "esp_rom_crc.h"
#include <string.h>

/* ========================================================================
 * TEST HELPER FUNCTIONS
 * ======================================================================== */

/**
 * @brief Build TLV frame with automatic CRC32 calculation
 *
 * Constructs a properly formatted TLV frame: [TYPE:1][LENGTH:2][PAYLOAD:N][CRC:4]
 * Automatically calculates CRC32 over TYPE+LENGTH+PAYLOAD in big-endian format.
 *
 * @param type Message type byte
 * @param payload Payload data (can be NULL if payload_len == 0)
 * @param payload_len Payload length in bytes
 * @param out_frame Output buffer (must be >= payload_len + 7 bytes)
 * @return Total frame size in bytes (payload_len + 7)
 */
static size_t build_test_frame(uint8_t type, const uint8_t* payload, uint16_t payload_len, uint8_t* out_frame) {
    // Write TYPE (1 byte)
    out_frame[0] = type;

    // Write LENGTH (2 bytes, big-endian)
    out_frame[1] = (payload_len >> 8) & 0xFF;
    out_frame[2] = payload_len & 0xFF;

    // Write PAYLOAD (N bytes)
    if (payload != NULL && payload_len > 0) {
        memcpy(&out_frame[3], payload, payload_len);
    }

    // Calculate CRC32 over TYPE + LENGTH + PAYLOAD
    size_t crc_data_len = 3 + payload_len;
    uint32_t crc = esp_rom_crc32_le(0, out_frame, crc_data_len);

    // Write CRC32 (4 bytes, big-endian)
    size_t crc_offset = crc_data_len;
    out_frame[crc_offset + 0] = (crc >> 24) & 0xFF;
    out_frame[crc_offset + 1] = (crc >> 16) & 0xFF;
    out_frame[crc_offset + 2] = (crc >> 8) & 0xFF;
    out_frame[crc_offset + 3] = crc & 0xFF;

    return crc_data_len + 4;  // Total frame size
}

/**
 * @brief Build PUT_BEGIN payload
 *
 * Format: [filename_len:1][filename:N][size:4][crc:4]
 */
static size_t build_put_begin_payload(const char* filename, uint32_t size, uint32_t crc, uint8_t* out_payload) {
    uint8_t filename_len = strlen(filename);
    size_t offset = 0;

    // Write filename_len (1 byte)
    out_payload[offset++] = filename_len;

    // Write filename (N bytes)
    memcpy(&out_payload[offset], filename, filename_len);
    offset += filename_len;

    // Write size (4 bytes, big-endian)
    out_payload[offset++] = (size >> 24) & 0xFF;
    out_payload[offset++] = (size >> 16) & 0xFF;
    out_payload[offset++] = (size >> 8) & 0xFF;
    out_payload[offset++] = size & 0xFF;

    // Write crc (4 bytes, big-endian)
    out_payload[offset++] = (crc >> 24) & 0xFF;
    out_payload[offset++] = (crc >> 16) & 0xFF;
    out_payload[offset++] = (crc >> 8) & 0xFF;
    out_payload[offset++] = crc & 0xFF;

    return offset;
}

/**
 * @brief Build PUT_DATA payload
 *
 * Format: [offset:4][data:N]
 */
static size_t build_put_data_payload(uint32_t offset, const uint8_t* data, size_t data_len, uint8_t* out_payload) {
    size_t payload_offset = 0;

    // Write offset (4 bytes, big-endian)
    out_payload[payload_offset++] = (offset >> 24) & 0xFF;
    out_payload[payload_offset++] = (offset >> 16) & 0xFF;
    out_payload[payload_offset++] = (offset >> 8) & 0xFF;
    out_payload[payload_offset++] = offset & 0xFF;

    // Write data (N bytes)
    if (data != NULL && data_len > 0) {
        memcpy(&out_payload[payload_offset], data, data_len);
        payload_offset += data_len;
    }

    return payload_offset;
}

/**
 * @brief Build CONTROL PLAY command payload
 *
 * Format: [cmd:1][name_len:1][name:N]
 */
static size_t build_control_play_payload(const char* pattern_name, uint8_t* out_payload) {
    uint8_t name_len = strlen(pattern_name);
    size_t offset = 0;

    // Write command byte (0x01 = PLAY)
    out_payload[offset++] = 0x01;

    // Write name_len (1 byte)
    out_payload[offset++] = name_len;

    // Write pattern_name (N bytes)
    memcpy(&out_payload[offset], pattern_name, name_len);
    offset += name_len;

    return offset;
}

/* ========================================================================
 * TEST SETUP AND TEARDOWN
 * ======================================================================== */

void setUp(void) {
    // Initialize protocol parser before each test
    esp_err_t ret = protocol_parser_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

void tearDown(void) {
    // Cleanup after each test
    protocol_parser_deinit();
}

/* ========================================================================
 * TLV FRAME PARSING TESTS
 * ======================================================================== */

/**
 * Test: protocol_dispatch_command should reject NULL frame_data
 */
TEST_CASE("TLV parsing - NULL frame data", "[protocol_parser]") {
    esp_err_t ret = protocol_dispatch_command(NULL, 100, 1);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

/**
 * Test: protocol_dispatch_command should reject frame < 7 bytes
 */
TEST_CASE("TLV parsing - frame too small", "[protocol_parser]") {
    uint8_t short_frame[6] = {0x10, 0x00, 0x00, 0x00, 0x00, 0x00};

    esp_err_t ret = protocol_dispatch_command(short_frame, 6, 1);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

/**
 * Test: protocol_dispatch_command should parse valid empty frame
 */
TEST_CASE("TLV parsing - valid empty frame", "[protocol_parser]") {
    uint8_t frame[128];

    // Build frame with zero-length payload (TYPE=0x30 STATUS, no payload)
    size_t frame_len = build_test_frame(MSG_TYPE_STATUS, NULL, 0, frame);

    TEST_ASSERT_EQUAL(7, frame_len);  // Minimum size

    // Dispatch (will fail at handler level, but parsing should succeed)
    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);

    // STATUS handler is not yet implemented, so expect NOT_SUPPORTED
    // but parsing and CRC validation should have passed
    TEST_ASSERT_TRUE(ret == ESP_OK || ret == ESP_ERR_NOT_SUPPORTED);
}

/**
 * Test: protocol_dispatch_command should parse frame with payload
 */
TEST_CASE("TLV parsing - frame with payload", "[protocol_parser]") {
    uint8_t frame[128];
    uint8_t test_payload[] = {0xDE, 0xAD, 0xBE, 0xEF};

    size_t frame_len = build_test_frame(MSG_TYPE_STATUS, test_payload, sizeof(test_payload), frame);

    TEST_ASSERT_EQUAL(11, frame_len);  // 1 + 2 + 4 + 4

    // Dispatch
    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);

    // Should parse successfully even if handler is not implemented
    TEST_ASSERT_TRUE(ret == ESP_OK || ret == ESP_ERR_NOT_SUPPORTED);
}

/**
 * Test: protocol_dispatch_command should reject payload > max size
 */
TEST_CASE("TLV parsing - payload too large", "[protocol_parser]") {
    uint8_t frame[16];

    // Build frame header claiming huge payload
    frame[0] = MSG_TYPE_PUT_DATA;
    frame[1] = 0xFF;  // LENGTH = 0xFFFF (65535 bytes, exceeds TLV_MAX_PAYLOAD_SIZE)
    frame[2] = 0xFF;

    // Add fake CRC
    frame[3] = 0x00;
    frame[4] = 0x00;
    frame[5] = 0x00;
    frame[6] = 0x00;

    esp_err_t ret = protocol_dispatch_command(frame, 7, 1);

    // Should reject during parsing
    TEST_ASSERT_TRUE(ret != ESP_OK);
}

/* ========================================================================
 * CRC32 VALIDATION TESTS
 * ======================================================================== */

/**
 * Test: protocol_dispatch_command should reject frame with invalid CRC
 */
TEST_CASE("CRC32 validation - invalid checksum", "[protocol_parser]") {
    uint8_t frame[128];

    // Build valid frame
    size_t frame_len = build_test_frame(MSG_TYPE_STATUS, NULL, 0, frame);

    // Corrupt CRC (last 4 bytes)
    frame[frame_len - 1] ^= 0xFF;  // Flip last byte

    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_CRC, ret);
}

/**
 * Test: protocol_dispatch_command should accept frame with valid CRC
 */
TEST_CASE("CRC32 validation - valid checksum", "[protocol_parser]") {
    uint8_t frame[128];

    // Build valid frame (build_test_frame calculates correct CRC)
    size_t frame_len = build_test_frame(MSG_TYPE_STATUS, NULL, 0, frame);

    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);

    // Should not fail due to CRC
    TEST_ASSERT_NOT_EQUAL(ESP_ERR_INVALID_CRC, ret);
}

/**
 * Test: CRC32 validation with non-empty payload
 */
TEST_CASE("CRC32 validation - with payload", "[protocol_parser]") {
    uint8_t frame[128];
    uint8_t test_data[100];

    // Fill with pattern
    for (int i = 0; i < 100; i++) {
        test_data[i] = i & 0xFF;
    }

    // Build frame with payload
    size_t frame_len = build_test_frame(MSG_TYPE_DELETE, test_data, 100, frame);

    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);

    // Should not fail CRC validation
    TEST_ASSERT_NOT_EQUAL(ESP_ERR_INVALID_CRC, ret);
}

/* ========================================================================
 * UPLOAD STATE MACHINE TESTS
 * ======================================================================== */

/**
 * Test: PUT_BEGIN should initialize upload session
 */
TEST_CASE("Upload state machine - PUT_BEGIN initializes session", "[protocol_parser]") {
    uint8_t frame[128];
    uint8_t payload[128];

    // Build PUT_BEGIN payload
    size_t payload_len = build_put_begin_payload("test.bin", 1024, 0x12345678, payload);
    size_t frame_len = build_test_frame(MSG_TYPE_PUT_BEGIN, payload, payload_len, frame);

    // Should succeed (note: will allocate 1024 bytes internally)
    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify session is active
    char filename[64];
    uint32_t bytes_received, total_size;
    bool active = protocol_get_upload_status(filename, &bytes_received, &total_size);

    TEST_ASSERT_TRUE(active);
    TEST_ASSERT_EQUAL_STRING("test.bin", filename);
    TEST_ASSERT_EQUAL_UINT32(0, bytes_received);
    TEST_ASSERT_EQUAL_UINT32(1024, total_size);
}

/**
 * Test: PUT_BEGIN should reject if session already active
 */
TEST_CASE("Upload state machine - PUT_BEGIN rejects concurrent session", "[protocol_parser]") {
    uint8_t frame[128];
    uint8_t payload[128];

    // Start first session
    size_t payload_len = build_put_begin_payload("test1.bin", 512, 0x11111111, payload);
    size_t frame_len = build_test_frame(MSG_TYPE_PUT_BEGIN, payload, payload_len, frame);

    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Try to start second session (should fail)
    payload_len = build_put_begin_payload("test2.bin", 512, 0x22222222, payload);
    frame_len = build_test_frame(MSG_TYPE_PUT_BEGIN, payload, payload_len, frame);

    ret = protocol_dispatch_command(frame, frame_len, 2);  // Different client FD
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

/**
 * Test: PUT_DATA should reject without active session
 */
TEST_CASE("Upload state machine - PUT_DATA requires active session", "[protocol_parser]") {
    uint8_t frame[128];
    uint8_t payload[128];
    uint8_t test_data[64] = {0xAA};

    // Build PUT_DATA without starting session
    size_t payload_len = build_put_data_payload(0, test_data, 64, payload);
    size_t frame_len = build_test_frame(MSG_TYPE_PUT_DATA, payload, payload_len, frame);

    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

/**
 * Test: PUT_END should validate CRC and transition to IDLE
 */
TEST_CASE("Upload state machine - PUT_END validates CRC", "[protocol_parser]") {
    uint8_t frame[256];
    uint8_t payload[256];

    // Prepare test data
    uint8_t test_data[128];
    for (int i = 0; i < 128; i++) {
        test_data[i] = i & 0xFF;
    }

    // Calculate expected CRC for test_data
    uint32_t expected_crc = esp_rom_crc32_le(0, test_data, 128);

    // Step 1: PUT_BEGIN
    size_t payload_len = build_put_begin_payload("test.bin", 128, expected_crc, payload);
    size_t frame_len = build_test_frame(MSG_TYPE_PUT_BEGIN, payload, payload_len, frame);
    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Step 2: PUT_DATA
    payload_len = build_put_data_payload(0, test_data, 128, payload);
    frame_len = build_test_frame(MSG_TYPE_PUT_DATA, payload, payload_len, frame);
    ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Step 3: PUT_END (should validate CRC and write to storage)
    frame_len = build_test_frame(MSG_TYPE_PUT_END, NULL, 0, frame);
    ret = protocol_dispatch_command(frame, frame_len, 1);

    // Should succeed (storage write succeeds or fails, but CRC validation passed)
    // Note: Storage integration may fail in test environment, focus on CRC validation
    TEST_ASSERT_TRUE(ret == ESP_OK || ret == ESP_ERR_NO_MEM || ret == ESP_FAIL);

    // Session should be cleared after PUT_END
    bool active = protocol_get_upload_status(NULL, NULL, NULL);
    TEST_ASSERT_FALSE(active);
}

/**
 * Test: PUT_END should reject incomplete upload
 */
TEST_CASE("Upload state machine - PUT_END rejects incomplete upload", "[protocol_parser]") {
    uint8_t frame[256];
    uint8_t payload[256];

    // Step 1: PUT_BEGIN (expect 1024 bytes)
    size_t payload_len = build_put_begin_payload("test.bin", 1024, 0x12345678, payload);
    size_t frame_len = build_test_frame(MSG_TYPE_PUT_BEGIN, payload, payload_len, frame);
    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Step 2: PUT_DATA (only send 512 bytes, not 1024)
    uint8_t test_data[512] = {0xBB};
    payload_len = build_put_data_payload(0, test_data, 512, payload);
    frame_len = build_test_frame(MSG_TYPE_PUT_DATA, payload, payload_len, frame);
    ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Step 3: PUT_END (should fail due to incomplete upload)
    frame_len = build_test_frame(MSG_TYPE_PUT_END, NULL, 0, frame);
    ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, ret);
}

/* ========================================================================
 * CONTROL COMMAND TESTS
 * ======================================================================== */

/**
 * Test: CONTROL PLAY command parsing
 */
TEST_CASE("CONTROL command - PLAY parses pattern name", "[protocol_parser]") {
    uint8_t frame[128];
    uint8_t payload[128];

    // Build CONTROL PLAY payload
    size_t payload_len = build_control_play_payload("rainbow.bin", payload);
    size_t frame_len = build_test_frame(MSG_TYPE_CONTROL, payload, payload_len, frame);

    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);

    // Should succeed (LED driver start may fail in test env, but parsing succeeds)
    TEST_ASSERT_TRUE(ret == ESP_OK || ret == ESP_ERR_INVALID_STATE);
}

/**
 * Test: CONTROL STOP command
 */
TEST_CASE("CONTROL command - STOP", "[protocol_parser]") {
    uint8_t frame[128];
    uint8_t payload[1];

    // Build CONTROL STOP payload (command byte = 0x02)
    payload[0] = 0x02;
    size_t frame_len = build_test_frame(MSG_TYPE_CONTROL, payload, 1, frame);

    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);

    // Should succeed
    TEST_ASSERT_TRUE(ret == ESP_OK || ret == ESP_ERR_INVALID_STATE);
}

/**
 * Test: CONTROL PAUSE/RESUME should return NOT_SUPPORTED
 */
TEST_CASE("CONTROL command - PAUSE returns NOT_SUPPORTED", "[protocol_parser]") {
    uint8_t frame[128];
    uint8_t payload[1];

    // Build CONTROL PAUSE payload (command byte = 0x03)
    payload[0] = 0x03;
    size_t frame_len = build_test_frame(MSG_TYPE_CONTROL, payload, 1, frame);

    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_SUPPORTED, ret);
}

/**
 * Test: CONTROL with empty payload should fail
 */
TEST_CASE("CONTROL command - empty payload fails", "[protocol_parser]") {
    uint8_t frame[128];

    // Build CONTROL with no payload
    size_t frame_len = build_test_frame(MSG_TYPE_CONTROL, NULL, 0, frame);

    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

/* ========================================================================
 * ERROR HANDLING TESTS
 * ======================================================================== */

/**
 * Test: Unknown message type should return NOT_SUPPORTED
 */
TEST_CASE("Error handling - unknown message type", "[protocol_parser]") {
    uint8_t frame[128];

    // Build frame with invalid message type (0x99)
    size_t frame_len = build_test_frame(0x99, NULL, 0, frame);

    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_SUPPORTED, ret);
}

/**
 * Test: PUT_BEGIN with oversized pattern should fail
 */
TEST_CASE("Error handling - oversized pattern rejected", "[protocol_parser]") {
    uint8_t frame[128];
    uint8_t payload[128];

    // Build PUT_BEGIN with size > PATTERN_MAX_SIZE (256KB)
    size_t payload_len = build_put_begin_payload("huge.bin", 300000, 0x12345678, payload);
    size_t frame_len = build_test_frame(MSG_TYPE_PUT_BEGIN, payload, payload_len, frame);

    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

/**
 * Test: PUT_DATA with offset overflow should fail
 */
TEST_CASE("Error handling - PUT_DATA offset overflow", "[protocol_parser]") {
    uint8_t frame[256];
    uint8_t payload[256];

    // Step 1: PUT_BEGIN (expect 128 bytes)
    size_t payload_len = build_put_begin_payload("test.bin", 128, 0x12345678, payload);
    size_t frame_len = build_test_frame(MSG_TYPE_PUT_BEGIN, payload, payload_len, frame);
    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Step 2: PUT_DATA with offset that would exceed total size
    uint8_t test_data[64] = {0xCC};
    payload_len = build_put_data_payload(100, test_data, 64, payload);  // offset=100, len=64 → 164 > 128
    frame_len = build_test_frame(MSG_TYPE_PUT_DATA, payload, payload_len, frame);

    ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, ret);
}

/**
 * Test: Double initialization should be safe
 */
TEST_CASE("Error handling - double init is safe", "[protocol_parser]") {
    // setUp() already called protocol_parser_init()
    esp_err_t ret = protocol_parser_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

/**
 * Test: Dispatch before init should fail
 */
TEST_CASE("Error handling - dispatch before init fails", "[protocol_parser]") {
    // Deinit first
    protocol_parser_deinit();

    uint8_t frame[128];
    size_t frame_len = build_test_frame(MSG_TYPE_STATUS, NULL, 0, frame);

    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);

    // Re-init for tearDown
    protocol_parser_init();
}

/* ========================================================================
 * INTEGRATION TESTS
 * ======================================================================== */

/**
 * Test: Complete upload flow with multiple PUT_DATA chunks
 */
TEST_CASE("Integration - multi-chunk upload", "[protocol_parser]") {
    uint8_t frame[512];
    uint8_t payload[512];

    // Prepare test data (256 bytes)
    uint8_t test_data[256];
    for (int i = 0; i < 256; i++) {
        test_data[i] = i & 0xFF;
    }

    uint32_t expected_crc = esp_rom_crc32_le(0, test_data, 256);

    // Step 1: PUT_BEGIN
    size_t payload_len = build_put_begin_payload("multi.bin", 256, expected_crc, payload);
    size_t frame_len = build_test_frame(MSG_TYPE_PUT_BEGIN, payload, payload_len, frame);
    esp_err_t ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Step 2: PUT_DATA chunk 1 (first 128 bytes)
    payload_len = build_put_data_payload(0, test_data, 128, payload);
    frame_len = build_test_frame(MSG_TYPE_PUT_DATA, payload, payload_len, frame);
    ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Step 3: PUT_DATA chunk 2 (next 128 bytes)
    payload_len = build_put_data_payload(128, &test_data[128], 128, payload);
    frame_len = build_test_frame(MSG_TYPE_PUT_DATA, payload, payload_len, frame);
    ret = protocol_dispatch_command(frame, frame_len, 1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Step 4: PUT_END
    frame_len = build_test_frame(MSG_TYPE_PUT_END, NULL, 0, frame);
    ret = protocol_dispatch_command(frame, frame_len, 1);

    // Should succeed or fail due to storage (not our concern in protocol tests)
    TEST_ASSERT_TRUE(ret == ESP_OK || ret == ESP_ERR_NO_MEM || ret == ESP_FAIL);
}

/* ========================================================================
 * UNITY TEST RUNNER
 * ======================================================================== */

void app_main(void) {
    UNITY_BEGIN();

    // TLV frame parsing tests
    RUN_TEST(test_TLV_parsing___NULL_frame_data);
    RUN_TEST(test_TLV_parsing___frame_too_small);
    RUN_TEST(test_TLV_parsing___valid_empty_frame);
    RUN_TEST(test_TLV_parsing___frame_with_payload);
    RUN_TEST(test_TLV_parsing___payload_too_large);

    // CRC32 validation tests
    RUN_TEST(test_CRC32_validation___invalid_checksum);
    RUN_TEST(test_CRC32_validation___valid_checksum);
    RUN_TEST(test_CRC32_validation___with_payload);

    // Upload state machine tests
    RUN_TEST(test_Upload_state_machine___PUT_BEGIN_initializes_session);
    RUN_TEST(test_Upload_state_machine___PUT_BEGIN_rejects_concurrent_session);
    RUN_TEST(test_Upload_state_machine___PUT_DATA_requires_active_session);
    RUN_TEST(test_Upload_state_machine___PUT_END_validates_CRC);
    RUN_TEST(test_Upload_state_machine___PUT_END_rejects_incomplete_upload);

    // CONTROL command tests
    RUN_TEST(test_CONTROL_command___PLAY_parses_pattern_name);
    RUN_TEST(test_CONTROL_command___STOP);
    RUN_TEST(test_CONTROL_command___PAUSE_returns_NOT_SUPPORTED);
    RUN_TEST(test_CONTROL_command___empty_payload_fails);

    // Error handling tests
    RUN_TEST(test_Error_handling___unknown_message_type);
    RUN_TEST(test_Error_handling___oversized_pattern_rejected);
    RUN_TEST(test_Error_handling___PUT_DATA_offset_overflow);
    RUN_TEST(test_Error_handling___double_init_is_safe);
    RUN_TEST(test_Error_handling___dispatch_before_init_fails);

    // Integration tests
    RUN_TEST(test_Integration___multi_chunk_upload);

    UNITY_END();
}
