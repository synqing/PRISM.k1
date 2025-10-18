# PRISM K1 CANONICAL SPECIFICATIONS
*Single Source of Truth - Generated from Architecture Decision Records*
*DO NOT EDIT MANUALLY - Changes will be overwritten*

> **AUTHORITY:** This document is the authoritative specification for PRISM K1.
> All code, tests, and documentation MUST match these specifications.
>
> **GENERATED:** 2025-10-18 06:32:13 UTC
> **FROM ADRs:**  001-partition-table.md 002-websocket-buffer.md 003-led-count.md 004-pattern-max-size.md 005-storage-path.md 006-pattern-count-revision.md 007-partition-alignment-correction.md 008-led-fps-increase.md
> **VALIDATION:** Automated validation required before code changes

---

## Table of Contents

- [1. ESP32-S3 Partition Table Layout](#1-esp32-s3-partition-table-layout)
- [2. WebSocket Buffer Size](#2-websocket-buffer-size)
- [3. LED Count Standardization](#3-led-count-standardization)
- [4. Pattern Maximum Size](#4-pattern-maximum-size)
- [5. Storage Mount Path](#5-storage-mount-path)
- [6. Pattern Count Revision](#6-pattern-count-revision)
- [7. Partition Table Alignment Correction](#7-partition-table-alignment-correction)
- [8. LED Refresh Target Increase (60 FPS → 120 FPS)](#8-led-refresh-target-increase-60-fps--120-fps)

---

## 1. ESP32-S3 Partition Table Layout
*Source: [001-partition-table.md](decisions/001-partition-table.md)*
*Status: APPROVED*
*Last Updated: 2025-10-15*

### Specification


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


**Rationale:** See [001-partition-table.md](decisions/001-partition-table.md) for full context and alternatives considered.

---

## 2. WebSocket Buffer Size
*Source: [002-websocket-buffer.md](decisions/002-websocket-buffer.md)*
*Status: APPROVED*
*Last Updated: 2025-10-15*

### Specification


Set WebSocket frame buffer to 4096 bytes (4KB).


**Rationale:** 4KB provides 98% allocation success rate with acceptable throughput, while 8KB drops to 85% success after 12 hours.


### Machine-Readable

```yaml
ws_buffer_size: 4096
ws_max_clients: 2
ws_timeout_ms: 5000

```
**Rationale:** See [002-websocket-buffer.md](decisions/002-websocket-buffer.md) for full context and alternatives considered.

---

## 3. LED Count Standardization
*Source: [003-led-count.md](decisions/003-led-count.md)*
*Status: APPROVED*
*Last Updated: 2025-10-15*

### Specification


LED count is 320 addressable LEDs.



### Machine-Readable

```yaml
led_count: 320
led_type: WS2812B
led_fps_target: 60

```
**Rationale:** See [003-led-count.md](decisions/003-led-count.md) for full context and alternatives considered.

---

## 4. Pattern Maximum Size
*Source: [004-pattern-max-size.md](decisions/004-pattern-max-size.md)*
*Status: APPROVED*
*Last Updated: 2025-10-15*

### Specification


Maximum pattern file size is 262,144 bytes (256KB).



### Machine-Readable

```yaml
pattern_max_size: 262144  # 256KB
pattern_min_count: 25     # Minimum patterns that must fit
storage_reserved: 102400  # 100KB safety margin

```
**Rationale:** See [004-pattern-max-size.md](decisions/004-pattern-max-size.md) for full context and alternatives considered.

---

## 5. Storage Mount Path
*Source: [005-storage-path.md](decisions/005-storage-path.md)*
*Status: APPROVED*
*Last Updated: 2025-10-15*

### Specification


Mount LittleFS at `/littlefs` path.



### Machine-Readable

```yaml
storage_mount_path: "/littlefs"
storage_type: "littlefs"
storage_label: "littlefs"

```
**Rationale:** See [005-storage-path.md](decisions/005-storage-path.md) for full context and alternatives considered.

---

## 6. Pattern Count Revision
*Source: [006-pattern-count-revision.md](decisions/006-pattern-count-revision.md)*
*Status: APPROVED*
*Last Updated: 2025-10-15*

### Specification


**Update minimum pattern count from 25 to 15.**

ADR-004 remains valid for `pattern_max_size: 256KB`. This ADR supersedes ONLY the `pattern_min_count` field.


**Rationale:**
1. **Captain Authority:** Direct clarification that 15 is sufficient
2. **Template Alignment:** Matches 15-template system design
3. **Storage Headroom:** 15 patterns @ ~100KB avg leaves comfortable margin
4. **Expandability:** Additional patterns can be added if memory permits
5. **Realistic:** 25 patterns was arbitrary/outdated requirement


### Machine-Readable

```yaml
pattern_max_size: 262144  # 256KB (UNCHANGED from ADR-004)
pattern_min_count: 15     # UPDATED: 15 templates minimum (was 25)
storage_reserved: 102400  # 100KB safety margin (UNCHANGED)

```
**Rationale:** See [006-pattern-count-revision.md](decisions/006-pattern-count-revision.md) for full context and alternatives considered.

---

## 7. Partition Table Alignment Correction
*Source: [007-partition-alignment-correction.md](decisions/007-partition-alignment-correction.md)*
*Status: APPROVED*
*Last Updated: 2025-10-16*

### Specification


**APPROVED: Use 64KB-aligned partition offsets as currently implemented in `partitions.csv`**

### Partition Layout (CORRECTED)

```csv
# Name,     Type, SubType, Offset,   Size,     Flags
nvs,        data, nvs,     0x9000,   0x6000,   # 24KB
otadata,    data, ota,     0xF000,   0x2000,   # 8KB
app0,       app,  ota_0,   0x20000,  0x180000, # 1.5MB @ 128KB (64KB aligned)
app1,       app,  ota_1,   0x1A0000, 0x180000, # 1.5MB @ 1.625MB (64KB aligned)
littlefs,   data, 0x82,    0x320000, 0x180000, # 1.5MB @ 3.125MB (64KB aligned)
```

### Total Flash Usage

```
End of littlefs: 0x320000 + 0x180000 = 0x4A0000 (4.75MB)
8MB flash remaining: 8MB - 4.75MB = 3.25MB unused (acceptable)
```

### Gap Analysis

**Wasted space from alignment:**
- Gap after otadata: 0x20000 - 0x11000 = 0xF000 (60KB unused)
- This is **mandatory waste** to satisfy ESP-IDF requirements
- No configuration option to reclaim this space


**Rationale:** See [007-partition-alignment-correction.md](decisions/007-partition-alignment-correction.md) for full context and alternatives considered.

---

## 8. LED Refresh Target Increase (60 FPS → 120 FPS)
*Source: [008-led-fps-increase.md](decisions/008-led-fps-increase.md)*
*Status: APPROVED*
*Last Updated: 2025-10-16*

### Specification


Increase LED refresh target to 120 FPS.



### Machine-Readable

```yaml
led_count: 320          # unchanged
led_type: WS2812B       # unchanged
led_fps_target: 120     # updated

```
**Rationale:** See [008-led-fps-increase.md](decisions/008-led-fps-increase.md) for full context and alternatives considered.

---


## Validation Status

This document validated against:
- **ADRs:** ✓ (8 decisions processed)
- **Code constants:** (Run validate-canon.sh)
- **Partition table:** (Run validate-canon.sh)

**Last Validation:** 2025-10-18 06:32:13 UTC
**Generator:** canon-generator v1.0.0
**Status:** GENERATED (validation pending)

---

## Change History

| Date | ADR | Change Summary |
|------|-----|----------------|
| 2025-10-15 | ADR-001 | ESP32-S3 Partition Table Layout |
| 2025-10-15 | ADR-002 | WebSocket Buffer Size |
| 2025-10-15 | ADR-003 | LED Count Standardization |
| 2025-10-15 | ADR-004 | Pattern Maximum Size |
| 2025-10-15 | ADR-005 | Storage Mount Path |
| 2025-10-15 | ADR-006 | Pattern Count Revision |
| 2025-10-16 | ADR-007 | Partition Table Alignment Correction |
| 2025-10-16 | ADR-008 | LED Refresh Target Increase (60 FPS → 120 FPS) |

---

**Document Signature:**
```
Generated: 2025-10-18 06:32:13 UTC
Source: ADR-001 through ADR-008
Generator: canon-generator v1.0.0
SHA256: f5c56dd6717c895df6a12c687a7b1def8c626b602c88cec844331f0de51ee15b
```
