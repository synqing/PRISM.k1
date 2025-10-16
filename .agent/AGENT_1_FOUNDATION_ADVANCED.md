# AGENT 1: Foundation → Advanced Pipeline
**Mission: Critical Path - Foundation to CUSTOM Mode**

## Your Sequential Pipeline (20 subtasks)

```
Task 11: Motion/Sync Enums (3 subtasks) → Foundation
   ↓
Task 12: SYNC/OFFSET Temporal (5 subtasks) → Core Logic
   ↓
Task 14: Temporal Integration (4 subtasks) → System Integration
   ↓
Task 19: CUSTOM Mode + Web Editor (8 subtasks) → Advanced Features
```

**Total: 20 subtasks, ~4-6 hours**

---

## 📋 WORKFLOW FOR EACH SUBTASK

```bash
# 1. Check task details
task-master show <task-id>

# 2. Implement the subtask
# ... code code code ...

# 3. Commit your work
git add .
git commit -m "feat(task-X): complete subtask X.Y - description"
git push

# 4. Mark complete
task-master set-status --id=X.Y --status=done

# 5. Move to next subtask
```

---

## 🎯 TASK 11: Motion & Sync Enumerations (3 subtasks)

**Goal:** Create foundational data structures for temporal sequencing
**Location:** `firmware/components/playback/include/`

### Research Summary (Task 11):
- Define motion_direction_t and sync_mode_t enums (5 values each)
- Create prism_temporal_ctx_t struct for frame calculations
- Add prism_pattern_meta_v11_t for .prism file headers
- All fields uint8_t/uint16_t for network efficiency
- Static assertions enforce protocol compatibility

---

### Subtask 11.1: Create prism_motion.h

**File:** `firmware/components/playback/include/prism_motion.h`

```c
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
```

**Checklist:**
- [ ] Create header file
- [ ] Add both enums with explicit values
- [ ] Add validation macros
- [ ] Add static assertions
- [ ] Add extern "C" guards

**Commit:** `feat(task-11): complete subtask 11.1 - create prism_motion.h with enums`

---

### Subtask 11.2: Create prism_temporal.h

**File:** `firmware/components/playback/include/prism_temporal.h`

```c
#ifndef PRISM_TEMPORAL_H
#define PRISM_TEMPORAL_H

#include <stdint.h>
#include "prism_motion.h"

#ifdef __cplusplus
extern "C" {
#endif

// Sync parameters structure (12 bytes)
typedef struct {
    uint16_t delay_ms;              // Base delay for OFFSET mode
    uint16_t progressive_start_ms;   // PROGRESSIVE mode start delay
    uint16_t progressive_end_ms;     // PROGRESSIVE mode end delay
    uint16_t wave_amplitude_ms;      // WAVE mode amplitude
    uint16_t wave_frequency_hz;      // WAVE mode frequency
    uint16_t wave_phase_deg;         // WAVE mode phase offset (0-360)
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

#endif // PRISM_TEMPORAL_H
```

**Checklist:**
- [ ] Create temporal header
- [ ] Define sync_params_t struct (6x uint16_t = 12 bytes)
- [ ] Define prism_temporal_ctx_t struct
- [ ] Document delay_table ownership (critical!)
- [ ] Include prism_motion.h

**Commit:** `feat(task-11): complete subtask 11.2 - create prism_temporal.h structs`

---

### Subtask 11.3: Create pattern_metadata.h

**File:** `firmware/components/storage/include/pattern_metadata.h`

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

**Also update CMakeLists.txt:**

`firmware/components/storage/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "pattern_storage.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    REQUIRES "playback"  # Add this to access prism_motion.h
)
```

**Checklist:**
- [ ] Create metadata header
- [ ] Define packed struct (exactly 16 bytes)
- [ ] Add version constants
- [ ] Add size static assertion
- [ ] Include prism_motion.h
- [ ] Update storage CMakeLists.txt to require playback component

**Commit:** `feat(task-11): complete subtask 11.3 - create pattern_metadata.h`

**After Task 11:** Run `task-master set-status --id=11 --status=done`

---

## 🎯 TASK 12: SYNC & OFFSET Temporal Sequencing (5 subtasks)

**Goal:** Implement core temporal modes with <3.38ms performance
**Location:** `firmware/components/playback/`

### Research Summary (Task 12):
- SYNC mode: Direct memcpy for simultaneous edges
- OFFSET mode: Per-LED zero-fill until delay threshold
- Frame timing via esp_timer for monotonic time
- Inline helpers for branchless hot paths
- Performance target: <3.38ms total, SYNC <100μs, OFFSET <500μs

---

### Subtask 12.1: Create prism_temporal.c with init

**File:** `firmware/components/playback/prism_temporal.c` (NEW)

