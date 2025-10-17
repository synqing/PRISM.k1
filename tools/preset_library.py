#!/usr/bin/env python3
"""Build the reference preset library and packaged artifacts."""
from __future__ import annotations

import argparse
import csv
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Sequence

from tools import prism_packaging, show_to_prism

try:
    import hsluv  # type: ignore
    _HSLUV_AVAILABLE = True
except Exception:  # pragma: no cover - optional dependency
    hsluv = None  # type: ignore
    _HSLUV_AVAILABLE = False
    _HSLUV_WARNED = False
else:
    _HSLUV_WARNED = True  # skip warning when dependency present


@dataclass
class PresetDefinition:
    preset_id: str
    title: str
    show: str
    palette: Sequence[str]
    led_count: int = 160
    duration: float = 6.0
    fps: float = 24.0
    seed: str | None = None
    params: Dict[str, float] | None = None


PRESET_LIBRARY: List[PresetDefinition] = [
    PresetDefinition("sine-daybreak", "Daybreak Sweep", "sine", ["#ff4500", "#ffd700", "#00bcd4"], params={
        "wave_amplitude": 0.45,
        "wave_frequency": 1.2,
        "wave_speed": 0.35,
        "wave_direction": 1.0,
    }),
    PresetDefinition("sine-backbeat", "Backbeat Pulse", "sine", ["#00ffcc", "#0000ff"], params={
        "wave_amplitude": 0.5,
        "wave_frequency": 0.8,
        "wave_speed": 0.55,
        "wave_direction": -1.0,
        "wave_phase": 1.2,
    }),
    PresetDefinition("sine-glacier", "Glacier Drift", "sine", ["#003366", "#66ccff", "#f0f8ff"], params={
        "wave_amplitude": 0.35,
        "wave_frequency": 1.6,
        "wave_speed": 0.28,
        "wave_direction": 1.0,
        "wave_phase": 0.5,
    }),
    PresetDefinition("sine-midnight", "Midnight Tide", "sine", ["#1a237e", "#311b92", "#4a148c"], params={
        "wave_amplitude": 0.6,
        "wave_frequency": 1.0,
        "wave_speed": 0.48,
        "wave_direction": -1.0,
    }),
    PresetDefinition("sine-marquee", "Marquee Sweep", "sine", ["#ffee58", "#f4511e", "#d32f2f"], params={
        "wave_amplitude": 0.4,
        "wave_frequency": 2.0,
        "wave_speed": 0.75,
        "wave_direction": 1.0,
    }),
    PresetDefinition("noise-cascade", "Cascade Noise", "noise", ["#ffb300", "#ff6f00", "#6d4c41"], seed="0x1a2b3c", params={
        "noise_scale": 1.4,
        "noise_speed": 0.55,
        "noise_octaves": 3,
        "noise_persistence": 0.55,
        "noise_lacunarity": 2.3,
    }),
    PresetDefinition("noise-holo", "Hologram Fade", "noise", ["#40c4ff", "#ff4081", "#ffffff"], seed="0xbead", params={
        "noise_scale": 1.9,
        "noise_speed": 0.65,
        "noise_octaves": 4,
        "noise_persistence": 0.6,
        "noise_lacunarity": 2.0,
    }),
    PresetDefinition("noise-vapor", "Vaporwave Drift", "noise", ["#ff75ff", "#00ffd5", "#120458"], seed="1337", params={
        "noise_scale": 1.1,
        "noise_speed": 0.45,
        "noise_octaves": 3,
        "noise_persistence": 0.5,
        "noise_lacunarity": 1.9,
    }),
    PresetDefinition("noise-rain", "Rain Mist", "noise", ["#1de9b6", "#1dc4e9", "#1d8fe9"], seed="rain", params={
        "noise_scale": 0.9,
        "noise_speed": 0.35,
        "noise_octaves": 2,
        "noise_persistence": 0.45,
        "noise_lacunarity": 2.1,
    }),
    PresetDefinition("noise-storm", "Storm Front", "noise", ["#ffffff", "#90caf9", "#1a237e"], seed="storm", params={
        "noise_scale": 2.2,
        "noise_speed": 0.8,
        "noise_octaves": 5,
        "noise_persistence": 0.65,
        "noise_lacunarity": 2.4,
    }),
    PresetDefinition("flow-aurora", "Aurora Curl", "flow", ["#b2ff59", "#69f0ae", "#00e5ff"], seed="aurora", params={
        "flow_field_scale": 1.3,
        "flow_step_size": 0.18,
        "flow_speed": 0.42,
        "flow_curl": 0.7,
        "flow_octaves": 3,
        "flow_persistence": 0.55,
        "flow_lacunarity": 2.0,
    }),
    PresetDefinition("flow-fall", "Digital Waterfall", "flow", ["#4fc3f7", "#0288d1", "#01579b"], seed="water", params={
        "flow_field_scale": 1.6,
        "flow_step_size": 0.16,
        "flow_speed": 0.5,
        "flow_curl": 0.4,
        "flow_octaves": 2,
        "flow_persistence": 0.6,
        "flow_lacunarity": 2.3,
    }),
    PresetDefinition("flow-trace", "Tracer Arcs", "flow", ["#ff6f61", "#6b5b95", "#88b04b"], seed="trace", params={
        "flow_field_scale": 0.9,
        "flow_step_size": 0.22,
        "flow_speed": 0.35,
        "flow_curl": 1.1,
        "flow_octaves": 3,
        "flow_persistence": 0.5,
        "flow_lacunarity": 1.8,
    }),
    PresetDefinition("flow-horizon", "Rising Horizon", "flow", ["#ffea00", "#ff9100", "#ff1744"], seed="horizon", params={
        "flow_field_scale": 1.8,
        "flow_step_size": 0.2,
        "flow_speed": 0.48,
        "flow_curl": 0.2,
        "flow_octaves": 4,
        "flow_persistence": 0.65,
        "flow_lacunarity": 2.2,
    }),
    PresetDefinition("flow-lanterns", "Lantern Drift", "flow", ["#ff8a80", "#ffcc80", "#ffff8d"], seed="lantern", params={
        "flow_field_scale": 1.2,
        "flow_step_size": 0.14,
        "flow_speed": 0.32,
        "flow_curl": 0.5,
        "flow_octaves": 3,
        "flow_persistence": 0.58,
        "flow_lacunarity": 1.9,
    }),
    PresetDefinition("sine-ripple", "Ripple Chain", "sine", ["#f0f4c3", "#aed581", "#7cb342"], params={
        "wave_amplitude": 0.55,
        "wave_frequency": 2.2,
        "wave_speed": 0.6,
        "wave_direction": 1.0,
    }),
    PresetDefinition("sine-mirror", "Mirror March", "sine", ["#ff80ab", "#ea80fc", "#b388ff"], params={
        "wave_amplitude": 0.33,
        "wave_frequency": 1.1,
        "wave_speed": 0.4,
        "wave_direction": -1.0,
        "wave_phase": 3.14,
    }),
    PresetDefinition("noise-meadow", "Meadow Breeze", "noise", ["#a5d6a7", "#66bb6a", "#2e7d32"], seed="meadow", params={
        "noise_scale": 1.0,
        "noise_speed": 0.38,
        "noise_octaves": 3,
        "noise_persistence": 0.52,
        "noise_lacunarity": 2.05,
    }),
    PresetDefinition("noise-sparks", "Ember Sparks", "noise", ["#ff7043", "#ffa726", "#ffcc80"], seed="ember", params={
        "noise_scale": 1.7,
        "noise_speed": 0.9,
        "noise_octaves": 4,
        "noise_persistence": 0.6,
        "noise_lacunarity": 2.5,
    }),
    PresetDefinition("flow-orbit", "Orbit Lines", "flow", ["#00c853", "#64dd17", "#aeea00"], seed="orbit", params={
        "flow_field_scale": 1.1,
        "flow_step_size": 0.19,
        "flow_speed": 0.44,
        "flow_curl": 0.9,
        "flow_octaves": 2,
        "flow_persistence": 0.57,
        "flow_lacunarity": 1.95,
    }),
    PresetDefinition("flow-lattice", "Lattice Spin", "flow", ["#ff4081", "#e040fb", "#7c4dff"], seed="lattice", params={
        "flow_field_scale": 2.0,
        "flow_step_size": 0.17,
        "flow_speed": 0.5,
        "flow_curl": 0.6,
        "flow_octaves": 3,
        "flow_persistence": 0.6,
        "flow_lacunarity": 2.4,
    }),
]


