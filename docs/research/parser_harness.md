# Parser Harness Overview

Owner: Agent 2  
Status: Draft (host tooling landed; awaiting Task #30 packaging outputs)

## Purpose

- Provide a host-side reference implementation for PRISM v1.1 header handling so metadata changes can be validated without compiling firmware.
- Exercise CRC coverage expectations from R2.2 (up to `crc32` + first 6 bytes of metadata) with both golden and mutated vectors.
- Verify tolerance rules from R2.1 by generating optional metadata, unknown fields, and absent-field defaults via synthetic test cases.
- Supply a regression harness that future packaging tooling (Task #30) can feed once palette+indices exporters are available.

## Directory Layout

```
tools/parser_testbed/
├── __init__.py                 # Public package exports
├── builder.py                  # Header pack/unpack + CRC helpers
├── runner.py                   # CLI entrypoint (`python3 -m tools.parser_testbed.runner`)
├── vectors.py                  # Vector definitions, validation logic, disk IO
├── Makefile                    # Convenience targets for local/CI runs
└── vectors/                    # Generated artefacts (golden + mutations)
```

Unit coverage lives in `tools/tests/test_parser_testbed.py`.

## Header Encoding Summary

- Base header mirrors `prism_header_v10_t` (64 bytes).  
- Metadata segment matches `prism_pattern_meta_v11_t` (16 bytes).  
- Extended metadata payload:
  - 16-bit little-endian length field.
  - JSON blob (sorted keys, UTF-8) containing optional fields:
    - `palette_id` (string)
    - `ramp_space` (`hsv|hsl|hsluv|oklab|oklch`)
    - `show_params` (JSON object or CBOR-in-JSON placeholder)
  - Unknown keys allowed; parser harness surfaces them as warnings.
- CRC32 computation: IEEE polynomial via Python `binascii.crc32` over:
  - Bytes 0..19 (base header up to _but not including_ the stored `crc32`).
  - Bytes 64..69 (first 6 bytes of the metadata struct).

## CLI Workflows

```
# Generate golden vectors (with optional mutations)
python3 -m tools.parser_testbed.runner generate --include-mutations

# Generate only mutation vectors
python3 -m tools.parser_testbed.runner mutate --output-dir tmp/vectors

# Run regression (optionally regenerate vectors first)
python3 -m tools.parser_testbed.runner regress --regenerate
python3 -m tools.parser_testbed.runner regress --input-dir tmp/vectors --category mutations

# Inspect packaged artifacts
python3 -m tools.parser_testbed.runner inspect --files out/preset.prism
python3 -m tools.parser_testbed.runner inspect --glob "out/**/*.prism" --report reports/parser-artifacts.json
```

`runner.py` emits binary headers (`.bin`) and manifests (`.json`) into `tools/parser_testbed/vectors/<category>/`.

## Vector Set (Initial)

### Golden
- `default_palette`: recognised optional fields populated, validates CRC happy path.
- `unknown_fields_noop`: additional key (`experimental_hint`) exercises ignore behaviour.
- `no_optional_fields`: ensures defaults apply when extended metadata absent.

### Mutations
- `crc_corrupted`: stored CRC byte flipped; validation expects mismatch detection.
- `meta_bitflip_crc`: mutation in CRC-covered metadata region.
- `truncated_extended_metadata`: length field larger than payload (parse failure expected).
- `invalid_ramp_space`: ramp space outside enum; harness flags fallback requirement.

Each manifest captures recognised/unknown keys plus computed CRC (`crc32` + hex string).

## Integration Notes

- Harness operates entirely on host Python (3.11+).  
- Uses shared CLI helpers from `tools/tooling_core.py` to maintain deterministic JSON/CSV writers.  
- Designed to feed future firmware parser stubs via binary vectors; Task #30 can drop packaging outputs into `tools/parser_testbed/vectors/packaging/` and reuse regression pipeline, or hand `.prism` files directly to the `inspect` command.
- Results may be written to JSON via `--report` for CI attachments.

## Next Steps

1. Integrate Makefile targets (`make parser-vectors`, `make parser-regress`) into CI once hooks are defined.
2. Integrate firmware parser binary via FFI or ctypes shim for cross-checking against host expectations.
3. Expand vector catalog when Task #30 delivers palette/index sequences (round-trip, XOR/RLE options).
4. Wire the `inspect` command into packaging CI once end-to-end artifacts are generated.