```c
#include "prism_temporal.h"
#include "esp_check.h"
#include "esp_log.h"
#include <string.h>

#define TAG "prism_temporal"

// Initialize temporal context with validation
esp_err_t prism_motion_init(prism_temporal_ctx_t *ctx,
                             const uint16_t *ch1_frame,
                             uint16_t *ch2_frame,
                             size_t led_count) {
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "ctx is NULL");
    ESP_RETURN_ON_FALSE(ch1_frame != NULL, ESP_ERR_INVALID_ARG, TAG, "ch1_frame is NULL");
    ESP_RETURN_ON_FALSE(ch2_frame != NULL, ESP_ERR_INVALID_ARG, TAG, "ch2_frame is NULL");
    ESP_RETURN_ON_FALSE(led_count > 0, ESP_ERR_INVALID_ARG, TAG, "led_count is zero");
    ESP_RETURN_ON_FALSE(led_count <= 160, ESP_ERR_INVALID_ARG, TAG, "led_count exceeds 160");

    // Zero-initialize context
    memset(ctx, 0, sizeof(*ctx));

    // Set safe defaults
    ctx->sync_mode = PRISM_SYNC_SYNC;
    ctx->motion_direction = PRISM_MOTION_STATIC;
    ctx->frame_index = 0;
    ctx->frame_time_ms = 0;
    ctx->delay_table = NULL;  // No delay table initially

    ESP_LOGI(TAG, "Temporal context initialized for %zu LEDs", led_count);
    return ESP_OK;
}
```

**Also create header:**

`firmware/components/playback/include/prism_temporal_api.h`:
```c
#ifndef PRISM_TEMPORAL_API_H
#define PRISM_TEMPORAL_API_H

#include "prism_temporal.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize temporal context
esp_err_t prism_motion_init(prism_temporal_ctx_t *ctx,
                             const uint16_t *ch1_frame,
                             uint16_t *ch2_frame,
                             size_t led_count);

// Calculate CH2 frame from CH1 using temporal context
void calculate_ch2_frame(const prism_temporal_ctx_t *ctx,
                         const uint16_t *restrict ch1_frame,
                         uint16_t *restrict ch2_frame,
                         size_t led_count);

#ifdef __cplusplus
}
#endif

#endif // PRISM_TEMPORAL_API_H
```

**Update CMakeLists.txt:**

`firmware/components/playback/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "led_playback.c" "prism_temporal.c"  # Add new file
    INCLUDE_DIRS "include"
)
```

**Checklist:**
- [ ] Create prism_temporal.c
- [ ] Implement prism_motion_init with validation
- [ ] Create prism_temporal_api.h header
- [ ] Update CMakeLists.txt
- [ ] Zero-initialize context
- [ ] Set safe defaults

**Commit:** `feat(task-12): complete subtask 12.1 - implement prism_motion_init`

---

### Subtask 12.2: Add inline helper functions

**Add to** `firmware/components/playback/prism_temporal.c`:

```c
// Calculate current frame time in milliseconds
static inline uint32_t prism_frame_time_ms(const prism_temporal_ctx_t *ctx,
                                            uint16_t frame_period_ms) {
    return ctx->frame_index * (uint32_t)frame_period_ms;
}

// Apply offset delay to LED value
// Returns 0 if frame_time_ms < delay_ms, otherwise returns base_value
static inline uint16_t apply_offset(uint16_t base_value,
                                     uint16_t delay_ms,
                                     uint32_t frame_time_ms) {
    return (frame_time_ms < delay_ms) ? 0U : base_value;
}
```

**Checklist:**
- [ ] Add prism_frame_time_ms helper
- [ ] Add apply_offset helper
- [ ] Use uint32_t to prevent overflow
- [ ] Keep functions static inline

**Commit:** `feat(task-12): complete subtask 12.2 - add inline temporal helpers`

---

### Subtask 12.3: Implement calculate_ch2_frame with SYNC mode

**Add to** `firmware/components/playback/prism_temporal.c`:

```c
void calculate_ch2_frame(const prism_temporal_ctx_t *ctx,
                         const uint16_t *restrict ch1_frame,
                         uint16_t *restrict ch2_frame,
                         size_t led_count) {
    // Null pointer validation
    if (!ctx || !ch1_frame || !ch2_frame) {
        ESP_LOGE(TAG, "NULL pointer in calculate_ch2_frame");
        return;
    }

    switch (ctx->sync_mode) {
    case PRISM_SYNC_SYNC:
        // Fast path: direct memcpy for synchronized mode
        // Both edges illuminate simultaneously (50% CPU savings)
        memcpy(ch2_frame, ch1_frame, led_count * sizeof(uint16_t));
        break;

    default:
        // Zero output for unimplemented modes
        memset(ch2_frame, 0, led_count * sizeof(uint16_t));
        ESP_LOGW(TAG, "Unimplemented sync mode: %d", ctx->sync_mode);
        break;
    }
}
```

