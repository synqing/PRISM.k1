# Task 4 Brief: TLV Protocol Parser & Command Dispatcher
**Agent:** Agent 4 (Claude Code - NOW ASSIGNED)
**Status:** Ready to start immediately
**Dependencies:** Task 3 ✅ COMPLETE
**Priority:** CRITICAL PATH - Unblocks entire protocol stack

---

## 🚨 URGENT REASSIGNMENT

**Agent 4, you are reassigned from idle to Task 4 immediately.**

**Why:** Task 3 (WebSocket) is complete. Task 4 is now unblocked and is CRITICAL PATH.
**Previous:** Task 8 (LED Driver) - ✅ Complete
**Current:** Task 4 (TLV Protocol Parser) - 🔄 Starting NOW

---

## Mission

Implement binary TLV protocol parser that validates frames, performs CRC checks, and dispatches commands to storage and playback subsystems.

## Completion Criteria

✅ TLV frame validation (Type-Length-Value parsing)
✅ CRC32 verification using esp_rom_crc32
✅ Upload session state machine (PUT_BEGIN → PUT_DATA → PUT_END)
✅ 256KB size enforcement per ADR-004
✅ Command dispatch to storage & playback
✅ Error frames with ADR-002 codes (0x02, 0x03, 0x04, 0x05)
✅ Unity tests with crafted frames
✅ Integration with Task 3 WebSocket handlers

---

## Critical Context

### What's Already Done (Task 3)
Agent 2 completed WebSocket server with:
- Binary frame reception (2-step: header → payload)
- Session management (2 clients, 4KB buffers)
- Timeout detection (5 seconds)
- Basic frame structures

**Your job:** Parse those binary frames and dispatch commands.

### File Structure
```
components/network/
├── network_manager.c          # Existing (WiFi + WebSocket)
├── network_private.h          # Existing (internal structures)
├── protocol_parser.c          # NEW - Task 4 implementation
├── protocol_parser.h          # NEW - Task 4 API
└── test/
    └── test_protocol.c        # NEW - Unity tests
```

---

## TLV Protocol Specification (ADR-002)

### Frame Format
```
[TYPE: 1 byte][LENGTH: 2 bytes][PAYLOAD: N bytes][CRC32: 4 bytes]
```

### Message Types
```c
// Upload commands
#define MSG_PUT_BEGIN   0x10    // Start pattern upload
#define MSG_PUT_DATA    0x11    // Pattern data chunk
#define MSG_PUT_END     0x12    // Finalize pattern upload

// Management commands
#define MSG_DELETE      0x20    // Delete pattern
#define MSG_LIST        0x21    // List patterns

// Playback commands
#define MSG_CONTROL     0x30    // Playback control (play/pause/crossfade)
#define MSG_STATUS      0x31    // Request status/telemetry

// Response types
#define MSG_ACK         0x40    // Success acknowledgment
#define MSG_ERROR       0x41    // Error response
```

### Error Codes (ADR-002)
```c
#define ERR_INVALID_CRC     0x02    // CRC mismatch
#define ERR_INVALID_TLV     0x03    // Malformed frame
#define ERR_STORAGE_FULL    0x04    // Pattern storage full
#define ERR_NOT_FOUND       0x05    // Pattern not found
#define ERR_SIZE_EXCEEDED   0x06    // Pattern > 256KB (ADR-004)
```

---

## Implementation Plan

### Phase 1: TLV Parser Core (Subtask 4.1)

**File:** `components/network/protocol_parser.c`

```c
#include "protocol_parser.h"
#include "esp_rom_crc32.h"
#include "prism_config.h"

typedef struct {
    uint8_t type;
    uint16_t length;
    uint8_t *payload;
    uint32_t crc32;
} tlv_frame_t;

/**
 * @brief Parse TLV frame from binary data
 * @param data Raw frame data
 * @param len Data length
 * @param frame Output parsed frame
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t protocol_parse_tlv(const uint8_t *data, size_t len, tlv_frame_t *frame) {
    if (!data || !frame || len < 7) {  // Min: 1+2+0+4 = 7 bytes
        return ESP_ERR_INVALID_ARG;
    }

    // Parse header
    frame->type = data[0];
    frame->length = (data[1] << 8) | data[2];
    frame->payload = (uint8_t *)&data[3];

    // Validate length
    size_t expected_len = 3 + frame->length + 4;  // Header + payload + CRC
    if (len != expected_len) {
        ESP_LOGE(TAG, "Length mismatch: got %zu, expected %zu", len, expected_len);
        return ESP_ERR_INVALID_SIZE;
    }

    // Extract CRC32 (last 4 bytes)
    uint32_t received_crc = 0;
    const uint8_t *crc_bytes = &data[3 + frame->length];
    received_crc = (crc_bytes[0] << 24) | (crc_bytes[1] << 16) |
                   (crc_bytes[2] << 8)  | crc_bytes[3];

    // Calculate CRC32 (over TYPE + LENGTH + PAYLOAD)
    uint32_t calculated_crc = esp_rom_crc32_le(0, data, 3 + frame->length);

    if (calculated_crc != received_crc) {
        ESP_LOGE(TAG, "CRC mismatch: got 0x%08lX, expected 0x%08lX",
                 received_crc, calculated_crc);
        return ESP_ERR_INVALID_CRC;
    }

    frame->crc32 = received_crc;
    return ESP_OK;
}
```

