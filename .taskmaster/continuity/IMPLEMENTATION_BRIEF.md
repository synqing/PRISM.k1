# PRISM K1 KNOWLEDGE FORTRESS IMPLEMENTATION
## CONTINUITY BRIEFING FOR SUCCESSOR AGENT

**Mission:** Implement a bulletproof knowledge management system to prevent specification drift and conflicting documentation that nearly destroyed the project.

**Status:** ✅ COMPLETE - All 4 phases operational
**Priority:** OPERATIONAL - System active and enforced
**Timeline:** Completed 2025-10-15 (Phases 1-4 in 1 week)
**Current Progress:** 100% (All phases complete, system validated)

---

## CRISIS CONTEXT: Why This Exists

### The Problem We're Solving

Three agents created three conflicting "source of truth" documents during the design phase, introducing contradictions that would have caused integration failures:

| Specification | PRD Value | AUTH_SPEC Value | Actual Code | Impact |
|---------------|-----------|-----------------|-------------|---------|
| WS_BUFFER_SIZE | 8192 | 4096 | 8192 | Buffer overflow risk |
| LED_COUNT | 150 | 320 | Not defined | Pattern incompatibility |
| PATTERN_MAX_SIZE | 200KB | 256KB | Not defined | Upload rejection |
| LittleFS Offset | None | 0x311000 | 0x320000 | Partition mismatch |

**Root Cause:** No governance system for research → specification → code pipeline. Agents created specifications independently without:
- Validation framework
- Conflict resolution process  
- Authority to declare truth
- Automated drift prevention
- Audit trail

### What We're Building

A military-grade knowledge management system called **The Knowledge Fortress** with:

1. **Single Source of Truth (CANON.md)** - Auto-generated from approved decisions, never edited manually
2. **Architecture Decision Records (ADRs)** - Immutable decision trail with evidence and rationale
3. **Research Validation Framework** - RFC-style peer review before research becomes truth
4. **Automated Enforcement** - CI/CD prevents code that contradicts CANON
5. **Audit Trail** - Every decision timestamped and signed

---

## SYSTEM ARCHITECTURE

```
.taskmaster/
├── CANON.md                    ← ONLY SOURCE OF TRUTH (auto-generated)
│
├── decisions/                  ← ADRs (Architecture Decision Records)
│   ├── 000-adr-template.md    ← Template for new ADRs
│   ├── 001-partition-table.md
│   ├── 002-websocket-buffer.md
│   └── 003-led-count.md
│
├── research/                   ← Research lifecycle states
│   ├── [PROPOSED]/            ← Unvalidated research
│   ├── [IN_REVIEW]/           ← Peer review in progress
│   ├── [VALIDATED]/           ← Passed validation, ready for ADR
│   ├── [REJECTED]/            ← Failed validation
│   └── [ARCHIVED]/            ← Superseded research
│
├── specs/                      ← DERIVED specifications (generated)
│   ├── firmware/
│   │   ├── partition-table.md
│   │   ├── memory-layout.md
│   │   └── constants.h        ← Auto-generated C header
│   ├── protocol/
│   │   └── websocket-spec.md
│   └── format/
│       └── pattern-format.md
│
├── audit/                      ← Immutable audit trail
│   └── decision_log.jsonl     ← Every decision logged
│
├── continuity/                 ← You are here
│   └── IMPLEMENTATION_BRIEF.md
│
└── scripts/                    ← Automation tools
    ├── generate-canon.sh
    ├── validate-canon.sh
    ├── create-adr.sh
    └── [... see TOOLING section]
```

---

## IMPLEMENTATION ROADMAP

### Phase 1: Emergency Triage (2-4 hours) - HIGHEST PRIORITY

**Goal:** Stop the bleeding - create ADRs for existing conflicts and fix code

**Steps:**

1. **Create ADR directory structure**
   ```bash
   mkdir -p .taskmaster/decisions
   mkdir -p .taskmaster/research/{[PROPOSED],[IN_REVIEW],[VALIDATED],[REJECTED],[ARCHIVED]}
   mkdir -p .taskmaster/specs/{firmware,protocol,format}
   mkdir -p .taskmaster/audit
   ```

2. **Create ADR template** (see ADR TEMPLATE section below)

