# ADR-008: LED Refresh Target Increase (60 FPS → 120 FPS)

**Status:** APPROVED
**Date:** 2025-10-16
**Decided By:** Captain SpectraSynq
**Supersedes:** ADR-003 (LED FPS target only)
**Superseded By:** None

## Context

Initial target (ADR-003) standardized LED refresh at 60 FPS. Based on updated product goals and performance measurements (parametric effects are CPU-cheap; RMT transmit is deterministic), we can double the refresh rate without compromising stability.

## Decision

Increase LED refresh target to 120 FPS.

```yaml
led_count: 320          # unchanged
led_type: WS2812B       # unchanged
led_fps_target: 120     # updated
```

## Rationale

- Parametric effects (no pre-rendered frames) keep CPU headroom high.
- RMT-driven WS2812B output is timer-based and scales with frame submission.
- Higher temporal resolution improves motion smoothness for wave/field/aurora.

## Consequences

### Positive
- Smoother animations and reduced motion aliasing.
- Still well within CPU budget (frame gen per LED is sub-µs on ESP32-S3).

### Negative
- Tighter frame budget (8.33ms vs 16.67ms); effects must avoid blocking.
- Slightly higher power due to increased refresh.

### Neutral
- No change to memory footprint (frame size unchanged).

## Implementation

- Update LED driver and playback engine to use `LED_FPS_TARGET=120` and `LED_FRAME_TIME_MS=8`.
- Keep SoT (`.taskmaster/CANON.md`) in sync via `scripts/generate-canon.sh` (pending regeneration).

## Validation

- Verify no frame underruns (driver logs) under 120 FPS with built-in effects.
- Scope LED output for timing correctness.
- Run 24-hour stress with playback active; zero crashes, stable heap.

---
**IMMUTABLE AFTER APPROVAL**
