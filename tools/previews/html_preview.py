"""
HTML preview exporter derived from terminal ANSI renderer.

Generates standalone HTML by capturing the terminal preview output and converting
ANSI escape sequences to HTML using ansi2html when available, or falling back to
the `aha` CLI.
"""
from __future__ import annotations

import argparse
import json
import json
import re
import shutil
import subprocess
from io import StringIO
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

try:  # pragma: no cover - dependency optional
    from ansi2html import Ansi2HTMLConverter
except Exception:  # pragma: no cover
    Ansi2HTMLConverter = None

from . import terminal_preview
from ..tooling_core import build_parser as tooling_build_parser
from ..tooling_core import ensure_parent_dir, load_metadata, log_info

HTML_TEMPLATE = """<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <title>{title}</title>
  <style>
    body {{
      background: #000;
      color: #fff;
      font-family: monospace;
      white-space: pre;
    }}
    .frame {{
      margin: 0.25rem 0;
    }}
  </style>
</head>
<body>
{body}
</body>
</html>
"""


def convert_with_ansi2html(ansi_text: str) -> str:
    if Ansi2HTMLConverter is None:
        raise RuntimeError("ansi2html is not available")
    converter = Ansi2HTMLConverter(inline=True)
    return converter.convert(ansi_text, full=False)


def convert_with_aha(ansi_text: str) -> str:
    aha_path = shutil.which("aha")
    if not aha_path:
        raise RuntimeError("aha CLI not available")
    proc = subprocess.run(
        [aha_path, "--no-header"],
        input=ansi_text.encode("utf-8"),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True,
    )
    return proc.stdout.decode("utf-8")


ANSI_ESCAPE_RE = re.compile(r"\x1B\[[0-9;]*[A-Za-z]")


def convert_plain(ansi_text: str) -> str:
    stripped = ANSI_ESCAPE_RE.sub("", ansi_text)
    escaped = stripped.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    return f"<pre>{escaped}</pre>"


def frames_to_ansi(frames: Sequence[terminal_preview.Frame], config: terminal_preview.PreviewConfig) -> List[str]:
    captured: List[str] = []
    for frame in frames:
        buffer = StringIO()
        terminal_preview.render_terminal(
            [frame],
            fps=1,
            loop=False,
            static=True,
            config=config,
            output=buffer,
            sleep_fn=lambda _: None,
        )
        captured.append(buffer.getvalue())
    return captured


def assemble_html(
    frames: Sequence[terminal_preview.Frame],
    *,
    title: str,
    config: terminal_preview.PreviewConfig,
) -> str:
    ansi_frames = frames_to_ansi(frames, config)
    body_parts: List[str] = []
    for idx, ansi in enumerate(ansi_frames):
        try:
            html_fragment = convert_with_ansi2html(ansi)
        except RuntimeError:
            try:
                html_fragment = convert_with_aha(ansi)
            except RuntimeError:
                html_fragment = convert_plain(ansi)
        body_parts.append(f'<div class="frame" data-frame="{idx}">{html_fragment}</div>')
    return HTML_TEMPLATE.format(title=title, body="\n".join(body_parts))


def load_frames(path: Path) -> Tuple[List[terminal_preview.Frame], dict]:
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
    normalized = []
    for frame in frames:
        normalized.append([tuple(pixel[:3]) for pixel in frame])
    return normalized, meta


def build_parser() -> argparse.ArgumentParser:
    parser = tooling_build_parser("Generate HTML preview from PRISM payload", allow_csv=False)
    parser.add_argument("--input", required=True, help="Input PRISM palette/show JSON")
    parser.add_argument("--width", type=int, default=80, help="Preview width (default 80)")
    parser.add_argument("--block", default="█", help="Glyph used for rendering blocks")
    parser.add_argument("--gamma", type=float, default=terminal_preview.DEFAULT_GAMMA)
    parser.add_argument("--brightness", type=float, default=terminal_preview.DEFAULT_BRIGHTNESS)
    parser.add_argument("--saturation", type=float, default=terminal_preview.DEFAULT_SATURATION)
    parser.add_argument("--title", default="PRISM Preview", help="HTML document title")
    return parser


def main(argv: Optional[Sequence[str]] = None) -> None:
    parser = build_parser()
    args = parser.parse_args(argv)

    frames, meta = load_frames(Path(args.input))
    config = terminal_preview.PreviewConfig(
        gamma=args.gamma,
        brightness=args.brightness,
        saturation=args.saturation,
        width=args.width,
        block=args.block,
    )
    html_content = assemble_html(frames, title=args.title, config=config)

    metadata = load_metadata(args.meta)
    extra_meta: Dict[str, object] = {
        "source": args.input,
        "title": args.title,
        "width": args.width,
        "gamma": args.gamma,
        "brightness": args.brightness,
        "saturation": args.saturation,
    }
    merged_meta = dict(metadata)
    merged_meta.update(extra_meta)

    output_path = Path(args.output)
    ensure_parent_dir(output_path)
    with output_path.open("w", encoding="utf-8", newline="\n") as fh:
        fh.write(f"<!-- meta: {json.dumps(merged_meta, sort_keys=True)} -->\n")
        fh.write(html_content)
        if not html_content.endswith("\n"):
            fh.write("\n")
    log_info(f"Wrote HTML preview to {args.output}")


if __name__ == "__main__":  # pragma: no cover
    main()
