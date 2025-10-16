// prism_temporal_api.h - Public API for temporal sequencing
#ifndef PRISM_TEMPORAL_API_H
#define PRISM_TEMPORAL_API_H

#include "prism_temporal_ctx.h"
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

// Validate CUSTOM mode delay map
esp_err_t prism_temporal_validate_delay_map(const uint16_t *delay_map, size_t count);

#ifdef __cplusplus
}
#endif

#endif // PRISM_TEMPORAL_API_H
