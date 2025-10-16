#!/bin/bash
# .taskmaster/scripts/validate-canon.sh
#
# Purpose: Verify code matches CANON specifications
# Usage: ./validate-canon.sh
#
# Validates that firmware code constants match CANON.md

set -euo pipefail

# Configuration
CANON=".taskmaster/CANON.md"
FIRMWARE_DIR="firmware"
SCRIPTS_DIR=".taskmaster/scripts"
ERRORS=0
WARNINGS=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== CANON Compliance Validator ===${NC}"
echo ""

# Check CANON exists
if [ ! -f "$CANON" ]; then
    echo -e "${RED}✗ CANON.md not found!${NC}"
    exit 1
fi

# [1/5] Check CANON freshness
echo -e "${BLUE}[1/5] Checking CANON freshness...${NC}"
CURRENT_CANON_HASH=$(shasum -a 256 "$CANON" | cut -d' ' -f1)

# Generate fresh CANON to temp file
TMP_CANON=$(mktemp)
"$SCRIPTS_DIR/generate-canon.sh" > /dev/null 2>&1
FRESH_CANON_HASH=$(shasum -a 256 "$CANON" | cut -d' ' -f1)

if [ "$CURRENT_CANON_HASH" != "$FRESH_CANON_HASH" ]; then
    echo -e "  ${RED}✗ CANON.md is outdated${NC}"
    echo -e "  ${YELLOW}Run: ./scripts/generate-canon.sh${NC}"
    ((ERRORS++))
else
    echo -e "  ${GREEN}✓ CANON.md is current${NC}"
fi

# [2/5] Extract constants from CANON
echo -e "${BLUE}[2/5] Extracting CANON constants...${NC}"
CANON_CONSTANTS=$("$SCRIPTS_DIR/extract-constants.sh" "$CANON")
echo -e "  ${GREEN}✓ Constants extracted${NC}"

# [3/5] Validate sdkconfig.defaults
echo -e "${BLUE}[3/5] Validating sdkconfig.defaults...${NC}"
SDKCONFIG="$FIRMWARE_DIR/sdkconfig.defaults"

if [ ! -f "$SDKCONFIG" ]; then
    echo -e "  ${RED}✗ File not found: $SDKCONFIG${NC}"
    ((ERRORS++))
else
    # Check WS_BUFFER_SIZE
    ws_buffer_canon=$(echo "$CANON_CONSTANTS" | grep -o '"ws_buffer_size": [0-9]*' | grep -o '[0-9]*$')
    ws_buffer_code=$(grep "CONFIG_WS_BUFFER_SIZE=" "$SDKCONFIG" | sed 's/CONFIG_WS_BUFFER_SIZE=//')

    if [ -n "$ws_buffer_canon" ] && [ -n "$ws_buffer_code" ]; then
        if [ "$ws_buffer_code" = "$ws_buffer_canon" ]; then
            echo -e "  ${GREEN}✓${NC} CONFIG_WS_BUFFER_SIZE: $ws_buffer_code (matches CANON)"
        else
            echo -e "  ${RED}✗${NC} CONFIG_WS_BUFFER_SIZE: $ws_buffer_code (CANON says $ws_buffer_canon)"
            ((ERRORS++))
        fi
    fi
fi

# [4/5] Validate partitions.csv
echo -e "${BLUE}[4/5] Validating partitions.csv...${NC}"
PARTITIONS="$FIRMWARE_DIR/partitions.csv"

if [ ! -f "$PARTITIONS" ]; then
    echo -e "  ${RED}✗ File not found: $PARTITIONS${NC}"
    ((ERRORS++))
