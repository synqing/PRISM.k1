# Video 2 Script — Motion Directions Explained

## Runtime Target
- 12 minutes (plan ~11:30 edited)

## Segment Outline
1. **Hook (0:00–0:25)** — Split-screen showing LEFT vs RIGHT vs CENTER playback. Caption: "Five motion directions, one preset format."
2. **Recap (0:25–1:10)** — Brief reminder from Video 1 about temporal sequencing basics.
3. **Direction overview slide (1:10–2:00)** — Bullet summary of LEFT_ORIGIN, RIGHT_ORIGIN, CENTER_ORIGIN, EDGE_ORIGIN, STATIC. Mention metadata field `meta.motion.direction`.
4. **CLI setup (2:00–2:45)** — Show preset scaffold located in `out/tutorial_motion/`.
5. **Direction demos (2:45–9:30)** — For each direction:
   - Show CLI command (e.g., `python3 -m tools.show_to_prism --show sine --motion-direction LEFT_ORIGIN ...`).
   - Display resulting JSON snippet highlighting `meta.motion`.
   - Run terminal preview (`python3 -m tools.previews.terminal_preview ... --loop`).
   - Cut to hardware footage. Annotate LED travel direction with overlay arrows.
   - Provide usage tips (e.g., LEFT_ORIGIN for left-to-right signage).
6. **Static mode contextualization (9:30–10:30)** — Explain use cases (ambient glow, shockless transitions).
7. **Metadata deep dive (10:30–11:15)** — Show how migration CLI populates defaults, emphasize backwards compatibility.
8. **Wrap-up (11:15–12:00)** — Preview upcoming sync modes video, encourage viewers to experiment with direction + color combos.

## Script Notes
- Clarify naming: "LEFT_ORIGIN moves light from left to right" etc.
- Use consistent on-screen labeling for each direction.
- Mention recommended LED counts/depth cues to accentuate motion.
- Tie back to documentation: `docs/user-manual.md` motion section.

## Shot List
- 00:00–00:25: Triptych hardware footage (LEFT/RIGHT/CENTER) with overlay text.
- 00:25–01:10: Presenter voiceover over recap slide; include bullet list from Video 1.
- 01:10–02:00: Slide with icons for each direction; animate highlighting per bullet.
- 02:00–02:45: Terminal capture showing directory prep and CLI skeleton command.
- 02:45–09:30: For each direction:
  - CLI execution shot with highlighted `--motion-direction` flag.
  - JSON snippet view with `meta.motion.direction`.
  - Terminal preview animation capture.
  - Hardware close-up with overlay arrow direction.
- 09:30–10:30: Static mode demo with ambient lighting B-roll.
- 10:30–11:15: Migration CLI output snippet showing default direction metadata.
- 11:15–12:00: Outro slide previewing sync modes video and references.

## Narration Script (timed)
- **Hook VO:** "Direction metadata drives how light travels. Here’s left origin, right origin, and center origin in action."
- **Recap:** "Previously we saw temporal sequencing at a high level. Each preset now includes motion direction metadata controlling LED order along the edge."
- **Overview slide:** "LEFT_ORIGIN sweeps left to right. RIGHT_ORIGIN mirrors it. CENTER_ORIGIN blooms outward, EDGE_ORIGIN collapses inward, and STATIC keeps both edges steady for ambient glows."
- **CLI setup:** "All demos live under `out/tutorial_motion`. I’m using the sine show, but direction tags work with any engine. Let’s scaffold the first command."
- **LEFT_ORIGIN demo:** "With `--motion-direction LEFT_ORIGIN`, the JSON records that direction in `meta.motion`. Terminal preview shows the sweep from index 0 upwards, and on hardware you can see the motion hugging the left edge."
- **RIGHT_ORIGIN demo:** "Switching to RIGHT_ORIGIN flips the sweep. Same palette, but metadata tells firmware to reverse the index ordering."
- **CENTER_ORIGIN demo:** "CENTER_ORIGIN starts in the middle and radiates outward. This is great for spotlight effects or symmetrical reveals."
- **EDGE_ORIGIN demo:** "EDGE_ORIGIN is the inverse—both edges collapse towards the center. Use it for closing curtain effects."
- **STATIC demo:** "STATIC leaves LEDs in sync; combine it with sync modes for subtle breathing or color morphs when movement isn’t desired."
- **Metadata reminder:** "Migration tooling backfills direction defaults so legacy presets remain valid. You can edit direction tags manually for precise choreography."
- **Outro:** "Experiment by pairing directions with different palettes. Up next we’ll combine direction metadata with sync modes to shape timing between edges."

## Demo Command Cheatsheet
```bash
# LEFT_ORIGIN
python3 -m tools.show_to_prism --show sine --palette "#ff4500,#ffd700,#00bcd4" \
  --motion-direction LEFT_ORIGIN --sync-mode SYNC --led-count 160 \
  --duration 6 --fps 24 --output out/tutorial_motion/left_origin.json \
  --csv out/tutorial_motion/left_origin.csv --ramp-space hsluv

# RIGHT_ORIGIN
python3 -m tools.show_to_prism --show sine --palette "#ff4500,#ffd700,#00bcd4" \
  --motion-direction RIGHT_ORIGIN --sync-mode SYNC --led-count 160 \
  --duration 6 --fps 24 --output out/tutorial_motion/right_origin.json \
  --csv out/tutorial_motion/right_origin.csv --ramp-space hsluv

# CENTER_ORIGIN
python3 -m tools.show_to_prism --show sine --palette "#ff4500,#ffd700,#00bcd4" \
  --motion-direction CENTER_ORIGIN --sync-mode SYNC --led-count 160 \
  --duration 6 --fps 24 --output out/tutorial_motion/center_origin.json \
  --csv out/tutorial_motion/center_origin.csv --ramp-space hsluv

# EDGE_ORIGIN
python3 -m tools.show_to_prism --show sine --palette "#ff4500,#ffd700,#00bcd4" \
  --motion-direction EDGE_ORIGIN --sync-mode SYNC --led-count 160 \
  --duration 6 --fps 24 --output out/tutorial_motion/edge_origin.json \
  --csv out/tutorial_motion/edge_origin.csv --ramp-space hsluv

# STATIC
python3 -m tools.show_to_prism --show sine --palette "#ff4500,#ffd700,#00bcd4" \
  --motion-direction STATIC --sync-mode SYNC --led-count 160 \
  --duration 6 --fps 24 --output out/tutorial_motion/static.json \
  --csv out/tutorial_motion/static.csv --ramp-space hsluv
```

## Demo Preparation
- Create directory `out/tutorial_motion/` with one preset per direction.
- Confirm hardware mounting allows clear view of edges.
- Pre-record B-roll with slow-motion option for clarity if needed.
- Verify direction metadata values align with firmware expectations.

## Visual Assets
- Direction iconography (arrow overlays).
- Metadata callout bubble showing JSON keys.
- Table overlay summarizing when to use each direction.

## Audio Cues
- Use distinct audio sting when transitioning between directions.
- Keep narration pace steady; pause briefly before each hardware clip.

## Recording Checklist
- [ ] Terminal font size ≥ 18 pt, dark theme.
- [ ] Hardware camera positioned to clearly show left/right edges simultaneously.
- [ ] Arrow overlays prepared in editing software.
- [ ] Direction icons exported with transparent backgrounds.
- [ ] Ensure `hsluv` installed to match final color look.

## Post Production Checklist
- Add chapter markers per direction.
- Ensure overlay text is legible against hardware footage.
- Update YouTube description with sample CLI commands and docs links.
- Export 1080p 30fps, same style lower thirds as Video 1.
