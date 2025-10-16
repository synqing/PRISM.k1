# PRISM.K1 Temporal Sequencing Architecture
## Dual-Edge LGP Motion System Design

**Research Date:** 2025-10-16
**Status:** PROPOSED - Awaiting Captain Approval
**Architect:** Agent 5 (Software Architect)

---

## Executive Summary

This document defines the complete temporal sequencing architecture for PRISM.K1's dual-edge Light Guide Plate (LGP) system. The design enables artistic shape creation through controlled timing relationships between CH1 (bottom, 160 LEDs) and CH2 (top, 160 LEDs), operating at 120 FPS with a 3.38ms CPU budget per frame.

**Key Innovation:** Temporal sequencing creates the illusion of vertical motion, depth, and complex shapes by controlling WHEN each channel fires, not WHERE the LEDs are positioned.

---

## 1. TEMPORAL MODE DESIGN (3 Tiers)

### Tier 1: Simple Mode (80% of users)

**Single Delay Value**

```c
typedef struct {
    temporal_mode_t mode;      // TEMPORAL_OFFSET
    uint16_t delay_ms;         // 0-500ms
} temporal_simple_t;
```

**Format Overhead:** 3 bytes
**CPU Cost:** ~10 cycles (single addition)
**Use Cases:**
- Rising effect: CH2 fires 150ms after CH1
- Falling effect: CH1 fires 150ms after CH2
- Depth illusion: Slight offset (30-50ms) creates layering

**Example .prism Field:**
```c
.temporal_mode = TEMPORAL_OFFSET
.temporal_params.offset.delay_ms = 150
```

**Visual Result (Rising Fire):**
```
T=0ms:    Bottom edge glows (CH1 fire starts)
T=150ms:  Top edge begins (CH2 fire starts)
T=300ms:  Both channels full (overlapping fire)

User Perception: Fire "rising" from bottom to top
```

---

### Tier 2: Advanced Mode (15% of users)

**Progressive Delay (Linear Gradient)**

```c
typedef struct {
    temporal_mode_t mode;           // TEMPORAL_PROGRESSIVE
    uint16_t start_delay_ms;        // Delay at LED 0
    uint16_t end_delay_ms;          // Delay at LED 159
} temporal_progressive_t;
```

**Format Overhead:** 5 bytes
**CPU Cost:** ~20 cycles per LED (linear interpolation)
**Use Cases:**
- Diagonal sweep: Delay increases from left to right
- Triangle shapes: Delay peaks at center, decreases at edges
- Angular effects: Creates visual "tilt" perception

**Example .prism Field:**
```c
.temporal_mode = TEMPORAL_PROGRESSIVE
.temporal_params.progressive.start_delay_ms = 0
.temporal_params.progressive.end_delay_ms = 300
```

**Calculation at Render Time:**
```c
// For each LED position (0-159):
uint16_t delay_for_led = start_delay_ms +
    ((end_delay_ms - start_delay_ms) * led_index) / 159;
```

**Visual Result (Diagonal Sweep):**
```
T=0ms:    Left side of both channels fire
T=150ms:  Center of both channels fire
T=300ms:  Right side of both channels fire

User Perception: Diagonal slash from bottom-left to top-right
```

---

### Tier 3: Expert Mode (5% of users)

**Arbitrary Delay Map (Per-LED Control)**

```c
typedef struct {
    temporal_mode_t mode;           // TEMPORAL_CUSTOM
    uint16_t delay_map_offset;      // Offset in file to delay map
    uint8_t  delay_map_count;       // Number of entries (1-160)
} temporal_custom_t;
```

**Format Overhead:** 4 bytes header + (2 bytes × count) delay map
**CPU Cost:** ~5 cycles per LED (array lookup)
**Use Cases:**
- Complex shapes (custom curves, artistic patterns)
- Per-LED choreography
- Experimental artistic effects

**Delay Map Structure (in file):**
```c
// At offset delay_map_offset:
uint16_t delay_map[delay_map_count];  // Delays in milliseconds

// If delay_map_count < 160, interpolate between entries
```

**Visual Result (Custom Triangle):**
```
Delay map: [0, 50, 100, 150, 200, 150, 100, 50, 0, ...]
           ^----- Creates peak at LED 80 (center)

T=0ms:    Edges fire (LED 0, 159)
T=100ms:  Quarter points fire (LED 40, 120)
T=200ms:  Center fires (LED 80)

User Perception: Triangle pointing up (apex at top center)
```

---

## 2. TEMPORAL MODE ENUM

```c
/**
 * Temporal sequencing modes
 *
 * Controls the timing relationship between CH1 and CH2 for artistic
 * shape creation on dual-edge LGP systems.
 */
typedef enum {
    /**
     * TEMPORAL_SYNC: No delay (CH1 = CH2 timing)
     *
     * Visual: Unified surface, maximum brightness
     * CPU: 0 cycles overhead
     * Use: MIRROR mode, simple reinforced patterns
     */
    TEMPORAL_SYNC          = 0x00,

    /**
     * TEMPORAL_OFFSET: Fixed delay (CH2 = CH1 + delay_ms)
     *
     * Visual: Rising/falling effects, depth illusion
     * CPU: ~10 cycles (single addition)
     * Use: Vertical motion, layered depth
     * Parameters: delay_ms (0-500)
     */
    TEMPORAL_OFFSET        = 0x01,

    /**
     * TEMPORAL_PROGRESSIVE: Linear delay gradient across strip
     *
     * Visual: Diagonal sweeps, angular effects
     * CPU: ~20 cycles per LED (lerp)
     * Use: Triangle shapes, diagonal motion
     * Parameters: start_delay_ms, end_delay_ms
     */
    TEMPORAL_PROGRESSIVE   = 0x02,

    /**
     * TEMPORAL_WAVE: Sinusoidal delay pattern
     *
     * Visual: Wave-like vertical motion
     * CPU: ~50 cycles per LED (sin8 lookup)
     * Use: Organic motion, wave effects
     * Parameters: period_ms, amplitude_ms, phase_offset
     */
    TEMPORAL_WAVE          = 0x03,

    /**
     * TEMPORAL_CUSTOM: Arbitrary per-LED delay map
     *
     * Visual: Complex shapes, custom choreography
     * CPU: ~5 cycles per LED (array lookup)
     * Use: Expert artistic control, experimental
     * Parameters: delay_map_offset, delay_map_count
     */
    TEMPORAL_CUSTOM        = 0x04,

    /**
     * TEMPORAL_MIRROR: CH2 mirrors CH1 with inverted delay
     *
     * Visual: Symmetric vertical effects
     * CPU: Same as base mode
     * Use: Symmetric shapes (diamonds, arrows)
     * Parameters: Base mode parameters + mirror flag
     */
    TEMPORAL_MIRROR        = 0x05,

} temporal_mode_t;
```