**Checklist:**
- [ ] Implement calculate_ch2_frame function
- [ ] Add NULL pointer checks
- [ ] Implement SYNC mode with memcpy
- [ ] Add restrict qualifiers for compiler optimization
- [ ] Default case zeros buffer safely

**Commit:** `feat(task-12): complete subtask 12.3 - implement SYNC mode`

---

### Subtask 12.4: Implement OFFSET mode

**Update switch in** `calculate_ch2_frame`:

```c
    case PRISM_SYNC_OFFSET: {
        const uint32_t now_ms = prism_frame_time_ms(ctx, 16); // 60 FPS = 16.67ms
        const uint16_t delay_ms = ctx->params.delay_ms;

        // Early exit optimization: if delay is zero, behave like SYNC
        if (delay_ms == 0) {
            memcpy(ch2_frame, ch1_frame, led_count * sizeof(uint16_t));
            break;
        }

        // Apply per-LED offset
        for (size_t i = 0; i < led_count; ++i) {
            ch2_frame[i] = apply_offset(ch1_frame[i], delay_ms, now_ms);
        }
        break;
    }
```

**Checklist:**
- [ ] Add OFFSET case to switch
- [ ] Use prism_frame_time_ms helper
- [ ] Optimize zero-delay case (memcpy shortcut)
- [ ] Iterate all LEDs with apply_offset

**Commit:** `feat(task-12): complete subtask 12.4 - implement OFFSET mode`

---

### Subtask 12.5: Create Unity test suite

**File:** `firmware/components/playback/test/test_prism_temporal.c` (NEW)

```c
#include "unity.h"
#include "prism_temporal_api.h"
#include "esp_timer.h"
#include <string.h>

#define LED_COUNT 160
#define FRAME_PERIOD_MS 16  // 60 FPS

static prism_temporal_ctx_t ctx;
static uint16_t ch1_frame[LED_COUNT];
static uint16_t ch2_frame[LED_COUNT];

void setUp(void) {
    memset(&ctx, 0, sizeof(ctx));
    memset(ch1_frame, 0, sizeof(ch1_frame));
    memset(ch2_frame, 0, sizeof(ch2_frame));

    // Fill ch1 with test pattern
    for (size_t i = 0; i < LED_COUNT; i++) {
        ch1_frame[i] = (uint16_t)(i * 10);
    }
}

void tearDown(void) {
}

TEST_CASE("prism_motion_init validates pointers", "[temporal]") {
    esp_err_t err;

    // Valid init
    err = prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(PRISM_SYNC_SYNC, ctx.sync_mode);
    TEST_ASSERT_EQUAL(0, ctx.frame_index);

    // NULL ctx
    err = prism_motion_init(NULL, ch1_frame, ch2_frame, LED_COUNT);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);

    // NULL ch1_frame
    err = prism_motion_init(&ctx, NULL, ch2_frame, LED_COUNT);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);

    // Zero LED count
    err = prism_motion_init(&ctx, ch1_frame, ch2_frame, 0);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

TEST_CASE("SYNC mode copies ch1 to ch2", "[temporal]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_SYNC;

    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);

    // Verify exact copy
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ch1_frame, ch2_frame, LED_COUNT);
}

TEST_CASE("OFFSET mode with zero delay behaves like SYNC", "[temporal]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_OFFSET;
    ctx.params.delay_ms = 0;
    ctx.frame_index = 10;

    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);

    // Zero delay should behave like SYNC
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ch1_frame, ch2_frame, LED_COUNT);
}

TEST_CASE("OFFSET mode zeros LEDs during delay window", "[temporal]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_OFFSET;
    ctx.params.delay_ms = 150;  // 150ms delay
    ctx.frame_index = 5;  // 5 frames * 16ms = 80ms < 150ms

    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);

    // All LEDs should be zero (still in delay window)
    for (size_t i = 0; i < LED_COUNT; i++) {
        TEST_ASSERT_EQUAL_UINT16(0, ch2_frame[i]);
    }
}

TEST_CASE("OFFSET mode activates after delay threshold", "[temporal]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_OFFSET;
    ctx.params.delay_ms = 150;
    ctx.frame_index = 10;  // 10 * 16 = 160ms > 150ms

    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);

    // Should now match ch1
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ch1_frame, ch2_frame, LED_COUNT);
}

TEST_CASE("calculate_ch2_frame meets performance budget", "[temporal]") {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_OFFSET;
    ctx.params.delay_ms = 100;
    ctx.frame_index = 20;

    int64_t start = esp_timer_get_time();
    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    int64_t end = esp_timer_get_time();

    int64_t elapsed_us = end - start;

    // Must complete in <3380 microseconds (3.38ms budget)
    TEST_ASSERT_LESS_THAN(3380, elapsed_us);

    printf("calculate_ch2_frame took %lld us\n", elapsed_us);
}
```

**Create test CMakeLists.txt:**

