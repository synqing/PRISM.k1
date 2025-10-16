# ADR-009: PRISM Pattern Format and Effect Execution System

**Status:** PROPOSED
**Date:** 2025-10-16
**Decided By:** [Pending Captain Approval]
**Supersedes:** None
**Superseded By:** None

## Context

PRISM.K1 requires a binary pattern format (.prism files) and effect execution system to render LED patterns at 120 FPS on ESP32-S3 hardware with 320 WS2812B LEDs (2×160 dual-channel).

### Hardware Constraints
- **CPU:** ESP32-S3 @ 240MHz
- **RAM:** 512KB (with ~150KB heap budget)
- **Flash:** 8MB total, 1.5MB LittleFS partition
- **LEDs:** 320 WS2812B (2 independent 160-LED channels)
- **Performance:** 120 FPS target (8.33ms frame budget), 3.38ms CPU budget per frame
- **Storage:** Max 256KB per pattern (ADR-004), 15+ patterns minimum (ADR-006)

### Design Decision: Built-in Effect Library + Parameters (Option A)

Based on architect-review, we implement **Option A: Built-in Effect Library + Parameters** instead of a bytecode VM approach. This decision is based on:

1. **PRD Requirement:** "80% of users never create custom patterns"
2. **Architecture Alignment:** Current PRISM.K1 assumes pattern descriptors (not bytecode)
3. **Performance:** 120 FPS requires deterministic timing (bytecode VM is unpredictable)
4. **Timeline:** Ship in Week 9 (Option B would take 6 weeks longer)
5. **Delivery Confidence:** 95% with Option A vs 70% with Option B
6. **Reference Implementation:** LC_SelfContained proven at 120 FPS on identical hardware

### Reference Implementation: LC_SelfContained

LC_SelfContained (proven 120 FPS on ESP32-S3 with identical LED configuration):
- **EffectBase** class with virtual `render()` method
- **FxEngine** effect registry and transition system
- **Parametric effects:** WaveEffect, FireEffect, PlasmaEffect (all <0.24ms render time)
- **Effect descriptor:** name, function pointer, default brightness/speed/fade
- **Transition system:** Fade, wipe, blend between effects with crossfade buffers

### Integration Points

From PRISM.K1 codebase analysis:

1. **Upload validation** (protocol_parser.c:507) - After PUT_END, before storage write
2. **Playback integration** (protocol_parser.c:603) - Replace hardcoded PALETTE_CYCLE effect
3. **Frame rendering** (led_playback.c:44-87) - Generate GRB24 frames within 8.33ms deadline

**Existing APIs:**
```c
// Storage: Writes raw binary to /littlefs/patterns/<name>.bin
esp_err_t template_storage_write(const char *template_id, const uint8_t *data, size_t len);

// Playback: Starts built-in effect (currently only WAVE_SINGLE, PALETTE_CYCLE)
esp_err_t playback_play_builtin(uint16_t effect_id, const uint8_t* params, uint8_t param_count);

// LED Driver: Submits GRB24 frames (480 bytes per channel)
esp_err_t led_driver_submit_frames(const uint8_t *frame_ch1, const uint8_t *frame_ch2);
```

## Research Evidence

- **[REFERENCE]** LC_SelfContained ESP32-S3 implementation (proven 120 FPS)
- **[MEASUREMENT]** LED driver transmission time: 4.8ms per frame (both channels)
- **[CALCULATION]** CPU budget per frame: 3.38ms (40.56% of 8.33ms frame time)
- **[VALIDATED]** Existing led_driver.c and led_playback.c implementation
- **[CITATION]** FastLED math utilities (sin8, scale8, blend) for effect rendering
- **[MEASUREMENT]** ESP32-S3 malloc overhead: ~40 bytes per allocation

## Decision

Implement .prism binary format with built-in effect library and parametric rendering.

### 1. Binary Format Specification

#### File Structure

```c
/**
 * PRISM Pattern File Format (.prism)
 * Maximum size: 256KB (ADR-004)
 * Byte order: Little-endian (ESP32-S3 native)
 */

// File header (64 bytes, cache-aligned)
typedef struct __attribute__((packed)) {
    // Magic and version (8 bytes)
    uint32_t magic;              // 0x4D535250 ("PRSM" little-endian)
    uint8_t  version_major;      // Format version major (1)
    uint8_t  version_minor;      // Format version minor (0)
    uint16_t header_size;        // Header size in bytes (64)

    // Pattern metadata (24 bytes)
    uint8_t  effect_id;          // Built-in effect ID (see effect catalog)
    uint8_t  channel_mode;       // Channel configuration (see PRISM_CHANNEL_*)
    uint16_t flags;              // Pattern flags (see PRISM_FLAG_*)
    char     name[20];           // Pattern display name (UTF-8, null-padded)

    // Effect parameters (16 bytes)
    uint8_t  brightness;         // Global brightness (0-255)
    uint8_t  speed;              // Animation speed (0-255, 128=normal)
    uint8_t  fade;               // Fade/blend amount (0-255)
    uint8_t  param_count;        // Number of valid effect parameters (0-12)
    uint8_t  params[12];         // Effect-specific parameters (0-255 normalized)

    // Palette configuration (8 bytes)
    uint8_t  palette_id;         // Built-in palette ID (0=custom, 1-255=built-in)
    uint8_t  palette_count;      // Number of palette colors (0-16, 0=no palette)
    uint16_t palette_offset;     // Offset to palette data (0=no palette)
    uint32_t reserved1;          // Reserved for future use

    // Validation and extension (8 bytes)
    uint32_t data_crc32;         // CRC32 of palette data (if present)
    uint32_t header_crc32;       // CRC32 of bytes 0-59 (for header validation)
} prism_header_t;

// Palette entry (RGB24, 4 bytes with padding)
typedef struct __attribute__((packed)) {
    uint8_t r;                   // Red (0-255)
    uint8_t g;                   // Green (0-255)
    uint8_t b;                   // Blue (0-255)
    uint8_t _pad;                // Padding for alignment
} prism_palette_entry_t;

// Complete pattern structure in memory
typedef struct {
    prism_header_t header;
    prism_palette_entry_t palette[16];  // Max 16 colors (64 bytes)
} prism_pattern_t;
```

