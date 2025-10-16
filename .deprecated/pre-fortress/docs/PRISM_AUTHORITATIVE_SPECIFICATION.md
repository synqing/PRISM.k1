# PRISM K1 - AUTHORITATIVE TECHNICAL SPECIFICATION
*Version 1.0 - Final Reconciled Specification*
*Based on Forensic Evidence Analysis*

## Document Authority

This document represents the **FINAL AUTHORITATIVE SPECIFICATION** for PRISM K1 firmware, reconciling all conflicts through evidence-based forensic analysis. All development must follow these specifications.

**Supersedes:**
- `storage_layout.md` (partial)
- `template_catalog.md` (partial)
- Conflicting portions of `firmware_architecture.md`

**Incorporates:**
- PRD requirements from `prism-firmware-prd.txt`
- Hardware constraints from `esp32_constraints_research.md`
- Valid portions of existing documentation

---

## 1. PARTITION TABLE SPECIFICATION

### 1.1 Final Partition Layout

```csv
# ESP32-S3 Partition Table for PRISM K1
# Name,     Type, SubType, Offset,   Size,     Flags
nvs,        data, nvs,     0x9000,   0x6000,   # 24KB - WiFi credentials, config
otadata,    data, ota,     0xF000,   0x2000,   # 8KB - OTA state
app0,       app,  ota_0,   0x11000,  0x180000, # 1.5MB - Primary app
app1,       app,  ota_1,   0x191000, 0x180000, # 1.5MB - OTA app
littlefs,   data, 0x82,    0x311000, 0x180000, # 1.5MB - Pattern storage
# Total: 4,558,336 bytes used of 8,388,608 available (54%)
```

### 1.2 Partition Rationale

- **OTA Support:** Required per PRD TASK-30, enables safe remote updates
- **App Size:** 1.5MB each sufficient for ESP32-S3 firmware with optimization
- **Storage:** 1.5MB provides space for 25-35 patterns as specified
- **Future:** 3.8MB unused for expansion

---

## 2. FILESYSTEM SPECIFICATION

### 2.1 Mount Configuration

```c
#define STORAGE_PATH        "/littlefs"     // Mount point
#define STORAGE_PARTITION   "littlefs"      // Partition label
#define STORAGE_MAX_FILES   5                // Max open files
```

### 2.2 Directory Structure

```
/littlefs/
├── templates/                   # Deployed templates (from firmware)
│   ├── pattern_001.prism       # Ocean Wave (deployed)
│   ├── pattern_006.prism       # Rave Pulse (deployed)
│   └── manifest.json           # Deployment tracking
├── patterns/                    # User-uploaded custom patterns
│   ├── [uuid].prism            # Pattern files (UUID named)
│   └── index.json              # Pattern metadata cache
├── config/                      # System configuration
│   ├── network.json            # WiFi settings (backup to NVS)
│   └── device.json             # Device configuration
├── cache/                       # Future: CDN template cache
│   └── manifest.json           # Available templates from CDN
└── .temp/                       # Temporary upload directory
    └── [upload_id].tmp         # In-progress uploads
```

### 2.3 Storage Allocations

- **Total Space:** 1,572,864 bytes (1.5MB)
- **Filesystem Overhead:** ~102,400 bytes (100KB)
- **Usable Space:** ~1,470,464 bytes (1.43MB)
- **Pattern Capacity:** 25-35 patterns (40-60KB average)
- **Reserved Space:** 100KB minimum free required

---

## 3. WEBSOCKET PROTOCOL SPECIFICATION

### 3.1 Protocol Type

**BINARY TLV (Type-Length-Value)** as specified in PRD.

### 3.2 Frame Structure

```
┌────────┬────────────┬──────────────┬────────┐
│ TYPE   │ LENGTH     │ PAYLOAD      │ CRC32  │
│ 1 byte │ 2 bytes LE │ N bytes      │ 4 bytes│
└────────┴────────────┴──────────────┴────────┘
```

### 3.3 Message Types