`firmware/components/playback/test/CMakeLists.txt`:
```cmake
idf_component_register(
    SRC_DIRS "."
    INCLUDE_DIRS "."
    REQUIRES unity playback
)
```

**Checklist:**
- [ ] Create test file
- [ ] Add setUp/tearDown
- [ ] Test init validation (5 test cases)
- [ ] Test SYNC mode copy
- [ ] Test OFFSET zero-delay optimization
- [ ] Test OFFSET delay window behavior
- [ ] Test OFFSET activation after threshold
- [ ] Test performance budget <3.38ms
- [ ] Create test CMakeLists.txt

**Build & Run Tests:**
```bash
cd firmware
idf.py build
idf.py unity -T test_prism_temporal
```

**Commit:** `test(task-12): complete subtask 12.5 - add temporal test suite`

**After Task 12:** Run `task-master set-status --id=12 --status=done`

---

## 🎯 TASK 14: Temporal Integration with Playback (4 subtasks)

**Goal:** Wire temporal calculation into LED playback pipeline
**Location:** `firmware/components/playback/led_playback.c`

### Research Summary (Task 14):
- Pre-allocate static ch2_frame[160] buffer
- Populate prism_temporal_ctx_t from pattern metadata
- Coordinate with RMT transmission (before rmt_write_items)
- Frame timing via esp_timer_get_time()

---

### Subtask 14.1: Pre-allocate ch2_frame buffer

**Update** `firmware/components/playback/led_playback.c`:

```c
#include "led_playback.h"
#include "prism_temporal_api.h"

#define PRISM_LGP_LED_COUNT 160

// Static buffer for channel 2 frame data
static uint16_t ch2_frame[PRISM_LGP_LED_COUNT];

// Temporal context (shared across frames)
static prism_temporal_ctx_t temporal_ctx;
```

**In existing playback_init() function, add:**

```c
esp_err_t playback_init(void) {
    // ... existing init code ...

    // Zero-initialize temporal buffers
    memset(ch2_frame, 0, sizeof(ch2_frame));

    // Initialize temporal context (ch1_frame will be set per-pattern)
    esp_err_t err = prism_motion_init(&temporal_ctx, NULL, ch2_frame, PRISM_LGP_LED_COUNT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init temporal context: %d", err);
        return err;
    }

    ESP_LOGI(TAG, "Playback initialized with temporal support");
    return ESP_OK;
}
```

**Checklist:**
- [ ] Add ch2_frame static buffer
- [ ] Add temporal_ctx static variable
- [ ] Zero-initialize in playback_init
- [ ] Call prism_motion_init
- [ ] Add error handling

**Commit:** `feat(task-14): complete subtask 14.1 - allocate ch2_frame buffer`

---

### Subtask 14.2: Populate temporal context from pattern

**Add function to** `led_playback.c`:

```c
// Update temporal context from loaded pattern metadata
static void update_temporal_context(const prism_pattern_meta_v11_t *meta) {
    if (!meta) {
        ESP_LOGW(TAG, "NULL metadata, using SYNC defaults");
        temporal_ctx.sync_mode = PRISM_SYNC_SYNC;
        temporal_ctx.motion_direction = PRISM_MOTION_STATIC;
        memset(&temporal_ctx.params, 0, sizeof(temporal_ctx.params));
        return;
    }

    // Validate and copy metadata
    if (PRISM_SYNC_IS_VALID(meta->sync_mode)) {
        temporal_ctx.sync_mode = (prism_sync_mode_t)meta->sync_mode;
    } else {
        ESP_LOGW(TAG, "Invalid sync mode %d, defaulting to SYNC", meta->sync_mode);
        temporal_ctx.sync_mode = PRISM_SYNC_SYNC;
    }

    if (PRISM_MOTION_IS_VALID(meta->motion_direction)) {
        temporal_ctx.motion_direction = (prism_motion_t)meta->motion_direction;
    } else {
        ESP_LOGW(TAG, "Invalid motion %d, defaulting to STATIC", meta->motion_direction);
        temporal_ctx.motion_direction = PRISM_MOTION_STATIC;
    }

    // Copy sync parameters
    memcpy(&temporal_ctx.params, &meta->params, sizeof(temporal_ctx.params));

    // Reset frame timing
    temporal_ctx.frame_index = 0;
    temporal_ctx.frame_time_ms = 0;

    ESP_LOGI(TAG, "Temporal context updated: mode=%d, motion=%d",
             temporal_ctx.sync_mode, temporal_ctx.motion_direction);
}
```

**Checklist:**
- [ ] Create update_temporal_context function
- [ ] Validate sync_mode and motion enums
- [ ] Copy sync parameters
- [ ] Reset frame timing
- [ ] Add logging

**Commit:** `feat(task-14): complete subtask 14.2 - populate temporal context`

---

### Subtask 14.3: Integrate with RMT transmission

