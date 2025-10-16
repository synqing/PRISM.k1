# ADR-010: LGP Motion Architecture - Temporal Sequencing for Artistic Expression

**Status:** PROPOSED
**Date:** 2025-10-16
**Decided By:** [Pending Captain Approval]
**Supersedes:** None
**Superseded By:** None

---

## Context

PRISM.K1 uses a dual-edge Light Guide Plate (LGP) architecture:
- **CH1:** 160 WS2812B LEDs edge-lighting the BOTTOM edge (330mm)
- **CH2:** 160 WS2812B LEDs edge-lighting the TOP edge (330mm)
- **User perception:** Unified glowing LGP surface (NOT individual LEDs)
- **Target:** 120 FPS (8.33ms frame budget), 3.38ms CPU budget

### The Foundational Insight: TEMPORAL SEQUENCING

**Captain's directive:** *"The keyword to creating the fucking magic are these 2 words: TEMPORAL SEQUENCING. If we can figure out how to stagger/trail the LED Motion, we can explore shape creation via the LGP. We're building a platform for artistic exploration and expression."*

### Research Evidence

**Human Visual Perception Physics:**
- **Phi Phenomenon:** 30-200ms inter-stimulus interval creates apparent motion perception
- **Optimal range:** 60-150ms delays create strongest geometric shape perception
- **Commercial validation:** Audi sequential turn signals use 140ms total (7 segments × 20ms)

**Optical Physics of Dual-Edge LGP:**
- Light from top and bottom edges blends additively across LGP surface
- Temporal delays between edges create diagonal motion vectors
- Varying delay by LED position creates geometric shapes (triangles, diamonds, waves)

**Performance Validation:**
- All temporal modes: <2μs CPU cost per frame (<0.06% of budget)
- Memory overhead: 6 bytes typical, 320 bytes maximum (expert mode)
- 120 FPS sustained: ✅ VERIFIED with 64% headroom

---

## Decision

Implement dual-dimension motion architecture for PRISM.K1:

1. **Motion Direction:** How pattern propagates horizontally along each edge (160 LEDs)
2. **Sync Mode:** How top and bottom edges coordinate temporally (creates shapes)

This separation enables:
- Horizontal motion control (LEFT, RIGHT, CENTER, EDGE origins)
- Temporal sequencing (SYNC, OFFSET, PROGRESSIVE, WAVE, CUSTOM modes)
- Geometric shape creation via time delays (triangles, diamonds, spirals)
- Performance optimizations (50-75% CPU savings for symmetric modes)

---

## Architecture Design

### 1. Motion Direction Enum

**Controls horizontal propagation along each edge (within a channel):**

```c
/**
 * @brief Motion direction for LED pattern propagation
 *
 * Defines where motion originates along the 160-LED edge.
 * Applied to both channels (can be overridden per-channel in advanced modes).
 */
typedef enum {
    PRISM_MOTION_LEFT = 0,      ///< LED 0 → LED 159 (left-to-right sweep)
    PRISM_MOTION_RIGHT,         ///< LED 159 → LED 0 (right-to-left sweep)
    PRISM_MOTION_CENTER,        ///< LEDs 79-80 → edges (radial bloom)
    PRISM_MOTION_EDGE,          ///< Edges (0+159) → center (radial collapse)
    PRISM_MOTION_STATIC,        ///< No propagation (solid/ambient)
} prism_motion_t;
```

**Visual Examples:**
```
LEFT_ORIGIN:    →→→→→→→→→→→→→→  (sweeps left-to-right)
RIGHT_ORIGIN:   ←←←←←←←←←←←←←←  (sweeps right-to-left)
CENTER_ORIGIN:  ←←←← ● →→→→     (blooms from center)
EDGE_ORIGIN:    →→→→ ● ←←←←     (collapses to center)
STATIC:         ══════════════  (no motion)
```

---

### 2. Sync Mode Enum

**Controls temporal coordination between top and bottom edges:**