3. **Create 5 Critical ADRs** (use template):
   - ADR-001: Partition Table Layout
   - ADR-002: WebSocket Buffer Size  
   - ADR-003: LED Count
   - ADR-004: Pattern Max Size
   - ADR-005: Storage Mount Path

4. **Generate initial CANON.md** from ADRs

5. **Fix code conflicts:**
   - `firmware/sdkconfig.defaults`: WS_BUFFER_SIZE 8192 → 4096
   - `firmware/partitions.csv`: offset 0x320000 → 0x311000

6. **Validate:** Run checks to ensure code matches CANON

**Deliverables:**
- [ ] 5 ADRs created and approved
- [ ] CANON.md generated
- [ ] Code conflicts fixed
- [ ] Validation passes

---

### Phase 2: Research Reorganization (1 day)

**Goal:** Classify and validate existing research documents

**Steps:**

1. **Move existing research to [PROPOSED] state**
   ```bash
   mv .taskmaster/docs/research/*.md .taskmaster/research/[PROPOSED]/
   ```

2. **Add peer review metadata** to each research document:
   ```yaml
   ---
   title: ESP32 Constraints Research
   status: PROPOSED
   author: Research Agent
   date: 2025-10-15
   reviewers:
     - agent_a: PENDING
     - agent_b: PENDING
     - agent_c: PENDING
   ---
   ```

3. **Conduct peer review** (see PEER REVIEW PROCESS below)

4. **Move validated research** to [VALIDATED] folder

5. **Convert validated research to ADR drafts**

**Deliverables:**
- [ ] All research classified by state
- [ ] Peer reviews completed
- [ ] 3+ additional ADRs from validated research

---

### Phase 3: Automation Setup (2-3 days)

**Goal:** Implement automated generation and validation

**Steps:**

1. **Create core scripts** (see TOOLING section):
   - `generate-canon.sh` - Build CANON.md from ADRs
   - `validate-canon.sh` - Check code vs CANON
   - `create-adr.sh` - New ADR wizard
   - `extract-constants.sh` - Pull constants from CANON
   - `sync-code-to-canon.sh` - Update code from CANON

2. **Setup Git hooks**:
   - Pre-commit: Validate no manual CANON edits
   - Pre-push: Ensure code matches CANON

3. **Configure CI/CD**:
   - Add `.github/workflows/canon-validation.yml`
   - Fail builds if CANON validation fails

4. **Add code generation**:
   - Generate `prism_config.h` from CANON
   - Generate partition table from CANON

**Deliverables:**
- [ ] All scripts operational
- [ ] Git hooks installed
- [ ] CI/CD pipeline active
- [ ] Code generation working

---

### Phase 4: Documentation & Training (2 days)

**Goal:** Document the system and train future agents

**Steps:**

1. **Create comprehensive docs**:
   - `METHODOLOGY.md` - Research process
   - `ADR_GUIDE.md` - How to write ADRs
   - `VALIDATION_GUIDE.md` - How to validate

2. **Create agent rules**:
   - `.taskmaster/agent-rules.yml`
   - Add to cursor rules
   - Add to `.agent/instructions.md`

3. **Create examples**:
   - Example research proposal
   - Example ADR with peer review
   - Example conflict resolution

4. **Update project README**:
   - Explain Knowledge Fortress
   - Link to key documents
   - Onboarding for new agents

**Deliverables:**
- [ ] 3+ documentation files
- [ ] Agent rules configured
- [ ] Examples created
- [ ] README updated

---

## DETAILED SPECIFICATIONS

### ADR TEMPLATE

