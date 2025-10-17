# PRISM Protocol – Source of Truth (SoT)

Purpose: single, pinned index of authoritative protocol + packing artifacts used by PRISM Studio (app) and ESP32‑S3 firmware (device). Changes to these require coordinated updates.

## Pinned SHAs

- Repo: synqing/PRISM.k1 (this repo)
  - ADR‑009 packer (Studio): `studio/src/lib/bake/packPrism.ts` @ HEAD
  - Uploader (Tauri/Rust): `studio/src-tauri/src/lib.rs` @ HEAD
- Repo: synqing/PRISM.k1 (firmware subpaths)
  - Protocol parser API: `firmware/components/network/include/protocol_parser.h` @ HEAD
  - Protocol parser impl: `firmware/components/network/protocol_parser.c` @ HEAD
  - Core config (limits): `firmware/components/core/include/prism_config.h` @ HEAD

Tip: Update these lines with exact commit SHAs when releasing (e.g., via release checklist). For development branches, HEAD is acceptable but must be frozen before firmware/app releases.

## TLV Framing (device owns)

Frame layout: `[TYPE:1][LEN_BE:2][PAYLOAD:LEN][CRC32_BE:4]`

Types in use:
- `0x10` PUT_BEGIN – payload: `[name_len:1][name:bytes][size_be:u32][crc32_be:u32]`
- `0x11` PUT_DATA  – payload: `[offset_be:u32][data:bytes]`, chunked (LEN ≤ TLV_MAX_PAYLOAD_SIZE)
- `0x12` PUT_END   – payload: empty; device recomputes CRC and validates session
- `0x20` CONTROL   – subcommands: PLAY/STOP/BRIGHTNESS/GAMMA (see below)
- `0x30` STATUS    – payload: `[verlen_le:u32][ver][ledCount_le:u16][avail_le:u32][maxChunk_le:u16][caps_le:u32?]`

Lengths/limits (device owns):
- `TLV_MAX_PAYLOAD_SIZE` (typ. 4089 = 4096 − 7) – payload MAX
- `PATTERN_MAX_SIZE` 262144 (256KB) – .prism total bytes limit

## CONTROL Subcommands (device owns)

CONTROL (`0x20`) subcommands and payloads:
- `0x01` PLAY     – `[name_len:1][name:bytes]`
- `0x02` STOP     – `[]`
- `0x10` BRIGHTNESS – `[target:u8][duration_ms_be:u16]` (target 0..255)
- `0x11` GAMMA      – `[gamma_x100_be:u16][duration_ms_be:u16]` (e.g., 220 == 2.20)

## STATUS Schema (device owns)

Payload fields, little-endian multi-byte integers:
- `verlen_le:u32` + `ver` (implementation-defined version bytes)
- `ledCount_le:u16`
- `avail_le:u32` bytes available
- `maxChunk_le:u16` maximum PUT_DATA payload size
- Optional `caps_le:u32` – bitmask: `1<<0 supports_compression`, `1<<1 supports_resume`, `1<<2 supports_events`, `1<<3 supports_palette`

## .prism Packing (shared contract)

- 64‑byte header parity (ADR‑009): magic `PRSM`, version, header_size, effect/channel/flags, name, brightness/speed, palette metadata.
- Optional palette block: `count * 4` bytes (RGBA), CRC32 at header[56:60]; header CRC32 at [60:64] over bytes 0..59.
- Payload: raw RGB frames (default). Optional compressed payload (XOR/RLE) is behind a capability gate and NOT enabled by default.
- Size guard: total bytes ≤ 256KB (PATTERN_MAX_SIZE).

## Contracts

- Device validates CRC at PUT_END (no CRC in PUT_END payload). PUT_END payload is empty.
- App cancels upload by closing WS (no PUT_END); device must clear session on close.
- App queries STATUS and honors `maxChunk` before PUT_DATA.

## Proposed v0.9 Deltas (for discussion)

- WebSocket frame header: adopt a richer header (type, flags, rsv, seq, len, crc32) behind a STATUS capability bit; provide back-compat with current simple TLV.
- PUT_END payload: include final CRC in payload when v0.9 is negotiated; current default remains empty payload with device-side CRC recompute.
- CONTROL types: consider splitting CONTROL subcommands into top-level message types when caps negotiated; otherwise keep CONTROL (0x20) with subcommands.

## Back‑links (to add in code headers)

- Studio uploader header: link to this file + exact commit SHA.
- Firmware protocol_parser.h header: link to this file + firmware commit SHA.

## Release Checklist (excerpt)

1) Pin SHAs in this document and in code headers.
2) Confirm TLV types and CONTROL subcommands remain unchanged.
3) Confirm STATUS schema (including caps if present).
4) Run parity + soak tests (50 uploads) and ensure thresholds met.

## Related Specs

- Motion Node Kit mapping (from FL.ledstrip): `docs/motion/fl-node-kit.md`
- PR Template adoption notes: `docs/process/pr-templates.md`
