"""
Shared palette sampling and frame-generation utilities.
"""
from __future__ import annotations

import colorsys
import math
import random
from dataclasses import dataclass
from typing import Callable, Dict, Iterable, List, MutableSequence, Protocol, Sequence, Tuple, Union

RGBTuple = Tuple[int, int, int]
PaletteInput = Sequence[str]
PaletteLookup = Sequence[RGBTuple]
EasingFunction = Callable[[float], float]

try:  # pragma: no cover - optional dependency
    import hsluv  # type: ignore
except Exception:  # pragma: no cover - match existing tooling behaviour
    hsluv = None

try:  # pragma: no cover - optional dependency
    from rgbw_colorspace_converter.colors.converters import Hex  # type: ignore
except Exception:  # pragma: no cover - fall back to stdlib paths
    Hex = None

DEFAULT_PALETTE_STEPS = 1024
_PALETTE_SPACES = {"hsv", "hsl", "hsluv"}


def clamp(value: float, lo: float = 0.0, hi: float = 1.0) -> float:
    """Clamp a float value to the inclusive range [lo, hi]."""
    return max(lo, min(hi, value))


def _hex_to_rgb(hex_str: str) -> RGBTuple:
    value = hex_str.lstrip("#")
    if len(value) == 3:
        value = "".join(ch * 2 for ch in value)
    if len(value) != 6:
        raise ValueError(f"Invalid hex colour {hex_str!r}")
    r = int(value[0:2], 16)
    g = int(value[2:4], 16)
    b = int(value[4:6], 16)
    return r, g, b


def _hex_to_hsv(hex_str: str) -> Tuple[float, float, float]:
    if Hex is not None:  # pragma: no cover - leverage faster converter when present
        return Hex(hex_str).hsv
    r, g, b = _hex_to_rgb(hex_str)
    return colorsys.rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)


def _hex_to_hsluv(hex_str: str) -> Tuple[float, float, float]:
    if hsluv is None:
        raise RuntimeError("hsluv library required for ramp_space 'hsluv'.")
    r, g, b = _hex_to_rgb(hex_str)
    return hsluv.rgb_to_hsluv([r / 255.0, g / 255.0, b / 255.0])


def _hsv_to_rgb(hsv: Tuple[float, float, float]) -> RGBTuple:
    r, g, b = colorsys.hsv_to_rgb(*hsv)
    return (
        int(round(clamp(r) * 255)),
        int(round(clamp(g) * 255)),
        int(round(clamp(b) * 255)),
    )


def _hsluv_to_rgb(hsluv_triplet: Tuple[float, float, float]) -> RGBTuple:
    if hsluv is None:
        raise RuntimeError("hsluv library required for ramp_space 'hsluv'.")
    r, g, b = hsluv.hsluv_to_rgb(list(hsluv_triplet))
    return (
        int(round(clamp(r) * 255)),
        int(round(clamp(g) * 255)),
        int(round(clamp(b) * 255)),
    )


