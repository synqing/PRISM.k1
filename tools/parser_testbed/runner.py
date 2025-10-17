"""
CLI entrypoint for the parser testbed.
"""
from __future__ import annotations

import argparse
import glob
from pathlib import Path
from typing import Sequence

from ..tooling_core import log_error, log_info, write_json

from .vectors import (
    ParserVector,
    default_vector_set,
    load_vectors_from_disk,
    log_results,
    validate_vector,
    write_vectors,
)
from .artifacts import (
    validate_artifact,
    summarise_results as summarise_artifacts,
    dump_report as dump_artifact_report,
)


DEFAULT_VECTOR_DIR = Path("tools/parser_testbed/vectors")


def _ensure_output_dir(path: Path) -> Path:
    path.mkdir(parents=True, exist_ok=True)
    return path


def _add_common_output_argument(parser: argparse.ArgumentParser, *, required: bool = False) -> None:
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_VECTOR_DIR,
        required=required,
        help=f"Directory to write vector artifacts (default: {DEFAULT_VECTOR_DIR})",
    )


def _add_common_input_argument(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--input-dir",
        type=Path,
        default=DEFAULT_VECTOR_DIR,
        help=f"Directory containing vector artifacts (default: {DEFAULT_VECTOR_DIR})",
    )


def _command_generate(args: argparse.Namespace) -> int:
    vectors = default_vector_set(include_mutations=args.include_mutations)
    output_dir = _ensure_output_dir(args.output_dir)
    artefacts = write_vectors(vectors, output_dir)
    log_info(f"Wrote {len(artefacts)} vectors to {output_dir}")
    return 0


def _command_mutate(args: argparse.Namespace) -> int:
    all_vectors = default_vector_set(include_mutations=True)
    mutation_vectors = [vec for vec in all_vectors if vec.category == "mutations"]
    output_dir = _ensure_output_dir(args.output_dir)
    artefacts = write_vectors(mutation_vectors, output_dir)
    log_info(f"Wrote {len(artefacts)} mutation vectors to {output_dir}")
    return 0


def _maybe_filter_by_category(
    vectors: Sequence[ParserVector], category: str | None
) -> Sequence[ParserVector]:
    if category is None:
        return vectors
    return [vec for vec in vectors if vec.category == category]


def _command_regress(args: argparse.Namespace) -> int:
    input_dir = args.input_dir
    if args.regenerate:
        log_info("Regenerating default vector set before regression run.")
        _ensure_output_dir(input_dir)
        write_vectors(default_vector_set(include_mutations=True), input_dir)

    if not input_dir.exists():
        log_error(f"Input directory {input_dir} does not exist.")
        return 1

    vectors = load_vectors_from_disk(input_dir)
    vectors = list(_maybe_filter_by_category(vectors, args.category))
    if not vectors:
        log_error("No vectors found for regression.")
        return 1

    results = [validate_vector(vec) for vec in vectors]
    log_results(results)

    if args.report:
        write_json(
            args.report,
            {
                "vectors": [
                    {
                        "name": result.vector.name,
                        "category": result.vector.category,
                        "status": result.status,
                        "warnings": result.warnings,
                        "detail": result.detail,
                        "expected": result.vector.expected_outcome,
                    }
                    for result in results
                ],
            },
            metadata={"source": "parser-testbed", "vector_count": len(results)},
        )
        log_info(f"Wrote regression report to {args.report}")
    return 0 if all(result.is_success() for result in results) else 2


def _collect_artifact_paths(files: Sequence[str] | None, globs: Sequence[str] | None) -> list[Path]:
    paths: list[Path] = []
    if files:
        paths.extend(Path(item) for item in files)
    if globs:
        for pattern in globs:
            matches = glob.glob(pattern, recursive=True)
            if not matches:
                log_error(f"No files matched pattern: {pattern}")
            for match in matches:
                candidate = Path(match)
                if candidate.is_file():
                    paths.append(candidate)
    seen: set[Path] = set()
    deduped: list[Path] = []
    for path in paths:
        if path not in seen:
            deduped.append(path)
            seen.add(path)
    return deduped


def _command_inspect(args: argparse.Namespace) -> int:
    paths = _collect_artifact_paths(args.files, args.glob)
    if not paths:
        log_error("No artifact files supplied.")
        return 1

    results = []
    for path in paths:
        if not path.exists():
            log_error(f"Artifact not found: {path}")
            return 1
        result = validate_artifact(path)
        status = "OK" if result.is_success() else "FAIL"
        log_info(f"{status} {path}")
        for warning in result.warnings:
            log_info(f"[warn] {warning}")
        if result.detail and not result.is_success():
            log_error(result.detail)
        results.append(result)

    if args.report:
        dump_artifact_report(results, args.report)
        log_info(f"Wrote artifact report to {args.report}")

    summary = summarise_artifacts(results)
    if summary["failures"]:
        log_error(f"Artifact validation failed for {len(summary['failures'])} file(s).")
        return 2
    return 0


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="PRISM parser metadata testbed utilities."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    generate_parser = subparsers.add_parser(
        "generate", help="Generate golden vectors (optionally with mutations)."
    )
    _add_common_output_argument(generate_parser)
    generate_parser.add_argument(
        "--include-mutations",
        action="store_true",
        help="Include mutation vectors alongside golden outputs.",
    )
    generate_parser.set_defaults(func=_command_generate)

    mutate_parser = subparsers.add_parser(
        "mutate", help="Generate only mutation vectors."
    )
    _add_common_output_argument(mutate_parser)
    mutate_parser.set_defaults(func=_command_mutate)

    regress_parser = subparsers.add_parser(
        "regress", help="Validate vectors and report results."
    )
    _add_common_input_argument(regress_parser)
    regress_parser.add_argument(
        "--category",
        choices=["golden", "mutations"],
        help="Limit regression to a specific vector category.",
    )
    regress_parser.add_argument(
        "--regenerate",
        action="store_true",
        help="Regenerate default vectors in the target directory before regression.",
    )
    regress_parser.add_argument(
        "--report",
        type=Path,
        help="Optional JSON report output path.",
    )
    regress_parser.set_defaults(func=_command_regress)

    inspect_parser = subparsers.add_parser(
        "inspect", help="Validate packaged .prism artifacts."
    )
    group = inspect_parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--files", nargs="+", help="Explicit artifact paths.")
    group.add_argument("--glob", nargs="+", help="Glob pattern(s) for artifacts.")
    inspect_parser.add_argument(
        "--report",
        type=Path,
        help="Optional JSON report output path.",
    )
    inspect_parser.set_defaults(func=_command_inspect)

    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
