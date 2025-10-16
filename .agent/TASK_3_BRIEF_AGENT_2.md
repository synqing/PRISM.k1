# Task 3 Brief: WebSocket Binary TLV Server
**Agent:** Agent 2 (Claude Code - Network Track)
**Status:** Ready to start
**Dependencies:** Task 1 ✅ | Task 2 ✅
**PM Status:** Tasks 2 & 8 integrated, build verified successful

---

## Mission
Implement production WebSocket server in `components/network/` supporting binary TLV protocol for pattern upload/management operations.

## Completion Criteria
✅ Binary TLV frame handling (NOT JSON)
✅ 2 concurrent client limit enforced
✅ 4KB RX buffer per client (fixed allocation)
✅ State machine: IDLE→RECEIVING→VALIDATING→STORING
✅ Exponential backoff reconnection
✅ ADR-002 error codes implemented
✅ Unity tests with mocked frames
✅ 500KB/s throughput validated

## Critical Specifications (CANON.md - ADR-002)

### WebSocket Configuration
```c
#define WS_MAX_CLIENTS      2       // Per ADR-002
#define WS_BUFFER_SIZE      4096    // 4KB RX buffer per client
#define WS_TIMEOUT_MS       5000    // Frame receive timeout
```

### Binary TLV Protocol (NOT JSON!)
**Frame Structure:**
```
[Type: 1 byte][Length: 2 bytes][Value: N bytes]
```

**Message Types (Per ADR-002):**
- `0x10` - PUT_BEGIN (start pattern upload)
- `0x11` - PUT_CHUNK (pattern data chunk)
- `0x12` - PUT_END (finalize pattern)
- `0x20` - DELETE (remove pattern)
- `0x21` - LIST (request pattern list)
- `0x30` - STATUS (heap/cache metrics response)
- `0x40` - ERROR (error response)

**Error Codes:**
- `0x01` - MAX_CLIENTS_REACHED
- `0x02` - BUFFER_OVERFLOW
- `0x03` - INVALID_TLV
- `0x04` - STORAGE_FULL
- `0x05` - PATTERN_NOT_FOUND

### State Machine
```
IDLE ──[0x10]──> RECEIVING ──[validate]──> VALIDATING ──[store]──> STORING
  ^                  │                         │                      │
  └──────────────────┴─────────────────────────┴──────────────────────┘
                            [timeout/error]
```

## Architecture

### File Structure
```
components/network/
├── network_manager.c          # Existing (Task 2) ✅
├── network_private.h          # Existing (Task 2) ✅
├── websocket_handler.c        # NEW - Task 3 implementation
├── websocket_handler.h        # NEW - Task 3 API
├── idf_component.yml          # Existing (mdns dependency)
└── test/
    └── test_websocket.c       # NEW - Unity tests
```

### Integration Points

**Task 2 Components (Available):**
- `network_manager.c` - HTTP server already running on port 80
- `httpd_handle_t` - Use existing server handle
- `network_private.h` - Internal network structures

**Task 5 Components (NOT YET AVAILABLE):**
- ⚠️ Storage APIs (Agent 3 working on Task 5)
- ⚠️ Pattern validation (Task 6 - blocked)
- For now: stub storage calls or queue operations

**Core Components (Available from Task 1):**
- `prism_heap_monitor.h` - Heap metrics for STATUS frames
- `prism_memory_pool.h` - Fixed buffer allocation
- `prism_error.h` - Error code framework

## Implementation Phases

### Phase 1: WebSocket Upgrade (Subtask 3.1)
**File:** `websocket_handler.c`

