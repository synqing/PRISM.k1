from __future__ import annotations

import math
from typing import Sequence, Tuple

import pytest

from tools.show_engine import (
    FrameContext,
    PaletteSampler,
    build_palette_lookup,
    clamp,
    generate_frames,
    get_easing_function,
    list_easing_functions,
)


def test_palette_lookup_sampling_basic() -> None:
    lookup = build_palette_lookup(["#000000", "#ffffff"], space="hsv", steps=5)
    assert lookup[0] == (0, 0, 0)
    assert lookup[-1] == (255, 255, 255)

    sampler = PaletteSampler(["#000000", "#ffffff"], space="hsv", steps=5)
    assert sampler.sample(0.0) == (0, 0, 0)
    assert sampler.sample(1.0) == (255, 255, 255)
    assert sampler.sample(0.5) == lookup[2]


def test_clamp_bounds() -> None:
    assert clamp(-1.0) == 0.0
    assert clamp(2.0) == 1.0
    assert clamp(0.25) == 0.25


def test_get_easing_function() -> None:
    names = list_easing_functions()
    assert "linear" in names
    ease = get_easing_function("smoothstep")
    assert math.isclose(ease(0.0), 0.0)
    assert 0.0 < ease(0.5) < 1.0
    with pytest.raises(ValueError):
        get_easing_function("nonexistent")


class DummyShow:
    def __init__(self, palette: PaletteSampler) -> None:
        self.palette = palette

    def frame_at(self, context: FrameContext) -> Sequence[Tuple[int, int, int]]:
        easing = get_easing_function("ease_in_out_sine")
        time_value = easing(context.time % 1.0)
        noise = context.rng.random()
        return [self.palette.sample(clamp((time_value + noise) / 2))]


def test_generate_frames_deterministic() -> None:
    palette = PaletteSampler(["#000000", "#ffffff"], space="hsv", steps=32)
    show = DummyShow(palette)
    frames_a = generate_frames(show, duration=1.0, fps=4, seed=42)
    frames_b = generate_frames(show, duration=1.0, fps=4, seed=42)
    assert frames_a == frames_b
    assert len(frames_a) == 4

    # Changing seed should change output
    frames_c = generate_frames(show, duration=1.0, fps=4, seed=99)
    assert frames_c != frames_a