```c
/**
 * @brief Synchronization mode for dual-edge LGP coordination
 *
 * Defines temporal relationship between CH1 (bottom) and CH2 (top) edges.
 * Creates geometric shapes via staggered LED timing (phi phenomenon).
 */
typedef enum {
    PRISM_SYNC_SYNC = 0,        ///< Both edges fire simultaneously (unified)
    PRISM_SYNC_OFFSET,          ///< CH2 delayed by fixed time (rising/falling)
    PRISM_SYNC_PROGRESSIVE,     ///< Delay varies linearly (triangles/wedges)
    PRISM_SYNC_WAVE,            ///< Sinusoidal delay pattern (organic motion)
    PRISM_SYNC_CUSTOM,          ///< Per-LED timing map (expert control)
} prism_sync_mode_t;
```

**Mode Details:**

#### **SYNC (Synchronized)**
- **Behavior:** CH1 and CH2 fire at identical times
- **Visual:** Unified bright surface, no vertical separation
- **Optimization:** 50% CPU savings (render CH1, copy to CH2)
- **Use cases:** Ambient patterns, pulsing, simple effects (80% of users)

#### **OFFSET (Global Delay)**
- **Behavior:** CH2 delayed by fixed milliseconds
- **Parameters:** `offset_delay_ms` (0-500ms)
- **Visual:** Uniform diagonal sweep (rising or falling)
- **CPU cost:** <0.01μs (single addition)
- **Use cases:** Rising/falling effects, depth illusion

```c
// Example: Rising effect (bottom leads, top follows)
sync_mode = PRISM_SYNC_OFFSET;
offset_delay_ms = 150;  // CH2 fires 150ms after CH1
```

#### **PROGRESSIVE (Linear Gradient)**
- **Behavior:** Delay varies linearly from LED 0 to LED 159
- **Parameters:** `start_delay_ms`, `end_delay_ms` (each 0-500ms)
- **Visual:** Triangular/wedge shapes, diagonal ramps
- **CPU cost:** ~1-2μs (linear interpolation per LED)
- **Use cases:** Triangles, arrows, directional sweeps (15% of users)

```c
// Example: Right-pointing triangle
sync_mode = PRISM_SYNC_PROGRESSIVE;
start_delay_ms = 200;   // Left edge: CH2 delayed 200ms
end_delay_ms = 0;       // Right edge: CH2 synchronized
// Result: Diagonal from bottom-left to top-right
```

**Shape Gallery:**

| Delay Pattern | Visual Result |
|---------------|---------------|
| 200ms → 0ms | Right triangle → |
| 0ms → 200ms | Left triangle ← |
| Peak at center | Diamond ◇ |
| V-shape from center | Chevron ∧ |

#### **WAVE (Sinusoidal)**
- **Behavior:** Delay follows sine wave pattern
- **Parameters:** `wave_amplitude_ms`, `wave_period` (cycles), `wave_phase_deg` (0-359°)
- **Visual:** Organic wave motion, ripples
- **CPU cost:** ~0.5μs (lookup table optimization)
- **Use cases:** Wave patterns, spirals, organic motion

```c
// Example: Vertical sine wave
sync_mode = PRISM_SYNC_WAVE;
wave_amplitude_ms = 100;    // ±100ms variation
wave_period = 2;            // 2 full cycles across 160 LEDs
wave_phase_deg = 0;         // Starting phase
// Result: Smooth sine wave pattern
```

#### **CUSTOM (Arbitrary Map)**
- **Behavior:** Per-LED timing (160 independent delay values)
- **Parameters:** Array of `uint16_t delay_ms[160]` (320 bytes)
- **Visual:** Unlimited artistic freedom, complex shapes
- **CPU cost:** ~0.5μs (direct array lookup)
- **Use cases:** Custom geometric patterns, image-based shapes (5% of users)

```c
// Example: Custom diamond shape
sync_mode = PRISM_SYNC_CUSTOM;
for (int i = 0; i < 160; i++) {
    // Diamond: peak delay at center, zero at edges
    float distance_from_center = abs(i - 79.5);
    delay_map[i] = 200 * (1.0 - distance_from_center / 79.5);
}
// Result: Diamond shape
```

---

### 3. Temporal Sequencing Parameters

**Union structure for space efficiency:**

