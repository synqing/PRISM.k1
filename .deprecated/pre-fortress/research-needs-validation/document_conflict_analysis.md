---
title: Document Conflict Analysis Report
status: PROPOSED
author: Integration Agent
date: 2025-10-15
category: ANALYTICAL
question: What conflicts exist between different specification documents?
methodology: |
  Cross-document comparison
  Conflict identification and classification
  Impact assessment
impact: HIGH - Identified specification crisis that led to ADR system
superseded_by: forensic_specification_analysis.md
reviewers:
  conflict_validator:
    status: APPROVED_WITH_NOTES
    date: 2025-10-15
    comments: "Excellent conflict identification. Some conclusions superseded by forensic analysis. Archive after review."
  documentation_validator:
    status: APPROVED
    date: 2025-10-15
    comments: "Thorough analysis. Led to creation of Knowledge Fortress system. Historical value only."
  integration_validator:
    status: APPROVED
    date: 2025-10-15
    comments: "This research exposed the crisis. Superseded by forensic_specification_analysis.md. Move to [ARCHIVED]."
---

# Document Conflict Analysis Report
*Comparing newly created foundation documents with existing documentation*

## Executive Summary

After thorough analysis, I've identified several areas of **overlap and potential conflict** between the newly created foundation documents and previously existing documentation. While the new documents are more comprehensive and production-ready, there are inconsistencies that need resolution.

## 🔴 CRITICAL CONFLICTS IDENTIFIED

### 1. **Partition Table Conflict**

#### Existing (`storage_layout.md`):
```
nvs,        data, nvs,     0x9000,  0x6000,
phy_init,   data, phy,     0xf000,  0x1000,
factory,    app,  factory, 0x10000, 0x200000,  # 2MB firmware
storage,    data, spiffs,  0x210000,0x180000,  # 1.5MB LittleFS
coredump,   data, coredump,0x390000,0x10000,
```

#### New (`firmware_architecture.md`):
```
nvs,      data,   nvs,     0x9000,   0x6000,
otadata,  data,   ota,     0xF000,   0x2000,
app0,     app,    ota_0,   0x11000,  0x170000,  # 1.47MB
app1,     app,    ota_1,   0x181000, 0x170000,  # 1.47MB
prismfs,  data,   0x82,    0x2F1000, 0x100000,  # 1MB
```

**CONFLICT:**
- Different partition names (storage vs prismfs)
- Different sizes (1.5MB vs 1MB for filesystem)
- OTA support in new version, single app in old
- Different offsets and layout

### 2. **Storage Path Conflict**

#### Existing (`storage_layout.md`):
- Mount point: `/littlefs/`
- Pattern path: `/littlefs/patterns/`
- Template path: `/littlefs/templates/`

#### New (`firmware_architecture.md`):
- Mount point: `/prism/`
- Pattern path: `/prism/*.prism`
- No separate template directory (embedded in firmware)

**CONFLICT:** Different filesystem mount points and directory structure

### 3. **Binary Format Conflict**

#### Existing (`storage_layout.md`):
```c
typedef struct {
    uint32_t magic;           // 0x5052534D ('PRSM')
    uint8_t version_major;    // 1
    uint8_t version_minor;    // 0
    uint8_t compression;      // 0=none, 1=delta, 2=RLE
    // ... 64 byte header
}
```

#### New (implied in specs):
```c
typedef struct {
    uint32_t magic;           // 'PRIS' (0x53495250)
    uint16_t version;         // 0x0001
    uint16_t flags;           // bit0=compressed
    // ... 32 byte header
}
```

**CONFLICT:**
- Different magic numbers ('PRSM' vs 'PRIS')
- Different header sizes (64B vs 32B)
- Different compression approaches

### 4. **Template Storage Conflict**

#### Existing (`template_catalog.md`):
- Templates compiled into firmware
- 20 templates defined
- Function pointers in ROM

#### New (`template_specifications.md`):
- 15 templates exactly
- Binary .prism format
- Can be stored or embedded

**CONFLICT:** Different numbers and storage strategies