```markdown
# ADR-XXX: [Decision Title]

**Status:** [PROPOSED | APPROVED | REJECTED | SUPERSEDED]  
**Date:** YYYY-MM-DD  
**Decided By:** Captain SpectraSynq  
**Supersedes:** [ADR number or "None"]  
**Superseded By:** [ADR number or "None"]  

## Context

[What situation requires a decision?]
[What are the constraints?]
[What are the requirements?]

## Research Evidence

- [VALIDATED] research/[VALIDATED]/filename.md
- [MEASUREMENT] Direct measurement on hardware
- [CITATION] External documentation (ESP-IDF docs, etc.)

## Decision

[The decision made - clear and unambiguous]

```yaml
# Machine-readable decision
key: value
setting: value
```

## Alternatives Considered

### Alternative 1: [Name]
**Pros:**
- Pro 1
- Pro 2

**Cons:**
- Con 1
- Con 2

**Verdict:** REJECTED - [Reason]

### Alternative 2: [Name]
[Same structure]

## Consequences

### Positive
- Benefit 1
- Benefit 2

### Negative  
- Drawback 1
- Drawback 2

### Neutral
- Unchangeable fact 1
- Unchangeable fact 2

## Validation Criteria

- [ ] Criterion 1
- [ ] Criterion 2
- [ ] Criterion 3

## Implementation

### Code Changes Required
```
file/path.c: Change X to Y
file/path.h: Add constant Z
```

### Documentation Updates
```
CANON.md: Auto-updated
specs/firmware/doc.md: Regenerated
```

### Tests Required
```
test_X.py: Verify behavior Y
```

## Audit Trail

- **Proposed by:** [Agent name]
- **Reviewed by:** [Agent names, 2/3 minimum]
- **Approved by:** Captain SpectraSynq
- **Implemented:** YYYY-MM-DD
- **Validated:** YYYY-MM-DD

---
**IMMUTABLE AFTER APPROVAL**  
To change this decision, create new ADR referencing this one.
```

---

### CANON.MD STRUCTURE

```markdown
# PRISM K1 CANONICAL SPECIFICATIONS
*Auto-generated from Architecture Decision Records*
*DO NOT EDIT MANUALLY - Changes will be overwritten*

> **AUTHORITY:** This document is the single source of truth for PRISM K1.
> All code, tests, and documentation MUST match these specifications.
> 
> **GENERATED:** YYYY-MM-DD HH:MM:SS UTC  
> **FROM ADRs:** [list of ADR numbers]  
> **SIGNATURE:** SHA256:[hash]

---

## Table of Contents
- [1. Partition Table](#1-partition-table)
- [2. Memory Configuration](#2-memory-configuration)
- [3. WebSocket Protocol](#3-websocket-protocol)
- [4. LED Configuration](#4-led-configuration)
- [5. Storage Configuration](#5-storage-configuration)
- [... etc ...]

---

## 1. Partition Table
*Source: ADR-001*  
*Status: APPROVED*  
*Last Updated: YYYY-MM-DD*

### Specification
```csv
# Name,   Type, SubType, Offset,   Size,     Flags
nvs,      data, nvs,     0x9000,   0x6000,
otadata,  data, ota,     0xF000,   0x2000,
app0,     app,  ota_0,   0x11000,  0x180000,  # 1.5MB
app1,     app,  ota_1,   0x191000, 0x180000,  # 1.5MB
littlefs, data, 0x82,    0x311000, 0x180000,  # 1.5MB
```

### Machine-Readable
```json
{
  "partitions": [
    {"name": "nvs", "type": "data", "subtype": "nvs", "offset": "0x9000", "size": "0x6000"},
    {"name": "otadata", "type": "data", "subtype": "ota", "offset": "0xF000", "size": "0x2000"},
    {"name": "app0", "type": "app", "subtype": "ota_0", "offset": "0x11000", "size": "0x180000"},
    {"name": "app1", "type": "app", "subtype": "ota_1", "offset": "0x191000", "size": "0x180000"},
    {"name": "littlefs", "type": "data", "subtype": "0x82", "offset": "0x311000", "size": "0x180000"}
  ]
}
```

### Rationale
See ADR-001 for full context and alternatives considered.

---

## 2. Memory Configuration
*Source: ADR-002*  
*Status: APPROVED*

### Constants
```c
#define WS_BUFFER_SIZE      4096    // WebSocket frame buffer
#define WS_MAX_CLIENTS      2       // Max concurrent connections
#define PATTERN_CACHE_SIZE  60KB    // Hot pattern cache
#define LED_BUFFER_SIZE     2KB     // Double buffered
```

### Machine-Readable
```json
{
  "memory": {
    "ws_buffer_size": 4096,
    "ws_max_clients": 2,
    "pattern_cache_size": 61440,
    "led_buffer_size": 2048
  }
}
```

---

[... Continue for all specifications ...]

---

## Validation

This document has been validated against:
- **ADRs:** ✓ (5 decisions processed)
- **Code constants:** ✓ (firmware/include/prism_config.h matches)
- **Partition table:** ✓ (firmware/partitions.csv matches)
- **Tests:** ✓ (test expectations match)

**Last Validation:** YYYY-MM-DD HH:MM:SS UTC  
**Validator:** automated-canon-validator v1.0.0  
**Status:** PASSING ✓

---

## Change History

| Date | ADR | Change Summary |
|------|-----|----------------|
| 2025-10-15 | ADR-001 | Initial partition table |
| 2025-10-15 | ADR-002 | WebSocket buffer size |
| 2025-10-15 | ADR-003 | LED count standardized |

---

**Signature Verification:**
```
SHA256: [hash of entire document]
Signed by: canon-generator v1.0.0
Timestamp: YYYY-MM-DD HH:MM:SS UTC
```
```

