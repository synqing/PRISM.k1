# Video 3 Script — Sync Modes Deep Dive

## Runtime Target
- 15 minutes (target 14:30 final cut)

## Segment Outline
1. **Opening teaser (0:00–0:30)** — Side-by-side playback of SYNC vs PROGRESSIVE vs WAVE vs CUSTOM with overlay "Which sync mode fits your show?"
2. **Architecture refresher (0:30–1:45)** — Slide showing dual edge timing table. Explain delay slots, phi phenomenon (60–150 ms).
3. **Sync mode overview (1:45–3:00)** — Table listing SYNC, OFFSET, PROGRESSIVE, WAVE, CUSTOM with intended effects.
4. **Instrumentation setup (3:00–4:00)** — Show `tools/show_to_prism` parameters for sync, plus telemetry hooks (`meta.sync.*`). Highlight default values.
5. **Mode walkthroughs (4:00–12:30)** — For each mode:
   - Show CLI command with relevant flags (`--sync-mode OFFSET --sync-offset-ms 80`, etc.).
   - Display JSON metadata snippet.
   - Use dual-camera layout: top = Edge A, bottom = Edge B, with time markers.
   - Provide explanation of viewer perception and best practices.
   - Mention interactions with motion direction (e.g., combine PROGRESSIVE with EDGE_ORIGIN to create expanding bloom).
6. **CUSTOM editor demo (12:30–13:45)** — Screen capture `prism-editor` delay map. Show exporting custom sync table and verifying metadata.
7. **Troubleshooting (13:45–14:30)** — Common pitfalls (overlapping delays, exceeding frame budget, unbalanced brightness) plus quick fixes.
8. **Outro (14:30–15:00)** — Point to soak test runbook for validating timing, preview next video focusing on building first pattern.

## Script Notes
- Emphasize that sync modes modify per-edge timing, not color data.
- Include practical numbers (e.g., OFFSET 80 ms for marching effect, PROGRESSIVE gradient used in triangle shape).
- Reinforce QA metrics: frame budget, heap fragmentation.
- Reference ADR-010 for further reading.

## Shot List
- 00:00–00:30: Four-way split hardware footage; overlay mode labels.
- 00:30–01:45: Slide deck explaining timing diagram; animate edge offsets.
- 01:45–03:00: Table slide with bullet overlay for each mode.
- 03:00–04:00: Terminal capture highlighting sync flags; JSON and telemetry overlay.
- 04:00–12:30: For each mode: CLI execution, metadata callout, dual-camera hardware view.
- 12:30–13:45: Screen recording from `prism-editor`, showing delay map creation and export.
- 13:45–14:30: Overlay of troubleshooting checklist while showing telemetry plots.
- 14:30–15:00: Outro slide with link to soak runbook and next video preview.

## Narration Script (timed)
- **Teaser:** "Temporal sequencing is more than direction—the sync mode sets edge timing. Watch how SYNC, PROGRESSIVE, WAVE, and CUSTOM change the feel."
- **Architecture refresher:** "Each PRISM K1 edge has its own delay table. Temporal sequencing schedules those delays per frame. A 60 to 150 millisecond offset is enough to trigger the phi phenomenon—our eyes see motion even with static frames."
- **Overview slide:** "SYNC fires both edges simultaneously. OFFSET applies a constant delay. PROGRESSIVE creates a gradient from start to end. WAVE oscillates delays for organic flow, and CUSTOM hands you complete control via delay tables."
- **Instrumentation:** "When we call `show_to_prism`, sync parameters live alongside motion direction. The generated JSON places them in `meta.sync`. Our telemetry script watches fragmentation, frame p99, and temperature so you know the mode performs well."
- **SYNC demo:** "Here’s the baseline—`--sync-mode SYNC`. Watch both edges illuminate together. Use this for static reveals or when motion direction already gives you movement."
- **OFFSET demo:** "Adding `--sync-mode OFFSET --sync-offset-ms 80` staggers the second edge by 80 milliseconds. Great for marching band style chases."
- **PROGRESSIVE demo:** "`--sync-mode PROGRESSIVE --sync-progressive-start-ms 0 --sync-progressive-end-ms 120` creates a ramped delay. Combine with EDGE_ORIGIN for expanding diamonds."
- **WAVE demo:** "WAVE turns the delay table into a sine wave. Here I am using amplitude 90 ms and frequency 0.6 Hz. It’s dynamic, but keep an eye on frame budget."
- **CUSTOM demo:** "When you need something unique, export a delay map from the editor and load it with `--sync-mode CUSTOM --sync-custom-file custom_delay.json`. Validate the metadata after export."
- **Troubleshooting:** "If frames stutter, reduce amplitude or shorten offsets. Fragmentation above five percent? Trim palette size or segment the preset. Our soak telemetry should stay green before you ship."
- **Outro:** "You now have full timing control. In the next video we’ll build a preset from scratch and apply these modes end to end."