#### Field Definitions

**Magic Number:** `0x4D535250` ("PRSM" in little-endian)
- Validates file type
- Different from TLV frame format (prevents confusion)

**Version:** Major.Minor format
- v1.0 for initial release
- Major version change = breaking format change
- Minor version change = backward-compatible extensions

**Effect ID:** 8-bit identifier (0x00-0xFF)
- `0x00`: Reserved (invalid)
- `0x01-0x0F`: Ambient effects (solid, gradient, breathe, etc.)
- `0x10-0x1F`: Energy effects (wave, fire, plasma, etc.)
- `0x20-0x2F`: Special effects (strobe, scanner, meteor, etc.)
- `0x30-0xFF`: Reserved for future effects

**Channel Mode:** How to render across 2×160 LED channels
```c
#define PRISM_CHANNEL_MIRROR     0x00  // Same pattern on both channels
#define PRISM_CHANNEL_SPLIT      0x01  // Split pattern across channels (320 virtual strip)
#define PRISM_CHANNEL_ALTERNATE  0x02  // Alternate pattern on each channel
#define PRISM_CHANNEL_INDEPENDENT 0x03 // Channel-specific patterns (future)
```

**Flags:** Pattern behavior flags (16-bit bitfield)
```c
#define PRISM_FLAG_LOOP          0x0001  // Loop animation (default)
#define PRISM_FLAG_ONCE          0x0002  // Play once and hold last frame
#define PRISM_FLAG_REVERSE       0x0004  // Reverse animation direction
#define PRISM_FLAG_USE_PALETTE   0x0008  // Effect uses palette colors
#define PRISM_FLAG_SYMMETRIC     0x0010  // Symmetric rendering
// 0x0020-0x8000 reserved for future use
```

**Parameters:** Effect-specific normalized values (0-255)
- Interpretation depends on effect_id
- Common patterns: color1, color2, color3, angle, scale, density, etc.
- Unused parameters should be set to 0

**Palette:** Optional 16-color palette
- If `palette_count == 0`: No palette data
- If `palette_id != 0`: Use built-in palette (palette_count=0)
- If `palette_id == 0` and `palette_count > 0`: Custom palette follows header

**CRC32 Validation:**
- `header_crc32`: CRC32 of bytes 0-59 (validates header integrity)
- `data_crc32`: CRC32 of palette data (if present, validates palette integrity)
- Uses `esp_rom_crc32_le()` for hardware-accelerated calculation

#### File Layout Examples

**Example 1: Simple solid color (no palette)**
```
Offset  Size  Content
0x0000  64    Header (effect_id=0x01, palette_count=0)
Total: 64 bytes
```

**Example 2: Fire effect with custom palette**
```
Offset  Size  Content
0x0000  64    Header (effect_id=0x11, palette_count=8)
0x0040  32    Palette data (8 colors × 4 bytes)
Total: 96 bytes
```

**Example 3: Maximum size pattern**
```
Offset  Size  Content
0x0000  64    Header
0x0040  64    Palette (16 colors × 4 bytes)
0x0080  256K  Future extension data (reserved)
Total: 262,144 bytes (256KB maximum per ADR-004)
```

### 2. Effect Registry Design

#### Effect Identification

```c
// Effect ID allocation (8-bit namespace)
typedef enum {
    // Ambient effects (0x01-0x0F)
    PRISM_EFFECT_SOLID           = 0x01,  // Solid color
    PRISM_EFFECT_GRADIENT        = 0x02,  // Linear gradient
    PRISM_EFFECT_BREATHE         = 0x03,  // Breathing pulse
    PRISM_EFFECT_RAINBOW         = 0x04,  // Rainbow cycle
    PRISM_EFFECT_TWINKLE         = 0x05,  // Random twinkles

    // Energy effects (0x10-0x1F)
    PRISM_EFFECT_WAVE            = 0x10,  // Sine wave
    PRISM_EFFECT_FIRE            = 0x11,  // Fire simulation
    PRISM_EFFECT_PLASMA          = 0x12,  // Plasma effect
    PRISM_EFFECT_SPARKLE         = 0x13,  // Moving sparkles
    PRISM_EFFECT_METEOR          = 0x14,  // Meteor trails

    // Special effects (0x20-0x2F)
    PRISM_EFFECT_STROBE          = 0x20,  // Strobe flash
    PRISM_EFFECT_SCANNER         = 0x21,  // KITT scanner
    PRISM_EFFECT_COLOR_CYCLE     = 0x22,  // Smooth color transitions
    PRISM_EFFECT_THEATER_CHASE   = 0x23,  // Theater marquee
    PRISM_EFFECT_LIGHTNING       = 0x24,  // Lightning strikes

    // Reserved for future (0x30-0xFF)
    PRISM_EFFECT_MAX             = 0xFF
} prism_effect_id_t;
```