```c
// File Transfer Messages
#define MSG_PUT_BEGIN    0x10  // Initiate upload
#define MSG_PUT_DATA     0x11  // Transfer chunk
#define MSG_PUT_END      0x12  // Complete upload

// Control Messages
#define MSG_CONTROL      0x20  // Pattern control
#define MSG_SET_PARAM    0x21  // Parameter update

// Status Messages
#define MSG_STATUS_REQ   0x30  // Request status
#define MSG_STATUS_RESP  0x31  // Status response
#define MSG_HEARTBEAT    0x32  // Keep-alive

// Template Messages
#define MSG_LIST_TMPL    0x40  // List templates
#define MSG_DEPLOY_TMPL  0x41  // Deploy template
#define MSG_PREVIEW_TMPL 0x42  // Preview template (live render)
```

### 3.4 Protocol Parameters

```c
#define WS_BUFFER_SIZE      4096   // Optimal frame size
#define WS_TIMEOUT_MS       5000   // Frame timeout
#define WS_MAX_CLIENTS      2      // Concurrent connections
#define WS_HEARTBEAT_MS     10000  // Keep-alive interval
```

---

## 4. BINARY FILE FORMAT (.prism)

### 4.1 File Structure

```c
typedef struct __attribute__((packed)) {
    // Header (32 bytes)
    uint32_t magic;         // 0x5053524D ('PRSM')
    uint16_t version;       // 0x0100 (1.0)
    uint16_t flags;         // bit0=compressed, bit1=encrypted
    uint32_t file_size;     // Total file size including header
    uint32_t data_size;     // Pattern data size (compressed)
    uint32_t orig_size;     // Original size (uncompressed)
    uint32_t crc32;         // CRC32 of entire file
    uint32_t reserved[2];   // Reserved for future use

    // Pattern data follows (variable length)
    uint8_t data[];         // Compressed pattern data
} prism_header_t;
```

### 4.2 Compression

- **Algorithm:** Heatshrink (embedded-friendly)
- **Window:** 10 bits (1KB)
- **Lookahead:** 5 bits (32 bytes)
- **Typical Ratio:** 40-60% compression

### 4.3 Pattern Data Format

Pattern data (after decompression) contains:
- Timeline definition
- Effect sequences
- Parameter curves
- Color palettes
- Timing information

---

## 5. TEMPLATE SYSTEM SPECIFICATION

**Reference Document**: See `.taskmaster/docs/template_system_architecture.md` for complete architecture.

### 5.1 Template Configuration

```c
#define TEMPLATE_COUNT      15              // Exactly 15 templates
#define TEMPLATE_STORAGE    FLASH_EMBEDDED  // Compiled into firmware
```

### 5.2 Template Categories

| Category | Count | IDs | Description | Power Draw |
|----------|-------|-----|-------------|------------|
| Ambient | 5 | 0x01-0x05 | Calm, atmospheric | Low (30-50%) |
| Energy | 5 | 0x06-0x0A | Dynamic, intense | High (70-100%) |
| Artistic | 5 | 0x0B-0x0F | Creative, unique | Medium (50-70%) |

**Design Rationale**: Serves 80% of users who never create custom patterns (per PRD line 8).

### 5.3 Storage Architecture (Three-Tier)

#### Tier 1: Firmware-Embedded Templates (Flash ROM)
```c
// Location: firmware/components/templates/template_registry.c
// Size: ~60KB compiled code for all 15 templates
// Access: Direct function pointer call (NO file I/O)

typedef struct {
    uint8_t id;                           // 0x01-0x0F
    const char* name;                     // "Ocean Wave"
    const char* category;                 // "ambient"
    render_func_t render;                 // Function pointer
    const char* metadata_json;            // Embedded JSON
} template_entry_t;

extern const template_entry_t TEMPLATE_REGISTRY[15];
```

**Characteristics**:
- Compiled into firmware binary
- Zero runtime initialization cost
- Survives factory reset & firmware updates
- Instant access (<10ms load time)
- No fragmentation risk

#### Tier 2: Deployed Templates (User-Requested .prism)
```bash
# Location: /littlefs/templates/
# Generated on-demand from Tier 1 templates when user deploys
# Size: 8-28KB compressed per template

/littlefs/templates/
├── pattern_001.prism    # Deployed "Ocean Wave"
├── pattern_006.prism    # Deployed "Rave Pulse"
└── manifest.json        # Deployment index
```

**Characteristics**:
- Generated on-demand via MSG_DEPLOY_TMPL (0x41)
- Persistent across reboots
- Can be deleted to free space
- Supports parameter customization
- Load time: <50ms (cached)

