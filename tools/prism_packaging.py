#!/usr/bin/env python3
"""Package PRISM payloads with palette+indices encoding (XOR delta + simple RLE)."""
from __future__ import annotations

import argparse
import json
import time
import struct
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

try:
    import zlib
except Exception as exc:  # pragma: no cover
    raise SystemExit("zlib is required for CRC32 computation") from exc

from .parser_testbed.builder import (
    PrismHeaderV10,
    PrismPatternMetaV11,
    build_header_blob,
    parse_header_blob,
)


MAGIC = b"PRSM"
VERSION = 0x0101

FRAME_HEADER_STRUCT = struct.Struct("<BH")  # flags (uint8), payload length (uint16)
FLAG_DELTA = 0x01
FLAG_RLE = 0x02

RLE_MARK = 0x80  # top bit indicates run-length entry
MAX_RLE_LEN = 0x7F


@dataclass
class PrismMeta:
    led_count: int
    frame_count: int
    fps: float
    palette_hex: Sequence[str]
    ramp_space: str


def clamp(value: int, lo: int, hi: int) -> int:
    if value < lo or value > hi:
        raise ValueError(f"Value {value} outside [{lo}, {hi}]")
    return value


def rgb_to_hex(rgb: Tuple[int, int, int]) -> str:
    return f"#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}"


