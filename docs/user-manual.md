# PRISM K1 User Manual v1.1

## Temporal Sequencing Overview

PRISM K1 firmware v1.1 introduces temporal sequencing — the ability to stagger LED timing between the two edges of the Light Guide Plate.

### Sync Modes

- SYNC: Both edges illuminate simultaneously (v1.0 behavior)
- OFFSET: Fixed delay between edges
- PROGRESSIVE: Linear delay gradient across LEDs
- WAVE: Sinusoidal delays create flowing motion
- CUSTOM: Expert-defined per-LED timing curves

### Motion Directions

- LEFT (0→159)
- RIGHT (159→0)
- CENTER (middle outward)
- EDGE (edges inward)
- STATIC (no propagation)

### Getting Started

1. Power on the device and connect to the captive portal Wi‑Fi.
2. Select a built‑in effect from the dashboard (e.g., WAVE).
3. Adjust speed and amplitude to taste.

### Built-in Effects

- WAVE Single: LUT‑driven sinusoid across both channels (green demo)
- Palette Cycle: Simple GRB cycling with spatial gradient

### Notes

- WAVE uses precomputed `sin8` table for smooth motion with minimal CPU.
- Profiling can be enabled via `menuconfig → PRISM Playback → Enable temporal profiling`.

