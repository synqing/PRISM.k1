# IR v0.1 — Intermediate Representation (PROPOSED)
**Goal:** Encode a small, deterministic program that the ESP32-S3 executes **per pixel per frame** to produce RGB.
Implicit inputs per pixel: **u, v** (0..1), uniforms: **time**, **macros[4]**.

## Binary layout
```
struct K1IRHeader {
  char     magic[4];   // "K1IR"
  uint16_t version;    // 1
  uint16_t nops;       // number of ops
  uint16_t nconst;     // number of 32-bit constants
  uint16_t npal;       // number of palettes (256*RGB8 each)
};

// [constants ...]  (float32 array length nconst)
// [palettes  ...]  (npal blocks of 256*3 bytes)
// [ops       ...]  (nops variable-size instructions)
```
All values little-endian.

## VM model
- 16 float registers r0..r15 (per-pixel)
- 1 color register c0 (RGB float 0..1)
- uniforms: time, macros[4] (provided separately each frame)
- instruction stream is linear; no branches in v0.1

## Opcodes (encoding: [u8 opcode][payload...])
- `0x01 ANGLE dst`            → r[dst] = atan2(v-0.5, u-0.5) / (2*pi) in [0..1]
- `0x02 RADIUS dst`           → r[dst] = sqrt((u-0.5)^2 + (v-0.5)^2) * 2 in [0..1]
- `0x03 LOAD_CONST dst idx`   → r[dst] = const[idx]
- `0x04 ADD dst a b`          → r[dst] = r[a] + r[b]
- `0x05 MUL dst a b`          → r[dst] = r[a] * r[b]
- `0x06 ADDK dst a kidx`      → r[dst] = r[a] + const[kidx]
- `0x07 MULK dst a kidx`      → r[dst] = r[a] * const[kidx]
- `0x08 SINE dst a`           → r[dst] = sin(2π * r[a])
- `0x09 HUESHIFT dst a kidx`  → r[dst] = fract(r[a] + const[kidx])  // 0..1
- `0x0A PALETTEMAP pal a`     → c0 = LUT[pal][ clamp01(r[a])*255 ]  // RGB
- `0x0B BRIGHTNESS kidx`      → c0 = c0 * const[kidx]
- `0x0C MIX_COLOR kidx`       → c0 = lerp(prevColor, c0, const[kidx]) // fade
- `0x0D OUTPUT`               → write c0 to LED buffer (after geometry/color pipeline)
```
Notes:
- ANGLE/RADIUS are cheap to compute and match your FL.ledstrip fields.
- `prevColor` is the previous-frame pixel color (for temporal fade).
- All angles normalized to [0..1] for convenience.
