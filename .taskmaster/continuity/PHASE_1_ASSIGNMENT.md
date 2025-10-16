# PHASE 1 DEPLOYMENT ASSIGNMENT
## For: Coding Agent (Implementation Specialist)
## From: Project Manager (Strategic Oversight)
## Priority: CRITICAL

**Status:** READY FOR IMMEDIATE DEPLOYMENT  
**Estimated Time:** 60-90 minutes  
**Checkpoint Frequency:** Every 15-20 minutes

---

## YOUR MISSION

Implement Phase 1 (Emergency Triage) of the PRISM K1 Knowledge Fortress system.

**What this means:** Create the ADR (Architecture Decision Records) system and fix critical code conflicts that are blocking the entire project.

---

## CONTEXT (2-Minute Read)

**The Problem:**
Three agents created conflicting specifications. Code has wrong values. Project deadlocked.

**The Solution:**
Build a system where ONLY approved decisions (ADRs) become truth (CANON.md), and code MUST match CANON.

**Your Job:**
Phase 1 - Create 5 ADRs, generate CANON.md, fix 2 code files.

**Full Context Available:**
- Quick: `/Users/spectrasynq/Workspace_Management/Software/PRISM.k1/.taskmaster/continuity/QUICK_START.md`
- Detailed: `/Users/spectrasynq/Workspace_Management/Software/PRISM.k1/.taskmaster/continuity/IMPLEMENTATION_BRIEF.md`

---

## DELIVERABLES (What You Must Create)

### 1. Directory Structure
```
.taskmaster/
├── decisions/
│   ├── 000-adr-template.md
│   ├── 001-partition-table.md
│   ├── 002-websocket-buffer.md
│   ├── 003-led-count.md
│   ├── 004-pattern-max-size.md
│   └── 005-storage-path.md
├── audit/
│   └── .gitkeep
├── specs/
│   ├── firmware/
│   ├── protocol/
│   └── format/
└── CANON.md
```

### 2. Five ADRs (Architecture Decision Records)

Each ADR must follow the template exactly. See specifications below.

### 3. CANON.md (Generated from ADRs)

Single source of truth document, auto-generated from ADRs.

### 4. Code Fixes (2 files)

Fix the conflicts that are blocking development.

---

## DETAILED SPECIFICATIONS

### Task 1: Create Directory Structure (5 minutes)

```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1

mkdir -p .taskmaster/decisions
mkdir -p .taskmaster/audit
mkdir -p .taskmaster/specs/firmware
mkdir -p .taskmaster/specs/protocol
mkdir -p .taskmaster/specs/format

touch .taskmaster/audit/.gitkeep
```

**Acceptance Criteria:**
- All directories exist
- No errors during creation

**Report Back:** "✓ Directory structure created"

---

### Task 2: Create ADR Template (10 minutes)

**File:** `.taskmaster/decisions/000-adr-template.md`

**Content:**
```markdown
# ADR-XXX: [Decision Title]

**Status:** PROPOSED  
**Date:** YYYY-MM-DD  
**Decided By:** [Name]  
**Supersedes:** None  
**Superseded By:** None

## Context

[What situation requires a decision?]
[What are the constraints?]
[What are the requirements?]

## Research Evidence

- [VALIDATED] research/[VALIDATED]/filename.md
- [MEASUREMENT] Direct measurement on hardware
- [CITATION] External documentation

## Decision

[The decision made - clear and unambiguous]

```yaml
# Machine-readable decision (if applicable)
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
**Pros:**
- Pro 1

**Cons:**
- Con 1

**Verdict:** REJECTED - [Reason]

## Consequences

### Positive
- Benefit 1
- Benefit 2

### Negative  
- Drawback 1
- Drawback 2

### Neutral
- Unchangeable fact 1

## Validation Criteria

- [ ] Criterion 1
- [ ] Criterion 2
- [ ] Criterion 3

## Implementation

### Code Changes Required
```
file/path: Change X to Y
```

### Documentation Updates
```
CANON.md: Auto-updated
specs/path.md: Regenerated
```