---

## 3. MODE-SPECIFIC PARAMETERS

### TEMPORAL_SYNC
**Parameters:** None
**Visual:** Both channels fire simultaneously
**Effect:** Maximum brightness, unified surface
**Example:** Solid color, ambient glow

---

### TEMPORAL_OFFSET
**Parameters:**
- `delay_ms` (uint16_t): 0-500ms

**Visual Effects:**
- **Rising (delay > 0):** CH1 → CH2 (bottom first)
- **Falling (delay < 0):** CH2 → CH1 (top first)
- **Depth (30-50ms):** Subtle layering

**Frame-by-Frame Example (Rising Fire, delay=150ms):**
```
Frame 0 (T=0ms):
  CH1: [Fire frame 0]  ← Starts immediately
  CH2: [Black]         ← Waiting

Frame 18 (T=150ms):
  CH1: [Fire frame 18] ← Advanced 18 frames
  CH2: [Fire frame 0]  ← Just starting

Frame 36 (T=300ms):
  CH1: [Fire frame 36]
  CH2: [Fire frame 18] ← Always 18 frames behind

User Perception: Fire rises from bottom, reaches top 150ms later
```

**CPU Cost:**
```c
uint32_t ch2_frame = ch1_frame - (delay_ms * 120 / 1000);  // ~10 cycles
```

---

### TEMPORAL_PROGRESSIVE
**Parameters:**
- `start_delay_ms` (uint16_t): Delay at LED 0
- `end_delay_ms` (uint16_t): Delay at LED 159

**Visual Effects:**
- **Diagonal sweep:** 0ms → 300ms creates diagonal slash
- **Triangle (center peak):** 0ms → 200ms → 0ms (requires custom map)
- **Angular tilt:** Creates perceived "angle" in vertical direction

**Algorithm:**
```c
// For each LED position i (0-159):
uint16_t delay_i = start_delay_ms +
    ((end_delay_ms - start_delay_ms) * i) / 159;

// Convert to frame offset:
int32_t frame_offset_i = (delay_i * 120) / 1000;

// Render CH2 with per-LED frame offset:
ch2_frame_for_led_i = ch1_current_frame - frame_offset_i;
```

**Frame-by-Frame Example (Diagonal Sweep, 0→300ms):**
```
T=0ms:
  CH1 LED 0:   [Wave frame 0]
  CH1 LED 159: [Wave frame 0]
  CH2 LED 0:   [Wave frame 0]  ← No delay (start_delay=0)
  CH2 LED 159: [Black]         ← 300ms delay (not started)

T=150ms:
  CH1 LED 0:   [Wave frame 18]
  CH1 LED 159: [Wave frame 18]
  CH2 LED 0:   [Wave frame 18] ← No delay
  CH2 LED 80:  [Wave frame 9]  ← 150ms delay (mid-strip)
  CH2 LED 159: [Black]         ← 300ms delay (halfway there)

T=300ms:
  CH1 LED 0:   [Wave frame 36]
  CH1 LED 159: [Wave frame 36]
  CH2 LED 0:   [Wave frame 36] ← No delay
  CH2 LED 80:  [Wave frame 18] ← 150ms delay
  CH2 LED 159: [Wave frame 0]  ← 300ms delay (just started)

User Perception: Diagonal line sweeping bottom-left to top-right
```

**CPU Cost:**
```c
// Per LED (160 iterations):
// - 2 multiplications
// - 1 division
// - 1 subtraction
// Total: ~20 cycles × 160 LEDs = 3,200 cycles (<1% of 811,200 budget)
```

---

### TEMPORAL_WAVE
**Parameters:**
- `period_ms` (uint16_t): Wave period (wavelength in time)
- `amplitude_ms` (uint16_t): Max delay (wave amplitude)
- `phase_offset` (uint8_t): Starting phase (0-255)

**Visual Effects:**
- **Sinusoidal vertical motion:** Smooth wave propagation
- **Organic breathing:** Gentle rise/fall rhythm
- **Complex patterns:** Multiple peaks/valleys across strip

**Algorithm:**
```c
// For each LED position i (0-159):
uint8_t theta = ((i * 256) / 160 + phase_offset) & 0xFF;
uint8_t wave_val = sin8(theta);  // 0-255

// Convert to delay:
uint16_t delay_i = (amplitude_ms * wave_val) / 255;

// Convert to frame offset:
int32_t frame_offset_i = (delay_i * 120) / 1000;
```

**Visual Result (Sine Wave Vertical Motion):**
```
Delay map (amplitude=200ms, period matches strip):
LED 0:   delay=100ms (sin(0°) = 0.5)
LED 40:  delay=200ms (sin(90°) = 1.0)   ← Peak
LED 80:  delay=100ms (sin(180°) = 0.5)
LED 120: delay=0ms   (sin(270°) = 0.0)  ← Trough
LED 159: delay=100ms (sin(360°) = 0.5)

User Perception: Smooth sinusoidal vertical wave motion
```

**CPU Cost:**
```c
// Per LED:
// - sin8 lookup: ~50 cycles (table lookup + scaling)
// - Arithmetic: ~10 cycles
// Total: ~60 cycles × 160 LEDs = 9,600 cycles (~1.2% of budget)
```

---

### TEMPORAL_CUSTOM
**Parameters:**
- `delay_map_offset` (uint16_t): File offset to delay map
- `delay_map_count` (uint8_t): Number of delay entries (1-160)

**Delay Map Format:**
```c
// At file offset delay_map_offset:
typedef struct __attribute__((packed)) {
    uint16_t delays[delay_map_count];  // Milliseconds per LED
} temporal_delay_map_t;

// If delay_map_count < 160, linearly interpolate between entries
```

