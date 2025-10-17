# Task 3: WebSocket Server with TLV Protocol - Architectural Plan

**Generated**: 2025-10-16
**Status**: Pre-Implementation Analysis
**Agent**: Agent 2
**Complexity**: 8/10

## Executive Summary

This document maps out the complete architecture for Task 3 (WebSocket Server with TLV Protocol) before any code implementation. It identifies all integration points, function signatures, data structures, memory allocation strategies, and dependencies required to add binary WebSocket communication to the existing network_manager component.

---

## 1. Current State Analysis

### 1.1 Existing Infrastructure

**HTTP Server** (network_manager.c:535-596)
- Already running on port 80 via `g_net_state.http_server`
- Serving captive portal with 3 URI handlers:
  - `GET /` - Portal HTML form
  - `POST /connect` - Credential submission
  - `GET /*` - Wildcard for captive portal detection
- Server lifecycle: Started in `start_captive_portal()`, can be stopped in `stop_captive_portal()`

**Network State Structure** (network_private.h:29-48)
```c
typedef struct {
    wifi_op_mode_t current_mode;
    esp_netif_t *ap_netif;
    esp_netif_t *sta_netif;
    bool wifi_initialized;
    uint32_t retry_count;
    uint32_t retry_delay_ms;
    httpd_handle_t http_server;      // ← REUSE THIS FOR WEBSOCKET
    bool portal_active;
    bool credentials_available;
    char sta_ssid[33];
    char sta_password[64];
    bool mdns_initialized;
} network_state_t;
```

**Memory Pool System** (from core component)
- `prism_pool_alloc()` - Custom allocator for controlled heap usage
- `prism_pool_free()` - Matching deallocation
- Used throughout network_manager for temporary buffers

**CANON Specifications** (ADR-002)
- `WS_BUFFER_SIZE`: 4096 bytes
- `WS_MAX_CLIENTS`: 2 concurrent connections
- `WS_TIMEOUT_MS`: 5000 milliseconds

---

## 2. WebSocket Integration Architecture

### 2.1 State Structure Extensions

**New Data Structures** (to be added to network_private.h)

```c
/**
 * @brief WebSocket client session state
 */
typedef struct {
    bool active;                    // Slot occupied?
    int socket_fd;                  // File descriptor for socket
    uint32_t last_activity_ms;      // Last received data timestamp (for timeout)
    uint8_t* rx_buffer;             // 4KB receive buffer (prism_pool_alloc)
    size_t rx_buffer_size;          // Always WS_BUFFER_SIZE (4096)
    httpd_handle_t server_handle;   // Reference to HTTP server (for sending)
} ws_client_session_t;

/**
 * @brief WebSocket server state (add to network_state_t)
 */
typedef struct {
    // ... existing fields ...
    httpd_handle_t http_server;     // Existing field, now dual-purpose

    // NEW WEBSOCKET FIELDS:
    bool ws_handler_registered;     // WebSocket endpoint registered?
    ws_client_session_t ws_clients[WS_MAX_CLIENTS];  // 2 client slots
    SemaphoreHandle_t ws_mutex;     // Protect client array access
} network_state_t;
```

**Rationale**:
- `active` flag: Quick slot availability check
- `socket_fd`: Required for `httpd_ws_send_frame()` targeting specific client
- `last_activity_ms`: Enables timeout detection (5-second idle check)
- `rx_buffer`: Pre-allocated 4KB buffer per client (reduces allocation overhead)
- `ws_mutex`: FreeRTOS mutex for thread-safe client array access

### 2.2 WebSocket URI Handler Registration

**Integration Point**: Modify `start_captive_portal()` or create new `init_websocket_handler()`

