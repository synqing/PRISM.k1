# Forensic Specification Analysis Report
*Evidence-based determination of correct specifications*

## Executive Summary

After forensic analysis of requirements, hardware constraints, and technical feasibility, I have determined which specifications should be used. This analysis is based on **evidence, not assumptions**.

## 🔬 FORENSIC EVIDENCE

### Evidence Source 1: Original PRD Requirements

**From `prism-firmware-prd.txt`:**

| Specification | Line # | Exact Quote | Implication |
|---------------|--------|-------------|-------------|
| Storage Size | 16, 30 | "25-35 patterns in 1.47MB LittleFS" / "Partition: 1.47MB usable space" | Storage should be ~1.5MB |
| Protocol Type | 23 | "Binary protocol with TLV message structure" | WebSocket should use BINARY, not JSON |
| Storage Path | 115 | `#define STORAGE_PATH "/littlefs"` | Mount point should be `/littlefs` |
| Templates | 44 | "15 patterns pre-loaded in flash" | Exactly 15 templates required |
| OTA Support | 230, 304 | "TASK-30: Create OTA update system" / "OTA update functional" | OTA IS required |
| Flash Size | 61 | "Flash: 8MB (6.5MB for firmware + storage)" | 8MB total flash available |
| Header Size | Not specified | - | No requirement for header size |

### Evidence Source 2: ESP32-S3 Hardware Constraints

**From `esp32_constraints_research.md`:**

| Constraint | Value | Source | Impact |
|------------|-------|--------|--------|
| Total Flash | 8MB | ESP32-S3 spec | Maximum space for all partitions |
| Usable Heap | ~200-230KB | Measured | After WiFi/system overhead |
| WebSocket Frame | 4KB optimal | Testing | Balances memory vs performance |
| DMA Buffer | 14.4KB | Calculated | For 150 LEDs (actually 320 in K1) |
| Fragmentation | 78% after 24h | Measured | Critical issue without pools |

### Evidence Source 3: Partition Math

**8MB Flash Breakdown Analysis:**

```
Total Flash: 8,388,608 bytes (0x800000)

Required Partitions:
├── Bootloader:       36,864 bytes  (0x9000)
├── NVS:              24,576 bytes  (0x6000)
├── OTA Data:          8,192 bytes  (0x2000)
├── App0 (OTA):    1,572,864 bytes  (0x180000) ~1.5MB
├── App1 (OTA):    1,572,864 bytes  (0x180000) ~1.5MB
├── LittleFS:      1,572,864 bytes  (0x180000) ~1.5MB
├── Reserved:         65,536 bytes  (0x10000)
└── Unused:        3,534,848 bytes

Feasibility: ✅ FITS with 3.5MB spare
```

**Alternative without OTA:**
```
├── Bootloader:       36,864 bytes
├── NVS:              24,576 bytes
├── PHY:               4,096 bytes
├── Single App:    2,097,152 bytes  (0x200000) 2MB
├── LittleFS:      1,572,864 bytes  (0x180000) 1.5MB
└── Unused:        4,653,056 bytes

Feasibility: ✅ FITS with 4.6MB spare (wasteful)
```

### Evidence Source 4: Protocol Performance

**Binary vs JSON WebSocket Analysis:**

| Aspect | Binary TLV | JSON | Evidence |
|--------|------------|------|----------|
| Message Size | 100 bytes | 250 bytes | For typical command |
| Parse Time | <1ms | 3-5ms | ESP32 measured |
| Memory Usage | Fixed buffers | Dynamic strings | Fragmentation risk |
| Debugging | Harder | Easier | Developer experience |
| PRD Requirement | ✅ REQUIRED | ❌ Not specified | Line 23 explicit |

### Evidence Source 5: Storage Path Analysis

**Path Configuration Impact:**

| Path | Code References | Migration Impact | Consistency |
|------|-----------------|------------------|-------------|
| `/littlefs` | PRD line 115 | None - as specified | ✅ Original |
| `/prism` | New docs only | Requires code changes | ❌ Deviation |

## 📊 FORENSIC CONCLUSIONS

### 1. **Partition Table - USE OTA VERSION**

**Evidence:**
- PRD explicitly requires OTA (TASK-30, line 230)
- 8MB flash has plenty of space for dual 1.5MB apps + 1.5MB storage
- OTA prevents bricking during updates
- Industry standard for production devices

**Correct Partition Table:**
```csv
# Name,   Type, SubType, Offset,   Size,     Flags
nvs,      data, nvs,     0x9000,   0x6000,
otadata,  data, ota,     0xF000,   0x2000,
app0,     app,  ota_0,   0x11000,  0x180000,  # 1.5MB
app1,     app,  ota_1,   0x191000, 0x180000,  # 1.5MB
littlefs, data, 0x82,    0x311000, 0x180000,  # 1.5MB
```

**Rationale:**
- Matches PRD requirement for OTA
- Provides 1.5MB storage as specified
- Uses standard ESP-IDF OTA structure
- Leaves 3.8MB unused (future expansion)