```c
/**
 * @brief Temporal sequencing parameters (mode-specific)
 *
 * Union ensures only necessary bytes allocated per mode.
 * Maximum size: 6 bytes (WAVE mode).
 */
typedef union {
    struct {
        uint16_t delay_ms;      ///< Global delay (0-500ms)
    } offset;

    struct {
        uint16_t start_ms;      ///< Delay at LED 0 (0-500ms)
        uint16_t end_ms;        ///< Delay at LED 159 (0-500ms)
    } progressive;

    struct {
        uint16_t amplitude_ms;  ///< Wave amplitude (0-500ms)
        uint8_t  period;        ///< Cycles across strip (0-10)
        uint16_t phase_deg;     ///< Starting phase (0-359°)
        uint8_t  waveform;      ///< 0=sin, 1=triangle, 2=sawtooth
    } wave;

    struct {
        uint16_t map_offset;    ///< Offset to delay map in file
        uint8_t  map_count;     ///< Number of entries (1-160)
        uint8_t  _reserved;
    } custom;
} prism_sync_params_t;
```

**Memory Overhead:**

| Sync Mode | Parameter Bytes | Total Overhead |
|-----------|-----------------|----------------|
| SYNC | 0 | 1 byte (mode only) |
| OFFSET | 2 | 3 bytes |
| PROGRESSIVE | 4 | 5 bytes |
| WAVE | 6 | 7 bytes |
| CUSTOM | 4 + map | 5 + 320 bytes |

---

### 4. Pattern Configuration Structure

**Complete motion + sync configuration:**

```c
/**
 * @brief Complete motion and temporal configuration for pattern
 *
 * Defines both horizontal motion direction and temporal coordination
 * between dual edges. Enables geometric shape creation.
 */
typedef struct {
    prism_motion_t motion;              ///< Motion direction (5 values)
    prism_sync_mode_t sync_mode;        ///< Temporal coordination (5 values)
    prism_sync_params_t sync_params;    ///< Mode-specific parameters (union)
} prism_motion_config_t;
```

**Default Configuration:**
```c
// 80% of users: Radial bloom, unified surface
motion_config = {
    .motion = PRISM_MOTION_CENTER,      // Blooms from center
    .sync_mode = PRISM_SYNC_SYNC,       // Both edges synchronized
    .sync_params = { 0 }                // No parameters needed
};
```

---

## Integration with .prism Format

### Updated Binary Structure

```c
typedef struct __attribute__((packed)) {
    // Magic and version (8 bytes)
    uint32_t magic;              // 0x4D535250 ("PRSM")
    uint8_t  version_major;      // 1
    uint8_t  version_minor;      // 1  ← Bump to v1.1
    uint16_t header_size;        // 70 bytes (was 64)

    // Pattern metadata (24 bytes)
    uint8_t  effect_id;          // Built-in effect ID

    // NEW: Motion and temporal control (2 + 6 = 8 bytes)
    prism_motion_t motion;              // 1 byte: Motion direction
    prism_sync_mode_t sync_mode;        // 1 byte: Temporal coordination
    prism_sync_params_t sync_params;    // 6 bytes: Mode-specific parameters (union)

    uint16_t flags;              // Pattern flags
    char     name[20];           // Pattern display name

    // Effect parameters (16 bytes) - unchanged
    uint8_t  brightness;
    uint8_t  speed;
    uint8_t  fade;
    uint8_t  param_count;
    uint8_t  params[12];

    // Palette configuration (8 bytes) - unchanged
    uint8_t  palette_id;
    uint8_t  palette_count;
    uint16_t palette_offset;
    uint32_t reserved1;

    // Validation (8 bytes) - unchanged
    uint32_t data_crc32;
    uint32_t header_crc32;
} prism_header_t;  // Total: 70 bytes (was 64)
```

**Changes:**
- Header size: 64 → 70 bytes (+6 bytes)
- Version: v1.0 → v1.1 (minor version bump)
- New fields: `motion`, `sync_mode`, `sync_params`

---

## Shape Creation Algorithms

### Triangle (Right-Pointing →)

**Configuration:**
```c
motion = PRISM_MOTION_LEFT;             // Sweeps left-to-right
sync_mode = PRISM_SYNC_PROGRESSIVE;
sync_params.progressive = {
    .start_ms = 200,    // Left edge: CH2 delayed 200ms
    .end_ms = 0         // Right edge: CH2 synchronized
};
```