**Option A**: Add to existing `start_captive_portal()` (network_manager.c:535)
```c
esp_err_t start_captive_portal(void) {
    // ... existing HTTP server start code ...

    // Register WebSocket endpoint AFTER HTTP handlers
    httpd_uri_t ws_uri = {
        .uri        = "/ws",               // WebSocket endpoint path
        .method     = HTTP_GET,            // WebSocket upgrade is GET request
        .handler    = ws_handler,          // New handler function
        .user_ctx   = NULL,
        .is_websocket = true,              // ← CRITICAL FLAG
        .handle_ws_control_frames = true   // Handle PING/PONG internally
    };

    ret = httpd_register_uri_handler(g_net_state.http_server, &ws_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WebSocket handler: %s", esp_err_to_name(ret));
        return ret;
    }

    g_net_state.ws_handler_registered = true;
    ESP_LOGI(TAG, "WebSocket handler registered at /ws");

    return ESP_OK;
}
```

**Option B**: Separate function `init_websocket_handler()` called from `network_task()`
- Cleaner separation of concerns
- Can be called independently of captive portal lifecycle
- **RECOMMENDED APPROACH**

**Decision**: Use Option B for modularity

---

## 3. Function Signatures and Call Flow

### 3.1 Public API (network_manager.h additions)

```c
/**
 * @brief Initialize WebSocket handler on existing HTTP server
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t init_websocket_handler(void);

/**
 * @brief Shutdown WebSocket handler and disconnect all clients
 * @return ESP_OK on success
 */
esp_err_t deinit_websocket_handler(void);

/**
 * @brief Send binary frame to all connected WebSocket clients
 * @param data Pointer to binary data
 * @param len Length of data in bytes
 * @return ESP_OK if sent to at least one client
 */
esp_err_t ws_broadcast_binary(const uint8_t* data, size_t len);
```

### 3.2 Internal Functions (network_private.h additions)

```c
/**
 * @brief Main WebSocket request handler (registered with httpd)
 * @param req HTTP request structure (contains WebSocket frame data)
 * @return ESP_OK on success, ESP_FAIL to close connection
 */
esp_err_t ws_handler(httpd_req_t *req);

/**
 * @brief Handle a single WebSocket frame from a client
 * @param req HTTP request structure
 * @param client_idx Index in ws_clients array
 * @return ESP_OK to keep connection, ESP_FAIL to close
 */
esp_err_t handle_ws_frame(httpd_req_t *req, int client_idx);

/**
 * @brief Find a free slot in ws_clients array
 * @return Index 0-1 on success, -1 if no slots available
 */
int find_free_ws_slot(void);

/**
 * @brief Find client index by socket file descriptor
 * @param fd Socket file descriptor from httpd_req_t
 * @return Index 0-1 on success, -1 if not found
 */
int find_ws_client_by_fd(int fd);

/**
 * @brief Check if client has exceeded timeout period
 * @param client_idx Index in ws_clients array
 * @return true if timeout exceeded, false otherwise
 */
bool is_ws_client_timeout(int client_idx);

/**
 * @brief Clean up client session and free resources
 * @param client_idx Index in ws_clients array
 */
void cleanup_ws_client(int client_idx);

/**
 * @brief Send error frame to WebSocket client
 * @param req HTTP request structure
 * @param error_code TLV error code (to be defined in Task 4)
 * @return ESP_OK on success
 */
esp_err_t send_ws_error(httpd_req_t *req, uint8_t error_code);

/**
 * @brief Send status frame to WebSocket client
 * @param req HTTP request structure
 * @param status_code TLV status code
 * @param message Optional status message (can be NULL)
 * @return ESP_OK on success
 */
esp_err_t send_ws_status(httpd_req_t *req, uint8_t status_code, const char* message);
```

### 3.3 Call Flow Diagram

