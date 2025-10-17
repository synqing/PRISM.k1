# Video 4 Script — Creating Your First Temporal Sequencing Pattern

## Runtime Target
- 15 minutes (aim for 14:45 finished)

## Segment Outline
1. **Intro (0:00–0:40)** — Recap of what viewers have learned; promise a full walkthrough from blank template to hardware playback.
2. **Project setup (0:40–2:30)** — Show cloning repo (if needed), creating `shows/first_pattern/` workspace, installing dependencies (`pip install -r tools/requirements.txt`).
3. **Baseline SYNC pattern (2:30–5:30)** — Run `python3 -m tools.show_to_prism --show sine --motion-direction LEFT_ORIGIN --sync-mode SYNC ...`. Explain each parameter.
4. **Preview & iterate (5:30–7:30)** — Use terminal preview and HTML preview to sanity check. Mention brightness adjustments.
5. **Add OFFSET (7:30–9:30)** — Re-run CLI with `--sync-mode OFFSET --sync-offset-ms 80`. Compare terminal preview vs hardware.
6. **Switch to PROGRESSIVE (9:30–11:30)** — Add `--sync-mode PROGRESSIVE --sync-progressive-start-ms 0 --sync-progressive-end-ms 120`. Discuss visual effect.
7. **Metadata review (11:30–12:30)** — Open JSON, highlight `meta.motion`, `meta.sync`. Show integration with manifest (`out/presets/manifest.json`).
8. **Upload & test (12:30–14:00)** — Push to device, film playback, note frame rate and brightness.
9. **Wrap-up (14:00–15:00)** — Encourage experimentation, point to advanced WAVE/CUSTOM video.

## Script Notes
- Speak to beginners while keeping technical detail (call out defaults, CLI help).
- Emphasize iterative workflow: generate -> preview -> tweak -> upload.
- Mention Git versioning for presets.
- Encourage using README checklist before sharing patterns.

## Shot List
- 00:00–00:40: Presenter on-camera or voiceover over recap slide.
- 00:40–02:30: Desktop capture showing repo clone and environment setup.
- 02:30–05:30: Terminal close-up running baseline command; overlay callouts for flags.
- 05:30–07:30: Terminal preview followed by HTML preview in browser; picture-in-picture hardware check.
- 07:30–11:30: Split screen comparing OFFSET and PROGRESSIVE outputs (CLI vs hardware).
- 11:30–12:30: JSON metadata view with animated highlights.
- 12:30–14:00: Dashboard upload process and hardware playback.
- 14:00–15:00: Outro slide listing next steps and advanced video teaser.

## Narration Script (timed)
- **Intro:** "By now you’ve seen what motion and sync can do. Let’s build a pattern from scratch and deploy it to hardware."
- **Setup:** "Start by cloning the tools repo, or pull latest if you already have it. I’m creating a `shows/first_pattern` folder to keep assets versioned. Install dependencies with `pip install -r tools/requirements.txt` so we get hsluv and preview utilities."
- **Baseline pattern:** "Here’s our first command. I’m using the sine engine, left-origin motion, and SYNC mode. The palette is a warm sunrise gradient. Notice the metadata defaults for temporal sequencing."
- **Preview:** "Run the terminal preview to confirm the sweep. For documentation or sharing, generate an HTML preview too. Adjust brightness if your panel clips highlights."
- **OFFSET iteration:** "Let’s stagger the edges. Add `--sync-mode OFFSET --sync-offset-ms 80`. The terminal preview shows the delay; on hardware you’ll see the second edge chasing the first."
- **PROGRESSIVE iteration:** "For a softer reveal, switch to PROGRESSIVE. I’m stretching from zero to 120 ms. Pairing this with the left origin direction creates a cascading ribbon."
- **Metadata review:** "Open the JSON and review `meta.motion` and `meta.sync`. When you add presets to a manifest, these fields travel with the asset."
- **Upload:** "Use the dashboard uploader or CLI to push the preset. After upload, verify frame rate with the on-device stats overlay."
- **Wrap-up:** "You’ve built a complete temporal pattern. Commit your work, document it, and get ready for the advanced WAVE and CUSTOM tricks in the next video."

## Demo Command Cheatsheet
```bash
# Baseline SYNC pattern
python3 -m tools.show_to_prism --show sine --palette "#ff6f61,#ffd54f,#00bcd4" \
  --motion-direction LEFT_ORIGIN --sync-mode SYNC --led-count 160 \
  --duration 6 --fps 24 --output shows/first_pattern/sync.json \
  --csv shows/first_pattern/sync.csv --ramp-space hsluv

# OFFSET iteration
python3 -m tools.show_to_prism --show sine --palette "#ff6f61,#ffd54f,#00bcd4" \
  --motion-direction LEFT_ORIGIN --sync-mode OFFSET --sync-offset-ms 80 \
  --led-count 160 --duration 6 --fps 24 --output shows/first_pattern/offset.json \
  --csv shows/first_pattern/offset.csv --ramp-space hsluv

# PROGRESSIVE iteration
python3 -m tools.show_to_prism --show sine --palette "#ff6f61,#ffd54f,#00bcd4" \
  --motion-direction LEFT_ORIGIN --sync-mode PROGRESSIVE \
  --sync-progressive-start-ms 0 --sync-progressive-end-ms 120 \
  --led-count 160 --duration 6 --fps 24 --output shows/first_pattern/progressive.json \
  --csv shows/first_pattern/progressive.csv --ramp-space hsluv

# HTML preview
python3 -m tools.previews.html_preview --input shows/first_pattern/progressive.json \
  --output shows/first_pattern/progressive.html
```

## Demo Preparation
- Prepare workspace folder with gitignore entry for large assets.
- Ensure device is flashed with v1.1 firmware and connected to Wi-Fi.
- Preload dashboard in browser for quick upload.
- Have brightness knob or software control ready for camera adjustments.

## Visual Assets
- CLI flag callouts for each step.
- Animated comparison between SYNC, OFFSET, PROGRESSIVE.
- Metadata highlight overlays.
- Upload checklist slide.

## Audio Cues
- Subtle click or swoosh when switching modes.
- Lower music bed during CLI explanation for clarity.

## Recording Checklist
- [ ] Terminal theme consistent with previous videos.
- [ ] Browser zoom set so upload UI elements are legible on screen capture.
- [ ] Camera exposure locked before filming hardware segments.
- [ ] HTML preview loads locally without security prompts.
- [ ] Example files cleaned up or committed after recording to avoid workspace clutter.

## Post Production Checklist
- Add chapters: Setup, SYNC, OFFSET, PROGRESSIVE, Upload, Wrap-up.
- Include Git commands and preset paths in description.
- Verify hardware footage aligns with narration (no mismatched patterns).
- Export 1080p 30fps, match intro/outro style from earlier videos.