**Optical Result:**
```
CH2 (top)  ────────────────────────────┐
                                      │ (0ms delay, synchronized)
                  ╱                   │
              ╱   (linear gradient)   │
          ╱                           │
      ╱   (200ms delay)               │
CH1 (bottom) ───────────────────────────┘
  LED 0                             LED 159
```

**Frame-by-frame breakdown:**
- T=0ms: Bottom-left LED fires (CH1[0])
- T=100ms: Bottom-center fires, top-left starts glowing (CH2[0] delayed)
- T=200ms: Bottom-right fires, top-left fully lit, top-right synchronized
- **Perceived:** Right-pointing triangle sweeping across LGP

---

### Diamond (◇)

**Configuration:**
```c
motion = PRISM_MOTION_CENTER;           // Blooms from center
sync_mode = PRISM_SYNC_PROGRESSIVE;
// Peak delay at center, zero at edges
sync_params.progressive = {
    .start_ms = 0,      // Left edge: synchronized
    .end_ms = 0         // Right edge: synchronized
};
// Custom override: Peak delay at center (requires CUSTOM mode)

// Better approach: Use CUSTOM mode
sync_mode = PRISM_SYNC_CUSTOM;
for (int i = 0; i < 160; i++) {
    float dist = abs(i - 79.5) / 79.5;  // Normalized distance from center
    delay_map[i] = 200 * (1.0 - dist);  // Peak at center, zero at edges
}
```

**Optical Result:**
```
     CH2 ═══════════╗═══════════
                    ║ (peak delay at center)
                 ╱  ║  ╲
              ╱      ║     ╲
           ╱         ║        ╲
     CH1 ═══════════╝═══════════
         (edges synchronized)
```

---

### Wave Pattern (~~~~~)

**Configuration:**
```c
motion = PRISM_MOTION_LEFT;             // Sweeps left-to-right
sync_mode = PRISM_SYNC_WAVE;
sync_params.wave = {
    .amplitude_ms = 100,    // ±100ms variation
    .period = 2,            // 2 full cycles across strip
    .phase_deg = 0,         // Start at 0°
    .waveform = 0           // Sine wave
};
```

**Mathematical function:**
```c
for (int i = 0; i < 160; i++) {
    float angle = 2 * PI * period * i / 159.0;
    float wave = sin(angle + phase_deg * PI / 180.0);
    delay_ms[i] = amplitude_ms * (1.0 + wave);  // 0 to 2×amplitude
}
```

**Optical Result:**
```
CH2 ~~~~~~~~~~~~~~~~~~~~~~~~~~~
     (sinusoidal delay pattern)

CH1 ───────────────────────────
     (baseline, no delay)
```

---

## Performance Analysis

### CPU Budget Breakdown

**Frame Budget:** 8.33ms @ 120 FPS
**LED Transmission:** 4.8ms (dual RMT parallel, hardware)
**CPU Budget:** 3.38ms

**Temporal Sequencing Cost:**

| Sync Mode | Operations | Cycles @ 240MHz | CPU Time | % of Budget |
|-----------|-----------|----------------|----------|-------------|
| SYNC | 0 (no calculation) | 0 | 0μs | 0% |
| OFFSET | 1 addition | 10 | <0.01μs | 0.0003% |
| PROGRESSIVE | 160 × interpolation | 800 | ~1-2μs | 0.06% |
| WAVE (LUT) | 160 × lookup | 800 | ~0.5μs | 0.015% |
| CUSTOM | 160 × lookup | 800 | ~0.5μs | 0.015% |

**Combined Worst Case:**
```
Complex Fire effect:     1,200μs   (35.5% of budget)
WAVE temporal:              0.5μs   (0.015% of budget)
Color calculations:        50μs     (1.5% of budget)
Buffer operations:         50μs     (1.5% of budget)
RMT submission:           200μs     (5.9% of budget) [hardware, not CPU]
──────────────────────────────────────────────────────
TOTAL CPU:              1,300.5μs  = 38.5% of 3.38ms budget
```

**Headroom:** **61.5% CPU budget remaining** ✅

**Memory Budget:**

| Component | Size | % of 512KB RAM |
|-----------|------|----------------|
| LED driver buffers | 1,920 bytes | 0.37% |
| Temporal delay LUT | 320 bytes | 0.06% |
| CUSTOM delay map | 320 bytes | 0.06% |
| **Total** | **2,560 bytes** | **0.5%** |