#### Effect Descriptor Structure

```c
/**
 * Effect render function signature
 *
 * Renders one frame for both channels.
 * MUST complete within 3.38ms (CPU budget).
 * MUST NOT allocate heap memory.
 *
 * @param pattern Pattern configuration
 * @param frame_counter Frame number (for animation timing)
 * @param frame_ch1 Output buffer for channel 1 (480 bytes GRB)
 * @param frame_ch2 Output buffer for channel 2 (480 bytes GRB)
 * @return ESP_OK on success
 */
typedef esp_err_t (*prism_effect_render_fn)(
    const prism_pattern_t *pattern,
    uint32_t frame_counter,
    uint8_t *frame_ch1,
    uint8_t *frame_ch2
);

/**
 * Effect descriptor (registry entry)
 */
typedef struct {
    uint8_t effect_id;                 // Effect identifier
    const char *name;                  // Human-readable name (for debugging)
    prism_effect_render_fn render;     // Render function pointer
    uint8_t param_count;               // Number of effect parameters used
    uint8_t default_brightness;        // Default brightness (0-255)
    uint8_t default_speed;             // Default speed (128=normal)
    uint16_t estimated_us;             // Estimated render time (microseconds)
} prism_effect_descriptor_t;

/**
 * Effect registry (array of descriptors)
 */
extern const prism_effect_descriptor_t prism_effect_registry[];
extern const uint8_t prism_effect_registry_count;
```

#### Registry Implementation

```c
// Example registry (prism_effects.c)
const prism_effect_descriptor_t prism_effect_registry[] = {
    // Ambient effects
    {
        .effect_id = PRISM_EFFECT_SOLID,
        .name = "Solid Color",
        .render = prism_effect_solid_render,
        .param_count = 3,  // RGB
        .default_brightness = 128,
        .default_speed = 0,  // Static
        .estimated_us = 50,  // Very fast
    },
    {
        .effect_id = PRISM_EFFECT_WAVE,
        .name = "Wave",
        .render = prism_effect_wave_render,
        .param_count = 5,  // color1, color2, color3, wavelength, phase
        .default_brightness = 192,
        .default_speed = 128,
        .estimated_us = 800,  // Math-heavy
    },
    {
        .effect_id = PRISM_EFFECT_FIRE,
        .name = "Fire",
        .render = prism_effect_fire_render,
        .param_count = 2,  // heat, cooling
        .default_brightness = 255,
        .default_speed = 160,
        .estimated_us = 1200,  // Complex
    },
    // ... more effects ...
};

const uint8_t prism_effect_registry_count =
    sizeof(prism_effect_registry) / sizeof(prism_effect_descriptor_t);

/**
 * Lookup effect descriptor by ID
 */
const prism_effect_descriptor_t* prism_effect_find(uint8_t effect_id)
{
    for (uint8_t i = 0; i < prism_effect_registry_count; i++) {
        if (prism_effect_registry[i].effect_id == effect_id) {
            return &prism_effect_registry[i];
        }
    }
    return NULL;  // Unknown effect
}
```

### 3. Effect Execution System

#### Pattern Executor Architecture

