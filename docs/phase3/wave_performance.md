# WAVE Mode Performance Report (v1.1)

This document summarizes cycle-count profiling for the LUT-based WAVE effect path.

Configuration:
- Target: ESP32-S3
- Build: Release (O3)
- FPS: 120
- LEDs: 2 × 160 (320 total)
- Profiling: CONFIG_PRISM_PROFILE_TEMPORAL=y

Measurements (cycles per frame, LUT path only):

| Metric | Cycles | Notes |
|--------|--------|-------|
| Min    | ~TBD   | Dependent on core frequency and cache state |
| Max    | ~TBD   | Observed during cache cold/warm transitions |
| Avg    | ~TBD   | Printed once per second when profiling enabled |

Notes:
- `sin8_table` is placed in DRAM via `DRAM_ATTR` to avoid flash stalls.
- Profiling is sampled per frame in the WAVE branch and reported once per second.
- Additional PMU cache counters can be integrated via `perfmon` if needed.

To enable profiling:
```
idf.py menuconfig  # Components → PRISM Playback → Enable temporal profiling
```

