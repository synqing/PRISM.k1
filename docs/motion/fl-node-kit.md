# FL.ledstrip → PRISM K1 Motion Node Kit

Source: https://github.com/synqing/FL.ledstrip

This document captures the minimal, battle-tested motion/color vocabulary from FL.ledstrip and maps it directly to PRISM K1 nodes and a tiny IR. The goal is to ship a clean first node kit that feels identical to the lab while being easy to extend.

## Core Ideas (kept 1:1)

- Spatial fields are precomputed
  - AngleField: per-pixel angle (center-origin), used to index palettes or modulate phase
  - RadiusField: per-pixel radius (distance from center)
- Motion primitives are tiny & expressive
  - Oscillator (sin/beat), Phase accumulator, Distance ring test
- Temporal feel is a fade
  - Fade/decay step each frame (e.g., fadeToBlackBy)
- Palette machinery works well
  - Blend toward target palette; index lookup per-pixel
- Center-origin symmetry
  - Dual-strip symmetry around the center (79/80 for 160-edge); mirror outward

## Node/IR Mapping

- AngleField → scalar field (0..1)
- RadiusField → scalar field (0..1 or pixel units)
- SinOsc (phase, freq, offset) → sin8/beat oscillators
- PhaseAccum (speed*dt) → phase accumulation
- DistCenter → compute distance from center index
- Ring(center,width) → ripple band mask (± width)
- Fade(amount) → temporal decay per frame
- PaletteMap(indexStrategy) → palette lookup; index by angle or luminance
- CenterOutMirror → mirror values around center index
- Mixers: Add, Multiply

With these, we can express Wave, Sinelon (center-out), Ripple, Interference, and Confetti.

## Geometry Profile (K1 addition)

- GeometryProfile asset (K1_LGP_v1): per-pixel (u,v), edge weights (edgeW0/edgeW1), gain, white balance, gamma
- Pipeline: render → edge gain → uniformity gain → white balance → gamma → driver
- Parallel output: submit two edges in parallel (two RMT channels)

## Palette Strategy

- Host generates 256×RGB LUT (OKLCH interpolated, gamut policy)
- Device uses palette lookup akin to ColorFromPalette
- Optional PaletteManager behavior (blend toward target palette on device or host)

## Concrete Patterns

- Wave: AngleField → Add(phase) → SinOsc → PaletteMap → ToK1
- Sinelon (center-origin): Phase → beatsin(range=halfLen) → CenterOutMirror → Fade → PaletteMap
- Ripple: RandomImpulse → Ring(center,width) → Multiply(decay) → PaletteMap → Add → Fade
- Interference: AngleField → Sin(fA,φA) + Sin(fB,φB) → Scale → PaletteMap
- Confetti: Spawn@center → OutwardShift (two directions) → Fade

## Action Items (cross-repo)

1) Implement nodes/ops above in Studio runtime; add tests
2) Add GeometryProfile schema and device-side gains
3) Decide palette blend location (host vs device) and surface UI
4) Ensure 120 FPS perf for Wave/Sinelon/Ripple with telemetry thresholds