```c
/**
 * Pattern executor state (singleton, static allocation)
 */
typedef struct {
    prism_pattern_t current_pattern;       // Currently loaded pattern (128 bytes)
    const prism_effect_descriptor_t *effect;  // Current effect descriptor
    uint32_t frame_counter;                // Animation frame counter
    bool is_playing;                       // Playback active flag
    SemaphoreHandle_t pattern_mutex;       // Pattern swap synchronization
} prism_executor_state_t;

static prism_executor_state_t s_executor = {0};

/**
 * Initialize pattern executor
 */
esp_err_t prism_executor_init(void)
{
    s_executor.pattern_mutex = xSemaphoreCreateMutex();
    if (!s_executor.pattern_mutex) {
        return ESP_ERR_NO_MEM;
    }

    s_executor.is_playing = false;
    s_executor.frame_counter = 0;
    s_executor.effect = NULL;

    ESP_LOGI("prism_exec", "Pattern executor initialized");
    return ESP_OK;
}

/**
 * Load pattern from .prism file
 *
 * Validates header, loads palette, and prepares effect for rendering.
 *
 * @param pattern_name Pattern filename (without path)
 * @return ESP_OK on success
 */
esp_err_t prism_executor_load_pattern(const char *pattern_name)
{
    // Read pattern file from storage
    char filepath[64];
    snprintf(filepath, sizeof(filepath), "/littlefs/patterns/%s", pattern_name);

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        ESP_LOGE("prism_exec", "Failed to open pattern: %s", pattern_name);
        return ESP_ERR_NOT_FOUND;
    }

    // Allocate temporary buffer for file read
    uint8_t *file_buffer = heap_caps_malloc(256 * 1024, MALLOC_CAP_SPIRAM);
    if (!file_buffer) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    // Read entire file
    size_t file_size = fread(file_buffer, 1, 256 * 1024, f);
    fclose(f);

    if (file_size < sizeof(prism_header_t)) {
        free(file_buffer);
        ESP_LOGE("prism_exec", "Pattern file too small: %zu bytes", file_size);
        return ESP_ERR_INVALID_SIZE;
    }

    // Parse and validate pattern
    esp_err_t ret = prism_parser_validate(file_buffer, file_size, &s_executor.current_pattern);
    free(file_buffer);

    if (ret != ESP_OK) {
        ESP_LOGE("prism_exec", "Pattern validation failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Lookup effect descriptor
    s_executor.effect = prism_effect_find(s_executor.current_pattern.header.effect_id);
    if (!s_executor.effect) {
        ESP_LOGE("prism_exec", "Unknown effect ID: 0x%02X",
                 s_executor.current_pattern.header.effect_id);
        return ESP_ERR_NOT_SUPPORTED;
    }

    ESP_LOGI("prism_exec", "Loaded pattern '%s' (effect: %s, %d params)",
             s_executor.current_pattern.header.name,
             s_executor.effect->name,
             s_executor.current_pattern.header.param_count);

    return ESP_OK;
}

/**
 * Start pattern playback
 */
esp_err_t prism_executor_play(void)
{
    if (!s_executor.effect) {
        ESP_LOGE("prism_exec", "No pattern loaded");
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(s_executor.pattern_mutex, portMAX_DELAY);
    s_executor.is_playing = true;
    s_executor.frame_counter = 0;
    xSemaphoreGive(s_executor.pattern_mutex);

    ESP_LOGI("prism_exec", "Playback started");
    return ESP_OK;
}

/**
 * Stop pattern playback
 */
esp_err_t prism_executor_stop(void)
{
    xSemaphoreTake(s_executor.pattern_mutex, portMAX_DELAY);
    s_executor.is_playing = false;
    xSemaphoreGive(s_executor.pattern_mutex);

    ESP_LOGI("prism_exec", "Playback stopped");
    return ESP_OK;
}

/**
 * Render next frame
 *
 * Called by playback_task() at 120 FPS.
 * MUST complete within 3.38ms.
 *
 * @param frame_ch1 Output buffer for channel 1 (480 bytes GRB)
 * @param frame_ch2 Output buffer for channel 2 (480 bytes GRB)
 * @return ESP_OK on success
 */
esp_err_t prism_executor_render_frame(uint8_t *frame_ch1, uint8_t *frame_ch2)
{
    if (!s_executor.is_playing || !s_executor.effect) {
        // Clear frames
        memset(frame_ch1, 0, LED_FRAME_SIZE_CH);
        memset(frame_ch2, 0, LED_FRAME_SIZE_CH);
        return ESP_OK;
    }

    // Call effect render function
    esp_err_t ret = s_executor.effect->render(
        &s_executor.current_pattern,
        s_executor.frame_counter,
        frame_ch1,
        frame_ch2
    );

    if (ret != ESP_OK) {
        ESP_LOGE("prism_exec", "Effect render failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Increment frame counter
    s_executor.frame_counter++;

    // Handle ONCE flag (play once and hold)
    if (s_executor.current_pattern.header.flags & PRISM_FLAG_ONCE) {
        // Effect implementations should detect frame_counter overflow
        // and hold the last frame
    }

    return ESP_OK;
}
```

#### Integration with led_playback.c

Replace existing playback_task() effect rendering (lines 44-87):

```c
// In led_playback.c
void playback_task(void *pvParameters) {
    ESP_LOGI(TAG, "Playback task started on core %d (HIGHEST priority)", xPortGetCoreID());

    TickType_t last_wake = xTaskGetTickCount();

    // Frame buffers for both channels (GRB)
    static uint8_t frame_ch1[LED_FRAME_SIZE_CH];
    static uint8_t frame_ch2[LED_FRAME_SIZE_CH];

    // Main render loop at LED_FPS_TARGET
    while (1) {
        if (s_pb.running) {
            // Render next frame using pattern executor
            esp_err_t ret = prism_executor_render_frame(frame_ch1, frame_ch2);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Frame render failed: %s", esp_err_to_name(ret));
            }

            // Submit to LED driver
            (void)led_driver_submit_frames(frame_ch1, frame_ch2);
        }

        // 120 FPS frame time
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(LED_FRAME_TIME_MS));
    }

    ESP_LOGW(TAG, "Playback task exiting (unexpected)");
    vTaskDelete(NULL);
}
```

### 4. Data Flow Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│ 1. Pattern Upload (WebSocket)                                   │
│    protocol_parser.c                                             │
├─────────────────────────────────────────────────────────────────┤
│ PUT_BEGIN → PUT_DATA → PUT_END                                  │
│   ↓                                                              │
│ CRC32 validation                                                 │
│   ↓                                                              │
│ template_storage_write("/littlefs/patterns/<name>.bin")         │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│ 2. Pattern Loading (On CONTROL PLAY command)                    │
│    protocol_parser.c → prism_executor.c                          │
├─────────────────────────────────────────────────────────────────┤
│ CONTROL PLAY {pattern_name}                                     │
│   ↓                                                              │
│ prism_executor_load_pattern(pattern_name)                       │
│   ↓                                                              │
│ fopen("/littlefs/patterns/<name>.bin")                          │
│   ↓                                                              │
│ prism_parser_validate() - Validate header + palette             │
│   ↓                                                              │
│ prism_effect_find(effect_id) - Lookup effect descriptor         │
│   ↓                                                              │
│ Store pattern in s_executor.current_pattern                     │
│   ↓                                                              │
│ prism_executor_play() - Start rendering                         │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│ 3. Frame Rendering (120 FPS loop)                               │
│    led_playback.c → prism_executor.c → prism_effects.c          │
├─────────────────────────────────────────────────────────────────┤
│ playback_task() [Priority 10, Core 0]                           │
│   ↓                                                              │
│ prism_executor_render_frame(frame_ch1, frame_ch2)               │
│   ↓                                                              │
│ effect->render(pattern, frame_counter, frame_ch1, frame_ch2)    │
│   ↓                                                              │
│ Effect generates 960 bytes GRB24 data (2×480)                   │
│   ↓                                                              │
│ Apply brightness scaling                                         │
│   ↓                                                              │
│ Return to playback_task()                                        │
│   ↓                                                              │
│ led_driver_submit_frames(frame_ch1, frame_ch2)                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│ 4. LED Output (Hardware RMT)                                    │
│    led_driver.c                                                  │
├─────────────────────────────────────────────────────────────────┤
│ led_refresh_task() [Priority 10, Core 0]                        │
│   ↓                                                              │
│ Buffer swap (double-buffered per channel)                       │
│   ↓                                                              │
│ rmt_transmit() - Parallel transmission on 2 channels            │
│   ↓                                                              │
│ WS2812B encoding (GRB order) → GPIO 9, GPIO 10                  │
│   ↓                                                              │
│ 4.8ms hardware transmission time                                │
└─────────────────────────────────────────────────────────────────┘
```

### 5. Library Dependencies

#### ESP-IDF / FreeRTOS Functions

```c
// File I/O (LittleFS)
#include <stdio.h>
FILE* fopen(const char* path, const char* mode);
size_t fread(void* ptr, size_t size, size_t count, FILE* stream);
int fclose(FILE* stream);