```
USER CLIENT                    ESP32 WEBSOCKET HANDLER              TASK 4 (TLV)
    |                                   |                                |
    |-- WebSocket Upgrade (GET /ws) -->|                                |
    |                                   |-- find_free_ws_slot() ------->|
    |                                   |<- return slot_idx ------------|
    |                                   |-- Allocate 4KB rx_buffer ---->|
    |                                   |-- Store socket_fd ----------->|
    |<---------- 101 Switching ---------|                                |
    |                                   |                                |
    |-- Binary Frame (TLV PUT_BEGIN) ->|                                |
    |                                   |-- find_ws_client_by_fd() ---->|
    |                                   |-- httpd_ws_recv_frame() ----->|
    |                                   |-- handle_ws_frame() --------->|
    |                                   |                                |-- tlv_parse()
    |                                   |                                |-- tlv_validate()
    |                                   |                                |<- ESP_OK
    |                                   |<-------------------------------| (Task 4 logic)
    |<--------- Status Frame (0x30) ---|                                |
    |                                   |                                |
    |-- Binary Frame (Pattern Data) -->|                                |
    |                                   |-- handle_ws_frame() --------->|
    |                                   |                                |-- storage_write()
    |<--------- Status Frame ----------|<-------------------------------|
    |                                   |                                |
    |-- Close Frame ---------------->  |                                |
    |                                   |-- cleanup_ws_client() -------->|
    |                                   |-- Free rx_buffer ------------->|
    |<--------- Close Acknowledged -----|                                |
```

---

## 4. Memory Allocation Strategy

### 4.1 Static Allocations

**WebSocket State (global BSS)**
```c
// In network_manager.c
static network_state_t g_net_state = {0};

// Memory usage:
sizeof(ws_client_session_t) * 2 = (1 + 4 + 4 + 8 + 4 + 4) * 2 = 50 bytes
sizeof(SemaphoreHandle_t) = 8 bytes
sizeof(bool) = 1 byte
Total static: ~60 bytes
```

### 4.2 Dynamic Allocations

**Per-Client RX Buffers** (allocated on connect, freed on disconnect)
```c
// In find_free_ws_slot() or ws_handler() on new connection
uint8_t* rx_buffer = prism_pool_alloc(WS_BUFFER_SIZE);  // 4096 bytes
if (rx_buffer == NULL) {
    ESP_LOGE(TAG, "Failed to allocate WebSocket RX buffer");
    return -1;  // Reject connection
}

g_net_state.ws_clients[slot_idx].rx_buffer = rx_buffer;
g_net_state.ws_clients[slot_idx].rx_buffer_size = WS_BUFFER_SIZE;
```

**Memory Budget**:
- 2 clients × 4096 bytes = 8192 bytes (8KB)
- Plus static 60 bytes = **8252 bytes total**
- Well within 150KB heap budget (<6% usage)

### 4.3 Deallocation (Critical for Memory Safety)

**On Client Disconnect** (in cleanup_ws_client())
```c
void cleanup_ws_client(int client_idx) {
    if (!g_net_state.ws_clients[client_idx].active) {
        return;  // Already cleaned up
    }

    // Free RX buffer
    if (g_net_state.ws_clients[client_idx].rx_buffer != NULL) {
        prism_pool_free(g_net_state.ws_clients[client_idx].rx_buffer);
        g_net_state.ws_clients[client_idx].rx_buffer = NULL;
    }

    // Clear session
    memset(&g_net_state.ws_clients[client_idx], 0, sizeof(ws_client_session_t));

    ESP_LOGI(TAG, "WebSocket client %d cleaned up", client_idx);
}
```

**Deallocation Triggers**:
1. Client sends CLOSE frame
2. Timeout detected (5000ms idle)
3. Connection error (broken pipe, etc.)
4. Server shutdown (`deinit_websocket_handler()`)

---

## 5. ESP-IDF WebSocket API Usage

### 5.1 Receiving Binary Frames

**Two-Step Receive Pattern** (per ESP-IDF documentation)