```c
// Client session structure
typedef struct {
    int fd;                          // Socket file descriptor
    uint8_t *rx_buffer;              // 4KB from memory pool
    ws_state_t state;                // Current state machine state
    uint32_t bytes_received;         // For multi-chunk uploads
    TickType_t last_activity;        // For timeout detection
} ws_client_t;

static ws_client_t clients[WS_MAX_CLIENTS];
static httpd_handle_t ws_server;     // From network_manager

esp_err_t websocket_handler_init(httpd_handle_t server) {
    ws_server = server;

    // Register WebSocket URI handler at /ws
    httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = true
    };

    return httpd_register_uri_handler(server, &ws_uri);
}
```

### Phase 2: Client Limit Enforcement (Subtask 3.2)
```c
static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        // Find free client slot
        int slot = find_free_slot();
        if (slot < 0) {
            // Send error 0x01 - MAX_CLIENTS_REACHED
            send_error_frame(req, 0x01);
            return ESP_FAIL;
        }

        // Allocate 4KB buffer from pool
        clients[slot].rx_buffer = memory_pool_alloc(WS_BUFFER_SIZE);
        clients[slot].fd = httpd_req_to_sockfd(req);
        clients[slot].state = WS_STATE_IDLE;

        return ESP_OK;
    }

    // Handle WebSocket frames...
    return ws_receive_frame(req);
}
```

### Phase 3: TLV State Machine (Subtask 3.3)
```c
static esp_err_t ws_receive_frame(httpd_req_t *req) {
    httpd_ws_frame_t frame = {
        .type = HTTPD_WS_TYPE_BINARY,
        .payload = client->rx_buffer,
        .len = WS_BUFFER_SIZE
    };

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, WS_TIMEOUT_MS);
    if (ret != ESP_OK) {
        handle_timeout(client);
        return ret;
    }

    // Parse TLV header
    uint8_t type = frame.payload[0];
    uint16_t length = (frame.payload[1] << 8) | frame.payload[2];
    uint8_t *value = &frame.payload[3];

    // Validate length
    if (length + 3 > frame.len) {
        send_error_frame(req, 0x03);  // INVALID_TLV
        return ESP_ERR_INVALID_SIZE;
    }

    // State machine dispatch
    switch (client->state) {
        case WS_STATE_IDLE:
            if (type == 0x10) {  // PUT_BEGIN
                return handle_put_begin(client, value, length);
            } else if (type == 0x21) {  // LIST
                return handle_list_request(client);
            }
            break;

        case WS_STATE_RECEIVING:
            if (type == 0x11) {  // PUT_CHUNK
                return handle_put_chunk(client, value, length);
            } else if (type == 0x12) {  // PUT_END
                return handle_put_end(client);
            }
            break;

        // ... other states
    }

    return ESP_OK;
}
```

### Phase 4: Reconnect & Error Handling (Subtask 3.4)
```c
// Exponential backoff for reconnection
static uint32_t calculate_backoff(ws_client_t *client) {
    uint32_t base = 1000;  // 1 second
    uint32_t max = 30000;  // 30 seconds

    uint32_t delay = base * (1 << client->reconnect_attempts);
    return (delay > max) ? max : delay;
}

static void handle_disconnect(ws_client_t *client) {
    // Free resources
    memory_pool_free(client->rx_buffer);

    // Schedule reconnect
    client->reconnect_timer = xTimerCreate(
        "ws_reconnect",
        pdMS_TO_TICKS(calculate_backoff(client)),
        pdFALSE,
        client,
        reconnect_timer_callback
    );

    // Log error
    error_handler(PRISM_ERR_WS_DISCONNECT, "Client %d disconnected", client->fd);
}
```

### Phase 5: Testing (Subtask 3.5)
**File:** `test/test_websocket.c`

