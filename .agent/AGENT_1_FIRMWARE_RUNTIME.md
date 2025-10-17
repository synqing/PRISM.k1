# Agent 1 – Firmware Runtime & Performance Envelope

Pre‑Reading
- `firmware/docs/research/R1.1_decode_budget.md`
- Review WAVE/profiling context as needed

Scope
1) On‑Device Validation Hooks
   - Provide cycle‑count wrappers and minimal harness to verify packaging decode paths stay within budget when prototype (#30) is available
   - Ensure no per‑frame heap allocs and ≤4 KB working set during decode

2) Assist with Preview Validation (optional)
   - Spot‑check preview mapping defaults against device output for representative palettes/shows

3) Critical Path Awareness
   - Coordinate with PM if any runtime constraints change that would affect packaging or previews

Usage
- Keep changes minimal and focused on validation; do not block Agent 3 execution
- Commit with task IDs; mark status in Task Master as instructed by PM

Next Actions (Now → #30 landing)
- Add decode instrumentation header exposing begin/end hooks for cycle/us capture (done)
- Add Unity-based microbench harness with dummy single-pass loop to validate plumbing (done)
- Capture avg/p99/max timing over N frames and heap free/min snapshots (done)
- Leave adapter function stub to connect to real packaging decode when Task 30 lands (done)

Post-#30 Integration Steps
- Wire harness to packaging decode entrypoint; pass representative payloads
- Run 500–2000 frame samples at 160 LEDs; export avg/p99/max
- Confirm: ≤0.5 ms avg, ≤1.0 ms p99, O(N) single-pass; no per-frame heap; ≤4 KB working set
- File a report with any regressions and proposed constraints/tweaks

Reporting Snippet
- AGENT 1 REPORTS: Microbench harness ready ✅ / Waiting on #30
