# Tutorial Series Storyboards

Five narrated tutorials (10–15 minutes each) that introduce PRISM K1 firmware v1.1, temporal sequencing, and the release workflow. Each section lists the goal, suggested flow, assets to capture, and key call-outs that should land in the narration.

## Video 1 – Welcome, Hardware, and First Pattern

- **Goal:** Introduce PRISM K1, temporal sequencing, and help new users play their first pattern.
- **Flow:**
  1. Title card + mission overview (v1.1 highlights, ADR-010 reference).
  2. Hardware tour (ESP32-S3, dual-edge LGP, storage layout).
  3. Bring device online: captive portal → Wi-Fi join → dashboard tour.
  4. Generate quick `sine` preset with `tools/show_to_prism.py`.
  5. Upload via WebSocket UI and demonstrate playback controls.
- **Assets:** Screen capture (dashboard + CLI), camera footage of LGP, intro/outro slides, narration script.
- **Call-outs:** Mention default sync (SYNC) vs new temporal options, reinforce need for tools repo clone, include safety tip about power budget.

## Video 2 – Motion Directions in Action

- **Goal:** Teach the five motion directions using hardware-first demos.
- **Flow:**
  1. Recap temporal sequencing concept and why direction metadata matters.
  2. Explain each direction: `LEFT_ORIGIN`, `RIGHT_ORIGIN`, `CENTER_ORIGIN`, `EDGE_ORIGIN`, `STATIC`.
  3. For each direction show: storyboard slide → CLI command → hardware reaction.
  4. Highlight metadata block in JSON (`meta.motion.direction`) and how presets consume it.
  5. Close with best-practices (when to choose each direction).
- **Assets:** Animated overlays indicating LED sweep, device close-ups, CLI snippets, motion comparison chart.
- **Call-outs:** Stress that direction metadata survives migration, demo `tools/previews/terminal_preview` for quick verification, remind viewers to keep frame budgets in mind.

## Video 3 – Sync Modes & Shape Creation

- **Goal:** Explain the dual-edge architecture and showcase shape-driven sync patterns.
- **Flow:**
  1. Brief architecture refresher: Edge A vs Edge B timing and delay tables.
  2. Walk through sync modes: `SYNC`, `OFFSET`, `PROGRESSIVE`, `WAVE`, `CUSTOM`.
  3. Build a shape gallery (triangle, diamond, chevron, sine ribbon) using preset parameters.
  4. Visualize phi phenomenon (60–150 ms delay) with side-by-side device footage.
  5. Show how metadata (`meta.sync.mode`, delay fields) drives firmware behaviour.
- **Assets:** Diagram of edge timing, overlays for each shape, JSON metadata highlights, hardware capture with split view.
- **Call-outs:** Note relationships between sync mode and motion direction, mention `tools/show_engine` helpers, emphasize QA expectations (no frame drops, balanced brightness).

## Video 4 – Advanced Techniques & Performance Guardrails

- **Goal:** Combine motion + sync creatively while staying inside v1.1 performance envelopes.
- **Flow:**
  1. Compose a complex multi-layer preset (direction sweeps + progressive sync).
  2. Demonstrate adaptive FPS and segmenting for large shows (staying under 256 KB).
  3. Showcase CUSTOM sync editor in `prism-editor` and exporting delay maps.
  4. Discuss palette considerations on LGP (additive colour, contrast, diffusion tips).
  5. Run `tools/previews.html_preview` + terminal preview to validate results.
- **Assets:** Screen capture of editor + CLI, hardware macro shots, performance dashboard output (`soak_telemetry` snippet), slides summarizing limits.
- **Call-outs:** Reiterate heap fragmentation <5 %, highlight profiling toggle, point to ADR-010 for deeper theory, remind viewers to version-control presets.

## Video 5 – Migration, Soak Validation, and Release Handoff

- **Goal:** Walk viewers through migration tooling, soak testing, and packaging for public release.
- **Flow:**
  1. Compare v1.0 vs v1.1 headers (temporal metadata additions).
  2. Demo `tools/migrate_prism.py` + preset validation (`tools.validation.prism_sanity`).
  3. Outline 24 h soak setup (3 devices, playlist automation, telemetry capture).
  4. Review `docs/release/soak_test_runbook.md` + new `soak_test_report.md` template.
  5. Assemble release bundle (preset library, firmware build, release notes) and confirm OTA/rollback checklist.
- **Assets:** CLI captures, spreadsheet/summary outputs, footage of test rack, release notes draft, GitHub release walkthrough.
- **Call-outs:** Stress evidence requirements (logs, summary JSON, photos), warn about checksum verification, link to support channels for issues.

---

**Recording Checklist**
- [ ] Intro/outro branding consistent across videos.
- [ ] 1080p (or higher) capture for screen + hardware, clean audio mix.
- [ ] Hardware demonstrations well lit with clear view of both edges.
- [ ] CLI commands and JSON snippets provided in video descriptions.
- [ ] YouTube descriptions include timestamps, required tooling, and relevant docs.
- [ ] README and release notes updated with final video links.

> Keep this document synchronized with the published release notes. Update flow details if tooling flags or scripts change before recording.