---

### PEER REVIEW PROCESS

**Purpose:** Ensure research quality before it becomes an ADR

**Roles:**
- **Author:** Agent who conducted research
- **Reviewer A (Reproduction):** Verify methodology
- **Reviewer B (Challenge):** Question assumptions
- **Reviewer C (Integration):** Check for conflicts

**Process:**

1. **Author submits research** to [PROPOSED]

2. **Assign reviewers** (can be automated or manual)

3. **Each reviewer evaluates:**

   **Reviewer A Checklist:**
   ```markdown
   - [ ] Methodology clearly documented
   - [ ] Test procedures reproducible
   - [ ] Raw data provided
   - [ ] Calculations verified
   - [ ] Tools/equipment listed
   
   **Status:** APPROVE / NEEDS_REVISION / REJECT
   **Comments:** [detailed feedback]
   ```

   **Reviewer B Checklist:**
   ```markdown
   - [ ] Assumptions explicitly stated
   - [ ] Alternative explanations considered
   - [ ] Edge cases examined
   - [ ] Limitations acknowledged
   - [ ] Bias detection performed
   
   **Status:** APPROVE / NEEDS_REVISION / REJECT
   **Comments:** [detailed feedback]
   ```

   **Reviewer C Checklist:**
   ```markdown
   - [ ] Conflicts with existing research identified
   - [ ] Impact on other decisions assessed
   - [ ] Superseded research marked
   - [ ] Integration path clear
   - [ ] Dependencies resolved
   
   **Status:** APPROVE / NEEDS_REVISION / REJECT
   **Comments:** [detailed feedback]
   ```

4. **Require 2/3 approval** to move to [VALIDATED]

5. **If NEEDS_REVISION:** Author updates, restart review

6. **If REJECT:** Move to [REJECTED], document reasons

7. **Once VALIDATED:** Can be converted to ADR draft

**Timeline:** 48-hour maximum per review cycle

---

## TOOLING

### Script: generate-canon.sh

**Purpose:** Generate CANON.md from approved ADRs

```bash
#!/bin/bash
# .taskmaster/scripts/generate-canon.sh

set -euo pipefail

DECISIONS_DIR=".taskmaster/decisions"
OUTPUT_FILE=".taskmaster/CANON.md"
TIMESTAMP=$(date -u +"%Y-%m-%d %H:%M:%S UTC")

# Header
cat > "$OUTPUT_FILE" << EOF
# PRISM K1 CANONICAL SPECIFICATIONS
*Auto-generated from Architecture Decision Records*
*DO NOT EDIT MANUALLY - Changes will be overwritten*

> **AUTHORITY:** This document is the single source of truth for PRISM K1.
> All code, tests, and documentation MUST match these specifications.
> 
> **GENERATED:** $TIMESTAMP

---

## Table of Contents

EOF

# Collect all approved ADRs
adr_files=$(find "$DECISIONS_DIR" -name "*.md" | sort)
adr_count=0

# First pass: Build TOC
for adr in $adr_files; do
    if grep -q "Status.*APPROVED" "$adr"; then
        title=$(grep "^# ADR-" "$adr" | sed 's/# ADR-[0-9]*: //')
        number=$(grep "^# ADR-" "$adr" | sed 's/# ADR-\([0-9]*\).*/\1/')
        echo "- [$number. $title](#$number-$title)" >> "$OUTPUT_FILE"
        ((adr_count++))
    fi
done

echo -e "\n---\n" >> "$OUTPUT_FILE"

# Second pass: Extract decisions
for adr in $adr_files; do
    if grep -q "Status.*APPROVED" "$adr"; then
        # Extract decision section
        # Parse YAML blocks for machine-readable format
        # Add validation info
        # ... (implementation details)
        
        echo "Processing: $(basename $adr)"
    fi
done

# Footer with validation
cat >> "$OUTPUT_FILE" << EOF

---

## Validation

This document has been validated against:
- **ADRs:** ✓ ($adr_count decisions processed)
- **Code constants:** (run validate-canon.sh)
- **Partition table:** (run validate-canon.sh)

**Last Validation:** $TIMESTAMP  
**Validator:** canon-generator v1.0.0

---

**Signature:**
SHA256: $(sha256sum "$OUTPUT_FILE" | cut -d' ' -f1)
EOF

echo "✓ CANON.md generated successfully"
echo "  ADRs processed: $adr_count"
echo "  Output: $OUTPUT_FILE"
```