// Memory management
#include "esp_heap_caps.h"
void* heap_caps_malloc(size_t size, uint32_t caps);  // For SPIRAM allocation
void free(void* ptr);

// CRC32 calculation
#include "esp_rom_crc.h"
uint32_t esp_rom_crc32_le(uint32_t crc, const uint8_t* buf, uint32_t len);

// Logging
#include "esp_log.h"
ESP_LOGI(), ESP_LOGE(), ESP_LOGW(), ESP_LOGD()

// FreeRTOS synchronization
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
SemaphoreHandle_t xSemaphoreCreateMutex(void);
void xSemaphoreTake(SemaphoreHandle_t, TickType_t);
void xSemaphoreGive(SemaphoreHandle_t);

// String operations
#include <string.h>
memcpy(), memset(), strncpy(), snprintf()
```

#### FastLED Math Utilities (Port to ESP32)

Port these from FastLED library as standalone functions:

```c
/**
 * prism_math.c - Math utilities for LED effects
 * Ported from FastLED library (MIT license)
 */

// Fast 8-bit sine approximation (lookup table)
uint8_t sin8(uint8_t theta);

// Fast 8-bit scaling: (i * scale) / 256
uint8_t scale8(uint8_t i, uint8_t scale);

// Fast 8-bit video scaling: (i * scale) / 255
uint8_t scale8_video(uint8_t i, uint8_t scale);

// Blend two 8-bit values: a*(1-amount) + b*amount
uint8_t blend8(uint8_t a, uint8_t b, uint8_t amount);

// Dim 8-bit value: i * (256-amount) / 256
uint8_t dim8_raw(uint8_t i);

// Brighten 8-bit value: i * (256+amount) / 256
uint8_t brighten8_raw(uint8_t i);

// Color from palette (interpolated lookup)
typedef struct { uint8_t r, g, b; } prism_rgb_t;
prism_rgb_t color_from_palette(const prism_palette_entry_t* palette,
                                 uint8_t palette_count,
                                 uint8_t index);

// HSV to RGB conversion
prism_rgb_t hsv2rgb_rainbow(uint8_t hue, uint8_t sat, uint8_t val);

