// prism_temporal_ctx.h - Temporal context and parameters for sequencing
#ifndef PRISM_TEMPORAL_CTX_H
#define PRISM_TEMPORAL_CTX_H

#include <stdint.h>
#include "prism_motion.h"

#ifdef __cplusplus
extern "C" {
#endif

// Sync parameters structure (12 bytes)
typedef struct {
    uint16_t delay_ms;              // Base delay for OFFSET mode
    uint16_t progressive_start_ms;  // PROGRESSIVE mode start delay
    uint16_t progressive_end_ms;    // PROGRESSIVE mode end delay
    uint16_t wave_amplitude_ms;     // WAVE mode amplitude
    uint16_t wave_frequency_hz;     // WAVE mode frequency
    uint16_t wave_phase_deg;        // WAVE mode phase offset (0-360)
} prism_sync_params_t;

// Temporal context for frame calculation
// NOTE: delay_table pointer is owned by pattern cache (Task 7)
// Do NOT free this pointer - it points into cached pattern data
typedef struct {
    uint32_t frame_index;                   // Current frame number
    const uint16_t *delay_table;            // Pointer to 160-entry delay map (PROGRESSIVE/CUSTOM)
    uint32_t frame_time_ms;                 // Milliseconds since pattern start
    prism_sync_mode_t sync_mode;            // Active sync mode
    prism_motion_t motion_direction;        // Active motion direction
    prism_sync_params_t params;             // Mode-specific parameters
} prism_temporal_ctx_t;

#ifdef __cplusplus
}
#endif

#endif // PRISM_TEMPORAL_CTX_H