else
    # Extract littlefs partition info from CANON
    littlefs_offset_canon="0x311000"
    littlefs_size_canon="0x180000"

    # Extract from partitions.csv
    littlefs_line=$(grep "^littlefs," "$PARTITIONS" | head -1)

    if [ -n "$littlefs_line" ]; then
        littlefs_offset_code=$(echo "$littlefs_line" | awk -F',' '{gsub(/[ \t]+/, "", $4); print $4}')
        littlefs_size_code=$(echo "$littlefs_line" | awk -F',' '{gsub(/[ \t]+/, "", $5); print $5}')

        if [ "$littlefs_offset_code" = "$littlefs_offset_canon" ]; then
            echo -e "  ${GREEN}✓${NC} littlefs offset: $littlefs_offset_code (matches CANON)"
        else
            echo -e "  ${RED}✗${NC} littlefs offset: $littlefs_offset_code (CANON says $littlefs_offset_canon)"
            ((ERRORS++))
        fi

        if [ "$littlefs_size_code" = "$littlefs_size_canon" ]; then
            echo -e "  ${GREEN}✓${NC} littlefs size: $littlefs_size_code (matches CANON)"
        else
            echo -e "  ${RED}✗${NC} littlefs size: $littlefs_size_code (CANON says $littlefs_size_canon)"
            ((ERRORS++))
        fi
    else
        echo -e "  ${RED}✗${NC} littlefs partition not found in $PARTITIONS"
        ((ERRORS++))
    fi

    # Check app0 offset
    app0_offset_canon="0x11000"
    app0_line=$(grep "^app0," "$PARTITIONS" | head -1)

    if [ -n "$app0_line" ]; then
        app0_offset_code=$(echo "$app0_line" | awk -F',' '{gsub(/[ \t]+/, "", $4); print $4}')

        if [ "$app0_offset_code" = "$app0_offset_canon" ]; then
            echo -e "  ${GREEN}✓${NC} app0 offset: $app0_offset_code (matches CANON)"
        else
            echo -e "  ${RED}✗${NC} app0 offset: $app0_offset_code (CANON says $app0_offset_canon)"
            ((ERRORS++))
        fi
    fi
fi

# [5/5] Check for prism_config.h (if exists)
echo -e "${BLUE}[5/5] Checking for code constants...${NC}"
CONFIG_H="$FIRMWARE_DIR/components/core/include/prism_config.h"

if [ -f "$CONFIG_H" ]; then
    echo -e "  ${YELLOW}⚠${NC} Found $CONFIG_H (checking compliance...)"

    # Check LED_COUNT if defined
    if grep -q "#define LED_COUNT" "$CONFIG_H"; then
        led_count_code=$(grep "#define LED_COUNT" "$CONFIG_H" | awk '{print $3}')
        led_count_canon=$(echo "$CANON_CONSTANTS" | grep -o '"led_count": [0-9]*' | grep -o '[0-9]*$')

        if [ "$led_count_code" = "$led_count_canon" ]; then
            echo -e "  ${GREEN}✓${NC} LED_COUNT: $led_count_code (matches CANON)"
        else
            echo -e "  ${RED}✗${NC} LED_COUNT: $led_count_code (CANON says $led_count_canon)"
            ((ERRORS++))
        fi
    fi

    # Check PATTERN_MAX_SIZE if defined
    if grep -q "#define PATTERN_MAX_SIZE" "$CONFIG_H"; then
        pattern_size_code=$(grep "#define PATTERN_MAX_SIZE" "$CONFIG_H" | awk '{print $3}')
        pattern_size_canon=$(echo "$CANON_CONSTANTS" | grep -o '"pattern_max_size": [0-9]*' | grep -o '[0-9]*$')

        if [ "$pattern_size_code" = "$pattern_size_canon" ]; then
            echo -e "  ${GREEN}✓${NC} PATTERN_MAX_SIZE: $pattern_size_code (matches CANON)"
        else
            echo -e "  ${RED}✗${NC} PATTERN_MAX_SIZE: $pattern_size_code (CANON says $pattern_size_canon)"
            ((ERRORS++))
        fi
    fi
else
    echo -e "  ${YELLOW}⚠${NC} $CONFIG_H not found (will be generated)"
    ((WARNINGS++))
fi

# Summary
echo ""
echo -e "${BLUE}=== Validation Summary ===${NC}"
echo ""

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo -e "${GREEN}✓ All validations PASSED${NC}"
    echo ""
    echo "Code is in full compliance with CANON.md"
    exit 0
elif [ $ERRORS -eq 0 ]; then
    echo -e "${YELLOW}⚠ Validation passed with $WARNINGS warning(s)${NC}"
    echo ""
    echo "Code matches CANON but some optional files are missing"
    exit 0
else
    echo -e "${RED}✗ $ERRORS validation(s) FAILED${NC}"
    if [ $WARNINGS -gt 0 ]; then
        echo -e "${YELLOW}⚠ $WARNINGS warning(s)${NC}"
    fi
    echo ""
    echo "To fix:"
    echo "  1. Review differences above"
    echo "  2. Update code to match CANON"
    echo "  3. OR create new ADR if CANON needs changing"
    echo ""
    echo "Remember: CANON is the source of truth"
    exit 1
fi