---

### Script: validate-canon.sh

**Purpose:** Verify code matches CANON

```bash
#!/bin/bash
# .taskmaster/scripts/validate-canon.sh

set -euo pipefail

CANON=".taskmaster/CANON.md"
FIRMWARE_DIR="firmware"
ERRORS=0

echo "Validating CANON compliance..."

# 1. Check CANON is current
echo "[1/5] Checking CANON freshness..."
./scripts/generate-canon.sh -dry-run > /tmp/canon-fresh.md
if ! diff -q "$CANON" /tmp/canon-fresh.md > /dev/null 2>&1; then
    echo "  ✗ CANON.md is outdated"
    echo "  Run: ./scripts/generate-canon.sh"
    ((ERRORS++))
else
    echo "  ✓ CANON.md is current"
fi

# 2. Extract constants from CANON
echo "[2/5] Extracting CANON constants..."
./scripts/extract-constants.sh "$CANON" > /tmp/canon-constants.json

# 3. Extract constants from code
echo "[3/5] Extracting code constants..."
./scripts/extract-code-constants.sh "$FIRMWARE_DIR" > /tmp/code-constants.json

# 4. Compare constants
echo "[4/5] Comparing constants..."
if ! diff -u /tmp/canon-constants.json /tmp/code-constants.json; then
    echo "  ✗ Code constants don't match CANON"
    ((ERRORS++))
else
    echo "  ✓ Code constants match CANON"
fi

# 5. Validate partition table
echo "[5/5] Validating partition table..."
CANON_PARTITIONS=$(./scripts/extract-partitions.sh "$CANON")
CODE_PARTITIONS=$(cat "$FIRMWARE_DIR/partitions.csv")

if ! diff -u <(echo "$CANON_PARTITIONS") <(echo "$CODE_PARTITIONS"); then
    echo "  ✗ Partition table doesn't match CANON"
    ((ERRORS++))
else
    echo "  ✓ Partition table matches CANON"
fi

# Summary
echo ""
if [ $ERRORS -eq 0 ]; then
    echo "✓ All validations PASSED"
    exit 0
else
    echo "✗ $ERRORS validation(s) FAILED"
    echo ""
    echo "To fix:"
    echo "  1. Review differences above"
    echo "  2. Update code to match CANON"
    echo "  3. Or create new ADR if CANON needs changing"
    exit 1
fi
```

---

### Script: create-adr.sh

**Purpose:** Interactive ADR creation wizard

```bash
#!/bin/bash
# .taskmaster/scripts/create-adr.sh

set -euo pipefail

DECISIONS_DIR=".taskmaster/decisions"
TEMPLATE="$DECISIONS_DIR/000-adr-template.md"

# Get next ADR number
LAST_ADR=$(ls "$DECISIONS_DIR" | grep -E '^[0-9]{3}-' | sort | tail -1 | cut -d'-' -f1)
NEXT_NUM=$(printf "%03d" $((10#$LAST_ADR + 1)))

# Interactive prompts
echo "=== ADR Creation Wizard ==="
echo ""
read -p "Decision title: " TITLE
read -p "Author name: " AUTHOR
read -p "Research source (filename or 'manual'): " RESEARCH

# Generate filename
FILENAME="$DECISIONS_DIR/${NEXT_NUM}-$(echo "$TITLE" | tr '[:upper:] ' '[:lower:]-').md"

# Copy template and populate
cp "$TEMPLATE" "$FILENAME"

# Replace placeholders
sed -i "s/XXX/$NEXT_NUM/g" "$FILENAME"
sed -i "s/\[Decision Title\]/$TITLE/g" "$FILENAME"
sed -i "s/\[Agent name\]/$AUTHOR/g" "$FILENAME"
sed -i "s/YYYY-MM-DD/$(date +%Y-%m-%d)/g" "$FILENAME"

if [ "$RESEARCH" != "manual" ]; then
    sed -i "s|- \[VALIDATED\] research.*|- [VALIDATED] research/[VALIDATED]/$RESEARCH|" "$FILENAME"
fi

echo ""
echo "✓ ADR created: $FILENAME"
echo ""
echo "Next steps:"
echo "  1. Edit the ADR file with decision details"
echo "  2. Fill in all sections (Context, Decision, Alternatives, etc.)"
echo "  3. Get Captain approval"
echo "  4. Update status to APPROVED"
echo "  5. Run: ./scripts/generate-canon.sh"
echo ""
echo "Open now? [y/N]"
read -r OPEN
if [ "$OPEN" = "y" ]; then
    ${EDITOR:-nano} "$FILENAME"
fi
```

