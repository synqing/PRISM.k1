"""Summarize soak-test telemetry CSV logs and enforce thresholds."""
from __future__ import annotations

import argparse
import csv
import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, List, Sequence


@dataclass
class DeviceMetrics:
    samples: int = 0
    heap_fragmentation_pct: List[float] = field(default_factory=list)
    frame_ms: List[float] = field(default_factory=list)
    temp_c: List[float] = field(default_factory=list)
    errors: int = 0

    def add(self, row: Dict[str, str], fps: float) -> None:
        try:
            heap_free = float(row["heap_free"])
            heap_min = float(row["heap_min"])
            frame_ms = float(row["frame_ms"])
            temp_c = float(row["temp_c"])
            errors = int(row.get("errors", 0))
        except (ValueError, KeyError) as exc:  # pragma: no cover - validation is simple
            raise ValueError(f"Invalid row values: {row}") from exc

        if heap_free <= 0:
            raise ValueError("heap_free must be positive")
        frag_pct = max(0.0, (heap_free - heap_min) / heap_free * 100.0)

        self.samples += 1
        self.heap_fragmentation_pct.append(frag_pct)
        self.frame_ms.append(frame_ms)
        self.temp_c.append(temp_c)
        self.errors += errors

    def max_fragmentation(self) -> float:
        return max(self.heap_fragmentation_pct or [0.0])

    def frame_p99(self) -> float:
        if not self.frame_ms:
            return 0.0
        sorted_frames = sorted(self.frame_ms)
        index = min(len(sorted_frames) - 1, max(0, int(round(0.99 * (len(sorted_frames) - 1)))))
        return sorted_frames[index]

    def max_temp(self) -> float:
        return max(self.temp_c or [0.0])


def parse_csv(path: Path, fps: float) -> Dict[str, DeviceMetrics]:
    metrics: Dict[str, DeviceMetrics] = {}
    with path.open("r", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        required = {"timestamp", "device", "heap_free", "heap_min", "frame_ms", "temp_c"}
        missing = required - set(reader.fieldnames or [])
        if missing:
            raise ValueError(f"Missing columns: {', '.join(sorted(missing))}")
        for row in reader:
            device = row["device"]
            metrics.setdefault(device, DeviceMetrics()).add(row, fps)
    return metrics


def summarize(metrics: Dict[str, DeviceMetrics], fps: float) -> Dict[str, Dict[str, float]]:
    target_ms = 1000.0 / fps if fps > 0 else 0.0
    summary: Dict[str, Dict[str, float]] = {}
    for device, metric in metrics.items():
        summary[device] = {
            "samples": metric.samples,
            "max_fragmentation_pct": metric.max_fragmentation(),
            "frame_p99_ms": metric.frame_p99(),
            "frame_budget_ms": target_ms,
            "temp_max_c": metric.max_temp(),
            "errors": metric.errors,
        }
    return summary


def enforce_thresholds(summary: Dict[str, Dict[str, float]], *, frag_limit: float, frame_margin: float, temp_limit: float) -> None:
    violations = []
    for device, metrics in summary.items():
        if metrics["max_fragmentation_pct"] > frag_limit:
            violations.append(f"{device}: fragmentation {metrics['max_fragmentation_pct']:.2f}% > {frag_limit}%")
        if metrics["frame_p99_ms"] > metrics["frame_budget_ms"] * frame_margin:
            violations.append(
                f"{device}: frame p99 {metrics['frame_p99_ms']:.2f}ms exceeds budget {metrics['frame_budget_ms'] * frame_margin:.2f}ms"
            )
        if metrics["temp_max_c"] > temp_limit:
            violations.append(f"{device}: temperature {metrics['temp_max_c']:.1f}°C > {temp_limit}°C")
        if metrics["errors"] > 0:
            violations.append(f"{device}: reported {metrics['errors']} errors")
    if violations:
        raise SystemExit("\n".join(violations))


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Summarize soak test telemetry")
    parser.add_argument("--input", required=True, help="Telemetry CSV produced during soak test")
    parser.add_argument("--summary", help="Optional path to write JSON summary")
    parser.add_argument("--fps", type=float, default=24.0, help="Target frames per second")
    parser.add_argument("--fragmentation-limit", type=float, default=5.0, help="Maximum allowed heap fragmentation percentage")
    parser.add_argument("--frame-margin", type=float, default=1.1, help="Frame time margin multiplier vs budget")
    parser.add_argument("--temperature-limit", type=float, default=65.0, help="Maximum expected device temperature")
    parser.add_argument("--no-enforce", action="store_true", help="Skip enforcing thresholds (report only)")
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> None:
    args = parse_args(argv)
    path = Path(args.input)
    metrics = parse_csv(path, args.fps)
    summary = summarize(metrics, args.fps)
    if args.summary:
        Path(args.summary).write_text(json.dumps(summary, indent=2), encoding="utf-8")
    for device, metrics in summary.items():
        print(f"[{device}] samples={metrics['samples']} frag={metrics['max_fragmentation_pct']:.2f}% frame_p99={metrics['frame_p99_ms']:.2f}ms temp={metrics['temp_max_c']:.1f}°C errors={metrics['errors']}")
    if not args.no_enforce:
        enforce_thresholds(summary, frag_limit=args.fragmentation_limit, frame_margin=args.frame_margin, temp_limit=args.temperature_limit)


if __name__ == "__main__":  # pragma: no cover
    main()
