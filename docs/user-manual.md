# PRISM K1 User Manual v1.1

## Temporal Sequencing Overview

PRISM K1 firmware v1.1 introduces temporal sequencing — the ability to stagger LED timing between the two edges of the Light Guide Plate.

### Sync Modes

- SYNC: Both edges illuminate simultaneously (v1.0 behavior)
- OFFSET: Fixed delay between edges
- PROGRESSIVE: Linear delay gradient across LEDs
- WAVE: Sinusoidal delays create flowing motion
- CUSTOM: Expert-defined per-LED timing curves

### Motion Directions

- LEFT (0→159)
- RIGHT (159→0)
- CENTER (middle outward)
- EDGE (edges inward)
- STATIC (no propagation)

### Getting Started

1. Power on the device and connect to the captive portal Wi‑Fi.
2. Select a built‑in effect from the dashboard (e.g., WAVE).
3. Adjust speed and amplitude to taste.

### Built-in Effects

- WAVE Single: LUT‑driven sinusoid across both channels (green demo)
- Palette Cycle: Simple GRB cycling with spatial gradient

### Notes

- WAVE uses precomputed `sin8` table for smooth motion with minimal CPU.
- Profiling can be enabled via `menuconfig → PRISM Playback → Enable temporal profiling`.
- Packaged presets (JSON/CSV/PRISM) live under `out/presets/` (zip archive: `out/presets_v1.1.zip`); use `python3 -m tools.prism_packaging` to create additional bundles.

### Resources

- 📽️ Tutorial series (10–15 min each): <!-- TODO: add YouTube playlist link once published -->
  - Video 1: Introduction to PRISM K1 and Temporal Sequencing
  - Video 2: Motion Directions Explained
  - Video 3: Sync Modes Deep Dive
  - Video 4: Creating Your First Pattern
  - Video 5: Advanced Techniques — WAVE & CUSTOM
- 📚 Release notes: `docs/release/v1.1_release_notes.md`
- 🧪 Soak test runbook: `docs/release/soak_test_runbook.md`
- 🧾 Soak report template: `docs/release/soak_test_report.md`
- 🔁 OTA validation checklist: `docs/release/ota_validation_checklist.md`
- 🎛️ Automation scripts:
  - Playlist generator: `python3 -m tools.validation.soak_playlist --help`
  - Telemetry summariser: `python3 -m tools.validation.soak_telemetry --help`

## Developer Diagnostics

Advanced (optional): Enable temporal profiling and export live metrics for validation and soak tests.

- Enable profiling: `menuconfig → Components → PRISM Playback → Enable temporal profiling`
- Choose counters: enable D$/I$ hit/miss and instruction count as needed
- HTTP export (when enabled in Kconfig):
  - JSON: `GET /metrics/wave`
  - Prometheus: `GET /metrics` (text exposition)
  - CSV: `GET /metrics.csv`
- CLI export: run `prism_metrics` in the console to print a snapshot
- Optional background push: enable in Kconfig and set URL + interval

These developer features are intended for testing and should be disabled in production builds.