**Interpolation Algorithm (if count < 160):**
```c
// Map 160 LEDs to delay_map_count entries:
for (uint8_t i = 0; i < 160; i++) {
    float map_index = (i * (delay_map_count - 1)) / 159.0;
    uint8_t lower = (uint8_t)map_index;
    uint8_t upper = lower + 1;
    float frac = map_index - lower;

    uint16_t delay_lower = delay_map[lower];
    uint16_t delay_upper = delay_map[upper];
    uint16_t delay_i = delay_lower + (delay_upper - delay_lower) * frac;

    // Convert to frame offset
    frame_offset[i] = (delay_i * 120) / 1000;
}
```

**Visual Result (Triangle Pointing Up):**
```
Delay map (8 entries):
[0, 50, 100, 150, 200, 150, 100, 50]
 ^                  ^              ^
 LED 0              LED 80         LED 159

Interpolated to 160 LEDs:
LED 0:   0ms
LED 20:  25ms
LED 40:  75ms
LED 60:  125ms
LED 80:  200ms  ← Peak
LED 100: 125ms
LED 120: 75ms
LED 140: 25ms
LED 159: 50ms

T=0ms:    Edges fire
T=100ms:  Quarter points fire
T=200ms:  Center fires (apex)

User Perception: Triangle shape pointing upward
```

**CPU Cost:**
```c
// With pre-computed interpolation (during pattern load):
// Per LED: ~5 cycles (array lookup)
// Total: ~5 × 160 = 800 cycles (<0.1% of budget)

// If interpolating at render time (not recommended):
// Per LED: ~100 cycles (float math)
// Total: ~100 × 160 = 16,000 cycles (~2% of budget)
```

---

### TEMPORAL_MIRROR
**Parameters:**
- Base mode parameters (OFFSET, PROGRESSIVE, etc.)
- Mirror flag enabled

**Visual Effects:**
- **Symmetric shapes:** Diamond, arrow, hourglass
- **Mirrored motion:** CH1 and CH2 move in symmetric patterns

**Algorithm:**
```c
// For MIRROR mode:
// CH1 uses normal delay calculation
// CH2 uses INVERTED delay calculation

// Example with TEMPORAL_PROGRESSIVE (0ms → 200ms):
CH1 delays: [0, 10, 20, 30, ..., 190, 200]
CH2 delays: [200, 190, 180, 170, ..., 10, 0]  ← Mirrored

// Result: Diamond shape
```

**Visual Result (Diamond with PROGRESSIVE):**
```
T=0ms:
  CH1 left edge fires  (LED 0)
  CH2 right edge fires (LED 159)

T=100ms:
  CH1 center fires     (LED 80)
  CH2 center fires     (LED 80)

  Diamond shape forms with apex at center

T=200ms:
  CH1 right edge fires (LED 159)
  CH2 left edge fires  (LED 0)

  Diamond fully formed
```

**CPU Cost:** Same as base mode (just inverts delay values)

---

## 4. FORMAT STRUCTURE DESIGN

### Binary Layout (Optimized for Space)

```c
/**
 * Pattern header with temporal sequencing support
 *
 * Size: 64 bytes (cache-aligned, unchanged from ADR-009)
 * Added: temporal_mode + temporal_params (8 bytes total)
 */
typedef struct __attribute__((packed)) {
    // Magic and version (8 bytes) - UNCHANGED
    uint32_t magic;              // 0x4D535250 ("PRSM")
    uint8_t  version_major;      // 1 (v1.0)
    uint8_t  version_minor;      // 1 (v1.1 adds temporal sequencing)
    uint16_t header_size;        // 64 bytes

    // Pattern metadata (24 bytes) - UNCHANGED
    uint8_t  effect_id;          // Built-in effect ID
    uint8_t  channel_mode;       // MIRROR, SPLIT, etc. (from LGP research)
    uint16_t flags;              // Pattern flags
    char     name[20];           // Pattern display name

    // Effect parameters (16 bytes) - UNCHANGED
    uint8_t  brightness;         // Global brightness (0-255)
    uint8_t  speed;              // Animation speed (0-255)
    uint8_t  fade;               // Fade/blend amount (0-255)
    uint8_t  param_count;        // Number of effect parameters
    uint8_t  params[12];         // Effect-specific parameters

    // Temporal sequencing (8 bytes) - NEW
    uint8_t  temporal_mode;      // Temporal mode enum (see above)
    uint8_t  _reserved_temporal; // Reserved for future use

    union {
        // TEMPORAL_OFFSET (3 bytes used)
        struct {
            uint16_t delay_ms;   // 0-500ms
            uint8_t  _pad[4];
        } offset;

        // TEMPORAL_PROGRESSIVE (5 bytes used)
        struct {
            uint16_t start_delay_ms;
            uint16_t end_delay_ms;
            uint8_t  _pad[2];
        } progressive;

        // TEMPORAL_WAVE (6 bytes used)
        struct {
            uint16_t period_ms;
            uint16_t amplitude_ms;
            uint8_t  phase_offset;
            uint8_t  _pad[1];
        } wave;

        // TEMPORAL_CUSTOM (4 bytes used, rest in external map)
        struct {
            uint16_t delay_map_offset;  // File offset
            uint8_t  delay_map_count;   // 1-160
            uint8_t  _pad[3];
        } custom;

        // TEMPORAL_MIRROR (1 byte flag, rest from base mode)
        struct {
            uint8_t  base_mode;         // Which mode to mirror
            uint8_t  _pad[5];
        } mirror;

        // Raw padding to ensure 6 bytes
        uint8_t _raw[6];
    } temporal_params;

    // Palette configuration (8 bytes) - UNCHANGED
    uint8_t  palette_id;         // Built-in palette ID
    uint8_t  palette_count;      // Number of palette colors
    uint16_t palette_offset;     // Offset to palette data
    uint32_t reserved1;          // Reserved

    // Validation (8 bytes) - UNCHANGED
    uint32_t data_crc32;         // CRC32 of palette + delay map
    uint32_t header_crc32;       // CRC32 of bytes 0-59
} prism_header_t;

// Compile-time size validation
_Static_assert(sizeof(prism_header_t) == 64, "Header must be 64 bytes");
```

### Byte Overhead Summary

| Temporal Mode | Header Bytes | External Data | Total Overhead |
|---------------|-------------|---------------|----------------|
| TEMPORAL_SYNC | 1 | 0 | **1 byte** |
| TEMPORAL_OFFSET | 3 | 0 | **3 bytes** |
| TEMPORAL_PROGRESSIVE | 5 | 0 | **5 bytes** |
| TEMPORAL_WAVE | 6 | 0 | **6 bytes** |
| TEMPORAL_CUSTOM | 4 | 2×count | **4 + 320 bytes max** |
| TEMPORAL_MIRROR | 2 | 0 | **2 bytes** |

