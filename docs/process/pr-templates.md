# K1 PR Templates v0.1 — Adoption Notes

Source package: `.cursor/K1_PR_Templates_v0_1`

Usefulness: High as a checklist and scaffolding. Do not overwrite working code; instead, mine the templates for structure and PR breakdown. This doc maps the templates to our current codebase and Task Master tasks.

## Studio (Tauri/React)

- Pattern Builder route (PatternBuilder.tsx)
  - We already mounted a GraphCanvas and device panel. Use this template as a reference for splitting Graph UI into a dedicated route if needed.
  - Task links: 16 (Motion Node Kit), 2 (Graph editor), 7 (Bake & Upload UI).

- Node library/types
  - Our node types live under `studio/src/lib/graph/types.ts` and `registry.ts`; we added Motion nodes.
  - Template’s `NODE_LIBRARY` can inform a palette for UI.

- Publish split button (Clip vs Program)
  - Matches Task 19 (K1 Program export). Implement UI toggle in Device/Builder once IR exporter lands.

- Tauri device commands (device_upload/play/stop/set_param)
  - We already implement upload + control in `src-tauri/src/lib.rs`. The template’s modular layout is optional; we can refactor later if useful.

## Firmware (ESP-IDF)

- TLV/websocket scaffolding, renderer, geometry, program runtime
  - Aligns with Tasks 12–14 (protocol), 17 (GeometryProfile), 19 (Program VM). Use function names and file layout as naming inspiration when opening firmware PRs.

## Proposed PR Breakdown (Adapted)

1) Studio: Motion Node Kit + Graph UI refinements
2) Studio: ToK1 publish toggle (Clip/Program) + IR exporter
3) Studio: Tauri command refactor (optional) and param set surfaces
4) Firmware: TLV alignments + cancel semantics + error taxonomy
5) Firmware: GeometryProfile + dual-edge output
6) Firmware: Program VM (IR v0.1) + palette bank

## Notes

- Treat code in the care package as templates (do not drop in). We already have a working uploader, CONTROL commands, and bake/pack path.
- Keep using Task Master to track each slice with acceptance criteria.