**Update playback loop in** `led_playback.c`:

Find the existing RMT transmission code and add temporal calculation:

```c
// In playback_task() or similar function:

void playback_frame(const uint16_t *ch1_frame, size_t led_count) {
    // Calculate CH2 frame using temporal logic
    calculate_ch2_frame(&temporal_ctx, ch1_frame, ch2_frame, led_count);

    // Increment frame counter for next iteration
    temporal_ctx.frame_index++;

    // TODO: Transmit both channels via RMT
    // This will be completed when RMT dual-channel support is added
    // For now, we're calculating ch2_frame correctly

    ESP_LOGD(TAG, "Frame %lu calculated with mode=%d",
             temporal_ctx.frame_index, temporal_ctx.sync_mode);
}
```

**Checklist:**
- [ ] Call calculate_ch2_frame before RMT transmission
- [ ] Increment frame_index after each frame
- [ ] Add debug logging
- [ ] Document RMT integration point

**Commit:** `feat(task-14): complete subtask 14.3 - integrate temporal with playback`

---

### Subtask 14.4: Implement frame timing with esp_timer

**Add to** `led_playback.c`:

```c
#include "esp_timer.h"

// Pattern start timestamp (microseconds)
static int64_t pattern_start_time_us = 0;

// Start a new pattern with temporal sequencing
void playback_start_pattern(const prism_pattern_meta_v11_t *meta) {
    // Update temporal context from pattern metadata
    update_temporal_context(meta);

    // Record pattern start time
    pattern_start_time_us = esp_timer_get_time();
    temporal_ctx.frame_time_ms = 0;
    temporal_ctx.frame_index = 0;

    ESP_LOGI(TAG, "Pattern started at %lld us", pattern_start_time_us);
}

// Update frame timing before each frame calculation
void playback_update_timing(void) {
    // Calculate elapsed time since pattern start
    int64_t now_us = esp_timer_get_time();
    int64_t elapsed_us = now_us - pattern_start_time_us;
    temporal_ctx.frame_time_ms = (uint32_t)(elapsed_us / 1000);

    ESP_LOGD(TAG, "Frame %lu at %lu ms",
             temporal_ctx.frame_index, temporal_ctx.frame_time_ms);
}
```

**Update playback_frame:**

```c
void playback_frame(const uint16_t *ch1_frame, size_t led_count) {
    // Update timing before calculation
    playback_update_timing();

    // Calculate CH2 frame using temporal logic
    calculate_ch2_frame(&temporal_ctx, ch1_frame, ch2_frame, led_count);

    // Increment frame counter
    temporal_ctx.frame_index++;

    // Transmit via RMT...
}
```

**Checklist:**
- [ ] Add pattern_start_time_us tracking
- [ ] Implement playback_start_pattern
- [ ] Implement playback_update_timing
- [ ] Update playback_frame to call timing function
- [ ] Use esp_timer_get_time for monotonic clock

**Commit:** `feat(task-14): complete subtask 14.4 - add frame timing with esp_timer`

**After Task 14:** Run `task-master set-status --id=14 --status=done`

---

## 🎯 TASK 19: CUSTOM Mode + Web Editor (8 subtasks)

**Goal:** Expert-level per-LED delay maps with WebAssembly editor
**Location:** Multiple - firmware + web tooling

### Research Summary (Task 19):
- CUSTOM mode: 160-entry delay map (320 bytes)
- WebAssembly build for .prism export consistency
- Spline-based curve editor (Bezier/Catmull-Rom)
- Real-time LED preview renderer
- Validation: delay values, total size, CRC integrity

---

### Subtask 19.1: Set up WebAssembly serializer build

**Create:** `tools/prism-editor/wasm/`

```bash
mkdir -p tools/prism-editor/wasm
cd tools/prism-editor/wasm
```

**File:** `tools/prism-editor/wasm/serialize_prism.c`

```c
#include <stdint.h>
#include <string.h>

// Shared types (copy from firmware)
typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t motion_direction;
    uint8_t sync_mode;
    uint8_t reserved;
    uint16_t delay_ms;
    uint16_t progressive_start_ms;
    uint16_t progressive_end_ms;
    uint16_t wave_amplitude_ms;
    uint16_t wave_frequency_hz;
    uint16_t wave_phase_deg;
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

    // Copy parameters
    memcpy(&header->delay_ms, params, 12);

    // If CUSTOM mode, append delay map
    if (sync_mode == 4) {  // CUSTOM = 4
        memcpy(buffer + 16, delay_map, 320);
    }

    return buffer;
}
```

**File:** `tools/prism-editor/wasm/build.sh`

```bash
#!/bin/bash
# Build WASM module for .prism export

emcc serialize_prism.c \
  -s MODULARIZE=1 \
  -s EXPORT_NAME='PrismExporter' \
  -s ENVIRONMENT='web' \
  -s EXPORTED_FUNCTIONS='["_create_prism_header"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -O3 \
  -o prism_exporter.js

echo "WASM build complete: prism_exporter.js + prism_exporter.wasm"
```