**All modes fit within existing 64-byte header** (except CUSTOM delay map, which goes in pattern data section after palette).

---

## 5. SHAPE CREATION ALGORITHMS

### Triangle (Pointing Right →)

```c
/**
 * Configuration:
 * - Motion: LEFT_ORIGIN (horizontal sweep L→R)
 * - Temporal: TEMPORAL_PROGRESSIVE (0ms → 200ms)
 * - Result: Triangle pointing right
 */

prism_header_t triangle_right = {
    .effect_id = PRISM_EFFECT_WAVE,
    .channel_mode = CHANNEL_MODE_MIRROR,  // Both channels same
    .temporal_mode = TEMPORAL_PROGRESSIVE,
    .temporal_params.progressive = {
        .start_delay_ms = 0,      // Left edge fires first
        .end_delay_ms = 200       // Right edge fires last
    },
    .params = {
        .wavelength = 32,
        .color1_r = 255, .color1_g = 0, .color1_b = 0,
        .color2_r = 0, .color2_g = 0, .color2_b = 255
    }
};
```

**Frame-by-Frame Breakdown:**
```
T=0ms:
  CH1 left edge:  [Wave starts]  ──┐
  CH2 left edge:  [Wave starts]  ──┘  Unified left edge
  Right side:     [Dark]

T=100ms:
  Left half:      [Wave active]  ──────────┐
  CH1 right half: [Starting]               │ Triangle
  CH2 right half: [Starting]               │ forming
  Far right:      [Dark]         ──────────┘

T=200ms:
  Entire strip:   [Wave active]  ──────────────────────→

Visual: Triangle sweeps left to right
```

---

### Diamond ◇

```c
/**
 * Configuration:
 * - Motion: CENTER_ORIGIN (both channels bloom from center)
 * - Temporal: TEMPORAL_MIRROR + TEMPORAL_PROGRESSIVE
 * - Result: Diamond shape
 */

prism_header_t diamond = {
    .effect_id = PRISM_EFFECT_GRADIENT,
    .channel_mode = CHANNEL_MODE_MIRROR,
    .temporal_mode = TEMPORAL_MIRROR,
    .temporal_params.mirror = {
        .base_mode = TEMPORAL_PROGRESSIVE
    },
    .temporal_params.progressive = {
        .start_delay_ms = 200,    // Edges fire last
        .end_delay_ms = 0         // Center fires first
    }
};
```

**Effective Delay Maps:**
```
CH1 (normal):    [200, 190, 180, ..., 10, 0, 10, ..., 180, 190, 200]
                  ^--- Edges                Center ---^          Edges ---^

CH2 (mirrored):  [0, 10, 20, ..., 190, 200, 190, ..., 20, 10, 0]
                  ^--- Edges                Center ---^      Edges ---^

Combined Visual:
  T=0ms:   Center fires (both channels) ──── ◇ apex
  T=100ms: Middle regions fire          ──── ◇ expanding
  T=200ms: Edges fire                   ──── ◇ full diamond
```

**Frame-by-Frame:**
```
T=0ms:
  ┌─────────────────┐
  │       ║         │ ← Center glows (both CH1+CH2)
  └─────────────────┘

T=100ms:
  ┌─────────────────┐
  │     ◇◇◇         │ ← Diamond forming
  │    ◇   ◇        │
  │     ◇◇◇         │
  └─────────────────┘

T=200ms:
  ┌─────────────────┐
  │  ◇       ◇      │ ← Full diamond
  │ ◇         ◇     │
  │◇           ◇    │
  │ ◇         ◇     │
  │  ◇       ◇      │
  └─────────────────┘
```

---

### Arrow (Pointing Up ↑)

```c
/**
 * Configuration:
 * - Motion: EDGE_ORIGIN (both channels converge to center)
 * - Temporal: TEMPORAL_CUSTOM (custom apex shape)
 * - Result: Arrow pointing upward
 */

prism_header_t arrow_up = {
    .effect_id = PRISM_EFFECT_SOLID,
    .channel_mode = CHANNEL_MODE_SPLIT,  // CH1 = base, CH2 = arrowhead
    .temporal_mode = TEMPORAL_CUSTOM,
    .temporal_params.custom = {
        .delay_map_offset = 128,  // After header+palette
        .delay_map_count = 8      // 8 control points
    }
};

// Delay map (8 entries, interpolated to 160):
uint16_t arrow_delay_map[8] = {
    0,    // LED 0:   Edges start
    50,   // LED 20
    100,  // LED 40
    150,  // LED 60
    200,  // LED 80:  Center (apex) fires last
    150,  // LED 100
    100,  // LED 120
    0     // LED 159: Edges start
};
```

**Visual Result:**
```
T=0ms:
  ┌─────────────────┐
  │█               █│ ← Edges fire (CH1 bottom, CH2 top)
  └─────────────────┘

T=100ms:
  ┌─────────────────┐
  │ ▲             ▲ │ ← Arrow forming
  │ █             █ │
  │ █             █ │
  └─────────────────┘

T=200ms:
  ┌─────────────────┐
  │     ▲▲▲▲▲       │ ← Apex fires (arrowhead)
  │    ▲     ▲      │
  │   ▲       ▲     │
  │  ▲         ▲    │
  │ ▲           ▲   │
  │█             █  │
  └─────────────────┘

Perception: Arrow pointing upward
```

---

### Wave (Sine Pattern ~~~~~)

```c
/**
 * Configuration:
 * - Motion: LEFT_ORIGIN (horizontal sweep)
 * - Temporal: TEMPORAL_WAVE (sinusoidal delay)
 * - Result: Vertical sine wave motion
 */

prism_header_t wave = {
    .effect_id = PRISM_EFFECT_WAVE,
    .channel_mode = CHANNEL_MODE_MIRROR,
    .temporal_mode = TEMPORAL_WAVE,
    .temporal_params.wave = {
        .period_ms = 400,       // Full sine wave period
        .amplitude_ms = 150,    // Max delay
        .phase_offset = 0       // Starting phase
    },
    .params = {
        .wavelength = 64,
        .color1_r = 0, .color1_g = 255, .color1_b = 255,  // Cyan
        .color2_r = 0, .color2_g = 0, .color2_b = 255     // Blue
    }
};
```