```c
esp_err_t handle_ws_frame(httpd_req_t *req, int client_idx) {
    // Step 1: Get frame size
    httpd_ws_frame_t ws_pkt = {0};
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;  // Expect binary frames

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);  // max_len=0 queries size
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get frame size: %s", esp_err_to_name(ret));
        return ret;
    }

    // Step 2: Validate size and receive into buffer
    if (ws_pkt.len > WS_BUFFER_SIZE) {
        ESP_LOGW(TAG, "Frame too large (%zu bytes), max is %d", ws_pkt.len, WS_BUFFER_SIZE);
        send_ws_error(req, TLV_ERROR_FRAME_TOO_LARGE);  // TLV error code from Task 4
        return ESP_FAIL;
    }

    // Receive into pre-allocated buffer
    ws_pkt.payload = g_net_state.ws_clients[client_idx].rx_buffer;
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to receive frame: %s", esp_err_to_name(ret));
        return ret;
    }

    // Update activity timestamp
    g_net_state.ws_clients[client_idx].last_activity_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // PASS TO TASK 4 TLV PARSER (placeholder for now)
    ESP_LOGI(TAG, "Received %zu bytes from client %d", ws_pkt.len, client_idx);
    // TODO: tlv_parse(ws_pkt.payload, ws_pkt.len);

    return ESP_OK;
}
```

### 5.2 Sending Binary Frames

**Synchronous Send** (for small status messages <1KB)
```c
esp_err_t send_ws_status(httpd_req_t *req, uint8_t status_code, const char* message) {
    // Build TLV status frame (Task 4 will define format)
    uint8_t frame[128];
    size_t frame_len = 0;

    // Placeholder TLV encoding (Task 4 will implement)
    frame[0] = 0x30;  // Status message type
    frame[1] = status_code;
    frame_len = 2;

    if (message != NULL) {
        size_t msg_len = strlen(message);
        if (msg_len > 125) msg_len = 125;  // Cap at 125 bytes
        memcpy(&frame[2], message, msg_len);
        frame_len += msg_len;
    }

    // Send synchronously
    httpd_ws_frame_t ws_pkt = {0};
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;
    ws_pkt.payload = frame;
    ws_pkt.len = frame_len;

    esp_err_t ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send status frame: %s", esp_err_to_name(ret));
    }

    return ret;
}
```

**Asynchronous Send** (for large pattern data >1KB, not in Task 3 scope)
```c
// Future enhancement for streaming pattern data
// Use httpd_ws_send_frame_async() with callback
```

### 5.3 Broadcast to All Clients

```c
esp_err_t ws_broadcast_binary(const uint8_t* data, size_t len) {
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    httpd_ws_frame_t ws_pkt = {0};
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;
    ws_pkt.payload = (uint8_t*)data;  // Cast away const (httpd API design)
    ws_pkt.len = len;

    int sent_count = 0;

    xSemaphoreTake(g_net_state.ws_mutex, portMAX_DELAY);

    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        if (!g_net_state.ws_clients[i].active) {
            continue;  // Skip inactive slots
        }

        // Build temporary httpd_req_t for sending
        // NOTE: httpd_ws_send_frame() requires httpd_req_t, but for broadcast
        // we need to use httpd_ws_send_data_to_client() or similar
        // INVESTIGATION NEEDED: Check if ESP-IDF has broadcast helper

        // Placeholder - Task 4 will implement proper broadcast
        ESP_LOGD(TAG, "Broadcasting %zu bytes to client %d (fd=%d)",
                 len, i, g_net_state.ws_clients[i].socket_fd);
        sent_count++;
    }

    xSemaphoreGive(g_net_state.ws_mutex);

    return (sent_count > 0) ? ESP_OK : ESP_FAIL;
}
```

**Investigation Required**: ESP-IDF may require different approach for broadcasting without `httpd_req_t`. Research `httpd_queue_work()` for async send from non-handler context.

---

## 6. Timeout and Heartbeat Handling

### 6.1 Timeout Detection

**Passive Timeout Checking** (no active heartbeat pings)

```c
bool is_ws_client_timeout(int client_idx) {
    if (!g_net_state.ws_clients[client_idx].active) {
        return false;  // Not active, can't timeout
    }

    uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t idle_ms = now_ms - g_net_state.ws_clients[client_idx].last_activity_ms;

    if (idle_ms > WS_TIMEOUT_MS) {
        ESP_LOGW(TAG, "Client %d timeout: %lu ms idle (max %d ms)",
                 client_idx, idle_ms, WS_TIMEOUT_MS);
        return true;
    }

    return false;
}
```