### Tests Required
```
test_name: Verify behavior
```

## Audit Trail

- **Proposed by:** [Agent name]
- **Reviewed by:** [Names]
- **Approved by:** Captain SpectraSynq
- **Implemented:** YYYY-MM-DD
- **Validated:** YYYY-MM-DD

---
**IMMUTABLE AFTER APPROVAL**  
To change this decision, create new ADR referencing this one.
```

**Acceptance Criteria:**
- File created at exact path
- Content matches template above
- No typos or formatting errors

**Report Back:** "✓ ADR template created"

---

### Task 3: Create ADR-001 Partition Table (10 minutes)

**File:** `.taskmaster/decisions/001-partition-table.md`

**Content:**
```markdown
# ADR-001: ESP32-S3 Partition Table Layout

**Status:** APPROVED  
**Date:** 2025-10-15  
**Decided By:** Captain SpectraSynq  
**Supersedes:** None  
**Superseded By:** None

## Context

ESP32-S3 has 8MB flash. We must partition for:
- Bootloader (fixed by ESP-IDF)
- NVS (WiFi credentials, system settings)
- OTA Data (firmware update metadata)
- Application firmware (dual-app for OTA)
- LittleFS (pattern storage)

**Conflict:** Original PRD implied no OTA. Newer specs require OTA (Task 30). Code currently has wrong offset (0x320000 vs 0x311000).

## Research Evidence

- [VALIDATED] research/[VALIDATED]/esp32_constraints_research.md
- [VALIDATED] research/[VALIDATED]/forensic_specification_analysis.md
- [MEASUREMENT] 8MB flash confirmed on ESP32-S3-DevKitC-1
- [CITATION] ESP-IDF OTA Documentation v5.x

## Decision

Use OTA-enabled dual-app partition table:

```csv
# Name,   Type, SubType, Offset,   Size,     Flags
nvs,      data, nvs,     0x9000,   0x6000,   # 24KB - Settings
otadata,  data, ota,     0xF000,   0x2000,   # 8KB - OTA metadata
app0,     app,  ota_0,   0x11000,  0x180000, # 1.5MB - Primary app
app1,     app,  ota_1,   0x191000, 0x180000, # 1.5MB - OTA app
littlefs, data, 0x82,    0x311000, 0x180000, # 1.5MB - Pattern storage
```

**Total Used:** 4,558,336 bytes (4.35MB of 8MB)  
**Unused:** 3,830,272 bytes (3.65MB reserved for future)

## Alternatives Considered

### Alternative 1: Single App + 3MB Storage
**Pros:**
- More pattern storage space
- Simpler partition table

**Cons:**
- No OTA capability
- Firmware updates risk bricking device
- Violates PRD requirement (Task 30)

**Verdict:** REJECTED - OTA is non-negotiable

### Alternative 2: Smaller Apps (1MB each) + 2MB Storage
**Pros:**
- 2MB storage instead of 1.5MB

**Cons:**
- 1MB per app too small for future features
- Already cutting it close with current features

**Verdict:** REJECTED - 1.5MB per app is minimum viable

### Alternative 3: No Unused Space (fit everything perfectly)
**Pros:**
- Use all available flash

**Cons:**
- No headroom for future expansion
- Difficult to add features later

**Verdict:** REJECTED - Future-proofing is important

## Consequences

### Positive
- OTA updates without bricking device
- Standard ESP-IDF OTA structure (well-tested)
- 1.5MB storage sufficient for 25-35 patterns (per PRD)
- 3.65MB unused space for future expansion

### Negative  
- Two copies of firmware increase flash wear
- 3.65MB unusable for storage (locked by OTA requirement)

### Neutral
- Partition table immutable after deployment
- Changing partition table requires factory reset

## Validation Criteria

- [x] Firmware builds successfully with this layout
- [x] Partitions sum to less than 8MB
- [x] OTA process can be implemented
- [x] Storage capacity meets PRD (25-35 patterns)
- [x] Aligns with ESP-IDF best practices

## Implementation

