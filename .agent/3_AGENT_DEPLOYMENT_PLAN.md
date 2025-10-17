# Agent PM Handover – PRISM K1 (Research‑First, Dependency‑Gated)

Date: 2025‑10‑16
Status: READY – Orchestrate agents, enforce decisions, do not code
PM Role: High‑leverage coordination, gating, and dependency hygiene

---

## 🎯 IMMEDIATE MISSION (YOU)
- Spin up Agents 1/2/3 with the exact briefs below.
- Enforce research decisions (no drift) and task dependencies.
- Keep work flowing in strict topological order; validate after each change.

## 📚 REQUIRED PRE‑READS (LOAD IN AGENT SESSIONS)
- PRDs (parsed):
  - `.taskmaster/docs/prd_foundation_research_color_tooling.txt`
  - `.taskmaster/docs/prd_color_tooling_integration.txt`
- Research memos (decisions):
  - `firmware/docs/research/R3.1_color_space_decision.md`
  - `firmware/docs/research/R3.2_rgbw_strategy.md`
  - `firmware/docs/research/R3.3_show_families.md`
  - `firmware/docs/research/R3.4_packaging_tradeoffs.md`
  - `firmware/docs/research/R3.5_preview_guidelines.md`
  - `firmware/docs/research/R2.1_metadata_extensions.md`
  - `firmware/docs/research/R2.2_header_crc_implications.md`

Supporting briefs
- `.agent/AGENT_3_WAVE_RELEASE.md` (Agent 3 kickoff; scope, sequencing, DoD)

## 🔐 NON‑NEGOTIABLE DECISIONS (DO NOT OVERRIDE)
- Palette ramps: HSLuv default; OKLab/OKLCH as optional next.
- Shows: waves (phase fields), noise morphs, flow fields; shared parameter schema.
- Previews: gamma=2.2, brightness=0.85, saturation=0.9 (flags override).
- Packaging: palette+indices (≤64), optional XOR deltas + simple RLE; single‑pass O(N) decode.
- Decode budget (ESP32‑S3): ≤0.5 ms/frame target (~120k cycles @ 240 MHz); no per‑frame heap; ≤4 KB working set.
- Parser: add `palette_id`, `ramp_space`, `show_params`; keep v1.1 CRC coverage unchanged.

## 🧭 SYSTEM OF RECORD
- Task Master (tag: master). Validate after any changes.
- Commands: `task-master list | next | show <id> | set-status --id=<id> --status=… | validate-dependencies`
- Models: `task-master models` (ensure a working research provider when expanding)

---

## 👷‍♂️ AGENT 3 – TOOLING & RELEASE (PRIMARY EXECUTION)
Pre‑read: R3.1, R3.3, R3.5, R3.4, R2.1, R2.2

Pipeline (in order)
1) CLI & color dependencies (gates for previews/shows)
   - Task #21 Establish CLI tooling core → Task #22 Configure color library dependencies
   - Task #24 Finalize palette_to_prism CLI and metadata output (depends on #21/#22/#23)
2) Previews
   - Task #26 Build terminal preview renderer
     - Defaults per R3.5; flags: `--gamma --brightness --saturation --fps`
     - DoD: renders sample palette/show at fps; flags override defaults
   - Task #27 Implement HTML preview exporter (ANSI → HTML)
     - Same mapping; DoD: writes static HTML preview
3) Shows
   - Task #28 Minimal show engine (sine|noise|flow)
     - Param schema (common): duration,fps,led_count,palette,ramp_space,seed + family knobs
     - DoD: deterministic frames (seeded); metadata recorded
   - Task #29 Parameterized show CLI
     - Validate ranges; DoD: invalid inputs rejected with clear messages; meta persisted
4) Packaging
   - Task #30 Prototype .prism v1.1 packaging
     - Encoding: palette+indices + optional XOR deltas + simple RLE
     - Constraints: single‑pass decode; no per‑frame heap; ≤4 KB WS
     - Parser: R2.1 fields; CRC per R2.2
     - DoD: packaged artifact parses; bench decode ≤ budget
5) Release
   - Task #20 Finalize Docs/Migration/Release
    - After #23 (Done), #26/#27, #28; DoD: presets, docs, artifacts; no decision drift