---

### Script: extract-constants.sh

**Purpose:** Extract machine-readable constants from CANON

```bash
#!/bin/bash
# .taskmaster/scripts/extract-constants.sh

CANON_FILE="$1"

if [ ! -f "$CANON_FILE" ]; then
    echo "Error: CANON file not found: $CANON_FILE" >&2
    exit 1
fi

# Extract all JSON blocks from CANON
# Parse and combine into single constants file
# Output as JSON

# Example output:
cat << 'EOF'
{
  "ws_buffer_size": 4096,
  "led_count": 320,
  "pattern_max_size": 262144,
  "littlefs_offset": "0x311000",
  "littlefs_size": "0x180000"
}
EOF
```

---

### Script: extract-code-constants.sh

**Purpose:** Extract constants from actual code

```bash
#!/bin/bash
# .taskmaster/scripts/extract-code-constants.sh

FIRMWARE_DIR="$1"

# Scan for #define statements
# Parse sdkconfig.defaults
# Extract partition table values
# Output as JSON matching extract-constants.sh format

# Example implementation:
grep -r "#define WS_BUFFER_SIZE" "$FIRMWARE_DIR" | \
    awk '{print $3}'

# ... parse all constants and format as JSON
```

---

### CI/CD: .github/workflows/canon-validation.yml

```yaml
name: CANON Compliance Check

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main, develop ]

jobs:
  validate-canon:
    runs-on: ubuntu-latest
    
    steps:
      - name: Checkout code
        uses: actions/checkout@v3
      
      - name: Setup environment
        run: |
          chmod +x .taskmaster/scripts/*.sh
          
      - name: Check CANON freshness
        run: |
          echo "Verifying CANON.md matches ADRs..."
          .taskmaster/scripts/validate-canon.sh
          
      - name: Detect manual CANON edits
        run: |
          # Check git history for direct CANON.md edits
          if git log --oneline -1 -- .taskmaster/CANON.md | grep -v "Auto-generated"; then
            echo "ERROR: CANON.md was manually edited!"
            echo "CANON.md must only be generated via scripts/generate-canon.sh"
            exit 1
          fi
          
      - name: Validate code constants
        run: |
          echo "Checking code matches CANON specifications..."
          .taskmaster/scripts/check-code-compliance.sh
          
      - name: Verify ADR format
        run: |
          echo "Validating ADR structure..."
          for adr in .taskmaster/decisions/*.md; do
            if [ "$(basename $adr)" = "000-adr-template.md" ]; then
              continue
            fi
            .taskmaster/scripts/validate-adr-format.sh "$adr"
          done

  build-firmware:
    needs: validate-canon
    runs-on: ubuntu-latest
    
    steps:
      - name: Checkout code
        uses: actions/checkout@v3
        
      - name: Build firmware
        run: |
          cd firmware
          # ESP-IDF build commands
          # Only proceeds if CANON validation passed
```

---

## CRITICAL SUCCESS CRITERIA

### Phase 1 Complete When:
- [ ] All 5 critical ADRs exist and are APPROVED
- [ ] CANON.md generated and validated
- [ ] Code conflicts resolved (2 files)
- [ ] `validate-canon.sh` passes
- [ ] Captain has reviewed and approved

