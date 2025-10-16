// Minimal WASM serializer for .prism v1.1 header + CUSTOM delay map
#include <stdint.h>
#include <string.h>

// Shared types (mirrors firmware pattern_metadata.h)
typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t motion_direction;
    uint8_t sync_mode;
    uint8_t reserved;
    union {
        struct {
            uint16_t delay_ms;
            uint16_t progressive_start_ms;
            uint16_t progressive_end_ms;
            uint16_t wave_amplitude_ms;
            uint16_t wave_frequency_hz;
            uint16_t wave_phase_deg;
        } params;
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

// Export to JavaScript
__attribute__((used))
uint8_t* create_prism_header(uint8_t motion, uint8_t sync_mode,
                             uint16_t *params, uint16_t *delay_map) {
    static uint8_t buffer[16 + 320]; // Header + optional delay map

    prism_pattern_meta_v11_t *header = (prism_pattern_meta_v11_t*)buffer;
    header->version = 0x01;
    header->motion_direction = motion;
    header->sync_mode = sync_mode;
    header->reserved = 0;

    // Copy parameters (12 bytes)
    memcpy(&header->param0, params, 12);

    // If CUSTOM mode, append delay map
    if (sync_mode == 4) {  // CUSTOM = 4
        memcpy(buffer + 16, delay_map, 320);
    }

    return buffer;
}

