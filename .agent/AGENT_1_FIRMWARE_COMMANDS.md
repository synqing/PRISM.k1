# AGENT 1 - FIRMWARE COMMANDS MISSION BRIEF

**Agent ID:** AGENT-1-FIRMWARE
**Mission:** Implement firmware protocol commands for Studio integration
**Duration:** 2 days (9 hours total)
**Status:** 🟢 READY TO START (No blockers)
**PM Contact:** Captain (via this system)

---

## 🎯 MISSION OBJECTIVES

Implement 4 firmware protocol commands to enable PRISM Studio device integration:

1. **Task 55:** DELETE command (MSG_TYPE_DELETE 0x21) - 1h
2. **Task 56:** STATUS/HELLO command (MSG_TYPE_STATUS 0x30) - 3h
3. **Task 57:** LIST command (MSG_TYPE_LIST 0x22) - 2h
4. **Task 58:** mDNS responder (prism-k1.local) - 3h

**Critical Success Factor:** These commands BLOCK Studio Device Discovery (Task 42) and Upload/Sync (Task 50). Must be completed before Agent 2 can integrate.

---

## 📋 PRE-FLIGHT CHECKLIST

Before starting, verify:

```bash
# 1. You're in the correct directory
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1

# 2. Check current task status
task-master show 55
task-master show 56
task-master show 57
task-master show 58

# 3. Verify firmware builds
cd firmware
idf.py build

# 4. Check existing protocol implementation
cat components/network/protocol_parser.c | grep -A 10 "handle_delete\|handle_status\|handle_list"
```

**Expected Output:**
- All 4 tasks show status: `○ pending`
- Firmware builds successfully
- Handler functions exist as stubs returning `ESP_ERR_NOT_SUPPORTED`

---

## 🔧 TASK BREAKDOWN

### Task 55: DELETE Command (1 hour)

**File:** `firmware/components/network/protocol_parser.c:658`

**Current State:**
```c
static esp_err_t handle_delete(const tlv_frame_t* frame, int client_fd)
{
    ESP_LOGW(TAG, "handle_delete: NOT YET IMPLEMENTED (Phase 3)");
    return ESP_ERR_NOT_SUPPORTED;
}
```

**Requirements:**
1. Extract filename from TLV payload
2. Sanitize filename (strip `.prism`, reject path traversal)
3. Call `storage_pattern_delete()` from `pattern_storage_crud.c:81`
4. Build TLV response:
   - Success: MSG_TYPE_STATUS with `{0x00, filename}`
   - Error: MSG_TYPE_ERROR with `{error_code, reason}`
5. Send response via WebSocket (use `httpd_ws_send_frame_async`)

**Implementation Steps:**
```bash
# 1. Start the task
task-master set-status --id=55 --status=in-progress

# 2. Read existing storage API
cat firmware/components/storage/pattern_storage_crud.c | grep -A 20 "storage_pattern_delete"

# 3. Implement handler (see detailed spec below)

# 4. Test
cd firmware/components/network/test
# Add test case to test_protocol_parser.c

# 5. Complete
task-master set-status --id=55 --status=done
```

**Detailed Implementation Spec:**
```c
static esp_err_t handle_delete(const tlv_frame_t* frame, int client_fd)
{
    // 1. Validate payload
    if (frame->payload_len == 0 || frame->payload_len > PATTERN_MAX_FILENAME) {
        return send_error_response(client_fd, ERR_INVALID_FRAME, "Empty or oversized filename");
    }

    // 2. Extract and sanitize filename
    char filename[PATTERN_MAX_FILENAME + 1];
    memcpy(filename, frame->payload, frame->payload_len);
    filename[frame->payload_len] = '\0';

    // Strip .prism extension if present
    char* ext = strstr(filename, ".prism");
    if (ext) *ext = '\0';

    // Reject path traversal
    if (strstr(filename, "..") || strstr(filename, "/")) {
        return send_error_response(client_fd, ERR_INVALID_FRAME, "Invalid filename");
    }

    // 3. Call storage delete
    esp_err_t ret = storage_pattern_delete(filename);

    // 4. Build and send response
    if (ret == ESP_OK) {
        uint8_t response[256];
        response[0] = 0x00; // Success code
        memcpy(response + 1, filename, strlen(filename));
        return send_tlv_response(client_fd, MSG_TYPE_STATUS, response, strlen(filename) + 1);
    } else if (ret == ESP_ERR_NOT_FOUND) {
        return send_error_response(client_fd, ERR_NOT_FOUND, "Pattern not found");
    } else {
        return send_error_response(client_fd, ERR_STORAGE_FULL, "Delete failed");
    }
}
```