def _segment_counts(stop_count: int, steps: int) -> List[int]:
    if stop_count < 2:
        raise ValueError("Palette must contain at least two colour stops.")
    segments = stop_count - 1
    counts = [steps // segments] * segments
    remainder = steps - sum(counts)
    index = 0
    while remainder > 0:
        counts[index] += 1
        index = (index + 1) % segments
        remainder -= 1
    return counts


def _lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def _lerp_hue_unit(a: float, b: float, t: float) -> float:
    delta = (b - a) % 1.0
    if delta > 0.5:
        delta -= 1.0
    return (a + delta * t) % 1.0


def _lerp_hue_degrees(a: float, b: float, t: float) -> float:
    delta = (b - a) % 360.0
    if delta > 180.0:
        delta -= 360.0
    return (a + delta * t) % 360.0


def _interpolate_hsv(stops: Sequence[Tuple[float, float, float]], steps: int) -> List[Tuple[float, float, float]]:
    counts = _segment_counts(len(stops), steps)
    result: List[Tuple[float, float, float]] = []
    for idx, count in enumerate(counts):
        a = stops[idx]
        b = stops[idx + 1]
        if count <= 1:
            result.append(a)
            continue
        for step in range(count):
            t = step / (count - 1)
            h = _lerp_hue_unit(a[0], b[0], t)
            s = _lerp(a[1], b[1], t)
            v = _lerp(a[2], b[2], t)
            result.append((h, s, v))
    return result[:steps]


def _interpolate_hsluv(stops: Sequence[Tuple[float, float, float]], steps: int) -> List[Tuple[float, float, float]]:
    counts = _segment_counts(len(stops), steps)
    result: List[Tuple[float, float, float]] = []
    for idx, count in enumerate(counts):
        a = stops[idx]
        b = stops[idx + 1]
        if count <= 1:
            result.append(a)
            continue
        for step in range(count):
            t = step / (count - 1)
            h = _lerp_hue_degrees(a[0], b[0], t)
            s = _lerp(a[1], b[1], t)
            l = _lerp(a[2], b[2], t)
            result.append((h, s, l))
    return result[:steps]


def build_palette_lookup(
    palette: PaletteInput,
    *,
    space: str = "hsluv",
    steps: int = DEFAULT_PALETTE_STEPS,
) -> List[RGBTuple]:
    """
    Build an RGB lookup table for the supplied hex palette.

    Args:
        palette: Sequence of hex strings (``#RRGGBB`` or ``#RGB``).
        space: Interpolation colour space (hsv|hsl|hsluv).
        steps: Number of discrete lookup entries to generate.
    """
    if steps <= 0:
        raise ValueError("steps must be positive")
    if space not in _PALETTE_SPACES:
        raise ValueError(f"Unsupported palette space '{space}'.")
    if len(palette) < 2:
        raise ValueError("Palette must contain at least two colours.")

    if space == "hsluv":
        stops = [_hex_to_hsluv(hex_colour) for hex_colour in palette]
        interpolated = _interpolate_hsluv(stops, steps)
        return [_hsluv_to_rgb(stop) for stop in interpolated]

    hsv_stops = [_hex_to_hsv(hex_colour) for hex_colour in palette]
    if space == "hsl":
        # Interpret HSV tuple as HSL by reusing interpolation but mapping S/V.
        # Convert HSV to HSL (approximation).
        converted: List[Tuple[float, float, float]] = []
        for h, s, v in hsv_stops:
            l = v * (1 - s / 2)
            if l in (0, 1):
                sat = 0
            else:
                sat = (v - l) / min(l, 1 - l)
            converted.append((h, clamp(sat), clamp(l)))
        interpolated = _interpolate_hsv(converted, steps)
        # Convert back to RGB via coloursys.hls_to_rgb
        rgb_lookup: List[RGBTuple] = []
        for h, s, l in interpolated:
            r, g, b = colorsys.hls_to_rgb(h, clamp(l), clamp(s))
            rgb_lookup.append(
                (
                    int(round(clamp(r) * 255)),
                    int(round(clamp(g) * 255)),
                    int(round(clamp(b) * 255)),
                )
            )
        return rgb_lookup

    interpolated_hsv = _interpolate_hsv(hsv_stops, steps)
    return [_hsv_to_rgb(hsv_triplet) for hsv_triplet in interpolated_hsv]


class PaletteSampler:
    """Lookup-based sampler mapping [0,1] values onto RGB tuples."""

    def __init__(
        self,
        palette: PaletteInput,
        *,
        space: str = "hsluv",
        steps: int = DEFAULT_PALETTE_STEPS,
    ):
        self._lookup = build_palette_lookup(palette, space=space, steps=steps)

    @property
    def lookup(self) -> PaletteLookup:
        return self._lookup

    def sample(self, value: float) -> RGBTuple:
        idx = int(clamp(value) * (len(self._lookup) - 1))
        return self._lookup[idx]


def sample_palette(lookup: PaletteLookup, value: float) -> RGBTuple:
    """Sample a pre-built palette lookup with a normalised value."""
    if not lookup:
        raise ValueError("Palette lookup cannot be empty.")
    idx = int(clamp(value) * (len(lookup) - 1))
    return lookup[idx]


def linear_ease(t: float) -> float:
    return clamp(t)


def smoothstep_ease(t: float) -> float:
    t = clamp(t)
    return t * t * (3 - 2 * t)


def ease_in_out_sine(t: float) -> float:
    t = clamp(t)
    return 0.5 * (1 - math.cos(math.pi * t))


_EASING_FUNCTIONS: Dict[str, EasingFunction] = {
    "linear": linear_ease,
    "smoothstep": smoothstep_ease,
    "ease_in_out_sine": ease_in_out_sine,
}


def get_easing_function(name: str) -> EasingFunction:
    """Return a registered easing function."""
    try:
        return _EASING_FUNCTIONS[name]
    except KeyError as exc:
        raise ValueError(f"Unknown easing '{name}'. Available: {sorted(_EASING_FUNCTIONS)}") from exc


def list_easing_functions() -> List[str]:
    """List registered easing identifiers."""
    return sorted(_EASING_FUNCTIONS)


@dataclass(frozen=True)
class FrameContext:
    """Context supplied to frame generators."""

    index: int
    time: float
    rng: random.Random


class FrameProducer(Protocol):
    """Protocol for classes that can generate frame RGB data."""

    def frame_at(self, context: FrameContext) -> Sequence[RGBTuple]:
        ...


FrameCallable = Callable[[FrameContext], Sequence[RGBTuple]]
FrameSource = Union[FrameProducer, FrameCallable]


def generate_frames(
    source: FrameSource,
    *,
    duration: float,
    fps: float,
    seed: int | None = None,
) -> List[List[RGBTuple]]:
    """
    Generate a deterministic list of frames from a producer.

    Args:
        source: Object implementing ``frame_at`` or a callable accepting ``FrameContext``.
        duration: Total playback duration in seconds.
        fps: Frames per second.
        seed: Optional RNG seed. A per-frame ``random.Random`` instance is seeded
              with ``seed + frame_index`` to ensure deterministic but independent streams.
    """
    if fps <= 0:
        raise ValueError("fps must be positive")
    if duration <= 0:
        raise ValueError("duration must be positive")

    frame_count = max(1, int(round(duration * fps)))
    base_seed = seed if seed is not None else 0

    frames: List[List[RGBTuple]] = []
    for index in range(frame_count):
        frame_seed = (base_seed + index) & 0xFFFFFFFF
        rng = random.Random(frame_seed)
        context = FrameContext(index=index, time=index / fps, rng=rng)
        if hasattr(source, "frame_at"):
            raw_frame = source.frame_at(context)  # type: ignore[union-attr]
        else:
            raw_frame = source(context)  # type: ignore[arg-type]
        rgb_frame = [tuple(int(component) for component in colour) for colour in raw_frame]
        frames.append(rgb_frame)
    return frames
