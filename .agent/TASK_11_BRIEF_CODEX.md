# Task 11: Motion & Sync Enumerations - Codex Agent Brief

## Mission
Implement Motion and Sync mode enumerations for PRISM.k1 temporal sequencing system.

## Context
- Project: ESP32-S3 LED controller firmware
- Component: `firmware/components/playback/`
- Research: Complete (see task details below)
- Subtasks: 3 (all LOW complexity)

## Task Details
Run: `task-master show 11`

## Implementation Steps

### Subtask 11.1: Create prism_motion.h header
**File**: `firmware/components/playback/include/prism_motion.h`

```c
#ifndef PRISM_MOTION_H
#define PRISM_MOTION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Motion direction enumerations
typedef enum {
    PRISM_MOTION_LEFT = 0,      // LED 0 → LED 159
    PRISM_MOTION_RIGHT,         // LED 159 → LED 0
    PRISM_MOTION_CENTER,        // LEDs 79-80 → edges
    PRISM_MOTION_EDGE,          // Edges → center
    PRISM_MOTION_STATIC,        // No propagation
    PRISM_MOTION_COUNT
} prism_motion_t;

// Sync mode enumerations
typedef enum {
    PRISM_SYNC_SYNC = 0,        // Both edges simultaneous
    PRISM_SYNC_OFFSET,          // CH2 delayed by fixed time
    PRISM_SYNC_PROGRESSIVE,     // Delay varies linearly
    PRISM_SYNC_WAVE,            // Sinusoidal delay
    PRISM_SYNC_CUSTOM,          // Per-LED timing
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
```

**Checklist:**
- [ ] Create header file with enums
- [ ] Add validation macros
- [ ] Add static assertions
- [ ] Add extern "C" guards
- [ ] Update `firmware/components/playback/CMakeLists.txt` to include new header

### Subtask 11.2: Implement temporal context structs
**File**: `firmware/components/playback/include/prism_temporal.h`

```c
#ifndef PRISM_TEMPORAL_H
#define PRISM_TEMPORAL_H

#include <stdint.h>
#include "prism_motion.h"

#ifdef __cplusplus
extern "C" {
#endif

// Sync parameters structure
typedef struct {
    uint16_t delay_ms;              // Base delay for OFFSET mode
    uint16_t progressive_start_ms;   // PROGRESSIVE mode start delay
    uint16_t progressive_end_ms;     // PROGRESSIVE mode end delay
    uint16_t wave_amplitude_ms;      // WAVE mode amplitude
    uint16_t wave_frequency_hz;      // WAVE mode frequency
    uint16_t wave_phase_deg;         // WAVE mode phase offset
} prism_sync_params_t;

// Temporal context for frame calculation
// NOTE: delay_table pointer owned by pattern cache (Task 7)
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

#endif // PRISM_TEMPORAL_H
```

**Checklist:**
- [ ] Create temporal header file
- [ ] Define sync_params_t struct (6x uint16_t fields)
- [ ] Define prism_temporal_ctx_t struct
- [ ] Document delay_table ownership
- [ ] Add to CMakeLists.txt

### Subtask 11.3: Add pattern metadata header
**File**: `firmware/components/storage/include/pattern_metadata.h`

```c
#ifndef PATTERN_METADATA_H
#define PATTERN_METADATA_H

#include <stdint.h>
#include "prism_motion.h"

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
    prism_sync_params_t params;     // 12 bytes (6x uint16_t)
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
```

**Checklist:**
- [ ] Create metadata header
- [ ] Define packed struct (16 bytes total)
- [ ] Add version constants
- [ ] Add size static assertion
- [ ] Include prism_motion.h
- [ ] Update storage CMakeLists.txt

## Completion Criteria
- [ ] All 3 header files created
- [ ] All static assertions compile
- [ ] No compiler warnings
- [ ] CMakeLists.txt updated for all 3 headers
- [ ] Run: `task-master set-status --id=11.1 --status=done` (for each subtask)
- [ ] Run: `task-master set-status --id=11 --status=done` (when all complete)

## Build Test
```bash
cd firmware
idf.py build
# Should compile cleanly with new headers
```

## Notes
- Keep enums byte-sized for network transmission
- Use uint16_t for timing values (matches ADR-010 spec)
- Static assertions catch protocol drift at compile time
- Reserved fields enable future expansion

## Next Tasks
After Task 11, these become unblocked:
- Task 12: Use these enums in temporal sequencing
- Task 13: Extend .prism parser with metadata
- Task 14: Wire temporal context into playback loop
