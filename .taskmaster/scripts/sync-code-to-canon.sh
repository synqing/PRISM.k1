#!/bin/bash
# .taskmaster/scripts/sync-code-to-canon.sh
#
# Purpose: Generate code files from CANON specifications
# Usage: ./sync-code-to-canon.sh
#
# Generates prism_config.h and updates configuration files from CANON

set -euo pipefail

# Configuration
CANON=".taskmaster/CANON.md"
SCRIPTS_DIR=".taskmaster/scripts"
FIRMWARE_DIR="firmware"
CONFIG_DIR="$FIRMWARE_DIR/components/core/include"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== Sync Code to CANON ===${NC}"
echo ""

# Check CANON exists
if [ ! -f "$CANON" ]; then
    echo -e "${RED}✗ CANON.md not found!${NC}"
    exit 1
fi

# Extract constants
echo -e "${BLUE}[1/3] Extracting constants from CANON...${NC}"
CANON_CONSTANTS=$("$SCRIPTS_DIR/extract-constants.sh" "$CANON")
echo -e "  ${GREEN}✓${NC} Constants extracted"

# Create config directory if needed
mkdir -p "$CONFIG_DIR"

# Generate prism_config.h
echo -e "${BLUE}[2/3] Generating prism_config.h...${NC}"
CONFIG_H="$CONFIG_DIR/prism_config.h"

# Extract individual values
ws_buffer=$(echo "$CANON_CONSTANTS" | grep -o '"ws_buffer_size": [0-9]*' | grep -o '[0-9]*$')
ws_clients=$(echo "$CANON_CONSTANTS" | grep -o '"ws_max_clients": [0-9]*' | grep -o '[0-9]*$')
ws_timeout=$(echo "$CANON_CONSTANTS" | grep -o '"ws_timeout_ms": [0-9]*' | grep -o '[0-9]*$')
led_count=$(echo "$CANON_CONSTANTS" | grep -o '"led_count": [0-9]*' | grep -o '[0-9]*$')
led_fps=$(echo "$CANON_CONSTANTS" | grep -o '"led_fps_target": [0-9]*' | grep -o '[0-9]*$')
pattern_size=$(echo "$CANON_CONSTANTS" | grep -o '"pattern_max_size": [0-9]*' | grep -o '[0-9]*$')
pattern_count=$(echo "$CANON_CONSTANTS" | grep -o '"pattern_min_count": [0-9]*' | grep -o '[0-9]*$')
storage_reserved=$(echo "$CANON_CONSTANTS" | grep -o '"storage_reserved": [0-9]*' | grep -o '[0-9]*$')
mount_path=$(echo "$CANON_CONSTANTS" | grep -o '"storage_mount_path": "[^"]*"' | sed 's/"storage_mount_path": "\(.*\)"/\1/')

# Generate header file
cat > "$CONFIG_H" << EOF
/**
 * @file prism_config.h
 * @brief PRISM K1 Configuration Constants
 *
 * AUTO-GENERATED FROM CANON.md
 * DO NOT EDIT MANUALLY - Changes will be overwritten
 *
 * Generated: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
 * Source: CANON.md (Architecture Decision Records)
 */

#ifndef PRISM_CONFIG_H
#define PRISM_CONFIG_H

/*
 * WebSocket Configuration
 * Source: ADR-002 (WebSocket Buffer Size)
 */
#define WS_BUFFER_SIZE      ${ws_buffer}    /**< WebSocket frame buffer size (bytes) */
#define WS_MAX_CLIENTS      ${ws_clients}       /**< Maximum concurrent WebSocket connections */
#define WS_TIMEOUT_MS       ${ws_timeout}    /**< WebSocket frame timeout (milliseconds) */

/*
 * LED Configuration
 * Source: ADR-003 (LED Count Standardization)
 */
#define LED_COUNT           ${led_count}     /**< Number of addressable LEDs */
#define LED_TYPE_WS2812B    1        /**< LED type: WS2812B */
#define LED_FPS_TARGET      ${led_fps}      /**< Target frame rate (frames per second) */

/*
 * Pattern Configuration
 * Source: ADR-004 (Pattern Maximum Size)
 */
#define PATTERN_MAX_SIZE    ${pattern_size}  /**< Maximum pattern file size (bytes) - 256KB */
#define PATTERN_MIN_COUNT   ${pattern_count}      /**< Minimum patterns that must fit in storage */
#define STORAGE_RESERVED    ${storage_reserved}  /**< Reserved storage space (bytes) - 100KB safety margin */

/*
 * Storage Configuration
 * Source: ADR-005 (Storage Mount Path)
 */
#define STORAGE_MOUNT_PATH  "${mount_path}"    /**< LittleFS VFS mount point */
#define STORAGE_TYPE        "littlefs"      /**< Filesystem type */
#define STORAGE_LABEL       "littlefs"      /**< Partition label */

#endif /* PRISM_CONFIG_H */
EOF

echo -e "  ${GREEN}✓${NC} Generated: $CONFIG_H"

# Update sdkconfig.defaults (if needed)
echo -e "${BLUE}[3/3] Checking sdkconfig.defaults...${NC}"
SDKCONFIG="$FIRMWARE_DIR/sdkconfig.defaults"

if [ -f "$SDKCONFIG" ]; then
    current_ws_buffer=$(grep "CONFIG_WS_BUFFER_SIZE=" "$SDKCONFIG" | sed 's/CONFIG_WS_BUFFER_SIZE=//')

    if [ "$current_ws_buffer" != "$ws_buffer" ]; then
        echo -e "  ${YELLOW}⚠${NC} Updating CONFIG_WS_BUFFER_SIZE: $current_ws_buffer → $ws_buffer"

        # Platform-specific sed
        if [[ "$OSTYPE" == "darwin"* ]]; then
            sed -i '' "s/CONFIG_WS_BUFFER_SIZE=.*/CONFIG_WS_BUFFER_SIZE=$ws_buffer/" "$SDKCONFIG"
        else
            sed -i "s/CONFIG_WS_BUFFER_SIZE=.*/CONFIG_WS_BUFFER_SIZE=$ws_buffer/" "$SDKCONFIG"
        fi

        echo -e "  ${GREEN}✓${NC} Updated $SDKCONFIG"
    else
        echo -e "  ${GREEN}✓${NC} $SDKCONFIG already matches CANON"
    fi
else
    echo -e "  ${YELLOW}⚠${NC} $SDKCONFIG not found (skipping)"
fi

# Summary
echo ""
echo -e "${GREEN}✓ Code generation complete${NC}"
echo ""
echo "Generated files:"
echo "  • $CONFIG_H"
echo ""
echo "Next steps:"
echo "  1. Review generated files"
echo "  2. Run: ./scripts/validate-canon.sh"
echo "  3. Build firmware to verify"
echo "  4. Commit changes with message: 'sync: Update code from CANON'"