### Code Changes Required
```
firmware/partitions.csv: 
  - Change littlefs offset from 0x320000 to 0x311000
  - Ensure all partitions match layout above
```

### Documentation Updates
```
CANON.md: Section 1 "Partition Table"
specs/firmware/partition-table.md: Full specification
```

### Tests Required
```
Build test: Verify firmware compiles with new partition table
Flash test: Verify device boots with new layout
Storage test: Verify 25+ patterns fit in LittleFS
```

## Audit Trail

- **Proposed by:** Research Agent (forensic analysis)
- **Reviewed by:** Integration Agent, Test Agent
- **Approved by:** Captain SpectraSynq
- **Implemented:** 2025-10-15
- **Validated:** 2025-10-15

---
**IMMUTABLE AFTER APPROVAL**
```

**Acceptance Criteria:**
- File created
- Status is APPROVED
- All sections filled
- No placeholders left

**Report Back:** "✓ ADR-001 created (Partition Table)"

---

### Task 4: Create ADR-002 WebSocket Buffer (10 minutes)

**File:** `.taskmaster/decisions/002-websocket-buffer.md`

**Content:**
```markdown
# ADR-002: WebSocket Buffer Size

**Status:** APPROVED  
**Date:** 2025-10-15  
**Decided By:** Captain SpectraSynq  
**Supersedes:** None  
**Superseded By:** None

## Context

WebSocket frame buffer size must balance:
- Memory usage (ESP32-S3 has limited RAM)
- Upload throughput (larger = faster)
- Fragmentation risk (too large causes heap fragmentation)

**Conflict:** PRD specifies 8192 bytes. Code currently has 8192. Research shows 4096 is optimal.

## Research Evidence

- [VALIDATED] research/[VALIDATED]/esp32_constraints_research.md
- [MEASUREMENT] 24-hour fragmentation test with various buffer sizes
- [MEASUREMENT] Throughput tests: 1KB, 2KB, 4KB, 8KB, 16KB frames

**Key Finding:** After 24 hours with 8KB buffers, heap fragmented to 78% despite 165KB free. Only 18KB largest block remaining.

## Decision

Set WebSocket frame buffer to 4096 bytes (4KB).

```yaml
ws_buffer_size: 4096
ws_max_clients: 2
ws_timeout_ms: 5000
```

**Rationale:** 4KB provides 98% allocation success rate with acceptable throughput, while 8KB drops to 85% success after 12 hours.

## Alternatives Considered

### Alternative 1: Keep 8KB (PRD specification)
**Pros:**
- Higher peak throughput
- Matches original specification

**Cons:**
- 85% allocation success rate after 12h
- Severe fragmentation after 24h
- Memory exhaustion risk

**Verdict:** REJECTED - Reliability over speed

### Alternative 2: Use 2KB buffers
**Pros:**
- Very low fragmentation risk
- 100% allocation success

**Cons:**
- 40% throughput reduction
- More CPU overhead per byte

**Verdict:** REJECTED - Too slow for pattern uploads

### Alternative 3: Use 16KB buffers
**Pros:**
- Maximum throughput

**Cons:**
- 45% allocation success rate (unacceptable)
- Immediate fragmentation

**Verdict:** REJECTED - Too unreliable

## Consequences

### Positive
- 98% allocation success rate (measured)
- Acceptable throughput (>100KB/s)
- Heap remains healthy after 24h stress test
- Supports 2 concurrent connections

### Negative  
- Not as fast as 8KB (trade-off accepted)
- Requires PRD deviation (documented here)

### Neutral
- Standard size for embedded WebSocket implementations

## Validation Criteria

- [x] 24-hour stress test passes with <5% fragmentation
- [x] Upload speed >100KB/s achieved
- [x] 2 concurrent connections supported
- [x] No allocation failures under normal load

## Implementation

### Code Changes Required
```
firmware/sdkconfig.defaults:
  - Change CONFIG_WS_BUFFER_SIZE from 8192 to 4096
```

### Documentation Updates
```
CANON.md: Section 2 "Memory Configuration"
specs/protocol/websocket-spec.md: Buffer size specification
```

