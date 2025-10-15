# WebSocket Protocol Specification

**Version:** 1.0
**Purpose:** Define binary protocol for PRISM K1 firmware communication
**Transport:** WebSocket Binary Frames

## Protocol Overview

Binary protocol using Type-Length-Value (TLV) encoding with CRC32 validation.

### Frame Structure
```
┌────────┬────────────┬──────────────┬────────┐
│ TYPE   │ LENGTH     │ PAYLOAD      │ CRC32  │
│ 1 byte │ 2 bytes    │ N bytes      │ 4 bytes│
└────────┴────────────┴──────────────┴────────┘
```

- **TYPE**: Message type identifier (see Message Types)
- **LENGTH**: Payload length in bytes (little-endian)
- **PAYLOAD**: Message-specific data
- **CRC32**: Checksum of TYPE+LENGTH+PAYLOAD

## Message Types

### File Transfer Messages (0x10-0x1F)

#### PUT_BEGIN (0x10)
Initiates pattern upload.

**Request Payload:**
```c
struct {
    char filename[32];    // Null-terminated filename
    uint32_t file_size;   // Total file size in bytes
    uint32_t file_crc;    // CRC32 of complete file
}
```

**Response Payload:**
```c
struct {
    uint8_t status;       // 0x00=ready, 0x01=busy, 0x02=storage_full
    uint32_t session_id;  // Transfer session ID
}
```

#### PUT_DATA (0x11)
Transfers file chunk.

**Request Payload:**
```c
struct {
    uint32_t session_id;  // From PUT_BEGIN response
    uint32_t offset;      // Byte offset in file
    uint16_t data_len;    // Length of this chunk
    uint8_t data[];       // Actual data bytes
}
```

**Response Payload:**
```c
struct {
    uint8_t status;       // 0x00=ok, 0x03=invalid_session, 0x04=bad_offset
    uint32_t bytes_recv;  // Total bytes received so far
}
```

#### PUT_END (0x12)
Completes file transfer.

**Request Payload:**
```c
struct {
    uint32_t session_id;  // Transfer session ID
}
```

**Response Payload:**
```c
struct {
    uint8_t status;       // 0x00=success, 0x05=crc_mismatch, 0x06=write_failed
    char filename[32];    // Stored filename
}
```

### Control Messages (0x20-0x2F)

#### CONTROL (0x20)
Sends control commands.

**Request Payload:**
```c
struct {
    uint8_t command;      // Command ID
    uint8_t params[];     // Command-specific parameters
}
```

**Commands:**
- `0x01`: LOAD_PATTERN `{char filename[32]}`
- `0x02`: DELETE_PATTERN `{char filename[32]}`
- `0x03`: SET_BRIGHTNESS `{uint8_t brightness}`
- `0x04`: SET_SPEED `{uint8_t speed}`
- `0x05`: PAUSE_PLAYBACK `{}`
- `0x06`: RESUME_PLAYBACK `{}`
- `0x07`: FACTORY_RESET `{uint32_t magic=0xDEADBEEF}`
- `0x08`: REBOOT `{}`

**Response Payload:**
```c
struct {
    uint8_t status;       // 0x00=success, 0x07=unknown_command
    uint8_t result[];     // Command-specific results
}
```

### Status Messages (0x30-0x3F)

#### STATUS_REQUEST (0x30)
Requests device status.

**Request Payload:** Empty

**Response Payload:**
```c
struct {
    uint32_t uptime_sec;      // Seconds since boot
    uint32_t heap_free;       // Free heap bytes
    uint32_t heap_largest;    // Largest free block
    uint8_t pattern_count;    // Stored patterns
    uint32_t storage_free;    // Free storage bytes
    uint8_t wifi_rssi;        // WiFi signal strength
    char current_pattern[32]; // Active pattern name
    uint8_t fps;             // Current FPS
    uint8_t brightness;      // Current brightness (0-255)
}
```

#### HEARTBEAT (0x31)
Keep-alive ping.

**Request Payload:**
```c
struct {
    uint32_t timestamp;   // Client timestamp
}
```

**Response Payload:**
```c
struct {
    uint32_t timestamp;   // Echo client timestamp
    uint32_t device_time; // Device uptime in ms
}
```

### Template Messages (0x40-0x4F)

#### LIST_TEMPLATES (0x40)
Lists available templates.

**Request Payload:** Empty

**Response Payload:**
```c
struct {
    uint8_t count;        // Number of templates
    struct {
        uint8_t id;       // Template ID
        char name[32];    // Template name
        uint8_t category; // 0=Ambient, 1=Energy, 2=Special
    } templates[];
}
```

#### DEPLOY_TEMPLATE (0x41)
Deploys template as active pattern.

**Request Payload:**
```c
struct {
    uint8_t template_id;  // Template to deploy
    char save_as[32];     // Optional filename (empty = temporary)
}
```