### 2. **WebSocket Protocol - USE BINARY TLV**

**Evidence:**
- PRD line 23: "Binary protocol with TLV message structure"
- PRD lines 122-132: Binary format specification
- Binary uses 60% less bandwidth
- Binary prevents JSON parsing fragmentation

**Correct Protocol:** Binary TLV as originally documented

### 3. **Storage Path - USE `/littlefs`**

**Evidence:**
- PRD line 115: `#define STORAGE_PATH "/littlefs"`
- Existing code may depend on this path
- No technical reason to change

**Correct Path:** `/littlefs/`

### 4. **Binary File Format - USE COMPACT HEADER**

**Evidence:**
- No PRD requirement for specific header size
- Smaller header = more pattern space
- 32 bytes sufficient for all metadata

**Correct Format:**
```c
typedef struct {
    uint32_t magic;       // 'PRSM' (0x5053524D) - from PRD
    uint16_t version;     // 0x0100
    uint16_t flags;       // Compression, etc.
    uint32_t size;        // Total file size
    uint32_t crc32;       // Integrity check
    uint32_t data_offset; // Start of pattern data
    uint32_t data_size;   // Size of pattern data
    uint8_t reserved[8];  // Future use
} prism_header_t;  // 32 bytes total
```

### 5. **Template Count - USE 15 TEMPLATES**

**Evidence:**
- PRD line 44: "15 patterns pre-loaded in flash"
- Consistent throughout PRD
- Week 3 tasks specify 5+5+5 templates

**Correct Count:** 15 templates exactly

### 6. **Template Storage - EMBED IN FIRMWARE**

**Evidence:**
- Templates are "pre-loaded in flash" (PRD line 44)
- `template_patterns.c` with "hardcoded patterns" (PRD line 103)
- No LittleFS space allocated for templates

**Correct Approach:** Compile templates into firmware, not as .prism files

## ❌ SPECIFICATION ERRORS TO CORRECT

### In My New Documents:
1. ❌ Mount point `/prism/` → ✅ Should be `/littlefs/`
2. ❌ JSON WebSocket → ✅ Should be Binary TLV
3. ❌ 1MB storage → ✅ Should be 1.5MB
4. ❌ Magic 'PRIS' → ✅ Should be 'PRSM'

### In Old Documents:
1. ❌ No OTA support → ✅ Should have OTA
2. ❌ 64-byte header → ✅ Should be 32-byte
3. ❌ 20 templates → ✅ Should be 15

## ✅ FINAL AUTHORITATIVE SPECIFICATIONS

Based on forensic evidence, these are the CORRECT specifications:

### Partition Table (FINAL)
```csv
# Name,     Type, SubType, Offset,   Size,     Flags
nvs,        data, nvs,     0x9000,   0x6000,
otadata,    data, ota,     0xF000,   0x2000,
app0,       app,  ota_0,   0x11000,  0x180000, # 1.5MB
app1,       app,  ota_1,   0x191000, 0x180000, # 1.5MB
littlefs,   data, 0x82,    0x311000, 0x180000, # 1.5MB
```

### Configuration Constants (FINAL)
```c
#define STORAGE_PATH        "/littlefs"
#define STORAGE_SIZE        0x180000    // 1.5MB
#define WS_PROTOCOL         BINARY_TLV
#define TEMPLATE_COUNT      15
#define PATTERN_MAGIC       0x5053524D  // 'PRSM'
#define HEADER_SIZE         32
```

### WebSocket Protocol (FINAL)
- Binary TLV format as specified in original `websocket_protocol.md`
- Message types: 0x10-0x12 (file transfer), 0x20 (control), 0x30 (status)
- 4KB frame size optimal

### Memory Allocations (FINAL)
- Total heap available: ~200KB after system
- WebSocket buffers: 20KB (no SSL)
- Pattern cache: 60KB (3 patterns)
- LED buffers: 2KB (double buffered)
- Safety reserve: 80KB

## 📋 ACTION ITEMS

1. **Update `firmware_architecture.md`:**
   - Change partition table to match above
   - Change `/prism/` to `/littlefs/`
   - Update to 1.5MB storage

2. **Revert `websocket_protocol.md`:**
   - Keep original binary TLV version
   - Delete JSON version I created

3. **Update `template_specifications.md`:**
   - Confirm 15 templates only
   - Add note about firmware embedding

4. **Create `partitions.csv`:**
   - Use the final partition table above

## 🏁 CONCLUSION

The forensic analysis reveals that:
- **Original `storage_layout.md`** had the right storage size (1.5MB) but wrong partition layout (no OTA)
- **Original `websocket_protocol.md`** had the CORRECT binary protocol
- **My new documents** had better OTA support but wrong protocol and paths

The truth is a combination: Use the partition table with OTA, binary WebSocket protocol, `/littlefs` path, 1.5MB storage, and 15 embedded templates.

---
*Analysis completed: 2025-10-15*
*Evidence-based, not assumption-based*