### Phase 2: Upload Session State Machine (Subtask 4.2)

```c
typedef enum {
    UPLOAD_IDLE,
    UPLOAD_IN_PROGRESS,
    UPLOAD_FINALIZING
} upload_state_t;

typedef struct {
    upload_state_t state;
    char pattern_id[32];
    uint32_t total_size;
    uint32_t bytes_received;
    uint32_t running_crc;
    TickType_t start_time;
} upload_session_t;

static upload_session_t upload_sessions[WS_MAX_CLIENTS];  // One per client

esp_err_t protocol_handle_put_begin(int client_id, const uint8_t *payload, uint16_t len) {
    if (upload_sessions[client_id].state != UPLOAD_IDLE) {
        ESP_LOGW(TAG, "Upload already in progress for client %d", client_id);
        return ESP_ERR_INVALID_STATE;
    }

    // Parse PUT_BEGIN payload: [pattern_id: 32 bytes][total_size: 4 bytes]
    if (len < 36) {
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(upload_sessions[client_id].pattern_id, payload, 32);
    upload_sessions[client_id].total_size =
        (payload[32] << 24) | (payload[33] << 16) |
        (payload[34] << 8)  | payload[35];

    // Enforce 256KB limit (ADR-004)
    if (upload_sessions[client_id].total_size > PATTERN_SIZE_MAX) {
        ESP_LOGE(TAG, "Pattern size %lu exceeds max %d",
                 upload_sessions[client_id].total_size, PATTERN_SIZE_MAX);
        return ESP_ERR_INVALID_SIZE;
    }

    // Initialize session
    upload_sessions[client_id].state = UPLOAD_IN_PROGRESS;
    upload_sessions[client_id].bytes_received = 0;
    upload_sessions[client_id].running_crc = 0;
    upload_sessions[client_id].start_time = xTaskGetTickCount();

    ESP_LOGI(TAG, "Upload started: pattern=%s, size=%lu bytes",
             upload_sessions[client_id].pattern_id,
             upload_sessions[client_id].total_size);

    return ESP_OK;
}

esp_err_t protocol_handle_put_data(int client_id, const uint8_t *payload, uint16_t len) {
    if (upload_sessions[client_id].state != UPLOAD_IN_PROGRESS) {
        return ESP_ERR_INVALID_STATE;
    }

    // Update CRC
    upload_sessions[client_id].running_crc = esp_rom_crc32_le(
        upload_sessions[client_id].running_crc, payload, len);

    // Write to storage (Task 5 - stub for now)
    // esp_err_t ret = storage_pattern_write_chunk(
    //     upload_sessions[client_id].pattern_id,
    //     payload, len);

    upload_sessions[client_id].bytes_received += len;

    ESP_LOGD(TAG, "Received %u/%lu bytes",
             upload_sessions[client_id].bytes_received,
             upload_sessions[client_id].total_size);

    return ESP_OK;
}

esp_err_t protocol_handle_put_end(int client_id, const uint8_t *payload, uint16_t len) {
    if (upload_sessions[client_id].state != UPLOAD_IN_PROGRESS) {
        return ESP_ERR_INVALID_STATE;
    }

    // Verify size
    if (upload_sessions[client_id].bytes_received != upload_sessions[client_id].total_size) {
        ESP_LOGE(TAG, "Size mismatch: received %lu, expected %lu",
                 upload_sessions[client_id].bytes_received,
                 upload_sessions[client_id].total_size);
        upload_sessions[client_id].state = UPLOAD_IDLE;
        return ESP_ERR_INVALID_SIZE;
    }

    // Verify CRC from payload
    if (len < 4) {
        return ESP_ERR_INVALID_SIZE;
    }

    uint32_t expected_crc = (payload[0] << 24) | (payload[1] << 16) |
                           (payload[2] << 8)  | payload[3];

    if (upload_sessions[client_id].running_crc != expected_crc) {
        ESP_LOGE(TAG, "Upload CRC mismatch: got 0x%08lX, expected 0x%08lX",
                 upload_sessions[client_id].running_crc, expected_crc);
        upload_sessions[client_id].state = UPLOAD_IDLE;
        return ESP_ERR_INVALID_CRC;
    }

    // Finalize storage (Task 5 - stub for now)
    // esp_err_t ret = storage_pattern_finalize(upload_sessions[client_id].pattern_id);

    ESP_LOGI(TAG, "Upload complete: pattern=%s, %lu bytes",
             upload_sessions[client_id].pattern_id,
             upload_sessions[client_id].bytes_received);

    upload_sessions[client_id].state = UPLOAD_IDLE;
    return ESP_OK;
}
```