**Checklist:**
- [ ] Create wasm directory
- [ ] Create serialize_prism.c
- [ ] Create build.sh script
- [ ] Make build.sh executable: `chmod +x build.sh`
- [ ] Test build: `./build.sh` (requires emscripten)

**Commit:** `feat(task-19): complete subtask 19.1 - setup WASM serializer`

---

### Subtask 19.2: Implement spline-based curve editor UI

**Create:** `tools/prism-editor/src/CurveEditor.js`

```javascript
// Bezier curve evaluation for delay map generation
export class CurveEditor {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.controlPoints = [];
        this.curveType = 'bezier'; // 'bezier' or 'catmull-rom'
    }

    // Add control point
    addPoint(x, y) {
        this.controlPoints.push({ x, y });
        this.redraw();
    }

    // Bezier curve calculation
    evaluateBezier(t, points) {
        if (points.length < 2) return { x: 0, y: 0 };

        // Recursive De Casteljau algorithm
        if (points.length === 1) return points[0];

        const newPoints = [];
        for (let i = 0; i < points.length - 1; i++) {
            const x = (1 - t) * points[i].x + t * points[i + 1].x;
            const y = (1 - t) * points[i].y + t * points[i + 1].y;
            newPoints.push({ x, y });
        }
        return this.evaluateBezier(t, newPoints);
    }

    // Sample curve to 160 LED delay values
    sampleToDelayMap(minDelay, maxDelay) {
        const delayMap = new Uint16Array(160);

        for (let i = 0; i < 160; i++) {
            const t = i / 159.0;
            const point = this.evaluateBezier(t, this.controlPoints);

            // Map y-coordinate to delay range
            const normalizedY = point.y / this.canvas.height;
            const delay = minDelay + (maxDelay - minDelay) * normalizedY;
            delayMap[i] = Math.round(Math.max(minDelay, Math.min(maxDelay, delay)));
        }

        return delayMap;
    }

    // Render curve to canvas
    redraw() {
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

        // Draw grid
        this.ctx.strokeStyle = '#333';
        this.ctx.lineWidth = 1;
        for (let i = 0; i <= 160; i += 20) {
            const x = (i / 160) * this.canvas.width;
            this.ctx.beginPath();
            this.ctx.moveTo(x, 0);
            this.ctx.lineTo(x, this.canvas.height);
            this.ctx.stroke();
        }

        // Draw curve
        this.ctx.strokeStyle = '#0f0';
        this.ctx.lineWidth = 2;
        this.ctx.beginPath();

        for (let i = 0; i <= 160; i++) {
            const t = i / 160.0;
            const point = this.evaluateBezier(t, this.controlPoints);
            if (i === 0) this.ctx.moveTo(point.x, point.y);
            else this.ctx.lineTo(point.x, point.y);
        }
        this.ctx.stroke();

        // Draw control points
        this.ctx.fillStyle = '#f00';
        this.controlPoints.forEach(p => {
            this.ctx.beginPath();
            this.ctx.arc(p.x, p.y, 5, 0, 2 * Math.PI);
            this.ctx.fill();
        });
    }
}
```

**Checklist:**
- [ ] Create CurveEditor class
- [ ] Implement Bezier evaluation
- [ ] Implement delay map sampling
- [ ] Add canvas rendering
- [ ] Support control point manipulation

**Commit:** `feat(task-19): complete subtask 19.2 - add curve editor UI`

---

### Subtask 19.3: Create curve-to-delay sampling engine

**Already implemented in 19.2!** The `sampleToDelayMap` method handles this.

**Add validation function:**

```javascript
// Add to CurveEditor class
validateDelayMap(delayMap) {
    const errors = [];

    // Check size
    if (delayMap.length !== 160) {
        errors.push(`Invalid length: ${delayMap.length}, expected 160`);
    }

    // Check range (0-65535 ms)
    for (let i = 0; i < delayMap.length; i++) {
        if (delayMap[i] > 65535) {
            errors.push(`LED ${i}: delay ${delayMap[i]} exceeds 65535ms`);
        }
    }

    // Check total size (320 bytes)
    const totalBytes = delayMap.length * 2;
    if (totalBytes !== 320) {
        errors.push(`Total size ${totalBytes} bytes, expected 320`);
    }

    return errors;
}
```

**Commit:** `feat(task-19): complete subtask 19.3 - add delay map validation`

---

### Subtask 19.4: Add firmware delay map validation

**Update** `firmware/components/playback/prism_temporal.c`:

