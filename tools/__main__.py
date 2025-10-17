"""
Module entry point for `python -m tools`.

Exports a lightweight catalog of available tool modules, primarily to exercise
the shared CLI scaffolding established in `tools.tooling_core`.
"""
from __future__ import annotations

try:
    from .tooling_core import build_parser, load_metadata, log_info, write_json
except ImportError:  # pragma: no cover - direct module execution fallback
    from tooling_core import build_parser, load_metadata, log_info, write_json

CATALOG_PAYLOAD = {
    "commands": [
        {
            "name": "palette_to_prism",
            "module": "tools.palette_to_prism",
            "description": "Convert palette stops into RGB/RGBW payloads",
        },
    ]
}


def main(argv=None):
    parser = build_parser("PRISM tooling catalog export", allow_csv=False)
    parser.add_argument(
        "--label",
        default="tooling-catalog",
        help="Label recorded in the metadata payload",
    )
    args = parser.parse_args(argv)

    metadata = load_metadata(args.meta)
    extra_meta = {
        "tool": "tools/__main__",
        "label": args.label,
    }
    write_json(args.output, CATALOG_PAYLOAD, metadata, extra_meta=extra_meta)
    log_info(f"Tooling catalog written to {args.output}")


if __name__ == "__main__":
    main()
