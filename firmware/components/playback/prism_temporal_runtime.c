// prism_temporal_runtime.c - Temporal sequencing runtime (SYNC/OFFSET)
#include "prism_temporal_ctx.h"
#include "prism_temporal.h"
#include "prism_wave_tables.h"
#include "led_driver.h"  // for LED_FRAME_TIME_MS
#include "esp_check.h"
#include "esp_log.h"
#include "esp_attr.h"
#include <string.h>

#define TAG "prism_temporal"

// --- WAVE mode support -----------------------------------------------------
// Static delay table for WAVE mode (computed from params)
static DRAM_ATTR uint16_t wave_delay_table[160] __attribute__((aligned(64)));
static bool s_wave_table_inited = false;

// Calculate per-LED wave delay using sin8_table
static inline uint16_t calculate_wave_delay(size_t led_index,
                                            const prism_sync_params_t *params)
{
    // Map LED index 0..159 to 0..255 phase domain
    const uint8_t phase = (uint8_t)((led_index * 256u) / 160u);
    const int16_t sine = (int16_t)sin8_table[phase] - 128;  // -128..127
    // base ± amplitude scaled by sine
    int32_t delay = (int32_t)params->delay_ms + ((int32_t)sine * (int32_t)params->wave_amplitude_ms) / 128;
    if (delay < 0) delay = 0;
    if (delay > 65535) delay = 65535;
    return (uint16_t)delay;
}

static void init_wave_table(const prism_sync_params_t *params)
{
    for (size_t i = 0; i < 160; ++i) {
        wave_delay_table[i] = calculate_wave_delay(i, params);
    }
    s_wave_table_inited = true;
}

// Calculate current frame time in milliseconds
static inline uint32_t prism_frame_time_ms(const prism_temporal_ctx_t *ctx,
                                           uint16_t frame_period_ms) {
    (void)frame_period_ms; // unused; using global LED_FRAME_TIME_MS
    return ctx ? (ctx->frame_index * (uint32_t)LED_FRAME_TIME_MS) : 0U;
}

// Apply offset delay to LED value
// Returns 0 if frame_time_ms < delay_ms, otherwise returns base_value
static inline uint16_t apply_offset(uint16_t base_value,
                                    uint16_t delay_ms,
                                    uint32_t frame_time_ms) {
    return (frame_time_ms < delay_ms) ? 0U : base_value;
}

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

    // Reset WAVE table so it will be rebuilt on first use
    s_wave_table_inited = false;

    ESP_LOGI(TAG, "Temporal context initialized for %zu LEDs", led_count);
    return ESP_OK;
}

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

    case PRISM_SYNC_OFFSET: {
        const uint32_t now_ms = prism_frame_time_ms(ctx, LED_FRAME_TIME_MS);
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

    case PRISM_SYNC_CUSTOM: {
        const uint32_t now_ms = ctx->frame_time_ms; // runtime provides absolute ms
        if (ctx->delay_table == NULL) {
            ESP_LOGW(TAG, "CUSTOM mode without delay_table; zeroing CH2");
            memset(ch2_frame, 0, led_count * sizeof(uint16_t));
            break;
        }
        for (size_t i = 0; i < led_count; ++i) {
            const uint16_t d = ctx->delay_table[i];
            ch2_frame[i] = apply_offset(ch1_frame[i], d, now_ms);
        }
        break;
    }

    case PRISM_SYNC_WAVE: {
        // Lazy init wave table based on current params
        if (!s_wave_table_inited) {
            init_wave_table(&ctx->params);
        }

        const uint32_t now_ms = prism_frame_time_ms(ctx, LED_FRAME_TIME_MS);
        for (size_t i = 0; i < led_count; ++i) {
            // Apply motion mapping to pick the per-LED delay
            size_t j = prism_apply_motion_index(i, led_count, ctx->motion_direction);
            uint16_t delay = (j < 160) ? wave_delay_table[j] : 0;
            ch2_frame[i] = (now_ms >= delay) ? ch1_frame[i] : 0;
        }
        break;
    }

    default:
        // Zero output for unimplemented modes
        memset(ch2_frame, 0, led_count * sizeof(uint16_t));
        ESP_LOGW(TAG, "Unimplemented sync mode: %d", ctx->sync_mode);
        break;
    }
}

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
