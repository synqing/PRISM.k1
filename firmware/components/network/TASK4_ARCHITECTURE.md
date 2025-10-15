# Task 4: TLV Protocol Parser - Architecture & Implementation Plan

**Date**: 2025-10-16
**Agent**: Agent 2
**Status**: Architectural Planning
**Task**: Implement TLV protocol parser and command dispatcher
**Complexity**: 7/10
**Dependencies**: Task 3 (WebSocket Server) ✅ COMPLETE

---

## Executive Summary

Task 4 implements the **application-layer protocol** for PRISM.K1, sitting on top of the WebSocket transport layer (Task 3). This protocol parser decodes binary TLV (Type-Length-Value) messages, validates integrity with CRC32 checksums, manages upload sessions with state machines, and dispatches commands to storage and playback subsystems.

**Key Challenges**:
- Binary protocol parsing with strict validation
- Stateful upload session management (PUT_BEGIN → PUT_DATA → PUT_END)
- 256KB pattern upload buffering and streaming
- Integration with Task 5 storage (currently in recovery - stub initially)
- Thread-safe command dispatching from WebSocket context
- CRC32 accumulation across fragmented uploads

**Integration Points**:
- **Input**: Task 3 WebSocket `handle_ws_frame()` at line 1238
- **Output**: Task 5 Storage API (pattern CRUD operations)
- **Output**: Task 8 Playback API (pattern loading and effects control)

---

## Table of Contents

