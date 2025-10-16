#!/bin/bash
# .taskmaster/scripts/generate-canon.sh
#
# Purpose: Generate CANON.md from approved ADRs
# Usage: ./generate-canon.sh
#
# This script is the ONLY way CANON.md should be modified.
# Manual edits to CANON.md will be overwritten.

set -euo pipefail

# Configuration
DECISIONS_DIR=".taskmaster/decisions"
OUTPUT_FILE=".taskmaster/CANON.md"
TIMESTAMP=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
VERSION="1.0.0"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== CANON Generator v${VERSION} ==="
echo "Timestamp: $TIMESTAMP"
echo ""

# Check decisions directory exists
if [ ! -d "$DECISIONS_DIR" ]; then
    echo -e "${RED}Error: Decisions directory not found: $DECISIONS_DIR${NC}"
    exit 1
fi

# Collect all approved ADRs
adr_files=$(find "$DECISIONS_DIR" -name "[0-9][0-9][0-9]-*.md" | sort)
adr_count=0
adr_list=""

echo "Scanning for approved ADRs..."
for adr in $adr_files; do
    if grep -q "^\*\*Status:\*\* APPROVED" "$adr"; then
        echo -e "${GREEN}✓${NC} $(basename $adr)"
        ((adr_count++))
        adr_list="$adr_list $(basename $adr)"
    else
        echo -e "${YELLOW}⊘${NC} $(basename $adr) (not approved)"
    fi
done

if [ $adr_count -eq 0 ]; then
    echo -e "${RED}Error: No approved ADRs found!${NC}"
    exit 1
fi

echo ""
echo "Found $adr_count approved ADR(s)"
echo ""

# Generate CANON.md
echo "Generating CANON.md..."

# Header
cat > "$OUTPUT_FILE" << EOF
# PRISM K1 CANONICAL SPECIFICATIONS
*Single Source of Truth - Generated from Architecture Decision Records*
*DO NOT EDIT MANUALLY - Changes will be overwritten*

> **AUTHORITY:** This document is the authoritative specification for PRISM K1.
> All code, tests, and documentation MUST match these specifications.
>
> **GENERATED:** $TIMESTAMP
> **FROM ADRs:** $adr_list
> **VALIDATION:** Automated validation required before code changes

---

## Table of Contents

EOF

# First pass: Build TOC
for adr in $adr_files; do
    if grep -q "^\*\*Status:\*\* APPROVED" "$adr"; then
        # Extract title from "# ADR-XXX: Title" format
        title=$(grep "^# ADR-" "$adr" | head -1 | sed 's/^# ADR-[0-9]*: //')
        number=$(grep "^# ADR-" "$adr" | head -1 | sed 's/^# ADR-\([0-9]*\).*/\1/' | sed 's/^0*//')

        # Create anchor-safe title
        anchor=$(echo "$title" | tr '[:upper:]' '[:lower:]' | tr ' ' '-' | tr -cd '[:alnum:]-')

        echo "- [$number. $title](#$number-$anchor)" >> "$OUTPUT_FILE"
    fi
done

echo -e "\n---\n" >> "$OUTPUT_FILE"

# Second pass: Extract decision content
for adr in $adr_files; do
    if grep -q "^\*\*Status:\*\* APPROVED" "$adr"; then
        title=$(grep "^# ADR-" "$adr" | head -1 | sed 's/^# ADR-[0-9]*: //')
        number=$(grep "^# ADR-" "$adr" | head -1 | sed 's/^# ADR-\([0-9]*\).*/\1/' | sed 's/^0*//')
        date=$(grep "^\*\*Date:\*\* " "$adr" | head -1 | sed 's/^\*\*Date:\*\* //')

        cat >> "$OUTPUT_FILE" << EOF
## $number. $title
*Source: [$(basename $adr)](decisions/$(basename $adr))*
*Status: APPROVED*
*Last Updated: $date*

