Hardware Validation (Task 16)
=============================

Goal
----
- Validate PROGRESSIVE temporal sequencing using a high‑speed camera.
- Confirm phi‑phenomenon thresholds (60–150ms) and motion direction.

Setup
-----
- Camera: Chronos 2.1 or equivalent at ≥240 FPS.
- Mount camera static, perpendicular to the LED strip(s).
- Record short clips for triangle and wedge shapes at different params.

Procedure
---------
1. Build and flash firmware with PROGRESSIVE patterns enabled.
2. Record 3–5 second clips for each shape/direction.
3. Transfer videos to this repo and run analyzer:
   - `python3 tools/validation/analyze_progressive.py --video path/to/clip.mp4 --leds 160`
4. Review timing report and visual overlay for delay gradients.

Artifacts
---------
- Save analyzer JSON results under `.taskmaster/reports/validation/`.
- Attach snapshots to TaskMaster via `task-master update-subtask`.

