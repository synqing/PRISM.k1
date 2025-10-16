#!/usr/bin/env bash
# Archive transient build artefacts and handover docs to keep the repo root tidy.

set -euo pipefail

ARCHIVE_ROOT="archive/$(date +%Y-%m)"
LOG_DIR="$ARCHIVE_ROOT/logs"
DOC_DIR="$ARCHIVE_ROOT/docs"
MISC_DIR="$ARCHIVE_ROOT/misc"

mkdir -p "$LOG_DIR" "$DOC_DIR" "$MISC_DIR"

shopt -s nullglob

for log_path in build_output_tmp*.log; do
  mv "$log_path" "$LOG_DIR/"
done

for doc_path in HANDOVER_*.md; do
  mv "$doc_path" "$DOC_DIR/"
done

for misc_path in my-report.json task_20_*.txt; do
  if [ -e "$misc_path" ]; then
    mv "$misc_path" "$MISC_DIR/"
  fi
done

shopt -u nullglob

echo "Cleanup complete. Archived assets under $ARCHIVE_ROOT/"