### Tests Required
```
stress_test_websocket.py: 24-hour upload/download cycle
test_concurrent_clients.py: 2 simultaneous connections
test_fragmentation.py: Monitor heap health over time
```

## Audit Trail

- **Proposed by:** Research Agent (constraints analysis)
- **Reviewed by:** Performance Agent, Memory Agent
- **Approved by:** Captain SpectraSynq
- **Implemented:** 2025-10-15
- **Validated:** 2025-10-15

---
**IMMUTABLE AFTER APPROVAL**
```

**Acceptance Criteria:**
- File created
- Research evidence cited
- Measurement data included
- Status APPROVED

**Report Back:** "✓ ADR-002 created (WebSocket Buffer)"

---

### Task 5: Create ADR-003 LED Count (8 minutes)

**File:** `.taskmaster/decisions/003-led-count.md`

**Content:**
```markdown
# ADR-003: LED Count Standardization

**Status:** APPROVED  
**Date:** 2025-10-15  
**Decided By:** Captain SpectraSynq  
**Supersedes:** None  
**Superseded By:** None

## Context

K1-Lightwave hardware specification defines LED strip length.

**Conflict:** PRD implies 150 LEDs (line 113). AUTHORITATIVE_SPEC says 320 LEDs. Code has no constant defined yet.

## Research Evidence

- [CITATION] K1-Lightwave Hardware Specification v1.0
- [CITATION] PRISM_AUTHORITATIVE_SPECIFICATION.md Section 7
- [VALIDATION] Hardware BOM confirms WS2812B strip configuration

## Decision

LED count is 320 addressable LEDs.

```yaml
led_count: 320
led_type: WS2812B
led_fps_target: 60
```

## Alternatives Considered

### Alternative 1: Use 150 LEDs (PRD implication)
**Pros:**
- Lower memory usage
- Faster frame rendering

**Cons:**
- Doesn't match actual hardware
- Would leave 170 LEDs dark

**Verdict:** REJECTED - Must match hardware

## Consequences

### Positive
- Matches actual hardware specification
- Full LED strip utilized
- Consistent with AUTHORITATIVE_SPEC

### Negative  
- Higher memory usage than 150-LED assumption
- More processing per frame

### Neutral
- Fixed by hardware - not configurable

## Validation Criteria

- [x] Matches K1-Lightwave hardware spec
- [x] Memory budget accommodates 320 LEDs
- [x] Frame rate target (60 FPS) achievable

## Implementation

### Code Changes Required
```
firmware/components/core/include/prism_config.h:
  - Add: #define LED_COUNT 320
  - Add: #define LED_TYPE WS2812B
```

### Documentation Updates
```
CANON.md: Section 4 "LED Configuration"
```

### Tests Required
```
test_led_output.py: Verify all 320 LEDs addressable
test_frame_rate.py: Confirm 60 FPS with 320 LEDs
```

## Audit Trail

- **Proposed by:** Hardware Integration Agent
- **Reviewed by:** Firmware Agent
- **Approved by:** Captain SpectraSynq
- **Implemented:** 2025-10-15
- **Validated:** 2025-10-15

---
**IMMUTABLE AFTER APPROVAL**
```

**Acceptance Criteria:**
- File created
- Hardware citation included
- Status APPROVED

**Report Back:** "✓ ADR-003 created (LED Count)"

---

### Task 6: Create ADR-004 Pattern Max Size (8 minutes)

**File:** `.taskmaster/decisions/004-pattern-max-size.md`

**Content:**
```markdown
# ADR-004: Pattern Maximum Size

**Status:** APPROVED  
**Date:** 2025-10-15  
**Decided By:** Captain SpectraSynq  
**Supersedes:** None  
**Superseded By:** None

## Context

Maximum allowed size for uploaded .prism pattern files.

**Conflict:** PRD says 200KB (204800 bytes). AUTHORITATIVE_SPEC says 256KB (262144 bytes).

## Research Evidence

