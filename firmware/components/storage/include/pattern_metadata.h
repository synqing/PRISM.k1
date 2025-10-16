/**
 * @file pattern_metadata.h
 * @brief Packed metadata header for .prism v1.1 files
 */

#ifndef PATTERN_METADATA_H
#define PATTERN_METADATA_H

#include <stdint.h>
#include "prism_motion.h"
#include "prism_temporal_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

// Pattern metadata for v1.1 format
// NOTE: This prepends LED payload data in .prism files
typedef struct __attribute__((packed)) {
    uint8_t version;                // 0x01 for v1.1
    uint8_t motion_direction;       // prism_motion_t as uint8_t
    uint8_t sync_mode;              // prism_sync_mode_t as uint8_t
    uint8_t reserved;               // Padding for future use
    union {
        prism_sync_params_t params; // 12 bytes (6x uint16_t)
        struct {
            uint16_t param0;
            uint16_t param1;
            uint16_t param2;
            uint16_t param3;
            uint16_t param4;
            uint16_t param5;
        };
    };
} prism_pattern_meta_v11_t;

// Version constants
#define PRISM_PATTERN_VERSION_1_0  0x00
#define PRISM_PATTERN_VERSION_1_1  0x01

// Size validation
_Static_assert(sizeof(prism_pattern_meta_v11_t) == 16, "Metadata must be 16 bytes");

#ifdef __cplusplus
}
#endif

#endif // PATTERN_METADATA_H

