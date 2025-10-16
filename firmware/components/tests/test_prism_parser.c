/**
 * @file test_prism_parser.c
 * @brief Unit tests for .prism v1.0/v1.1 header parser
 */

#include "unity.h"
#include <string.h>
#include "prism_parser.h"

TEST_CASE("Parser handles v1.0 files", "[storage]")
{
    uint8_t v10_data[64];
    memset(v10_data, 0, sizeof(v10_data));

    prism_header_v10_t *v10 = (prism_header_v10_t*)v10_data;
    memcpy(v10->magic, PRISM_MAGIC, 4);
    v10->version = 0x0100;

    prism_header_v11_t parsed;
    esp_err_t err = parse_prism_header(v10_data, sizeof(v10_data), &parsed);

    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL_UINT16(0x0100, parsed.base.version);
    TEST_ASSERT_EQUAL_UINT8(PRISM_SYNC_SYNC, parsed.meta.sync_mode);
    TEST_ASSERT_EQUAL_UINT8(PRISM_MOTION_STATIC, parsed.meta.motion_direction);
}

TEST_CASE("Parser handles v1.1 files (>=70 bytes)", "[storage]")
{
    prism_header_v11_t v11;
    memset(&v11, 0, sizeof(v11));
    memcpy(v11.base.magic, PRISM_MAGIC, 4);
    v11.base.version = 0x0101;
    v11.meta.version = 0x01;
    v11.meta.sync_mode = PRISM_SYNC_PROGRESSIVE;
    v11.meta.motion_direction = PRISM_MOTION_LEFT;
    v11.meta.param0 = 150; // example parameter

    prism_header_v11_t parsed;

    // Case A: len = 70 (base + first 6 meta bytes)
    uint8_t buf70[70];
    memcpy(buf70, &v11, 70);
    esp_err_t errA = parse_prism_header(buf70, sizeof(buf70), &parsed);
    TEST_ASSERT_EQUAL(ESP_OK, errA);
    TEST_ASSERT_EQUAL_UINT16(0x0101, parsed.base.version);
    TEST_ASSERT_EQUAL_UINT8(PRISM_SYNC_PROGRESSIVE, parsed.meta.sync_mode);
    TEST_ASSERT_EQUAL_UINT8(PRISM_MOTION_LEFT, parsed.meta.motion_direction);

    // Case B: len = full struct size (80)
    esp_err_t errB = parse_prism_header((const uint8_t*)&v11, sizeof(v11), &parsed);
    TEST_ASSERT_EQUAL(ESP_OK, errB);
    TEST_ASSERT_EQUAL_UINT8(PRISM_SYNC_PROGRESSIVE, parsed.meta.sync_mode);
    TEST_ASSERT_EQUAL_UINT8(PRISM_MOTION_LEFT, parsed.meta.motion_direction);
}

