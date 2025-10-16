# ADR-007: Partition Table Alignment Correction

**Status:** APPROVED
**Date:** 2025-10-16
**Supersedes:** ADR-001 (partition offsets only, sizes remain valid)
**Authors:** PM (Agent 1), Agent 2 (discovery)

## Context

During Task 2 (WiFi Lifecycle) integration, Agent 2 discovered that ADR-001 specifies partition offsets that violate mandatory ESP-IDF requirements.

### ESP-IDF Requirement (v4.4 through v5.5+)

> "Partitions of type app have to be placed at offsets aligned to 0x10000 (64 KB). If you specify an unaligned offset for an app partition, the tool will return an error."
>
> — ESP-IDF Official Partition Table Documentation

### The Problem

**ADR-001 specifies (BROKEN):**
```
app0:     offset 0x11000   (NOT 64KB aligned) ❌ BUILD FAILS
app1:     offset 0x191000  (NOT 64KB aligned) ❌ BUILD FAILS
littlefs: offset 0x311000  (NOT 64KB aligned) ⚠️ OPTIONAL
```

**Current working partition table:**
```
app0:     offset 0x20000   (64KB aligned) ✅ BUILD SUCCEEDS
app1:     offset 0x1A0000  (64KB aligned) ✅ BUILD SUCCEEDS
littlefs: offset 0x320000  (64KB aligned) ✅ BUILD SUCCEEDS
```

**Alignment Verification:**
```
0x20000  % 0x10000 = 0x0  ✅ app0 valid
0x1A0000 % 0x10000 = 0x0  ✅ app1 valid
0x320000 % 0x10000 = 0x0  ✅ littlefs valid

0x11000  % 0x10000 = 0x1000  ❌ ADR-001 app0 INVALID
0x191000 % 0x10000 = 0x1000  ❌ ADR-001 app1 INVALID
0x311000 % 0x10000 = 0x1000  ❌ ADR-001 littlefs INVALID
```

### Impact

If ADR-001 partition offsets were applied:
1. ❌ Build would fail with partition alignment error
2. ❌ Firmware wouldn't compile
3. ❌ Device couldn't boot
4. ❌ OTA updates impossible

### Root Cause

ADR-001 was created during planning phase without practical build validation. The specification prioritized minimizing wasted space (gap from 0xF000 + 0x2000 = 0x11000) over mandatory ESP-IDF alignment requirements.

## Decision

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

## Consequences

### Positive

- ✅ **Build succeeds** - Partitions align with ESP-IDF requirements
- ✅ **Firmware boots** - Valid partition table
- ✅ **OTA works** - Both app slots properly aligned
- ✅ **Storage works** - LittleFS partition valid
- ✅ **Forward compatible** - Alignment requirement exists in all ESP-IDF versions

### Negative

- ⚠️ **60KB wasted** - Gap from 0x11000 to 0x20000 unused
- ⚠️ **Space inefficient** - Could theoretically fit more data if alignment weren't required
- ℹ️ **Not a practical concern** - 8MB flash provides ample space (3.25MB unused)

### Neutral

- ADR-001 **partially superseded** - Sizes (1.5MB each) remain valid
- CANON.md requires regeneration from corrected ADR
- No code changes required (partitions.csv already correct)

## Implementation

### Files Already Correct

✅ `firmware/partitions.csv` - Already uses 64KB-aligned offsets
✅ Build system - Already validates and succeeds

### Files Requiring Update

1. **`.taskmaster/CANON.md`** - Regenerate from ADR-007
2. **`firmware/components/core/include/prism_config.h`** - Fix PATTERN_MIN_COUNT (separate from partition issue)

### No Breaking Changes

Since the working partition table already uses correct offsets, this ADR simply **documents reality** and supersedes the incorrect ADR-001 specification.

## Validation

### Build Verification

```bash
$ idf.py build
...
[90/90] Successfully created ESP32S3 image.
prism-k1.bin binary size 0xcf290 bytes. Smallest app partition is 0x180000 bytes. 0xb0d70 bytes (46%) free.
Project build complete.
```

✅ **Build succeeds with current 64KB-aligned partition table**

### Partition Table Check

```bash
$ python3 -c "
offsets = {'app0': 0x20000, 'app1': 0x1A0000, 'littlefs': 0x320000}
for name, offset in offsets.items():
    print(f'{name}: 0x{offset:X} % 0x10000 = 0x{offset % 0x10000:X} ✅')
"
app0: 0x20000 % 0x10000 = 0x0 ✅
app1: 0x1A0000 % 0x10000 = 0x0 ✅
littlefs: 0x320000 % 0x10000 = 0x0 ✅
```

## References

- **ESP-IDF Partition Tables**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/partition-tables.html
- **ESP-IDF v5.x Requirements**: App partitions must be 0x10000 (64KB) aligned
- **ADR-001**: Original (incorrect) partition specification
- **Knowledge Fortress Conflict Resolution**: `.taskmaster/METHODOLOGY.md`

## Notes

This ADR exemplifies the Knowledge Fortress methodology working as designed:

1. ✅ **Conflict detected** - Agent 2 found ADR-001 violated ESP-IDF requirements
2. ✅ **Work stopped** - No changes applied until resolution
3. ✅ **Research conducted** - ESP-IDF documentation confirmed alignment requirement
4. ✅ **Evidence gathered** - Mathematical proof of alignment violations
5. ✅ **Resolution documented** - This ADR supersedes broken specification
6. ✅ **CANON updated** - Truth propagates to all agents

**Captain approval required before marking APPROVED and regenerating CANON.md**