#### Tier 3: User Patterns (Custom Creations)
```bash
# Location: /littlefs/patterns/
# Uploaded from timeline editor or compiler
# Size: 40-200KB per pattern

/littlefs/patterns/
├── [uuid].prism         # Timeline editor exports
└── index.json           # Pattern metadata cache
```

### 5.4 Template Metadata Schema

```c
// Metadata for each template includes:
typedef struct {
    char id[16];                    // "pattern_001"
    uint8_t numeric_id;             // 1-15
    char name[32];                  // "Ocean Wave"
    char category[16];              // "ambient"
    char description[256];          // User-facing description
    param_def_t parameters[8];      // 3-8 parameters
    performance_profile_t perf;     // FPS, CPU, power
    memory_profile_t memory;        // Compressed/runtime sizes
} template_metadata_t;
```

**JSON Schema**: See `.taskmaster/docs/template_metadata_schema.json`

### 5.5 WebSocket Template API

#### MSG_LIST_TEMPLATES (0x40)
Returns metadata for all 15 templates (names, categories, parameters, etc.)

**Response Format**:
```c
struct {
    uint8_t count;              // 15
    struct {
        uint8_t id;             // 0x01-0x0F
        char name[32];
        char category[16];
        uint16_t metadata_len;
        char metadata_json[];   // Complete JSON metadata
    } templates[count];
} __attribute__((packed));
```

#### MSG_DEPLOY_TEMPLATE (0x41)
Converts firmware template → .prism file with optional parameter overrides

**Request Format**:
```c
struct {
    uint8_t template_id;        // 0x01-0x0F
    char save_as[32];           // Optional custom name
    uint8_t param_count;
    struct {
        char name[16];
        float value;
    } parameters[];
} __attribute__((packed));
```

**Response**: Returns file path and size on success

#### MSG_PREVIEW_TEMPLATE (0x42) - NEW
Live render template to LEDs without saving to storage

**Request Format**:
```c
struct {
    uint8_t template_id;        // 0x01-0x0F
    uint32_t duration_ms;       // 0 = until stopped
    uint8_t param_count;
    struct {
        char name[16];
        float value;
    } parameters[];
} __attribute__((packed));
```

**Use Case**: Instant preview with parameter adjustments before deploying

### 5.6 Template Registry Structure

```c
// firmware/components/templates/template_registry.c

const template_entry_t TEMPLATE_REGISTRY[15] = {
    // Ambient (0x01-0x05)
    {0x01, "Ocean Wave",      "ambient", &render_ocean_wave,      METADATA[0]},
    {0x02, "Aurora Borealis", "ambient", &render_aurora,          METADATA[1]},
    {0x03, "Breathing",       "ambient", &render_breathing,       METADATA[2]},
    {0x04, "Candlelight",     "ambient", &render_candlelight,     METADATA[3]},
    {0x05, "Starfield",       "ambient", &render_starfield,       METADATA[4]},

    // Energy (0x06-0x0A)
    {0x06, "Rave Pulse",      "energy",  &render_rave_pulse,      METADATA[5]},
    {0x07, "Strobe",          "energy",  &render_strobe,          METADATA[6]},
    {0x08, "Fire",            "energy",  &render_fire,            METADATA[7]},
    {0x09, "Lightning",       "energy",  &render_lightning,       METADATA[8]},
    {0x0A, "Plasma",          "energy",  &render_plasma,          METADATA[9]},

    // Artistic (0x0B-0x0F)
    {0x0B, "Rainbow Flow",    "artistic", &render_rainbow,        METADATA[10]},
    {0x0C, "Color Gradient",  "artistic", &render_gradient,       METADATA[11]},
    {0x0D, "Checkerboard",    "artistic", &render_checkerboard,   METADATA[12]},
    {0x0E, "Spiral",          "artistic", &render_spiral,         METADATA[13]},
    {0x0F, "Lava Lamp",       "artistic", &render_lava_lamp,      METADATA[14]}
};
```

### 5.7 Template Implementation Example

