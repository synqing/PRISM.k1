# Task 12: SYNC & OFFSET Temporal Sequencing - Codex Agent Brief

## Mission
Implement SYNC and OFFSET temporal sequencing modes with <3.38ms performance target.

## Prerequisites
- ✅ Task 11 MUST be complete (enums/structs defined)
- Component: `firmware/components/playback/`
- Target: <3.38ms execution time, <0.1% CPU overhead

## Task Details
Run: `task-master show 12`

## Implementation Steps

### Subtask 12.1: Implement prism_motion_init()
**File**: `firmware/components/playback/prism_temporal.c` (NEW)

```c
#include "prism_temporal.h"
#include "esp_check.h"
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

    // Zero-initialize context
    memset(ctx, 0, sizeof(*ctx));

    // Set defaults
    ctx->sync_mode = PRISM_SYNC_SYNC;
    ctx->motion_direction = PRISM_MOTION_STATIC;
    ctx->frame_index = 0;
    ctx->frame_time_ms = 0;

    ESP_LOGI(TAG, "Temporal context initialized for %zu LEDs", led_count);
    return ESP_OK;
}
```

**Checklist:**
- [ ] Create prism_temporal.c file
- [ ] Implement prism_motion_init with pointer validation
- [ ] Use ESP_RETURN_ON_FALSE for checks
- [ ] Zero-initialize context with memset
- [ ] Add logging
- [ ] Update CMakeLists.txt

### Subtask 12.2: Add inline helper functions
**File**: `firmware/components/playback/prism_temporal.c`

```c
// Calculate current frame time in milliseconds
static inline uint32_t prism_frame_time_ms(const prism_temporal_ctx_t *ctx, uint16_t frame_period_ms) {
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
- [ ] Add prism_frame_time_ms inline helper
- [ ] Add apply_offset inline helper
- [ ] Use uint32_t to prevent overflow
- [ ] Keep functions branchless where possible

### Subtask 12.3: Implement SYNC mode
**File**: `firmware/components/playback/prism_temporal.c`

```c
void calculate_ch2_frame(const prism_temporal_ctx_t *ctx,
                         const uint16_t *restrict ch1_frame,
                         uint16_t *restrict ch2_frame,
                         size_t led_count) {
    if (!ctx || !ch1_frame || !ch2_frame) {
        ESP_LOGE(TAG, "NULL pointer in calculate_ch2_frame");
        return;
    }

    switch (ctx->sync_mode) {
    case PRISM_SYNC_SYNC:
        // Fast path: direct memcpy for synchronized mode
        memcpy(ch2_frame, ch1_frame, led_count * sizeof(uint16_t));
        break;

    // Other modes handled in subsequent subtasks
    default:
        // Zero output for unimplemented modes
        memset(ch2_frame, 0, led_count * sizeof(uint16_t));
        break;
    }
}
```

**Checklist:**
- [ ] Implement calculate_ch2_frame function
- [ ] Add NULL pointer checks
- [ ] Implement SYNC mode with memcpy
- [ ] Add restrict qualifiers for optimization
- [ ] Default case zeros buffer

### Subtask 12.4: Implement OFFSET mode
**File**: `firmware/components/playback/prism_temporal.c`

Add OFFSET case to calculate_ch2_frame switch:

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
- [ ] Optimize zero-delay case
- [ ] Iterate all LEDs with apply_offset

### Subtask 12.5: Create Unity tests
**File**: `firmware/components/playback/test/test_prism_temporal.c` (NEW)

```c
#include "unity.h"
#include "prism_temporal.h"
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

void test_prism_motion_init_valid(void) {
    esp_err_t err = prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(PRISM_SYNC_SYNC, ctx.sync_mode);
    TEST_ASSERT_EQUAL(0, ctx.frame_index);
}

void test_prism_motion_init_null_ctx(void) {
    esp_err_t err = prism_motion_init(NULL, ch1_frame, ch2_frame, LED_COUNT);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_sync_mode_copy(void) {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_SYNC;

    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);

    // Verify exact copy
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ch1_frame, ch2_frame, LED_COUNT);
}

void test_offset_mode_zero_delay(void) {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_OFFSET;
    ctx.params.delay_ms = 0;
    ctx.frame_index = 10;

    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);

    // Zero delay should behave like SYNC
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ch1_frame, ch2_frame, LED_COUNT);
}

void test_offset_mode_active_delay(void) {
    prism_motion_init(&ctx, ch1_frame, ch2_frame, LED_COUNT);
    ctx.sync_mode = PRISM_SYNC_OFFSET;
    ctx.params.delay_ms = 150;  // 150ms delay
    ctx.frame_index = 5;  // 5 frames * 16ms = 80ms < 150ms

    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);

    // All LEDs should be zero (still in delay window)
    for (size_t i = 0; i < LED_COUNT; i++) {
        TEST_ASSERT_EQUAL_UINT16(0, ch2_frame[i]);
    }

    // Advance past delay threshold
    ctx.frame_index = 10;  // 10 * 16 = 160ms > 150ms
    calculate_ch2_frame(&ctx, ch1_frame, ch2_frame, LED_COUNT);

    // Now should match ch1
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ch1_frame, ch2_frame, LED_COUNT);
}

void test_performance_budget(void) {
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

    printf("calculate_ch2_frame took %lld us\\n", elapsed_us);
}

void app_main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_prism_motion_init_valid);
    RUN_TEST(test_prism_motion_init_null_ctx);
    RUN_TEST(test_sync_mode_copy);
    RUN_TEST(test_offset_mode_zero_delay);
    RUN_TEST(test_offset_mode_active_delay);
    RUN_TEST(test_performance_budget);

    UNITY_END();
}
```

**Checklist:**
- [ ] Create test file
- [ ] Add setUp/tearDown functions
- [ ] Test prism_motion_init validation
- [ ] Test SYNC mode copy
- [ ] Test OFFSET zero-delay optimization
- [ ] Test OFFSET active delay behavior
- [ ] Test performance budget <3.38ms
- [ ] Add to test CMakeLists.txt

## Completion Criteria
- [ ] All 5 subtasks complete
- [ ] All tests pass: `cd firmware && idf.py build && idf.py unity`
- [ ] Performance test shows <3.38ms execution
- [ ] No compiler warnings
- [ ] Code follows ESP-IDF style guide
- [ ] Run: `task-master set-status --id=12 --status=done`

## Build & Test Commands
```bash
cd firmware
idf.py build
idf.py unity -T test_prism_temporal
```

## Performance Target
- **SYNC mode**: <100μs (memcpy optimization)
- **OFFSET mode**: <500μs (160 LED loop)
- **Total budget**: <3.38ms (8.33ms frame ÷ 2.5 safety margin)

## Next Tasks
After Task 12:
- Task 14: Wire this into LED playback loop
- Task 15: Add PROGRESSIVE mode to the switch
- Task 17: Add WAVE mode to the switch