**Where to Check Timeouts**:
1. **In `ws_handler()`** - Before processing each frame
2. **In `network_task()` periodic loop** - Check all clients every 1 second

**Timeout Handler Integration** (add to network_task() in network_manager.c)
```c
void network_task(void* pvParameters) {
    while (1) {
        // ... existing WiFi state machine ...

        // NEW: Check WebSocket client timeouts
        if (g_net_state.ws_handler_registered) {
            xSemaphoreTake(g_net_state.ws_mutex, portMAX_DELAY);

            for (int i = 0; i < WS_MAX_CLIENTS; i++) {
                if (is_ws_client_timeout(i)) {
                    ESP_LOGW(TAG, "Cleaning up timed-out WebSocket client %d", i);
                    cleanup_ws_client(i);
                }
            }

            xSemaphoreGive(g_net_state.ws_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // Check every 1 second
    }
}
```

### 6.2 WebSocket PING/PONG (Handled by ESP-IDF)

**Configuration** (already set in URI handler registration)
```c
httpd_uri_t ws_uri = {
    // ...
    .handle_ws_control_frames = true   // ← ESP-IDF handles PING/PONG automatically
};
```

**Behavior**:
- ESP-IDF automatically responds to PING frames with PONG
- No manual PING/PONG handling required
- Control frames do NOT update `last_activity_ms` (investigation needed)

**Investigation Required**: Confirm if control frames reset idle timer or if only DATA frames count.

---

## 7. Error Handling and Edge Cases

### 7.1 Connection Rejection Scenarios

| Scenario | Detection Point | Action |
|----------|----------------|--------|
| **Max clients reached** | `ws_handler()` on new connection | Return `ESP_FAIL` before upgrade completes |
| **RX buffer allocation failure** | `find_free_ws_slot()` | Return `-1`, handler rejects connection |
| **Invalid frame type (text)** | `handle_ws_frame()` | Send error frame, close connection |
| **Frame too large (>4KB)** | `handle_ws_frame()` size check | Send error frame, close connection |
| **Timeout (5s idle)** | `network_task()` periodic check | Close connection, cleanup slot |

### 7.2 Error Frame Format

**Placeholder for Task 4** (TLV protocol will define)
```c
// Proposed structure (to be finalized in Task 4)
typedef struct {
    uint8_t type;        // 0xFF = Error message
    uint8_t error_code;  // Error codes TBD
    uint16_t length;     // Length of error message (optional)
    uint8_t message[];   // Optional error message
} __attribute__((packed)) tlv_error_frame_t;
```

### 7.3 Graceful Shutdown

**On Server Stop** (add to `stop_captive_portal()` or new `deinit_websocket_handler()`)
```c
esp_err_t deinit_websocket_handler(void) {
    if (!g_net_state.ws_handler_registered) {
        return ESP_OK;  // Not initialized
    }

    xSemaphoreTake(g_net_state.ws_mutex, portMAX_DELAY);

    // Send CLOSE frames to all active clients
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        if (g_net_state.ws_clients[i].active) {
            // Send CLOSE frame (ESP-IDF handles close handshake)
            httpd_ws_frame_t ws_pkt = {0};
            ws_pkt.type = HTTPD_WS_TYPE_CLOSE;

            // Build temporary httpd_req_t (investigation needed)
            // PLACEHOLDER: Proper close frame sending

            cleanup_ws_client(i);
        }
    }

    xSemaphoreGive(g_net_state.ws_mutex);

    g_net_state.ws_handler_registered = false;
    ESP_LOGI(TAG, "WebSocket handler deinitialized");

    return ESP_OK;
}
```

---

## 8. Thread Safety and Concurrency

### 8.1 Mutex Protection

