/**
 * @file prism_parser.h
 * @brief .prism file header types and parser helpers (v1.0/v1.1)
 */

#ifndef PRISM_PARSER_H
#define PRISM_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "pattern_metadata.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Magic constant at start of .prism files */
#define PRISM_MAGIC "PRSM"

/** v1.0 header (64 bytes total) */
typedef struct __attribute__((packed)) {
    uint8_t  magic[4];           /**< "PRSM" */
    uint16_t version;            /**< 0x0100 for v1.0, 0x0101 for v1.1 */
    uint16_t led_count;          /**< 160 (per strip) */
    uint32_t frame_count;        /**< Total frames */
    uint32_t fps;                /**< Frames per second */
    uint8_t  color_format;       /**< Pixel encoding */
    uint8_t  compression;        /**< Compression type */
    uint16_t reserved1;          /**< Reserved */
    uint32_t crc32;              /**< Header CRC (covers up to this field) */
    uint8_t  padding[40];        /**< Pad to 64 bytes */
} prism_header_v10_t;

/** v1.1 header (80 bytes struct; first 70 bytes on disk are mandatory) */
typedef struct __attribute__((packed)) {
    prism_header_v10_t     base; /**< v1.0 base (64 bytes) */
    prism_pattern_meta_v11_t meta; /**< v1.1 metadata (16 bytes; only first 6 used in CRC) */
} prism_header_v11_t;

_Static_assert(sizeof(prism_header_v10_t) == 64, "v1.0 header must be 64 bytes");
_Static_assert(sizeof(prism_header_v11_t) == 80, "v1.1 header struct is 80 bytes");

/**
 * @brief Parse .prism header supporting v1.0 and v1.1 formats.
 *
 * For v1.0 (0x0100): fills base and zeroes meta with safe defaults.
 * For v1.1 (0x0101): copies base and first 6 bytes of meta (remaining are zeroed).
 *
 * @param data Pointer to file data (at least 64 bytes)
 * @param len  Total bytes available starting at data
 * @param out_header Output parsed header
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t parse_prism_header(const uint8_t *data, size_t len, prism_header_v11_t *out_header);

/**
 * @brief Calculate header CRC (v1.0/v1.1 aware).
 *
 * CRC coverage:
 * - v1.0: up to but not including base.crc32
 * - v1.1: v1.0 coverage + first 6 bytes of meta
 */
uint32_t calculate_header_crc(const prism_header_v11_t *header);

#ifdef __cplusplus
}
#endif

#endif // PRISM_PARSER_H

