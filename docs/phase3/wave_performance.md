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
- PMU counters are configurable via Kconfig to tailor overhead.

Counters and derived metrics:
- D$ hits/misses: counted via `XTPERF_CNT_D_ACCESS_U1`
  - Hits = `HITS_SHARED|HITS_EXCLUSIVE|HITS_MODIFIED`
  - Misses = `CACHE_MISSES`
  - D$ hit rate = hits / (hits + misses)
- I$ hits/misses: counted via `XTPERF_CNT_I_MEM`
  - Hits = `CACHE_HITS`
  - Misses = `CACHE_MISSES`
  - I$ hit rate = hits / (hits + misses)
- Instructions (optional): `XTPERF_CNT_INSN` with `INXN_ALL`
  - IPC(x100) = (instructions / avg_cycles) * 100 (scaled to avoid float)

Kconfig toggles (Components → PRISM Playback):
- `PRISM_PROFILE_TEMPORAL` (enable profiling)
- `PRISM_PROFILE_COUNT_DCACHE` (D$ hits/misses)
- `PRISM_PROFILE_COUNT_ICACHE` (I$ hits/misses)
- `PRISM_PROFILE_COUNT_INSN` (instruction count; may be skipped if PMU counters are exhausted)

PMU counter limits:
- Up to 4 counters are configured simultaneously. If toggles exceed capacity,
  instruction count is skipped in favor of cache metrics.

To enable profiling:
```
idf.py menuconfig  # Components → PRISM Playback → Enable temporal profiling
```

Endpoints & Export
- HTTP JSON (`/metrics/wave`): one‑second snapshot with cycles, D$/I$ hits/misses + hit_rate, optional INSN+IPC(x100)
- Prometheus (`/metrics`): text exposition format for scraping
- CSV (`/metrics.csv`): simple header + one row per request
- CLI: `prism_metrics` prints the current snapshot to the console

Enable endpoints via Kconfig (Components → PRISM Playback → PRISM Metrics Exposure):
- `PRISM_METRICS_HTTP` (master switch)
- `PRISM_METRICS_PROMETHEUS` (adds `/metrics`)
- `PRISM_METRICS_CSV` (adds `/metrics.csv`)
- `PRISM_METRICS_CLI` (adds `prism_metrics`)

Optional Push
- `PRISM_METRICS_PUSH` enables a background task that POSTs JSON snapshots every N seconds
- `PRISM_METRICS_PUSH_URL` target URL
- `PRISM_METRICS_PUSH_INTERVAL_SEC` interval in seconds (default 10)

Example JSON (abbreviated)
```json
{
  "samples": 120,
  "cycles": { "min": 330, "max": 610, "avg": 355 },
  "dcache": { "hits": 123456, "misses": 2345, "hit_rate": 98 },
  "icache": { "hits": 456789, "misses": 123, "hit_rate": 99 },
  "insn": { "count": 1234567, "ipc_x100": 87 }
}
```
