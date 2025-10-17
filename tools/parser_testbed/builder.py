"""
Low-level helpers for building and parsing PRISM v1.1 headers on the host.

The routines in this module mirror the firmware-side layout (see
``firmware/components/storage/include/prism_parser.h``) so that synthetic
test vectors can be generated without compiling the firmware.
"""
from __future__ import annotations

from dataclasses import dataclass, field, replace
import json
import struct
from binascii import crc32
from typing import Dict, Iterable, Mapping, MutableMapping, Sequence, Tuple

BASE_STRUCT = struct.Struct("<4sHHIIBBHI40s")
META_STRUCT = struct.Struct("<BBBB6H")

BASE_SIZE = BASE_STRUCT.size  # 64 bytes
META_SIZE = META_STRUCT.size  # 16 bytes

CRC_PREFIX_SIZE = struct.calcsize("<4sHHIIBBH")  # bytes prior to crc32 field
META_CRC_PREFIX = 6  # first 6 bytes of metadata are covered by CRC

OPTIONAL_METADATA_FIELDS = frozenset({"palette_id", "ramp_space", "show_params"})
VALID_RAMP_SPACES = frozenset({"hsv", "hsl", "hsluv", "oklab", "oklch"})


@dataclass
class PrismHeaderV10:
    magic: bytes = b"PRSM"
    version: int = 0x0101
    led_count: int = 160
    frame_count: int = 0
    fps: int = 120
    color_format: int = 0
    compression: int = 0
    reserved1: int = 0
    crc32: int = 0
    padding: bytes = field(default_factory=lambda: b"\x00" * 40)


@dataclass
class PrismPatternMetaV11:
    version: int = 0x01
    motion_direction: int = 0
    sync_mode: int = 0
    reserved: int = 0
    params: Tuple[int, int, int, int, int, int] = (
        0,
        0,
        0,
        0,
        0,
        0,
    )

    def __post_init__(self) -> None:
        if len(self.params) != 6:
            raise ValueError("params must contain exactly 6 integers")


@dataclass
class ParsedHeader:
    base: PrismHeaderV10
    meta: PrismPatternMetaV11
    extra_fields: Dict[str, object]
    raw: bytes
    extra_length: int
    base_bytes: bytes
    meta_bytes: bytes

    def computed_crc32(self) -> int:
        return calculate_crc32(self.base_bytes, self.meta_bytes)


def _pack_base(header: PrismHeaderV10) -> bytes:
    if len(header.magic) != 4:
        raise ValueError("magic must be exactly 4 bytes")
    if len(header.padding) != 40:
        raise ValueError("padding must be exactly 40 bytes")
    return BASE_STRUCT.pack(
        header.magic,
        header.version,
        header.led_count,
        header.frame_count,
        header.fps,
        header.color_format,
        header.compression,
        header.reserved1,
        header.crc32,
        header.padding,
    )


def _pack_meta(meta: PrismPatternMetaV11) -> bytes:
    return META_STRUCT.pack(
        meta.version,
        meta.motion_direction,
        meta.sync_mode,
        meta.reserved,
        *meta.params,
    )


def calculate_crc32(base_bytes: bytes, meta_bytes: bytes) -> int:
    """
    Calculate CRC32 using the same coverage as the firmware implementation.

    The CRC spans all bytes up to (but not including) ``crc32`` in the base
    header and the first six bytes of the v1.1 metadata structure.
    """
    crc_input = base_bytes[:CRC_PREFIX_SIZE] + meta_bytes[:META_CRC_PREFIX]
    return crc32(crc_input) & 0xFFFFFFFF


def _encode_extra_metadata(extra_fields: Mapping[str, object]) -> Tuple[int, bytes]:
    if not extra_fields:
        return 0, b""

    payload = json.dumps(extra_fields, sort_keys=True).encode("utf-8")
    if len(payload) > 0xFFFF:
        raise ValueError("extended metadata payload exceeds 64KiB limit")
    return len(payload), payload


def _make_manifest(
    base: PrismHeaderV10,
    meta: PrismPatternMetaV11,
    *,
    crc_value: int,
    extra_length: int,
    extra_fields: Mapping[str, object],
) -> MutableMapping[str, object]:
    recognised, unknown = validate_optional_fields(extra_fields)
    return {
        "base": {
            "magic": base.magic.decode("ascii"),
            "version": base.version,
            "led_count": base.led_count,
            "frame_count": base.frame_count,
            "fps": base.fps,
            "color_format": base.color_format,
            "compression": base.compression,
            "reserved1": base.reserved1,
            "crc32": crc_value,
            "crc32_hex": f"0x{crc_value:08X}",
        },
        "meta": {
            "version": meta.version,
            "motion_direction": meta.motion_direction,
            "sync_mode": meta.sync_mode,
            "reserved": meta.reserved,
            "params": list(meta.params),
        },
        "extra": {
            "length": extra_length,
            "fields": dict(extra_fields),
            "recognised_fields": recognised,
            "unknown_fields": unknown,
        },
    }