### 5. **WebSocket Protocol Conflict**

#### Existing (`websocket_protocol.md` - original):
- Binary TLV protocol
- CRC32 validation
- Type-Length-Value encoding

#### New (`websocket_protocol.md` - attempted update):
- JSON-based protocol
- UUID message correlation
- Command/response/event model

**CONFLICT:** Completely different protocol approaches (binary vs JSON)

## 🟡 MINOR OVERLAPS

### 1. **Memory Allocations**
- Both define heap allocations but with different granularity
- New version more detailed (338KB usable with specific allocations)
- Old version less specific about breakdown

### 2. **Pattern Parameters**
- Both define similar parameters (speed, intensity, etc.)
- New version has standardized ranges
- Some naming differences

### 3. **Performance Targets**
- Both mention 60 FPS target
- New version has more specific latency requirements
- Upload speed targets differ slightly

## 🟢 COMPLEMENTARY AREAS

### 1. **Research Documents**
The research documents in `/research/` folder complement rather than conflict:
- `esp32_constraints_research.md` - Hardware analysis
- `critical_risks.md` - Risk assessment
- `implementation_priority.md` - Development order
- `websocket_validation.md` - Protocol testing

### 2. **Task Definitions**
The JSON task files define work items, not technical specs, so no conflict.

## 📋 RECONCILIATION RECOMMENDATIONS

### Priority 1: Partition Table
**Resolution:** Adopt the NEW partition table from `firmware_architecture.md` because:
- Includes OTA support (critical for updates)
- More realistic size allocations
- Properly aligned boundaries

**Action Required:**
```csv
# Use this partition table going forward:
nvs,      data, nvs,     0x9000,  0x6000,
otadata,  data, ota,     0xF000,  0x2000,
app0,     app,  ota_0,   0x11000, 0x170000,
app1,     app,  ota_1,   0x181000,0x170000,
prismfs,  data, 0x82,    0x2F1000,0x100000,
```

### Priority 2: Filesystem Paths
**Resolution:** Use `/prism/` mount point consistently
- Update all references to use `/prism/` not `/littlefs/`
- Flatten directory structure (no subdirectories needed for 1MB)

### Priority 3: Binary Format
**Resolution:** Use the COMPREHENSIVE format from original requirements:
- 32-byte header with 'PRIS' magic
- TLV body structure
- SHA-256 trailer
- Heatshrink compression

### Priority 4: WebSocket Protocol
**Resolution:** Use JSON protocol from new `websocket_protocol.md`:
- More debuggable
- Industry standard
- Easier client implementation
- Binary protocol reserved for v2.0

### Priority 5: Template Count
**Resolution:** Start with 15 templates as specified:
- More achievable for initial release
- Can add 16-20 in updates
- Focus on quality over quantity

## 🔧 FILES TO UPDATE

1. **Delete or Archive:**
   - `storage_layout.md` (superseded by new specs)
   - `template_catalog.md` (superseded by template_specifications.md)

2. **Keep As-Is:**
   - `firmware_architecture.md` (new, authoritative)
   - `template_specifications.md` (new, complete)
   - All research documents (complementary)

3. **Needs Clarification:**
   - `websocket_protocol.md` (which version to use?)

## ✅ FINAL RECOMMENDATIONS

1. **Use the NEW foundation documents** as the source of truth
2. **Archive the conflicting old documents** to `.archive/` folder
3. **Update any code references** to match new specs
4. **Document the decision** in a DECISIONS.md file

The new foundation documents are more comprehensive, production-ready, and internally consistent. They should be adopted as the authoritative specification for PRISM.K1 firmware implementation.

## Conflict Resolution Priority

1. **IMMEDIATE:** Fix partition table (blocks all storage work)
2. **HIGH:** Resolve filesystem paths (affects all file operations)
3. **MEDIUM:** Finalize binary format (affects pattern creation)
4. **LOW:** Reconcile template counts (can adjust later)

---

*Generated: 2025-10-15*
*Status: Requires decision on conflicts*