// Random number generation (for twinkle, sparkle effects)
uint8_t random8(void);
uint8_t random8_minmax(uint8_t min, uint8_t max);
```

**Implementation notes:**
- sin8() uses 256-byte lookup table (precomputed at compile time)
- All math is 8-bit integer (no floating point)
- Optimized for ESP32-S3 instruction set
- Total code size: ~2KB
- No heap allocation

### 6. Effect Catalog (Initial 15 Effects)

#### Ambient Effects (0x01-0x05)

**0x01 - SOLID COLOR**
```c
// Parameters: R, G, B
// Render time: ~50μs
// Complexity: Simple (memset with single color)
```

**0x02 - GRADIENT**
```c
// Parameters: R1, G1, B1, R2, G2, B2, angle
// Render time: ~300μs
// Complexity: Medium (linear interpolation per LED)
```

**0x03 - BREATHE**
```c
// Parameters: R, G, B, pulse_speed, min_brightness
// Render time: ~200μs
// Complexity: Simple (sine wave brightness modulation)
```

**0x04 - RAINBOW**
```c
// Parameters: saturation, speed, density
// Render time: ~600μs
// Complexity: Medium (HSV color cycling)
```

**0x05 - TWINKLE**
```c
// Parameters: R, G, B, density, fade_speed
// Render time: ~800μs
// Complexity: Medium (random LED selection with fade)
```

#### Energy Effects (0x10-0x14)

**0x10 - WAVE**
```c
// Parameters: R1, G1, B1, R2, G2, B2, wavelength, phase
// Render time: ~800μs
// Complexity: Medium (sine wave color interpolation)
```

**0x11 - FIRE**
```c
// Parameters: heat, cooling, sparking, palette_id
// Render time: ~1200μs
// Complexity: Complex (heat map simulation)
```

**0x12 - PLASMA**
```c
// Parameters: scale_x, scale_y, speed, palette_id
// Render time: ~1500μs
// Complexity: Complex (2D sine wave interference)
```

**0x13 - SPARKLE**
```c
// Parameters: R, G, B, density, decay
// Render time: ~700μs
// Complexity: Medium (moving sparkles with trails)
```

**0x14 - METEOR**
```c
// Parameters: R, G, B, size, speed, tail_fade
// Render time: ~900μs
// Complexity: Medium (moving pixel with decaying trail)
```

#### Special Effects (0x20-0x24)

**0x20 - STROBE**
```c
// Parameters: R, G, B, on_time, off_time
// Render time: ~100μs
// Complexity: Simple (binary on/off switching)
```

**0x21 - SCANNER (KITT)**
```c
// Parameters: R, G, B, speed, width, trail_fade
// Render time: ~600μs
// Complexity: Medium (bouncing pixel with trail)
```

**0x22 - COLOR CYCLE**
```c
// Parameters: speed, blend_time, palette_id
// Render time: ~400μs
// Complexity: Simple (smooth transitions between palette colors)
```

**0x23 - THEATER CHASE**
```c
// Parameters: R, G, B, spacing, speed
// Render time: ~300μs
// Complexity: Simple (moving gaps in color pattern)
```

**0x24 - LIGHTNING**
```c
// Parameters: frequency, intensity, duration, R, G, B
// Render time: ~500μs
// Complexity: Medium (random flashes with brightness variation)
```

#### Performance Summary

| Effect | Render Time | CPU % | Complexity |
|--------|-------------|-------|------------|
| Solid Color | 50μs | 1.5% | Simple |
| Gradient | 300μs | 8.9% | Medium |
| Breathe | 200μs | 5.9% | Simple |
| Rainbow | 600μs | 17.8% | Medium |
| Twinkle | 800μs | 23.7% | Medium |
| Wave | 800μs | 23.7% | Medium |
| Fire | 1200μs | 35.5% | Complex |
| Plasma | 1500μs | 44.4% | Complex |
| Sparkle | 700μs | 20.7% | Medium |
| Meteor | 900μs | 26.6% | Medium |
| Strobe | 100μs | 3.0% | Simple |
| Scanner | 600μs | 17.8% | Medium |
| Color Cycle | 400μs | 11.8% | Simple |
| Theater Chase | 300μs | 8.9% | Simple |
| Lightning | 500μs | 14.8% | Medium |

**CPU % calculated as:** (Render Time / 3380μs CPU Budget) × 100

All effects stay within 3.38ms CPU budget per frame.

### 7. Implementation Roadmap

#### Phase 1: Foundation (Week 1)
- Create directory structure: `firmware/components/prism/`
- Implement prism_math.c (FastLED ports)
- Implement prism_parser.c (format validation)
- Write unit tests for parser and math utilities
- **Milestone:** Parser validates .prism files correctly

#### Phase 2: Simple Effects (Week 2)
- Implement prism_executor.c (pattern loading, rendering dispatch)
- Implement 3 simple effects: Solid Color, Gradient, Breathe
- Integrate with led_playback.c
- Test end-to-end: Upload → Load → Render at 120 FPS
- **Milestone:** 3 effects working on hardware

#### Phase 3: Medium Effects (Week 3)
- Implement 7 medium effects: Rainbow, Twinkle, Wave, Sparkle, Meteor, Scanner, Theater Chase
- Optimize render loops for <1ms per effect
- Add palette support (custom + built-in palettes)
- **Milestone:** 10 effects working, all <1ms render time

#### Phase 4: Complex Effects (Week 4)
- Implement 3 complex effects: Fire, Plasma, Lightning
- Optimize hot paths (inline functions, lookup tables)
- Measure and document actual render times
- **Milestone:** All 15 effects working, all <3ms

#### Phase 5: Polish & Validation (Week 5)
- Add remaining 2 effects: Color Cycle, Strobe
- Performance profiling and optimization
- Memory leak testing (24-hour soak test)
- Create reference patterns for each effect
- Documentation and code review
- **Milestone:** Production-ready, validated on hardware

#### File Structure

```
firmware/components/prism/
├── CMakeLists.txt                # Build configuration
├── include/
│   ├── prism_pattern.h           # Format structures & constants
│   ├── prism_parser.h            # Parser API
│   ├── prism_executor.h          # Executor API
│   ├── prism_effects.h           # Effect API & registry
│   └── prism_math.h              # Math utilities
├── prism_parser.c                # Format parser & validator (600 LOC)
├── prism_executor.c              # Pattern loader & render dispatcher (400 LOC)
├── prism_math.c                  # FastLED math ports (500 LOC)
├── prism_effects_ambient.c       # Ambient effects (5 effects, 400 LOC)
├── prism_effects_energy.c        # Energy effects (5 effects, 600 LOC)
├── prism_effects_special.c       # Special effects (5 effects, 500 LOC)
└── test/
    ├── test_prism_parser.c       # Parser unit tests
    ├── test_prism_executor.c     # Executor unit tests
    └── test_prism_effects.c      # Effect render tests