**Critical Sections Requiring Mutex**:
1. Accessing `g_net_state.ws_clients[]` array
2. Finding free slots
3. Cleaning up clients
4. Broadcasting to all clients

**Mutex Type**: FreeRTOS Binary Semaphore (created with `xSemaphoreCreateMutex()`)

**Initialization** (in `init_websocket_handler()`)
```c
g_net_state.ws_mutex = xSemaphoreCreateMutex();
if (g_net_state.ws_mutex == NULL) {
    ESP_LOGE(TAG, "Failed to create WebSocket mutex");
    return ESP_ERR_NO_MEM;
}
```

### 8.2 Concurrent Access Patterns

**Scenario 1**: New client connects while `network_task()` is checking timeouts
- **Solution**: Both acquire `ws_mutex` before accessing `ws_clients[]`
- **Worst-case blocking**: <10ms (timeout check is fast)

**Scenario 2**: Two clients send frames simultaneously
- **ESP-IDF Handling**: httpd server uses thread pool, each connection on separate thread
- **Our Handling**: Each `ws_handler()` call acquires mutex briefly to look up client slot
- **Mutex hold time**: <1ms per frame (just array lookup)

**Scenario 3**: Client disconnect while sending frame
- **ESP-IDF Handling**: httpd detects broken pipe and calls handler with error
- **Our Handling**: Cleanup on error return, mutex ensures no double-free

---

## 9. Dependencies and Includes

### 9.1 New Header Includes (network_manager.c)

```c
#include "esp_http_server.h"    // Already included for captive portal
#include <sys/socket.h>          // For socket file descriptor operations
#include "freertos/semphr.h"     // For SemaphoreHandle_t
```

**Note**: `esp_http_server.h` already includes WebSocket types (`httpd_ws_frame_t`, etc.)

### 9.2 CMakeLists.txt Dependencies

**Current Dependencies** (firmware/components/network/CMakeLists.txt)
```cmake
REQUIRES esp_wifi esp_netif esp_event nvs_flash freertos esp_http_server mdns core
PRIV_REQUIRES lwip
```

**No new dependencies required** - WebSocket support is part of `esp_http_server` component.

### 9.3 Task 4 Interface (Forward Declarations)

**Placeholder for TLV Protocol** (to be implemented in Task 4)
```c
// In network_private.h or tlv_protocol.h (Task 4)

/**
 * @brief Parse TLV binary message (Task 4)
 * @param data Pointer to binary TLV data
 * @param len Length of data
 * @param out_cmd Parsed command structure (output)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t tlv_parse(const uint8_t* data, size_t len, tlv_command_t* out_cmd);

/**
 * @brief Encode TLV status message (Task 4)
 * @param status_code Status code
 * @param message Optional message (can be NULL)
 * @param out_buffer Output buffer (caller-allocated)
 * @param buffer_size Size of output buffer
 * @param out_len Actual encoded length (output)
 * @return ESP_OK on success
 */
esp_err_t tlv_encode_status(uint8_t status_code, const char* message,
                             uint8_t* out_buffer, size_t buffer_size, size_t* out_len);
```

**Task 3 Scope**: Implement WebSocket transport, leave TLV parsing as TODO comments for Task 4.

---

## 10. Testing Strategy

### 10.1 Unit Tests (Mocked)

**Test Cases for Task 3** (add to test_network_manager.c)
1. `test_ws_handler_registration` - Verify URI handler registered correctly
2. `test_ws_client_connect_max` - Connect 2 clients, verify 3rd rejected
3. `test_ws_rx_buffer_allocation` - Verify 4KB buffers allocated per client
4. `test_ws_frame_size_validation` - Send >4KB frame, verify rejection
5. `test_ws_timeout_detection` - Simulate 5s idle, verify cleanup
6. `test_ws_client_cleanup` - Verify buffer freed on disconnect
7. `test_ws_broadcast_two_clients` - Send to 2 clients, verify both receive

### 10.2 Integration Tests (Hardware)