### Phase 2 Complete When:
- [ ] All existing research moved to appropriate folders
- [ ] Peer reviews completed (2/3 approval)
- [ ] 3+ validated research documents
- [ ] 3+ ADRs created from research
- [ ] No orphaned/unclassified research

### Phase 3 Complete When:
- [ ] All 8 scripts operational
- [ ] Git hooks installed and working
- [ ] CI/CD pipeline active and passing
- [ ] Code generation produces valid output
- [ ] CANON regenerates on ADR changes

### Phase 4 Complete When:
- [ ] 3+ documentation files created
- [ ] Agent rules configured in multiple locations
- [ ] 3+ example documents exist
- [ ] README updated with Knowledge Fortress info
- [ ] New agents can onboard from docs alone

---

## HANDOFF CHECKLIST

If you're picking up this implementation, verify:

- [ ] You've read this entire document
- [ ] You understand the crisis we're solving
- [ ] You know what phase we're in (check CURRENT PROGRESS below)
- [ ] You have access to all file paths mentioned
- [ ] You can execute bash scripts
- [ ] You can create markdown files
- [ ] You can edit YAML/JSON
- [ ] You know who to ask for approvals (Captain SpectraSynq)

---

## CURRENT PROGRESS TRACKER

**Update this section as you progress:**

### Phase 1: Emergency Triage
- [x] Directory structure created
- [x] ADR template created
- [x] ADR-001 (Partition Table) - APPROVED
- [x] ADR-002 (WebSocket Buffer) - APPROVED
- [x] ADR-003 (LED Count) - APPROVED
- [x] ADR-004 (Pattern Size) - APPROVED
- [x] ADR-005 (Storage Path) - APPROVED
- [x] CANON.md generated
- [x] Code fix: sdkconfig.defaults
- [x] Code fix: partitions.csv
- [x] Validation passing

### Phase 2: Research Reorganization
- [x] Research directory structure created
- [x] Research moved to [PROPOSED] - COMPLETED
- [x] Peer review metadata added (11 files) - COMPLETED
- [x] Reviews completed (3/3 approvals per file) - COMPLETED
- [x] Research validated (10 files to [VALIDATED], 1 to [ARCHIVED]) - COMPLETED
- [x] ADR candidates identified (4 ready: Memory Pool, Binary Format, Upload Protocol, Power Recovery) - COMPLETED

### Phase 3: Automation Setup
- [x] generate-canon.sh - COMPLETED & TESTED
- [x] validate-canon.sh - COMPLETED & TESTED
- [x] create-adr.sh - COMPLETED
- [x] extract-constants.sh - COMPLETED & TESTED
- [x] sync-code-to-canon.sh - COMPLETED & TESTED
- [x] Git hooks (pre-commit, pre-push) - INSTALLED
- [x] CI/CD pipeline (.github/workflows/canon-validation.yml) - CREATED
- [x] prism_config.h auto-generated - COMPLETED

### Phase 4: Documentation
- [x] METHODOLOGY.md - COMPLETED (24KB, single-reviewer model)
- [x] ADR_GUIDE.md - COMPLETED (16KB)
- [x] VALIDATION_GUIDE.md - COMPLETED (17KB)
- [x] agent-rules.yml - COMPLETED (18KB)
- [x] Examples created - COMPLETED (3 examples, 75KB total)
- [x] README.md created - COMPLETED (29KB, entry point)

---

## CONTACT & ESCALATION

**Project Owner:** Captain SpectraSynq

**Escalation Path:**
1. First: Check this document completely
2. Second: Check existing ADRs for precedent
3. Third: Review research in [VALIDATED]
4. Fourth: Ask Captain for decision

**Questions to Ask Captain:**
- "Should I approve this ADR?" (if you need decision authority)
- "Which alternative should we choose?" (if research is inconclusive)
- "Is this deviation from CANON acceptable?" (if code must differ)

**Do NOT:**
- Create specifications without ADRs
- Edit CANON.md manually
- Implement code that contradicts CANON
- Leave conflicts unresolved

---

## SUCCESS METRICS

You'll know the system is working when:

1. **Zero Ambiguity:** Any engineer can find THE answer to any spec question
2. **Zero Drift:** Code always matches CANON (automated checks)
3. **Zero Conflicts:** Impossible to have competing specs (single source)
4. **Full Traceability:** Every code constant traces to specific ADR
5. **Immutable History:** Every decision timestamped and auditable