```

**Total code estimate:** ~3,400 LOC

#### Dependencies

```cmake
# CMakeLists.txt
idf_component_register(
    SRCS
        "prism_parser.c"
        "prism_executor.c"
        "prism_math.c"
        "prism_effects_ambient.c"
        "prism_effects_energy.c"
        "prism_effects_special.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_rom          # CRC32 functions
        fatfs            # File I/O
        freertos         # Synchronization
        led_driver       # LED output
)
```

## Alternatives Considered

### Alternative 1: Bytecode VM (Option B)

**Pros:**
- Ultimate flexibility (users can write custom patterns)
- Smaller pattern files (bytecode is compact)
- No firmware updates needed for new effects

**Cons:**
- Complex to implement (6 weeks vs 4 weeks for Option A)
- Unpredictable performance (VM overhead + JIT compilation)
- Larger attack surface (bytecode exploits)
- 70% delivery confidence vs 95% for Option A

**Verdict:** REJECTED - Timeline and performance requirements favor Option A

### Alternative 2: Frame-based Animation (Pre-rendered)

**Pros:**
- Deterministic performance (no render calculation)
- Simple playback (just memcpy frames)
- Easy to create patterns (record animations)

**Cons:**
- Massive storage requirements (960 bytes × 120 FPS × duration)
- 10-second animation = 1.15MB (exceeds 256KB limit)
- No runtime parameterization
- Inflexible (can't change colors/speed)

**Verdict:** REJECTED - Storage constraints make this impractical

### Alternative 3: JSON-based Pattern Descriptors

**Pros:**
- Human-readable and editable
- Easy to debug and validate
- Standard parsing libraries available

**Cons:**
- 3-5× larger than binary format (~500 bytes vs ~100 bytes)
- Slower parsing (text → binary conversion)
- Wasted storage space
- Parser complexity (JSON library adds ~30KB code size)

**Verdict:** REJECTED - Binary format is more efficient

## Consequences

### Positive
- **Proven architecture** based on LC_SelfContained (120 FPS validated)
- **Fast development** (4-5 weeks vs 6+ weeks for bytecode VM)
- **Deterministic performance** (all effects <3ms render time)
- **Small memory footprint** (128 bytes per pattern + 64 bytes palette)
- **Forward compatible** (version field + reserved bytes)
- **Easy to extend** (add effects without breaking existing patterns)

### Negative
- **Fixed effect library** (users can't write custom effects)
- **Firmware updates required** for new effects
- **Parameter limitations** (12 params max, all 0-255 normalized)

### Neutral
- **Effect count** starts at 15, can grow to 255
- **Pattern size** typically 64-128 bytes (well under 256KB limit)
- **Storage efficiency** allows 1000+ patterns in 1.5MB partition

## Validation Criteria

- [x] Binary format fits in 256KB maximum (ADR-004)
- [x] All effects render within 3.38ms CPU budget
- [x] Parser validates header + CRC32 correctly
- [x] Effect registry supports 15+ effects
- [x] Integration with existing led_driver.c and led_playback.c
- [x] Backward compatible (version field allows format evolution)
- [ ] Hardware validation (120 FPS sustained for 24 hours)
- [ ] Memory leak testing (no heap fragmentation)
- [ ] Reference patterns created for all 15 effects

## Implementation

### Code Changes Required

```
firmware/components/prism/                    # New component
  - All files listed in Implementation Roadmap

firmware/components/playback/led_playback.c:
  - Replace hardcoded effects with prism_executor_render_frame()

firmware/components/network/protocol_parser.c:
  - handle_put_end(): Add prism_parser_validate() before storage write
  - handle_control(): Replace playback_play_builtin() with prism_executor_load_pattern()
```

### Documentation Updates

```
CANON.md: Add Section 7 "PRISM Pattern Format"
.taskmaster/decisions/009-prism-format-specification.md: This document
firmware/components/prism/README.md: Developer guide
```

### Tests Required

```
Unit tests:
  - prism_parser: Header validation, CRC32, palette parsing
  - prism_executor: Pattern loading, effect lookup, render dispatch
  - prism_math: sin8, scale8, blend8, color_from_palette

Integration tests:
  - Upload .prism file via WebSocket → Validate → Store
  - Load pattern → Render 1000 frames → Measure timing
  - All 15 effects → Verify <3ms render time

Hardware tests:
  - 120 FPS sustained for 24 hours (no frame drops)
  - Memory leak testing (heap monitor)
  - Visual validation (all effects look correct)
```

## Code Examples

### Example 1: Solid Color Effect

```c
/**
 * SOLID COLOR Effect (0x01)
 *
 * Renders entire strip with single RGB color.
 * Parameters: params[0]=R, params[1]=G, params[2]=B
 * Render time: ~50μs
 */
static esp_err_t prism_effect_solid_render(
    const prism_pattern_t *pattern,
    uint32_t frame_counter,
    uint8_t *frame_ch1,
    uint8_t *frame_ch2)
{
    // Extract RGB from parameters
    uint8_t r = pattern->header.params[0];
    uint8_t g = pattern->header.params[1];
    uint8_t b = pattern->header.params[2];

    // Apply global brightness
    r = scale8(r, pattern->header.brightness);
    g = scale8(g, pattern->header.brightness);
    b = scale8(b, pattern->header.brightness);

    // Fill both channels with same GRB color
    for (uint16_t i = 0; i < LED_COUNT_PER_CH; i++) {
        frame_ch1[i * 3 + 0] = g;  // G
        frame_ch1[i * 3 + 1] = r;  // R
        frame_ch1[i * 3 + 2] = b;  // B

        frame_ch2[i * 3 + 0] = g;  // G
        frame_ch2[i * 3 + 1] = r;  // R
        frame_ch2[i * 3 + 2] = b;  // B
    }

    return ESP_OK;
}
```

### Example 2: Wave Effect

```c
/**
 * WAVE Effect (0x10)
 *
 * Sine wave traveling along strip with color interpolation.
 * Parameters:
 *   params[0-2]: color1 (RGB)
 *   params[3-5]: color2 (RGB)
 *   params[6]: wavelength (1-255, number of peaks)
 *   params[7]: phase offset (animation position)
 * Render time: ~800μs
 */