**Delay Pattern (Sinusoidal):**
```
LED 0:   delay=75ms   (sin(0°) = 0.5)
LED 20:  delay=120ms  (sin(45°) = 0.8)
LED 40:  delay=150ms  (sin(90°) = 1.0)   ← Peak
LED 60:  delay=120ms  (sin(135°) = 0.8)
LED 80:  delay=75ms   (sin(180°) = 0.5)
LED 100: delay=30ms   (sin(225°) = 0.2)
LED 120: delay=0ms    (sin(270°) = 0.0)  ← Trough
LED 140: delay=30ms   (sin(315°) = 0.2)
LED 159: delay=75ms   (sin(360°) = 0.5)
```

**Visual Result (Animated):**
```
T=0ms:
  ┌─────────────────┐
  │~   ~   ~   ~   ~│ ← Initial sine wave
  └─────────────────┘

T=200ms:
  ┌─────────────────┐
  │ ~   ~   ~   ~   │ ← Wave propagates
  │~               ~│   (vertical motion)
  └─────────────────┘

T=400ms:
  ┌─────────────────┐
  │  ~   ~   ~   ~  │ ← Full period
  └─────────────────┘

Perception: Smooth vertical sine wave motion
```

---

## 6. RENDERING ARCHITECTURE

### Playback Loop Integration

```c
/**
 * Main playback task (120 FPS)
 *
 * Replaces existing led_playback.c render loop
 */
void playback_task(void *pvParameters) {
    ESP_LOGI(TAG, "Playback task started on core %d", xPortGetCoreID());

    TickType_t last_wake = xTaskGetTickCount();

    // Frame buffers for both channels (GRB24)
    static uint8_t frame_ch1[LED_FRAME_SIZE_CH];  // 480 bytes
    static uint8_t frame_ch2[LED_FRAME_SIZE_CH];  // 480 bytes

    uint32_t global_frame_counter = 0;

    while (1) {
        if (s_pb.running) {
            // Calculate frame numbers for each channel
            uint32_t ch1_frame = global_frame_counter;
            uint32_t ch2_frame = calculate_ch2_frame(
                pattern.temporal_mode,
                &pattern.temporal_params,
                global_frame_counter
            );

            // Render both channels (using pattern executor)
            esp_err_t ret = prism_executor_render_frame_temporal(
                ch1_frame,
                ch2_frame,
                frame_ch1,
                frame_ch2
            );

            if (ret == ESP_OK) {
                // Submit to LED driver (hardware RMT)
                led_driver_submit_frames(frame_ch1, frame_ch2);
            } else {
                ESP_LOGE(TAG, "Frame render failed: %s", esp_err_to_name(ret));
            }

            global_frame_counter++;
        }

        // 120 FPS timing (8.33ms per frame)
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(LED_FRAME_TIME_MS));
    }

    vTaskDelete(NULL);
}
```

---

### calculate_ch2_frame() Implementation

```c
/**
 * Calculate CH2 frame number based on temporal sequencing mode
 *
 * @param mode Temporal mode enum
 * @param params Mode-specific parameters
 * @param ch1_frame Current CH1 frame number
 * @return CH2 frame number (may be in past or future)
 */
static inline uint32_t calculate_ch2_frame(
    temporal_mode_t mode,
    const temporal_params_union_t *params,
    uint32_t ch1_frame)
{
    switch (mode) {
        case TEMPORAL_SYNC:
            // No offset
            return ch1_frame;

        case TEMPORAL_OFFSET: {
            // Fixed delay: CH2 = CH1 - delay
            uint16_t delay_ms = params->offset.delay_ms;
            int32_t frame_offset = (delay_ms * 120) / 1000;  // Convert to frames
            return (ch1_frame >= frame_offset) ? (ch1_frame - frame_offset) : 0;
        }

        case TEMPORAL_PROGRESSIVE:
            // Progressive uses per-LED calculation (see below)
            return ch1_frame;  // Placeholder (calculated per-LED)

        case TEMPORAL_WAVE:
            // Wave uses per-LED calculation (see below)
            return ch1_frame;  // Placeholder (calculated per-LED)

        case TEMPORAL_CUSTOM:
            // Custom uses per-LED delay map lookup
            return ch1_frame;  // Placeholder (calculated per-LED)

        case TEMPORAL_MIRROR:
            // Mirror uses base mode calculation with inversion
            return ch1_frame;  // Placeholder (calculated per-LED)

        default:
            ESP_LOGE("temporal", "Unknown temporal mode: %d", mode);
            return ch1_frame;
    }
}
```

---

### Per-LED Frame Calculation (PROGRESSIVE, WAVE, CUSTOM)