```c
// firmware/components/templates/src/template_patterns.c

// Ocean Wave pattern implementation
void render_ocean_wave(rgb_t* leds, uint32_t time_ms, const params_t* params) {
    for (int i = 0; i < NUM_LEDS; i++) {
        float phase = (float)i / NUM_LEDS * params->wave_count * TWO_PI;
        float time_offset = time_ms * 0.001 * params->speed;
        float wave = sin(phase + time_offset);
        float intensity_mod = (wave + 1.0) * 0.5 * params->intensity;

        HSV hsv = {
            .h = params->color_shift + (wave * params->depth * 30),
            .s = 200 + (wave * 55),
            .v = 100 + (intensity_mod * 155)
        };
        leds[i] = hsv_to_rgb(hsv);
    }
}
```

**Complete Algorithms**: See `.taskmaster/docs/template_specifications.md` for all 15 patterns.

### 5.8 Template Performance Requirements

```c
// All templates MUST meet these requirements:

#define TEMPLATE_FRAME_BUDGET_US    16667   // 60 FPS = 16.67ms max
#define TEMPLATE_LIST_LATENCY_MS    50      // List all templates
#define TEMPLATE_PREVIEW_START_MS   100     // Start live preview
#define TEMPLATE_DEPLOY_MAX_MS      2000    // Full .prism generation
#define TEMPLATE_LOAD_DEPLOYED_MS   100     // Load from storage
```

### 5.9 Template Cache Strategy

```c
// Hot cache for instant pattern switching

typedef enum {
    STORAGE_TIER_FIRMWARE,    // Render directly from code (fastest)
    STORAGE_TIER_DEPLOYED,    // Load from /templates/ (fast, cached)
    STORAGE_TIER_CUSTOM       // Load from /patterns/ (normal speed)
} storage_tier_t;

// Load priority:
// 1. Check if template ID (0x01-0x0F) → Direct render (NO file I/O)
// 2. Check hot cache → Return cached data
// 3. Check /templates/ → Load and cache
// 4. Check /patterns/ → Load normally

// Result:
// - Template patterns: <10ms load time
// - Deployed templates: <50ms load time
// - Custom patterns: <100ms load time
```

### 5.10 Future Extensibility: CDN Templates

**Phase 2 Feature** (not in v1.0):
- Download templates from PRISM CDN
- Store in `/littlefs/cache/`
- Template IDs 0x10+ (0x01-0x0F reserved for firmware)
- Offline mode: Fall back to firmware templates
- Enables seasonal/event patterns without firmware updates

---

## 6. MEMORY ALLOCATION SPECIFICATION

### 6.1 Heap Budget (200KB Available)

```c
// Static Allocations (Never freed)
#define HEAP_WIFI_STACK     50KB   // WiFi subsystem
#define HEAP_WS_BUFFERS     20KB   // WebSocket (no SSL)
#define HEAP_PATTERN_CACHE  60KB   // 3 patterns cached
#define HEAP_LED_BUFFERS    2KB    // Double buffered
#define HEAP_SYSTEM_POOLS   20KB   // Message queues, etc
#define HEAP_SAFETY_RESERVE 48KB   // Fragmentation buffer
// Total: 200KB
```

### 6.2 Stack Allocations

```c
#define STACK_MAIN          8192   // Main task
#define STACK_WIFI          4096   // WiFi task (minimum)
#define STACK_WEBSOCKET     6144   // WebSocket handler
#define STACK_LED_PLAYBACK  4096   // LED output task
#define STACK_STORAGE       3072   // File operations
// Total: 25KB
```

### 6.3 Memory Pools (Anti-Fragmentation)

```c
// Fixed-size allocation pools
typedef struct {
    uint8_t pool_4k[5][4096];    // 20KB - WebSocket frames
    uint8_t pool_1k[10][1024];   // 10KB - Small buffers
    uint8_t pool_256[20][256];   // 5KB - Messages
} memory_pools_t;
// Total: 35KB (part of HEAP_SYSTEM_POOLS)
```

---

## 7. CRITICAL CONFIGURATION CONSTANTS

```c
// System Configuration
#define DEVICE_NAME         "PRISM-K1"
#define FIRMWARE_VERSION    "1.0.0"
#define LED_COUNT           320         // K1-Lightwave spec
#define LED_FPS             60          // Target frame rate

// Network Configuration
#define WIFI_SSID_MAX       32
#define WIFI_PASS_MAX       64
#define WIFI_RETRY_MAX      10
#define MDNS_SERVICE        "_prism._tcp"
#define MDNS_HOSTNAME       "prism-k1"

// Storage Configuration
#define PATTERN_MAX_SIZE    262144      // 256KB max per pattern
#define PATTERN_MIN_FREE    102400      // 100KB minimum free
#define PATTERN_MAX_COUNT   35          // Maximum patterns

// Performance Targets
#define SWITCH_LATENCY_MS   100         // Pattern switch time
#define UPLOAD_SPEED_MIN    102400      // 100KB/s minimum
#define HEAP_PANIC_LEVEL    51200       // 50KB panic threshold
```

