# Playback Component (LED + Temporal Sequencing)

This component drives the dual‑channel LED playback engine and implements temporal sequencing (SYNC/OFFSET/PROGRESSIVE/WAVE).

- WAVE mode uses a DRAM‑resident 256‑entry `sin8` LUT for smooth, fixed‑point evaluation.
- Temporal runtime calculates CH2 from CH1 according to the active sync mode and motion direction.

## Developer Profiling

Enable cycle + cache profiling for WAVE to validate performance budgets.

- Kconfig: Components → PRISM Playback
  - `PRISM_PROFILE_TEMPORAL` (master switch)
  - `PRISM_PROFILE_COUNT_DCACHE` (D$ hits/misses)
  - `PRISM_PROFILE_COUNT_ICACHE` (I$ hits/misses)
  - `PRISM_PROFILE_COUNT_INSN` (retired instructions, for IPC x100)
- Logs: once/second summary (min/max/avg cycles, D$/I$ hit/miss, IPC when enabled)
- Accessor API:
  - `esp_err_t playback_get_wave_metrics(prism_wave_metrics_t *out);`
  - Struct includes: samples, min/max/avg cycles, D$/I$ hits/misses + hit_pct, insn_count, ipc_x100
- CLI (optional):
  - Enable `PRISM_METRICS_CLI` then run `prism_metrics` in console.

Notes
- Up to 4 PMU counters are configured. If limits are reached, instruction counting is skipped in favor of cache metrics.
- All profiling is gated by Kconfig; disable for production builds.

## Files of Interest

- `include/prism_wave_tables.h` — LUT + helpers (triangle/sawtooth)
- `prism_wave_tables.c` — DRAM‑placed `sin8_table`
- `prism_temporal.c` — PROGRESSIVE shape builders and motion mapping
- `prism_temporal_runtime.c` — Temporal CH2 calculation (adds WAVE with LUT)
- `led_playback.c` — Built‑in effects, profiling instrumentation, CLI command
- `include/led_playback.h` — Public API + metrics struct

## Quick Usage (Metrics)

```c
#include "led_playback.h"

void dump_metrics(void) {
    prism_wave_metrics_t m = {0};
    if (playback_get_wave_metrics(&m) == ESP_OK) {
        // Use m.avg_cycles, m.dcache_hit_pct, m.icache_hit_pct, m.ipc_x100, etc.
    }
}
```

## Build/Deps

- CMake declares `perfmon` and `console` dependencies when enabled via Kconfig.
- `sin8_table` and WAVE delay table are placed in DRAM to avoid flash stalls.