```c
/**
 * Render frame with per-LED temporal offsets
 *
 * Used for PROGRESSIVE, WAVE, CUSTOM, MIRROR modes
 */
esp_err_t prism_executor_render_frame_temporal(
    uint32_t ch1_frame,
    uint32_t ch2_base_frame,
    uint8_t *frame_ch1,
    uint8_t *frame_ch2)
{
    // Render CH1 normally
    esp_err_t ret = effect->render(
        &pattern,
        ch1_frame,
        frame_ch1
    );

    if (ret != ESP_OK) return ret;

    // Render CH2 with per-LED temporal offsets
    switch (pattern.temporal_mode) {
        case TEMPORAL_SYNC:
        case TEMPORAL_OFFSET:
            // Simple mode: single frame number for all LEDs
            ret = effect->render(
                &pattern,
                ch2_base_frame,
                frame_ch2
            );
            break;

        case TEMPORAL_PROGRESSIVE: {
            // Per-LED linear interpolation
            uint16_t start_delay = pattern.temporal_params.progressive.start_delay_ms;
            uint16_t end_delay = pattern.temporal_params.progressive.end_delay_ms;

            // Render each LED with its own frame offset
            for (uint8_t led = 0; led < LED_COUNT_PER_CH; led++) {
                // Calculate delay for this LED
                uint16_t delay_ms = start_delay +
                    ((end_delay - start_delay) * led) / (LED_COUNT_PER_CH - 1);

                int32_t frame_offset = (delay_ms * 120) / 1000;
                uint32_t led_frame = (ch1_frame >= frame_offset) ?
                    (ch1_frame - frame_offset) : 0;

                // Render single LED at its specific frame
                render_single_led(
                    &pattern,
                    led_frame,
                    led,
                    &frame_ch2[led * 3]
                );
            }
            break;
        }

        case TEMPORAL_WAVE: {
            // Per-LED sinusoidal delay
            uint16_t amplitude = pattern.temporal_params.wave.amplitude_ms;
            uint8_t phase_offset = pattern.temporal_params.wave.phase_offset;

            for (uint8_t led = 0; led < LED_COUNT_PER_CH; led++) {
                // Calculate sine wave delay
                uint8_t theta = ((led * 256) / LED_COUNT_PER_CH + phase_offset) & 0xFF;
                uint8_t wave_val = sin8(theta);  // 0-255

                uint16_t delay_ms = (amplitude * wave_val) / 255;
                int32_t frame_offset = (delay_ms * 120) / 1000;
                uint32_t led_frame = (ch1_frame >= frame_offset) ?
                    (ch1_frame - frame_offset) : 0;

                render_single_led(
                    &pattern,
                    led_frame,
                    led,
                    &frame_ch2[led * 3]
                );
            }
            break;
        }

        case TEMPORAL_CUSTOM: {
            // Per-LED delay map lookup
            const uint16_t *delay_map = pattern.temporal_delay_map;  // Pre-loaded

            for (uint8_t led = 0; led < LED_COUNT_PER_CH; led++) {
                uint16_t delay_ms = delay_map[led];
                int32_t frame_offset = (delay_ms * 120) / 1000;
                uint32_t led_frame = (ch1_frame >= frame_offset) ?
                    (ch1_frame - frame_offset) : 0;

                render_single_led(
                    &pattern,
                    led_frame,
                    led,
                    &frame_ch2[led * 3]
                );
            }
            break;
        }

        default:
            ESP_LOGE("temporal", "Unhandled temporal mode: %d", pattern.temporal_mode);
            return ESP_FAIL;
    }

    return ESP_OK;
}
```

---

### Performance Optimization: Lookup Tables

For PROGRESSIVE and WAVE modes, pre-compute frame offsets at pattern load time:

```c
/**
 * Pre-computed frame offset table (static allocation)
 *
 * Calculated once when pattern loads, used every frame
 */
typedef struct {
    int32_t frame_offsets[LED_COUNT_PER_CH];  // 160 × 4 bytes = 640 bytes
    bool is_valid;
} temporal_offset_table_t;

static temporal_offset_table_t s_offset_table = {0};

/**
 * Pre-compute frame offsets when pattern loads
 */
void temporal_precompute_offsets(const prism_header_t *pattern) {
    switch (pattern->temporal_mode) {
        case TEMPORAL_PROGRESSIVE: {
            uint16_t start = pattern->temporal_params.progressive.start_delay_ms;
            uint16_t end = pattern->temporal_params.progressive.end_delay_ms;

            for (uint8_t led = 0; led < LED_COUNT_PER_CH; led++) {
                uint16_t delay_ms = start + ((end - start) * led) / (LED_COUNT_PER_CH - 1);
                s_offset_table.frame_offsets[led] = (delay_ms * 120) / 1000;
            }
            s_offset_table.is_valid = true;
            break;
        }

        case TEMPORAL_WAVE: {
            uint16_t amplitude = pattern->temporal_params.wave.amplitude_ms;
            uint8_t phase = pattern->temporal_params.wave.phase_offset;

            for (uint8_t led = 0; led < LED_COUNT_PER_CH; led++) {
                uint8_t theta = ((led * 256) / LED_COUNT_PER_CH + phase) & 0xFF;
                uint8_t wave_val = sin8(theta);
                uint16_t delay_ms = (amplitude * wave_val) / 255;
                s_offset_table.frame_offsets[led] = (delay_ms * 120) / 1000;
            }
            s_offset_table.is_valid = true;
            break;
        }

        default:
            s_offset_table.is_valid = false;
            break;
    }
}

/**
 * Fast per-LED rendering using pre-computed offsets
 */
void render_ch2_with_offsets(uint32_t ch1_frame, uint8_t *frame_ch2) {
    if (!s_offset_table.is_valid) return;

    for (uint8_t led = 0; led < LED_COUNT_PER_CH; led++) {
        int32_t offset = s_offset_table.frame_offsets[led];
        uint32_t led_frame = (ch1_frame >= offset) ? (ch1_frame - offset) : 0;

        render_single_led(&pattern, led_frame, led, &frame_ch2[led * 3]);
    }
}
```

**Memory Cost:** 640 bytes (static allocation, one-time per pattern)
**CPU Savings:** ~15 cycles per LED (eliminates runtime interpolation)
**Total Savings:** 15 × 160 = 2,400 cycles per frame (~21% reduction for PROGRESSIVE/WAVE)

---

## 7. PERFORMANCE VALIDATION (120 FPS @ 240MHz)

### CPU Budget Breakdown

**Frame Period:** 8.33ms (120 FPS)
**CPU Budget:** 3.38ms (40.56% of frame time)
**Total Cycles:** 240 MHz × 3.38ms = **811,200 cycles**

### Per-Mode CPU Cost

| Temporal Mode | Cycles per Frame | % of Budget | Status |
|---------------|------------------|-------------|--------|
| **TEMPORAL_SYNC** | 0 | 0.0% | ✅ PASS |
| **TEMPORAL_OFFSET** | 10 | 0.001% | ✅ PASS |
| **TEMPORAL_PROGRESSIVE** | 3,200 (or 800 with LUT) | 0.39% (0.10%) | ✅ PASS |
| **TEMPORAL_WAVE** | 9,600 (or 800 with LUT) | 1.18% (0.10%) | ✅ PASS |
| **TEMPORAL_CUSTOM** | 800 | 0.10% | ✅ PASS |
| **TEMPORAL_MIRROR** | ~3,200 | 0.39% | ✅ PASS |

### Worst-Case Scenario

**Pattern:** Complex Fire effect (1,200μs render time) + TEMPORAL_WAVE (9,600 cycles)

**Total Cost:**
- Fire effect: 1,200μs = 288,000 cycles
- Temporal WAVE: 9,600 cycles
- **Total:** 297,600 cycles = 1,240μs

**Budget Check:**
- Budget: 811,200 cycles (3.38ms)
- Used: 297,600 cycles (1.24ms)
- **Remaining:** 513,600 cycles (2.14ms)
- **Safety Margin:** 2.72× (172% headroom)

### Validation: ALL MODES FIT WITHIN BUDGET ✅

