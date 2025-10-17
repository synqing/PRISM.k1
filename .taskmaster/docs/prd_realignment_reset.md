# PRISM.k1 Realignment PRD — Reset for Visual-First Delivery

Date: 2025-10-17
Owner: Captain
Status: Draft (ready for parse)

## Executive Summary
We are resetting the task universe. This PRD defines the minimal, visual‑first delivery that uses the device as the preview surface. It replaces prior plans and tasks. Timeline/DAW UI is explicitly out of scope for this phase.

## Goals (Phase A: Bake-first, IR-later)
- Node graph authoring with a small, potent node set.
- Host‑side “Bake” compiler: evaluate graph, pack .prism per ADR‑009, stream to device, play.
- Device control surface: discover/status/upload/list/delete/play/stop, with progress & errors.
- Color: host‑side OKLCH palettes baked to 256×RGB LUT; device does lookups.

## Non‑Goals
- Timeline/DAW UI, audio paths, multi‑device sync, advanced editor chrome.

## User Stories
1. As a creator, I build a tiny node graph, click “Bake & Upload,” and see the pattern on hardware in < 3 minutes TTFL.
2. As a developer, I can reliably upload 50× in a row with CRC validation and clear errors.
3. As an operator, I can play/stop and adjust brightness/gamma from the app.

## Functional Requirements
### R1 — Node Graph (Tiny Kit)
- Generators: Noise2D, Gradient, Solid
- Transforms: Rotate, Scale, Mirror
- Combiners: Add, Multiply
- Color: PaletteMap, HueShift, Brightness
- Output: ToK1 (declares geometry + edge map)

### R2 — Bake Compiler (.prism)
- Evaluate N seconds @ 60–120 FPS to RGB frames.
- Pack ADR‑009 compliant header and payload; enforce ≤256 KB.
- Compute CRC‑32 of full payload (IEEE 0xEDB88320).

### R3 — Upload & Control
- STATUS → read `maxChunk`; stream PUT_BEGIN/PUT_DATA/PUT_END on one WS connection.
- CONTROL PLAY/STOP per firmware spec; brightness & gamma via CONTROL subcommands.
- Progress UI: percent and throughput; errors mapped to actionable messages.

### R4 — Color/Palletes
- OKLCH interpolation on host; gamma‑correct 8‑bit LUTs (256×RGB) baked into patterns.

## Acceptance Criteria
- TTFL ≤ 3 minutes: app open → small graph → Bake → Upload → Play.
- 120 FPS on 2×160 LEDs verified visually; upload retries/backoff not required for happy path.
- 50 consecutive uploads succeed with CRC‑checked PUT_END.

## Architecture Notes
- Host compiler uses Culori for OKLCH; persistent WebSocket per upload.
- Firmware protocol is fixed‑format TLV: [TYPE:1][LEN:2][PAYLOAD:N][CRC:4]; CONTROL subcommands for playback & params.

## Milestones
M1 — Bake E2E: graph → bakeToPrism() → device_upload() → PLAY.
M2 — Device control: buttons + status/throughput; brightness/gamma.
M3 — Node kit completeness + tests.

## Risks
- Large graphs producing >256 KB patterns; mitigate by capping duration and/or palette/indexed encoding.
- WebSocket instability; mitigate with single‑connection uploads and final ACK checks.

## Out of Scope
- IR (program mode) runtime on device — will be a Phase B PRD.

