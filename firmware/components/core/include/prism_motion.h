// prism_motion.h - Motion and Sync enums for temporal sequencing
#ifndef PRISM_MOTION_H
#define PRISM_MOTION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Motion direction enumerations
typedef enum {
    PRISM_MOTION_LEFT = 0,      // LED 0 → LED 159 (left-to-right)
    PRISM_MOTION_RIGHT,         // LED 159 → LED 0 (right-to-left)
    PRISM_MOTION_CENTER,        // LEDs 79-80 → edges (radial bloom)
    PRISM_MOTION_EDGE,          // Edges → center (collapse)
    PRISM_MOTION_STATIC,        // No propagation
    PRISM_MOTION_COUNT
} prism_motion_t;

// Sync mode enumerations
typedef enum {
    PRISM_SYNC_SYNC = 0,        // Both edges simultaneous (50% CPU save)
    PRISM_SYNC_OFFSET,          // CH2 delayed by fixed time
    PRISM_SYNC_PROGRESSIVE,     // Delay varies linearly (triangles/wedges)
    PRISM_SYNC_WAVE,            // Sinusoidal delay (organic motion)
    PRISM_SYNC_CUSTOM,          // Per-LED timing (320 bytes, expert)
    PRISM_SYNC_COUNT
} prism_sync_mode_t;

// Validation macros
#define PRISM_MOTION_IS_VALID(dir) ((dir) >= PRISM_MOTION_LEFT && (dir) < PRISM_MOTION_COUNT)
#define PRISM_SYNC_IS_VALID(mode)  ((mode) >= PRISM_SYNC_SYNC && (mode) < PRISM_SYNC_COUNT)

// Static assertions for protocol compatibility
_Static_assert(PRISM_MOTION_COUNT == 5, "Motion enum must have exactly 5 values");
_Static_assert(PRISM_SYNC_COUNT == 5, "Sync enum must have exactly 5 values");

#ifdef __cplusplus
}
#endif

#endif // PRISM_MOTION_H
