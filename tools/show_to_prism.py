#!/usr/bin/env python3
"""
Offline show generator emitting PRISM-compatible frame payloads.

Supports sine, noise, and flow families described in R3.3 with deterministic
seed control and shared metadata output.
"""
from __future__ import annotations

import argparse
import colorsys
import json
import math
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple, Any

try:
    import hsluv  # type: ignore
except Exception:  # pragma: no cover
    hsluv = None

try:
    from rgbw_colorspace_converter.colors.converters import Hex  # pragma: no cover
except Exception:
    Hex = None
from .tooling_core import (
    build_parser as tooling_build_parser,
    load_metadata,
    log_info,
    write_csv,
    write_json,
)

DEFAULT_DURATION = 10.0
DEFAULT_FPS = 24.0
DEFAULT_SPACE = "hsluv"
PALETTE_LOOKUP_STEPS = 1024
MAX_FPS = 120
MAX_LED_COUNT = 4096

MOTION_DIRECTIONS: Dict[str, int] = {
    "LEFT_ORIGIN": 0,
    "RIGHT_ORIGIN": 1,
    "CENTER_ORIGIN": 2,
    "EDGE_ORIGIN": 3,
    "STATIC": 4,
}

SYNC_MODES: Dict[str, int] = {
    "SYNC": 0,
    "OFFSET": 1,
    "PROGRESSIVE": 2,
    "WAVE": 3,
    "CUSTOM": 4,
}


def clamp(value: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return max(lo, min(hi, value))


def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def fade(t: float) -> float:
    # Perlin fade curve for smoother interpolation.
    return t * t * t * (t * (t * 6 - 15) + 10)


def parse_hex_list(value: str) -> List[str]:
    items = [item.strip() for item in value.split(",") if item.strip()]
    if not items:
        raise argparse.ArgumentTypeError("Palette must contain at least one color.")
    for item in items:
        if not item.startswith("#") or len(item) not in (4, 7):
            raise argparse.ArgumentTypeError(f"Invalid hex color: {item}")
    return items


def hex_to_rgb_local(hex_str: str) -> Tuple[int, int, int]:
    value = hex_str.lstrip("#")
    if len(value) == 3:
        value = "".join(ch * 2 for ch in value)
    if len(value) != 6:
        raise ValueError(f"Invalid hex color: {hex_str}")
    r = int(value[0:2], 16)
    g = int(value[2:4], 16)
    b = int(value[4:6], 16)
    return r, g, b


def hex_to_hsv_local(hex_str: str) -> Tuple[float, float, float]:
    if Hex is not None:  # pragma: no cover - prefer library path when available
        return Hex(hex_str).hsv
    r, g, b = hex_to_rgb_local(hex_str)
    return colorsys.rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)


def hex_to_hsluv_tuple(hex_str: str) -> Tuple[float, float, float]:
    if hsluv is None:
        raise RuntimeError("hsluv library is required for ramp_space 'hsluv'.")
    r, g, b = hex_to_rgb_local(hex_str)
    return hsluv.rgb_to_hsluv([r / 255.0, g / 255.0, b / 255.0])


def hsv_to_rgb_local(hsv: Tuple[float, float, float]) -> Tuple[int, int, int]:
    r, g, b = colorsys.hsv_to_rgb(*hsv)
    return (
        int(round(clamp(r) * 255)),
        int(round(clamp(g) * 255)),
        int(round(clamp(b) * 255)),
    )


def lerp_hue(a: float, b: float, t: float) -> float:
    delta = (b - a) % 1.0
    if delta > 0.5:
        delta -= 1.0
    return (a + delta * t) % 1.0


def lerp_hue_deg(a: float, b: float, t: float) -> float:
    delta = (b - a) % 360.0
    if delta > 180.0:
        delta -= 360.0
    return (a + delta * t) % 360.0