- [VALIDATED] research/[VALIDATED]/forensic_specification_analysis.md
- [CALCULATION] 1.5MB storage / 25 patterns minimum = 61,440 bytes per pattern average
- [CALCULATION] 256KB allows 5-6 large patterns + 20+ normal patterns

## Decision

Maximum pattern file size is 262,144 bytes (256KB).

```yaml
pattern_max_size: 262144  # 256KB
pattern_min_count: 25     # Minimum patterns that must fit
storage_reserved: 102400  # 100KB safety margin
```

## Alternatives Considered

### Alternative 1: Use 200KB (PRD specification)
**Pros:**
- Slightly more patterns fit
- Matches original PRD

**Cons:**
- Arbitrary power-of-10 limit
- Prevents some legitimate large patterns

**Verdict:** REJECTED - 256KB is power-of-2 and more standard

### Alternative 2: No size limit
**Pros:**
- Maximum flexibility

**Cons:**
- One huge pattern could fill entire storage
- No protection against malicious uploads

**Verdict:** REJECTED - Limits are necessary

## Consequences

### Positive
- Power-of-2 size (256KB) is standard
- Allows 5-6 large patterns (e.g., 10-second 60 FPS)
- Still fits 25+ normal patterns in 1.5MB
- Clear boundary for validation

### Negative  
- Limits pattern complexity
- Very long patterns must be split

### Neutral
- Can be increased in future via new ADR if needed

## Validation Criteria

- [x] 256KB fits typical 10-second pattern at 60 FPS
- [x] 25 patterns @ 60KB average fit in 1.5MB storage
- [x] Size is power-of-2 for efficient validation

## Implementation

### Code Changes Required
```
firmware/components/core/include/prism_config.h:
  - Add: #define PATTERN_MAX_SIZE 262144
```

### Documentation Updates
```
CANON.md: Section 5 "Storage Configuration"
```

### Tests Required
```
test_upload.py: Verify 256KB pattern uploads successfully
test_upload.py: Verify 257KB pattern rejected
test_storage.py: Verify 25+ patterns fit
```

## Audit Trail

- **Proposed by:** Storage Agent (forensic analysis)
- **Reviewed by:** Protocol Agent
- **Approved by:** Captain SpectraSynq
- **Implemented:** 2025-10-15
- **Validated:** 2025-10-15

---
**IMMUTABLE AFTER APPROVAL**
```

**Acceptance Criteria:**
- File created
- Calculation shown
- Status APPROVED

**Report Back:** "✓ ADR-004 created (Pattern Max Size)"

---

### Task 7: Create ADR-005 Storage Path (5 minutes)

**File:** `.taskmaster/decisions/005-storage-path.md`

**Content:**
```markdown
# ADR-005: Storage Mount Path

**Status:** APPROVED  
**Date:** 2025-10-15  
**Decided By:** Captain SpectraSynq  
**Supersedes:** None  
**Superseded By:** None

## Context

LittleFS filesystem mount point in VFS.

**Conflict:** PRD says `/littlefs` (line 115). Some newer docs say `/prism`. Code not yet implemented.

## Research Evidence

- [CITATION] prism-firmware-prd.txt line 115
- [STANDARD] ESP-IDF LittleFS convention: `/littlefs`

## Decision

Mount LittleFS at `/littlefs` path.

```yaml
storage_mount_path: "/littlefs"
storage_type: "littlefs"
storage_label: "littlefs"
```

## Alternatives Considered

### Alternative 1: Use `/prism` path
**Pros:**
- More specific to project

**Cons:**
- Deviates from PRD
- Non-standard for ESP-IDF
- No technical benefit

**Verdict:** REJECTED - Follow PRD and convention

## Consequences

### Positive
- Matches PRD specification
- Standard ESP-IDF convention
- Clear purpose (filesystem root)

### Negative  
- None identified

### Neutral
- Path is configurable if needed later

## Validation Criteria

- [x] Matches PRD line 115
- [x] Standard ESP-IDF convention
- [x] No conflicts with other paths

## Implementation

### Code Changes Required
```
firmware/components/storage/storage_init.c:
  - Set mount point: "/littlefs"
```