**Helper Functions Needed:**
```c
// Add to protocol_parser.c
static esp_err_t send_tlv_response(int client_fd, uint8_t msg_type, const uint8_t* payload, size_t len);
static esp_err_t send_error_response(int client_fd, uint8_t error_code, const char* message);
```

**Test Case:**
```c
TEST_CASE("DELETE removes pattern file", "[protocol]")
{
    // Create test pattern
    storage_pattern_save("test_pattern", test_data, sizeof(test_data));

    // Build DELETE TLV frame
    uint8_t frame[128];
    build_tlv_frame(frame, MSG_TYPE_DELETE, (uint8_t*)"test_pattern", 12);

    // Dispatch
    esp_err_t ret = protocol_dispatch_command(frame, sizeof(frame), mock_client_fd);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify file gone
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, storage_pattern_load("test_pattern", NULL, 0));
}
```

---

### Task 56: STATUS/HELLO Command (3 hours)

**File:** `firmware/components/network/protocol_parser.c` (new handler)

**Requirements:**
1. Return device info TLV payload:
   - Firmware version string (from `PROJECT_VER`)
   - LED count (320)
   - Storage available (query LittleFS)
   - Max chunk size (4089 bytes from ADR-002)
2. Used by Studio for device discovery validation

**Payload Format (TLV within TLV):**
```
[STATUS Response]
Type: MSG_TYPE_STATUS (0x30)
Payload:
  [0-3]:   Version string length (uint32_t)
  [4-N]:   Version string (e.g., "v1.1.0")
  [N+1-N+4]: LED count (uint16_t = 320)
  [N+5-N+8]: Storage available (uint32_t bytes)
  [N+9-N+12]: Max chunk size (uint16_t = 4089)
```

**Implementation Steps:**
```bash
# 1. Start task
task-master set-status --id=56 --status=in-progress

# 2. Check version macro
grep PROJECT_VER firmware/CMakeLists.txt

# 3. Implement handler
```

**Detailed Implementation:**
```c
static esp_err_t handle_status(const tlv_frame_t* frame, int client_fd)
{
    uint8_t response[256];
    size_t offset = 0;

    // 1. Firmware version
    const char* version = PROJECT_VER;  // From CMakeLists.txt
    uint32_t ver_len = strlen(version);
    memcpy(response + offset, &ver_len, 4);
    offset += 4;
    memcpy(response + offset, version, ver_len);
    offset += ver_len;

    // 2. LED count (320 total: 2 channels × 160)
    uint16_t led_count = 320;
    memcpy(response + offset, &led_count, 2);
    offset += 2;

    // 3. Storage available (query LittleFS)
    size_t total, used;
    esp_littlefs_info("/littlefs", &total, &used);
    uint32_t available = total - used;
    memcpy(response + offset, &available, 4);
    offset += 4;

    // 4. Max chunk size (from ADR-002: 4096 - 7 TLV overhead)
    uint16_t max_chunk = 4089;
    memcpy(response + offset, &max_chunk, 2);
    offset += 2;

    return send_tlv_response(client_fd, MSG_TYPE_STATUS, response, offset);
}
```

**Integration Point:**
```c
// In protocol_dispatch_command() switch statement
case MSG_TYPE_STATUS:
    ret = handle_status(&frame, client_fd);
    break;
```

**Test Case:**
```c
TEST_CASE("STATUS returns device info", "[protocol]")
{
    uint8_t frame[16];
    build_tlv_frame(frame, MSG_TYPE_STATUS, NULL, 0);

    esp_err_t ret = protocol_dispatch_command(frame, sizeof(frame), mock_client_fd);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify response contains version, 320 LEDs, etc.
    // (Mock WebSocket would capture sent data)
}
```

---

### Task 57: LIST Command (2 hours)

**File:** `firmware/components/network/protocol_parser.c:663`

**Current State:**
```c
static esp_err_t handle_list(const tlv_frame_t* frame, int client_fd)
{
    ESP_LOGW(TAG, "handle_list: NOT YET IMPLEMENTED (Phase 3)");
    return ESP_ERR_NOT_SUPPORTED;
}
```

