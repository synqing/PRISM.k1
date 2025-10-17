# Wireframe ↔ Task Traceability Matrix

Source bundle: `User-Facing App Wireframes 3/` (see README for Figma link)

## Screens / Components

- Timeline Editor
  - File(s): `User-Facing App Wireframes 3/src/components/prism/TimelineEditorScreen.tsx`, `User-Facing App Wireframes 3/src/App.tsx:Timeline`
  - Docs: `User-Facing App Wireframes 3/src/PRODUCTION_READY_SUMMARY.md` (Timeline), `User-Facing App Wireframes 3/src/WIRED_FUNCTIONALITY.md` (Timeline, zoom, scrubbing)

- Device Manager
  - File(s): `User-Facing App Wireframes 3/src/components/prism/DeviceManagerScreen.tsx`, `User-Facing App Wireframes 3/src/App.tsx:Devices`
  - Docs: `PRODUCTION_READY_SUMMARY.md` (Device Manager), `WIRED_FUNCTIONALITY.md` (Device add/remove/status)

- Progress / Bake→Upload Flow
  - File(s): Compose within Devices/Timeline UI; reference progress and transport controls in `App.tsx`
  - Docs: `PRODUCTION_READY_SUMMARY.md` (WebSocket Device Sync, transport controls), `WIRED_FUNCTIONALITY.md` (Upload progress, error states)

## Task Mapping

- Task 2: Build Visual Node Graph Editor
  - Wireframe refs: Timeline Editor screen (layout, zoom, keyboard), docs on Timeline zoom/scrub.
  - Acceptance additions: match zoom states, scrubbing behavior, and keyboard shortcuts shown in wireframes.

- Task 6: Implement Streaming Upload Backend
  - Wireframe refs: Device Sync in `PRODUCTION_READY_SUMMARY.md` and progress notes in `WIRED_FUNCTIONALITY.md`.
  - Acceptance additions: backend emits progress/ack states that UI renders as in wireframes.

- Task 7: Integrate Bake & Upload Workflow in UI
  - Wireframe refs: Progress states, cancel/retry, TTFL display.
  - Acceptance additions: UI matches progress panels/states in wireframes.

- Task 8: Implement Device Playback & Parameter Controls
  - Wireframe refs: Transport controls and device interaction states.
  - Acceptance additions: Controls/shortcuts align with wireframe interactions.

- Task 9: Surface Upload Progress, Errors, and TTFL Metrics
  - Wireframe refs: Progress and error display patterns.
  - Acceptance additions: UI/telemetry presentation matches wireframe examples.
## Update 2025-10-17

- DevicePanel: Bake → Upload flow implemented; ProgressPanel shows percent/throughput (EMA), ETA, TTFL, Cancel/Reset.
- Graph Editor: Initial GraphCanvas scaffold added; node add/remove wired to graph store; mounted before DevicePanel in app.
- Playback Controls: Brightness/Gamma sliders + Play/Stop buttons and mod+P/mod+S shortcuts aligned with transport controls.
- Telemetry: Upload telemetry JSONL emitted to `logs/upload.jsonl` in Tauri mode.
- Error Mapping: Frontend maps WS/TLV/size errors to friendly messages.

Acceptance criteria references updated where applicable in Task Master (Tasks 6,7,8,9). Remaining visual parity items for timeline/editor details tracked under Task 2 follow-ups.

