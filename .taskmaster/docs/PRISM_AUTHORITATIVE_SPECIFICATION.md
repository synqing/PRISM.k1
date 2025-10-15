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
├── patterns/                    # User-uploaded patterns
│   ├── [uuid].prism            # Pattern files (UUID named)
│   └── index.json              # Pattern metadata cache
├── config/                     # System configuration
│   ├── network.json            # WiFi settings (backup to NVS)
│   └── device.json             # Device configuration
└── .temp/                      # Temporary upload directory
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

## 5. TEMPLATE SPECIFICATIONS

### 5.1 Template Configuration

```c
#define TEMPLATE_COUNT      15              // Exactly 15 templates
#define TEMPLATE_STORAGE    FLASH_EMBEDDED  // Compiled into firmware
```

### 5.2 Template Categories

| Category | Count | IDs | Description |
|----------|-------|-----|-------------|
| Ambient | 5 | 0x01-0x05 | Calm, atmospheric |
| Energy | 5 | 0x06-0x0A | Dynamic, intense |
| Special | 5 | 0x0B-0x0F | Unique, showcase |

### 5.3 Template Implementation

Templates are:
- Compiled into firmware as constant data
- Stored in `template_patterns.c`
- Accessible via function pointers
- Not stored as .prism files
- Deployed to LittleFS on user request

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