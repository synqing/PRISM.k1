# PRISM.k1 — Phase A Execution Plan (Bake-first)

This is the single-source execution plan with milestones, priorities, and acceptance checkpoints wired to the wireframe bundle.

## Milestones
- M1 Upload Backbone (High)
  - Task 6: Streaming Upload Backend
  - Task 7: Bake→Pack→Upload UI Orchestration
  - Task 9: Telemetry (progress/TTFL/error)
- M2 Compile Surface (High)
  - Task 3: Graph Evaluation Engine
  - Task 4: OKLCH LUT
  - Task 5: ADR‑009 Packer
- M3 Editor + Controls (Medium)
  - Task 2: Node Graph Editor (visual)
  - Task 8: Device Playback & Param Controls
- M4 Reliability (Medium)
  - Task 10: Reliability & Perf Validation
- Cross-cutting
  - Task 1: State Management
  - Task 11: Wireframe Traceability

## Priority & Order of Work
1) 6, 7, 9 (unlock end-to-end Bake→Upload + telemetry)
2) 3, 4, 5 (compile path backing the Bake flow)
3) 2, 8 (editor interactions + device controls)
4) 10 (reliability harness)
5) 1, 11 (store foundation, traceability upkeep)

## Wireframe References
See `.taskmaster/docs/traceability_wireframes.md`.

## Acceptance Gate (Go/No-Go) for Phase A
- Bake→Upload end-to-end succeeds with TTFL shown in UI on a dev device.
- 50-upload soak passes with CRC verification and artifacts captured.
- Editor renders nodes and edges at 60 FPS (100 nodes/200 edges) with keyboard a11y.
- Controls (PLAY/STOP/brightness/gamma) ramp correctly with throttling.