### Phase 3: Command Dispatcher (Subtasks 4.3 & 4.4)

```c
/**
 * @brief Main protocol dispatcher - called from WebSocket handler
 */
esp_err_t protocol_dispatch(int client_id, const uint8_t *frame_data, size_t frame_len) {
    tlv_frame_t frame;
    esp_err_t ret = protocol_parse_tlv(frame_data, frame_len, &frame);

    if (ret != ESP_OK) {
        // Send error frame
        return protocol_send_error(client_id, ERR_INVALID_TLV);
    }

    switch (frame.type) {
        // Upload commands
        case MSG_PUT_BEGIN:
            ret = protocol_handle_put_begin(client_id, frame.payload, frame.length);
            break;

        case MSG_PUT_DATA:
            ret = protocol_handle_put_data(client_id, frame.payload, frame.length);
            break;

        case MSG_PUT_END:
            ret = protocol_handle_put_end(client_id, frame.payload, frame.length);
            break;

        // Management commands
        case MSG_DELETE:
            ret = protocol_handle_delete(client_id, frame.payload, frame.length);
            break;

        case MSG_LIST:
            ret = protocol_handle_list(client_id);
            break;

        // Playback commands
        case MSG_CONTROL:
            ret = protocol_handle_control(client_id, frame.payload, frame.length);
            break;

        case MSG_STATUS:
            ret = protocol_handle_status(client_id);
            break;

        default:
            ESP_LOGW(TAG, "Unknown message type: 0x%02X", frame.type);
            return protocol_send_error(client_id, ERR_INVALID_TLV);
    }

    // Send ACK or ERROR based on result
    if (ret == ESP_OK) {
        return protocol_send_ack(client_id, frame.type);
    } else {
        return protocol_send_error(client_id, map_error_code(ret));
    }
}
```

### Phase 4: Integration with Task 3 WebSocket

In `network_manager.c`, update the WebSocket receive handler:

```c
#include "protocol_parser.h"

static esp_err_t ws_receive_frame(httpd_req_t *req, ws_client_session_t *client) {
    // ... existing frame receive code from Task 3 ...

    // NEW: Dispatch to protocol parser
    if (frame.type == HTTPD_WS_TYPE_BINARY && frame.len > 0) {
        esp_err_t ret = protocol_dispatch(client_id, frame.payload, frame.len);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Protocol dispatch failed: %s", esp_err_to_name(ret));
        }
    }

    return ESP_OK;
}
```

### Phase 5: Unity Tests (Subtask 4.5)

**File:** `test/test_protocol.c`

```c
#include "unity.h"
#include "protocol_parser.h"

TEST_CASE("protocol_parse_tlv validates CRC", "[task4][protocol]")
{
    // Valid frame: PUT_BEGIN with correct CRC
    uint8_t frame[] = {
        0x10,              // TYPE: PUT_BEGIN
        0x00, 0x04,        // LENGTH: 4 bytes
        0x01, 0x02, 0x03, 0x04,  // PAYLOAD
        0x00, 0x00, 0x00, 0x00   // CRC32 (calculated below)
    };

    // Calculate correct CRC
    uint32_t crc = esp_rom_crc32_le(0, frame, 7);
    frame[7] = (crc >> 24) & 0xFF;
    frame[8] = (crc >> 16) & 0xFF;
    frame[9] = (crc >> 8) & 0xFF;
    frame[10] = crc & 0xFF;

    tlv_frame_t parsed;
    TEST_ASSERT_EQUAL(ESP_OK, protocol_parse_tlv(frame, sizeof(frame), &parsed));
    TEST_ASSERT_EQUAL(0x10, parsed.type);
    TEST_ASSERT_EQUAL(4, parsed.length);
}

TEST_CASE("protocol_parse_tlv rejects bad CRC", "[task4][protocol]")
{
    uint8_t frame[] = {
        0x10, 0x00, 0x04,
        0x01, 0x02, 0x03, 0x04,
        0xFF, 0xFF, 0xFF, 0xFF  // Invalid CRC
    };

    tlv_frame_t parsed;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_CRC, protocol_parse_tlv(frame, sizeof(frame), &parsed));
}

TEST_CASE("upload_session enforces 256KB limit", "[task4][protocol]")
{
    // PUT_BEGIN with 300KB size
    uint8_t payload[36] = {0};
    strcpy((char*)payload, "test_pattern");
    payload[32] = 0x00;
    payload[33] = 0x04;  // 300KB
    payload[34] = 0x93;
    payload[35] = 0x00;

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, protocol_handle_put_begin(0, payload, 36));
}
```