EOF

        # Extract the Decision section
        in_decision=0
        in_yaml=0
        yaml_content=""
        decision_content=""

        while IFS= read -r line; do
            # Start of Decision section
            if [[ "$line" == "## Decision" ]]; then
                in_decision=1
                continue
            fi

            # End of Decision section (next ## heading)
            if [[ "$line" =~ ^##\  ]] && [ $in_decision -eq 1 ] && [[ "$line" != "## Decision" ]]; then
                in_decision=0
                break
            fi

            # Capture decision content
            if [ $in_decision -eq 1 ]; then
                # Check for YAML block start
                if [[ "$line" == '```yaml' ]]; then
                    in_yaml=1
                    continue
                fi

                # Check for YAML block end
                if [[ "$line" == '```' ]] && [ $in_yaml -eq 1 ]; then
                    in_yaml=0
                    continue
                fi

                # Capture YAML content
                if [ $in_yaml -eq 1 ]; then
                    yaml_content="$yaml_content$line"$'\n'
                else
                    decision_content="$decision_content$line"$'\n'
                fi
            fi
        done < "$adr"

        # Output decision text
        echo "### Specification" >> "$OUTPUT_FILE"
        echo "" >> "$OUTPUT_FILE"
        echo "$decision_content" >> "$OUTPUT_FILE"

        # Output machine-readable YAML if present
        if [ -n "$yaml_content" ]; then
            echo "### Machine-Readable" >> "$OUTPUT_FILE"
            echo "" >> "$OUTPUT_FILE"
            echo '```yaml' >> "$OUTPUT_FILE"
            echo "$yaml_content" >> "$OUTPUT_FILE"
            echo '```' >> "$OUTPUT_FILE"
        fi

        echo "**Rationale:** See [$(basename $adr)](decisions/$(basename $adr)) for full context and alternatives considered." >> "$OUTPUT_FILE"
        echo "" >> "$OUTPUT_FILE"
        echo "---" >> "$OUTPUT_FILE"
        echo "" >> "$OUTPUT_FILE"
    fi
done

# Footer with validation status
cat >> "$OUTPUT_FILE" << EOF

## Validation Status

This document validated against:
- **ADRs:** ✓ ($adr_count decisions processed)
- **Code constants:** (Run validate-canon.sh)
- **Partition table:** (Run validate-canon.sh)

**Last Validation:** $TIMESTAMP
**Generator:** canon-generator v$VERSION
**Status:** GENERATED (validation pending)

---

## Change History

| Date | ADR | Change Summary |
|------|-----|----------------|
EOF

# Extract change history from each ADR
for adr in $adr_files; do
    if grep -q "^\*\*Status:\*\* APPROVED" "$adr"; then
        number=$(grep "^# ADR-" "$adr" | head -1 | sed 's/^# ADR-\([0-9]*\).*/\1/')
        date=$(grep "^\*\*Date:\*\* " "$adr" | head -1 | sed 's/^\*\*Date:\*\* //')
        title=$(grep "^# ADR-" "$adr" | head -1 | sed 's/^# ADR-[0-9]*: //')

        echo "| $date | ADR-$number | $title |" >> "$OUTPUT_FILE"
    fi
done

# Document signature
cat >> "$OUTPUT_FILE" << EOF

---

**Document Signature:**
\`\`\`
Generated: $TIMESTAMP
Source: ADR-001 through ADR-$(printf "%03d" $adr_count)
Generator: canon-generator v$VERSION
SHA256: $(shasum -a 256 "$OUTPUT_FILE" 2>/dev/null | cut -d' ' -f1 || echo "N/A")
\`\`\`
EOF

echo -e "${GREEN}✓ CANON.md generated successfully${NC}"
echo "  ADRs processed: $adr_count"
echo "  Output: $OUTPUT_FILE"
echo ""
echo "Next steps:"
echo "  1. Review CANON.md for accuracy"
echo "  2. Run validate-canon.sh to check code compliance"
echo "  3. Commit CANON.md with message: 'Generated from ADRs (auto)'"
