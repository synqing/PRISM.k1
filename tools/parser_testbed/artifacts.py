"""
Validation helpers for real .prism artifacts produced by the packaging pipeline.
"""
from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Sequence

from .builder import (
    ParsedHeader,
    manifest_from_parsed,
    parse_header_blob,
    ramp_space_is_valid,
    validate_optional_fields,
)


@dataclass
class ArtifactResult:
    path: Path
    status: str
    warnings: List[str]
    detail: Optional[str]
    manifest: Dict[str, object]

    def is_success(self) -> bool:
        return self.status == "ok"


def _build_manifest(parsed: ParsedHeader, *, file_size: int) -> Dict[str, object]:
    manifest = manifest_from_parsed(parsed)
    manifest["file_size"] = file_size
    return manifest


def validate_artifact(path: Path) -> ArtifactResult:
    """
    Parse a .prism artifact and verify header CRC and metadata rules.

    Returns a structured result capturing warnings and parsed manifest data.
    """
    data = path.read_bytes()
    warnings: List[str] = []
    try:
        parsed = parse_header_blob(data)
    except ValueError as exc:
        return ArtifactResult(
            path=path,
            status="fail",
            warnings=[],
            detail=str(exc),
            manifest={},
        )

    computed_crc = parsed.computed_crc32()
    if computed_crc != parsed.base.crc32:
        return ArtifactResult(
            path=path,
            status="fail",
            warnings=[],
            detail=f"Header CRC mismatch (computed=0x{computed_crc:08X} stored=0x{parsed.base.crc32:08X})",
            manifest=_build_manifest(parsed, file_size=len(data)),
        )

    recognised, unknown = validate_optional_fields(parsed.extra_fields)
    if unknown:
        warnings.append(f"Unknown metadata fields present: {', '.join(unknown)}")

    ramp_value = parsed.extra_fields.get("ramp_space")
    if isinstance(ramp_value, str) and not ramp_space_is_valid(ramp_value):
        warnings.append("Invalid ramp_space; downstream tools should apply defaults.")

    manifest = _build_manifest(parsed, file_size=len(data))
    manifest["metadata"] = parsed.extra_fields
    manifest["recognised_fields"] = recognised
    manifest["unknown_fields"] = unknown

    return ArtifactResult(
        path=path,
        status="ok",
        warnings=warnings,
        detail=None,
        manifest=manifest,
    )


def summarise_results(results: Sequence[ArtifactResult]) -> Dict[str, object]:
    return {
        "total": len(results),
        "passes": sum(1 for result in results if result.is_success()),
        "failures": [str(result.path) for result in results if not result.is_success()],
        "warnings": {
            str(result.path): result.warnings
            for result in results
            if result.warnings
        },
    }


def dump_report(results: Sequence[ArtifactResult], report_path: Path) -> None:
    payload = {
        "summary": summarise_results(results),
        "artifacts": [
            {
                "path": str(result.path),
                "status": result.status,
                "warnings": result.warnings,
                "detail": result.detail,
                "manifest": result.manifest,
            }
            for result in results
        ],
    }
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
