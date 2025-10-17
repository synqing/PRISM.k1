# PRISM Firmware v1.1 Release Notes (Template)

## Overview

- **Release Date:** <YYYY-MM-DD>
- **Firmware Tag:** `firmware-v1.1`
- **Tooling Tag:** `<commit hash>`
- **Maintainers:** <names>

## Highlights

- ✅ HSLuv palette tooling with deterministic JSON/CSV writers
- ✅ Show engine (sine | noise | flow) with reproducible seeds
- ✅ Terminal + HTML preview pipeline aligned with R3.5 mapping guidance
- ✅ `.prism` v1.1 packaging prototype (palette + indices + CRC32)
- ✅ Migration toolkit (`tools/migrate_prism.py`) for v1.0 assets

## Breaking Changes

- Describe any incompatible metadata or tooling requirements.
- Note minimum firmware/tool versions required for contributions.

## Tooling Workflow

| Step | Command | Notes |
| --- | --- | --- |
| Palette Authoring | `python3 -m tools.palette_to_prism ...` | Ramp spaces: hsv/hsl/hsluv |
| Show Generation | `python3 -m tools.show_to_prism ...` | Deterministic seeds & metadata |
| Packaging | `python3 -m tools.prism_packaging ...` | Outputs `.prism` with CRC |
| Preview | `python3 -m tools.previews.terminal_preview ...` | Quick CLI preview |
| Preset Library | `python3 -m tools.preset_library ...` | Builds release-ready bundles |

## Validation Summary

- **Soak Test:** <insert summary.> Attach `logs/soak_summary.json`.
- **Bench Results:** <link to `firmware/components/tests/test_decode_microbench` output>
- **Preview Compliance:** Verified default gamma/brightness/saturation.

## Migration Steps

1. Convert legacy `.prism` v1.0 assets: `python3 -m tools.migrate_prism <input> <output>`
2. Re-export presets using `tools/preset_library.py` or the CLI duo (show → package).
3. Run `python3 -m tools.validation.prism_sanity --glob "out/**/*.prism"`.
4. Update docs/screenshots referencing the new preview pipeline.

## Assets

- 📦 Preset bundle: `<link>`
- 🎬 Tutorial series: `<link>`
- 📚 User manual: `<link>`
- 🧪 Soak logs: `<link>`

## Known Issues / Follow-ups

- <List open tickets or post-release tasks>

## Acknowledgements

- <Contributors, QA, PM>

---

> Replace placeholders, attach raw evidence artefacts, and archive this document under `docs/release/archive/` after publication.