**Requirements:**
1. Enumerate `/littlefs/patterns/*.prism` files
2. Return TLV array with:
   - Filename (string)
   - File size (uint32_t)
   - Modification timestamp (uint32_t)
3. Used by Studio pattern library UI

**Payload Format:**
```
[LIST Response]
Type: MSG_TYPE_STATUS (0x30)
Payload:
  [0-1]: Pattern count (uint16_t)
  For each pattern:
    [0-1]:   Filename length (uint16_t)
    [2-N]:   Filename string
    [N+1-N+4]: File size (uint32_t)
    [N+5-N+8]: Timestamp (uint32_t)
```

**Implementation:**
```c
static esp_err_t handle_list(const tlv_frame_t* frame, int client_fd)
{
    DIR* dir = opendir("/littlefs/patterns");
    if (!dir) {
        return send_error_response(client_fd, ERR_STORAGE_FULL, "Cannot open patterns dir");
    }

    uint8_t response[4096];  // Max frame size from ADR-002
    size_t offset = 2;  // Reserve space for count
    uint16_t count = 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip non-.prism files
        if (!strstr(entry->d_name, ".prism")) continue;

        // Get file stats
        char filepath[256];
        snprintf(filepath, sizeof(filepath), "/littlefs/patterns/%s", entry->d_name);
        struct stat st;
        if (stat(filepath, &st) != 0) continue;

        // Check if response buffer has space
        size_t needed = 2 + strlen(entry->d_name) + 4 + 4;
        if (offset + needed > sizeof(response)) {
            ESP_LOGW(TAG, "LIST response truncated (too many patterns)");
            break;
        }

        // Write filename length + name
        uint16_t name_len = strlen(entry->d_name);
        memcpy(response + offset, &name_len, 2);
        offset += 2;
        memcpy(response + offset, entry->d_name, name_len);
        offset += name_len;

        // Write file size
        uint32_t size = st.st_size;
        memcpy(response + offset, &size, 4);
        offset += 4;

        // Write timestamp
        uint32_t mtime = st.st_mtime;
        memcpy(response + offset, &mtime, 4);
        offset += 4;

        count++;
    }

    closedir(dir);

    // Write count at start
    memcpy(response, &count, 2);

    return send_tlv_response(client_fd, MSG_TYPE_STATUS, response, offset);
}
```

**Integration:** Add to `protocol_dispatch_command()` switch.

---

### Task 58: mDNS Responder (3 hours)

**File:** `firmware/components/network/network_manager.c`

**Requirements:**
1. Advertise device as `prism-k1.local` on network
2. Service type: `_prism._tcp`
3. TXT records: `version=v1.1`, `led_count=320`, `capabilities=upload,control`
4. Used by Studio for zero-config device discovery

**Implementation Steps:**
```bash
# 1. Start task
task-master set-status --id=58 --status=in-progress

# 2. Check if mdns component available
grep mdns firmware/main/CMakeLists.txt
# If not: add "espressif__mdns" to dependencies

# 3. Implement in network_manager.c
```

**Detailed Implementation:**
```c
// In network_manager.c

#include "mdns.h"

static void init_mdns(void)
{
    // Initialize mDNS
    ESP_ERROR_CHECK(mdns_init());

    // Set hostname
    ESP_ERROR_CHECK(mdns_hostname_set("prism-k1"));
    ESP_ERROR_CHECK(mdns_instance_name_set("PRISM K1 LED Controller"));

    // Add service
    ESP_ERROR_CHECK(mdns_service_add(
        "PRISM K1",              // Instance name
        "_prism",                // Service type
        "_tcp",                  // Protocol
        80,                      // Port (WebSocket port)
        NULL,                    // TXT records (added below)
        0                        // Number of TXT records
    ));

    // Add TXT records
    mdns_service_txt_item_set("_prism", "_tcp", "version", PROJECT_VER);
    mdns_service_txt_item_set("_prism", "_tcp", "led_count", "320");
    mdns_service_txt_item_set("_prism", "_tcp", "capabilities", "upload,control,list,delete");

    ESP_LOGI("mdns", "mDNS responder started: prism-k1.local");
}

// Call from network_manager_init()
void network_manager_init(void)
{
    // ... existing WiFi init ...

    // Start mDNS after WiFi connected
    init_mdns();
}
```