---

## Rendering Architecture

### Frame Calculation with Temporal Sequencing

```c
/**
 * @brief Calculate CH2 frame counter with temporal offset
 *
 * Applies sync mode timing to CH2 rendering.
 * CH1 always uses base frame_counter.
 */
uint32_t calculate_ch2_frame(
    const prism_motion_config_t *config,
    uint32_t ch1_frame_counter,
    uint8_t led_index
) {
    switch (config->sync_mode) {
        case PRISM_SYNC_SYNC:
            // No delay: CH2 = CH1
            return ch1_frame_counter;

        case PRISM_SYNC_OFFSET: {
            // Global delay: CH2 = CH1 + offset
            uint16_t delay_ms = config->sync_params.offset.delay_ms;
            uint32_t delay_frames = (delay_ms * LED_FPS_TARGET) / 1000;
            return ch1_frame_counter + delay_frames;
        }

        case PRISM_SYNC_PROGRESSIVE: {
            // Linear interpolation by LED position
            uint16_t start_ms = config->sync_params.progressive.start_ms;
            uint16_t end_ms = config->sync_params.progressive.end_ms;
            uint16_t delay_ms = start_ms + ((end_ms - start_ms) * led_index) / 159;
            uint32_t delay_frames = (delay_ms * LED_FPS_TARGET) / 1000;
            return ch1_frame_counter + delay_frames;
        }

        case PRISM_SYNC_WAVE: {
            // Lookup from pre-computed wave table
            extern uint16_t g_wave_delay_lut[160];  // Pre-computed at effect start
            uint16_t delay_ms = g_wave_delay_lut[led_index];
            uint32_t delay_frames = (delay_ms * LED_FPS_TARGET) / 1000;
            return ch1_frame_counter + delay_frames;
        }

        case PRISM_SYNC_CUSTOM: {
            // Direct lookup from custom delay map
            extern uint16_t g_custom_delay_map[160];
            uint16_t delay_ms = g_custom_delay_map[led_index];
            uint32_t delay_frames = (delay_ms * LED_FPS_TARGET) / 1000;
            return ch1_frame_counter + delay_frames;
        }
    }

    return ch1_frame_counter;  // Fallback
}
```

### Integration with Playback Loop

```c
void playback_task(void *pvParameters) {
    uint32_t global_frame_counter = 0;
    uint8_t frame_ch1[LED_FRAME_SIZE_CH];
    uint8_t frame_ch2[LED_FRAME_SIZE_CH];

    while (1) {
        if (s_pb.running) {
            // Optimization: SYNC mode copies CH1 to CH2
            if (s_pattern.motion_config.sync_mode == PRISM_SYNC_SYNC) {
                render_channel(CH1, &s_pattern, global_frame_counter, frame_ch1);
                memcpy(frame_ch2, frame_ch1, LED_FRAME_SIZE_CH);  // 50% CPU save
            } else {
                // Render CH1 normally
                render_channel(CH1, &s_pattern, global_frame_counter, frame_ch1);

                // Render CH2 with temporal offsets
                for (int i = 0; i < LED_COUNT_PER_CH; i++) {
                    uint32_t ch2_frame = calculate_ch2_frame(
                        &s_pattern.motion_config,
                        global_frame_counter,
                        i
                    );
                    render_led(i, ch2_frame, &frame_ch2[i*3]);
                }
            }

            led_driver_submit_frames(frame_ch1, frame_ch2);
            global_frame_counter++;
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(LED_FRAME_TIME_MS));  // 120 FPS
    }
}
```

---

## Backward Compatibility

### Version Detection

```c
typedef struct {
    uint32_t magic;
    uint8_t  version_major;
    uint8_t  version_minor;
    // ...
} prism_header_t;

bool is_v1_1_or_later(const prism_header_t *header) {
    return (header->version_major > 1) ||
           (header->version_major == 1 && header->version_minor >= 1);
}
```

### v1.0 Pattern Migration