Even the most expensive combination (complex effect + TEMPORAL_WAVE) uses only **36.7% of CPU budget**, leaving **172% safety margin**.

---

## 8. BACKWARD COMPATIBILITY

### Version Detection

```c
/**
 * Pattern version field:
 *
 * v1.0: version_major=1, version_minor=0 (no temporal sequencing)
 * v1.1: version_major=1, version_minor=1 (temporal sequencing added)
 */

// On pattern load:
if (header.version_major == 1 && header.version_minor == 0) {
    // v1.0 pattern: Set default temporal mode
    header.temporal_mode = TEMPORAL_SYNC;
    header.channel_mode = CHANNEL_MODE_MIRROR;
}
```

### Migration Strategy (v1.0 → v1.1)

**Automatic Migration:**
```c
/**
 * Convert v1.0 pattern to v1.1 format
 *
 * Adds temporal_mode field with safe defaults
 */
esp_err_t migrate_pattern_v10_to_v11(prism_header_t *header) {
    if (header->version_major != 1 || header->version_minor != 0) {
        return ESP_ERR_INVALID_VERSION;
    }

    // Set safe defaults
    header->temporal_mode = TEMPORAL_SYNC;  // No delay (MIRROR behavior)
    header->channel_mode = CHANNEL_MODE_MIRROR;  // Both channels identical
    memset(&header->temporal_params, 0, sizeof(header->temporal_params));

    // Update version
    header->version_minor = 1;

    // Recalculate CRC
    header->header_crc32 = calculate_header_crc32(header);

    ESP_LOGI("migrate", "Migrated pattern '%s' from v1.0 to v1.1", header->name);

    return ESP_OK;
}
```

**User-Facing Migration:**
```
Pattern "Fire Warm" (v1.0) loaded
→ Auto-upgraded to v1.1 (no visual change)
→ Temporal mode: SYNC (same as before)
→ You can now customize temporal sequencing in settings
```

### Format Detection

```c
/**
 * Detect pattern version and apply compatibility layer
 */
esp_err_t load_pattern_with_compat(const char *filename) {
    prism_header_t header;

    // Read header from file
    FILE *f = fopen(filename, "rb");
    fread(&header, sizeof(header), 1, f);
    fclose(f);

    // Validate magic number
    if (header.magic != PRISM_MAGIC) {
        ESP_LOGE("loader", "Invalid magic: 0x%08X", header.magic);
        return ESP_ERR_INVALID_ARG;
    }

    // Check version
    if (header.version_major == 1) {
        if (header.version_minor == 0) {
            ESP_LOGI("loader", "Detected v1.0 pattern, migrating...");
            migrate_pattern_v10_to_v11(&header);
        } else if (header.version_minor == 1) {
            ESP_LOGI("loader", "Detected v1.1 pattern");
        } else {
            ESP_LOGW("loader", "Unknown minor version: %d", header.version_minor);
        }
    } else {
        ESP_LOGE("loader", "Unsupported major version: %d", header.version_major);
        return ESP_ERR_NOT_SUPPORTED;
    }

    // Continue with pattern loading...
    return ESP_OK;
}
```

---

## 9. EXTENSIBILITY

### Future Temporal Modes (v1.2+)

**Reserved Mode IDs:**
```c
typedef enum {
    // v1.1 modes (0x00-0x05)
    TEMPORAL_SYNC          = 0x00,
    TEMPORAL_OFFSET        = 0x01,
    TEMPORAL_PROGRESSIVE   = 0x02,
    TEMPORAL_WAVE          = 0x03,
    TEMPORAL_CUSTOM        = 0x04,
    TEMPORAL_MIRROR        = 0x05,

    // v1.2 future modes (0x06-0x0F)
    TEMPORAL_BOUNCE        = 0x06,  // Bouncing delay (triangle wave)
    TEMPORAL_RANDOM        = 0x07,  // Random per-LED delays
    TEMPORAL_RIPPLE        = 0x08,  // Radial ripple from point
    TEMPORAL_SPIRAL        = 0x09,  // Spiral delay pattern

    // v1.3+ advanced modes (0x10-0xFF)
    TEMPORAL_MULTI_SEGMENT = 0x10,  // Different delays per segment
    TEMPORAL_MODULATED     = 0x11,  // Delay that changes over time

    // Reserved
    TEMPORAL_RESERVED_MAX  = 0xFF
} temporal_mode_t;
```

### Per-Effect Temporal Overrides

```c
/**
 * Future extension: Per-effect temporal parameters
 *
 * Allows effect to override pattern-level temporal settings
 */
typedef struct {
    bool override_temporal;          // Effect wants custom temporal
    temporal_mode_t override_mode;   // Effect-specific mode
    temporal_params_union_t params;  // Effect-specific params
} effect_temporal_override_t;

// Example: Fire effect forces TEMPORAL_OFFSET for "rising flames"
effect_temporal_override_t fire_override = {
    .override_temporal = true,
    .override_mode = TEMPORAL_OFFSET,
    .params.offset.delay_ms = 100  // Always rise 100ms
};
```

### Temporal Modulation (Time-Varying Delays)

```c
/**
 * Future v1.3: Delays that change over time
 *
 * Creates dynamic shape morphing
 */
typedef struct {
    temporal_mode_t base_mode;       // Base delay pattern
    uint16_t modulation_period_ms;   // How fast delays change
    uint8_t  modulation_depth;       // How much delays vary (0-255)
} temporal_modulated_t;

// Example: Diamond → Triangle → Diamond morphing
temporal_modulated_t morph = {
    .base_mode = TEMPORAL_PROGRESSIVE,
    .modulation_period_ms = 2000,    // 2 second cycle
    .modulation_depth = 200          // Large variation
};
```

### Multi-Segment LGP Support

```c
/**
 * Future v1.4: Multi-segment LGP (4+ LED strips)
 *
 * Supports complex 3D structures
 */
typedef struct {
    uint8_t segment_count;           // Number of segments (2-8)
    uint16_t delays_per_segment[8];  // Independent delays
    temporal_mode_t segment_modes[8];// Per-segment temporal modes
} temporal_multi_segment_t;

// Example: 4-sided cube
temporal_multi_segment_t cube = {
    .segment_count = 4,
    .delays_per_segment = {0, 100, 200, 300},  // Ripple around cube
    .segment_modes = {
        TEMPORAL_PROGRESSIVE,  // Side 1
        TEMPORAL_PROGRESSIVE,  // Side 2
        TEMPORAL_PROGRESSIVE,  // Side 3
        TEMPORAL_PROGRESSIVE   // Side 4
    }
};
```

