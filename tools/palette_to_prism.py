#!/usr/bin/env python3
"""
Palette → PRISM helper

Utility to turn a list of input colors into LED payloads suitable for PRISM presets
and authoring. Designed for developer-side use (not on-device).

Features
- Accepts HEX stops; interpolates across LEDs
- Interpolation space: HSLuv (default), HSV, or HSL
- Emits JSON (RGB arrays per LED), optional CSV, and optional RGBW alongside RGB
- Ready to extend into .prism v1.1 packaging if desired

Examples
  # Simple two-stop gradient (red→blue), 160 LEDs
  python tools/palette_to_prism.py \
      --palette "#ff0000,#0000ff" --led-count 160 --output out/red_to_blue.json

  # Multi-stop palette with HSV interpolation, include RGBW in output
  python tools/palette_to_prism.py \
      --palette "#ff0000,#00ff00,#0000ff" --led-count 160 \
      --space hsv --include-rgbw --csv out/palette.csv --output out/palette.json

Install dependency
  pip install -r tools/requirements.txt
"""
import argparse
import math
import random
import sys
from datetime import datetime, timezone
from typing import List, Optional, Tuple

try:
    from .tooling_core import (
        build_parser,
        load_metadata,
        log_info,
        write_csv,
        write_json,
    )
except ImportError:  # pragma: no cover - direct script execution
    from tooling_core import (
        build_parser,
        load_metadata,
        log_info,
        write_csv,
        write_json,
    )

SOFTWARE_VERSION = "prism-tools/0.1.0"

try:
    import hsluv
except Exception as e:  # pragma: no cover
    print("Missing dependency: hsluv", file=sys.stderr)
    print("Install: pip install hsluv or use tools/requirements.txt", file=sys.stderr)
    raise

try:
    from rgbw_colorspace_converter.colors.converters import RGB, Hex
except Exception as e:  # pragma: no cover
    print("Missing dependency: rgbw_colorspace_converter", file=sys.stderr)
    print("Install: pip install rgbw_colorspace_converter", file=sys.stderr)
    raise


def parse_hex_list(s: str) -> List[str]:
    items = [x.strip() for x in s.split(",") if x.strip()]
    for item in items:
        if not item.startswith("#") or len(item) not in (4, 7):
            raise argparse.ArgumentTypeError(f"Invalid hex color: {item}")
    return items


def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def lerp_hue(h0: float, h1: float, t: float) -> float:
    """Interpolate hue (0..1), choosing the shortest wrap-around path."""
    d = (h1 - h0) % 1.0
    if d > 0.5:
        d -= 1.0
    return (h0 + d * t) % 1.0


def interpolate_hsv(stops: List[Tuple[float, float, float]], steps: int) -> List[Tuple[float, float, float]]:
    if len(stops) < 2:
        raise ValueError("Need at least two HSV stops")
    out = []
    segments = len(stops) - 1
    per_seg = [math.floor(steps / segments)] * segments
    rem = steps - sum(per_seg)
    # distribute remainder
    i = 0
    while rem > 0:
        per_seg[i] += 1
        rem -= 1
        i = (i + 1) % segments
    for idx in range(segments):
        a = stops[idx]
        b = stops[idx + 1]
        n = per_seg[idx]
        if n <= 0:
            continue
        for j in range(n):
            t = j / max(1, n - 1)
            h = lerp_hue(a[0], b[0], t)
            s = lerp(a[1], b[1], t)
            v = lerp(a[2], b[2], t)
            out.append((h, s, v))
    return out[:steps]


def interpolate_hsl(stops: List[Tuple[float, float, float]], steps: int) -> List[Tuple[float, float, float]]:
    # Represent H in 0..1, S/L in 0..1; LERP with hue wrap
    return interpolate_hsv(stops, steps)


def lerp_hue_deg(h0: float, h1: float, t: float) -> float:
    """Interpolate hue in degrees [0,360) with shortest wrap."""
    d = (h1 - h0) % 360.0
    if d > 180.0:
        d -= 360.0
    return (h0 + d * t) % 360.0


def hex_to_rgb_tuple(hex_str: str) -> Tuple[int, int, int]:
    value = hex_str.lstrip('#')
    lv = len(value)
    if lv == 3:
        r = int(value[0] * 2, 16)
        g = int(value[1] * 2, 16)
        b = int(value[2] * 2, 16)
    elif lv == 6:
        r = int(value[0:2], 16)
        g = int(value[2:4], 16)
        b = int(value[4:6], 16)
    else:
        raise ValueError(f"Invalid hex color: {hex_str}")
    return (r, g, b)


def hex_to_hsluv_tuple(hex_str: str) -> Tuple[float, float, float]:
    r, g, b = hex_to_rgb_tuple(hex_str)
    rf, gf, bf = r / 255.0, g / 255.0, b / 255.0
    h, s, l = hsluv.rgb_to_hsluv([rf, gf, bf])
    return (h, s, l)  # h in [0,360), s,l in [0,100]


def interpolate_hsluv(stops: List[Tuple[float, float, float]], steps: int) -> List[Tuple[float, float, float]]:
    if len(stops) < 2:
        raise ValueError("Need at least two HSLuv stops")
    out: List[Tuple[float, float, float]] = []
    segments = len(stops) - 1
    per_seg = [math.floor(steps / segments)] * segments
    rem = steps - sum(per_seg)
    i = 0
    while rem > 0:
        per_seg[i] += 1
        rem -= 1
        i = (i + 1) % segments
    for idx in range(segments):
        a = stops[idx]
        b = stops[idx + 1]
        n = per_seg[idx]
        if n <= 0:
            continue
        for j in range(n):
            t = j / max(1, n - 1)
            h = lerp_hue_deg(a[0], b[0], t)
            s = lerp(a[1], b[1], t)
            l = lerp(a[2], b[2], t)
            out.append((h, s, l))
    return out[:steps]