## Demo Command Cheatsheet
```bash
# OFFSET example
python3 -m tools.show_to_prism --show sine --palette "#ff4500,#ffd700,#00bcd4" \
  --motion-direction LEFT_ORIGIN --sync-mode OFFSET --sync-offset-ms 80 \
  --led-count 160 --duration 6 --fps 24 \
  --output out/tutorial_sync/offset.json --csv out/tutorial_sync/offset.csv --ramp-space hsluv

# PROGRESSIVE example
python3 -m tools.show_to_prism --show sine --palette "#ff4500,#ffd700,#00bcd4" \
  --motion-direction EDGE_ORIGIN --sync-mode PROGRESSIVE \
  --sync-progressive-start-ms 0 --sync-progressive-end-ms 120 \
  --led-count 160 --duration 6 --fps 24 \
  --output out/tutorial_sync/progressive.json --csv out/tutorial_sync/progressive.csv --ramp-space hsluv

# WAVE example
python3 -m tools.show_to_prism --show sine --palette "#ff4500,#ffd700,#00bcd4" \
  --motion-direction CENTER_ORIGIN --sync-mode WAVE \
  --sync-wave-amplitude-ms 90 --sync-wave-frequency-hz 0.6 --sync-wave-phase-deg 45 \
  --led-count 160 --duration 6 --fps 24 \
  --output out/tutorial_sync/wave.json --csv out/tutorial_sync/wave.csv --ramp-space hsluv

# CUSTOM example (assuming custom_delay.json exists)
python3 -m tools.show_to_prism --show sine --palette "#ff4500,#ffd700,#00bcd4" \
  --motion-direction LEFT_ORIGIN --sync-mode CUSTOM \
  --sync-custom-file assets/custom_delay.json \
  --led-count 160 --duration 6 --fps 24 \
  --output out/tutorial_sync/custom.json --csv out/tutorial_sync/custom.csv --ramp-space hsluv
```

## Demo Preparation
- Prepare presets in `out/tutorial_sync/` covering each mode.
- Set up scope or logging overlay if available to visualize delay (optional).
- Ensure hardware camera framing clearly shows both edges simultaneously.
- Pre-configure `prism-editor` with example delay map for CUSTOM segment.

## Visual Assets
- Timing diagram overlay with adjustable arrows indicating delay.
- Mode comparison table graphic.
- Highlight animations showing difference between frame budget compliance vs overrun.

## Audio Cues
- Use subtle ticking sound under timing diagrams (keep low volume).
- Pause narration briefly to let viewers observe hardware playback.

## Recording Checklist
- [ ] `out/tutorial_sync/` populated with sample presets for each mode.
- [ ] Dual-camera rig aligned to show Edge A and Edge B simultaneously.
- [ ] `prism-editor` project with custom delay map saved and tested.
- [ ] Telemetry overlay or logs ready for troubleshooting segment.
- [ ] Slide deck updated with timing diagrams and tables.

## Post Production Checklist
- Add on-screen captions for each CLI command.
- Sync B-roll audio to narration references (mute background noise).
- Update description with links to sync section of user manual and ADR-010.
- Include chapter markers: Architecture, Mode walkthroughs, Custom editor, Troubleshooting.