def run_show_cli(args: List[str]) -> None:
    show_to_prism.main(args)


def build_preset(defn: PresetDefinition, out_dir: Path, package: bool) -> Dict[str, object]:
    preset_dir = out_dir / defn.preset_id
    preset_dir.mkdir(parents=True, exist_ok=True)
    json_path = preset_dir / f"{defn.preset_id}.json"
    csv_path = preset_dir / f"{defn.preset_id}.csv"

    global _HSLUV_WARNED

    arg_list = [
        "--show", defn.show,
        "--palette", ",".join(defn.palette),
        "--led-count", str(defn.led_count),
        "--duration", str(defn.duration),
        "--fps", str(defn.fps),
        "--output", str(json_path),
        "--csv", str(csv_path),
    ]
    if _HSLUV_AVAILABLE:
        arg_list.extend(["--ramp-space", "hsluv"])
    else:
        arg_list.extend(["--ramp-space", "hsv"])
        if not _HSLUV_WARNED:
            print("⚠️  hsluv not available; falling back to HSV interpolation for preset generation.", flush=True)
            _HSLUV_WARNED = True
    if defn.seed:
        arg_list.extend(["--seed", defn.seed])
    if defn.params:
        for key, value in defn.params.items():
            arg_list.extend([f"--{key.replace('_', '-')}", str(value)])

    run_show_cli(arg_list)

    payload = json.loads(json_path.read_text(encoding="utf-8"))
    meta = payload.get("meta", {})

    prism_path = None
    report_path = None
    if package:
        prism_path = preset_dir / f"{defn.preset_id}.prism"
        report_path = preset_dir / f"{defn.preset_id}_report.json"
        prism_packaging.write_package(json_path, prism_path, report_path)

    return {
        "preset_id": defn.preset_id,
        "title": defn.title,
        "show": defn.show,
        "json": str(json_path.relative_to(out_dir)),
        "csv": str(csv_path.relative_to(out_dir)),
        "prism": str(prism_path.relative_to(out_dir)) if prism_path else None,
        "report": str(report_path.relative_to(out_dir)) if report_path else None,
        "led_count": meta.get("led_count", defn.led_count),
        "frame_count": meta.get("frame_count"),
        "fps": meta.get("fps", defn.fps),
        "duration": meta.get("duration", defn.duration),
        "palette": defn.palette,
        "seed": defn.seed,
        "params": defn.params or {},
    }


