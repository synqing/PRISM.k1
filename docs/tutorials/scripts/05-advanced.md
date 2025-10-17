# Video 5 Script — Advanced Techniques with WAVE and CUSTOM Modes

## Runtime Target
- 15 minutes (target 14:30 final)

## Segment Outline
1. **Hook (0:00–0:20)** — Showcase dramatic WAVE and CUSTOM presets. Text overlay: "Unlock the full temporal engine."
2. **Quick recap (0:20–1:10)** — Summarize what viewers built in Video 4 and set expectations for advanced control.
3. **WAVE fundamentals (1:10–3:30)** — Slide covering LUT-based sine delays, parameters (`wave_amplitude`, `wave_frequency`, `wave_phase`). Mention profiling toggle in firmware (`menuconfig → PRISM Playback → Enable temporal profiling`).
4. **WAVE demo (3:30–8:00)** — Steps:
   - Generate preset with `--sync-mode WAVE --wave-amplitude-ms 90 --wave-frequency-hz 0.6 --wave-phase-deg 45`.
   - Explain how amplitude impacts delay spread; frequency affects oscillation speed.
   - Preview in terminal and HTML, then show hardware playback with close-ups.
   - Discuss performance considerations (keep frame budget margin).
5. **Transition to CUSTOM (8:00–9:00)** — Explain when to use CUSTOM (bespoke choreography, multi-stage reveals).
6. **CUSTOM workflow (9:00–12:30)** — Demonstrate:
   - Using `prism-editor` to draw per-edge delay curve.
   - Exporting delay map JSON and feeding into `tools.show_to_prism` via `--sync-mode CUSTOM --sync-custom-file custom_delay.json`.
   - Validating metadata and ensuring values remain within allowed range.
7. **Performance guardrails (12:30–13:30)** — Reference soak telemetry thresholds, heap fragmentation, need for segmentation in large presets.
8. **Troubleshooting checklist (13:30–14:15)** — Tips for stutters, brightness imbalance, metadata mismatches.
9. **Series outro (14:15–15:00)** — Thank viewers, recap deliverables (tutorial playlist, docs, release notes), call to action to run soak test and share presets.

## Script Notes
- Keep explanations anchored to practical numbers (e.g., amplitude in ms, frequency in Hz).
- Mention that WAVE defaults rely on `hsluv`; remind viewers to install dependencies.
- Reinforce that CUSTOM mode requires thorough QA and should log telemetry.
- Encourage viewers to contribute presets back to community (tie-in with future roadmap).

## Shot List
- 00:00–00:20: High-energy montage of WAVE and CUSTOM presets; overlay text.
- 00:20–01:10: Presenter on camera or voiceover with recap slide.
- 01:10–03:30: Slide deck illustrating waveform and parameter definitions.
- 03:30–08:00: Terminal capture for WAVE command, metadata view, terminal preview, HTML preview, hardware macro footage.
- 08:00–12:30: `prism-editor` screen recording showing custom delay creation, CLI execution, hardware playback.
- 12:30–13:30: Telemetry dashboard or soak summary overlay (even if placeholder).
- 13:30–14:15: Overlay troubleshooting checklist while showing b-roll of logs.
- 14:15–15:00: Outro slide thanking viewers and pointing to soak runbook + preset overview.