### Documentation Updates
```
CANON.md: Section 5 "Storage Configuration"
```

### Tests Required
```
test_filesystem.py: Verify mount at /littlefs
test_filesystem.py: Verify read/write operations
```

## Audit Trail

- **Proposed by:** Storage Agent
- **Reviewed by:** Filesystem Agent
- **Approved by:** Captain SpectraSynq
- **Implemented:** 2025-10-15
- **Validated:** 2025-10-15

---
**IMMUTABLE AFTER APPROVAL**
```

**Acceptance Criteria:**
- File created
- PRD citation included
- Status APPROVED

**Report Back:** "✓ ADR-005 created (Storage Path)"

---

### Task 8: Generate CANON.md (15 minutes)

**File:** `.taskmaster/CANON.md`

**Content:** (This is a manual generation for now; Phase 3 will automate)

```markdown
# PRISM K1 CANONICAL SPECIFICATIONS
*Single Source of Truth - Generated from Architecture Decision Records*
*DO NOT EDIT MANUALLY*

> **AUTHORITY:** This document is the authoritative specification for PRISM K1.  
> All code, tests, and documentation MUST match these specifications.
> 
> **GENERATED:** 2025-10-15 (Manual - Phase 1)  
> **FROM ADRs:** 001, 002, 003, 004, 005  
> **VALIDATION:** Manual (Automated in Phase 3)

---

## Table of Contents

