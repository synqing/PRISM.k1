#!/usr/bin/env python3
"""
Dependency verification script for PRISM tooling.

Ensures the required Python packages import correctly and that the external
`aha` CLI is available for ANSI → HTML conversions used during preview export.
"""
from __future__ import annotations

import importlib
import shutil
import sys
from typing import Dict

# Required for core tooling
REQUIRED_PY_DEPS: Dict[str, str] = {
    "hsluv": "hsluv",
}

# Optional: used when specific features are enabled
OPTIONAL_PY_DEPS: Dict[str, str] = {
    # RGBW emission is out-of-scope for this project; keep optional
    "rgbw-colorspace-converter": "rgbw_colorspace_converter",
    # ANSI preview helper; terminal preview falls back if missing
    "colr": "colr",
    # HTML converter; will fall back to 'aha' or plain text
    "ansi2html": "ansi2html",
}

# Optional CLI; HTML preview will fall back to plain if unavailable
OPTIONAL_CLI_DEPS: Dict[str, str] = {
    "aha": "aha",
}


def check_python_modules() -> None:
    # Hard requirements
    for package_name, module_name in REQUIRED_PY_DEPS.items():
        try:
            importlib.import_module(module_name)
        except ImportError as exc:  # pragma: no cover - exercised in manual runs
            raise SystemExit(
                f"Missing required Python dependency '{package_name}'. "
                "Run `pip install -r tools/requirements.txt`."
            ) from exc

    # Optional dependencies – warn only
    for package_name, module_name in OPTIONAL_PY_DEPS.items():
        try:
            importlib.import_module(module_name)
        except ImportError:
            print(
                f"[warn] Optional Python dependency '{package_name}' not found. "
                "Related features will use fallbacks or be disabled.",
                file=sys.stderr,
            )


def check_cli_tools() -> None:
    for label, binary in OPTIONAL_CLI_DEPS.items():
        if shutil.which(binary) is None:
            print(
                f"[warn] Optional CLI tool '{label}' not found. "
                "HTML preview will fall back to plain rendering.",
                file=sys.stderr,
            )


def main() -> None:
    check_python_modules()
    check_cli_tools()
    print("All tooling dependencies satisfied.")


if __name__ == "__main__":
    main()