Notes
- Palette tooling (#23) is Done (HSLuv default) — use it to author presets.
- Commit after each subtask; include task ID; use report template below.

Gating & dependencies (authoritative)
- #26 → deps: 23, 24, 37
- #27 → deps: 26, 37
- #28 → deps: 23, 24, 35
- #29 → deps: 21, 22, 23, 24, 28, 35
- #30 → deps: 21, 22, 23, 24, 28, 29, 36, 39
- #20 → deps: 23, 26, 27, 28

---

## 🧩 AGENT 2 – PARSER & FORMAT
Pre‑read: R2.1, R2.2, R3.4

Pipeline
1) v1.1 Metadata extensions (R2.1)
   - Add `palette_id`, `ramp_space` (hsv|hsl|hsluv|oklab|oklch), `show_params`
   - DoD: defaults/backcompat; unknown fields → no‑op; unit tests
2) Header/CRC behavior (R2.2)
   - Keep v1.1 CRC coverage unchanged; document behavior
   - DoD: parser tests pass
3) Packaging integration
   - Parse artifacts from #30; round‑trip; DoD: pass/fail harness

---

## ⚙️ AGENT 1 – FIRMWARE RUNTIME
Pre‑read: R1.1

Scope
1) On‑device validation hooks
   - Provide cycle‑count wrappers for packaging decode (when #30 lands)
   - DoD: decode ≤ budget; no per‑frame heap; ≤4 KB WS
2) Optional preview checks
   - Spot‑check previews vs device output; DoD: documented deltas

---

## 🗓️ PM CADENCE
- Start of day: set In‑Progress on next tasks for each lane; ensure gates are respected
- Mid‑day: unblock; validate dependencies; keep execution in topo order
- End of day: mark Done; summarize; ensure no decision drift

## ✅ DEFINITION OF DONE (GLOBAL)
- Conforms to decisions & constraints; tested/benchmarked where applicable
- Artifacts and CLI documented
- Task Master status updated; dependencies valid

### Agent 3 – task‑specific DoD
- #26 Terminal preview: flags `--gamma(2.2) --brightness(0.85) --saturation(0.9)`, deterministic ANSI fixtures, mapping matches R3.5
- #27 HTML preview: static HTML mirrors terminal mapping; fixtures committed; deterministic
- #28 Shows: sine|noise|flow implemented; common schema; deterministic via `--seed`; frames+metadata emitted
- #29 Parameterized CLI: validates inputs; records `show_params`; round‑trip reproducibility from seed+params
- #30 Packaging: palette+indices + XOR delta + simple RLE; single‑pass O(N) decode; size/throughput benchmarks; CRC per R2.2

## 🧪 QUICK VALIDATION
- `task-master validate-dependencies` → should pass
- For packaging: bench decode on S3 within budget (when available)

## 🟢 RELEASE (#20) GREEN‑LIGHT CHECKLIST
- #23, #26, #27, #28 complete; #29/#30 included when used by release presets
- Previews validated against R3.5 defaults and CLI overrides
- Shows deterministic with seeds; metadata contains `ramp_space` and `show_params`
- Packaging decodes single‑pass under R1.1 budget (evidence from microbench)
- Docs updated (tools/README.md, CHANGELOG.md); preset library assembled; links verified

## 📝 COMMIT & REPORT TEMPLATES
- Commit: `feat(task-<id>): <short summary>` or `docs(task-<id>): <short>`
- Agent Report (paste on completion):
  "AGENT <N> REPORTS: <Task #> Complete ✅\nStatus: SUCCESS/Blocked\nSummary: <what changed>\nArtifacts: <paths>\nCommit: <hash>\nNext: <task id>\nQuestions: <if any>"

## 🚨 ESCALATION
- If any task conflicts with a research memo, pause and contact PM; update the memo before proceeding.
 - Blocker = unmet dependency, spec ambiguity with memos, or decode budget risk.
 - Timebox spikes ≤2h; if unresolved, raise to PM with options + preferred path.

---

## 🔎 FAST LINKS
- Tools: `tools/palette_to_prism.py`, `tools/README.md`
- Research: `firmware/docs/research/*.md`
- PRDs: `.taskmaster/docs/prd_*`
