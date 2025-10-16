#!/bin/bash
# .taskmaster/scripts/extract-constants.sh
#
# Purpose: Extract machine-readable constants from CANON.md
# Usage: ./extract-constants.sh [CANON_FILE]
#
# Outputs JSON with all constants defined in CANON

set -euo pipefail

CANON_FILE="${1:-.taskmaster/CANON.md}"

if [ ! -f "$CANON_FILE" ]; then
    echo "{\"error\": \"CANON file not found: $CANON_FILE\"}" >&2
    exit 1
fi

# Extract all YAML blocks from CANON
# Convert YAML to JSON format

in_yaml=0
yaml_content=""

while IFS= read -r line; do
    # Start of YAML block
    if [[ "$line" == '```yaml' ]]; then
        in_yaml=1
        continue
    fi

    # End of YAML block
    if [[ "$line" == '```' ]] && [ $in_yaml -eq 1 ]; then
        in_yaml=0
        continue
    fi

    # Capture YAML content
    if [ $in_yaml -eq 1 ]; then
        # Remove comments
        clean_line=$(echo "$line" | sed 's/#.*//')
        if [ -n "$clean_line" ]; then
            yaml_content="$yaml_content$clean_line"$'\n'
        fi
    fi
done < "$CANON_FILE"

# Convert YAML to JSON
# For simplicity, we'll output as JSON directly
echo "{"

# Extract ws_buffer_size
ws_buffer=$(echo "$yaml_content" | grep "ws_buffer_size:" | sed 's/.*: *//' | tail -1)
if [ -n "$ws_buffer" ]; then
    echo "  \"ws_buffer_size\": $ws_buffer,"
fi

# Extract ws_max_clients
ws_clients=$(echo "$yaml_content" | grep "ws_max_clients:" | sed 's/.*: *//' | tail -1)
if [ -n "$ws_clients" ]; then
    echo "  \"ws_max_clients\": $ws_clients,"
fi

# Extract ws_timeout_ms
ws_timeout=$(echo "$yaml_content" | grep "ws_timeout_ms:" | sed 's/.*: *//' | tail -1)
if [ -n "$ws_timeout" ]; then
    echo "  \"ws_timeout_ms\": $ws_timeout,"
fi

# Extract led_count
led_count=$(echo "$yaml_content" | grep "led_count:" | sed 's/.*: *//' | tail -1)
if [ -n "$led_count" ]; then
    echo "  \"led_count\": $led_count,"
fi

# Extract led_type
led_type=$(echo "$yaml_content" | grep "led_type:" | sed 's/.*: *//' | tail -1)
if [ -n "$led_type" ]; then
    echo "  \"led_type\": \"$led_type\","
fi

# Extract led_fps_target
led_fps=$(echo "$yaml_content" | grep "led_fps_target:" | sed 's/.*: *//' | tail -1)
if [ -n "$led_fps" ]; then
    echo "  \"led_fps_target\": $led_fps,"
fi

# Extract pattern_max_size
pattern_size=$(echo "$yaml_content" | grep "pattern_max_size:" | sed 's/.*: *//' | tail -1)
if [ -n "$pattern_size" ]; then
    echo "  \"pattern_max_size\": $pattern_size,"
fi

# Extract pattern_min_count
pattern_count=$(echo "$yaml_content" | grep "pattern_min_count:" | sed 's/.*: *//' | tail -1)
if [ -n "$pattern_count" ]; then
    echo "  \"pattern_min_count\": $pattern_count,"
fi

# Extract storage_reserved
storage_reserved=$(echo "$yaml_content" | grep "storage_reserved:" | sed 's/.*: *//' | tail -1)
if [ -n "$storage_reserved" ]; then
    echo "  \"storage_reserved\": $storage_reserved,"
fi

# Extract storage_mount_path
mount_path=$(echo "$yaml_content" | grep "storage_mount_path:" | sed 's/.*: *"//' | sed 's/".*//' | tail -1)
if [ -n "$mount_path" ]; then
    echo "  \"storage_mount_path\": \"$mount_path\","
fi

# Extract storage_type
storage_type=$(echo "$yaml_content" | grep "storage_type:" | sed 's/.*: *"//' | sed 's/".*//' | tail -1)
if [ -n "$storage_type" ]; then
    echo "  \"storage_type\": \"$storage_type\","
fi

# Extract storage_label
storage_label=$(echo "$yaml_content" | grep "storage_label:" | sed 's/.*: *"//' | sed 's/".*//' | tail -1)
if [ -n "$storage_label" ]; then
    echo "  \"storage_label\": \"$storage_label\""
fi

echo "}"