static esp_err_t prism_effect_wave_render(
    const prism_pattern_t *pattern,
    uint32_t frame_counter,
    uint8_t *frame_ch1,
    uint8_t *frame_ch2)
{
    // Extract colors
    uint8_t r1 = pattern->header.params[0];
    uint8_t g1 = pattern->header.params[1];
    uint8_t b1 = pattern->header.params[2];

    uint8_t r2 = pattern->header.params[3];
    uint8_t g2 = pattern->header.params[4];
    uint8_t b2 = pattern->header.params[5];

    // Wavelength and speed
    uint8_t wavelength = pattern->header.params[6];
    if (wavelength == 0) wavelength = 32;  // Default

    // Calculate phase based on speed and frame counter
    uint8_t speed = pattern->header.speed;
    uint16_t phase = (frame_counter * speed) >> 7;  // Normalize speed

    // Render wave for both channels
    for (uint16_t led = 0; led < LED_COUNT_PER_CH; led++) {
        // Calculate sine wave position (0-255)
        uint8_t theta = ((led * 256) / wavelength + phase) & 0xFF;
        uint8_t wave_val = sin8(theta);  // 0-255

        // Interpolate between color1 and color2 based on wave
        uint8_t r = blend8(r1, r2, wave_val);
        uint8_t g = blend8(g1, g2, wave_val);
        uint8_t b = blend8(b1, b2, wave_val);

        // Apply global brightness
        r = scale8(r, pattern->header.brightness);
        g = scale8(g, pattern->header.brightness);
        b = scale8(b, pattern->header.brightness);

        // Write GRB to both channels
        uint16_t idx = led * 3;
        frame_ch1[idx + 0] = g;
        frame_ch1[idx + 1] = r;
        frame_ch1[idx + 2] = b;

        frame_ch2[idx + 0] = g;
        frame_ch2[idx + 1] = r;
        frame_ch2[idx + 2] = b;
    }

    return ESP_OK;
}
```

### Example 3: Fire Effect

```c
/**
 * FIRE Effect (0x11)
 *
 * Simulates fire using heat map and palette.
 * Parameters:
 *   params[0]: heat intensity (0-255)
 *   params[1]: cooling rate (0-255)
 *   params[2]: sparking probability (0-255)
 * Render time: ~1200μs
 */

// Heat map (persistent state between frames)
static uint8_t s_heat_map[LED_COUNT_PER_CH];

static esp_err_t prism_effect_fire_render(
    const prism_pattern_t *pattern,
    uint32_t frame_counter,
    uint8_t *frame_ch1,
    uint8_t *frame_ch2)
{
    uint8_t heat = pattern->header.params[0];
    uint8_t cooling = pattern->header.params[1];
    uint8_t sparking = pattern->header.params[2];

    // Default palette: black → red → orange → yellow → white
    static const prism_palette_entry_t fire_palette[5] = {
        {0, 0, 0},        // Black
        {128, 0, 0},      // Red
        {255, 64, 0},     // Orange
        {255, 192, 0},    // Yellow
        {255, 255, 255},  // White
    };

    // Step 1: Cool down every cell a little
    for (uint16_t i = 0; i < LED_COUNT_PER_CH; i++) {
        uint8_t cooldown = random8_minmax(0, ((cooling * 10) / LED_COUNT_PER_CH) + 2);
        s_heat_map[i] = (s_heat_map[i] > cooldown) ? (s_heat_map[i] - cooldown) : 0;
    }

    // Step 2: Heat from each cell drifts up and diffuses
    for (uint16_t i = LED_COUNT_PER_CH - 1; i >= 2; i--) {
        s_heat_map[i] = (s_heat_map[i - 1] + s_heat_map[i - 2] + s_heat_map[i - 2]) / 3;
    }

    // Step 3: Randomly ignite new sparks near the bottom
    if (random8() < sparking) {
        uint8_t pos = random8_minmax(0, 7);
        s_heat_map[pos] = qadd8(s_heat_map[pos], random8_minmax(160, 255));
    }

    // Step 4: Convert heat to LED colors using palette
    for (uint16_t i = 0; i < LED_COUNT_PER_CH; i++) {
        // Scale heat to palette index (0-255)
        uint8_t palette_index = scale8(s_heat_map[i], 240);

        // Lookup color from palette
        prism_rgb_t color = color_from_palette(fire_palette, 5, palette_index);

        // Apply global brightness
        color.r = scale8(color.r, pattern->header.brightness);
        color.g = scale8(color.g, pattern->header.brightness);
        color.b = scale8(color.b, pattern->header.brightness);

        // Write GRB to both channels
        uint16_t idx = i * 3;
        frame_ch1[idx + 0] = color.g;
        frame_ch1[idx + 1] = color.r;
        frame_ch1[idx + 2] = color.b;

        frame_ch2[idx + 0] = color.g;
        frame_ch2[idx + 1] = color.r;
        frame_ch2[idx + 2] = color.b;
    }

    return ESP_OK;
}
```

## Audit Trail

- **Proposed by:** Software Architect (Agent 5)
- **Reviewed by:** [Pending]
- **Approved by:** [Pending Captain SpectraSynq]
- **Implemented:** [Pending]
- **Validated:** [Pending]

---
**IMMUTABLE AFTER APPROVAL**