```c
TEST_CASE("WebSocket enforces two client limit", "[task3][websocket]")
{
    // Mock three connection attempts
    httpd_req_t req1, req2, req3;

    // First two should succeed
    TEST_ASSERT_EQUAL(ESP_OK, ws_handler(&req1));
    TEST_ASSERT_EQUAL(ESP_OK, ws_handler(&req2));

    // Third should fail with error 0x01
    TEST_ASSERT_EQUAL(ESP_FAIL, ws_handler(&req3));

    // Verify error frame sent
    TEST_ASSERT_EQUAL(0x40, last_frame.type);      // ERROR
    TEST_ASSERT_EQUAL(0x01, last_frame.payload[0]); // MAX_CLIENTS_REACHED
}

TEST_CASE("WebSocket validates TLV frame structure", "[task3][websocket]")
{
    ws_client_t client;

    // Valid TLV frame
    uint8_t valid[] = {0x10, 0x00, 0x04, 0x01, 0x02, 0x03, 0x04};
    TEST_ASSERT_EQUAL(ESP_OK, parse_tlv_frame(&client, valid, sizeof(valid)));

    // Invalid length (truncated)
    uint8_t invalid[] = {0x10, 0x00, 0xFF, 0x01};
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, parse_tlv_frame(&client, invalid, sizeof(invalid)));
}
```

## Key Constraints

### Memory Budget (Task 3 Allocation)
- 2 clients × 4KB buffers = **8KB heap** (from memory pool)
- Client metadata: 2 × 128 bytes = **256 bytes**
- State machine overhead: **512 bytes**
- **Total: ~9KB** (well within 150KB budget)

### Performance Targets
- Frame latency: <10ms (single-threaded OK)
- Throughput: 500KB/s sustained (validated with test script)
- No heap growth over 1 hour operation

### ESP-IDF APIs
```c
#include "esp_http_server.h"

// WebSocket functions
httpd_ws_recv_frame()      // Receive binary frame
httpd_ws_send_frame()      // Send binary frame
httpd_req_to_sockfd()      // Get socket FD
httpd_sess_trigger_close() // Force disconnect
```

## Integration with Existing Code

### network_manager.c Changes (Minimal)
```c
// In network_manager.c, add after HTTP server start:

#include "websocket_handler.h"

static esp_err_t start_webserver(void) {
    // ... existing HTTP server setup ...

    // Register WebSocket handler (Task 3)
    ESP_ERROR_CHECK(websocket_handler_init(server));

    ESP_LOGI(TAG, "WebSocket server ready at ws://prism-k1.local/ws");
    return ESP_OK;
}
```

### CMakeLists.txt Update
```cmake
idf_component_register(
    SRCS
        "network_manager.c"
        "websocket_handler.c"      # NEW
    INCLUDE_DIRS "include"
    REQUIRES
        freertos
        esp_http_server
        esp_netif
        nvs_flash
        core                        # For memory pools
    PRIV_REQUIRES
        mdns
)
```

## Storage Integration Strategy

⚠️ **Task 5 NOT YET COMPLETE** - Agent 3 is working on storage.

**For now, use stub handlers:**
```c
static esp_err_t handle_put_begin(ws_client_t *client, uint8_t *data, uint16_t len) {
    // TODO(Task 5): Call storage_pattern_create()
    ESP_LOGW(TAG, "Storage stub: would create pattern ID from data");

    // For now, just acknowledge
    uint8_t response[] = {0x30, 0x00, 0x01, 0x00};  // STATUS: OK
    send_frame(client, response, sizeof(response));

    client->state = WS_STATE_RECEIVING;
    return ESP_OK;
}
```

**After Task 5 completes, replace with:**
```c
#include "pattern_storage.h"

static esp_err_t handle_put_begin(ws_client_t *client, uint8_t *data, uint16_t len) {
    esp_err_t ret = storage_pattern_create(data, len, &client->pattern_id);
    if (ret != ESP_OK) {
        send_error_frame(client, 0x04);  // STORAGE_FULL
        return ret;
    }

    client->state = WS_STATE_RECEIVING;
    return ESP_OK;
}
```

## Testing Approach

### Unit Tests (Unity)
```bash
# Run WebSocket unit tests
cd firmware
idf.py test websocket
```