---

## 10. EXAMPLE PATTERNS

### Example 1: Rising Fire

```c
prism_header_t rising_fire = {
    .magic = PRISM_MAGIC,
    .version_major = 1,
    .version_minor = 1,
    .effect_id = PRISM_EFFECT_FIRE,
    .channel_mode = CHANNEL_MODE_MIRROR,
    .temporal_mode = TEMPORAL_OFFSET,
    .temporal_params.offset.delay_ms = 150,
    .brightness = 255,
    .speed = 160,
    .params = {
        .heat = 200,
        .cooling = 80,
        .sparking = 120
    }
};
```

**Visual:** Fire starts at bottom, rises to top 150ms later

**Pattern File Size:** 64 bytes (header) + 32 bytes (palette) = **96 bytes**

---

### Example 2: Diagonal Rainbow Sweep

```c
prism_header_t diagonal_rainbow = {
    .magic = PRISM_MAGIC,
    .version_major = 1,
    .version_minor = 1,
    .effect_id = PRISM_EFFECT_RAINBOW,
    .channel_mode = CHANNEL_MODE_MIRROR,
    .temporal_mode = TEMPORAL_PROGRESSIVE,
    .temporal_params.progressive = {
        .start_delay_ms = 0,
        .end_delay_ms = 300
    },
    .brightness = 192,
    .speed = 128,
    .params = {
        .saturation = 255,
        .density = 64
    }
};
```

**Visual:** Rainbow sweeps diagonally from bottom-left to top-right

**Pattern File Size:** 64 bytes (header only) = **64 bytes**

---

### Example 3: Vertical Sine Wave

```c
prism_header_t vertical_wave = {
    .magic = PRISM_MAGIC,
    .version_major = 1,
    .version_minor = 1,
    .effect_id = PRISM_EFFECT_WAVE,
    .channel_mode = CHANNEL_MODE_MIRROR,
    .temporal_mode = TEMPORAL_WAVE,
    .temporal_params.wave = {
        .period_ms = 400,
        .amplitude_ms = 150,
        .phase_offset = 0
    },
    .brightness = 200,
    .speed = 128,
    .params = {
        .wavelength = 64,
        .color1_r = 0, .color1_g = 255, .color1_b = 255,  // Cyan
        .color2_r = 0, .color2_g = 0, .color2_b = 255     // Blue
    }
};
```

**Visual:** Smooth sine wave vertical motion

**Pattern File Size:** 64 bytes (header only) = **64 bytes**

---

### Example 4: Custom Diamond

```c
prism_header_t custom_diamond = {
    .magic = PRISM_MAGIC,
    .version_major = 1,
    .version_minor = 1,
    .effect_id = PRISM_EFFECT_GRADIENT,
    .channel_mode = CHANNEL_MODE_MIRROR,
    .temporal_mode = TEMPORAL_CUSTOM,
    .temporal_params.custom = {
        .delay_map_offset = 128,  // After header+palette
        .delay_map_count = 8
    },
    .brightness = 220,
    .params = {
        .color1_r = 255, .color1_g = 0, .color1_b = 0,    // Red
        .color2_r = 0, .color2_g = 0, .color2_b = 255     // Blue
    }
};

// Delay map (diamond shape):
uint16_t diamond_delays[8] = {
    200,  // LED 0:   Edges
    150,  // LED 20
    100,  // LED 40
    50,   // LED 60
    0,    // LED 80:  Center
    50,   // LED 100
    100,  // LED 120
    200   // LED 159: Edges
};
```

**Visual:** Diamond blooms from center

**Pattern File Size:** 64 (header) + 64 (palette) + 16 (delay map) = **144 bytes**

---

## 11. SUMMARY & RECOMMENDATIONS

### Architecture Decisions

1. **Three-Tier System:**
   - Tier 1 (SYNC/OFFSET): 80% use cases, minimal overhead
   - Tier 2 (PROGRESSIVE/WAVE): 15% use cases, moderate overhead
   - Tier 3 (CUSTOM): 5% use cases, maximum control

2. **Format Efficiency:**
   - All modes fit in 64-byte header (except CUSTOM delay maps)
   - Backward compatible with v1.0 patterns
   - Extensible to v1.2+ modes

3. **Performance:**
   - All modes stay within 3.38ms CPU budget
   - Worst case (complex effect + TEMPORAL_WAVE) = 36.7% of budget
   - 172% safety margin for future optimizations

4. **Artistic Control:**
   - Simple: Rising/falling effects (OFFSET)
   - Advanced: Diagonal sweeps, triangles (PROGRESSIVE)
   - Expert: Custom shapes, complex choreography (CUSTOM)

### Default Configuration

```c
// Recommended default for new patterns:
prism_header_t default_pattern = {
    .temporal_mode = TEMPORAL_SYNC,        // No delay (simple)
    .channel_mode = CHANNEL_MODE_MIRROR,   // Unified surface
    // ... rest of header
};
```

### Next Steps

1. **Captain Review:** Approve this architecture
2. **Create ADR:** Document as ADR-010 "Temporal Sequencing Architecture"
3. **Update CANON:** Add temporal sequencing specifications
4. **Implementation Tasks:**
   - Phase 1: Implement TEMPORAL_SYNC and TEMPORAL_OFFSET
   - Phase 2: Implement TEMPORAL_PROGRESSIVE and TEMPORAL_WAVE
   - Phase 3: Implement TEMPORAL_CUSTOM
   - Phase 4: Create example patterns for each mode
   - Phase 5: Hardware validation (24-hour soak test)

### Validation Criteria

- ✅ Tier 1 modes fit in 3 bytes overhead
- ✅ Tier 2 modes fit in 6 bytes overhead
- ✅ All modes stay within 3.38ms CPU budget
- ✅ Backward compatible with v1.0 patterns
- ✅ Extensible to future modes (v1.2+)
- ⏳ Hardware validation pending
- ⏳ User testing pending

---

**END OF DOCUMENT**

This temporal sequencing architecture enables Captain SpectraSynq's vision: "TEMPORAL SEQUENCING is the magic. If we can figure out how to stagger/trail the LED Motion, we can explore shape creation via the LGP. We're building a platform for artistic exploration and expression."
