/**
 * @file effect_engine.h
 * @brief Lightweight effect engine: parameter interpolation + effect chaining
 */

#ifndef PRISM_EFFECT_ENGINE_H
#define PRISM_EFFECT_ENGINE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize effect engine state. */
void effect_engine_init(void);

/** Clear the active effect chain. */
void effect_chain_clear(void);

/** Add a brightness scaling effect to the chain (0..255). */
void effect_add_brightness(uint8_t start_value);

/**
 * Smoothly transition brightness to target over duration_ms.
 * Non-blocking; call effect_engine_tick() each frame to advance.
 */
void effect_brightness_set_target(uint8_t target_value, uint32_t duration_ms);

/** Advance interpolators by elapsed_ms. */
void effect_engine_tick(uint32_t elapsed_ms);

/**
 * Apply the active chain to an in-place RGB buffer (8-bit RGBRGB...).
 * led_count = number of RGB triplets in buffer.
 */
void effect_chain_apply(uint8_t* rgb_buffer, size_t led_count);

/** Add a gamma correction effect to the chain (gamma x100, e.g. 100 = 1.0). */
void effect_add_gamma(uint16_t gamma_x100);

/** Smoothly change gamma (x100) to target over duration. */
void effect_gamma_set_target(uint16_t gamma_x100, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif // PRISM_EFFECT_ENGINE_H
