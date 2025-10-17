# PRISM Tools

This folder contains developer‑side utilities to assist with authoring and migration of pattern assets.

## Palette → PRISM (Authoring)

Script: `tools/palette_to_prism.py`

Purpose
- Turn a list of input colors (HEX) into interpolated LED payloads
- Interpolate in HSV/HSL and emit RGB arrays (and optional RGBW) for presets
- Output as JSON (and optional CSV) for ingestion into your content pipeline

Install dependencies
```
pip install -r tools/requirements.txt
python -m tools.check_deps
```

Usage
```
# Two‑stop gradient (red→blue), 160 LEDs, JSON output (HSLuv default)
python tools/palette_to_prism.py \
  --palette "#ff0000,#0000ff" --led-count 160 \
  --output out/red_to_blue.json

# Multi‑stop palette, HSV interpolation, include RGBW, also write CSV
python tools/palette_to_prism.py \
  --palette "#ff0000,#00ff00,#0000ff" --led-count 160 \
  --space hsv --include-rgbw \
  --csv out/palette.csv --output out/palette.json
```

Outputs
- JSON: `{ "version": "1.0", "meta": {...}, "data": { "rgb": [[r,g,b],...], "rgbw": [[r,g,b,w],...] } }`
- CSV: first line `# meta: {...}` followed by header `r,g,b` or `r,g,b,w` with one LED per line

Key options
- `--seed <value>` records a random seed (decimal or hex) and seeds the RNG for reproducible downstream steps.
- `--timestamp <iso8601>` overrides the embedded `generated_at` metadata (defaults to current UTC time; useful for deterministic fixtures).
- `--software-version <tag>` sets the `software_version` metadata field (defaults to `prism-tools/0.1.0`).

Metadata emitted by default includes palette stops, ramp space, LED count, RGBW inclusion flag, software version, timestamp, and any provided seed, matching the R2.1 metadata extensions.

Notes
- Interpolation uses HSLuv by default with hue wrap (shortest path). HSV/HSL are available via `--space`.
- RGBW tuples are computed via the library’s HSI→RGBW mapping and are intended
  for authoring workflows. Current PRISM firmware targets RGB hardware; keep RGBW
  for potential RGBW hardware SKUs or offline preview tools.

Future directions
- Add an option to package payloads into `.prism` v1.1 containers once payload
  layout is finalized (CRC and header helpers exist on firmware side).

## Tooling Environment Setup

1. Create a virtual environment (optional but recommended):
   ```
   python -m venv .venv
   source .venv/bin/activate
   ```
2. Install Python dependencies:
   ```
   pip install -r tools/requirements.txt
   ```
3. Install the `aha` CLI (ANSI → HTML adapter) via your package manager, for example:
   ```
   brew install aha          # macOS
   sudo apt-get install aha  # Debian/Ubuntu
   ```
4. Verify everything is wired up:
   ```
   python -m tools.check_deps
   ```
   The script confirms that required Python libraries (`hsluv`, `rgbw-colorspace-converter`,
   `colr`, `ansi2html`) import correctly and that the `aha` binary is on your PATH.

## Migration (v1.0 → v1.1)

Script: `tools/migrate_prism.py`

Purpose
- Update legacy `.prism` (v1.0) headers to v1.1 and append default temporal metadata.

Usage
```
python tools/migrate_prism.py input_v10.prism output_v11.prism
```

## Preview Validation Checklists (stub)

These will be used once preview tasks land:

- Terminal preview (Task #26)
  - Defaults applied: gamma=2.2, brightness=0.85, saturation=0.9
  - CLI flags override: `--gamma`, `--brightness`, `--saturation`, `--fps`
  - Deterministic ANSI fixtures generated from sample palette/show
  - Mapping matches R3.5 guidance; visual sanity check on light/dark terminals
  - Example: `prism-preview --palette "#ff0000,#0000ff" --led-count 160 --output out/preview.ansi`

- HTML preview (Task #27)
  - HTML mirrors terminal mapping exactly (no external assets/network)
  - Deterministic HTML fixture generated from the same inputs as terminal preview
  - Visual parity check vs ANSI output; document any known rendering differences
  - Example: `prism-preview-html --from out/preview.ansi --output out/preview.html`

## Terminal Preview CLI

Render palette/show payloads directly in the terminal:

```
python -m tools.previews.terminal_preview \
  --input out/red_to_blue.json \
  --width 80 --fps 24 \
  --gamma 2.2 --brightness 0.85 --saturation 0.9
```

Options:
- `--static` renders only the first frame (handy for docs/tests).
- `--loop` keeps playback running until interrupted.
- `--respect-tty` forces static mode when stdout is not a TTY.
- `--block` customises the glyph (defaults to `"█"`).

The CLI consumes the `palette_to_prism` JSON structure (`version`, `meta`, `data.rgb`) as well as future show payloads (`data.frames`) and applies the R3.5 preview defaults unless overridden.

## HTML Preview CLI

Generate standalone HTML snapshots that mirror the terminal preview:

```
python -m tools.previews.html_preview \
  --input out/red_to_blue.json \
  --output out/red_to_blue.html \
  --width 80 \
  --gamma 2.2 --brightness 0.85 --saturation 0.9
```

Details:
- Uses `ansi2html` when available, falling back to the `aha` CLI or a plain-text rendering.
- Embeds metadata at the top of the HTML file (`<!-- meta: ... -->`) describing the source payload and preview parameters.
- Output is a self-contained HTML document with inline styles—no external assets required—so it can be dropped directly into docs.

## Show Generation CLI

Create deterministic show sequences for the launch families:

```
python -m tools.show_to_prism \
  --show sine \
  --palette "#ff0000,#00ff00,#0000ff" \
  --led-count 160 \
  --duration 8 --fps 24 \
  --output out/sine_show.json \
  --csv out/sine_show.csv \
  --seed 1234
```

Key options:
- `--show sine|noise|flow` selects the family defined in R3.3.
- Common flags: `--palette`, `--led-count`, `--duration`, `--fps`, `--ramp-space`, `--seed`, `--software-version`.
- Wave params: `--wave-amplitude`, `--wave-frequency`, `--wave-speed`, `--wave-direction`, `--wave-phase`.
- Noise params: `--noise-scale`, `--noise-speed`, `--noise-octaves`, `--noise-persistence`, `--noise-lacunarity`.
- Flow params: `--flow-field-scale`, `--flow-step-size`, `--flow-speed`, `--flow-curl`, `--flow-octaves`, `--flow-persistence`, `--flow-lacunarity`.

Outputs mirror the palette tooling: JSON contains `version/meta/data.frames`, and optional CSV lists `frame,led,r,g,b` rows with metadata embedded in the header comment.

## Packaging CLI (v1.1)

Convert PRISM JSON payloads (palette or show frames) into a `.prism` v1.1 container using palette+indices with optional XOR delta and simple RLE. Header/CRC rules align with R2.1/R2.2.

```
python -m tools.prism_packaging \
  --input out/sine_show.json \
  --output out/sine_show.prism \
  --compression xor_rle \
  --palette-size 64 \
  --report out/sine_show_pack_report.json
```

Details:
- Encoding: global palette (≤64 colors) + 8-bit indices per LED.
- Compression options: `none`, `xor` (delta across frames), `rle` (run‑length), `xor_rle` (RLE on XOR delta).
- Report: optional JSON with size stats and the header manifest (fields covered by CRC and optional metadata).
- Validation: run header checks with the parser testbed:
  - `python -m tools.parser_testbed.runner regress --input-dir tools/parser_testbed/vectors` (goldens)
  - Or parse the generated header programmatically using `tools.parser_testbed.builder`.

Note: Payload layout is a tooling concern here (palette, frames, lengths). Firmware integration focuses on header correctness and decode complexity targets from R1.1; Agent 1’s bench harness validates decode performance on device when available.

## .prism Packaging CLI

Wrap palette/show JSON into `.prism` v1.1 containers:

```
python -m tools.prism_packaging \
  --input out/sine_show.json \
  --output out/sine_show.prism \
  --report out/sine_show_report.json
```

- Encodes frames using palette+indices with per-frame XOR deltas (when sparse) and simple RLE (when runs ≥4 occur).
- Validates palette size (≤64), rebuilds header/meta via parser testbed primitives, and appends payload CRC32.
- Outputs stats/report JSON including compression ratio, bytes/frame, palette size, encode/decode timings, and round-trip hashes.
- Ideal flow: `show_to_prism` → `prism_packaging` → `tools.validation.prism_sanity` → bench/preview.

## Preset Library Builder

Generate the release preset bundle and manifest:

```
python -m tools.preset_library \
  --output-dir out/presets
```

- Produces JSON/CSV pairs, optional `.prism` binaries, and `manifest.(json|csv)`.
- Definitions live in `tools/preset_library.PRESET_LIBRARY`—tweak palettes/parameters as designs evolve.
- Integrates with soak-test and release workflows (see docs in `docs/release/`).