1. [TLV Protocol Specification](#1-tlv-protocol-specification)
2. [Message Types & Flow](#2-message-types--flow)
3. [State Machine Design](#3-state-machine-design)
4. [Data Structures](#4-data-structures)
5. [Function Architecture](#5-function-architecture)
6. [CRC32 Validation Strategy](#6-crc32-validation-strategy)
7. [Upload Session Management](#7-upload-session-management)
8. [Integration with Task 3 (WebSocket)](#8-integration-with-task-3-websocket)
9. [Integration with Task 5 (Storage)](#9-integration-with-task-5-storage)
10. [Integration with Task 8 (Playback)](#10-integration-with-task-8-playback)
11. [Memory Management Strategy](#11-memory-management-strategy)
12. [Error Handling & Error Codes](#12-error-handling--error-codes)
13. [Testing Strategy](#13-testing-strategy)
14. [Implementation Phases](#14-implementation-phases)
15. [Open Questions & Risks](#15-open-questions--risks)

---

## 1. TLV Protocol Specification

### Frame Format

All messages follow the binary TLV structure:

```
[TYPE:1] [LENGTH:2] [PAYLOAD:N] [CRC32:4]
```

**Field Details**:
- `TYPE` (1 byte): Message type identifier (see section 2)
- `LENGTH` (2 bytes, little-endian): Payload size in bytes (0-65535)
- `PAYLOAD` (N bytes): Variable-length message data
- `CRC32` (4 bytes, little-endian): CRC32 checksum of [TYPE][LENGTH][PAYLOAD]

**Total Frame Size**: 7 + N bytes (minimum 7 bytes for zero-length payload)

**Constraints**:
- Maximum frame size: 4096 bytes (WebSocket buffer size from ADR-002)
- Maximum pattern upload: 256KB total (ADR-004), requires chunking
- Minimum frame size: 7 bytes (TYPE + LENGTH + CRC32)

**Endianness**: Little-endian for all multi-byte fields (ESP32 native byte order)

**CRC32 Algorithm**: ESP-IDF ROM function `esp_rom_crc32_le(uint32_t crc, const uint8_t *buf, uint32_t len)`
- Polynomial: CRC-32/ISO-HDLC (0xEDB88320)
- Initial value: 0xFFFFFFFF
- Final XOR: 0xFFFFFFFF

---

## 2. Message Types & Flow

### 2.1 Pattern Upload Messages (0x10-0x12)

**PUT_BEGIN (0x10)**: Initiates a pattern upload session
```
TYPE:     0x10
LENGTH:   6 bytes
PAYLOAD:  [pattern_id:2] [total_size:4]
CRC32:    4 bytes

Fields:
- pattern_id: 2-byte little-endian pattern ID (1-25 per ADR-006)
- total_size: 4-byte little-endian total pattern size (max 256KB per ADR-004)
```

**PUT_DATA (0x11)**: Streams pattern data chunks
```
TYPE:     0x11
LENGTH:   N bytes (variable, max 4089 bytes per WebSocket buffer)
PAYLOAD:  [raw_pattern_data:N]
CRC32:    4 bytes

Notes:
- Multiple PUT_DATA messages compose a single pattern
- Client must send PUT_BEGIN first to establish session
- CRC32 accumulates across all PUT_DATA messages
```

**PUT_END (0x12)**: Finalizes pattern upload
```
TYPE:     0x12
LENGTH:   4 bytes
PAYLOAD:  [final_crc32:4]
CRC32:    4 bytes (of this message frame)

Fields:
- final_crc32: Client's CRC32 of entire pattern data (all PUT_DATA payloads)

Validation:
- Server compares final_crc32 with accumulated CRC32
- Mismatch triggers error response and session abort
```

### 2.2 Control Messages (0x20-0x21)

**CONTROL_PLAY (0x20)**: Start playback with pattern
```
TYPE:     0x20
LENGTH:   2 bytes
PAYLOAD:  [pattern_id:2]
CRC32:    4 bytes

Action:
- Load pattern from storage (Task 5)
- Start LED playback (Task 8)
- Send STATUS response
```

**CONTROL_STOP (0x21)**: Stop current playback
```
TYPE:     0x21
LENGTH:   0 bytes
PAYLOAD:  (none)
CRC32:    4 bytes

Action:
- Stop LED playback (Task 8)
- Clear pattern from RAM cache (Task 7)
- Send STATUS response
```

### 2.3 Status & Error Messages (0x30, 0xFF)

**STATUS (0x30)**: Status update from server
```
TYPE:     0x30
LENGTH:   N bytes (variable)
PAYLOAD:  [status_code:1] [message:N-1]
CRC32:    4 bytes

Status Codes:
- 0x00: Success
- 0x01: Upload in progress
- 0x02: Playback active
- 0x03: Idle/Ready
```

**ERROR (0xFF)**: Error notification from server
```
TYPE:     0xFF
LENGTH:   N bytes (variable)
PAYLOAD:  [error_code:1] [message:N-1]
CRC32:    4 bytes

Error Codes (per ADR-002):
- 0x01: Invalid frame format
- 0x02: CRC mismatch
- 0x03: Pattern size exceeded (>256KB)
- 0x04: Storage full (>15 patterns per ADR-006)
- 0x05: Pattern not found
```

---

## 3. State Machine Design

### 3.1 Upload Session States

```
IDLE ──PUT_BEGIN──> RECEIVING ──PUT_DATA──> RECEIVING
                        │                       │
                        │                       │
                        └─────PUT_END──────> VALIDATING
                                                │
                    ┌───────────────────────────┴───────────────────────┐
                    │                                                   │
                VALIDATE_OK                                        VALIDATE_FAIL
                    │                                                   │
                    v                                                   v
                STORING ──success──> IDLE                          ERROR ──> IDLE
                    │
                    └─fail──> ERROR ──> IDLE
```

**State Definitions**:

- **IDLE**: No active upload session
  - Accepts: PUT_BEGIN, CONTROL_PLAY, CONTROL_STOP
  - Rejects: PUT_DATA, PUT_END (error: "No active session")

- **RECEIVING**: Pattern data streaming
  - Accepts: PUT_DATA (accumulate), PUT_END (finalize)
  - Rejects: PUT_BEGIN (error: "Upload already in progress")
  - Timeout: 5 seconds idle → abort session, send error

- **VALIDATING**: CRC32 verification
  - Compares client's final_crc32 with accumulated CRC32
  - Transition: STORING (match) or ERROR (mismatch)

- **STORING**: Writing to LittleFS
  - Calls Task 5 storage API: `pattern_storage_write(pattern_id, buffer, size)`
  - Transition: IDLE (success) or ERROR (storage failure)

- **ERROR**: Error occurred, cleanup resources
  - Always transitions to IDLE after sending error response
  - Frees upload buffer, resets CRC accumulator

### 3.2 Concurrent Session Handling

**Question**: Should each WebSocket client have independent upload sessions?

**Proposal**: Single global upload session (mutual exclusion)
- Simpler state management with one state machine
- Prevents race conditions on storage writes
- Client attempting PUT_BEGIN during active upload receives error

**Alternative**: Per-client upload sessions
- More complex: 2 state machines (one per WS_MAX_CLIENTS)
- Requires session isolation (separate buffers, CRC accumulators)
- Storage API must handle concurrent writes (mutex required)

**Recommendation**: Start with single global session, expand if needed.

---

## 4. Data Structures

### 4.1 TLV Frame Structure

```c
// firmware/components/network/include/protocol_parser.h

typedef struct {
    uint8_t type;           // Message type (0x10, 0x11, etc.)
    uint16_t length;        // Payload length (little-endian)
    uint8_t* payload;       // Pointer to payload data (NOT owned, points into WebSocket buffer)
    uint32_t crc32;         // CRC32 checksum from frame
} tlv_frame_t;
```

**Notes**:
- `payload` points into WebSocket RX buffer (Task 3 provides 4KB buffer)
- No memory allocation for `tlv_frame_t` - stack-allocated in parser
- Lifetime: Valid only during `tlv_dispatch_command()` call

### 4.2 Upload Session Context

```c
typedef enum {
    UPLOAD_STATE_IDLE = 0,
    UPLOAD_STATE_RECEIVING,
    UPLOAD_STATE_VALIDATING,
    UPLOAD_STATE_STORING,
    UPLOAD_STATE_ERROR
} upload_state_t;

typedef struct {
    upload_state_t state;            // Current state machine state
    uint16_t pattern_id;             // Target pattern ID (1-25)
    uint32_t total_size;             // Expected total size from PUT_BEGIN
    uint32_t bytes_received;         // Accumulator for PUT_DATA chunks
    uint32_t crc_accumulator;        // Running CRC32 of pattern data
    uint8_t* upload_buffer;          // Heap-allocated buffer for pattern assembly
    uint32_t last_activity_ms;       // Timeout detection
    int client_fd;                   // WebSocket client FD (for targeted responses)
} upload_session_t;
```

**Memory Management**:
- `upload_buffer`: Allocated on PUT_BEGIN, freed on PUT_END/error/timeout
- Maximum size: 256KB (ADR-004 limit)
- Allocation: `prism_pool_alloc(total_size)` from Task 1 memory pool
- Deallocation: `prism_pool_free(upload_buffer)` in all exit paths

**Concurrency**:
- Protected by `upload_mutex` (FreeRTOS binary semaphore)
- Mutex held during state transitions and buffer operations
- Minimal hold time (<5ms typical)

### 4.3 Protocol Context (Static Global)

```c
static struct {
    upload_session_t session;       // Single global upload session
    SemaphoreHandle_t mutex;        // Protects session state
    bool initialized;               // Init flag
} protocol_state = {0};
```

---

## 5. Function Architecture

### 5.1 Public API (protocol_parser.h)

```c
// Initialize protocol parser (called once at startup)
esp_err_t protocol_parser_init(void);

// Deinitialize protocol parser (cleanup)
void protocol_parser_deinit(void);

// Main entry point from Task 3 WebSocket handler
// Called from handle_ws_frame() at line 1238
esp_err_t protocol_dispatch_command(
    const uint8_t* frame_data,   // Raw binary frame from WebSocket
    size_t frame_len,            // Frame length (validated by Task 3 as <=4096)
    int client_fd                // WebSocket client FD (for responses)
);
```

**Integration with Task 3**:
```c
// In network_manager.c, line 1238 (replace TODO):
esp_err_t ret = protocol_dispatch_command(ws_pkt.payload, ws_pkt.len, client->socket_fd);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Protocol dispatch failed: %s", esp_err_to_name(ret));
    return ret;  // WebSocket handler will cleanup
}
```

### 5.2 Internal Functions (protocol_parser.c)

**Frame Parsing**:
```c
// Parse TLV frame structure
static esp_err_t parse_tlv_frame(
    const uint8_t* data,
    size_t len,
    tlv_frame_t* out_frame
);

// Validate CRC32 checksum
static bool validate_frame_crc(const tlv_frame_t* frame);
```

**Message Handlers**:
```c
// Pattern upload handlers
static esp_err_t handle_put_begin(const tlv_frame_t* frame, int client_fd);
static esp_err_t handle_put_data(const tlv_frame_t* frame, int client_fd);
static esp_err_t handle_put_end(const tlv_frame_t* frame, int client_fd);

// Control handlers
static esp_err_t handle_control_play(const tlv_frame_t* frame, int client_fd);
static esp_err_t handle_control_stop(const tlv_frame_t* frame, int client_fd);
```

**Session Management**:
```c
// Initialize upload session (allocate buffer, reset state)
static esp_err_t init_upload_session(uint16_t pattern_id, uint32_t total_size, int client_fd);

// Append data to upload buffer, update CRC
static esp_err_t append_upload_data(const uint8_t* data, size_t len);

// Finalize upload, validate CRC, store pattern
static esp_err_t finalize_upload(uint32_t client_crc32);

// Abort upload, cleanup resources
static void abort_upload_session(const char* reason);

// Check for upload timeout (called periodically)
static void check_upload_timeout(void);
```

**Response Functions**:
```c
// Send STATUS message (reuse Task 3's send_ws_status)
static esp_err_t send_status_response(int client_fd, uint8_t status_code, const char* message);

// Send ERROR message (reuse Task 3's send_ws_error)
static esp_err_t send_error_response(int client_fd, uint8_t error_code, const char* message);
```

---

## 6. CRC32 Validation Strategy

### 6.1 Two-Level CRC Validation

**Level 1: Frame-Level CRC** (each TLV message)
- Validates individual frame integrity
- CRC32 of: [TYPE][LENGTH][PAYLOAD]
- Checked in `validate_frame_crc()`
- Rejects corrupted frames immediately

**Level 2: Content-Level CRC** (entire pattern upload)
- Validates multi-frame pattern data integrity
- CRC32 of: concatenation of all PUT_DATA payloads
- Accumulated during RECEIVING state
- Validated in VALIDATING state against client's final_crc32

### 6.2 CRC Calculation Details

**ESP-IDF ROM Function**:
```c
#include "esp_rom_crc.h"

uint32_t esp_rom_crc32_le(uint32_t crc, const uint8_t *buf, uint32_t len);
```

**Frame CRC Calculation** (Level 1):
```c
uint32_t calculate_frame_crc(const tlv_frame_t* frame) {
    uint32_t crc = 0xFFFFFFFF;
    crc = esp_rom_crc32_le(crc, &frame->type, 1);            // TYPE
    uint16_t len_le = frame->length;  // Already little-endian
    crc = esp_rom_crc32_le(crc, (uint8_t*)&len_le, 2);       // LENGTH
    crc = esp_rom_crc32_le(crc, frame->payload, frame->length);  // PAYLOAD
    return crc ^ 0xFFFFFFFF;  // Final XOR
}
```

**Pattern CRC Accumulation** (Level 2):
```c
// In init_upload_session():
protocol_state.session.crc_accumulator = 0xFFFFFFFF;

// In append_upload_data():
protocol_state.session.crc_accumulator = esp_rom_crc32_le(
    protocol_state.session.crc_accumulator,
    data,
    len
);

// In finalize_upload():
uint32_t final_crc = protocol_state.session.crc_accumulator ^ 0xFFFFFFFF;
if (final_crc != client_crc32) {
    abort_upload_session("CRC mismatch");
    return send_error_response(client_fd, 0x02, "CRC validation failed");
}
```

---

## 7. Upload Session Management

### 7.1 PUT_BEGIN Flow

```c
esp_err_t handle_put_begin(const tlv_frame_t* frame, int client_fd) {
    // 1. Validate payload length
    if (frame->length != 6) {
        return send_error_response(client_fd, 0x01, "Invalid PUT_BEGIN payload");
    }

    // 2. Parse payload
    uint16_t pattern_id = frame->payload[0] | (frame->payload[1] << 8);
    uint32_t total_size = frame->payload[2] | (frame->payload[3] << 8) |
                          (frame->payload[4] << 16) | (frame->payload[5] << 24);

    // 3. Validate pattern_id (1-25 per ADR-006)
    if (pattern_id < 1 || pattern_id > 25) {
        return send_error_response(client_fd, 0x01, "Invalid pattern ID");
    }

    // 4. Validate total_size (max 256KB per ADR-004)
    if (total_size == 0 || total_size > 262144) {
        return send_error_response(client_fd, 0x03, "Invalid pattern size");
    }

    // 5. Check existing session
    xSemaphoreTake(protocol_state.mutex, portMAX_DELAY);
    if (protocol_state.session.state != UPLOAD_STATE_IDLE) {
        xSemaphoreGive(protocol_state.mutex);
        return send_error_response(client_fd, 0x01, "Upload already in progress");
    }

    // 6. Initialize session
    esp_err_t ret = init_upload_session(pattern_id, total_size, client_fd);
    if (ret != ESP_OK) {
        xSemaphoreGive(protocol_state.mutex);
        return send_error_response(client_fd, 0x04, "Memory allocation failed");
    }

    protocol_state.session.state = UPLOAD_STATE_RECEIVING;
    xSemaphoreGive(protocol_state.mutex);

    return send_status_response(client_fd, 0x01, "Upload session started");
}
```

### 7.2 PUT_DATA Flow

```c
esp_err_t handle_put_data(const tlv_frame_t* frame, int client_fd) {
    xSemaphoreTake(protocol_state.mutex, portMAX_DELAY);

    // 1. Validate state
    if (protocol_state.session.state != UPLOAD_STATE_RECEIVING) {
        xSemaphoreGive(protocol_state.mutex);
        return send_error_response(client_fd, 0x01, "No active upload session");
    }

    // 2. Validate client (prevent cross-client pollution)
    if (protocol_state.session.client_fd != client_fd) {
        xSemaphoreGive(protocol_state.mutex);
        return send_error_response(client_fd, 0x01, "Session owned by another client");
    }

    // 3. Check size overflow
    if (protocol_state.session.bytes_received + frame->length > protocol_state.session.total_size) {
        abort_upload_session("Size overflow");
        xSemaphoreGive(protocol_state.mutex);
        return send_error_response(client_fd, 0x03, "Pattern size exceeded");
    }

    // 4. Append data and update CRC
    esp_err_t ret = append_upload_data(frame->payload, frame->length);
    if (ret != ESP_OK) {
        abort_upload_session("Append failed");
        xSemaphoreGive(protocol_state.mutex);
        return send_error_response(client_fd, 0x04, "Storage write failed");
    }

    // 5. Update activity timestamp
    protocol_state.session.last_activity_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    xSemaphoreGive(protocol_state.mutex);
    return ESP_OK;  // No response for PUT_DATA (reduces overhead)
}
```

### 7.3 PUT_END Flow

```c
esp_err_t handle_put_end(const tlv_frame_t* frame, int client_fd) {
    // 1. Validate payload
    if (frame->length != 4) {
        return send_error_response(client_fd, 0x01, "Invalid PUT_END payload");
    }

    uint32_t client_crc32 = frame->payload[0] | (frame->payload[1] << 8) |
                            (frame->payload[2] << 16) | (frame->payload[3] << 24);

    xSemaphoreTake(protocol_state.mutex, portMAX_DELAY);

    // 2. Validate state
    if (protocol_state.session.state != UPLOAD_STATE_RECEIVING) {
        xSemaphoreGive(protocol_state.mutex);
        return send_error_response(client_fd, 0x01, "No active upload session");
    }

    // 3. Validate client
    if (protocol_state.session.client_fd != client_fd) {
        xSemaphoreGive(protocol_state.mutex);
        return send_error_response(client_fd, 0x01, "Session owned by another client");
    }

    // 4. Validate size completeness
    if (protocol_state.session.bytes_received != protocol_state.session.total_size) {
        abort_upload_session("Incomplete upload");
        xSemaphoreGive(protocol_state.mutex);
        return send_error_response(client_fd, 0x03, "Incomplete pattern data");
    }

    // 5. Finalize and store
    protocol_state.session.state = UPLOAD_STATE_VALIDATING;
    esp_err_t ret = finalize_upload(client_crc32);
    xSemaphoreGive(protocol_state.mutex);

    if (ret == ESP_OK) {
        return send_status_response(client_fd, 0x00, "Pattern uploaded successfully");
    } else {
        return send_error_response(client_fd, 0x02, "CRC validation failed");
    }
}
```

### 7.4 Timeout Handling

**Integration**: Called from `network_task()` (Task 3) every 1 second

```c
void check_upload_timeout(void) {
    xSemaphoreTake(protocol_state.mutex, portMAX_DELAY);

    if (protocol_state.session.state == UPLOAD_STATE_RECEIVING) {
        uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        uint32_t idle_ms = now_ms - protocol_state.session.last_activity_ms;

        if (idle_ms > 5000) {  // 5-second timeout
            ESP_LOGW(TAG, "Upload timeout: %lu ms idle", idle_ms);
            abort_upload_session("Timeout");
            // Send error to client (requires storing httpd_req_t?)
        }
    }

    xSemaphoreGive(protocol_state.mutex);
}
```

**Open Question**: How to send error response to client on timeout without `httpd_req_t`?
- Option 1: Store `httpd_req_t*` in session (thread-safety concerns)
- Option 2: Use `httpd_queue_work()` with client FD (research needed)
- Option 3: Client detects timeout via lack of response (simpler, rely on WebSocket timeout)

---

## 8. Integration with Task 3 (WebSocket)

### 8.1 Handoff Point

**File**: `firmware/components/network/network_manager.c`
**Location**: Line 1238 in `handle_ws_frame()`

**Current Code**:
```c
// TODO: Task 4 - Pass to TLV parser
// return tlv_dispatch_command(ws_pkt.payload, ws_pkt.len, req);
ESP_LOGI(TAG, "Received %zu bytes from client %d", ws_pkt.len, client_idx);
return ESP_OK;
```

**Replacement**:
```c
// Task 4 - Pass to TLV protocol parser
esp_err_t ret = protocol_dispatch_command(ws_pkt.payload, ws_pkt.len, client->socket_fd);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Protocol dispatch failed: %s", esp_err_to_name(ret));
    return ret;
}
return ESP_OK;
```

### 8.2 Response Functions

Task 4 will use Task 3's response functions for sending replies:

**From Task 3** (`network_manager.c`):
```c
// Line 1058: Send error frame
static esp_err_t send_ws_error(int socket_fd, uint8_t error_code);

// Line 1084: Send status frame
static esp_err_t send_ws_status(int socket_fd, uint8_t status_code, const char* message);
```

**Task 4 Wrapper** (protocol_parser.c):
```c
// Wrapper for error responses
static esp_err_t send_error_response(int client_fd, uint8_t error_code, const char* message) {
    // TODO: Implement proper TLV error frame format
    // For now, use Task 3's placeholder function
    return send_ws_error(client_fd, error_code);
}

// Wrapper for status responses
static esp_err_t send_status_response(int client_fd, uint8_t status_code, const char* message) {
    return send_ws_status(client_fd, status_code, message);
}
```

**Note**: Task 3's `send_ws_error` and `send_ws_status` are placeholders. Task 4 will need to implement proper TLV encoding for ERROR (0xFF) and STATUS (0x30) messages per section 2.3.

### 8.3 Timeout Integration

**Task 3 provides**: Periodic timeout checking in `network_task()` (line 1445)

**Task 4 addition**: Add `protocol_check_upload_timeout()` call

**Modification** to `network_manager.c`:
```c
// Around line 1455, add:
// Check protocol upload timeouts (Task 4)
extern void protocol_check_upload_timeout(void);
protocol_check_upload_timeout();
```

**Export** from `protocol_parser.h`:
```c
void protocol_check_upload_timeout(void);
```

---

## 9. Integration with Task 5 (Storage)

### 9.1 Storage API (Expected Interface)

**File**: `firmware/components/storage/include/pattern_storage.h`

**Expected Functions** (based on Task 5 brief):
```c
// Write pattern to LittleFS
esp_err_t pattern_storage_write(
    uint16_t pattern_id,       // Pattern ID (1-25)
    const uint8_t* data,       // Pattern data buffer
    size_t size                // Data size (max 256KB)
);

// Read pattern from LittleFS
esp_err_t pattern_storage_read(
    uint16_t pattern_id,       // Pattern ID to read
    uint8_t* buffer,           // Output buffer (caller-allocated)
    size_t buffer_size,        // Buffer capacity
    size_t* out_size           // Actual pattern size read
);

// Delete pattern from LittleFS
esp_err_t pattern_storage_delete(uint16_t pattern_id);

// List all stored patterns
esp_err_t pattern_storage_list(
    uint16_t* pattern_ids,     // Output array of pattern IDs
    size_t max_count,          // Array capacity
    size_t* out_count          // Actual count of patterns
);

// Check if pattern exists
bool pattern_storage_exists(uint16_t pattern_id);
```

### 9.2 Storage Integration in finalize_upload()

```c
esp_err_t finalize_upload(uint32_t client_crc32) {
    // 1. Validate CRC
    uint32_t computed_crc = protocol_state.session.crc_accumulator ^ 0xFFFFFFFF;
    if (computed_crc != client_crc32) {
        ESP_LOGE(TAG, "CRC mismatch: expected 0x%08lx, got 0x%08lx", computed_crc, client_crc32);
        abort_upload_session("CRC mismatch");
        return ESP_FAIL;
    }

    // 2. Store pattern (Task 5 integration)
    protocol_state.session.state = UPLOAD_STATE_STORING;
    esp_err_t ret = pattern_storage_write(
        protocol_state.session.pattern_id,
        protocol_state.session.upload_buffer,
        protocol_state.session.bytes_received
    );

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Pattern %d stored successfully (%lu bytes)",
                 protocol_state.session.pattern_id,
                 protocol_state.session.bytes_received);
    } else {
        ESP_LOGE(TAG, "Storage write failed: %s", esp_err_to_name(ret));
        abort_upload_session("Storage write failed");
        return ESP_FAIL;
    }

    // 3. Cleanup session
    prism_pool_free(protocol_state.session.upload_buffer);
    memset(&protocol_state.session, 0, sizeof(upload_session_t));
    protocol_state.session.state = UPLOAD_STATE_IDLE;

    return ESP_OK;
}
```

### 9.3 Stub Implementation (Until Task 5 Complete)

**File**: `firmware/components/network/protocol_parser.c`

```c
// Temporary stub for Task 5 storage API
static esp_err_t pattern_storage_write(uint16_t pattern_id, const uint8_t* data, size_t size) {
    ESP_LOGW(TAG, "STUB: pattern_storage_write(id=%d, size=%zu) - Task 5 not implemented", pattern_id, size);
    // Simulate success for testing
    return ESP_OK;
}

static bool pattern_storage_exists(uint16_t pattern_id) {
    ESP_LOGW(TAG, "STUB: pattern_storage_exists(id=%d) - Task 5 not implemented", pattern_id);
    return false;  // Assume no patterns exist
}
```

**Replacement**: Once Task 5 is complete, replace stubs with real includes:
```c
#include "pattern_storage.h"  // Task 5 header
// Remove stub implementations
```

---

## 10. Integration with Task 8 (Playback)

### 10.1 Playback API (Expected Interface)

**File**: `firmware/components/playback/include/led_driver.h`

**Expected Functions** (based on Task 8 completion):
```c
// Load pattern into playback buffer
esp_err_t led_driver_load_pattern(
    const uint8_t* pattern_data,
    size_t pattern_size
);

// Start playback with loaded pattern
esp_err_t led_driver_start_playback(void);

// Stop current playback
esp_err_t led_driver_stop_playback(void);

// Get current playback status
bool led_driver_is_playing(void);
```

### 10.2 Playback Integration in handle_control_play()

```c
esp_err_t handle_control_play(const tlv_frame_t* frame, int client_fd) {
    // 1. Validate payload
    if (frame->length != 2) {
        return send_error_response(client_fd, 0x01, "Invalid CONTROL_PLAY payload");
    }

    uint16_t pattern_id = frame->payload[0] | (frame->payload[1] << 8);

    // 2. Validate pattern_id
    if (pattern_id < 1 || pattern_id > 25) {
        return send_error_response(client_fd, 0x01, "Invalid pattern ID");
    }

    // 3. Check if pattern exists (Task 5)
    if (!pattern_storage_exists(pattern_id)) {
        return send_error_response(client_fd, 0x05, "Pattern not found");
    }

    // 4. Load pattern from storage
    uint8_t* pattern_buffer = prism_pool_alloc(262144);  // Max 256KB
    if (!pattern_buffer) {
        return send_error_response(client_fd, 0x04, "Memory allocation failed");
    }

    size_t pattern_size = 0;
    esp_err_t ret = pattern_storage_read(pattern_id, pattern_buffer, 262144, &pattern_size);
    if (ret != ESP_OK) {
        prism_pool_free(pattern_buffer);
        return send_error_response(client_fd, 0x05, "Pattern read failed");
    }

    // 5. Load into playback engine (Task 8)
    ret = led_driver_load_pattern(pattern_buffer, pattern_size);
    prism_pool_free(pattern_buffer);  // Playback makes internal copy
    if (ret != ESP_OK) {
        return send_error_response(client_fd, 0x04, "Playback load failed");
    }

    // 6. Start playback (Task 8)
    ret = led_driver_start_playback();
    if (ret != ESP_OK) {
        return send_error_response(client_fd, 0x04, "Playback start failed");
    }

    return send_status_response(client_fd, 0x02, "Playback started");
}
```

### 10.3 Stop Playback in handle_control_stop()

```c
esp_err_t handle_control_stop(const tlv_frame_t* frame, int client_fd) {
    // 1. Validate payload (should be empty)
    if (frame->length != 0) {
        return send_error_response(client_fd, 0x01, "CONTROL_STOP expects no payload");
    }

    // 2. Stop playback (Task 8)
    esp_err_t ret = led_driver_stop_playback();
    if (ret != ESP_OK) {
        return send_error_response(client_fd, 0x04, "Playback stop failed");
    }

    return send_status_response(client_fd, 0x03, "Playback stopped");
}
```

### 10.4 Stub Implementation (Playback Already Complete)

Task 8 is already complete, so real integration should be possible. If API doesn't match, create wrapper:

```c
// Wrapper for Task 8 LED driver (if needed)
static esp_err_t playback_load_pattern(const uint8_t* data, size_t size) {
    // TODO: Map to actual Task 8 API
    // May require parsing pattern format (Task 6 dependency?)
    ESP_LOGW(TAG, "STUB: playback_load_pattern(size=%zu)", size);
    return ESP_OK;
}
```

---

## 11. Memory Management Strategy

### 11.1 Memory Budget

**Total Heap Budget**: 150KB (from CANON ADR-004)

**Task 4 Allocations**:
1. **Upload Buffer**: 0-256KB (transient, during upload only)
2. **Protocol State**: ~60 bytes (static global)
3. **Mutex**: ~20 bytes (static allocation)
4. **Stack Usage**: ~2KB per protocol_dispatch_command() call

**Peak Memory**: 256KB during PUT_END (pattern fully buffered before storage write)

**Mitigation Strategy**:
- Only allocate upload buffer on PUT_BEGIN
- Free immediately after `pattern_storage_write()` succeeds
- Abort and free on any error
- Timeout cleanup frees buffer automatically

### 11.2 Allocation Patterns

**Upload Buffer Allocation**:
```c
esp_err_t init_upload_session(uint16_t pattern_id, uint32_t total_size, int client_fd) {
    // Allocate buffer from Task 1 memory pool
    uint8_t* buffer = prism_pool_alloc(total_size);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate %lu bytes for pattern upload", total_size);
        return ESP_ERR_NO_MEM;
    }

    protocol_state.session.upload_buffer = buffer;
    protocol_state.session.pattern_id = pattern_id;
    protocol_state.session.total_size = total_size;
    protocol_state.session.bytes_received = 0;
    protocol_state.session.crc_accumulator = 0xFFFFFFFF;
    protocol_state.session.client_fd = client_fd;
    protocol_state.session.last_activity_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    ESP_LOGI(TAG, "Upload session initialized: pattern_id=%d, size=%lu", pattern_id, total_size);
    return ESP_OK;
}
```

**Deallocation Paths**:
1. **Success**: `finalize_upload()` after `pattern_storage_write()`
2. **Error**: `abort_upload_session()` on any failure
3. **Timeout**: `check_upload_timeout()` detects idle session

### 11.3 Fragmentation Prevention

**Strategy**: Use Task 1's memory pool for large allocations
- Memory pool provides contiguous blocks
- Pre-allocated pools prevent heap fragmentation
- 256KB patterns fit within pool block sizes

**Alternative**: Stream directly to storage (future optimization)
- Avoid buffering entire pattern in RAM
- Write PUT_DATA chunks incrementally to LittleFS
- Requires Task 5 API support for append mode

---

## 12. Error Handling & Error Codes

### 12.1 Error Code Mapping (ADR-002)

| Error Code | Constant | Description | Trigger Conditions |
|------------|----------|-------------|-------------------|
| 0x01 | ERR_INVALID_FRAME | Invalid frame format | - Malformed TLV structure<br>- Wrong payload size<br>- Invalid message type<br>- State machine violation |
| 0x02 | ERR_CRC_MISMATCH | CRC validation failed | - Frame CRC mismatch<br>- Pattern CRC mismatch in PUT_END |
| 0x03 | ERR_SIZE_EXCEEDED | Pattern size exceeded | - Total size > 256KB<br>- PUT_DATA overflow |
| 0x04 | ERR_STORAGE_FULL | Storage full | - Memory allocation failed<br>- >15 patterns stored<br>- LittleFS full |
| 0x05 | ERR_NOT_FOUND | Pattern not found | - CONTROL_PLAY for non-existent pattern |

### 12.2 Error Response Format

**ERROR Message (0xFF)**:
```c
esp_err_t send_error_response(int client_fd, uint8_t error_code, const char* message) {
    // Build TLV frame:
    // [TYPE:0xFF][LENGTH:msg_len+1][error_code:1][message:N][CRC32:4]

    size_t msg_len = strlen(message);
    size_t payload_len = 1 + msg_len;  // error_code + message
    size_t frame_len = 1 + 2 + payload_len + 4;  // TYPE + LENGTH + PAYLOAD + CRC32

    uint8_t* frame = malloc(frame_len);
    if (!frame) return ESP_ERR_NO_MEM;

    // Encode frame
    frame[0] = 0xFF;  // TYPE
    frame[1] = payload_len & 0xFF;  // LENGTH low byte
    frame[2] = (payload_len >> 8) & 0xFF;  // LENGTH high byte
    frame[3] = error_code;  // PAYLOAD: error code
    memcpy(&frame[4], message, msg_len);  // PAYLOAD: message

    // Calculate CRC32
    uint32_t crc = 0xFFFFFFFF;
    crc = esp_rom_crc32_le(crc, frame, 3 + payload_len);  // TYPE + LENGTH + PAYLOAD
    crc ^= 0xFFFFFFFF;
    memcpy(&frame[3 + payload_len], &crc, 4);  // CRC32 (little-endian)

    // Send via WebSocket (requires httpd_ws_send_frame)
    // TODO: How to send without httpd_req_t? Use client_fd with httpd_queue_work?
    esp_err_t ret = ws_send_binary_to_fd(client_fd, frame, frame_len);

    free(frame);
    return ret;
}
```

**Open Question**: Task 3's `send_ws_error()` is a placeholder. Need to implement proper WebSocket send using client FD.

### 12.3 Error Recovery Strategy

**Transient Errors** (retry possible):
- 0x04 (Storage full): Client can delete old patterns and retry

**Permanent Errors** (require user action):
- 0x01 (Invalid frame): Client bug, fix protocol implementation
- 0x02 (CRC mismatch): Network corruption or client bug
- 0x03 (Size exceeded): Pattern too large, requires compression
- 0x05 (Not found): Pattern never uploaded

**Error Handling in Protocol**:
1. Send ERROR response to client
2. Abort upload session if active
3. Free all allocated resources
4. Transition to IDLE state
5. Log error for debugging

---

## 13. Testing Strategy

### 13.1 Unit Tests (ESP-IDF Unity Framework)

**File**: `firmware/components/network/test/test_protocol_parser.c`

**Test Cases**:

1. **TLV Frame Parsing**:
   - `test_parse_valid_tlv_frame` - Valid frame structure
   - `test_parse_invalid_frame_too_short` - <7 bytes
   - `test_parse_invalid_frame_length_mismatch` - LENGTH doesn't match actual payload

2. **CRC Validation**:
   - `test_frame_crc_valid` - Correct CRC32
   - `test_frame_crc_invalid` - Corrupted CRC32
   - `test_pattern_crc_accumulation` - Multi-chunk CRC

3. **Upload Session State Machine**:
   - `test_put_begin_idle_state` - Start upload from IDLE
   - `test_put_begin_already_active` - Reject second upload
   - `test_put_data_no_session` - Reject PUT_DATA without PUT_BEGIN
   - `test_put_data_accumulation` - Multiple PUT_DATA chunks
   - `test_put_end_size_mismatch` - Incomplete upload
   - `test_put_end_crc_mismatch` - Wrong final CRC
   - `test_put_end_success` - Valid upload flow

4. **Size Validation**:
   - `test_put_begin_size_zero` - Reject 0-byte pattern
   - `test_put_begin_size_too_large` - Reject >256KB
   - `test_put_data_overflow` - Reject oversized chunks

5. **Session Timeout**:
   - `test_upload_timeout_5_seconds` - Abort idle session
   - `test_upload_no_timeout_active` - Keep session alive with PUT_DATA

6. **Error Responses**:
   - `test_error_response_encoding` - Validate ERROR frame format
   - `test_status_response_encoding` - Validate STATUS frame format

7. **Integration Stubs**:
   - `test_storage_write_stub` - Mock pattern_storage_write()
   - `test_playback_load_stub` - Mock led_driver_load_pattern()

### 13.2 Manual Hardware Testing

**Test Procedure** (using WebSocket client):

**Test 1: Valid Upload Flow**
```bash
# 1. Connect to WebSocket
websocat ws://192.168.4.1/ws

# 2. Send PUT_BEGIN (pattern_id=1, total_size=100 bytes)
# [TYPE:0x10][LENGTH:0x0600][pattern_id:0x0100][total_size:0x64000000][CRC32:...]
# (binary frame, requires hex tool)

# 3. Send PUT_DATA (50 bytes)
# [TYPE:0x11][LENGTH:0x3200][data:50 bytes][CRC32:...]

# 4. Send PUT_DATA (50 bytes)
# [TYPE:0x11][LENGTH:0x3200][data:50 bytes][CRC32:...]

# 5. Send PUT_END (final_crc32=...)
# [TYPE:0x12][LENGTH:0x0400][final_crc32:...][CRC32:...]

# Expected: Receive STATUS (0x30) with code 0x00 ("Success")
```

**Test 2: CRC Mismatch**
- Send PUT_END with wrong final_crc32
- Expected: Receive ERROR (0xFF) with code 0x02

**Test 3: Size Overflow**
- Send PUT_BEGIN with total_size=100
- Send PUT_DATA with 120 bytes
- Expected: Receive ERROR (0xFF) with code 0x03

**Test 4: Upload Timeout**
- Send PUT_BEGIN
- Wait 6 seconds without PUT_DATA
- Expected: Session aborted, next PUT_DATA rejected

**Test 5: Playback Control**
- Upload pattern (Test 1)
- Send CONTROL_PLAY (pattern_id=1)
- Expected: LEDs light up, receive STATUS (0x30) with code 0x02
- Send CONTROL_STOP
- Expected: LEDs turn off, receive STATUS (0x30) with code 0x03

### 13.3 Integration Testing

**With Task 3 (WebSocket)**:
- Verify `protocol_dispatch_command()` receives correct frame data
- Verify responses sent via WebSocket
- Verify 2-client concurrency (independent sessions or mutual exclusion)

**With Task 5 (Storage)** (when available):
- Upload pattern, verify written to LittleFS
- List patterns, verify correct count
- Delete pattern, verify removed
- Upload 15 patterns, verify 16th rejected (ADR-006 limit)

**With Task 8 (Playback)**:
- Upload pattern, send CONTROL_PLAY, verify LED output
- Verify pattern data correctly parsed and displayed
- Verify CONTROL_STOP halts playback

---

## 14. Implementation Phases

### Phase 1: Core TLV Parser (Subtask 4.1)
**Estimated Time**: 2-3 hours
**Files**: `protocol_parser.c/h`

**Deliverables**:
1. `tlv_frame_t` structure
2. `parse_tlv_frame()` function
3. `validate_frame_crc()` function
4. Frame parsing unit tests
5. CRC validation unit tests

**Success Criteria**:
- Parse valid TLV frames
- Reject malformed frames
- Validate CRC32 checksums
- Unit tests pass

### Phase 2: Upload Session State Machine (Subtask 4.2)
**Estimated Time**: 3-4 hours
**Files**: `protocol_parser.c`

**Deliverables**:
1. `upload_session_t` structure
2. `protocol_state` global variable
3. `init_upload_session()` function
4. `append_upload_data()` function
5. `finalize_upload()` function
6. `abort_upload_session()` function
7. `handle_put_begin()` handler
8. `handle_put_data()` handler
9. `handle_put_end()` handler
10. State machine unit tests

**Success Criteria**:
- PUT_BEGIN → PUT_DATA → PUT_END flow works
- Upload buffer allocated and freed correctly
- CRC accumulation across chunks
- Size validation enforced
- State machine rejects invalid transitions

### Phase 3: Storage Integration (Subtask 4.3)
**Estimated Time**: 2-3 hours
**Files**: `protocol_parser.c`

**Deliverables**:
1. Storage API stubs (until Task 5 complete)
2. `pattern_storage_write()` integration in `finalize_upload()`
3. `pattern_storage_exists()` integration in `handle_control_play()`
4. Error handling for storage failures
5. Storage integration tests (with mocks)

**Success Criteria**:
- Successful upload writes pattern to storage
- Storage errors propagate correctly
- Pattern existence checked before playback

**Note**: Real integration deferred until Task 5 (Storage) complete. Use stubs for now.

### Phase 4: Playback Integration (Subtask 4.4)
**Estimated Time**: 2-3 hours
**Files**: `protocol_parser.c`

**Deliverables**:
1. `handle_control_play()` handler
2. `handle_control_stop()` handler
3. Playback API integration with Task 8
4. Error handling for playback failures
5. Playback control tests

**Success Criteria**:
- CONTROL_PLAY loads pattern and starts playback
- CONTROL_STOP halts playback
- Pattern not found errors handled
- Playback errors propagated

**Note**: Task 8 (LED Driver) already complete, should have real API available.

### Phase 5: Testing & Integration (Subtask 4.5)
**Estimated Time**: 3-4 hours
**Files**: `test/test_protocol_parser.c`, `test/README.md`

**Deliverables**:
1. Comprehensive unit test suite (20+ tests)
2. Manual hardware test procedure documentation
3. Integration testing with Tasks 3, 5, 8
4. Error path coverage validation
5. Memory leak testing

**Success Criteria**:
- All unit tests pass
- Manual hardware tests successful
- No memory leaks detected
- Integration with Task 3 verified
- Storage/playback stubs functional

### Phase 6: Response Encoding & Cleanup
**Estimated Time**: 2-3 hours
**Files**: `protocol_parser.c`, `network_manager.c`

**Deliverables**:
1. Proper TLV encoding for ERROR (0xFF) messages
2. Proper TLV encoding for STATUS (0x30) messages
3. Replace Task 3 placeholder functions
4. Implement broadcast mechanism (if needed)
5. Timeout handling integration

**Success Criteria**:
- ERROR/STATUS messages properly formatted
- Responses sent correctly via WebSocket
- Timeout cleanup functional

---

## 15. Open Questions & Risks

### 15.1 Open Questions

**Q1: Per-Client vs Global Upload Session?**
- Current plan: Single global session (mutual exclusion)
- Alternative: Per-client sessions (more complex)
- Decision: Start simple, expand if concurrency needed
- **Resolution**: Implement global session first

**Q2: Timeout Error Response Without httpd_req_t?**
- Problem: Timeout check runs in `network_task()`, no `httpd_req_t` available
- Options:
  1. Store `httpd_req_t*` in session (thread-safety concerns)
  2. Use `httpd_queue_work()` with client FD
  3. Rely on WebSocket 5-second timeout (simpler)
- **Recommendation**: Option 3 (rely on WebSocket timeout), log warning

**Q3: Streaming Upload vs Buffered Upload?**
- Current plan: Buffer entire pattern in RAM (simpler)
- Alternative: Stream chunks directly to LittleFS (memory-efficient)
- Tradeoff: Complexity vs memory usage
- **Recommendation**: Start with buffered, optimize later if needed

**Q4: Task 8 Playback API?**
- Need to verify actual API from Task 8 implementation
- May require pattern format parsing (Task 6 dependency?)
- **Action**: Review Task 8 code before Phase 4

**Q5: Response Encoding in Task 3?**
- Task 3's `send_ws_error()` and `send_ws_status()` are placeholders
- Task 4 needs to implement proper TLV encoding
- Or should Task 3 be updated?
- **Recommendation**: Task 4 implements proper encoding, replaces Task 3 stubs

### 15.2 Risks & Mitigations

| Risk | Severity | Mitigation |
|------|----------|------------|
| **Task 5 (Storage) not ready** | High | Implement stubs, defer real integration until Task 5 complete |
| **256KB RAM allocation fails** | Medium | Enforce pattern size limits, add memory pressure checks |
| **CRC32 performance impact** | Low | Use ESP-IDF ROM function (hardware-accelerated), profile if needed |
| **Concurrent upload race conditions** | Medium | Use global session with mutex, enforce mutual exclusion |
| **WebSocket send without httpd_req_t** | Medium | Research `httpd_queue_work()`, fallback to fd-based send |
| **Timeout cleanup complexity** | Low | Simple idle detection, cleanup on abort |
| **Task 8 API mismatch** | Medium | Review Task 8 code early, create wrapper if needed |
| **Build integration issues** | Low | Commit after each subtask, PM builds frequently |

### 15.3 Dependencies

**Blocking Dependencies**:
- Task 3 (WebSocket) ✅ COMPLETE - Ready for integration

**Non-Blocking Dependencies** (stub initially):
- Task 5 (Storage) - In recovery, use stubs
- Task 6 (Pattern Parser) - May be needed for playback
- Task 7 (RAM Cache) - Future optimization

**External Dependencies**:
- ESP-IDF v5.x `esp_rom_crc.h` - CRC32 functions
- ESP-IDF v5.x `esp_http_server.h` - WebSocket send (from Task 3)
- FreeRTOS - Mutex and task management

---

## Implementation Checklist

### Pre-Implementation
- [x] Review Task 3 WebSocket implementation
- [x] Review CANON specifications (ADR-002, ADR-004, ADR-006)
- [x] Review tasks.json Task 4 requirements
- [ ] Create `protocol_parser.h` header with public API
- [ ] Create `protocol_parser.c` stub with init/deinit
- [ ] Update `network/CMakeLists.txt` to include new source

### Phase 1: Core TLV Parser
- [ ] Implement `tlv_frame_t` structure
- [ ] Implement `parse_tlv_frame()`
- [ ] Implement `validate_frame_crc()`
- [ ] Write unit tests for parsing
- [ ] Write unit tests for CRC validation
- [ ] Verify all Phase 1 tests pass

### Phase 2: Upload State Machine
- [ ] Implement `upload_session_t` structure
- [ ] Implement `protocol_state` global
- [ ] Implement session management functions
- [ ] Implement PUT_BEGIN handler
- [ ] Implement PUT_DATA handler
- [ ] Implement PUT_END handler
- [ ] Write state machine unit tests
- [ ] Verify all Phase 2 tests pass
- [ ] **COMMIT**: `git commit -m "feat(task-4.2): Upload session state machine"`

### Phase 3: Storage Integration
- [ ] Create storage API stubs
- [ ] Integrate `pattern_storage_write()`
- [ ] Integrate `pattern_storage_exists()`
- [ ] Write storage integration tests
- [ ] Verify all Phase 3 tests pass
- [ ] **COMMIT**: `git commit -m "feat(task-4.3): Storage integration (stubs)"`

### Phase 4: Playback Integration
- [ ] Review Task 8 playback API
- [ ] Implement CONTROL_PLAY handler
- [ ] Implement CONTROL_STOP handler
- [ ] Write playback control tests
- [ ] Verify all Phase 4 tests pass
- [ ] **COMMIT**: `git commit -m "feat(task-4.4): Playback control integration"`

### Phase 5: Testing
- [ ] Write comprehensive unit test suite
- [ ] Document manual hardware test procedure
- [ ] Run unit tests, verify 100% pass rate
- [ ] Run memory leak tests with `prism_heap_monitor`
- [ ] Integration test with Task 3
- [ ] **COMMIT**: `git commit -m "feat(task-4.5): Comprehensive test suite"`

### Phase 6: Response Encoding
- [ ] Implement proper ERROR (0xFF) encoding
- [ ] Implement proper STATUS (0x30) encoding
- [ ] Replace Task 3 placeholder functions
- [ ] Integrate timeout handling
- [ ] Final integration testing
- [ ] **COMMIT**: `git commit -m "feat(task-4): Complete TLV protocol parser"`

### PM Handoff
- [ ] Create `TASK4_IMPLEMENTATION_COMPLETE.md`
- [ ] Request PM build verification
- [ ] Address any PM feedback
- [ ] Mark Task 4 complete in TaskMaster

---

## PM Review & Approval Request

**Agent 2 Requesting PM Approval**:

This architectural plan documents the complete design for Task 4 (TLV Protocol Parser). Key highlights:

1. **Comprehensive Protocol Specification**: Binary TLV format with CRC32 validation
2. **State Machine Design**: IDLE → RECEIVING → VALIDATING → STORING
3. **Memory Management**: 256KB buffered uploads with pool allocation
4. **Integration Strategy**: Clear handoff points with Tasks 3, 5, 8
5. **Testing Plan**: Unit tests + manual hardware tests
6. **6-Phase Implementation**: Subtasks mapped to phases with clear deliverables

**Estimated Total Time**: 14-20 hours across 5 subtasks

**Ready to Begin Implementation**: Awaiting PM approval to proceed with Phase 1.

**Questions for PM**:
1. Approve single global upload session vs per-client sessions?
2. Approve timeout handling strategy (rely on WebSocket timeout)?
3. Approve buffered upload strategy (vs streaming to storage)?

---

**Agent 2**
2025-10-16 | Task 4: TLV Protocol Parser - Architecture Complete ✅