```c
// Validate CUSTOM mode delay map
esp_err_t prism_temporal_validate_delay_map(const uint16_t *delay_map, size_t count) {
    ESP_RETURN_ON_FALSE(delay_map != NULL, ESP_ERR_INVALID_ARG, TAG, "delay_map is NULL");
    ESP_RETURN_ON_FALSE(count == 160, ESP_ERR_INVALID_SIZE, TAG,
                        "delay_map must have 160 entries, got %zu", count);

    // Check for reasonable delay values (0-10000ms)
    for (size_t i = 0; i < count; i++) {
        if (delay_map[i] > 10000) {
            ESP_LOGW(TAG, "LED %zu has unusually large delay: %u ms", i, delay_map[i]);
        }
    }

    ESP_LOGI(TAG, "Delay map validated: %zu entries", count);
    return ESP_OK;
}
```

**Add to** `prism_temporal_api.h`:

```c
// Validate CUSTOM mode delay map
esp_err_t prism_temporal_validate_delay_map(const uint16_t *delay_map, size_t count);
```

**Commit:** `feat(task-19): complete subtask 19.4 - add firmware delay validation`

---

### Subtask 19.5: Implement real-time LED preview renderer

**Create:** `tools/prism-editor/src/LEDPreview.js`

```javascript
export class LEDPreview {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.currentFrame = 0;
        this.playing = false;
    }

    // Render LEDs with temporal delays
    renderFrame(ch1Colors, delayMap, frameTimeMs) {
        this.ctx.fillStyle = '#000';
        this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

        const ledWidth = this.canvas.width / 160;
        const ledHeight = 20;

        // CH1 (bottom edge)
        for (let i = 0; i < 160; i++) {
            const x = i * ledWidth;
            this.ctx.fillStyle = ch1Colors[i];
            this.ctx.fillRect(x, this.canvas.height - ledHeight, ledWidth - 2, ledHeight);
        }

        // CH2 (top edge) with delay applied
        for (let i = 0; i < 160; i++) {
            const delay = delayMap[i];
            const active = frameTimeMs >= delay;
            const color = active ? ch1Colors[i] : '#000';

            const x = i * ledWidth;
            this.ctx.fillStyle = color;
            this.ctx.fillRect(x, 0, ledWidth - 2, ledHeight);
        }

        // Draw frame time indicator
        this.ctx.fillStyle = '#fff';
        this.ctx.font = '12px monospace';
        this.ctx.fillText(`Frame: ${this.currentFrame} | Time: ${frameTimeMs}ms`, 10, 40);
    }

    // Animate preview
    play(ch1Colors, delayMap, fps = 60) {
        this.playing = true;
        const framePeriodMs = 1000 / fps;
        let lastTime = performance.now();

        const animate = (currentTime) => {
            if (!this.playing) return;

            const deltaTime = currentTime - lastTime;
            if (deltaTime >= framePeriodMs) {
                this.currentFrame++;
                const frameTimeMs = this.currentFrame * framePeriodMs;
                this.renderFrame(ch1Colors, delayMap, frameTimeMs);
                lastTime = currentTime;
            }

            requestAnimationFrame(animate);
        };

        requestAnimationFrame(animate);
    }

    stop() {
        this.playing = false;
        this.currentFrame = 0;
    }
}
```

**Commit:** `feat(task-19): complete subtask 19.5 - add LED preview renderer`

---

### Subtask 19.6: .prism export and TLV integration

**Create:** `tools/prism-editor/src/PrismExporter.js`

```javascript
import PrismExporter from '../wasm/prism_exporter.js';

export class PrismFileExporter {
    constructor() {
        this.wasmModule = null;
        this.ready = this.initWasm();
    }

    async initWasm() {
        this.wasmModule = await PrismExporter();
        console.log('WASM module loaded');
    }

    // Export CUSTOM mode pattern to .prism binary
    async exportCustomPattern(delayMap, metadata) {
        await this.ready;

        // Prepare parameters array
        const params = new Uint16Array([
            metadata.delay_ms || 0,
            metadata.progressive_start_ms || 0,
            metadata.progressive_end_ms || 0,
            metadata.wave_amplitude_ms || 0,
            metadata.wave_frequency_hz || 0,
            metadata.wave_phase_deg || 0
        ]);

        // Call WASM function
        const headerPtr = this.wasmModule.ccall(
            'create_prism_header',
            'number',
            ['number', 'number', 'number', 'number'],
            [
                metadata.motion_direction || 0,
                4, // CUSTOM mode = 4
                params,
                delayMap
            ]
        );

        // Copy data from WASM memory
        const totalSize = 16 + 320; // Header + delay map
        const buffer = new Uint8Array(
            this.wasmModule.HEAPU8.buffer,
            headerPtr,
            totalSize
        );

        // Create blob for download
        const blob = new Blob([buffer], { type: 'application/octet-stream' });
        return blob;
    }

    // Download .prism file
    downloadPattern(blob, filename) {
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.click();
        URL.revokeObjectURL(url);
    }
}
```

