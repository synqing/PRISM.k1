# Agent 3 – Tooling & Release (Research‑First Execution)

Pre‑Reading
- `firmware/docs/research/R3.1_color_space_decision.md`
- `firmware/docs/research/R3.3_show_families.md`
- `firmware/docs/research/R3.5_preview_guidelines.md`
- `firmware/docs/research/R3.4_packaging_tradeoffs.md`
- `firmware/docs/research/R2.1_metadata_extensions.md`
- `firmware/docs/research/R2.2_header_crc_implications.md`

Pipeline (in order)
1) Previews
   - Task #26 Build terminal preview renderer
     - Defaults: gamma=2.2, brightness=0.85, saturation=0.9 (flags override)
     - CLI: `--gamma --brightness --saturation --fps`
   - Task #27 Implement HTML preview exporter (ANSI → HTML)
     - Same mapping; export a static HTML preview

2) Shows
   - Task #28 Minimal show engine (sine|noise|flow)
     - Param schema (common): duration, fps, led_count, palette, ramp_space, seed
     - Waves: amplitude, frequency, speed, direction, phase
     - Noise: scale, speed, octaves, persistence, lacunarity
     - Flow: field_scale, step_size, speed, curl (optional)
   - Task #29 Parameterized show CLI (validate + record meta)

3) Packaging
   - Task #30 Prototype .prism v1.1 packaging
     - Encoding: palette+indices (≤64), optional XOR deltas, simple RLE
     - Constraints: single‑pass O(N) decode; no per‑frame heap; ≤4 KB working set (R1.1)
     - Parser: R2.1 metadata (palette_id, ramp_space, show_params); CRC per R2.2

4) Release
   - Task #20 Finalize Docs/Migration/Release after #23, #26/#27, #28
     - Produce presets, docs (tools/README), and artifacts

Usage
- `task-master show <id>` → follow Implementation Details
- Commit after each subtask: `feat(task-<id>): <subtask>`
- Mark done: `task-master set-status --id=<id> --status=done`

Notes
- Palette tooling (#23) is already Done (HSLuv default); use it to author presets.
- Do not change foundation decisions without PM approval.