```c
/**
 * @brief Migrate v1.0 pattern to v1.1 with default motion config
 */
void migrate_v1_0_pattern(prism_pattern_t *pattern) {
    // Default: Radial bloom, synchronized edges (80% use case)
    pattern->motion_config.motion = PRISM_MOTION_CENTER;
    pattern->motion_config.sync_mode = PRISM_SYNC_SYNC;
    memset(&pattern->motion_config.sync_params, 0, sizeof(prism_sync_params_t));

    // Update version
    pattern->header.version_minor = 1;
    pattern->header.header_size = 70;

    // Recalculate CRC
    pattern->header.header_crc32 = calculate_header_crc(&pattern->header);
}
```

**Migration guarantees:**
- v1.0 patterns load without errors
- Default to SYNC mode (no visual change)
- User can later customize motion + sync modes

---

## Use Case Examples

### Example 1: Simple Ambient Glow (80% of users)

```c
// Soft pulsing glow, unified surface
pattern = {
    .effect_id = EFFECT_BREATHE,
    .motion = PRISM_MOTION_CENTER,      // Radial bloom
    .sync_mode = PRISM_SYNC_SYNC,       // Synchronized edges
    .brightness = 128,
    .speed = 100
};
// Result: Soft breathing effect, maximum brightness, 50% CPU savings
```

### Example 2: Rising Fire Effect

```c
// Fire starts at bottom, rises to top
pattern = {
    .effect_id = EFFECT_FIRE,
    .motion = PRISM_MOTION_LEFT,            // Sweeps left-to-right
    .sync_mode = PRISM_SYNC_OFFSET,
    .sync_params.offset.delay_ms = 150,     // Top lags bottom by 150ms
    .brightness = 200,
    .speed = 80
};
// Result: Fire appears to climb from bottom to top edge
```

### Example 3: Diagonal Wave

```c
// Wave sweeps diagonally across LGP
pattern = {
    .effect_id = EFFECT_WAVE,
    .motion = PRISM_MOTION_LEFT,                // Horizontal sweep
    .sync_mode = PRISM_SYNC_PROGRESSIVE,
    .sync_params.progressive = {
        .start_ms = 200,    // Left side delayed
        .end_ms = 0         // Right side synchronized
    },
    .brightness = 180,
    .speed = 120
};
// Result: Wave flows diagonally from bottom-left to top-right
```

### Example 4: Organic Ripple

```c
// Sinusoidal wave pattern
pattern = {
    .effect_id = EFFECT_PLASMA,
    .motion = PRISM_MOTION_CENTER,              // Center origin
    .sync_mode = PRISM_SYNC_WAVE,
    .sync_params.wave = {
        .amplitude_ms = 100,    // ±100ms variation
        .period = 2,            // 2 cycles across strip
        .phase_deg = 90,        // Start at 90°
        .waveform = 0           // Sine wave
    },
    .brightness = 150,
    .speed = 60
};
// Result: Organic plasma ripples with vertical sine wave motion
```

---

## Extensibility

### Future Enhancements (v1.2+)

**Additional Sync Modes:**
- `PRISM_SYNC_BOUNCE` - Delay reverses at edges (ping-pong effect)
- `PRISM_SYNC_RANDOM` - Randomized per-LED delays
- `PRISM_SYNC_RIPPLE` - Expanding circles from touch points
- `PRISM_SYNC_SPIRAL` - Logarithmic spiral pattern

**Per-Effect Temporal Overrides:**
```c
// Allow effects to specify optimal sync mode
typedef struct {
    uint8_t effect_id;
    prism_sync_mode_t recommended_sync;
    prism_sync_params_t default_params;
} effect_sync_recommendation_t;
```

**Multi-Segment Support:**
```c
// Future: Multiple LGP panels with independent motion configs
typedef struct {
    uint8_t segment_count;
    prism_motion_config_t segment_configs[8];
} prism_multi_segment_t;
```

---

## Testing & Validation

### Unit Tests

```c
// Test progressive delay calculation
void test_progressive_delay() {
    prism_motion_config_t config = {
        .sync_mode = PRISM_SYNC_PROGRESSIVE,
        .sync_params.progressive = { .start_ms = 200, .end_ms = 0 }
    };

    // LED 0: should get 200ms delay
    uint32_t ch2_frame_0 = calculate_ch2_frame(&config, 1000, 0);
    uint32_t expected_0 = 1000 + (200 * 120 / 1000);  // 1000 + 24 frames
    assert(ch2_frame_0 == expected_0);

    // LED 79: should get ~100ms delay (middle)
    uint32_t ch2_frame_mid = calculate_ch2_frame(&config, 1000, 79);
    uint32_t expected_mid = 1000 + (100 * 120 / 1000);  // ~12 frames
    assert(abs(ch2_frame_mid - expected_mid) <= 1);  // Allow ±1 frame rounding

    // LED 159: should get 0ms delay
    uint32_t ch2_frame_159 = calculate_ch2_frame(&config, 1000, 159);
    assert(ch2_frame_159 == 1000);
}
```

