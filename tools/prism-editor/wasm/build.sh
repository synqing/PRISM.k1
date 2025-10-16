#!/bin/bash
# Build WASM module for .prism export

set -euo pipefail

emcc serialize_prism.c \
  -s MODULARIZE=1 \
  -s EXPORT_NAME='PrismExporter' \
  -s ENVIRONMENT='web' \
  -s EXPORTED_FUNCTIONS='["_create_prism_header"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -O3 \
  -o prism_exporter.js

echo "WASM build complete: prism_exporter.js + prism_exporter.wasm"