def write_manifest(manifest: Iterable[Dict[str, object]], out_dir: Path) -> None:
    manifest = list(manifest)
    json_path = out_dir / "manifest.json"
    json_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    csv_path = out_dir / "manifest.csv"
    with csv_path.open("w", encoding="utf-8", newline="") as fh:
        writer = csv.writer(fh)
        writer.writerow([
            "preset_id", "title", "show", "json", "csv", "prism", "report",
            "led_count", "frame_count", "fps", "duration", "seed", "params",
        ])
        for entry in manifest:
            writer.writerow([
                entry["preset_id"],
                entry["title"],
                entry["show"],
                entry["json"],
                entry["csv"],
                entry["prism"] or "",
                entry["report"] or "",
                entry["led_count"],
                entry.get("frame_count", ""),
                entry["fps"],
                entry["duration"],
                entry.get("seed", "") or "",
                json.dumps(entry["params"]),
            ])


def build_library(output_dir: Path, *, package: bool) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    entries = []
    for definition in PRESET_LIBRARY:
        entries.append(build_preset(definition, output_dir, package))
    write_manifest(entries, output_dir)


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate the reference preset library")
    parser.add_argument("--output-dir", required=True, help="Directory to place generated presets")
    parser.add_argument("--skip-packaging", action="store_true", help="Do not emit .prism files")
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> None:
    args = parse_args(argv)
    build_library(Path(args.output_dir), package=not args.skip_packaging)


if __name__ == "__main__":  # pragma: no cover
    main()
