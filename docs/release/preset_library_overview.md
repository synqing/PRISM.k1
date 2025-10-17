# v1.1 Preset Library Overview

This document captures the current state of the preset library generated for the v1.1 release.

## Generation Command

```bash
python3 -m tools.preset_library --output-dir out/presets
```

- 21 presets spanning `sine`, `noise`, and `flow` families.
- Each preset directory now contains JSON, CSV, packaged `.prism`, and a quantisation report (`*_report.json`).
- `manifest.json` and `manifest.csv` summarise metadata, led counts, fps, and parameters for the entire library.

## Quantisation Notes

- `tools/prism_packaging.py` now performs automatic palette reduction (≤64 colours) before encoding.
- Reports include `palette_colors_before`, `palette_colors_after`, and `quantized` flags—use them to spot high-variance presets.
- Install dependencies via `pip install -r tools/requirements.txt` to keep `hsluv` interpolation active; otherwise the CLI falls back to HSV.

## Outstanding Actions

- Zip the final library (`out/presets_v1.1.zip`) alongside reports for release distribution.
- Capture payload previews (terminal + HTML) for README and release notes.
- Smoke-test a representative subset on hardware to confirm temporal metadata and quantised colours match expectations.

## Validation Checklist

- [x] Run `python3 -m tools.validation.prism_sanity --glob "out/presets/**/*.prism"`.
- [ ] Spot-check representative presets on hardware for temporal metadata correctness.
- [ ] Capture preview assets (terminal + HTML) for README and release notes.

> Keep this overview updated as packaging issues are resolved and release artefacts are finalised.
