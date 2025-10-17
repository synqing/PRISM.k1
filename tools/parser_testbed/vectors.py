"""
Vector generation and validation utilities for the parser harness.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
import json
from typing import Dict, List, Mapping, MutableMapping, Optional, Sequence

from ..tooling_core import ensure_parent_dir, log_info, write_json

from .builder import (
    CRC_PREFIX_SIZE,
    BASE_SIZE,
    META_SIZE,
    PrismHeaderV10,
    PrismPatternMetaV11,
    build_header_blob,
    manifest_from_parsed,
    parse_header_blob,
    validate_optional_fields,
    ramp_space_is_valid,
)


EXPECTED_PASS = "pass"
EXPECTED_CRC_MISMATCH = "crc-mismatch"
EXPECTED_LENGTH_ERROR = "length-error"
EXPECTED_INVALID_RAMP = "invalid-ramp-space"


@dataclass
class ParserVector:
    name: str
    category: str
    payload: bytes
    manifest: MutableMapping[str, object]
    expected_outcome: str = EXPECTED_PASS
    description: str = ""
    tags: Sequence[str] = field(default_factory=list)

    def manifest_payload(self) -> MutableMapping[str, object]:
        payload = dict(self.manifest)
        payload["description"] = self.description
        payload["expected"] = {"outcome": self.expected_outcome}
        if self.tags:
            payload["tags"] = list(self.tags)
        return payload

    def write(self, root: Path) -> Dict[str, Path]:
        bin_path = root / self.category / f"{self.name}.bin"
        ensure_parent_dir(bin_path)
        bin_path.write_bytes(self.payload)

        manifest_path = root / self.category / f"{self.name}.json"
        write_json(
            manifest_path,
            self.manifest_payload(),
            metadata={
                "vector": self.name,
                "category": self.category,
                "expected_outcome": self.expected_outcome,
            },
        )
        return {"binary": bin_path, "manifest": manifest_path}


@dataclass
class ValidationResult:
    vector: ParserVector
    status: str
    warnings: List[str] = field(default_factory=list)
    detail: Optional[str] = None
    recognised_fields: Sequence[str] = field(default_factory=list)
    unknown_fields: Sequence[str] = field(default_factory=list)
    computed_crc32: Optional[int] = None

    def is_success(self) -> bool:
        return self.status == "ok"


def _default_base() -> PrismHeaderV10:
    return PrismHeaderV10(
        version=0x0101,
        led_count=160,
        frame_count=96,
        fps=120,
        color_format=1,
        compression=0,
    )


def _default_meta() -> PrismPatternMetaV11:
    return PrismPatternMetaV11(
        version=0x01,
        motion_direction=1,
        sync_mode=2,
        reserved=0,
        params=(120, 0, 0, 0, 0, 0),
    )


def _vector_from_components(
    *,
    name: str,
    category: str,
    base: PrismHeaderV10,
    meta: PrismPatternMetaV11,
    extra_fields: Mapping[str, object],
    expected: str,
    description: str,
    tags: Sequence[str] = (),
) -> ParserVector:
    blob, manifest = build_header_blob(base, meta, extra_fields=extra_fields)
    return ParserVector(
        name=name,
        category=category,
        payload=blob,
        manifest=manifest,
        expected_outcome=expected,
        description=description,
        tags=list(tags),
    )


def _golden_vectors() -> List[ParserVector]:
    base = _default_base()
    meta = _default_meta()

    vectors = [
        _vector_from_components(
            name="default_palette",
            category="golden",
            base=base,
            meta=meta,
            extra_fields={
                "palette_id": "sunset-v1",
                "ramp_space": "hsluv",
                "show_params": {"gamma": 1.8, "brightness": 0.85},
            },
            expected=EXPECTED_PASS,
            description="Nominal header with recognised optional fields populated.",
            tags=("golden", "metadata"),
        ),
        _vector_from_components(
            name="unknown_fields_noop",
            category="golden",
            base=base,
            meta=meta,
            extra_fields={
                "palette_id": "sunrise-inline",
                "ramp_space": "oklab",
                "show_params": {"gamma": 2.0},
                "experimental_hint": "should-be-ignored",
            },
            expected=EXPECTED_PASS,
            description=(
                "Optional metadata containing an unknown key to confirm parser "
                "ignores non-recognised fields."
            ),
            tags=("golden", "unknown-field"),
        ),
        _vector_from_components(
            name="no_optional_fields",
            category="golden",
            base=base,
            meta=meta,
            extra_fields={},
            expected=EXPECTED_PASS,
            description="Header with no extended metadata to assert defaults apply.",
            tags=("golden", "defaults"),
        ),
    ]
    return vectors


def _mutation_crc_from_golden(source: ParserVector) -> ParserVector:
    buffer = bytearray(source.payload)
    crc_offset = CRC_PREFIX_SIZE
    buffer[crc_offset] ^= 0x01
    mutated = bytes(buffer)
    parsed = parse_header_blob(mutated)
    manifest = manifest_from_parsed(parsed)
    manifest["notes"] = "Stored CRC intentionally corrupted."
    return ParserVector(
        name="crc_corrupted",
        category="mutations",
        payload=mutated,
        manifest=manifest,
        expected_outcome=EXPECTED_CRC_MISMATCH,
        description="Header with flipped CRC byte to trigger validation failure.",
        tags=("mutation", "crc"),
    )


def _mutation_truncated_extra(source: ParserVector) -> ParserVector:
    buffer = bytearray(source.payload)
    length_offset = BASE_SIZE + META_SIZE
    extra_len = int.from_bytes(buffer[length_offset : length_offset + 2], "little")
    if extra_len < 2:
        raise ValueError("source vector must contain extended metadata")
    # Remove final byte so the length field overstates the payload size.
    truncated_payload = buffer[: -1]
    parsed_manifest = source.manifest_payload()
    parsed_manifest["notes"] = "Extended metadata truncated relative to length."
    return ParserVector(
        name="truncated_extended_metadata",
        category="mutations",
        payload=bytes(truncated_payload),
        manifest=parsed_manifest,
        expected_outcome=EXPECTED_LENGTH_ERROR,
        description="Vector with mismatched extended metadata length to force parse error.",
        tags=("mutation", "length"),
    )


def _mutation_invalid_ramp(base: PrismHeaderV10, meta: PrismPatternMetaV11) -> ParserVector:
    blob, manifest = build_header_blob(
        base,
        meta,
        extra_fields={
            "palette_id": "lab-test",
            "ramp_space": "xyz",  # deliberately invalid
            "show_params": {"gamma": 1.65},
        },
    )
    manifest["notes"] = "Ramp space deliberately invalid for fallback behaviour."
    return ParserVector(
        name="invalid_ramp_space",
        category="mutations",
        payload=blob,
        manifest=manifest,
        expected_outcome=EXPECTED_INVALID_RAMP,
        description="Optional metadata includes an invalid ramp_space enum member.",
        tags=("mutation", "enum"),
    )


def _mutation_bitflip_meta(source: ParserVector) -> ParserVector:
    buffer = bytearray(source.payload)
    # Flip a bit inside the metadata region covered by CRC to ensure mismatch.
    mut_offset = BASE_SIZE + 2  # adjust sync mode byte
    buffer[mut_offset] ^= 0x08
    mutated = bytes(buffer)
    parsed = parse_header_blob(mutated)
    manifest = manifest_from_parsed(parsed)
    manifest["notes"] = "Metadata byte flipped; CRC should detect change."
    return ParserVector(
        name="meta_bitflip_crc",
        category="mutations",
        payload=mutated,
        manifest=manifest,
        expected_outcome=EXPECTED_CRC_MISMATCH,
        description="Metadata change within CRC coverage to ensure detection.",
        tags=("mutation", "crc"),
    )


def _mutation_vectors(golden: List[ParserVector]) -> List[ParserVector]:
    golden_map = {vec.name: vec for vec in golden}
    default_vector = golden_map["default_palette"]
    unknown_vector = golden_map["unknown_fields_noop"]
    base = _default_base()
    meta = _default_meta()

    return [
        _mutation_crc_from_golden(default_vector),
        _mutation_bitflip_meta(default_vector),
        _mutation_truncated_extra(unknown_vector),
        _mutation_invalid_ramp(base, meta),
    ]


def default_vector_set(include_mutations: bool = True) -> List[ParserVector]:
    vectors = _golden_vectors()
    if include_mutations:
        vectors.extend(_mutation_vectors(vectors))
    return vectors


def write_vectors(vectors: Sequence[ParserVector], root: Path) -> Dict[str, Dict[str, Path]]:
    artefacts: Dict[str, Dict[str, Path]] = {}
    for vector in vectors:
        artefacts[vector.name] = vector.write(root)
    return artefacts


def load_vectors_from_disk(root: Path) -> List[ParserVector]:
    vectors: List[ParserVector] = []
    for manifest_path in sorted(root.rglob("*.json")):
        with manifest_path.open("r", encoding="utf-8") as fh:
            document = json.load(fh)
        payload = document.get("data", {})
        meta = document.get("meta", {})
        expected = payload.get("expected", {}).get("outcome", EXPECTED_PASS)
        description = payload.get("description", "")
        tags = payload.get("tags", [])
        manifest = {
            key: value
            for key, value in payload.items()
            if key not in {"description", "expected", "tags"}
        }
        manifest.setdefault("meta", {})
        manifest.setdefault("base", {})
        manifest.setdefault("extra", {})

        name = meta.get("vector") or manifest_path.stem
        category = meta.get("category") or manifest_path.parent.name

        bin_path = manifest_path.with_suffix(".bin")
        if not bin_path.exists():
            raise FileNotFoundError(f"Binary payload missing for {manifest_path}")
        payload_bytes = bin_path.read_bytes()

        vectors.append(
            ParserVector(
                name=name,
                category=category,
                payload=payload_bytes,
                manifest=manifest,
                expected_outcome=expected,
                description=description,
                tags=tags,
            )
        )
    return vectors


def validate_vector(vector: ParserVector) -> ValidationResult:
    try:
        parsed = parse_header_blob(vector.payload)
    except ValueError as exc:
        if vector.expected_outcome == EXPECTED_LENGTH_ERROR:
            return ValidationResult(
                vector=vector,
                status="ok",
                warnings=[str(exc)],
                detail="length-error detected during parse",
            )
        return ValidationResult(
            vector=vector,
            status="fail",
            detail=str(exc),
        )

    computed_crc = parsed.computed_crc32()
    recognised, unknown = validate_optional_fields(parsed.extra_fields)

    if vector.expected_outcome == EXPECTED_CRC_MISMATCH:
        if computed_crc == parsed.base.crc32:
            return ValidationResult(
                vector=vector,
                status="fail",
                detail="CRC expected to mismatch but matched stored value",
                computed_crc32=computed_crc,
                recognised_fields=recognised,
                unknown_fields=unknown,
            )
        return ValidationResult(
            vector=vector,
            status="ok",
            warnings=["CRC mismatch correctly detected"],
            computed_crc32=computed_crc,
            recognised_fields=recognised,
            unknown_fields=unknown,
        )

    if vector.expected_outcome == EXPECTED_INVALID_RAMP:
        ramp_value = parsed.extra_fields.get("ramp_space")
        if computed_crc != parsed.base.crc32:
            return ValidationResult(
                vector=vector,
                status="fail",
                detail="CRC mismatch encountered for invalid ramp vector",
                computed_crc32=computed_crc,
                recognised_fields=recognised,
                unknown_fields=unknown,
            )
        if ramp_space_is_valid(ramp_value if isinstance(ramp_value, str) else None):
            return ValidationResult(
                vector=vector,
                status="fail",
                detail="Ramp space expected to be invalid but validated successfully",
                computed_crc32=computed_crc,
                recognised_fields=recognised,
                unknown_fields=unknown,
            )
        return ValidationResult(
            vector=vector,
            status="ok",
            warnings=["ramp_space invalid; downstream parser should apply defaults"],
            computed_crc32=computed_crc,
            recognised_fields=recognised,
            unknown_fields=unknown,
        )

    if vector.expected_outcome == EXPECTED_LENGTH_ERROR:
        return ValidationResult(
            vector=vector,
            status="fail",
            detail="Vector expected to trigger length error but parsed successfully",
            computed_crc32=computed_crc,
            recognised_fields=recognised,
            unknown_fields=unknown,
        )

    # Default expectation is pass.
    if computed_crc != parsed.base.crc32:
        return ValidationResult(
            vector=vector,
            status="fail",
            detail="CRC mismatch for vector expected to pass",
            computed_crc32=computed_crc,
            recognised_fields=recognised,
            unknown_fields=unknown,
        )

    warnings: List[str] = []
    if unknown:
        warnings.append(f"Unknown fields present: {', '.join(unknown)}")
    ramp_value = parsed.extra_fields.get("ramp_space")
    if isinstance(ramp_value, str) and not ramp_space_is_valid(ramp_value):
        warnings.append("Invalid ramp_space encountered; defaults should apply")

    return ValidationResult(
        vector=vector,
        status="ok",
        warnings=warnings,
        computed_crc32=computed_crc,
        recognised_fields=recognised,
        unknown_fields=unknown,
    )


def summarise_results(results: Sequence[ValidationResult]) -> Dict[str, object]:
    summary = {
        "total": len(results),
        "passes": sum(1 for r in results if r.is_success()),
        "failures": [r.vector.name for r in results if not r.is_success()],
        "warnings": {
            r.vector.name: r.warnings
            for r in results
            if r.warnings
        },
    }
    return summary


def log_results(results: Sequence[ValidationResult]) -> None:
    summary = summarise_results(results)
    log_info(
        f"Validation results: {summary['passes']}/{summary['total']} passed, "
        f"{len(summary['failures'])} failed.",
    )
    if summary["failures"]:
        log_info(f"Failures: {', '.join(summary['failures'])}")
    if summary["warnings"]:
        for name, warnings in summary["warnings"].items():
            for warning in warnings:
                log_info(f"[warn] {name}: {warning}")