def load_payload(path: Path) -> Tuple[PrismMeta, List[List[List[int]]]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    frames = data.get("data", data).get("frames") or [data.get("data", data).get("rgb")]
    if not frames or frames[0] is None:
        raise ValueError("Input payload missing frame data")
    led_count = len(frames[0])
    for frame in frames:
        if len(frame) != led_count:
            raise ValueError("All frames must have consistent LED counts")
    frame_count = len(frames)
    meta_raw = data.get("meta", {})
    fps = float(meta_raw.get("fps", 24.0))
    palette_hex = meta_raw.get("palette") or []
    ramp_space = meta_raw.get("ramp_space", "hsluv")
    return PrismMeta(led_count, frame_count, fps, palette_hex, ramp_space), frames


def _distance_sq(a: Tuple[int, int, int], b: Tuple[int, int, int]) -> int:
    return (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 + (a[2] - b[2]) ** 2


def _merge_entries(entries: List[Dict[str, object]], i: int, j: int) -> None:
    entry_i = entries[i]
    entry_j = entries[j]
    count_i = entry_i["count"]  # type: ignore[assignment]
    count_j = entry_j["count"]  # type: ignore[assignment]
    colour_i = entry_i["color"]  # type: ignore[assignment]
    colour_j = entry_j["color"]  # type: ignore[assignment]
    total = count_i + count_j  # type: ignore[operator]
    new_colour = tuple(
        int(round((colour_i[idx] * count_i + colour_j[idx] * count_j) / total))  # type: ignore[index]
        for idx in range(3)
    )
    originals = entry_i["originals"] | entry_j["originals"]  # type: ignore[operator]

    # Remove higher index first to keep positions valid
    for index in sorted([i, j], reverse=True):
        entries.pop(index)

    # Check if colour already exists to avoid duplicates
    for entry in entries:
        if entry["color"] == new_colour:  # type: ignore[index]
            entry["count"] += total  # type: ignore[assignment, operator]
            entry["originals"] |= originals  # type: ignore[assignment, operator]
            return

    entries.append(
        {
            "color": new_colour,
            "count": total,
            "originals": originals,
        }
    )


def _quantize_palette(counter: Counter, max_size: int) -> Tuple[List[Tuple[int, int, int]], Dict[Tuple[int, int, int], Tuple[int, int, int]], Dict[str, int]]:
    entries: List[Dict[str, object]] = [
        {
            "color": colour,
            "count": count,
            "originals": {colour},
        }
        for colour, count in counter.items()
    ]

    stats = {
        "palette_colors_before": len(entries),
        "palette_colors_after": len(entries),
    }

    if len(entries) <= max_size:
        palette = [entry["color"] for entry in entries]  # type: ignore[index]
        mapping = {orig: entry["color"] for entry in entries for orig in entry["originals"]}  # type: ignore[assignment]
        return palette, mapping, stats

    while len(entries) > max_size:
        best_i = 0
        best_j = 1
        best_distance = _distance_sq(
            entries[0]["color"], entries[1]["color"]  # type: ignore[index]
        )
        entry_count = len(entries)
        for i in range(entry_count):
            colour_i = entries[i]["color"]  # type: ignore[index]
            for j in range(i + 1, entry_count):
                colour_j = entries[j]["color"]  # type: ignore[index]
                dist = _distance_sq(colour_i, colour_j)
                if dist < best_distance:
                    best_distance = dist
                    best_i = i
                    best_j = j
        _merge_entries(entries, best_i, best_j)

    palette = [entry["color"] for entry in entries]  # type: ignore[index]
    mapping = {
        original: entry["color"]  # type: ignore[index]
        for entry in entries
        for original in entry["originals"]  # type: ignore[index]
    }
    stats["palette_colors_after"] = len(palette)
    return palette, mapping, stats


def build_palette_and_remap(
    frames: Sequence[Sequence[Sequence[int]]],
    max_size: int = 64,
) -> Tuple[List[Tuple[int, int, int]], List[List[Tuple[int, int, int]]], Dict[str, int]]:
    counter: Counter = Counter()
    for frame in frames:
        for pixel in frame:
            counter[(pixel[0], pixel[1], pixel[2])] += 1

    palette, colour_map, stats = _quantize_palette(counter, max_size)

    remapped_frames: List[List[Tuple[int, int, int]]] = []
    for frame in frames:
        remapped_frames.append([colour_map[(pixel[0], pixel[1], pixel[2])] for pixel in frame])

    return palette, remapped_frames, stats


def indices_for_frame(frame: Sequence[Sequence[int]], palette_map: Dict[Tuple[int, int, int], int]) -> List[int]:
    indices: List[int] = []
    for pixel in frame:
        key = (pixel[0], pixel[1], pixel[2])
        if key not in palette_map:
            raise ValueError(f"Colour {key} missing from palette (palette too small)")
        indices.append(palette_map[key])
    return indices


def xor_delta(curr: Sequence[int], prev: Sequence[int]) -> List[int]:
    return [c ^ p for c, p in zip(curr, prev)]


def rle_encode(data: Sequence[int]) -> List[int]:
    encoded: List[int] = []
    i = 0
    n = len(data)
    while i < n:
        run_val = data[i]
        run_len = 1
        while i + run_len < n and data[i + run_len] == run_val and run_len < MAX_RLE_LEN:
            run_len += 1
        if run_len >= 4:
            encoded.extend([RLE_MARK | run_len, run_val])
            i += run_len
        else:
            encoded.append(run_val)
            i += 1
    return encoded


def rle_decode(data: Sequence[int]) -> List[int]:
    decoded: List[int] = []
    iterator = iter(data)
    for value in iterator:
        if value & RLE_MARK:
            run_len = value & MAX_RLE_LEN
            try:
                run_val = next(iterator)
            except StopIteration as exc:  # pragma: no cover - indicates corrupt payload
                raise ValueError("Incomplete RLE sequence") from exc
            decoded.extend([run_val] * run_len)
        else:
            decoded.append(value)
    return decoded


def encode_frame(indices: List[int], prev_indices: Optional[List[int]]) -> Tuple[bytes, bool, bool, List[int]]:
    use_delta = False
    baseline = indices
    if prev_indices is not None:
        delta = xor_delta(indices, prev_indices)
        zero_ratio = delta.count(0) / len(delta)
        if zero_ratio >= 0.4:
            baseline = delta
            use_delta = True

    rle_payload = rle_encode(baseline)
    use_rle = len(rle_payload) < len(baseline)
    payload = rle_payload if use_rle else baseline

    flags = 0
    if use_delta:
        flags |= FLAG_DELTA
    if use_rle:
        flags |= FLAG_RLE

    header = FRAME_HEADER_STRUCT.pack(flags, clamp(len(payload), 0, 0xFFFF))
    return header + bytes(payload), use_delta, use_rle, baseline


def encode_frameset(frames: Sequence[Sequence[Sequence[int]]], palette: Sequence[Tuple[int, int, int]]) -> Tuple[bytes, Dict[str, object]]:
    palette_map = {colour: idx for idx, colour in enumerate(palette)}
    stats: Dict[str, object] = {
        "raw_bytes": len(frames) * len(frames[0]) * 3,
        "frames": [],
    }

    start = time.perf_counter()
    prev_indices: Optional[List[int]] = None
    encoded_frames: List[bytes] = []
    for frame_index, frame in enumerate(frames):
        indices = indices_for_frame(frame, palette_map)
        blob, used_delta, used_rle, baseline = encode_frame(indices, prev_indices)
        encoded_frames.append(blob)
        stats["frames"].append(
            {
                "index": frame_index,
                "bytes": len(blob),
            "delta": used_delta,
            "rle": used_rle,
        }
        )
        prev_indices = indices

    encode_duration = time.perf_counter() - start

    payload = bytearray()
    payload.extend(struct.pack("<H", len(palette)))
    for colour in palette:
        payload.extend(colour)
    total_bytes = 2 + len(palette) * 3
    for blob in encoded_frames:
        payload.extend(blob)
        total_bytes += len(blob)

    stats["payload_bytes"] = total_bytes
    stats["encode_ms"] = encode_duration * 1000.0
    stats["compression_ratio"] = stats["raw_bytes"] / total_bytes if total_bytes else float("inf")

    return bytes(payload), stats


def decode_payload(payload: bytes, led_count: int) -> List[List[Tuple[int, int, int]]]:
    offset = 0
    palette_len = struct.unpack_from("<H", payload, offset)[0]
    offset += 2
    palette = [tuple(payload[offset + i * 3 : offset + (i + 1) * 3]) for i in range(palette_len)]
    offset += palette_len * 3
    frames: List[List[Tuple[int, int, int]]] = []
    prev_indices: Optional[List[int]] = None
    while offset < len(payload):
        flags, length = FRAME_HEADER_STRUCT.unpack_from(payload, offset)
        offset += FRAME_HEADER_STRUCT.size
        segment = list(payload[offset : offset + length])
        offset += length

        if flags & FLAG_RLE:
            segment = rle_decode(segment)

        if flags & FLAG_DELTA:
            if prev_indices is None:
                raise ValueError("Delta frame encountered without baseline")
            indices = [value ^ prev for value, prev in zip(segment, prev_indices)]
        else:
            indices = segment

        if len(indices) != led_count:
            raise ValueError("Decoded length mismatch")
        prev_indices = indices
        frames.append([tuple(palette[idx]) for idx in indices])

    return frames


def build_header(meta: PrismMeta, palette_size: int) -> Tuple[bytes, Dict[str, object]]:
    header = PrismHeaderV10(
        version=VERSION,
        led_count=meta.led_count,
        frame_count=meta.frame_count,
        fps=int(round(meta.fps * 256)),
        color_format=1,  # palette+indices
        compression=0,
    )

    pattern_meta = PrismPatternMetaV11(version=0x01)
    palette_id = f"palette-{palette_size}"
    extra_fields = {
        "ramp_space": meta.ramp_space,
        "palette_id": palette_id,
    }
    blob, manifest = build_header_blob(header, pattern_meta, extra_fields=extra_fields)
    return blob, manifest


def package(meta: PrismMeta, frames: List[List[List[int]]]) -> Tuple[bytes, Dict[str, object]]:
    palette, frames_quantised, quant_stats = build_palette_and_remap(frames)
    payload, stats = encode_frameset(frames_quantised, palette)

    header_blob, header_manifest = build_header(meta, len(palette))
    payload_crc = zlib.crc32(payload) & 0xFFFFFFFF
    file_blob = header_blob + payload + struct.pack("<I", payload_crc)

    decode_start = time.perf_counter()
    decoded_frames = decode_payload(payload, meta.led_count)
    decode_ms = (time.perf_counter() - decode_start) * 1000.0

    original_hash = zlib.crc32(json.dumps(frames_quantised, sort_keys=True).encode("utf-8")) & 0xFFFFFFFF
    decoded_hash = zlib.crc32(json.dumps(decoded_frames, sort_keys=True).encode("utf-8")) & 0xFFFFFFFF
    if original_hash != decoded_hash:
        raise ValueError("Round-trip decode mismatch")

    stats.update(
        {
            "palette": [rgb_to_hex(colour) for colour in palette],
            "palette_size": len(palette),
            "led_count": meta.led_count,
            "frame_count": meta.frame_count,
            "fps": meta.fps,
            "payload_crc32": f"0x{payload_crc:08X}",
            "roundtrip_hash": f"0x{decoded_hash:08X}",
            "decode_ms": decode_ms,
            "file_bytes": len(file_blob),
        }
    )
    stats.update(quant_stats)
    stats["quantized"] = quant_stats["palette_colors_before"] != quant_stats["palette_colors_after"]

    parsed = parse_header_blob(header_blob)
    stats["header_crc32"] = f"0x{parsed.base.crc32:08X}"
    stats["header_manifest"] = parsed.extra_fields

    return file_blob, stats


def write_package(input_path: Path, output_path: Path, report_path: Optional[Path]) -> Dict[str, object]:
    meta, frames = load_payload(input_path)
    blob, stats = package(meta, frames)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(blob)
    if report_path:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(json.dumps(stats, indent=2), encoding="utf-8")
    return stats


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Package PRISM frames with palette+indices encoding")
    parser.add_argument("--input", required=True, help="Input JSON frames file")
    parser.add_argument("--output", required=True, help="Output .prism path")
    parser.add_argument("--report", help="Optional JSON report path")
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> None:
    args = parse_args(argv)
    stats = write_package(Path(args.input), Path(args.output), Path(args.report) if args.report else None)
    print(json.dumps(stats, indent=2))


if __name__ == "__main__":  # pragma: no cover
    main()