def build_header_blob(
    base: PrismHeaderV10,
    meta: PrismPatternMetaV11,
    *,
    extra_fields: Mapping[str, object] | None = None,
) -> Tuple[bytes, MutableMapping[str, object]]:
    """
    Construct a binary header compliant with the v1.1 layout.

    Returns the binary payload alongside a descriptive manifest.
    """
    meta_bytes = _pack_meta(meta)
    zero_crc_base = _pack_base(replace(base, crc32=0))

    crc_value = calculate_crc32(zero_crc_base, meta_bytes)
    final_base_bytes = _pack_base(replace(base, crc32=crc_value))

    extra_fields = dict(extra_fields or {})
    extra_length, extra_payload = _encode_extra_metadata(extra_fields)
    extra_block = struct.pack("<H", extra_length) + extra_payload

    blob = final_base_bytes + meta_bytes + extra_block

    manifest = _make_manifest(
        replace(base, crc32=crc_value),
        meta,
        crc_value=crc_value,
        extra_length=extra_length,
        extra_fields=extra_fields,
    )
    return blob, manifest


def parse_header_blob(blob: bytes) -> ParsedHeader:
    """
    Parse a binary header emitted by :func:`build_header_blob`.

    Raises:
        ValueError: when the blob is too small or length fields are inconsistent.
    """
    if len(blob) < BASE_SIZE:
        raise ValueError("header blob shorter than v1.0 base header")

    base_tuple = BASE_STRUCT.unpack(blob[:BASE_SIZE])
    magic = base_tuple[0]
    if magic != b"PRSM":
        raise ValueError("invalid magic prefix in header blob")

    base = PrismHeaderV10(
        magic=magic,
        version=base_tuple[1],
        led_count=base_tuple[2],
        frame_count=base_tuple[3],
        fps=base_tuple[4],
        color_format=base_tuple[5],
        compression=base_tuple[6],
        reserved1=base_tuple[7],
        crc32=base_tuple[8],
        padding=base_tuple[9],
    )

    offset = BASE_SIZE
    meta = PrismPatternMetaV11()
    meta_bytes = b"\x00" * META_SIZE

    if base.version == 0x0101:
        if len(blob) < offset + META_SIZE:
            raise ValueError("v1.1 header missing metadata segment")
        meta_bytes = blob[offset : offset + META_SIZE]
        offset += META_SIZE

        meta_tuple = META_STRUCT.unpack(meta_bytes)
        params = tuple(meta_tuple[4:])
        meta = PrismPatternMetaV11(
            version=meta_tuple[0],
            motion_direction=meta_tuple[1],
            sync_mode=meta_tuple[2],
            reserved=meta_tuple[3],
            params=params,  # type: ignore[arg-type]
        )
    elif base.version != 0x0100:
        raise ValueError(f"unsupported header version 0x{base.version:04X}")

    extra_length = 0
    extra_fields: Dict[str, object] = {}
    if len(blob) >= offset + 2:
        extra_length = struct.unpack_from("<H", blob, offset)[0]
        offset += 2
        if len(blob) < offset + extra_length:
            raise ValueError("extended metadata truncated according to length field")
        extra_payload = blob[offset : offset + extra_length]
        offset += extra_length
        if extra_payload:
            extra_fields = json.loads(extra_payload.decode("utf-8"))
            if not isinstance(extra_fields, dict):
                raise ValueError("extended metadata payload must decode to a JSON object")

    return ParsedHeader(
        base=base,
        meta=meta,
        extra_fields=extra_fields,
        raw=blob,
        extra_length=extra_length,
        base_bytes=blob[:BASE_SIZE],
        meta_bytes=meta_bytes,
    )


def manifest_from_parsed(parsed: ParsedHeader) -> MutableMapping[str, object]:
    """Create a manifest dictionary for a :class:`ParsedHeader` instance."""
    return _make_manifest(
        parsed.base,
        parsed.meta,
        crc_value=parsed.base.crc32,
        extra_length=parsed.extra_length,
        extra_fields=parsed.extra_fields,
    )


def validate_optional_fields(extra_fields: Mapping[str, object]) -> Tuple[list[str], list[str]]:
    """
    Split recognised vs unknown optional metadata keys.

    Returns:
        recognised, unknown -- each as sorted iterables of field names.
    """
    recognised = sorted(OPTIONAL_METADATA_FIELDS.intersection(extra_fields.keys()))
    unknown = sorted(set(extra_fields.keys()) - OPTIONAL_METADATA_FIELDS)
    return recognised, unknown


def ramp_space_is_valid(value: str | None) -> bool:
    """Return True when the ramp space is a recognised enum member."""
    if value is None:
        return True
    return value in VALID_RAMP_SPACES