### Integration Test (Manual)
```bash
# Terminal 1: Flash and monitor
idf.py flash monitor

# Terminal 2: Test client script
cat > test_websocket.py << 'EOF'
import asyncio
import websockets

async def test_tlv():
    uri = "ws://prism-k1.local/ws"
    async with websockets.connect(uri) as ws:
        # Send LIST command (0x21)
        await ws.send(bytes([0x21, 0x00, 0x00]))

        # Receive response
        response = await ws.recv()
        print(f"Response: {response.hex()}")

asyncio.run(test_tlv())
EOF

python test_websocket.py
```

### Throughput Test
```python
# test_throughput.py - 500KB/s sustained load
import asyncio
import websockets
import time

async def throughput_test():
    uri = "ws://prism-k1.local/ws"
    async with websockets.connect(uri) as ws:
        # Send PUT_BEGIN
        await ws.send(bytes([0x10, 0x00, 0x04]) + b"test")

        # Send 500KB in chunks
        chunk_size = 1024
        total_size = 500 * 1024
        start = time.time()

        for i in range(total_size // chunk_size):
            chunk = bytes([0x11, 0x04, 0x00]) + b'\xff' * chunk_size
            await ws.send(chunk)

        # Send PUT_END
        await ws.send(bytes([0x12, 0x00, 0x00]))

        elapsed = time.time() - start
        rate = (total_size / 1024) / elapsed
        print(f"Throughput: {rate:.2f} KB/s")

asyncio.run(throughput_test())
```

## Success Criteria Checklist

- [ ] WebSocket endpoint registered at `/ws`
- [ ] Two concurrent clients max (third rejected with 0x01)
- [ ] 4KB RX buffer per client from memory pool
- [ ] Binary TLV parsing (type, length, value)
- [ ] State machine: IDLE→RECEIVING→VALIDATING→STORING
- [ ] All message types handled: 0x10-0x12, 0x20-0x21, 0x30, 0x40
- [ ] Error codes per ADR-002: 0x01-0x05
- [ ] Timeout detection (5 seconds)
- [ ] Exponential backoff reconnection (1s → 30s)
- [ ] Unity tests pass (buffer, state, error handling)
- [ ] Throughput: 500KB/s sustained, no heap growth
- [ ] Integration with network_manager.c (HTTP server)
- [ ] Storage stubs in place (ready for Task 5)

## Reference Documentation

**Must Read:**
- `.taskmaster/CANON.md` - ADR-002 (WebSocket Buffer Size)
- `.taskmaster/decisions/002-websocket-buffer-size.md` - Full ADR
- `components/network/network_manager.c` - Existing HTTP server (Task 2)
- `components/core/include/prism_memory_pool.h` - Buffer allocation

**ESP-IDF Docs:**
- [WebSocket Server](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/protocols/esp_http_server.html#websocket-server)
- [httpd_ws_recv_frame](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/protocols/esp_http_server.html#_CPPv420httpd_ws_recv_frameP12httpd_req_tP18httpd_ws_frame_t8uint32_t)

## PM Notes

**What's Ready:**
- ✅ HTTP server running (Task 2)
- ✅ mDNS at prism-k1.local (Task 2)
- ✅ Memory pools available (Task 1)
- ✅ Build system verified (832KB binary, 46% free)

**What's Blocked:**
- ⚠️ Storage APIs (Agent 3 working Task 5)
- ⚠️ Pattern validation (Task 6 - depends on Task 5)

**Commit Strategy:**
- Commit after each subtask completion
- Use git messages: "feat(task-3.X): <description>"
- Tag PM when ready for integration testing

**Questions for PM:**
- Report subtask completion in AGENT_ASSIGNMENTS.md
- Request integration testing after subtask 3.3
- Flag any CANON.md conflicts immediately

---

**Agent 2, you have clearance to begin Task 3. Report progress per subtask. Good luck! 🚀**