def hex_to_hsv_tuple(hex_str: str) -> Tuple[float, float, float]:
    c = Hex(hex_str)
    return c.hsv


def hsv_to_rgb_tuple(hsv: Tuple[float, float, float]) -> Tuple[int, int, int]:
    # Use converters via RGB from HSV object
    col = RGB(*RGB(*Hex("#000000").rgb).rgb)  # dummy init, replaced below
    # The library doesn’t expose HSV constructor directly; workaround:
    # Initialize from RGB then set HSV by constructing a Color through Hex path
    # using a temporary approximation. Simpler: use internal math? Keep via HSV helper:
    # Build color by sweeping through Hex path is clumsy; instead leverage converters.hsv_to_rgb
    from rgbw_colorspace_converter.colors.converters import hsv_to_rgb

    r, g, b = hsv_to_rgb(hsv)
    return (r, g, b)


def rgb_to_rgbw_tuple(rgb: Tuple[int, int, int]) -> Tuple[int, int, int, int]:
    # Use Color class to compute RGBW from RGB via HSI path
    col = RGB(*rgb)
    return col.rgbw


def _resolve_seed(seed_value: Optional[str]) -> Tuple[Optional[int], Optional[str]]:
    if seed_value is None:
        return None, None
    try:
        numeric = int(seed_value, 0)
    except ValueError:
        random.seed(seed_value)
        return None, seed_value
    else:
        random.seed(numeric)
        return numeric, seed_value


def main(argv=None):
    parser = build_parser("Convert palette stops to LED payloads (RGB/RGBW)")
    parser.add_argument(
        "--palette",
        type=parse_hex_list,
        required=True,
        help="Comma-separated HEX colors, e.g. #ff0000,#00ff00,#0000ff",
    )
    parser.add_argument(
        "--led-count",
        type=int,
        required=True,
        help="Number of LEDs to generate",
    )
    parser.add_argument(
        "--space",
        choices=["hsluv", "hsv", "hsl"],
        default="hsluv",
        help="Interpolation space (default hsluv)",
    )
    parser.add_argument(
        "--include-rgbw",
        action="store_true",
        help="Emit RGBW tuples alongside RGB",
    )
    parser.add_argument(
        "--seed",
        type=str,
        help="Optional random seed recorded in metadata (supports int or hex literal)",
    )
    parser.add_argument(
        "--timestamp",
        type=str,
        help="Override generated_at metadata (ISO-8601). Defaults to current UTC time.",
    )
    parser.add_argument(
        "--software-version",
        default=SOFTWARE_VERSION,
        help="Software version tag stored in metadata",
    )
    args = parser.parse_args(argv)
    seed_numeric, seed_literal = _resolve_seed(args.seed)
    generated_at = args.timestamp or datetime.now(timezone.utc).isoformat(timespec="seconds")

    if args.space == "hsluv":
        hsluv_stops: List[Tuple[float, float, float]] = [hex_to_hsluv_tuple(h) for h in args.palette]
        hsluv_list = interpolate_hsluv(hsluv_stops, args.led_count)
        # Convert each HSLuv to RGB
        rgb_list: List[Tuple[int, int, int]] = []
        for h, s, l in hsluv_list:
            rf, gf, bf = hsluv.hsluv_to_rgb([h, s, l])
            rgb_list.append((int(round(rf * 255)), int(round(gf * 255)), int(round(bf * 255))))
    else:
        # Build HSV/HSL stops in 0..1 domain
        hsv_stops: List[Tuple[float, float, float]] = [hex_to_hsv_tuple(h) for h in args.palette]
        if args.space == "hsv":
            hsv_list = interpolate_hsv(hsv_stops, args.led_count)
        else:
            hsv_list = interpolate_hsl(hsv_stops, args.led_count)
        rgb_list: List[Tuple[int, int, int]] = [hsv_to_rgb_tuple(hsv) for hsv in hsv_list]

    rgbw_list = None
    if args.include_rgbw:
        rgbw_list = [rgb_to_rgbw_tuple(rgb) for rgb in rgb_list]

    seed_meta = load_metadata(args.meta)
    extra_meta = {
        "palette": args.palette,
        "ramp_space": args.space,
        "led_count": args.led_count,
        "include_rgbw": bool(rgbw_list),
        "tool": "palette_to_prism",
        "generated_at": generated_at,
        "software_version": args.software_version,
    }
    if seed_literal is not None:
        extra_meta["seed"] = seed_literal
    if seed_numeric is not None:
        extra_meta["seed_numeric"] = seed_numeric

    payload = {"rgb": rgb_list}
    if rgbw_list is not None:
        payload["rgbw"] = rgbw_list

    write_json(
        args.output,
        payload,
        seed_meta,
        extra_meta=extra_meta,
    )

    if args.csv:
        headers = ["r", "g", "b", "w"] if rgbw_list is not None else ["r", "g", "b"]
        rows_source = rgbw_list if rgbw_list is not None else rgb_list
        write_csv(
            args.csv,
            rows_source,
            headers,
            seed_meta,
            extra_meta=extra_meta,
        )

    log_info(f"Wrote {args.output}" + (f" and {args.csv}" if args.csv else ""))


if __name__ == "__main__":
    main()
