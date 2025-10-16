# AGENT 2: Parser → Shapes → Validation Pipeline
**Mission: File Format Extension & Shape Generation**

## Your Sequential Pipeline (17 subtasks)

```
Task 13: .prism v1.1 Parser (5 subtasks) → File Format
   ↓
Task 15: PROGRESSIVE Mode (6 subtasks) → Shape Generation
   ↓
Task 16: Hardware Validation (6 subtasks) → Camera-Based Validation
```

**Total: 17 subtasks, ~3-5 hours**

---

## 📋 WORKFLOW FOR EACH SUBTASK

```bash
# 1. Pull latest from Agent 1
git pull --rebase

# 2. Check task details
task-master show <task-id>

# 3. Implement
# ... code code code ...

# 4. Commit
git add .
git commit -m "feat(task-X): complete subtask X.Y - description"
git push

# 5. Mark complete
task-master set-status --id=X.Y --status=done
```

---

## 🎯 TASK 13: .prism v1.1 Header & Parser (5 subtasks)

**Goal:** Extend .prism format from v1.0 (64 bytes) to v1.1 (70 bytes)
**Location:** `firmware/components/storage/`

### Full Research Available:
```bash
task-master show 13
# Read the research section for complete details
```

### Quick Overview:
- Extend header from 64→70 bytes (+6 bytes for motion/sync)
- Add TLV types 0x20 (MOTION) and 0x21 (SYNC)
- Backward compatible with v1.0 files
- Update CRC32 calculation to include new fields

---

### Subtask 13.1: Extend prism_header_t to 70 bytes

**Update:** `firmware/components/storage/include/pattern_storage.h`

```c
#include "pattern_metadata.h"  // Gets prism_pattern_meta_v11_t

// Original v1.0 header (64 bytes)
typedef struct __attribute__((packed)) {
    uint8_t magic[4];              // "PRSM"
    uint16_t version;              // 0x0100 for v1.0, 0x0101 for v1.1
    uint16_t led_count;            // 160
    uint32_t frame_count;
    uint32_t fps;
    uint8_t color_format;
    uint8_t compression;
    uint16_t reserved1;
    uint32_t crc32;
    uint8_t padding[40];
} prism_header_v10_t;

// Extended v1.1 header (70 bytes)
typedef struct __attribute__((packed)) {
    prism_header_v10_t base;       // 64 bytes
    prism_pattern_meta_v11_t meta; // 16 bytes (but we only use 6)
} prism_header_v11_t;

_Static_assert(sizeof(prism_header_v10_t) == 64, "v1.0 header must be 64 bytes");
_Static_assert(sizeof(prism_header_v11_t) == 80, "v1.1 header must be 80 bytes");
```

**Commit:** `feat(task-13): complete subtask 13.1 - extend header to v1.1`

---

### Subtask 13.2: Add TLV types for motion/sync

**Update:** `firmware/components/network/protocol_parser.c`

```c
// Add to TLV type enum
#define PRISM_TLV_MOTION  0x20
#define PRISM_TLV_SYNC    0x21

// Add validation
static bool validate_motion_tlv(uint8_t motion_value) {
    return PRISM_MOTION_IS_VALID(motion_value);
}

static bool validate_sync_tlv(uint8_t sync_value) {
    return PRISM_SYNC_IS_VALID(sync_value);
}
```

**Commit:** `feat(task-13): complete subtask 13.2 - add motion/sync TLV types`

---

### Subtask 13.3: Implement backward compatibility

**Add parser logic:**

```c
esp_err_t parse_prism_header(const uint8_t *data, size_t len,
                               prism_header_v11_t *out_header) {
    if (len < 64) {
        return ESP_ERR_INVALID_SIZE;
    }

    const prism_header_v10_t *v10 = (const prism_header_v10_t*)data;

    // Check version
    if (v10->version == 0x0100) {
        // v1.0 file - copy base, zero motion/sync
        memcpy(&out_header->base, v10, sizeof(prism_header_v10_t));
        memset(&out_header->meta, 0, sizeof(prism_pattern_meta_v11_t));
        out_header->meta.version = 0x00;
        out_header->meta.sync_mode = PRISM_SYNC_SYNC;  // Default
        out_header->meta.motion_direction = PRISM_MOTION_STATIC;
    } else if (v10->version == 0x0101 && len >= 70) {
        // v1.1 file - copy everything
        memcpy(out_header, data, sizeof(prism_header_v11_t));
    } else {
        return ESP_ERR_NOT_SUPPORTED;
    }

    return ESP_OK;
}
```

**Commit:** `feat(task-13): complete subtask 13.3 - add v1.0 backward compat`

---

### Subtask 13.4: Update CRC32 calculation

```c
uint32_t calculate_header_crc(const prism_header_v11_t *header) {
    // Include base + motion/sync in CRC
    const uint8_t *data = (const uint8_t*)header;
    size_t crc_len = offsetof(prism_header_v11_t, base.crc32);

    // If v1.1, include meta fields
    if (header->base.version == 0x0101) {
        crc_len += 6;  // motion + sync + params (first 6 bytes of meta)
    }

    return esp_rom_crc32_le(0, data, crc_len);
}
```

