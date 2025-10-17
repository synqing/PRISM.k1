# Video 1 Script — Introduction to PRISM K1 and Temporal Sequencing

## Runtime Target
- 10 minutes (aim for 9:30 finished cut to allow YouTube intro/outro cards)

## Segment Outline
1. **Cold open (0:00–0:20)** — Quick montage of PRISM K1 running three presets. Lower third: "PRISM K1 Firmware v1.1".
2. **Host intro (0:20–1:00)** — On-camera or voiceover with title slide. Key points: dual-edge LGP controller, firmware v1.1 release, temporal sequencing headline feature.
3. **Hardware tour (1:00–2:30)** — B-roll of device, callouts for ESP32-S3, power, LED connectors, storage. Overlay graphic: dual edge illustration.
4. **Temporal sequencing overview (2:30–4:00)** — Slide plus animated diagram showing edge A/B delay. Mention motion directions + sync modes at high level.
5. **Tooling snapshot (4:00–5:30)** — IDE/shell capture: show repo layout, highlight `tools/show_to_prism.py`, `tools/preset_library.py`, docs path.
6. **Live demo (5:30–8:30)** — Capture QuickTime/OBS of:
   - Running `python3 -m tools.show_to_prism --show sine --palette "#ff4500,#ffd700,#00bcd4" --led-count 160 --duration 6 --fps 24 --output out/tutorial_intro/tutorial_intro.json --csv out/tutorial_intro/tutorial_intro.csv --ramp-space hsluv`
   - Packaging preview with `python3 -m tools.previews.terminal_preview --input out/tutorial_intro/tutorial_intro.json --static`
   - Upload via web dashboard (or CLI) and filming hardware response. Include picture-in-picture of LEDs.
   - Narration: "This is the SYNC baseline; upcoming videos add direction and sync control."
7. **Roadmap teaser (8:30–9:30)** — Slide listing the remaining four videos with thumbnails.
8. **Call to action (9:30–10:00)** — Ask viewers to subscribe/bookmark, link README and docs, encourage prepping hardware.

## Shot List
- 00:00–00:20: Three-shot montage (camera on tripod) of different presets; overlay titles for each pattern.
- 00:20–01:00: Presenter talking head or animated title slide with voiceover; include PRISM K1 logo.
- 01:00–02:30: Macro shots of device; add motion graphics for key components.
- 02:30–04:00: Screen recording of slide deck with animated diagram; overlay pointer tracing delays.
- 04:00–05:30: Desktop capture of repository tree and tooling; highlight folders with callouts.
- 05:30–08:30: Split layout (CLI left, hardware right) showing command execution and LED response.
- 08:30–09:30: Slide with thumbnails for remaining videos; animate highlight around current video.
- 09:30–10:00: Outro graphic with links and CTA.

## Narration Script (include pauses for visual beats)
- **Cold open (VO over montage):** "This is PRISM K1—dual-edge light guide animation driven by firmware v1.1. In this series we unlock temporal sequencing."
- **Intro:** "Hi everyone, I’m [Name], and welcome to the PRISM K1 firmware v1.1 release series. Temporal sequencing is the headline feature, letting us choreograph light timing between both edges of the panel."
- **Hardware tour:** "At the core is an ESP32-S3 module, flanked by dual LED edge connectors. Storage lives onboard with LittleFS, and the power design feeds 320 LEDs. The release ships with tooling so you can program patterns offline before uploading over Wi-Fi."
- **Temporal overview:** "Temporal sequencing gives us motion directions—left, right, center, edge, static—and sync modes that control when each edge fires. Think of it as the timing layer above color palettes."
- **Tooling snapshot:** "Everything lives in the tools directory: `show_to_prism` generates frames, `preset_library` builds packs, and our docs walk through temporal concepts. You’ll need Python 3.11 and the dependencies from `tools/requirements.txt`."
- **Live demo:** "Let’s run the simplest sweep. I’ll call `show_to_prism` with the sine engine and three-color palette. The terminal preview matches what the panel will show. After uploading through the dashboard, watch the LEDs—this is SYNC mode, both edges lighting together. Remember, the migration CLI and user manual are already live if you need a refresher."
- **Roadmap teaser:** "Next we dive into motion directions, then sync modes, we’ll build a complete pattern, and finally push into advanced WAVE and CUSTOM techniques."
- **Call to action:** "Subscribe or bookmark the playlist so you don’t miss the deep dives. Grab your hardware, install the tools, and let’s build temporal sequencing together. Links are in the description—see you in Video 2."

## Checklist Before Recording
- [ ] Ensure `hsluv` dependency installed (`pip install -r tools/requirements.txt`) so CLI output matches narration.
- [ ] Delete `out/tutorial_intro/tutorial_intro.*` after recording to avoid repo cruft.
- [ ] Confirm Wi-Fi dashboard credentials saved locally; open to correct tab prior to recording.
- [ ] Calibrate camera white balance to avoid LED clipping; set FPS to 60 for smoother slow-mo if needed.
- [ ] Prepare slide deck assets (title, hardware diagram, roadmap).

## Script Notes
- Tone: welcoming and confident; assume technically savvy audience.
- Reinforce vocabulary: "temporal sequencing", "dual-edge timing", "preset metadata".
- Mention that the migration CLI and user manual are already available (credit Phase 1 work).
- Add disclaimer: final release requires installing Python dependencies (`pip install -r tools/requirements.txt`).

## Demo Preparation
- Ensure `out/tutorial_intro/tutorial_intro.json` is committed to `.gitignore` or deleted after recording.
- Pre-load hardware with neutral brightness for camera capture.
- Verify Wi-Fi dashboard credentials before rolling.
- Have terminal in dark theme with 125% font size.

## Visual Assets
- Title slide template (update date + version).
- Dual-edge animation (use Keynote/PowerPoint export or After Effects).
- Lower thirds for key terms (Temporal Sequencing, Dual Edge, Firmware v1.1).
- Capture overlay showing CLI command text.

## Audio Cues
- Background music (low volume) only during intro/outro.
- Record narration in quiet room; use pop filter.
- Add whoosh/stinger transitions between sections sparingly.

## Post Production Checklist
- Normalize audio to -16 LUFS.
- Color-correct hardware footage to avoid LED clipping.
- Add YouTube chapters at each major section.
- Insert links in description: README, user manual, migration playbook, preset overview.
- Export 1080p 30fps, high bitrate (16 Mbps).
