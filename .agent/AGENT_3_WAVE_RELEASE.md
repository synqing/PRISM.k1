# Agent 3 – Wave Release Kickoff (Tooling/Release)

Purpose
- Implement previews, show generators, parameterized CLI, and packaging prototype in a research‑gated order that unblocks release.
- Do not change research decisions; escalate if requirements conflict.

Pre‑Reading
- firmware/docs/research/R3.1_color_space_decision.md
- firmware/docs/research/R3.3_show_families.md
- firmware/docs/research/R3.5_preview_guidelines.md
- firmware/docs/research/R3.4_packaging_tradeoffs.md
- firmware/docs/research/R2.1_metadata_extensions.md
- firmware/docs/research/R2.2_header_crc_implications.md

System of Record
- Task Master. Use: `task-master list | next | show <id> | set-status --id=<id> --status=in-progress|done | validate-dependencies`.

Execution Order (with gates)
1) #21 Establish CLI tooling core → #22 Configure color library dependencies
2) #24 Finalize palette_to_prism CLI and metadata output (deps: 21,22,23)
3) #26 Terminal preview (deps: 23,24,37)
   - Defaults: gamma=2.2, brightness=0.85, saturation=0.9; flags override
4) #27 HTML preview (deps: 26,37) – mirror mapping from #26
5) #28 Minimal show engine (deps: 23,24,35)
6) #29 Parameterized show CLI (deps: 21,22,23,24,28,35)
7) #30 Packaging prototype (deps: 21,22,23,24,28,29,36,39)
8) #20 Release rollup (deps: 23,26,27,28)

CLI Specs
- Terminal preview (#26)
  - Flags: `--gamma <float>` (2.2), `--brightness <float>` (0.85), `--saturation <float>` (0.9), `--fps <int>` (default 60)
  - Input: palette or frames; Output: ANSI preview to stdout and optional fixture file
- HTML preview (#27)
  - Generate deterministic HTML using exact same mapping as terminal preview; no external assets or network

Definitions of Done
- #26 Terminal preview: deterministic ANSI fixtures; defaults match R3.5; flags override; documented usage
- #27 HTML preview: static HTML mirrors mapping; fixtures included
- #28 Shows: sine|noise|flow; common schema; deterministic via `--seed`; frames+metadata exported
- #29 Parameterized CLI: schema validation; `show_params` recorded; round‑trip reproducibility from seed+params
- #30 Packaging: palette+indices + XOR delta + simple RLE; single‑pass O(N) decode; size/throughput benchmarks; CRC behavior per R2.2

Notes
- Palette tooling (#23) with HSLuv default is already available at `tools/palette_to_prism.py` (HSV/HSL optional, RGBW optional).
- RGBW outputs are for authoring/offline preview; firmware target is RGB; keep RGBW optional.

Blockers & Escalation
- Blocker = unmet dependency, spec ambiguity with research memos, or decode budget risk.
- Timebox spikes ≤2h; if unresolved, report to PM with options and preferred path.
- After any rewiring, run: `task-master validate-dependencies`.

Kickoff One‑Liner (paste in your session)
Pre‑read firmware/docs/research/*.md, then execute: #21, #22, #24 (CLI & color deps), #26 terminal preview (gamma=2.2, brightness=0.85, sat=0.9, flags override), #27 HTML preview (mirror mapping), #25 RGBW pipeline, #28 shows (sine/noise/flow, seeded), #29 params/validation, #30 packaging (palette+indices + XOR delta/RLE, single‑pass O(N) decode). Commit after each subtask; mark done; notify PM on blockers.