## Narration Script (timed)
- **Hook:** "This is where temporal sequencing shines—WAVE and CUSTOM sync modes unlock fluid motion and bespoke choreography."
- **Recap:** "Previously we built a complete pattern with SYNC, OFFSET, and PROGRESSIVE. Now we’ll shape timing curves and wrap the series."
- **WAVE fundamentals:** "WAVE mode uses a sine lookup table to offset each frame. Amplitude defines the maximum delay in milliseconds. Frequency controls how fast the wave oscillates, and phase rotates where the wave begins."
- **WAVE demo:** "Let’s generate a preset with amplitude 90 ms, frequency 0.6 Hz, and phase 45 degrees. The terminal preview shows the delay rippling across both edges. On hardware, the motion feels fluid. Keep an eye on frame budget; high amplitudes can stress rendering."
- **Transition to CUSTOM:** "When choreography demands precision—spelling letters, syncing to music—CUSTOM mode lets you define every edge delay manually."
- **CUSTOM workflow:** "In `prism-editor`, sketch the delay curve and export `custom_delay.json`. Feed it into `show_to_prism` with `--sync-mode CUSTOM`. Inspect the JSON to confirm the delay table, then upload and verify timing on hardware."
- **Performance guardrails:** "Advanced modes demand telemetry. Fragmentation should stay under five percent, frame p99 under 1.1 times your frame budget, and thermals under 65 degrees. Run the soak test script before you ship."
- **Troubleshooting:** "If the wave stutters, reduce amplitude or shorten duration. For CUSTOM, ensure delays stay within firmware limits and avoid abrupt jumps. Re-run `prism_sanity` if packaging complains."
- **Outro:** "You now have the full toolkit—migration CLI, user manual, preset library, and this video series. Run the soak test, package your presets, and share the results with the community. Thanks for tuning in."

## Demo Command Cheatsheet
```bash
# WAVE preset
python3 -m tools.show_to_prism --show sine --palette "#a45fff,#27c2ff,#00ffb8" \
  --motion-direction CENTER_ORIGIN --sync-mode WAVE \
  --sync-wave-amplitude-ms 90 --sync-wave-frequency-hz 0.6 --sync-wave-phase-deg 45 \
  --led-count 160 --duration 8 --fps 24 \
  --output out/tutorial_wave/wave.json --csv out/tutorial_wave/wave.csv --ramp-space hsluv

# CUSTOM preset (assumes custom_delay.json)
python3 -m tools.show_to_prism --show sine --palette "#ff8a65,#fdd835,#7cb342" \
  --motion-direction EDGE_ORIGIN --sync-mode CUSTOM \
  --sync-custom-file out/tutorial_custom/custom_delay.json \
  --led-count 160 --duration 8 --fps 24 \
  --output out/tutorial_custom/custom.json --csv out/tutorial_custom/custom.csv --ramp-space hsluv

# Telemetry summary (for footage)
python3 -m tools.validation.soak_telemetry --input logs/soak_raw.csv \
  --summary logs/soak_summary.json --fps 24
```

## Demo Preparation
- Precompute WAVE preset in `out/tutorial_wave/` and CUSTOM preset in `out/tutorial_custom/`.
- Prepare `custom_delay.json` with smooth curve; keep a fallback copy.
- Enable profiling in firmware build for overlay logging (optional B-roll).
- Mount device for clear macro shots; consider slow-motion capture for highlight reels.

## Visual Assets
- Parameter overlay showing amplitude/frequency/phase adjustments in real time.
- Graph animation of delay curve as waveform.
- Callout boxes for profiling metrics (heap, frame p99).

## Audio Cues
- Use subtle pulsing bed under WAVE demo; fade out during narration to keep clarity.
- Pause narration briefly to let viewers observe CUSTOM choreography.

## Recording Checklist
- [ ] Generate WAVE and CUSTOM presets ahead of time and verify on hardware.
- [ ] `custom_delay.json` exported and validated; backup stored.
- [ ] Profiling logs enabled if showcasing performance metrics.
- [ ] Camera exposure locked for macro LED shots to avoid flicker.
- [ ] Slide deck finalized with waveform graphics and performance tables.
- [ ] Telemetry data (real or illustrative) prepared for overlay.

## Post-shoot Cleanup
- [ ] Delete temporary exports or add them to `.gitignore`.
- [ ] Archive example presets into `out/tutorial_assets/` if keeping for release bundles.
- [ ] Document any deviations from script for future revisions.

## Post Production Checklist
- Insert chapter markers: WAVE fundamentals, WAVE demo, CUSTOM mode, Performance guardrails, Outro.
- Add cards linking back to earlier videos and docs.
- Include download links for example presets and custom delay JSON in description.
- Ensure outro references soak test runbook/notes and invites feedback.
