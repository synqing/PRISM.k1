# PRISM K1 Firmware v1.1 Release Workspace

This repository tracks the artefacts and tooling required to ship PRISM K1 firmware v1.1, including temporal sequencing tutorials, preset libraries, soak validation, and release documentation.

## Quick Start

```bash
# Generate preset library (JSON/CSV/PRISM + reports)
python3 -m tools.preset_library --output-dir out/presets
zip -jr out/presets_v1.1.zip out/presets

# Create a 24h soak playlist schedule (5 min per preset)
python3 -m tools.validation.soak_playlist \
  --manifest out/presets/manifest.json \
  --output logs/soak_playlist.csv \
  --dwell-seconds 300 --hours 24 --shuffle --seed 42

# Collect and summarise soak telemetry
python3 -m tools.validation.soak_telemetry --devices dev1,dev2,dev3 --output logs/soak_raw.csv --fps 24
python3 -m tools.validation.soak_telemetry --input logs/soak_raw.csv --summary logs/soak_summary.json --fps 24
```

## Key Resources
- Tutorials (scripts & production checklist): `docs/tutorials/`
- User manual: `docs/user-manual.md`
- Release notes (draft): `docs/release/v1.1_release_notes.md`
- Soak test runbook & report template: `docs/release/soak_test_runbook.md`, `docs/release/soak_test_report.md`
- OTA upgrade/rollback checklist: `docs/release/ota_validation_checklist.md`
- Preset library overview: `docs/release/preset_library_overview.md`
- UI/UX reference pack: `docs/ui/ADR_Reference_for_Figma.txt`, `docs/ui/PRISM_technical_specs.txt`

## Workspace Layout
- `docs/ui/` centralises designer-facing ADR excerpts and spec sheets previously in the repo root.
- `archive/2025-10/` captures historical handover notes, transient logs, and miscellaneous exports to keep the workspace lean.
- Future archival runs can reuse the same structure (e.g., `archive/2025-11/`) to snapshot each month.

## Tooling Highlights
- `tools/show_to_prism.py` now emits motion/sync metadata via `--motion-direction` and `--sync-mode` flags.
- `tools/prism_packaging.py` automatically quantises palettes to ≤64 colours before encoding `.prism` files.
- `tools/validation/soak_playlist.py` generates per-device playback schedules aligned with the new preset library.
- `scripts/cleanup.sh` moves fresh build logs, handover summaries, and misc task artefacts into a dated archive directory.

## Next Actions
- **Tutorials (Task 20.3):** Scripts ready in `docs/tutorials/scripts/`; record, edit, and publish the five-video series.
- **Soak Test (Tasks 20.4/20.5):** Use the generated playlist + telemetry tools to execute and document the 24h soak on three devices.
- **Release Wrap-up (Tasks 20.6–20.8):** Finalise preset bundle, populate soak/OTA evidence, update docs with video links, and publish the v1.1 release.
