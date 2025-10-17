# Agent 2 – Parser & Format (v1.1, Packaging Integration)

Pre‑Reading
- `firmware/docs/research/R2.1_metadata_extensions.md`
- `firmware/docs/research/R2.2_header_crc_implications.md`
- `firmware/docs/research/R3.4_packaging_tradeoffs.md`

Pipeline
1) Metadata Extensions (v1.1)
   - Add optional fields: `palette_id`, `ramp_space` (hsv|hsl|hsluv|oklab|oklch), `show_params` (opaque JSON/CBOR)
   - Defaults apply if absent; unknown fields are no‑ops
   - Unit tests for backcompat and unknown fields

2) Header/CRC Behavior
   - Keep v1.1 CRC coverage unchanged: base up to crc32 + first 6 bytes of meta
   - Document parser behavior for extended metadata

3) Packaging Integration
   - Coordinate with Agent 3 (#30): parse packaged artifacts, validate round‑trip
   - Ensure decode and metadata read paths are robust

Usage
- Implement per existing parser structure; keep tests under `firmware/components/storage/tests` or equivalent
- Commit after each subtask; include task IDs; mark done in Task Master

Notes
- Do not expand CRC coverage without amending R2.2
- Keep changes backward compatible
