# Migration Playbook (v1.0 → v1.1)

## Scope

Upgrade legacy `.prism` assets, preview workflows, and documentation to the v1.1 toolchain without disrupting downstream teams.

## Requirements

- Python 3.11+
- `pip install -r tools/requirements.txt`
- Access to legacy asset directory (`legacy_presets/`)

## Step 1: Convert Legacy Packages

```bash
find legacy_presets -name "*.prism" -maxdepth 1 -print0 \
  | xargs -0 -I{} python tools/migrate_prism.py {} migrated_presets/{}/
```

- Idempotent: running on v1.1 files is a no-op.
- Adds default motion/sync metadata per R2.1.

## Step 2: Re-export with Updated Tooling

Regenerate JSON/CSV using palette/show CLIs (optional but recommended for deterministic metadata):

```bash
python3 -m tools.palette_to_prism --palette "#ff0000,#0000ff" --led-count 160 \
  --output refreshed/palette.json --csv refreshed/palette.csv

python3 -m tools.show_to_prism --show sine --palette "#ff0000,#0000ff" \
  --led-count 160 --duration 6 --fps 24 --output refreshed/sine.json
```

## Step 3: Package for Distribution

```bash
python3 -m tools.prism_packaging --input refreshed/sine.json --output refreshed/sine.prism
```

Validate assets:

```bash
python3 -m tools.validation.prism_sanity --glob "refreshed/**/*.prism"
```

## Step 4: Preview & QA

- **Terminal**: `python3 -m tools.previews.terminal_preview --input refreshed/sine.json --static`
- **HTML**: `python3 -m tools.previews.html_preview --input refreshed/sine.json --output previews/sine.html`
- Attach previews to design reviews/docs.

## Step 5: Update Documentation

- Link new assets in the user manual.
- Append migration status to release notes (see template).
- Archive old presets with `_legacy` suffix.

## Step 6: Sign-off Checklist

- [ ] All `.prism` files validated via `prism_sanity`
- [ ] Preview diffs reviewed (terminal + HTML)
- [ ] Preset manifest updated (`tools/preset_library --output-dir ...`)
- [ ] QA signature captured in soak log template

Keep this playbook version-controlled alongside tooling changes so regressions are easy to track.
