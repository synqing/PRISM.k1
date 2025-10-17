"""
Core utilities for PRISM show generation.

This package provides shared helpers for palette sampling, easing functions,
and deterministic frame generation that upcoming show presets will reuse.
"""

from .core import (  # noqa: F401
    FrameContext,
    PaletteSampler,
    build_palette_lookup,
    clamp,
    generate_frames,
    get_easing_function,
    list_easing_functions,
)

__all__ = [
    "FrameContext",
    "PaletteSampler",
    "build_palette_lookup",
    "clamp",
    "generate_frames",
    "get_easing_function",
    "list_easing_functions",
]