**Manual Test Procedure**:
1. Connect to PRISM-K1 WiFi
2. Open WebSocket client (e.g., `websocat` or browser console)
3. Connect to `ws://192.168.4.1/ws`
4. Send binary frame: `[0x10, 0x00, 0x01, 0xAA]` (placeholder TLV)
5. Verify status response received
6. Open 2nd WebSocket connection from another device
7. Verify both connections active
8. Attempt 3rd connection, verify rejection
9. Leave idle for 6 seconds, verify timeout and cleanup
10. Monitor serial logs for memory leaks with `prism_heap_monitor`

**Expected Logs**:
```
I (123) network_manager: WebSocket handler registered at /ws
I (456) network_manager: WebSocket client 0 connected (fd=54)
I (789) network_manager: Received 4 bytes from client 0
I (1000) network_manager: WebSocket client 1 connected (fd=55)
W (7000) network_manager: Client 0 timeout: 6000 ms idle (max 5000 ms)
I (7001) network_manager: WebSocket client 0 cleaned up
```

### 10.3 Load Testing (Future)

**Out of Scope for Task 3**, but document for future testing:
- Rapid connect/disconnect cycles (100 iterations)
- Large frame handling (3.9KB frames, near buffer limit)
- Concurrent frame receiving from both clients
- Stress test with 1 FPS pattern updates for 1 hour

---

## 11. Implementation Checklist

### Phase 1: Data Structures (Subtask 3.1)
- [ ] Add `ws_client_session_t` structure to network_private.h
- [ ] Extend `network_state_t` with WebSocket fields
- [ ] Initialize `ws_mutex` in `init_websocket_handler()`
- [ ] Initialize `ws_clients[]` array to zero

### Phase 2: Lifecycle Functions (Subtask 3.1)
- [ ] Implement `init_websocket_handler()` with URI registration
- [ ] Implement `deinit_websocket_handler()` with graceful close
- [ ] Add WebSocket init call to `network_task()` startup
- [ ] Add WebSocket deinit call to `network_stop()` (if exists)

### Phase 3: Session Management (Subtask 3.2)
- [ ] Implement `find_free_ws_slot()` with mutex protection
- [ ] Implement `find_ws_client_by_fd()` with mutex protection
- [ ] Implement `cleanup_ws_client()` with buffer deallocation
- [ ] Implement `is_ws_client_timeout()` idle check

### Phase 4: Frame Handling (Subtask 3.3)
- [ ] Implement `ws_handler()` main entry point
- [ ] Implement `handle_ws_frame()` with two-step receive
- [ ] Add frame size validation (max 4KB)
- [ ] Add frame type validation (binary only)
- [ ] Update `last_activity_ms` on each receive

### Phase 5: Sending (Subtask 3.3)
- [ ] Implement `send_ws_status()` with placeholder TLV
- [ ] Implement `send_ws_error()` with placeholder TLV
- [ ] Implement `ws_broadcast_binary()` (investigate broadcast method)

### Phase 6: Timeout Integration (Subtask 3.4)
- [ ] Add timeout check loop to `network_task()`
- [ ] Verify timeout triggers cleanup
- [ ] Test 5-second timeout threshold

### Phase 7: Testing (Subtask 3.5)
- [ ] Add 7 unit tests to test_network_manager.c
- [ ] Document manual test procedure in test/README.md
- [ ] Perform hardware validation with 2 clients
- [ ] Verify memory usage <150KB heap

### Phase 8: Documentation
- [ ] Update network_manager.h with new public API
- [ ] Update network_private.h with new internal functions
- [ ] Add Doxygen comments to all new functions
- [ ] Update component README with WebSocket usage examples

---

## 12. Open Questions and Investigations

### 12.1 ESP-IDF Behavior Clarifications

**Q1**: Do WebSocket control frames (PING/PONG) reset the idle timeout?
- **Investigation**: Review ESP-IDF source code for `handle_ws_control_frames` behavior
- **Impact**: May need to track data frame timestamps separately

