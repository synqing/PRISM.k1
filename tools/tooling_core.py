"""
Shared CLI scaffolding and deterministic writers for PRISM tooling.

This module centralises argparse helpers, lightweight logging, metadata
handling, and JSON/CSV writers so downstream CLIs can produce reproducible
artifacts that align with the PRD requirements.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
from typing import Iterable, Mapping, MutableMapping, Optional, Sequence, Union
import sys

PathLike = Union[str, os.PathLike[str]]

DEFAULT_VERSION = "1.0"


def log_info(message: str) -> None:
    """Write a message to stdout."""
    print(message, file=sys.stdout)


def log_error(message: str) -> None:
    """Write a message to stderr."""
    print(message, file=sys.stderr)


def _validate_output_path(path_str: str) -> str:
    """argparse type to ensure an output path is writable (parent directory exists or can be created)."""
    path = Path(path_str)
    parent = path.parent or Path(".")

    if path.exists() and path.is_dir():
        raise argparse.ArgumentTypeError(f"{path} is a directory; expected file path")

    if parent.exists() and not parent.is_dir():
        raise argparse.ArgumentTypeError(f"Parent {parent} is not a directory")

    return str(path)


def _validate_optional_output_path(path_str: Optional[str]) -> Optional[str]:
    return None if path_str is None else _validate_output_path(path_str)


def _validate_existing_file(path_str: str) -> str:
    path = Path(path_str)
    if not path.exists():
        raise argparse.ArgumentTypeError(f"Metadata seed {path} does not exist")
    if not path.is_file():
        raise argparse.ArgumentTypeError(f"Metadata seed {path} is not a file")
    return str(path)


def build_parser(description: str, *, allow_csv: bool = True) -> argparse.ArgumentParser:
    """
    Create an argparse parser with shared options.

    Injects --output (required), optional --csv and --meta flags, and ensures
    paths are validated before downstream code executes.
    """
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument(
        "--output",
        required=True,
        type=_validate_output_path,
        help="Path to write the primary JSON artifact",
    )
    if allow_csv:
        parser.add_argument(
            "--csv",
            type=_validate_optional_output_path,
            help="Optional CSV output path mirroring the JSON payload",
        )
    parser.add_argument(
        "--meta",
        type=_validate_existing_file,
        help="Path to metadata JSON seed to merge into generated artifacts",
    )
    return parser


def ensure_parent_dir(path: Path) -> None:
    """Create the parent directory for a file path if it is missing."""
    parent = path.parent or Path(".")
    parent.mkdir(parents=True, exist_ok=True)


def load_metadata(meta_path: Optional[PathLike]) -> MutableMapping[str, object]:
    """Load metadata JSON from disk (returns empty dict when path is None)."""
    if meta_path is None:
        return {}

    meta_file = Path(meta_path)
    with meta_file.open("r", encoding="utf-8") as fh:
        data = json.load(fh)
    if not isinstance(data, dict):
        raise ValueError(f"Metadata at {meta_file} must be a JSON object")
    return data


def _prepare_metadata(
    metadata: Optional[Mapping[str, object]],
    *,
    extra: Optional[Mapping[str, object]] = None,
) -> MutableMapping[str, object]:
    merged: MutableMapping[str, object] = {}
    if metadata:
        merged.update(metadata)
    if extra:
        merged.update(extra)
    return merged


def write_json(
    output_path: PathLike,
    payload: Union[Mapping[str, object], Sequence[object]],
    metadata: Optional[Mapping[str, object]] = None,
    *,
    version: str = DEFAULT_VERSION,
    extra_meta: Optional[Mapping[str, object]] = None,
    ensure_ascii: bool = False,
) -> Path:
    """
    Write a deterministic JSON file with shared metadata structure.

    Output layout:
    {
        "data": <payload>,
        "meta": {...},  # merged metadata
        "version": "1.0"
    }
    """
    path = Path(output_path)
    ensure_parent_dir(path)

    document: MutableMapping[str, object] = {
        "version": version,
        "data": payload,
        "meta": _prepare_metadata(metadata, extra=extra_meta),
    }

    with path.open("w", encoding="utf-8", newline="\n") as fh:
        json.dump(document, fh, indent=2, sort_keys=True, ensure_ascii=ensure_ascii)
        fh.write("\n")
    return path


def _format_csv_row(row: Sequence[object]) -> str:
    return ",".join(str(item) for item in row)


def write_csv(
    output_path: PathLike,
    rows: Iterable[Sequence[object]],
    headers: Sequence[str],
    metadata: Optional[Mapping[str, object]] = None,
    *,
    extra_meta: Optional[Mapping[str, object]] = None,
) -> Optional[Path]:
    """
    Write deterministic CSV with metadata preamble (comment lines).

    Metadata is encoded as JSON on the first comment line to keep parsing simple.
    """
    if output_path is None:
        return None

    path = Path(output_path)
    ensure_parent_dir(path)

    combined_meta = _prepare_metadata(metadata, extra=extra_meta)
    with path.open("w", encoding="utf-8", newline="\n") as fh:
        fh.write(f"# meta: {json.dumps(combined_meta, sort_keys=True)}\n")
        fh.write(_format_csv_row(headers) + "\n")
        for row in rows:
            if len(row) != len(headers):
                raise ValueError("CSV row length does not match headers")
            fh.write(_format_csv_row(row) + "\n")
    return path


__all__ = [
    "DEFAULT_VERSION",
    "build_parser",
    "ensure_parent_dir",
    "load_metadata",
    "log_error",
    "log_info",
    "write_csv",
    "write_json",
]