**Commit:** `feat(task-13): complete subtask 13.4 - update CRC for v1.1`

---

### Subtask 13.5: Create parser unit tests

**File:** `firmware/components/storage/test/test_prism_parser.c`

```c
TEST_CASE("Parser handles v1.0 files", "[storage]") {
    uint8_t v10_data[64];
    memset(v10_data, 0, sizeof(v10_data));

    prism_header_v10_t *v10 = (prism_header_v10_t*)v10_data;
    memcpy(v10->magic, "PRSM", 4);
    v10->version = 0x0100;

    prism_header_v11_t parsed;
    esp_err_t err = parse_prism_header(v10_data, 64, &parsed);

    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(PRISM_SYNC_SYNC, parsed.meta.sync_mode);
    TEST_ASSERT_EQUAL(PRISM_MOTION_STATIC, parsed.meta.motion_direction);
}

TEST_CASE("Parser handles v1.1 files", "[storage]") {
    prism_header_v11_t v11;
    memset(&v11, 0, sizeof(v11));
    memcpy(v11.base.magic, "PRSM", 4);
    v11.base.version = 0x0101;
    v11.meta.version = 0x01;
    v11.meta.sync_mode = PRISM_SYNC_PROGRESSIVE;
    v11.meta.motion_direction = PRISM_MOTION_LEFT;

    prism_header_v11_t parsed;
    esp_err_t err = parse_prism_header((uint8_t*)&v11, sizeof(v11), &parsed);

    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(PRISM_SYNC_PROGRESSIVE, parsed.meta.sync_mode);
    TEST_ASSERT_EQUAL(PRISM_MOTION_LEFT, parsed.meta.motion_direction);
}
```

**Commit:** `test(task-13): complete subtask 13.5 - add parser tests`

**Mark complete:** `task-master set-status --id=13 --status=done`

---

## 🎯 TASK 15: PROGRESSIVE Mode (6 subtasks)

**Goal:** Linear interpolation for triangle/wedge shape generation
**Location:** `firmware/components/playback/`

### Full Research:
```bash
task-master show 15
```

### Key Points:
- Linear interpolation: `delay[i] = start + (end-start)*i/159`
- Triangle: ramp up then down
- Wedge: ramp up then hold
- Motion direction: LEFT (0→159), RIGHT (159→0), EDGE/CENTER (mirrored)

---

### Subtask 15.1: build_progressive_ramp()

**Add to** `prism_temporal.c`:

```c
// Build linear delay ramp
static void build_progressive_ramp(uint16_t start_ms, uint16_t end_ms,
                                    uint16_t *table, size_t count) {
    const int32_t span = (int32_t)end_ms - (int32_t)start_ms;

    for (uint16_t i = 0; i < count; ++i) {
        const int32_t scaled = (span * i) / (count - 1);
        table[i] = (uint16_t)(start_ms + scaled);
    }
}
```

**Commit:** `feat(task-15): complete subtask 15.1 - implement progressive ramp`

---

### Subtask 15.2: Motion direction support

```c
// Apply motion direction to index
static size_t apply_motion_direction(size_t index, size_t count,
                                      prism_motion_t direction) {
    switch (direction) {
    case PRISM_MOTION_LEFT:
        return index;  // 0→159
    case PRISM_MOTION_RIGHT:
        return (count - 1) - index;  // 159→0
    case PRISM_MOTION_EDGE:
    case PRISM_MOTION_CENTER:
        // Mirror around center
        return (index < count/2) ? index : (count - 1) - index;
    default:
        return index;
    }
}
```

**Commit:** `feat(task-15): complete subtask 15.2 - add motion direction`

---

### Subtask 15.3-15.6: Shape builders + tests

Reference full task details:
```bash
task-master show 15
```

Implement:
- Triangle builder (two-phase ramp)
- Wedge builder (ramp + plateau)
- Unit tests for endpoints, monotonicity
- Golden array snapshot tests

**Commit each subtask separately**

**Mark complete:** `task-master set-status --id=15 --status=done`

---

## 🎯 TASK 16: Hardware Validation (6 subtasks)

**Goal:** Camera-based validation of temporal sequencing
**Location:** `tools/validation/`

### Overview:
- Chronos 2.1 high-speed camera (240 FPS)
- Python/OpenCV timing extraction
- Phi phenomenon validation (60-150ms)
- Performance profiling with esp_timer

---

### Subtasks 16.1-16.6:

Full details in research:
```bash
task-master show 16
```

Key deliverables:
1. Camera setup guide
2. `analyze_progressive.py` script
3. Phi phenomenon observer trials
4. esp_timer instrumentation
5. Visual shape verification
6. Artifact archival in TaskMaster

**Create Python scripts, run hardware tests, document results**

**Mark complete:** `task-master set-status --id=16 --status=done`

---

## 🎉 PIPELINE COMPLETE!

**Total: 17 subtasks across 3 tasks**

Final validation:
```bash
cd firmware
idf.py build
idf.py unity
```

**Great work! Parser, shapes, and validation complete! 🚀**
