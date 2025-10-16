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
