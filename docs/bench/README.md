# PRISM Decode Benchmark Harness

Location: `firmware/components/bench`

What it does
- Runs a periodic `esp_timer` at ~120 FPS (default 8333 µs)
- Measures per-frame elapsed microseconds and CPU cycles
- Uses a fixed ≤4 KB working arena (no per-frame heap)
- Emits line-delimited JSON to UART; optional file output

Configuration (menuconfig)
- PRISM_BENCH_FRAMES: total frames (default 480)
- PRISM_BENCH_LED_COUNT: LEDs per frame (default 160)
- PRISM_BENCH_PERIOD_US: frame period in µs (default 8333)
- PRISM_BENCH_ENABLE_FILE + PRISM_BENCH_FILE_PATH: write JSON summary to a file
- PRISM_BENCH_AUTORUN: auto-run harness on boot (disabled by default)
- PRISM_BENCH_PATTERN: workload (palette indices, XOR delta, or RLE burst)
- PRISM_BENCH_REGISTER_CLI: register `bench_decode` console command for ad-hoc runs

Build & Run
1) `cd firmware && idf.py build`
2) Flash and monitor: `idf.py -p <PORT> flash monitor`
3) If autorun is disabled, call `bench_decode_run()` from application code to start a run.

Console Command (optional)
- Enable `PRISM_BENCH_REGISTER_CLI` and run from the monitor prompt:
  - ``bench_decode pattern=xor frames=960 leds=160 period_us=8333 file=off``
  - `pattern` accepts `palette`, `xor`, or `rle`; `frames`, `leds`, and `period_us` override Kconfig defaults.

JSON Summary Example (UART)
```
{"bench":"decode","frames":480,"leds":160,"period_us":8333,
 "avg_us":312,"p99_us":540,"max_us":612,
 "avg_cycles":74532,"p99_cycles":128004,
 "bytes_total":230400,"workset_bytes":4096,
 "heap_free_before":123456,"heap_free_after":123456,
 "heap_min_before":120000,"heap_min_after":120000}
```

Integration Notes
- Replace the `decode_stub(...)` in `bench_decode.c` with the real packaging decode entry point when Task #30 lands.
- Generators now cover palette lookups, XOR deltas, and RLE bursts for quick regression sweeps.
- Keep decode work single-pass O(N), no per-frame heap, ≤4 KB working set to meet R1.1 envelope.
