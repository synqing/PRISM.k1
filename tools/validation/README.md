Hardware Validation (Task 16)
=============================

Goal
----
- Validate PROGRESSIVE temporal sequencing using a high‑speed camera.
- Confirm phi‑phenomenon thresholds (60–150ms) and motion direction.

Setup
-----
- Camera: Chronos 2.1 or equivalent at ≥240 FPS.
- Mount camera static, perpendicular to the LED strip(s).
- Record short clips for triangle and wedge shapes at different params.

Procedure
---------
1. Build and flash firmware with PROGRESSIVE patterns enabled.
2. Record 3–5 second clips for each shape/direction.
3. Transfer videos to this repo and run analyzer:
   - `python3 -m tools.validation.analyze_progressive --video path/to/clip.mp4 --leds 160`
4. Review timing report and visual overlay for delay gradients.

Artifacts
---------
- Save analyzer JSON results under `.taskmaster/reports/validation/`.
- Attach snapshots to TaskMaster via `task-master update-subtask`.

---

Soak Test Utilities (Task 20.4/20.5)
------------------------------------

### Playlist Generation
- `python3 -m tools.validation.soak_playlist --manifest out/presets/manifest.json --output logs/soak_playlist.csv --dwell-seconds 300 --hours 24`
- Produces a per-device playback schedule referencing packaged `.prism` files (default devices: dev1–dev3).
- Use `--shuffle` (with optional `--seed`) to randomise order, `--stagger-seconds` to offset device start times.

### Telemetry Collection
- `python3 -m tools.validation.soak_telemetry --devices dev1,dev2,dev3 --output logs/soak_raw.csv --fps 24`
- Append telemetry during the soak run, then summarise afterwards:
  - `python3 -m tools.validation.soak_telemetry --input logs/soak_raw.csv --summary logs/soak_summary.json --fps 24`

Remember to record playlist, raw telemetry, summary JSON, and the completed soak log/report templates for release evidence.
