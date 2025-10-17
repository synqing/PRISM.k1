/**
 * @file prism_parser.c
 * @brief .prism header parser implementation (v1.0/v1.1)
 */

#include "prism_parser.h"
#include "esp_log.h"
#include "esp_rom_crc.h"
#include <string.h>

static const char *TAG = "prism_parser";

esp_err_t parse_prism_header(const uint8_t *data, size_t len, prism_header_v11_t *out_header)
{
    if (!data || !out_header) {
        return ESP_ERR_INVALID_ARG;
    }

    if (len < sizeof(prism_header_v10_t)) {
        ESP_LOGE(TAG, ".prism header too small: %zu < %zu", len, sizeof(prism_header_v10_t));
        return ESP_ERR_INVALID_SIZE;
    }

    // Copy v1.0 header portion
    const prism_header_v10_t *v10 = (const prism_header_v10_t *)data;

    // Validate magic
    if (memcmp(v10->magic, PRISM_MAGIC, 4) != 0) {
        ESP_LOGE(TAG, "Invalid .prism magic");
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(&out_header->base, v10, sizeof(prism_header_v10_t));

    // Initialize meta with zeros (safe defaults)
    memset(&out_header->meta, 0, sizeof(out_header->meta));

    // Default meta for v1.0
    if (out_header->base.version == 0x0100) {
        out_header->meta.version = 0x00;
        out_header->meta.sync_mode = PRISM_SYNC_SYNC;            // default
        out_header->meta.motion_direction = PRISM_MOTION_STATIC; // default
        return ESP_OK;
    }

    // v1.1 header: require at least 70 bytes (64 base + first 6 meta bytes)
    if (out_header->base.version == 0x0101) {
        size_t min_v11 = sizeof(prism_header_v10_t) + 6;
        if (len < min_v11) {
            ESP_LOGE(TAG, ".prism v1.1 header too small: %zu < %zu", len, min_v11);
            return ESP_ERR_INVALID_SIZE;
        }

        // Copy only the first 6 bytes of meta from the file, leave rest zero
        memcpy(&out_header->meta, data + sizeof(prism_header_v10_t), 6);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Unsupported .prism version: 0x%04X", out_header->base.version);
    return ESP_ERR_NOT_SUPPORTED;
}

uint32_t calculate_header_crc(const prism_header_v11_t *header)
{
    if (!header) {
        return 0;
    }

    const uint8_t *bytes = (const uint8_t *)header;

    // CRC for v1.0 covers up to but not including base.crc32
    size_t crc_len = offsetof(prism_header_v10_t, crc32);

    // For v1.1, include first 6 bytes of meta
    if (header->base.version == 0x0101) {
        crc_len += 6; // version + sync + motion + reserved0 + param0(2)
    }

    return esp_rom_crc32_le(0, bytes, crc_len);
}