**Dependencies:**
Add to `firmware/main/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS ""
    REQUIRES
        network
        storage
        playback
        espressif__mdns  # <-- Add this
)
```

**Test:** Use Bonjour browser or:
```bash
# On macOS:
dns-sd -B _prism._tcp

# Should show:
# prism-k1._prism._tcp.local.
```

---

## 🧪 TESTING STRATEGY

### Unit Tests

Create/update: `firmware/components/network/test/test_protocol_parser.c`

```c
TEST_CASE("DELETE command removes pattern", "[protocol]") { /* ... */ }
TEST_CASE("STATUS returns device info", "[protocol]") { /* ... */ }
TEST_CASE("LIST enumerates patterns", "[protocol]") { /* ... */ }
```

### Integration Tests

1. **Manual WebSocket Test:**
```bash
# Terminal 1: Flash firmware
cd firmware
idf.py flash monitor

# Terminal 2: Connect with wscat
npm install -g wscat
wscat -c ws://prism-k1.local:80/ws --binary

# Send DELETE TLV (hex):
> 21 00 0D test_pattern 12345678  # TYPE=0x21, LEN=0x000D, payload, CRC

# Send STATUS:
> 30 00 00 12345678

# Send LIST:
> 22 00 00 12345678
```

2. **mDNS Discovery Test:**
```bash
# On same network:
ping prism-k1.local
# Should resolve to ESP32 IP
```

### Build Verification

```bash
cd firmware
idf.py clean
idf.py build

# Should compile without errors
# Check size:
# prism-k1.bin should be < 512KB (app0 partition size)
```

---

## 📚 REFERENCE DOCUMENTATION

### Key Files to Read:

1. **Protocol Spec:** `firmware/components/network/include/protocol_parser.h`
   - TLV frame format
   - Message types
   - Error codes

2. **Storage API:** `firmware/components/storage/pattern_storage_crud.c`
   - `storage_pattern_save()`
   - `storage_pattern_load()`
   - `storage_pattern_delete()`

3. **CANON:** `.taskmaster/CANON.md`
   - ADR-002: WebSocket buffer (4096 bytes)
   - ADR-004: Pattern size (256KB)
   - ADR-005: Storage mount (`/littlefs`)

4. **WebSocket Integration:** `firmware/components/network/websocket_server.c`
   - How to send frames via `httpd_ws_send_frame_async()`

### ESP-IDF Documentation:

- mDNS: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/protocols/mdns.html
- LittleFS: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/storage/littlefs.html
- WebSocket: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/protocols/esp_http_server.html

---

## 🚨 CRITICAL WARNINGS

1. **Never exceed 4096 byte WebSocket frames** (ADR-002)
2. **Always validate filenames** (prevent path traversal attacks)
3. **Use TLV CRC for all responses** (`esp_rom_crc32_le`)
4. **Test on actual hardware** if available (network I/O behaves differently than host)
5. **Update task status** after each completion

---

## 📊 SUCCESS CRITERIA

- [ ] All 4 tasks marked DONE in TaskMaster
- [ ] Firmware builds without errors
- [ ] Unit tests pass
- [ ] Manual WebSocket tests succeed
- [ ] mDNS resolution works (`ping prism-k1.local`)
- [ ] No memory leaks (run soak test if time permits)

---

## 🔗 HANDOFF TO AGENT 2

After completing Tasks 55-58, notify Captain and handoff:

```bash
# Mark ready for integration
task-master update-task --id=42 --prompt="Firmware commands (55-58) COMPLETE. Device Discovery ready for implementation. mDNS: prism-k1.local, STATUS/LIST/DELETE handlers functional."

# Notify
echo "AGENT 1 COMPLETE: Firmware commands shipped. Agent 2 cleared for Studio Device Discovery (Task 42) and Upload/Sync (Task 50)."
```

**Expected Integration:**
- Agent 2 will use mDNS to discover device
- STATUS command validates device capabilities
- LIST/DELETE used for pattern management UI
- Device Discovery (Task 42) depends on your work

---

## 💪 YOU GOT THIS, AGENT 1!

Clear objectives, detailed specs, no ambiguity. Execute with precision. The Studio team is counting on you.

**Questions?** Check CANON first, then ask Captain.

🫡 **Good hunting!**