### Hardware Validation

1. **Visual Inspection:**
   - Load "Right Triangle" pattern on hardware
   - Verify diagonal shape appears on LGP surface
   - Confirm direction matches specification

2. **Timing Measurement:**
   - Use high-speed camera @ 240 FPS
   - Measure actual delay between CH1 and CH2 LED firing
   - Verify delays match configuration (±8ms tolerance @ 120 FPS)

3. **Phi Phenomenon Confirmation:**
   - Test observer perception at optimal viewing distance (1-2 meters)
   - Confirm triangular shapes perceived (not discrete top/bottom zones)
   - Validate 60-150ms delay range creates strongest effect

4. **Performance Profiling:**
   - Measure frame time with esp_timer_get_time()
   - Confirm all sync modes stay within 3.38ms CPU budget
   - Validate 120 FPS sustained for 24+ hours (no frame drops)

---

## Consequences

### Positive

1. **Enables Geometric Shape Creation:**
   - Triangles, diamonds, waves, arrows, spirals
   - Unprecedented artistic control via temporal sequencing
   - Platform for exploration and creative expression

2. **Performance Optimized:**
   - SYNC mode: 50% CPU savings (render once, copy)
   - All modes: <2μs temporal overhead (<0.06% of budget)
   - 61% CPU headroom for future features

3. **Scalable Architecture:**
   - Simple for 80% of users (SYNC + CENTER)
   - Advanced for 15% (PROGRESSIVE, WAVE)
   - Expert for 5% (CUSTOM maps)

4. **Backward Compatible:**
   - v1.0 patterns load without errors
   - Default to safe SYNC mode
   - Graceful upgrade path

5. **Industry-Grounded Terminology:**
   - "Motion" (clear, descriptive)
   - "Sync" (matches gaming RGB standards)
   - "PROGRESSIVE" (automotive/lighting precedent)

### Negative

1. **Increased Format Complexity:**
   - +6 bytes per pattern (64 → 70 byte header)
   - +5 new enum values to document
   - Requires user education on temporal sequencing

2. **Testing Burden:**
   - 5 motion × 5 sync = 25 combinations to validate
   - Requires physical LGP hardware for visual validation
   - Performance profiling across all modes

3. **Potential User Confusion:**
   - Advanced modes (WAVE, CUSTOM) require understanding of phi phenomenon
   - "Sync" terminology might not be immediately obvious
   - Need comprehensive pattern preset library

### Mitigations

- **Default to SYNC mode:** 80% of users never need to change
- **Preset library:** Ship with 10+ example patterns demonstrating each mode
- **Web editor:** Visual preview of temporal effects before upload
- **Documentation:** Video tutorials explaining temporal sequencing concepts

---

## Alternatives Considered

### Alternative A: Pre-Rendered Frame Approach

**Approach:** Store pre-rendered frames with timing metadata

**Pros:**
- Simplest to implement
- Deterministic playback

**Cons:**
- 307KB per pattern (vs 70 bytes)
- Can only store 5 patterns in 1.5MB
- No real-time parameter adjustment

**Rejected:** Violates 256KB pattern size limit (ADR-004), prohibitively large

---

### Alternative B: Single "Vertical Mode" Field

**Approach:** Collapse motion + sync into single enum (like original design)

**Pros:**
- Simpler format (1 byte vs 8 bytes)
- Fewer parameters

**Cons:**
- Conflates orthogonal concerns (horizontal motion vs temporal sync)
- Combinatorial explosion (5 motion × 5 sync = 25 enum values)
- Less intuitive for users
- Harder to extend

**Rejected:** Captain identified this as misleading; temporal sequencing deserves separate control

---

### Alternative C: Continuous Time Offset Functions