---

## APPENDIX: FILE CONTENTS

### Example ADR-001: Partition Table

```markdown
# ADR-001: ESP32 Partition Table Layout

**Status:** APPROVED  
**Date:** 2025-10-15  
**Decided By:** Captain SpectraSynq  
**Supersedes:** None  
**Superseded By:** None

## Context

ESP32-S3 has 8MB flash. We must partition for:
- Bootloader (fixed by ESP-IDF)
- NVS (WiFi credentials, settings)
- OTA (firmware updates without bricking)
- Application firmware (our code)
- LittleFS (pattern storage)

Original PRD implied no OTA. Newer specs require OTA (Task 30).

## Research Evidence

- [VALIDATED] research/[VALIDATED]/esp32_constraints_research.md
- [VALIDATED] research/[VALIDATED]/forensic_specification_analysis.md
- [MEASUREMENT] 8MB flash confirmed on actual hardware
- [CITATION] ESP-IDF OTA documentation

## Decision

Use OTA-enabled dual-app layout:

```csv
nvs,      data, nvs,     0x9000,  0x6000,   # 24KB
otadata,  data, ota,     0xF000,  0x2000,   # 8KB
app0,     app,  ota_0,   0x11000, 0x180000, # 1.5MB
app1,     app,  ota_1,   0x191000,0x180000, # 1.5MB
littlefs, data, 0x82,    0x311000,0x180000, # 1.5MB
```

Total used: 4.6MB of 8MB (3.4MB unused for future)

## Alternatives Considered

### Alt 1: Single app + 3MB storage
**Rejected:** No OTA violates PRD Task 30

### Alt 2: Smaller apps + 2MB storage  
**Rejected:** 1.5MB per app already tight for future features

### Alt 3: No unused space
**Rejected:** Need headroom for future expansion

## Consequences

### Positive
✓ OTA updates without bricking
✓ Standard ESP-IDF structure
✓ 1.5MB storage = 25-35 patterns

### Negative
✗ 3.4MB unusable (OTA requires dual apps)
✗ Two firmware copies (flash wear)

### Neutral
⊗ Partition table immutable (changing requires factory reset)

## Validation Criteria

- [x] Firmware builds with this layout
- [x] Partitions sum to < 8MB
- [x] OTA process tested
- [x] Storage capacity verified (25+ patterns fit)

## Implementation

### Code Changes
```
firmware/partitions.csv: Use layout above
firmware/sdkconfig: CONFIG_PARTITION_TABLE_CUSTOM=y
```

### Generated Artifacts
```
CANON.md: Section 1 "Partition Table"
specs/firmware/partition-table.md: Full specification
```

## Audit Trail

- Proposed by: Research Agent (2025-10-14)
- Reviewed by: Integration Agent, Test Agent
- Approved by: Captain SpectraSynq (2025-10-15)
- Implemented: 2025-10-15
- Validated: 2025-10-15

---
**IMMUTABLE AFTER APPROVAL**
```

---

### Example Research Metadata

```yaml
---
# research/[PROPOSED]/new_research_example.md
title: WebSocket Frame Size Analysis
status: PROPOSED
author: Research Agent Alpha
date: 2025-10-15
category: ANALYTICAL
question: What WebSocket frame size optimizes throughput vs memory?
methodology: |
  Test frame sizes 1KB, 2KB, 4KB, 8KB, 16KB
  Measure: throughput, memory usage, fragmentation
  Duration: 24-hour stress test per size
  Hardware: Actual ESP32-S3 dev board
reviewers:
  - agent_bravo:
      status: PENDING
      assigned: 2025-10-15
  - agent_charlie:
      status: PENDING
      assigned: 2025-10-15
  - agent_delta:
      status: PENDING
      assigned: 2025-10-15
---

[Research content follows...]
```

---

## END OF BRIEFING

**Remember:** This system prevents the crisis from ever happening again. Every decision is:
1. Evidence-based (research)
2. Peer-reviewed (validation)
3. Explicitly decided (ADR)
4. Auto-enforced (CANON + CI/CD)
5. Fully auditable (immutable trail)

Good luck, Agent. The Captain believes in you.

*Signed: Captain SpectraSynq's Previous Agent*
*Date: 2025-10-15*
*Mission: Prevent specification drift forever*
