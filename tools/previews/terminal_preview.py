"""
Terminal preview rendering for palettes and shows.

Provides ANSI rendering utilities with perceptual tweaks aligned with R3.5
preview guidelines (gamma=2.2, brightness=0.85, saturation=0.9). Frames are
down-sampled to fit the terminal width and rendered using 24-bit ANSI escape
codes (via `colr` when available, otherwise a manual fallback).
"""
from __future__ import annotations

import argparse
import json
import math
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Sequence, Tuple

import colorsys

try:  # pragma: no cover - dependency injected at runtime
    from colr import color as colr_color
except Exception:  # pragma: no cover - fallback when colr missing
    colr_color = None

DEFAULT_GAMMA = 2.2
DEFAULT_BRIGHTNESS = 0.85
DEFAULT_SATURATION = 0.9
DEFAULT_FPS = 24.0

Frame = Sequence[Sequence[int]]


@dataclass
class PreviewConfig:
    gamma: float = DEFAULT_GAMMA
    brightness: float = DEFAULT_BRIGHTNESS
    saturation: float = DEFAULT_SATURATION
    width: int = 80
    block: str = "█"


def clamp(value: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return max(lo, min(hi, value))


def apply_preview_mapping(
    rgb: Sequence[int],
    *,
    gamma: float,
    brightness: float,
    saturation: float,
) -> Tuple[int, int, int]:
    """Apply gamma, brightness, and saturation adjustments to an RGB triple."""
    r, g, b = (clamp(channel / 255.0) for channel in rgb)
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    s = clamp(s * saturation)
    v = clamp(v * brightness)
    r_adj, g_adj, b_adj = colorsys.hsv_to_rgb(h, s, v)

    def correct(channel: float) -> int:
        if channel <= 0.0:
            return 0
        if channel >= 1.0:
            return 255
        corrected = math.pow(channel, 1.0 / gamma)
        return int(round(clamp(corrected) * 255))

    return (correct(r_adj), correct(g_adj), correct(b_adj))


def average_colors(colors: Sequence[Sequence[int]]) -> Tuple[int, int, int]:
    if not colors:
        return (0, 0, 0)
    total_r = sum(color[0] for color in colors)
    total_g = sum(color[1] for color in colors)
    total_b = sum(color[2] for color in colors)
    count = len(colors)
    return (
        int(round(total_r / count)),
        int(round(total_g / count)),
        int(round(total_b / count)),
    )


def downsample_frame(frame: Frame, width: int) -> List[Tuple[int, int, int]]:
    """Reduce a frame to the requested width by averaging LED groups."""
    if width <= 0:
        raise ValueError("width must be positive")
    if not frame:
        return []
    led_count = len(frame)
    if width >= led_count:
        return [tuple(pixel[:3]) for pixel in frame]

    step = led_count / width
    downsampled: List[Tuple[int, int, int]] = []
    for i in range(width):
        start = int(math.floor(i * step))
        end = int(math.floor((i + 1) * step))
        if end <= start:
            end = start + 1
        end = min(end, led_count)
        segment = frame[start:end]
        downsampled.append(average_colors(segment))
    return downsampled


def rgb_to_ansi(char: str, rgb: Tuple[int, int, int]) -> str:
    r, g, b = rgb
    if colr_color is not None:  # pragma: no cover - depends on external lib
        return colr_color(char, fore=(r, g, b))
    return f"\033[38;2;{r};{g};{b}m{char}\033[0m"


def render_frame(
    frame: Frame,
    *,
    config: PreviewConfig,
) -> str:
    """Render a single frame as an ANSI line."""
    downsampled = downsample_frame(frame, config.width)
    mapped = [apply_preview_mapping(pixel, gamma=config.gamma, brightness=config.brightness, saturation=config.saturation) for pixel in downsampled]
    rendered = "".join(rgb_to_ansi(config.block, rgb) for rgb in mapped)
    if colr_color is not None:
        return rendered + "\033[0m"
    return rendered


def render_terminal(
    frames: Sequence[Frame],
    *,
    fps: float,
    loop: bool,
    static: bool,
    config: PreviewConfig,
    output=None,
    sleep_fn=None,
) -> None:
    """Render frames to the terminal, optionally animating at the requested FPS."""
    if output is None:
        output = sys.stdout
    if sleep_fn is None:
        sleep_fn = time.sleep

    if not frames:
        output.write("No frames to render.\n")
        output.flush()
        return

    interval = 1.0 / fps if fps > 0 else 0.0
    rendered_frames = [render_frame(frame, config=config) for frame in frames]

    def display(frame_str: str) -> None:
        output.write("\033[2J\033[H")
        output.write(frame_str)
        output.write("\033[0m\n")
        output.flush()

    display(rendered_frames[0])
    if static:
        return

    try:
        while True:
            for frame_str in rendered_frames:
                display(frame_str)
                if interval > 0:
                    sleep_fn(interval)
            if not loop:
                break
    except KeyboardInterrupt:  # pragma: no cover - manual interrupt
        output.write("\nPreview interrupted.\n")
        output.flush()


def load_frames(path: Path) -> Tuple[List[Frame], dict]:
    with path.open("r", encoding="utf-8") as fh:
        payload = json.load(fh)

    meta = payload.get("meta", {})
    data = payload.get("data", payload)

    if "frames" in data:
        frames = data["frames"]
    elif "rgb" in data:
        frames = [data["rgb"]]
    else:
        raise ValueError(f"Input {path} missing 'frames' or 'rgb' keys")

    # Ensure tuple structure for rendering
    normalized_frames: List[Frame] = []
    for frame in frames:
        normalized_frames.append([tuple(pixel[:3]) for pixel in frame])
    return normalized_frames, meta


def build_cli_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Render PRISM payloads in the terminal.")
    parser.add_argument("--input", required=True, help="Path to palette/show JSON payload")
    parser.add_argument("--fps", type=float, help="Playback FPS (defaults to metadata fps or 24)")
    parser.add_argument("--loop", action="store_true", help="Loop playback until interrupted")
    parser.add_argument("--static", action="store_true", help="Render only the first frame")
    parser.add_argument("--width", type=int, default=80, help="Terminal character width (default 80)")
    parser.add_argument("--block", default="█", help="Block character to use for rendering")
    parser.add_argument("--gamma", type=float, default=DEFAULT_GAMMA, help="Gamma correction (default 2.2)")
    parser.add_argument("--brightness", type=float, default=DEFAULT_BRIGHTNESS, help="Brightness multiplier (default 0.85)")
    parser.add_argument("--saturation", type=float, default=DEFAULT_SATURATION, help="Saturation multiplier (default 0.9)")
    parser.add_argument("--respect-tty", action="store_true", help="Automatically force static mode when stdout is not a TTY")
    return parser


def main(argv: Optional[Sequence[str]] = None) -> None:
    parser = build_cli_parser()
    args = parser.parse_args(argv)

    input_path = Path(args.input)
    frames, meta = load_frames(input_path)
    fps = args.fps or meta.get("fps") or DEFAULT_FPS

    config = PreviewConfig(
        gamma=args.gamma,
        brightness=args.brightness,
        saturation=args.saturation,
        width=args.width,
        block=args.block,
    )

    static = args.static
    if args.respect_tty and not sys.stdout.isatty():
        static = True

    render_terminal(
        frames,
        fps=fps,
        loop=args.loop,
        static=static,
        config=config,
    )


if __name__ == "__main__":  # pragma: no cover
    main()