---

## Storage Integration Strategy

⚠️ **Task 5 NOT YET COMPLETE** - Use stubs for now.

**Current approach:**
```c
// In protocol_parser.c
esp_err_t protocol_handle_delete(int client_id, const uint8_t *payload, uint16_t len) {
    // TODO(Task 5): Call storage_pattern_delete()
    ESP_LOGW(TAG, "Storage stub: would delete pattern");
    return ESP_OK;
}
```

**After Task 5 completes:**
```c
#include "pattern_storage.h"

esp_err_t protocol_handle_delete(int client_id, const uint8_t *payload, uint16_t len) {
    char pattern_id[32];
    memcpy(pattern_id, payload, min(len, 32));

    esp_err_t ret = storage_pattern_delete(pattern_id);
    if (ret == ESP_ERR_NOT_FOUND) {
        return protocol_send_error(client_id, ERR_NOT_FOUND);
    }

    return ret;
}
```

---

## File Structure

### protocol_parser.h
```c
#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

// Message types
#define MSG_PUT_BEGIN   0x10
#define MSG_PUT_DATA    0x11
#define MSG_PUT_END     0x12
#define MSG_DELETE      0x20
#define MSG_LIST        0x21
#define MSG_CONTROL     0x30
#define MSG_STATUS      0x31
#define MSG_ACK         0x40
#define MSG_ERROR       0x41

// Error codes
#define ERR_INVALID_CRC     0x02
#define ERR_INVALID_TLV     0x03
#define ERR_STORAGE_FULL    0x04
#define ERR_NOT_FOUND       0x05
#define ERR_SIZE_EXCEEDED   0x06

/**
 * @brief Main protocol dispatcher
 */
esp_err_t protocol_dispatch(int client_id, const uint8_t *frame_data, size_t frame_len);

/**
 * @brief Initialize protocol parser
 */
esp_err_t protocol_parser_init(void);

/**
 * @brief Deinitialize protocol parser
 */
esp_err_t protocol_parser_deinit(void);
```

---

## CMakeLists.txt Update

```cmake
idf_component_register(
    SRCS
        "network_manager.c"
        "protocol_parser.c"      # NEW
    INCLUDE_DIRS "include"
    REQUIRES
        freertos
        esp_http_server
        esp_netif
        nvs_flash
        core
    PRIV_REQUIRES
        mdns
)
```

---

## Success Criteria

- [ ] TLV frame parsing with CRC verification
- [ ] Upload state machine (PUT_BEGIN → DATA → END)
- [ ] 256KB size enforcement (ADR-004)
- [ ] All message types handled (0x10-0x41)
- [ ] Error codes per ADR-002 (0x02-0x06)
- [ ] Storage command stubs (ready for Task 5)
- [ ] Playback command stubs (ready for Task 9)
- [ ] Unity tests pass (CRC, size, state machine)
- [ ] Integration with Task 3 WebSocket
- [ ] Build succeeds
- [ ] **COMMIT AFTER EACH SUBTASK**

---

## Testing Strategy

### Unit Tests
```bash
cd firmware
idf.py test protocol
```

### Integration Test (Manual)
```python
# test_protocol.py
import websocket
import struct

ws = websocket.create_connection("ws://prism-k1.local/ws")

# Send PUT_BEGIN
pattern_id = b"test_pattern" + b'\x00' * 20  # 32 bytes
total_size = struct.pack('>I', 1024)  # 1KB
payload = pattern_id + total_size

# Build TLV frame
frame = bytes([0x10]) + struct.pack('>H', len(payload)) + payload
crc = calculate_crc32(frame)
frame += struct.pack('>I', crc)

ws.send_binary(frame)
response = ws.recv()
print(f"Response: {response.hex()}")
```

---

## COMMIT PROTOCOL

**After EACH subtask:**
```bash
git add components/network/protocol_parser.c
git add components/network/protocol_parser.h
git commit -m "feat(task-4.X): <description>"
task-master set-status --id=4.X --status=done
```

---

**Agent 4, you are cleared to begin Task 4 immediately. Report progress after each subtask. Let's keep the critical path moving! 🚀**
