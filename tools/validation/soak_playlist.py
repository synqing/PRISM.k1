#!/usr/bin/env python3
"""Generate soak-test playlist schedules from preset manifests."""
from __future__ import annotations

import argparse
import csv
import json
from datetime import datetime, timedelta
from pathlib import Path
from random import Random
from typing import Iterable, List, Sequence, Dict, Any


def load_manifest(path: Path) -> List[Dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, list):
        raise ValueError("Manifest JSON must be a list of preset entries")
    return data  # type: ignore[return-value]


def normalize_devices(devices: Sequence[str] | None) -> List[str]:
    if not devices:
        return ["dev1", "dev2", "dev3"]
    result = []
    for device in devices:
        if not device:
            continue
        result.append(device.strip())
    return result or ["dev1"]


def build_playlist(
    manifest: Sequence[Dict[str, Any]],
    *,
    dwell_seconds: int,
    total_duration_seconds: int,
    shuffle: bool,
    seed: int | None,
) -> List[Dict[str, Any]]:
    if dwell_seconds <= 0:
        raise ValueError("dwell_seconds must be positive")
    if not manifest:
        raise ValueError("Manifest is empty")

    rng = Random(seed)
    entries = list(manifest)
    playlist: List[Dict[str, Any]] = []
    elapsed = 0
    cycle = 0

    while elapsed < total_duration_seconds:
        items = entries[:]
        if shuffle:
            rng.shuffle(items)
        for item in items:
            playlist.append(item)
            elapsed += dwell_seconds
            if elapsed >= total_duration_seconds:
                break
        cycle += 1
    return playlist


def write_schedule(
    *,
    schedule: Sequence[Dict[str, Any]],
    root: Path,
    devices: Sequence[str],
    dwell_seconds: int,
    output_csv: Path,
    playlist_duration: int,
    start_time: datetime,
    stagger_seconds: int,
) -> None:
    output_csv.parent.mkdir(parents=True, exist_ok=True)
    headers = [
        "step",
        "device",
        "preset_id",
        "title",
        "prism_path",
        "dwell_seconds",
        "planned_start",
        "planned_end",
    ]
    with output_csv.open("w", encoding="utf-8", newline="") as fh:
        writer = csv.writer(fh)
        writer.writerow(headers)
        for device_index, device in enumerate(devices):
            offset = timedelta(seconds=stagger_seconds * device_index)
            current_start = start_time + offset
            for step_index, entry in enumerate(schedule, start=1):
                prism_rel = entry.get("prism")
                prism_path = (root / prism_rel).resolve() if prism_rel else None
                planned_end = current_start + timedelta(seconds=dwell_seconds)
                writer.writerow(
                    [
                        step_index,
                        device,
                        entry.get("preset_id"),
                        entry.get("title"),
                        str(prism_path) if prism_path else "",
                        dwell_seconds,
                        current_start.isoformat(),
                        planned_end.isoformat(),
                    ]
                )
                current_start = planned_end


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate soak-test playlist schedule")
    parser.add_argument("--manifest", required=True, help="Path to preset manifest.json")
    parser.add_argument("--output", default="logs/soak_playlist.csv", help="Output CSV path")
    parser.add_argument("--dwell-seconds", type=int, default=300, help="Time to play each preset (seconds)")
    parser.add_argument("--hours", type=float, default=24.0, help="Total playlist duration (hours)")
    parser.add_argument("--devices", help="Comma-separated device IDs (default dev1,dev2,dev3)")
    parser.add_argument("--shuffle", action="store_true", help="Shuffle presets each cycle")
    parser.add_argument("--seed", type=int, help="Optional RNG seed when shuffling")
    parser.add_argument("--start", help="ISO8601 start timestamp (defaults to now)")
    parser.add_argument("--stagger-seconds", type=int, default=0, help="Start-time offset per device (seconds)")
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> None:
    args = parse_args(argv)
    manifest_path = Path(args.manifest)
    manifest = load_manifest(manifest_path)
    devices = normalize_devices(args.devices.split(",") if args.devices else None)
    total_seconds = int(args.hours * 3600)
    schedule = build_playlist(
        manifest,
        dwell_seconds=args.dwell_seconds,
        total_duration_seconds=total_seconds,
        shuffle=args.shuffle,
        seed=args.seed,
    )
    start_time = datetime.fromisoformat(args.start) if args.start else datetime.utcnow()
    output_path = Path(args.output)
    write_schedule(
        schedule=schedule,
        root=manifest_path.parent,
        devices=devices,
        dwell_seconds=args.dwell_seconds,
        output_csv=output_path,
        playlist_duration=total_seconds,
        start_time=start_time,
        stagger_seconds=args.stagger_seconds,
    )
    print(f"Wrote soak playlist with {len(schedule)} steps for {len(devices)} device(s) to {output_path}")


if __name__ == "__main__":  # pragma: no cover
    main()