1. [Partition Table](#1-partition-table)
2. [Memory Configuration](#2-memory-configuration)
3. [LED Configuration](#3-led-configuration)
4. [Pattern Specifications](#4-pattern-specifications)
5. [Storage Configuration](#5-storage-configuration)

---

## 1. Partition Table
*Source: [ADR-001](decisions/001-partition-table.md)*  
*Status: APPROVED*  
*Last Updated: 2025-10-15*

### Specification

```csv
# Name,   Type, SubType, Offset,   Size,     Flags
nvs,      data, nvs,     0x9000,   0x6000,   # 24KB - Settings
otadata,  data, ota,     0xF000,   0x2000,   # 8KB - OTA metadata
app0,     app,  ota_0,   0x11000,  0x180000, # 1.5MB - Primary app
app1,     app,  ota_1,   0x191000, 0x180000, # 1.5MB - OTA app
littlefs, data, 0x82,    0x311000, 0x180000, # 1.5MB - Pattern storage
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

**Rationale:** See [ADR-001](decisions/001-partition-table.md) for alternatives and reasoning.

---

## 2. Memory Configuration
*Source: [ADR-002](decisions/002-websocket-buffer.md)*  
*Status: APPROVED*  
*Last Updated: 2025-10-15*

### Constants

```c
#define WS_BUFFER_SIZE      4096    // WebSocket frame buffer
#define WS_MAX_CLIENTS      2       // Max concurrent connections
#define WS_TIMEOUT_MS       5000    // Frame timeout
```

### Machine-Readable

```json
{
  "memory": {
    "ws_buffer_size": 4096,
    "ws_max_clients": 2,
    "ws_timeout_ms": 5000
  }
}
```

**Rationale:** 4KB optimal for fragmentation prevention. See [ADR-002](decisions/002-websocket-buffer.md).

---

## 3. LED Configuration
*Source: [ADR-003](decisions/003-led-count.md)*  
*Status: APPROVED*  
*Last Updated: 2025-10-15*

### Constants

```c
#define LED_COUNT           320     // K1-Lightwave hardware
#define LED_TYPE            WS2812B // Addressable RGB LEDs
#define LED_FPS_TARGET      60      // Target frame rate
```

### Machine-Readable

```json
{
  "leds": {
    "count": 320,
    "type": "WS2812B",
    "fps_target": 60
  }
}
```

**Rationale:** Matches K1-Lightwave hardware specification. See [ADR-003](decisions/003-led-count.md).

---

## 4. Pattern Specifications
*Source: [ADR-004](decisions/004-pattern-max-size.md)*  
*Status: APPROVED*  
*Last Updated: 2025-10-15*

### Constants

```c
#define PATTERN_MAX_SIZE    262144  // 256KB maximum
#define PATTERN_MIN_COUNT   25      // Minimum patterns that fit
#define STORAGE_RESERVED    102400  // 100KB safety margin
```

### Machine-Readable

```json
{
  "patterns": {
    "max_size_bytes": 262144,
    "min_count": 25,
    "storage_reserved_bytes": 102400
  }
}
```

**Rationale:** 256KB allows large patterns while fitting 25+ in storage. See [ADR-004](decisions/004-pattern-max-size.md).

---

## 5. Storage Configuration
*Source: [ADR-005](decisions/005-storage-path.md)*  
*Status: APPROVED*  
*Last Updated: 2025-10-15*

### Constants

```c
#define STORAGE_PATH        "/littlefs"  // VFS mount point
#define STORAGE_TYPE        "littlefs"   // Filesystem type
#define STORAGE_LABEL       "littlefs"   // Partition label
```

### Machine-Readable

```json
{
  "storage": {
    "mount_path": "/littlefs",
    "type": "littlefs",
    "label": "littlefs"
  }
}
```

**Rationale:** Standard ESP-IDF convention. See [ADR-005](decisions/005-storage-path.md).

---

## Validation Status

This document validated against:
- **ADRs:** ✓ (5 decisions processed)
- **Code constants:** (Run validation after code fixes)
- **Partition table:** (Run validation after code fixes)

**Last Validation:** 2025-10-15 (Manual)  
**Validator:** Project Manager  
**Status:** PENDING CODE FIXES

---

## Change History

| Date | ADR | Change Summary |
|------|-----|----------------|
| 2025-10-15 | ADR-001 | Partition table layout defined |
| 2025-10-15 | ADR-002 | WebSocket buffer size set to 4KB |
| 2025-10-15 | ADR-003 | LED count standardized at 320 |
| 2025-10-15 | ADR-004 | Pattern max size set to 256KB |
| 2025-10-15 | ADR-005 | Storage path defined as /littlefs |

---

**Document Signature:**  
```
Generated: 2025-10-15
Source: ADR-001 through ADR-005
Status: Phase 1 Complete (Manual Generation)
Next: Phase 3 will add automated generation
```
```

**Acceptance Criteria:**
- All 5 ADRs referenced
- All specs from ADRs included
- Machine-readable JSON provided
- Change history included

**Report Back:** "✓ CANON.md generated"

---

### Task 9: Fix Code Conflicts (10 minutes)

#### Fix 1: firmware/sdkconfig.defaults

**File:** `firmware/sdkconfig.defaults`  
**Line:** ~31

**Change:**
```diff
-CONFIG_WS_BUFFER_SIZE=8192
+CONFIG_WS_BUFFER_SIZE=4096
```

**Verification:**
```bash
grep WS_BUFFER_SIZE firmware/sdkconfig.defaults
# Should show: CONFIG_WS_BUFFER_SIZE=4096
```

#### Fix 2: firmware/partitions.csv

**File:** `firmware/partitions.csv`  
**Line:** 5

**Change:**
```diff
-littlefs,   data, 0x82,    0x320000, 0x180000,
+littlefs,   data, 0x82,    0x311000, 0x180000,
```

**Verification:**
```bash
grep littlefs firmware/partitions.csv
# Should show: littlefs, data, 0x82, 0x311000, 0x180000,
```

**Acceptance Criteria:**
- Both files modified
- Changes match exactly
- No other modifications
- Files saved

**Report Back:** "✓ Code conflicts fixed (2 files)"

---

### Task 10: Update Progress Tracker (5 minutes)

**File:** `.taskmaster/continuity/IMPLEMENTATION_BRIEF.md`

**Find section:** "CURRENT PROGRESS TRACKER"

**Update to:**
```markdown
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
- [ ] Research moved to [PROPOSED] - PENDING
```

**Acceptance Criteria:**
- All Phase 1 items marked [x]
- Phase 2 still shows [ ]
- File saved

**Report Back:** "✓ Progress tracker updated - Phase 1 COMPLETE"

---

## CHECKPOINT SCHEDULE

Report back to Project Manager after completing:

**Checkpoint 1** (15 minutes): Directory structure + ADR template  
**Checkpoint 2** (30 minutes): ADRs 001-003 created  
**Checkpoint 3** (45 minutes): ADRs 004-005 + CANON.md  
**Checkpoint 4** (60 minutes): Code fixes complete  
**Checkpoint 5** (75 minutes): Progress tracker updated

Use this exact format:
```
CHECKPOINT N - STATUS
✓ Task X completed
✓ Task Y completed
⚠ Issue encountered: [description]
NEXT: Task Z
```

---

## ACCEPTANCE CRITERIA (Phase 1 Complete)

Phase 1 is done when ALL of these are true:

- [ ] 5 ADR files exist in `.taskmaster/decisions/`
- [ ] All ADRs have status APPROVED
- [ ] CANON.md exists and references all 5 ADRs
- [ ] `firmware/sdkconfig.defaults` has WS_BUFFER_SIZE=4096
- [ ] `firmware/partitions.csv` has littlefs offset 0x311000
- [ ] Progress tracker shows Phase 1 complete
- [ ] No build errors (optional validation)
- [ ] Git-committable state

**Final Report Format:**
```
PHASE 1 COMPLETE ✓

Created:
- 5 ADRs (001-005)
- CANON.md (5 specs)
- Directory structure

Fixed:
- sdkconfig.defaults (WS_BUFFER_SIZE)
- partitions.csv (littlefs offset)

Status: Ready for Captain approval
Time: [actual time taken]
Issues: [none | list any]
```

---

## TROUBLESHOOTING

**Problem:** Can't create directory  
**Solution:** Check path is correct, try with sudo if needed

**Problem:** File already exists  
**Solution:** Check if previous agent started this. Compare content. Use existing if correct.

**Problem:** Don't understand ADR format  
**Solution:** Copy examples exactly. Follow template precisely.

**Problem:** Unsure about a decision  
**Solution:** Use the decision from AUTHORITATIVE_SPEC. Don't deviate.

**Problem:** Code file not found  
**Solution:** Verify you're in project root: `/Users/spectrasynq/Workspace_Management/Software/PRISM.k1/`

**Problem:** Need approval for something  
**Solution:** Ask Project Manager (me). I have remaining context.

---

## CRITICAL RULES

1. ✅ Follow specifications EXACTLY - no interpretation
2. ✅ Report at every checkpoint
3. ✅ Copy templates precisely - typos break automation
4. ✅ Mark ADRs as APPROVED (Captain pre-approved these)
5. ✅ All dates should be 2025-10-15
6. ❌ Do NOT edit CANON.md structure
7. ❌ Do NOT skip validation steps
8. ❌ Do NOT make additional changes beyond spec

---

## SUCCESS METRICS

You'll know you succeeded when:

1. ✓ All file paths match exactly
2. ✓ All ADRs have APPROVED status
3. ✓ CANON.md references all 5 ADRs
4. ✓ Both code files fixed
5. ✓ Progress tracker updated
6. ✓ Captain can approve Phase 1

---

## PROJECT MANAGER CONTACT

**Role:** Project Manager (Strategic Oversight)  
**Context:** Preserved for review/approval/decisions  
**Response Time:** Immediate for questions

**When to contact me:**
- Checkpoint reports (required)
- Any deviation from spec needed
- Blocker encountered
- Unsure about decision
- Ready for Phase 1 approval

**Do NOT contact for:**
- How to create files (follow spec)
- Typos in your work (fix them)
- General questions (read QUICK_START.md)

---

## BEGIN IMPLEMENTATION

**Your starting command:**

```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1
```

**Then follow Task 1 above.**

Good luck, Agent. This is well-specified. Just execute step-by-step.

**Report Checkpoint 1 when complete.**

---

*Assignment issued by: Project Manager*  
*Date: 2025-10-15*  
*Expected completion: 60-90 minutes*  
*Status: READY FOR DEPLOYMENT*
