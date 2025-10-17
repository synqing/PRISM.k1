"""Sanity-check packaged .prism files (header, size, CRC)."""
from __future__ import annotations

import argparse
import glob
from pathlib import Path
from typing import Iterable, Sequence

import zlib

from tools.parser_testbed import builder


def check_file(path: Path) -> None:
    data = path.read_bytes()
    base_size = builder.BASE_SIZE
    meta_size = builder.META_SIZE
    if len(data) < base_size + meta_size + 2 + 4:
        raise ValueError(f"{path}: file too small to contain header + payload")

    extra_len = int.from_bytes(data[base_size + meta_size : base_size + meta_size + 2], "little")
    header_len = base_size + meta_size + 2 + extra_len
    header_blob = data[:header_len]
    parsed = builder.parse_header_blob(header_blob)
    if parsed.base.magic != b"PRSM":
        raise ValueError(f"{path}: invalid magic {parsed.base.magic!r}")
    if parsed.base.version != 0x0101:
        raise ValueError(f"{path}: unexpected version {parsed.base.version:#04x}")
    if parsed.base.led_count == 0 or parsed.base.frame_count == 0:
        raise ValueError(f"{path}: invalid counts led={parsed.base.led_count} frames={parsed.base.frame_count}")

    payload = data[header_len:-4]
    crc_stored = int.from_bytes(data[-4:], "little")
    crc_calc = zlib.crc32(payload) & 0xFFFFFFFF
    if crc_calc != crc_stored:
        raise ValueError(f"{path}: payload CRC mismatch (expected {crc_calc:#08x}, stored {crc_stored:#08x})")


def iter_files(patterns: Sequence[str]) -> Iterable[Path]:
    for pattern in patterns:
        matches = glob.glob(pattern, recursive=True)
        if not matches:
            raise FileNotFoundError(f"No files found for pattern: {pattern}")
        for match in matches:
            path = Path(match)
            if path.is_file():
                yield path


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Sanity check .prism files")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--glob", nargs="+", help="Glob pattern(s) for .prism files")
    group.add_argument("--files", nargs="+", help="Explicit list of files to validate")
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> None:
    args = parse_args(argv)
    paths = args.files if args.files else args.glob
    failures = []
    for path in iter_files(paths):
        try:
            check_file(path)
            print(f"OK  {path}")
        except Exception as exc:  # pragma: no cover - validation errors are reported to user
            failures.append(f"{path}: {exc}")
    if failures:
        raise SystemExit("\n".join(failures))


if __name__ == "__main__":  # pragma: no cover
    main()