---

## 8. ERROR CODES

```c
// Storage Errors (0x01xx)
#define ERR_STORAGE_FULL        0x0100
#define ERR_STORAGE_CORRUPT     0x0101
#define ERR_PATTERN_NOT_FOUND   0x0102
#define ERR_PATTERN_TOO_LARGE   0x0103

// Network Errors (0x02xx)
#define ERR_WIFI_CONNECT        0x0200
#define ERR_WS_OVERFLOW         0x0201
#define ERR_WS_PROTOCOL         0x0202
#define ERR_WS_TIMEOUT          0x0203

// System Errors (0x03xx)
#define ERR_HEAP_LOW            0x0300
#define ERR_HEAP_FRAGMENT       0x0301
#define ERR_STACK_OVERFLOW      0x0302
#define ERR_WATCHDOG            0x0303

// Pattern Errors (0x04xx)
#define ERR_PATTERN_CORRUPT     0x0400
#define ERR_PATTERN_VERSION     0x0401
#define ERR_PATTERN_CRC         0x0402
#define ERR_PATTERN_DECOMPRESS  0x0403
```

---

## 9. IMPLEMENTATION PRIORITIES

### Phase 1: Foundation (Days 1-3)
1. ESP-IDF project setup with partition table
2. LittleFS mount and basic operations
3. WebSocket binary protocol handler
4. Memory pool initialization

### Phase 2: Core Features (Days 4-8)
1. Pattern upload via WebSocket
2. Pattern storage and indexing
3. LED output driver (RMT)
4. Pattern playback engine

### Phase 3: Templates & Polish (Days 9-12)
1. 15 embedded templates
2. Template deployment system
3. OTA update support
4. Performance optimization

---

## 10. VALIDATION CHECKLIST

### Pre-Implementation
- [ ] Partition table flashes successfully
- [ ] LittleFS mounts at `/littlefs`
- [ ] WebSocket accepts binary frames
- [ ] Memory pools allocate without fragmentation

### Feature Complete
- [ ] 15 templates present and playable
- [ ] Pattern upload works at >100KB/s
- [ ] Pattern switch <100ms
- [ ] LED output stable 60 FPS
- [ ] OTA update functional

### Production Ready
- [ ] 24-hour stress test passed
- [ ] No heap fragmentation
- [ ] <150KB heap usage
- [ ] 25+ patterns fit
- [ ] All error paths tested

---

## APPENDIX A: Decision Log

| Decision | Choice | Rationale | Evidence |
|----------|--------|-----------|----------|
| Partition Layout | Dual OTA + 1.5MB storage | PRD requires OTA, sufficient flash | PRD line 230 |
| WebSocket Protocol | Binary TLV | PRD explicit requirement | PRD line 23 |
| Storage Path | `/littlefs` | PRD specification | PRD line 115 |
| Template Count | 15 exactly | PRD requirement | PRD line 44 |
| Template Storage | Firmware embedded | PRD specifies "pre-loaded" | PRD line 103 |
| Header Size | 32 bytes | Sufficient, saves space | Analysis |

---

## APPENDIX B: File List

### Required Files to Create
```
firmware/
├── CMakeLists.txt
├── partitions.csv              # Use table from Section 1.1
├── sdkconfig.defaults           # ESP-IDF configuration
├── main/
│   ├── main.c
│   └── template_patterns.c     # 15 embedded templates
├── components/
│   ├── websocket/              # Binary protocol handler
│   ├── storage/                # LittleFS operations
│   ├── playback/              # LED & pattern engine
│   └── network/               # WiFi & mDNS
└── test/
    └── stress_test.c          # 24-hour validation
```

---

*This specification is final and authoritative based on forensic evidence analysis.*
*Last updated: 2025-10-15*
*Status: APPROVED FOR IMPLEMENTATION*