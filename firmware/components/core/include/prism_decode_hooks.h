/**
 * @file prism_decode_hooks.h
 * @brief Lightweight instrumentation hooks for on-device decode benchmarking.
 *
 * Packaging/decoder code may include this header and call the BEGIN/END
 * macros around its hot decode loop to expose cycle/time measurements to
 * the firmware microbench harness. These hooks are no-ops if unused.
 */

#ifndef PRISM_DECODE_HOOKS_H
#define PRISM_DECODE_HOOKS_H

#include <stdint.h>
#include <stddef.h>
#include "esp_attr.h"
#include "esp_cpu.h"      // esp_cpu_get_cycle_count
#include "esp_timer.h"    // esp_timer_get_time

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t cycles_start;
    uint64_t time_start_us;
} prism_decode_hook_ctx_t;

// Begin measurement for a decode section
static inline IRAM_ATTR void prism_decode_begin(prism_decode_hook_ctx_t *ctx) {
    if (!ctx) return;
    ctx->time_start_us = esp_timer_get_time();
    ctx->cycles_start  = esp_cpu_get_cycle_count();
}

// End measurement for a decode section; returns cycles and elapsed us via out params
static inline IRAM_ATTR void prism_decode_end(const prism_decode_hook_ctx_t *ctx,
                                             uint32_t *out_cycles,
                                             uint32_t *out_elapsed_us) {
    if (!ctx) return;
    if (out_cycles) {
        uint32_t end = esp_cpu_get_cycle_count();
        *out_cycles = end - ctx->cycles_start;
    }
    if (out_elapsed_us) {
        uint64_t end = esp_timer_get_time();
        uint64_t dt  = end - ctx->time_start_us;
        *out_elapsed_us = (uint32_t)dt;
    }
}

#ifdef __cplusplus
}
#endif

#endif // PRISM_DECODE_HOOKS_H