**Q2**: How to broadcast to multiple clients without `httpd_req_t`?
- **Investigation**: Check if `httpd_queue_work()` can send to specific socket FDs
- **Workaround**: Store `httpd_req_t` copy per client (thread-safety concerns)

**Q3**: Does ESP-IDF automatically handle WebSocket close handshake?
- **Investigation**: Test close frame sending and receiving
- **Impact**: May need manual close frame handling

### 12.2 Integration with Task 4

**Boundary Definition**: Where does Task 3 end and Task 4 begin?
- **Task 3 Scope**: WebSocket transport, frame receive/send, timeout, session management
- **Task 4 Scope**: TLV parsing, command dispatching, pattern storage coordination

**Interface Contract**:
```c
// Task 3 provides to Task 4:
esp_err_t handle_ws_frame(httpd_req_t *req, int client_idx) {
    // ... receive frame into buffer ...

    // HANDOFF TO TASK 4:
    return tlv_dispatch_command(ws_pkt.payload, ws_pkt.len, req);
}

// Task 4 provides to Task 3:
esp_err_t tlv_dispatch_command(const uint8_t* data, size_t len, httpd_req_t* req) {
    // Parse TLV, execute command, send response via Task 3's send_ws_status()
}
```

---

## 13. Success Criteria

Task 3 is complete when:
1. ✅ WebSocket endpoint `/ws` accepts connections
2. ✅ Maximum 2 concurrent clients enforced
3. ✅ Binary frames up to 4KB received correctly
4. ✅ Timeout mechanism disconnects idle clients after 5s
5. ✅ Memory allocated via `prism_pool_alloc()` and freed on disconnect
6. ✅ All 7 unit tests pass (or documented as placeholders with mock requirements)
7. ✅ Manual hardware test confirms 2-client limit and timeout
8. ✅ Heap usage remains <150KB total
9. ✅ Build completes with no warnings
10. ✅ Integration hooks ready for Task 4 TLV parser

---

## 14. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| **ESP-IDF API changes** | Low | High | Test on ESP-IDF v5.3+ LTS release |
| **Mutex deadlock** | Low | Critical | Minimize mutex hold time, audit all lock points |
| **Memory leak on error path** | Medium | High | Rigorous cleanup in all error handlers |
| **Broadcast method unclear** | Medium | Medium | Investigate `httpd_queue_work()`, document workaround |
| **Timeout not detecting idle** | Low | Medium | Test with actual WebSocket client idle scenario |
| **Task 4 interface mismatch** | Low | High | Define clear function signatures with PM review |

---

## 15. Estimated Effort

| Phase | Estimated Time | Complexity |
|-------|---------------|------------|
| **Phase 1**: Data structures | 30 minutes | Low |
| **Phase 2**: Lifecycle functions | 1 hour | Medium |
| **Phase 3**: Session management | 1.5 hours | Medium |
| **Phase 4**: Frame handling | 2 hours | High |
| **Phase 5**: Sending functions | 1 hour | Medium |
| **Phase 6**: Timeout integration | 30 minutes | Low |
| **Phase 7**: Testing | 2 hours | High |
| **Phase 8**: Documentation | 1 hour | Low |
| **Investigations** | 1 hour | Medium |
| **Total** | ~10.5 hours | **8/10** |

---

## 16. Next Steps

**Immediate Actions**:
1. **PM Review**: Present this architecture document for approval
2. **Clarify Open Questions**: Resolve ESP-IDF broadcast method (Q2)
3. **Begin Phase 1**: Implement data structures once approved
4. **Incremental Builds**: Compile after each phase to catch errors early

**Post-Task 3**:
- Hand off TLV protocol implementation to Task 4
- Integrate Task 4's `tlv_dispatch_command()` into `handle_ws_frame()`
- Full system test: WebSocket → TLV → Pattern Storage → LED Update

---

**Document Status**: Ready for PM review and approval before implementation begins.