**Commit:** `feat(task-19): complete subtask 19.6 - add .prism export`

---

### Subtask 19.7: Seed preset library with CUSTOM patterns

**Create:** `tools/prism-editor/presets/custom/`

**File:** `tools/prism-editor/presets/custom/wave_rise.json`

```json
{
    "name": "Wave Rise",
    "description": "Sinusoidal wave pattern rising from left to right",
    "version": "1.1",
    "motion_direction": 0,
    "sync_mode": 4,
    "delay_map": [
        0, 31, 62, 92, 121, 149, 175, 200, 222, 243,
        261, 277, 290, 301, 309, 314, 317, 317, 314, 309,
        301, 290, 277, 261, 243, 222, 200, 175, 149, 121,
        92, 62, 31, 0, 31, 62, 92, 121, 149, 175,
        200, 222, 243, 261, 277, 290, 301, 309, 314, 317,
        317, 314, 309, 301, 290, 277, 261, 243, 222, 200,
        175, 149, 121, 92, 62, 31, 0, 31, 62, 92,
        121, 149, 175, 200, 222, 243, 261, 277, 290, 301,
        309, 314, 317, 317, 314, 309, 301, 290, 277, 261,
        243, 222, 200, 175, 149, 121, 92, 62, 31, 0,
        31, 62, 92, 121, 149, 175, 200, 222, 243, 261,
        277, 290, 301, 309, 314, 317, 317, 314, 309, 301,
        290, 277, 261, 243, 222, 200, 175, 149, 121, 92,
        62, 31, 0, 31, 62, 92, 121, 149, 175, 200,
        222, 243, 261, 277, 290, 301, 309, 314, 317, 317,
        314, 309, 301, 290, 277, 261, 243, 222, 200, 175
    ]
}
```

**Create 5 more presets:**
- spiral.json
- chevron.json
- pulse_expand.json
- zigzag.json
- random_sparkle.json

**Commit:** `feat(task-19): complete subtask 19.7 - seed CUSTOM presets`

---

### Subtask 19.8: Build comprehensive CUSTOM mode test suite

**Create:** `firmware/components/playback/test/test_custom_mode.c`

```c
#include "unity.h"
#include "prism_temporal_api.h"

TEST_CASE("CUSTOM mode delay map validation", "[temporal][custom]") {
    uint16_t valid_map[160];
    for (int i = 0; i < 160; i++) {
        valid_map[i] = i * 10; // 0ms to 1590ms
    }

    esp_err_t err = prism_temporal_validate_delay_map(valid_map, 160);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

TEST_CASE("CUSTOM mode rejects NULL delay map", "[temporal][custom]") {
    esp_err_t err = prism_temporal_validate_delay_map(NULL, 160);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

TEST_CASE("CUSTOM mode rejects invalid size", "[temporal][custom]") {
    uint16_t map[100];
    esp_err_t err = prism_temporal_validate_delay_map(map, 100);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, err);
}

TEST_CASE("CUSTOM mode applies per-LED delays", "[temporal][custom]") {
    prism_temporal_ctx_t ctx;
    uint16_t ch1[160], ch2[160];
    uint16_t delay_map[160];

    // Setup: all CH1 LEDs bright, delays increase linearly
    for (int i = 0; i < 160; i++) {
        ch1[i] = 1000;
        delay_map[i] = i * 5; // 0, 5, 10, 15... ms
    }

    prism_motion_init(&ctx, ch1, ch2, 160);
    ctx.sync_mode = PRISM_SYNC_CUSTOM;
    ctx.delay_table = delay_map;

    // At frame_time=50ms, only LEDs 0-10 should be active
    ctx.frame_time_ms = 50;
    calculate_ch2_frame(&ctx, ch1, ch2, 160);

    for (int i = 0; i < 160; i++) {
        if (delay_map[i] <= 50) {
            TEST_ASSERT_EQUAL_UINT16(1000, ch2[i]);
        } else {
            TEST_ASSERT_EQUAL_UINT16(0, ch2[i]);
        }
    }
}
```

**Commit:** `test(task-19): complete subtask 19.8 - add CUSTOM mode tests`

**After Task 19:** Run `task-master set-status --id=19 --status=done`

---

## 🎉 PIPELINE COMPLETE!

When you finish all 4 tasks, you will have built:

✅ **Task 11:** Foundation enums/structs
✅ **Task 12:** SYNC/OFFSET temporal modes
✅ **Task 14:** Playback integration
✅ **Task 19:** CUSTOM mode with web editor

**Total: 20 subtasks completed!**

---

## Final Validation

```bash
cd firmware
idf.py build
idf.py unity
```

All tests should pass. Commit final status:

```bash
git add .taskmaster/tasks/tasks.json
git commit -m "chore(taskmaster): Agent 1 pipeline complete - 20 subtasks"
git push
```

**Congratulations! You've crushed the critical path! 🚀**
