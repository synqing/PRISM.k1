from __future__ import annotations

from pathlib import Path
from typing import Optional

import pytest

from tools.parser_testbed import (
    PrismHeaderV10,
    PrismPatternMetaV11,
    build_header_blob,
    default_vector_set,
    parse_header_blob,
    validate_vector,
    validate_artifact,
)
from tools.parser_testbed import runner as parser_runner
from tools.parser_testbed.vectors import (
    EXPECTED_CRC_MISMATCH,
    EXPECTED_INVALID_RAMP,
    EXPECTED_LENGTH_ERROR,
    ParserVector,
    load_vectors_from_disk,
)


def _write_artifact(tmp_path: Path, *, extra: Optional[dict] = None) -> Path:
    base = PrismHeaderV10(
        led_count=8,
        frame_count=4,
        fps=30,
        color_format=1,
        compression=0,
    )
    meta = PrismPatternMetaV11(
        version=0x01,
        motion_direction=1,
        sync_mode=0,
        params=(1, 2, 3, 4, 5, 6),
    )
    fields = {
        "palette_id": "artifact-test",
        "ramp_space": "hsluv",
        "show_params": {"gamma": 1.8},
    }
    if extra:
        fields.update(extra)

    blob, _ = build_header_blob(base, meta, extra_fields=fields)
    path = tmp_path / "artifact.prism"
    path.write_bytes(blob + b"\x00\x01\x02\x03")
    return path


def test_build_and_parse_roundtrip() -> None:
    base = PrismHeaderV10(frame_count=42, fps=144, color_format=2, compression=1)
    meta = PrismPatternMetaV11(
        version=0x01,
        motion_direction=3,
        sync_mode=1,
        params=(10, 20, 30, 40, 50, 60),
    )
    extra = {
        "palette_id": "sunset-test",
        "ramp_space": "oklch",
        "show_params": {"gamma": 1.9},
        "unknown_hint": "noop",
    }

    blob, manifest = build_header_blob(base, meta, extra_fields=extra)
    parsed = parse_header_blob(blob)

    assert parsed.base.version == base.version
    assert parsed.base.frame_count == base.frame_count
    assert parsed.meta.motion_direction == meta.motion_direction
    assert parsed.meta.params == meta.params
    assert parsed.extra_fields["palette_id"] == extra["palette_id"]
    assert parsed.extra_fields["ramp_space"] == extra["ramp_space"]
    assert parsed.extra_fields["show_params"] == extra["show_params"]

    assert parsed.computed_crc32() == parsed.base.crc32
    assert manifest["extra"]["unknown_fields"] == ["unknown_hint"]


def test_validate_vector_outcomes() -> None:
    vectors = default_vector_set(include_mutations=True)
    results = {vec.name: validate_vector(vec) for vec in vectors}

    assert all(result.is_success() for result in results.values())
    assert results["default_palette"].warnings == []
    assert results["unknown_fields_noop"].warnings, "unknown field should register warning"
    assert results["crc_corrupted"].vector.expected_outcome == EXPECTED_CRC_MISMATCH
    assert any("CRC mismatch" in warn for warn in results["crc_corrupted"].warnings)
    assert results["invalid_ramp_space"].vector.expected_outcome == EXPECTED_INVALID_RAMP
    assert any(
        "ramp_space invalid" in warn for warn in results["invalid_ramp_space"].warnings
    )
    assert results["truncated_extended_metadata"].vector.expected_outcome == EXPECTED_LENGTH_ERROR


def test_runner_cli_generates_and_regresses(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    output_dir = tmp_path / "vectors"

    exit_code = parser_runner.main(
        ["generate", "--output-dir", str(output_dir), "--include-mutations"]
    )
    assert exit_code == 0

    # Ensure files landed on disk and can be loaded.
    loaded_vectors = load_vectors_from_disk(output_dir)
    assert loaded_vectors, "no vectors loaded from generated artifacts"
    assert all(isinstance(vec, ParserVector) for vec in loaded_vectors)

    # Regression should succeed (mutations expected to pass validation logic).
    exit_code = parser_runner.main(["regress", "--input-dir", str(output_dir)])
    assert exit_code == 0


def test_validate_artifact_success(tmp_path: Path) -> None:
    artifact = _write_artifact(tmp_path)
    result = validate_artifact(artifact)
    assert result.is_success()
    assert result.warnings == []
    assert result.manifest["base"]["version"] == 0x0101


def test_validate_artifact_crc_failure(tmp_path: Path) -> None:
    artifact = _write_artifact(tmp_path)
    content = bytearray(artifact.read_bytes())
    content[0x40] ^= 0x01  # mutate metadata within CRC coverage
    artifact.write_bytes(content)
    result = validate_artifact(artifact)
    assert not result.is_success()
    assert result.detail is not None
    assert "CRC mismatch" in result.detail


def test_runner_cli_inspect(tmp_path: Path) -> None:
    artifact = _write_artifact(tmp_path)
    exit_code = parser_runner.main(["inspect", "--files", str(artifact)])
    assert exit_code == 0
