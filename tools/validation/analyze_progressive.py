#!/usr/bin/env python3
"""
Analyze PROGRESSIVE temporal sequencing from a high-speed video.

Requires: OpenCV (cv2), NumPy

Example:
  python3 tools/validation/analyze_progressive.py --video clip.mp4 --leds 160 --fps 240
"""

import argparse
import json
import os
from typing import List

try:
    import cv2  # type: ignore
    import numpy as np  # type: ignore
except Exception as e:
    cv2 = None
    np = None


def extract_intensity_trace(video_path: str, roi_rows: int = 10) -> np.ndarray:
    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        raise RuntimeError(f"Failed to open video: {video_path}")

    intensities: List[np.ndarray] = []

    while True:
        ret, frame = cap.read()
        if not ret:
            break
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        h, w = gray.shape
        row_start = max(0, h // 2 - roi_rows // 2)
        row_end = min(h, row_start + roi_rows)
        strip = gray[row_start:row_end, :]
        # Average across ROI rows to a 1D trace per frame
        intensities.append(strip.mean(axis=0))

    cap.release()
    return np.stack(intensities, axis=0)  # [frames, width]


def estimate_edge_timings(traces: np.ndarray, leds: int) -> List[float]:
    frames, width = traces.shape
    # Downsample spatial axis from width to leds bins
    bins = np.array_split(np.arange(width), leds)
    led_series = np.stack([traces[:, b].mean(axis=1) for b in bins], axis=1)  # [frames, leds]

    # Normalize per-LED
    led_series = (led_series - led_series.min(axis=0)) / (led_series.ptp(axis=0) + 1e-6)

    # For each LED, detect time of 50% rise (simple threshold crossing)
    timings = []
    for i in range(leds):
        s = led_series[:, i]
        idx = np.argmax(s >= 0.5)
        timings.append(float(idx))

    return timings


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--video', required=True, help='Path to high-speed video file')
    p.add_argument('--leds', type=int, default=160, help='Number of LEDs across strip')
    p.add_argument('--fps', type=float, default=240.0, help='Video FPS')
    p.add_argument('--out', default=None, help='Path to save JSON report')
    args = p.parse_args()

    if cv2 is None or np is None:
        raise SystemExit("OpenCV and NumPy are required. Please install: pip install opencv-python numpy")

    traces = extract_intensity_trace(args.video)
    led_timings_frames = estimate_edge_timings(traces, args.leds)
    led_timings_ms = [t * (1000.0 / args.fps) for t in led_timings_frames]

    report = {
        'video': os.path.abspath(args.video),
        'leds': args.leds,
        'fps': args.fps,
        'timings_ms': led_timings_ms,
        'gradient_ms_per_led': (max(led_timings_ms) - min(led_timings_ms)) / max(1, args.leds - 1),
    }

    out_path = args.out or 'progressive_timing_report.json'
    with open(out_path, 'w') as f:
        json.dump(report, f, indent=2)

    print(f"Saved report to {out_path}")


if __name__ == '__main__':
    main()