def _segment_counts(stops: Sequence[Tuple[float, ...]], steps: int) -> List[int]:
    segments = max(1, len(stops) - 1)
    counts = [steps // segments] * segments
    remainder = steps - sum(counts)
    idx = 0
    while remainder > 0:
        counts[idx] += 1
        idx = (idx + 1) % segments
        remainder -= 1
    return counts


def interpolate_hsv_local(stops: Sequence[Tuple[float, float, float]], steps: int) -> List[Tuple[float, float, float]]:
    if len(stops) < 2:
        raise ValueError("Need at least two HSV stops")
    counts = _segment_counts(stops, steps)
    result: List[Tuple[float, float, float]] = []
    for idx, count in enumerate(counts):
        a = stops[idx]
        b = stops[idx + 1]
        if count <= 0:
            continue
        for i in range(count):
            t = i / max(1, count - 1)
            h = lerp_hue(a[0], b[0], t)
            s = lerp(a[1], b[1], t)
            v = lerp(a[2], b[2], t)
            result.append((h, s, v))
    return result[:steps]


def interpolate_hsl_local(stops: Sequence[Tuple[float, float, float]], steps: int) -> List[Tuple[float, float, float]]:
    return interpolate_hsv_local(stops, steps)


def interpolate_hsluv_local(stops: Sequence[Tuple[float, float, float]], steps: int) -> List[Tuple[float, float, float]]:
    if len(stops) < 2:
        raise ValueError("Need at least two HSLuv stops")
    counts = _segment_counts(stops, steps)
    result: List[Tuple[float, float, float]] = []
    for idx, count in enumerate(counts):
        a = stops[idx]
        b = stops[idx + 1]
        if count <= 0:
            continue
        for i in range(count):
            t = i / max(1, count - 1)
            h = lerp_hue_deg(a[0], b[0], t)
            s = lerp(a[1], b[1], t)
            l = lerp(a[2], b[2], t)
            result.append((h, s, l))
    return result[:steps]


def hash_int(x: int, seed: int) -> int:
    n = x * 374761393 + seed * 668265263
    n = (n ^ (n >> 13)) & 0xFFFFFFFF
    n = (n * 1274126177) & 0xFFFFFFFF
    return n


def random_float(x: int, seed: int) -> float:
    return hash_int(x, seed) / 0xFFFFFFFF


def value_noise1d(x: float, seed: int) -> float:
    xi = math.floor(x)
    xf = x - xi
    v0 = random_float(xi, seed)
    v1 = random_float(xi + 1, seed)
    return lerp(v0, v1, fade(xf))


def value_noise2d(x: float, y: float, seed: int) -> float:
    xi = math.floor(x)
    yi = math.floor(y)
    xf = x - xi
    yf = y - yi

    def rand(ix: int, iy: int) -> float:
        n = hash_int(ix, seed)
        n = hash_int(iy ^ n, seed * 1619 + ix * 31337)
        return n / 0xFFFFFFFF

    v00 = rand(xi, yi)
    v10 = rand(xi + 1, yi)
    v01 = rand(xi, yi + 1)
    v11 = rand(xi + 1, yi + 1)
    ix0 = lerp(v00, v10, fade(xf))
    ix1 = lerp(v01, v11, fade(xf))
    return lerp(ix0, ix1, fade(yf))


def fractal_noise_1d(
    x: float,
    *,
    seed: int,
    octaves: int,
    persistence: float,
    lacunarity: float,
) -> float:
    amplitude = 1.0
    frequency = 1.0
    max_value = 0.0
    total = 0.0
    for octave in range(max(1, octaves)):
        total += amplitude * value_noise1d(x * frequency, seed + octave * 101)
        max_value += amplitude
        amplitude *= persistence
        frequency *= lacunarity
    if max_value == 0:
        return 0.0
    return total / max_value


def fractal_noise_2d(
    x: float,
    y: float,
    *,
    seed: int,
    octaves: int,
    persistence: float,
    lacunarity: float,
) -> float:
    amplitude = 1.0
    frequency = 1.0
    max_value = 0.0
    total = 0.0
    for octave in range(max(1, octaves)):
        total += amplitude * value_noise2d(x * frequency, y * frequency, seed + octave * 131)
        max_value += amplitude
        amplitude *= persistence
        frequency *= lacunarity
    if max_value == 0:
        return 0.0
    return total / max_value


class PaletteSampler:
    def __init__(self, palette: Sequence[str], space: str, steps: int = PALETTE_LOOKUP_STEPS):
        self.palette = list(palette)
        self.space = space
        self.steps = max(steps, 2)
        self.lookup = self._build_lookup()

    def _build_lookup(self) -> List[Tuple[int, int, int]]:
        if self.space == "hsluv":
            if hsluv is None:
                raise RuntimeError("hsluv library is required for ramp_space 'hsluv'.")
            stops = [hex_to_hsluv_tuple(hx) for hx in self.palette]
            interpolated = interpolate_hsluv_local(stops, self.steps)
            rgb = []
            for h, s, l in interpolated:
                r, g, b = hsluv.hsluv_to_rgb([h, s, l])
                rgb.append(
                    (
                        int(round(r * 255)),
                        int(round(g * 255)),
                        int(round(b * 255)),
                    )
                )
            return rgb
        else:
            hsv_stops = [hex_to_hsv_local(hx) for hx in self.palette]
            if self.space == "hsv":
                interpolated = interpolate_hsv_local(hsv_stops, self.steps)
            else:
                interpolated = interpolate_hsl_local(hsv_stops, self.steps)
            return [hsv_to_rgb_local(hsv) for hsv in interpolated]

    def sample(self, value: float) -> Tuple[int, int, int]:
        idx = int(clamp(value) * (self.steps - 1))
        return self.lookup[idx]


class ShowBase:
    def __init__(
        self,
        *,
        palette: PaletteSampler,
        led_count: int,
        fps: float,
        duration: float,
    ):
        self.palette = palette
        self.led_count = led_count
        self.fps = fps
        self.duration = duration
        self.frame_count = max(1, int(round(duration * fps)))
        self.led_positions = [i / max(1, led_count - 1) if led_count > 1 else 0.0 for i in range(led_count)]

    def generate_frames(self) -> List[List[Tuple[int, int, int]]]:
        frames: List[List[Tuple[int, int, int]]] = []
        for frame_index in range(self.frame_count):
            t = frame_index / self.fps
            frames.append(self.frame_at(t))
        return frames

    def frame_at(self, t: float) -> List[Tuple[int, int, int]]:
        raise NotImplementedError


class SineWaveShow(ShowBase):
    def __init__(
        self,
        *,
        palette: PaletteSampler,
        led_count: int,
        fps: float,
        duration: float,
        amplitude: float,
        frequency: float,
        speed: float,
        direction: float,
        phase: float,
        seed: Optional[int] = None,
    ):
        super().__init__(palette=palette, led_count=led_count, fps=fps, duration=duration)
        self.amplitude = amplitude
        self.frequency = frequency
        self.speed = speed
        self.direction = direction
        self.phase = phase
        self.seed = seed
        if seed is not None:
            self._phase_offsets = [random_float(idx, seed) * math.tau for idx in range(self.led_count)]
        else:
            self._phase_offsets = [0.0] * self.led_count

    def frame_at(self, t: float) -> List[Tuple[int, int, int]]:
        frame = []
        tau = math.tau
        for idx, pos in enumerate(self.led_positions):
            angle = tau * (self.frequency * pos * self.direction) + self.speed * tau * t + self.phase
            angle += self._phase_offsets[idx]
            value = 0.5 + self.amplitude * math.sin(angle)
            frame.append(self.palette.sample(clamp(value)))
        return frame


class NoiseMorphShow(ShowBase):
    def __init__(
        self,
        *,
        palette: PaletteSampler,
        led_count: int,
        fps: float,
        duration: float,
        scale: float,
        speed: float,
        octaves: int,
        persistence: float,
        lacunarity: float,
        seed: int,
    ):
        super().__init__(palette=palette, led_count=led_count, fps=fps, duration=duration)
        self.scale = scale
        self.speed = speed
        self.octaves = octaves
        self.persistence = persistence
        self.lacunarity = lacunarity
        self.seed = seed

    def frame_at(self, t: float) -> List[Tuple[int, int, int]]:
        frame = []
        for pos in self.led_positions:
            sample_pos = pos * self.scale + self.speed * t
            noise_value = fractal_noise_1d(
                sample_pos,
                seed=self.seed,
                octaves=self.octaves,
                persistence=self.persistence,
                lacunarity=self.lacunarity,
            )
            frame.append(self.palette.sample(clamp(noise_value)))
        return frame


class FlowFieldShow(ShowBase):
    def __init__(
        self,
        *,
        palette: PaletteSampler,
        led_count: int,
        fps: float,
        duration: float,
        field_scale: float,
        step_size: float,
        speed: float,
        curl: float,
        octaves: int,
        persistence: float,
        lacunarity: float,
        seed: int,
    ):
        super().__init__(palette=palette, led_count=led_count, fps=fps, duration=duration)
        self.field_scale = field_scale
        self.step_size = step_size
        self.speed = speed
        self.curl = curl
        self.octaves = octaves
        self.persistence = persistence
        self.lacunarity = lacunarity
        self.seed = seed

    def frame_at(self, t: float) -> List[Tuple[int, int, int]]:
        frame: List[Tuple[int, int, int]] = []
        accumulator = 0.5
        for idx, pos in enumerate(self.led_positions):
            x = pos * self.field_scale
            y = t * self.speed
            noise_val = fractal_noise_2d(
                x,
                y,
                seed=self.seed + idx,
                octaves=self.octaves,
                persistence=self.persistence,
                lacunarity=self.lacunarity,
            )
            angle = noise_val * math.tau + self.curl
            delta = math.sin(angle) * self.step_size
            accumulator = clamp(accumulator + delta)
            frame.append(self.palette.sample(accumulator))
        return frame


def parse_seed(seed_value: Optional[str]) -> Tuple[Optional[int], Optional[str]]:
    if seed_value is None:
        return None, None
    try:
        numeric = int(seed_value, 0)
        return numeric, seed_value
    except ValueError:
        return None, seed_value


def build_palette_sampler(palette: Sequence[str], space: str) -> PaletteSampler:
    return PaletteSampler(palette, space, steps=PALETTE_LOOKUP_STEPS)


def generate_show(
    show_type: str,
    *,
    palette: PaletteSampler,
    led_count: int,
    fps: float,
    duration: float,
    seed_numeric: Optional[int],
    params: Dict[str, float],
) -> Tuple[List[List[Tuple[int, int, int]]], Dict[str, float]]:
    seed = seed_numeric or 0
    if show_type == "sine":
        used_params = {
            "wave_amplitude": params["wave_amplitude"],
            "wave_frequency": params["wave_frequency"],
            "wave_speed": params["wave_speed"],
            "wave_direction": params["wave_direction"],
            "wave_phase": params["wave_phase"],
        }
        generator = SineWaveShow(
            palette=palette,
            led_count=led_count,
            fps=fps,
            duration=duration,
            amplitude=params["wave_amplitude"],
            frequency=params["wave_frequency"],
            speed=params["wave_speed"],
            direction=params["wave_direction"],
            phase=params["wave_phase"],
            seed=seed_numeric,
        )
    elif show_type == "noise":
        used_params = {
            "noise_scale": params["noise_scale"],
            "noise_speed": params["noise_speed"],
            "noise_octaves": params["noise_octaves"],
            "noise_persistence": params["noise_persistence"],
            "noise_lacunarity": params["noise_lacunarity"],
        }
        generator = NoiseMorphShow(
            palette=palette,
            led_count=led_count,
            fps=fps,
            duration=duration,
            scale=params["noise_scale"],
            speed=params["noise_speed"],
            octaves=int(params["noise_octaves"]),
            persistence=params["noise_persistence"],
            lacunarity=params["noise_lacunarity"],
            seed=seed,
        )
    elif show_type == "flow":
        used_params = {
            "flow_field_scale": params["flow_field_scale"],
            "flow_step_size": params["flow_step_size"],
            "flow_speed": params["flow_speed"],
            "flow_curl": params["flow_curl"],
            "flow_octaves": params["flow_octaves"],
            "flow_persistence": params["flow_persistence"],
            "flow_lacunarity": params["flow_lacunarity"],
        }
        generator = FlowFieldShow(
            palette=palette,
            led_count=led_count,
            fps=fps,
            duration=duration,
            field_scale=params["flow_field_scale"],
            step_size=params["flow_step_size"],
            speed=params["flow_speed"],
            curl=params["flow_curl"],
            octaves=int(params["flow_octaves"]),
            persistence=params["flow_persistence"],
            lacunarity=params["flow_lacunarity"],
            seed=seed,
        )
    else:
        raise ValueError(f"Unsupported show type: {show_type}")
    frames = generator.generate_frames()
    return frames, used_params


def build_cli_parser() -> argparse.ArgumentParser:
    parser = tooling_build_parser("Generate PRISM show payloads")
    parser.add_argument("--show", choices=["sine", "noise", "flow"], required=True, help="Show family to generate")
    parser.add_argument("--palette", type=parse_hex_list, required=True, help="Comma-separated HEX colors")
    parser.add_argument("--led-count", type=int, required=True, help="Number of LEDs in the strand")
    parser.add_argument("--duration", type=float, default=DEFAULT_DURATION, help="Duration in seconds (default 10)")
    parser.add_argument("--fps", type=float, default=DEFAULT_FPS, help="Frames per second (default 24)")
    parser.add_argument("--ramp-space", choices=["hsluv", "hsv", "hsl"], default=DEFAULT_SPACE, help="Palette interpolation space (default hsluv)")
    parser.add_argument("--seed", type=str, help="Deterministic seed (supports int or hex literal)")
    parser.add_argument("--software-version", default="prism-tools/0.1.0", help="Version tag stored in metadata")

    temporal_group = parser.add_argument_group("Temporal metadata")
    temporal_group.add_argument(
        "--motion-direction",
        choices=sorted(MOTION_DIRECTIONS.keys()),
        default="LEFT_ORIGIN",
        help="Motion direction metadata tag",
    )
    temporal_group.add_argument(
        "--sync-mode",
        choices=sorted(SYNC_MODES.keys()),
        default="SYNC",
        help="Sync mode metadata tag",
    )
    temporal_group.add_argument("--sync-offset-ms", type=float, help="Fixed edge delay in milliseconds (OFFSET mode)")
    temporal_group.add_argument(
        "--sync-progressive-start-ms",
        type=float,
        help="Starting delay for PROGRESSIVE mode (milliseconds)",
    )
    temporal_group.add_argument(
        "--sync-progressive-end-ms",
        type=float,
        help="Ending delay for PROGRESSIVE mode (milliseconds)",
    )
    temporal_group.add_argument(
        "--sync-wave-amplitude-ms",
        type=float,
        help="Delay amplitude for WAVE mode (milliseconds)",
    )
    temporal_group.add_argument(
        "--sync-wave-frequency-hz",
        type=float,
        help="Delay oscillation frequency for WAVE mode (Hz)",
    )
    temporal_group.add_argument(
        "--sync-wave-phase-deg",
        type=float,
        default=0.0,
        help="Delay phase offset for WAVE mode (degrees)",
    )
    temporal_group.add_argument(
        "--sync-custom-file",
        type=Path,
        help="Path to JSON file describing per-LED custom delay map",
    )

    wave_group = parser.add_argument_group("Sine wave parameters")
    wave_group.add_argument("--wave-amplitude", type=float, default=0.45, help="Amplitude of sine brightness modulation (0-1)")
    wave_group.add_argument("--wave-frequency", type=float, default=1.0, help="Spatial frequency multiplier")
    wave_group.add_argument("--wave-speed", type=float, default=0.5, help="Temporal speed factor")
    wave_group.add_argument("--wave-direction", type=float, default=1.0, help="Direction multiplier (+/-)")
    wave_group.add_argument("--wave-phase", type=float, default=0.0, help="Phase offset in radians")

    noise_group = parser.add_argument_group("Noise morph parameters")
    noise_group.add_argument("--noise-scale", type=float, default=1.5, help="Spatial scale of noise field")
    noise_group.add_argument("--noise-speed", type=float, default=0.6, help="Temporal speed of noise progression")
    noise_group.add_argument("--noise-octaves", type=int, default=3, help="Fractal octaves for noise")
    noise_group.add_argument("--noise-persistence", type=float, default=0.5, help="Amplitude decay per octave")
    noise_group.add_argument("--noise-lacunarity", type=float, default=2.0, help="Frequency increase per octave")

    flow_group = parser.add_argument_group("Flow field parameters")
    flow_group.add_argument("--flow-field-scale", type=float, default=1.0, help="Spatial scaling for flow noise input")
    flow_group.add_argument("--flow-step-size", type=float, default=0.2, help="Integration step size controlling smoothness")
    flow_group.add_argument("--flow-speed", type=float, default=0.4, help="Temporal evolution speed")
    flow_group.add_argument("--flow-curl", type=float, default=0.6, help="Additional curl applied to vector field (radians)")
    flow_group.add_argument("--flow-octaves", type=int, default=2, help="Number of octaves in flow noise")
    flow_group.add_argument("--flow-persistence", type=float, default=0.6, help="Amplitude decay for flow noise")
    flow_group.add_argument("--flow-lacunarity", type=float, default=2.2, help="Frequency growth for flow noise")

    return parser


def collect_params(args: argparse.Namespace) -> Dict[str, float]:
    return {
        "wave_amplitude": args.wave_amplitude,
        "wave_frequency": args.wave_frequency,
        "wave_speed": args.wave_speed,
        "wave_direction": args.wave_direction,
        "wave_phase": args.wave_phase,
        "noise_scale": args.noise_scale,
        "noise_speed": args.noise_speed,
        "noise_octaves": args.noise_octaves,
        "noise_persistence": args.noise_persistence,
        "noise_lacunarity": args.noise_lacunarity,
        "flow_field_scale": args.flow_field_scale,
        "flow_step_size": args.flow_step_size,
        "flow_speed": args.flow_speed,
        "flow_curl": args.flow_curl,
        "flow_octaves": args.flow_octaves,
        "flow_persistence": args.flow_persistence,
        "flow_lacunarity": args.flow_lacunarity,
    }


def load_custom_delay(path: Path) -> Any:
    try:
        raw = path.read_text(encoding="utf-8")
    except OSError as exc:  # pragma: no cover - simple file IO
        raise ValueError(f"Failed to read custom delay file '{path}': {exc}") from exc
    try:
        data = json.loads(raw)
    except json.JSONDecodeError as exc:
        raise ValueError(f"Invalid JSON in custom delay file '{path}': {exc}") from exc
    return data


def build_motion_meta(direction: str) -> Dict[str, object]:
    return {
        "direction": direction,
        "code": MOTION_DIRECTIONS[direction],
    }


def build_sync_meta(args: argparse.Namespace) -> Dict[str, object]:
    mode = args.sync_mode
    meta: Dict[str, object] = {
        "mode": mode,
        "code": SYNC_MODES[mode],
    }
    if mode == "OFFSET":
        meta["offset_ms"] = args.sync_offset_ms
    elif mode == "PROGRESSIVE":
        meta["start_ms"] = args.sync_progressive_start_ms
        meta["end_ms"] = args.sync_progressive_end_ms
    elif mode == "WAVE":
        meta["amplitude_ms"] = args.sync_wave_amplitude_ms
        meta["frequency_hz"] = args.sync_wave_frequency_hz
        meta["phase_deg"] = args.sync_wave_phase_deg
    elif mode == "CUSTOM":
        meta["custom_file"] = str(args.sync_custom_file)
        meta["custom_delay"] = load_custom_delay(args.sync_custom_file)
    return meta


def validate_args(parser: argparse.ArgumentParser, args: argparse.Namespace) -> None:
    def err(message: str) -> None:
        parser.error(message)

    if args.led_count < 1 or args.led_count > MAX_LED_COUNT:
        err(f"--led-count must be between 1 and {MAX_LED_COUNT}")
    if args.duration <= 0:
        err("--duration must be greater than 0")
    if args.fps <= 0 or args.fps > MAX_FPS:
        err(f"--fps must be between 1 and {MAX_FPS}")

    if args.show == "sine":
        if not 0.0 <= args.wave_amplitude <= 1.0:
            err("--wave-amplitude must be between 0.0 and 1.0")
        if args.wave_frequency < 0:
            err("--wave-frequency must be non-negative")
        if not -10.0 <= args.wave_direction <= 10.0:
            err("--wave-direction must be between -10 and 10")
    elif args.show == "noise":
        if args.noise_scale <= 0:
            err("--noise-scale must be positive")
        if args.noise_speed < 0:
            err("--noise-speed must be non-negative")
        if args.noise_octaves < 1 or args.noise_octaves > 8:
            err("--noise-octaves must be between 1 and 8")
        if not 0.0 <= args.noise_persistence <= 1.0:
            err("--noise-persistence must be between 0.0 and 1.0")
        if args.noise_lacunarity <= 0:
            err("--noise-lacunarity must be positive")
    elif args.show == "flow":
        if args.flow_field_scale <= 0:
            err("--flow-field-scale must be positive")
        if not 0 < args.flow_step_size <= 1.0:
            err("--flow-step-size must be within (0, 1]")
        if args.flow_speed < 0:
            err("--flow-speed must be non-negative")
        if args.flow_octaves < 1 or args.flow_octaves > 8:
            err("--flow-octaves must be between 1 and 8")
        if not 0.0 <= args.flow_persistence <= 1.0:
            err("--flow-persistence must be between 0.0 and 1.0")
        if args.flow_lacunarity <= 0:
            err("--flow-lacunarity must be positive")

    if args.seed is not None and args.seed.strip() == "":
        err("--seed cannot be empty when provided")

    sync_mode = args.sync_mode
    if sync_mode == "OFFSET":
        if args.sync_offset_ms is None:
            err("--sync-offset-ms is required when --sync-mode OFFSET")
        if args.sync_offset_ms < 0:
            err("--sync-offset-ms must be non-negative")
    elif sync_mode == "PROGRESSIVE":
        if args.sync_progressive_start_ms is None or args.sync_progressive_end_ms is None:
            err("--sync-progressive-start-ms and --sync-progressive-end-ms are required when --sync-mode PROGRESSIVE")
        if args.sync_progressive_start_ms < 0 or args.sync_progressive_end_ms < 0:
            err("Progressive delays must be non-negative")
        if args.sync_progressive_end_ms < args.sync_progressive_start_ms:
            err("--sync-progressive-end-ms must be greater than or equal to --sync-progressive-start-ms")
    elif sync_mode == "WAVE":
        if args.sync_wave_amplitude_ms is None or args.sync_wave_frequency_hz is None:
            err("--sync-wave-amplitude-ms and --sync-wave-frequency-hz are required when --sync-mode WAVE")
        if args.sync_wave_amplitude_ms < 0:
            err("--sync-wave-amplitude-ms must be non-negative")
        if args.sync_wave_frequency_hz < 0:
            err("--sync-wave-frequency-hz must be non-negative")
    elif sync_mode == "CUSTOM":
        if args.sync_custom_file is None:
            err("--sync-custom-file is required when --sync-mode CUSTOM")
        if not args.sync_custom_file.exists():
            err(f"--sync-custom-file path does not exist: {args.sync_custom_file}")


def build_csv_rows(frames: List[List[Tuple[int, int, int]]]) -> List[Tuple[int, int, int, int, int]]:
    rows: List[Tuple[int, int, int, int, int]] = []
    for frame_index, frame in enumerate(frames):
        for led_index, (r, g, b) in enumerate(frame):
            rows.append((frame_index, led_index, r, g, b))
    return rows


def main(argv: Optional[Sequence[str]] = None) -> None:
    parser = build_cli_parser()
    args = parser.parse_args(argv)

    validate_args(parser, args)

    seed_numeric, seed_literal = parse_seed(args.seed)

    palette_sampler = build_palette_sampler(args.palette, args.ramp_space)
    params = collect_params(args)
    frames, show_params = generate_show(
        args.show,
        palette=palette_sampler,
        led_count=args.led_count,
        fps=args.fps,
        duration=args.duration,
        seed_numeric=seed_numeric,
        params=params,
    )

    meta_seed = load_metadata(args.meta)
    extra_meta: Dict[str, object] = {
        "show_type": args.show,
        "duration": args.duration,
        "fps": args.fps,
        "led_count": args.led_count,
        "palette": args.palette,
        "ramp_space": args.ramp_space,
        "frame_count": len(frames),
        "software_version": args.software_version,
        "show_params": show_params,
    }
    if seed_literal is not None:
        extra_meta["seed"] = seed_literal
    if seed_numeric is not None:
        extra_meta["seed_numeric"] = seed_numeric
    extra_meta["motion"] = build_motion_meta(args.motion_direction)
    extra_meta["sync"] = build_sync_meta(args)

    payload = {
        "frames": frames,
    }

    write_json(
        args.output,
        payload,
        meta_seed,
        extra_meta=extra_meta,
    )

    if args.csv:
        headers = ["frame", "led", "r", "g", "b"]
        rows = build_csv_rows(frames)
        write_csv(
            args.csv,
            rows,
            headers,
            meta_seed,
            extra_meta=extra_meta,
        )

    log_info(f"Wrote show payload to {args.output}" + (f" and {args.csv}" if args.csv else ""))


if __name__ == "__main__":  # pragma: no cover
    main()