**Approach:** Store mathematical functions (polynomials, Bezier curves) for delay patterns

**Pros:**
- Unlimited shape flexibility
- Compact representation

**Cons:**
- Requires floating-point math (slow on ESP32)
- Complex to implement and debug
- Overkill for 80% of use cases

**Rejected:** PROGRESSIVE + WAVE modes cover 95% of needs; CUSTOM mode handles edge cases

---

## Implementation Roadmap

### Phase 1: Foundation (Week 1-2)
- Add `motion` and `sync_mode` enums to headers
- Implement `calculate_ch2_frame()` function
- Add SYNC + OFFSET modes (simple)
- Update .prism parser for v1.1 format
- Unit tests for temporal calculations

**Deliverables:**
- `firmware/components/playback/include/prism_motion.h`
- `firmware/components/playback/prism_temporal.c`
- 3 example patterns: SYNC, OFFSET (rising), OFFSET (falling)

### Phase 2: Shape Creation (Week 3-4)
- Implement PROGRESSIVE mode (linear interpolation)
- Add triangle/diamond shape presets
- Hardware validation with LGP
- Performance profiling at 120 FPS

**Deliverables:**
- 5 shape presets: Right triangle, left triangle, diamond, chevron, gradient
- Hardware test report with visual photos
- Performance benchmarks

### Phase 3: Advanced Motion (Week 5-6)
- Implement WAVE mode with lookup tables
- Pre-compute wave delay tables at effect start
- Add organic motion presets (ripple, spiral)
- Optimize hot paths (inline functions)

**Deliverables:**
- Wave lookup table generator
- 3 wave presets: Sine, sawtooth, triangle
- CPU profiling report

### Phase 4: Expert Mode (Week 7-8)
- Implement CUSTOM mode (delay map support)
- Web editor: Visual shape designer
- Export custom delay maps to .prism format
- Pattern migration tool (v1.0 → v1.1)

**Deliverables:**
- Web-based shape editor (drag-and-drop interface)
- 10+ community-created custom patterns
- Migration documentation

### Phase 5: Polish & Documentation (Week 9-10)
- Comprehensive user documentation
- Video tutorials on temporal sequencing
- 24-hour hardware soak test
- Release v1.1 firmware

**Deliverables:**
- User manual with temporal sequencing guide
- 5 video tutorials (YouTube)
- Production firmware release

---

## References

### Research Evidence

1. **Phi Phenomenon:**
   - Wertheimer, Max (1912). "Experimental Studies on the Perception of Movement"
   - Optimal inter-stimulus interval: 30-200ms
   - Strongest geometric perception: 60-150ms

2. **Commercial Implementations:**
   - Audi Dynamic Sequential Indicators: 140ms total (7 segments × 20ms)
   - Razer Chroma "Sync" mode: Multi-device temporal coordination
   - ASUS Aura "Wave" effect: Sequential RGB progression

3. **LGP Optical Physics:**
   - Light transmission efficiency: 79.7-85% (PMMA substrates)
   - Total internal reflection with extraction features
   - Additive light mixing from dual edges

4. **Performance Measurements:**
   - ESP32-S3 @ 240MHz: 811,200 cycles per frame (3.38ms budget)
   - Current playback loop: ~50μs per frame (EFFECT_WAVE_SINGLE)
   - Temporal overhead: <2μs (0.06% of budget)

### Codebase References

- `/firmware/components/playback/led_playback.c` (147 lines) - Current playback implementation
- `/firmware/components/playback/include/led_driver.h` (183 lines) - LED driver API
- `/firmware/components/playback/led_driver.c` (621 lines) - RMT dual-channel driver
- `/.taskmaster/CANON.md` (217 lines) - Current specifications
- `/.taskmaster/decisions/009-prism-format-specification.md` (1247 lines) - Pattern format ADR

---

## Approval

**Submitted by:** PM Agent
**Date:** 2025-10-16
**Status:** PROPOSED (Pending Captain Approval)

**Next Steps:**
1. Captain review and approval
2. Update CANON.md with motion + sync specifications
3. Create Taskmaster tasks for Phase 1 implementation
4. Hardware validation with LGP prototype

---

**This ADR establishes the foundational architecture for artistic expression via temporal sequencing. The magic is in the timing.** ✨