**Response Payload:**
```c
struct {
    uint8_t status;       // 0x00=success, 0x08=invalid_template
}
```

## Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0x00 | SUCCESS | Operation completed successfully |
| 0x01 | BUSY | Device busy with another operation |
| 0x02 | STORAGE_FULL | No space for new patterns |
| 0x03 | INVALID_SESSION | Session ID not found or expired |
| 0x04 | BAD_OFFSET | Data offset doesn't match expected |
| 0x05 | CRC_MISMATCH | File CRC doesn't match |
| 0x06 | WRITE_FAILED | Storage write operation failed |
| 0x07 | UNKNOWN_COMMAND | Command not recognized |
| 0x08 | INVALID_TEMPLATE | Template ID not found |
| 0x09 | MALFORMED_MESSAGE | Message structure invalid |
| 0x0A | BUFFER_OVERFLOW | Message too large |

## State Machine

### Connection States
```
DISCONNECTED -> CONNECTING -> CONNECTED -> DISCONNECTED
                     ↑             ↓
                     └─────────────┘
                       (reconnect)
```

### Transfer States
```
IDLE -> PUT_BEGIN -> RECEIVING -> PUT_END -> IDLE
            ↓            ↓           ↓
          ERROR       ERROR       ERROR
            ↓            ↓           ↓
          IDLE        IDLE        IDLE
```

## Protocol Rules

### Message Ordering
1. PUT_BEGIN must precede PUT_DATA
2. PUT_DATA chunks must arrive in order (by offset)
3. PUT_END must follow all PUT_DATA
4. CONTROL commands abort active transfers

### Timeout Behavior
- Session timeout: 30 seconds without PUT_DATA
- Heartbeat interval: 10 seconds
- Connection timeout: 60 seconds without any message

### Buffer Management
- Maximum message size: 8192 bytes
- Maximum payload: 8192 - 7 = 8185 bytes
- Recommended PUT_DATA chunk: 4096 bytes

## Implementation Examples

### Upload Pattern (JavaScript)
```javascript
async function uploadPattern(ws, filename, data) {
    // Send PUT_BEGIN
    const beginMsg = {
        type: 0x10,
        payload: {
            filename: filename,
            file_size: data.length,
            file_crc: crc32(data)
        }
    };
    const response = await sendMessage(ws, beginMsg);

    if (response.status !== 0x00) {
        throw new Error(`Upload rejected: ${response.status}`);
    }

    const sessionId = response.session_id;
    const chunkSize = 4096;

    // Send PUT_DATA chunks
    for (let offset = 0; offset < data.length; offset += chunkSize) {
        const chunk = data.slice(offset, offset + chunkSize);
        const dataMsg = {
            type: 0x11,
            payload: {
                session_id: sessionId,
                offset: offset,
                data: chunk
            }
        };
        await sendMessage(ws, dataMsg);
    }

    // Send PUT_END
    const endMsg = {
        type: 0x12,
        payload: {
            session_id: sessionId
        }
    };
    return await sendMessage(ws, endMsg);
}
```

### Handle Message (C/ESP32)
```c
void handle_websocket_message(uint8_t *data, size_t len) {
    if (len < 7) {  // Minimum: type(1) + length(2) + crc(4)
        send_error(0x09);  // MALFORMED_MESSAGE
        return;
    }

    uint8_t type = data[0];
    uint16_t length = (data[2] << 8) | data[1];  // Little-endian

    if (len != length + 7) {
        send_error(0x09);  // MALFORMED_MESSAGE
        return;
    }

    // Verify CRC
    uint32_t received_crc = *(uint32_t*)(data + len - 4);
    uint32_t calculated_crc = crc32(data, len - 4);

    if (received_crc != calculated_crc) {
        send_error(0x05);  // CRC_MISMATCH
        return;
    }

    // Dispatch by type
    switch (type) {
        case 0x10: handle_put_begin(data + 3, length); break;
        case 0x11: handle_put_data(data + 3, length); break;
        case 0x12: handle_put_end(data + 3, length); break;
        case 0x20: handle_control(data + 3, length); break;
        case 0x30: handle_status_request(); break;
        case 0x31: handle_heartbeat(data + 3, length); break;
        default: send_error(0x07); break;  // UNKNOWN_COMMAND
    }
}
```

## Performance Targets

- Message processing: <10ms
- File transfer: 500KB/s sustained
- Heartbeat latency: <50ms
- Status request: <20ms response
- Pattern switch: <100ms via CONTROL

## Security Considerations

1. **No authentication** - Device assumes local network trust
2. **CRC validation** - Detects corruption, not tampering
3. **Rate limiting** - 100 messages/second max
4. **Buffer bounds** - Strict size validation
5. **Session isolation** - One transfer at a time