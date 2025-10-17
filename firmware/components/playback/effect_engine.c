/**
 * @file effect_engine.c
 * @brief Lightweight effect engine: interpolation + chaining (MVP)
 */

#include "effect_engine.h"
#include <string.h>
#include <math.h>

typedef struct {
    uint8_t current;
    uint8_t start;
    uint8_t target;
    uint32_t elapsed_ms;
    uint32_t duration_ms;
    bool active;
} param8_t;

typedef struct {
    uint16_t current;
    uint16_t start;
    uint16_t target;
    uint32_t elapsed_ms;
    uint32_t duration_ms;
    bool active;
} param16_t;

typedef struct {
    // brightness effect (0..255)
    param8_t brightness;
    // gamma effect: gamma x100 (e.g. 100 = 1.0). Applies via 256-entry LUT
    param16_t gamma_x100;
    uint8_t gamma_lut[256];
    bool gamma_lut_dirty;
} fx_state_t;

static fx_state_t s_fx;

void effect_engine_init(void)
{
    memset(&s_fx, 0, sizeof(s_fx));
}

void effect_chain_clear(void)
{
    memset(&s_fx, 0, sizeof(s_fx));
}

void effect_add_brightness(uint8_t start_value)
{
    s_fx.brightness.active = true;
    s_fx.brightness.current = start_value;
    s_fx.brightness.start = start_value;
    s_fx.brightness.target = start_value;
    s_fx.brightness.elapsed_ms = 0;
    s_fx.brightness.duration_ms = 0;
}

void effect_brightness_set_target(uint8_t target_value, uint32_t duration_ms)
{
    if (!s_fx.brightness.active) {
        effect_add_brightness(target_value);
        return;
    }
    s_fx.brightness.start = s_fx.brightness.current;
    s_fx.brightness.target = target_value;
    s_fx.brightness.elapsed_ms = 0;
    s_fx.brightness.duration_ms = duration_ms;
}

void effect_engine_tick(uint32_t elapsed_ms)
{
    // brightness tick
    if (s_fx.brightness.active) {
        if (s_fx.brightness.duration_ms == 0) {
            s_fx.brightness.current = s_fx.brightness.target;
        } else {
            uint32_t t = s_fx.brightness.elapsed_ms + elapsed_ms;
            if (t >= s_fx.brightness.duration_ms) {
                s_fx.brightness.current = s_fx.brightness.target;
                s_fx.brightness.duration_ms = 0;
                s_fx.brightness.elapsed_ms = 0;
            } else {
                s_fx.brightness.elapsed_ms = t;
                float alpha = (float)t / (float)s_fx.brightness.duration_ms;
                int start = (int)s_fx.brightness.start;
                int target = (int)s_fx.brightness.target;
                int value = start + (int)((target - start) * alpha);
                if (value < 0) value = 0;
                if (value > 255) value = 255;
                s_fx.brightness.current = (uint8_t)value;
            }
        }
    }

    // gamma tick
    if (s_fx.gamma_x100.active) {
        if (s_fx.gamma_x100.duration_ms == 0) {
            if (s_fx.gamma_x100.current != s_fx.gamma_x100.target) {
                s_fx.gamma_x100.current = s_fx.gamma_x100.target;
                s_fx.gamma_lut_dirty = true;
            }
        } else {
            uint32_t t = s_fx.gamma_x100.elapsed_ms + elapsed_ms;
            if (t >= s_fx.gamma_x100.duration_ms) {
                if (s_fx.gamma_x100.current != s_fx.gamma_x100.target) {
                    s_fx.gamma_x100.current = s_fx.gamma_x100.target;
                    s_fx.gamma_lut_dirty = true;
                }
                s_fx.gamma_x100.duration_ms = 0;
                s_fx.gamma_x100.elapsed_ms = 0;
            } else {
                s_fx.gamma_x100.elapsed_ms = t;
                float alpha = (float)t / (float)s_fx.gamma_x100.duration_ms;
                float start = (float)s_fx.gamma_x100.start;
                float target = (float)s_fx.gamma_x100.target;
                uint16_t value = (uint16_t)(start + (target - start) * alpha);
                if (value != s_fx.gamma_x100.current) {
                    s_fx.gamma_x100.current = value;
                    s_fx.gamma_lut_dirty = true;
                }
            }
        }
    }
}

void effect_chain_apply(uint8_t* rgb_buffer, size_t led_count)
{
    if (!rgb_buffer || led_count == 0) return;

    // Refresh gamma LUT if needed
    if (s_fx.gamma_lut_dirty) {
        // Compute gamma factor as float (x100)
        float g = (s_fx.gamma_x100.current > 0) ? (s_fx.gamma_x100.current / 100.0f) : 1.0f;
        for (int i = 0; i < 256; ++i) {
            float x = (float)i / 255.0f;
            float y = (float) (255.0f * powf(x, g) + 0.5f);
            if (y < 0.0f) {
                y = 0.0f;
            }
            if (y > 255.0f) {
                y = 255.0f;
            }
            s_fx.gamma_lut[i] = (uint8_t)y;
        }
        s_fx.gamma_lut_dirty = false;
    }

    // Apply gamma first, then brightness scale
    if (s_fx.gamma_x100.active) {
        for (size_t i = 0; i < led_count; ++i) {
            size_t idx = i * 3;
            rgb_buffer[idx + 0] = s_fx.gamma_lut[rgb_buffer[idx + 0]];
            rgb_buffer[idx + 1] = s_fx.gamma_lut[rgb_buffer[idx + 1]];
            rgb_buffer[idx + 2] = s_fx.gamma_lut[rgb_buffer[idx + 2]];
        }
    }

    if (s_fx.brightness.active) {
        uint8_t scale = s_fx.brightness.current; // 0..255
        for (size_t i = 0; i < led_count; ++i) {
            size_t idx = i * 3;
            rgb_buffer[idx + 0] = (uint8_t)((rgb_buffer[idx + 0] * scale) >> 8);
            rgb_buffer[idx + 1] = (uint8_t)((rgb_buffer[idx + 1] * scale) >> 8);
            rgb_buffer[idx + 2] = (uint8_t)((rgb_buffer[idx + 2] * scale) >> 8);
        }
    }
}

void effect_add_gamma(uint16_t gamma_x100)
{
    if (gamma_x100 == 0) gamma_x100 = 100;
    s_fx.gamma_x100.active = true;
    s_fx.gamma_x100.current = gamma_x100;
    s_fx.gamma_x100.start = gamma_x100;
    s_fx.gamma_x100.target = gamma_x100;
    s_fx.gamma_x100.elapsed_ms = 0;
    s_fx.gamma_x100.duration_ms = 0;
    s_fx.gamma_lut_dirty = true;
}

void effect_gamma_set_target(uint16_t gamma_x100, uint32_t duration_ms)
{
    if (!s_fx.gamma_x100.active) {
        effect_add_gamma(gamma_x100);
        return;
    }
    s_fx.gamma_x100.start = s_fx.gamma_x100.current;
    s_fx.gamma_x100.target = gamma_x100;
    s_fx.gamma_x100.elapsed_ms = 0;
    s_fx.gamma_x100.duration_ms = duration_ms;
}
