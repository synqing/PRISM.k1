"""
Host-side parser harness for PRISM v1.1 metadata headers.

This package exposes helpers for building deterministic header vectors,
validating CRC coverage rules, and exercising tolerance for optional and
unknown metadata fields defined in R2.1/R2.2.
"""

from .builder import (
    PrismHeaderV10,
    PrismPatternMetaV11,
    ParsedHeader,
    build_header_blob,
    parse_header_blob,
    calculate_crc32,
    manifest_from_parsed,
)
from .vectors import (
    ParserVector,
    default_vector_set,
    validate_vector,
    write_vectors,
    load_vectors_from_disk,
    log_results,
)
from .artifacts import (
    ArtifactResult,
    validate_artifact,
    summarise_results as summarise_artifacts,
    dump_report as dump_artifact_report,
)

__all__ = [
    "PrismHeaderV10",
    "PrismPatternMetaV11",
    "ParsedHeader",
    "build_header_blob",
    "parse_header_blob",
    "calculate_crc32",
    "manifest_from_parsed",
    "ParserVector",
    "default_vector_set",
    "validate_vector",
    "write_vectors",
    "load_vectors_from_disk",
    "log_results",
    "ArtifactResult",
    "validate_artifact",
    "summarise_artifacts",
    "dump_artifact_report",
]
