#!/bin/bash
# .taskmaster/scripts/create-adr.sh
#
# Purpose: Interactive ADR creation wizard
# Usage: ./create-adr.sh
#
# Creates new ADR from template with auto-numbered filename

set -euo pipefail

# Configuration
DECISIONS_DIR=".taskmaster/decisions"
TEMPLATE="$DECISIONS_DIR/000-adr-template.md"
RESEARCH_DIR=".taskmaster/research/[VALIDATED]"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== ADR Creation Wizard ===${NC}"
echo ""

# Check template exists
if [ ! -f "$TEMPLATE" ]; then
    echo -e "${RED}Error: Template not found: $TEMPLATE${NC}"
    exit 1
fi

# Get next ADR number
LAST_ADR=$(find "$DECISIONS_DIR" -name "[0-9][0-9][0-9]-*.md" | sort | tail -1 | sed 's/.*\/\([0-9]*\).*/\1/' || echo "005")
NEXT_NUM=$(printf "%03d" $((10#$LAST_ADR + 1)))

echo -e "${GREEN}Next ADR number: $NEXT_NUM${NC}"
echo ""

# Interactive prompts
read -p "Decision title: " TITLE
if [ -z "$TITLE" ]; then
    echo -e "${RED}Error: Title cannot be empty${NC}"
    exit 1
fi

read -p "Author name (default: Agent): " AUTHOR
AUTHOR=${AUTHOR:-Agent}

# Show available validated research
echo ""
echo -e "${YELLOW}Available validated research:${NC}"
if [ -d "$RESEARCH_DIR" ]; then
    research_files=$(find "$RESEARCH_DIR" -name "*.md" 2>/dev/null || echo "")
    if [ -n "$research_files" ]; then
        i=1
        declare -a research_array
        while IFS= read -r file; do
            research_array[$i]=$(basename "$file")
            echo "  $i. ${research_array[$i]}"
            ((i++))
        done <<< "$research_files"
        echo "  0. None / Manual ADR"
        echo ""
        read -p "Select research source (number): " RESEARCH_NUM

        if [ "$RESEARCH_NUM" -gt 0 ] && [ "$RESEARCH_NUM" -lt ${#research_array[@]} ]; then
            RESEARCH="${research_array[$RESEARCH_NUM]}"
        else
            RESEARCH="manual"
        fi
    else
        echo "  (No validated research found)"
        RESEARCH="manual"
    fi
else
    RESEARCH="manual"
fi

# Generate filename from title
FILENAME_SLUG=$(echo "$TITLE" | tr '[:upper:]' '[:lower:]' | tr ' ' '-' | tr -cd '[:alnum:]-')
FILENAME="$DECISIONS_DIR/${NEXT_NUM}-${FILENAME_SLUG}.md"

echo ""
echo -e "${GREEN}Creating ADR:${NC}"
echo "  Number: $NEXT_NUM"
echo "  Title: $TITLE"
echo "  Author: $AUTHOR"
echo "  Research: $RESEARCH"
echo "  File: $(basename $FILENAME)"
echo ""

# Copy template and populate
cp "$TEMPLATE" "$FILENAME"

# Get current date
CURRENT_DATE=$(date +%Y-%m-%d)

# Platform-specific sed (macOS vs Linux)
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    sed -i '' "s/XXX/$NEXT_NUM/g" "$FILENAME"
    sed -i '' "s/\[Decision Title\]/$TITLE/g" "$FILENAME"
    sed -i '' "s/\[Name\]/$AUTHOR/g" "$FILENAME"
    sed -i '' "s/YYYY-MM-DD/$CURRENT_DATE/g" "$FILENAME"
    sed -i '' "s/PROPOSED/DRAFT/g" "$FILENAME"

    if [ "$RESEARCH" != "manual" ]; then
        sed -i '' "s|- \[VALIDATED\] research.*|- [VALIDATED] research/[VALIDATED]/$RESEARCH|" "$FILENAME"
    fi
else
    # Linux
    sed -i "s/XXX/$NEXT_NUM/g" "$FILENAME"
    sed -i "s/\[Decision Title\]/$TITLE/g" "$FILENAME"
    sed -i "s/\[Name\]/$AUTHOR/g" "$FILENAME"
    sed -i "s/YYYY-MM-DD/$CURRENT_DATE/g" "$FILENAME"
    sed -i "s/PROPOSED/DRAFT/g" "$FILENAME"

    if [ "$RESEARCH" != "manual" ]; then
        sed -i "s|- \[VALIDATED\] research.*|- [VALIDATED] research/[VALIDATED]/$RESEARCH|" "$FILENAME"
    fi
fi

echo -e "${GREEN}✓ ADR created successfully${NC}"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo "  1. Edit the ADR file with decision details"
echo "  2. Fill in all sections:"
echo "     - Context (what situation requires a decision?)"
echo "     - Research Evidence (cite validated research)"
echo "     - Decision (clear and unambiguous)"
echo "     - Alternatives Considered (with pros/cons)"
echo "     - Consequences (positive/negative/neutral)"
echo "     - Validation Criteria"
echo "     - Implementation details"
echo "  3. Get Captain approval"
echo "  4. Update status to APPROVED"
echo "  5. Run: ./scripts/generate-canon.sh"
echo ""
echo -e "${BLUE}Open the file now? [y/N]${NC}"
read -r OPEN

if [ "$OPEN" = "y" ] || [ "$OPEN" = "Y" ]; then
    ${EDITOR:-nano} "$FILENAME"
fi

echo ""
echo "ADR file: $FILENAME"
