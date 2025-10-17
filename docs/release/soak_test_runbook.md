# PRISM v1.1 Soak Test Runbook

This playbook coordinates the 24-hour validation soak across at least three hardware units. It aligns with R3.5 preview defaults, R3.4 packaging constraints, and the release-ready preset library produced by `tools/preset_library.py`.

## Goals

- Cycle ≥20 presets generated with the v1.1 tooling chain (palette/show/packaging).
- Capture telemetry every minute: heap fragmentation, frame timing, temperature, error counts.
- Verify packaging integrity (`.prism` headers/CRC) both before and after the run.
- Confirm no device exceeds thresholds: fragmentation <5%, frame time <110% target, temperature <65 °C (adjust per enclosure spec).

## Pre-Flight Checklist

1. **Artifacts**
   - Generate preset library: `python3 -m tools.preset_library --output-dir out/presets`
   - Package to `.prism`: ensure `out/presets/*/*.prism` exists.
   - Validate packages: `python3 -m tools.validation.prism_sanity --glob "out/presets/**/*.prism"`
2. **Firmware & Config**
   - Flash v1.1 firmware build with soak test build-flag enabled (logging @ 115200).
   - Deploy preset library via USB/OTA to all test devices.
   - Configure automation host with Python 3.11+, `pip install -r tools/requirements.txt`.
3. **Environment**
   - Place devices in thermally stable environment (ambient 20–25 °C).
   - Attach power/USB hubs with current monitoring if available.
   - Start synchronized clocks (NTP) on automation host.
   - Duplicate `docs/release/templates/soak_devices.yaml` into `logs/soak_devices.yaml` with actual hostnames/IPs.

## Execution Steps

1. Start logging script: `python3 -m tools.validation.soak_telemetry --devices dev1,dev2,dev3 --output logs/soak_raw.csv`
2. Generate playlist schedule (per device): `python3 -m tools.validation.soak_playlist --manifest out/presets/manifest.json --output logs/soak_playlist.csv --dwell-seconds 300 --hours 24`
3. Launch preset playback on each device using the generated `logs/soak_playlist.csv` (apply staggered starts if needed).
4. Every 60 seconds capture metrics (automation or firmware push) with columns:
   `timestamp,device,heap_free,heap_min,frame_ms,temp_c,errors`
5. At hour 12 and hour 24 run `python3 -m tools.validation.prism_sanity --glob "device_exports/**/*.prism"`
6. After 24 hours stop playback, archive logs, and export final preset state for diffing.

## Post-Run Analysis

1. Summarize telemetry: `python3 -m tools.validation.soak_telemetry --input logs/soak_raw.csv --summary logs/soak_summary.json --fps 24`
2. Inspect output:
   - `max_fragmentation_pct <= 5`
   - `frame_p99_ms <= (1000 / fps) * 1.1`
   - `temp_max_c <= 65`
   - `error_count == 0`
3. Document results in the soak log template (see `docs/release/templates/soak_log_template.csv`) and complete `docs/release/soak_test_report.md`.
4. File any anomalies as release blockers with device, timestamp, preset ID, and raw log snippet.

## Deliverables

- `logs/soak_raw.csv` (raw telemetry)
- `logs/soak_summary.json` (script output)
- `logs/soak_playlist.csv` (generated playback schedule)
- Completed `soak_log_template.csv`
- Completed `docs/release/soak_test_report.md`
- Annotated photos or short clips verifying hardware status at start/end
- Note any firmware restarts or preset anomalies

## Contingencies

- **Fragmentation >5 %**: reboot device, rerun with reduced playlist to isolate preset causing heap churn.
- **Frame overruns**: confirm packaging sizes, rerun with frame profiler, consider lowering fps for offending preset.
- **Thermal limits**: increase airflow, reduce brightness via `--brightness` preview defaults, or throttle playback intervals.

Maintain this runbook alongside release notes so the process remains reproducible for future firmware spins.
