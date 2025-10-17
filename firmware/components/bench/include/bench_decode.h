/**
 * @file bench_decode.h
 * @brief API to run the decode benchmark harness.
 */

#ifndef PRISM_BENCH_DECODE_H
#define PRISM_BENCH_DECODE_H

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run the decode benchmark once using Kconfig parameters.
 *
 * Starts a periodic esp_timer at CONFIG_PRISM_BENCH_PERIOD_US and processes
 * CONFIG_PRISM_BENCH_FRAMES frames for CONFIG_PRISM_BENCH_LED_COUNT LEDs.
 * Emits JSON metrics to UART and optionally to a file path if enabled.
 */
esp_err_t bench_decode_run(void);

#ifdef __cplusplus
}
#endif

#endif // PRISM_BENCH_DECODE_H

