# PRISM LED Controller Firmware Architecture
*Version 1.0.0 - Production Specification*

## 1.1 Hardware Constraints Table

| Resource | Total | OS Reserved | Available | Safety Margin | Usable |
|----------|-------|-------------|-----------|---------------|---------|
| RAM (DRAM) | 512KB | 90KB | 422KB | 20% (84KB) | 338KB |
| RAM (IRAM) | 128KB | 64KB | 64KB | 10% (6KB) | 58KB |
| Flash | 4MB | 2.5MB | 1.5MB | - | 1.5MB |
| CPU Core 0 | 240MHz | 40MHz (WiFi/BT) | 200MHz | 30% | 140MHz |
| CPU Core 1 | 240MHz | 20MHz (System) | 220MHz | 30% | 154MHz |
| Stack Total | - | 24KB | - | - | 24KB |
| Heap Total | - | - | 338KB | 163KB | 175KB |

### Flash Partition Layout
```
0x000000 - 0x008FFF  (36KB)  : Bootloader
0x009000 - 0x00EFFF  (24KB)  : NVS (WiFi credentials, config)
0x00F000 - 0x010FFF  (8KB)   : OTA data
0x011000 - 0x180FFF  (1.47MB): App0 (OTA partition 1)
0x181000 - 0x2F0FFF  (1.47MB): App1 (OTA partition 2)
0x2F1000 - 0x3F0FFF  (1MB)   : PrismFS (LittleFS pattern storage)
0x3F1000 - 0x3FFFFF  (60KB)  : Reserved
```

## 1.2 Memory Allocation Plan

### HEAP ALLOCATION (338KB total, 175KB usable after margin)
```
CRITICAL ALLOCATIONS (Never released):
  Network Stack       : 40KB  (WiFi buffers, TLS, TCP/IP)
  HTTP Server         : 16KB  (Request handling, URI routing)
  WebSocket Manager   : 24KB  (2x 12KB double buffer)
  Pattern Cache       : 60KB  (3x 20KB decompressed patterns)
  Effect Engine       : 30KB  (Working buffers, interpolation)
  LED Frame Buffer    : 2KB   (2x 960B double buffered)
  System Queues       : 8KB   (FreeRTOS message passing)
  Subtotal           : 180KB

DYNAMIC ALLOCATIONS (Temporary):
  Upload Buffer       : 8KB   (2x 4KB ping-pong during upload)
  JSON Parsing        : 4KB   (cJSON temporary)
  TLS Handshake      : 8KB   (temporary during connection)
  Subtotal           : 20KB

FRAGMENTATION RESERVE: 138KB (40% of total for long-term stability)
```

### IRAM ALLOCATION (64KB available, 58KB usable)
```
ISR Handlers        : 8KB   (WiFi, Timer, UART)
FastLED Assembly    : 12KB  (RMT driver, timing critical)
Critical Functions  : 8KB   (Marked with IRAM_ATTR)
WebSocket Parse     : 4KB   (Hot path optimization)
Reserved            : 26KB  (Future optimizations)
```

### STACK ALLOCATION (24KB total)
```
Main Task           : 4KB   (app_main, initialization)
HTTP Server Task    : 4KB   (esp_https_server internal)
WebSocket Task      : 4KB   (Message pump and routing)
Effect Engine Task  : 6KB   (Pattern execution, math heavy)
LED Output Task     : 3KB   (FastLED, timing critical)
Timer Task          : 2KB   (Lightweight scheduling)
Idle Task           : 1KB   (FreeRTOS minimum)
```

## 1.3 Module Architecture

### Core Module Definitions

#### MODULE: websocket_handler
**Purpose**: Bidirectional communication with desktop application
**Memory**: 24KB heap (static), 4KB stack
**CPU Budget**: 10% average, 25% peak
**Interfaces**:
```c
// Initialization
esp_err_t ws_handler_init(httpd_handle_t server);
esp_err_t ws_handler_deinit(void);

// Client Management
esp_err_t ws_client_add(int sockfd, const char* client_id);
esp_err_t ws_client_remove(int sockfd);
esp_err_t ws_broadcast(const char* json_message);

// Message Processing
esp_err_t ws_process_command(int sockfd, const cJSON* command);
esp_err_t ws_send_response(int sockfd, const char* msg_id, const cJSON* payload);
esp_err_t ws_send_event(int sockfd, const char* event_type, const cJSON* data);
```

**Dependencies**: esp_http_server, cJSON, FreeRTOS queues
**Error Modes**: Client overflow (max 4), malformed JSON, queue full

#### MODULE: pattern_storage
**Purpose**: Persistent storage of .prism pattern files
**Memory**: 8KB heap (dynamic during operations), 2KB stack
**CPU Budget**: 5% average (mostly idle), 40% during upload
**Interfaces**:
```c
// Filesystem Management
esp_err_t storage_init(void);
esp_err_t storage_format(void);
esp_err_t storage_get_info(size_t* used_bytes, size_t* free_bytes);

// Pattern Operations
esp_err_t storage_pattern_save(const char* id, const uint8_t* data, size_t len);
esp_err_t storage_pattern_load(const char* id, uint8_t** data, size_t* len);
esp_err_t storage_pattern_delete(const char* id);
esp_err_t storage_pattern_list(cJSON* json_array);

// Validation
esp_err_t storage_pattern_verify(const char* id, uint32_t expected_crc);
esp_err_t storage_rebuild_index(void);
```

**Dependencies**: esp_littlefs, cJSON
**Error Modes**: Filesystem full, corrupt index, I/O timeout

#### MODULE: led_driver
**Purpose**: Hardware abstraction for 320 RGB LED output
**Memory**: 2KB heap (frame buffers), 3KB stack
**CPU Budget**: 15% constant (60 FPS output)
**Interfaces**:
```c
// Initialization
esp_err_t led_driver_init(gpio_num_t data_pin, uint16_t num_leds);
esp_err_t led_driver_deinit(void);

// Output Control
esp_err_t led_set_frame(const rgb_t* frame, uint16_t len);
esp_err_t led_set_brightness(uint8_t brightness);  // 0-255
esp_err_t led_set_color_correction(rgb_t correction);
esp_err_t led_show(void);  // Blocking, ~5.3ms for 320 LEDs

// Utilities
esp_err_t led_fill_solid(rgb_t color);
esp_err_t led_fill_gradient(rgb_t start, rgb_t end);
esp_err_t led_apply_dithering(bool enable);
```

**Dependencies**: FastLED (or esp32-led-strip), RMT peripheral
**Error Modes**: RMT overflow, timing violation, power budget exceeded

#### MODULE: effect_engine
**Purpose**: Interpret and execute .prism pattern files
**Memory**: 30KB heap (working buffers), 6KB stack
**CPU Budget**: 30% average, 60% peak during transitions
**Interfaces**:
```c
// Engine Control
esp_err_t effect_engine_init(void);
esp_err_t effect_engine_start(void);
esp_err_t effect_engine_stop(void);

// Pattern Playback
esp_err_t effect_load_pattern(const uint8_t* pattern_data, size_t len);
esp_err_t effect_play(const char* pattern_id, uint32_t start_time_ms);
esp_err_t effect_pause(void);
esp_err_t effect_resume(void);

// Real-time Parameters
esp_err_t effect_set_param(const char* param_name, float value);
esp_err_t effect_get_param(const char* param_name, float* value);
esp_err_t effect_set_transition(transition_type_t type, uint32_t duration_ms);

// Frame Generation
esp_err_t effect_render_frame(uint32_t time_ms, rgb_t* output, uint16_t len);
```

**Dependencies**: Math library, heatshrink decompressor
**Error Modes**: Pattern decode failure, parameter out of range, CPU overrun

#### MODULE: template_loader
**Purpose**: Built-in patterns for first-run experience
**Memory**: 2KB heap (metadata only), patterns in flash
**CPU Budget**: <1% (one-time operations)
**Interfaces**:
```c
// Template Management
esp_err_t template_get_count(uint16_t* count);
esp_err_t template_get_info(uint16_t index, pattern_info_t* info);
esp_err_t template_load_to_storage(uint16_t index);
esp_err_t template_load_all(void);

// Direct Playback
esp_err_t template_play(uint16_t index);
const uint8_t* template_get_data(uint16_t index, size_t* len);
```

**Dependencies**: Pattern data embedded in flash
**Error Modes**: Invalid index, storage full

## 1.4 Critical State Machines

### WiFi Connection State Machine
```
                  ┌─────────────┐
                  │    INIT     │
                  └──────┬──────┘
                         │ wifi_init()
                         ▼
                  ┌─────────────┐
          ┌───────│ DISCONNECTED│◄──────┐
          │       └──────┬──────┘        │
          │              │ connect()     │ connection_lost
          │              ▼               │ (retry backoff:
          │       ┌─────────────┐        │  1s, 2s, 4s, 8s, 16s)
          │       │ CONNECTING  │────────┤
          │       └──────┬──────┘        │
          │              │ got_ip        │
          │              ▼               │
          │       ┌─────────────┐        │
          │       │  CONNECTED  │────────┘
          │       └──────┬──────┘
          │              │ start_services()
          │              ▼
          │       ┌─────────────┐
          │       │   READY     │
          │       └─────────────┘
          │              │
          └──────────────┘
          factory_reset()

State Behaviors:
- DISCONNECTED: Cache mode active, patterns play from storage
- CONNECTING: LED indicator pulsing blue
- CONNECTED: Start HTTP/WebSocket servers
- READY: Full functionality, accepting commands
```

### Pattern Switching State Machine
```
                  ┌─────────────┐
                  │    IDLE     │◄──────────────┐
                  └──────┬──────┘                │
                         │ play_pattern()        │
                         ▼                       │
                  ┌─────────────┐                │
                  │   LOADING   │                │ stop()
                  └──────┬──────┘                │
                         │ pattern_ready         │
                         ▼                       │
                  ┌─────────────┐                │
          ┌───────│   FADING    │                │
          │       └──────┬──────┘                │
          │              │ fade_complete         │
          │              ▼                       │
          │       ┌─────────────┐                │
          │       │   PLAYING   │────────────────┘
          │       └──────┬──────┘
          │              │ switch_pattern()
          │              │
          └──────────────┘
          (crossfade to new)

Timing Constraints:
- LOADING: Maximum 100ms (from cache) or 500ms (from storage)
- FADING: Default 500ms, configurable 0-2000ms
- PLAYING: Continuous at 60 FPS
```

### Error Recovery State Machine
```
                  ┌─────────────┐
                  │   NORMAL    │
                  └──────┬──────┘
                         │ error_detected
                         ▼
                  ┌─────────────┐
                  │  DEGRADED   │
                  └──────┬──────┘
                         │ critical_error
                         ▼
                  ┌─────────────┐
              ┌───│  RECOVERY   │───┐
              │   └─────────────┘   │
              │                     │
    recovery_failed           recovery_success
              │                     │
              ▼                     ▼
       ┌─────────────┐       ┌─────────────┐
       │  SAFE_MODE  │       │   NORMAL    │
       └─────────────┘       └─────────────┘
              │
              │ watchdog_timeout (30s)
              ▼
       ┌─────────────┐
       │   REBOOT    │
       └─────────────┘

Recovery Actions by Error Type:
- HEAP_LOW: Free caches, garbage collect
- PATTERN_CORRUPT: Load default, delete corrupt file
- THERMAL_HIGH: Reduce brightness 50%
- NETWORK_LOST: Continue in cache mode
- STORAGE_FULL: Delete oldest patterns
```

## 1.5 Failure Recovery Matrix

| Failure Mode | Detection Method | Recovery Action | Max Recovery Time | LED Indicator |
|--------------|------------------|-----------------|-------------------|---------------|
| Heap fragmentation > 40% | Runtime esp_get_free_heap_size() | Controlled restart | 3 seconds | Yellow pulse |
| Pattern file corrupted | CRC32 mismatch on load | Delete file, load default | 500ms | Red flash x2 |
| WiFi disconnected | Event handler DISCONNECTED | Exponential backoff reconnect | Indefinite | Blue pulse |
| WebSocket flood | >10 msg/sec from client | Drop messages, warn client | Immediate | - |
| Thermal > 70°C | Internal temp sensor | Brightness 50%, CPU throttle | Immediate | Orange pulse |
| Storage full | <100KB free space | Delete oldest patterns | 2 seconds | Purple pulse |
| Upload timeout | No data for 5 seconds | Abort upload, cleanup temp | Immediate | - |
| Pattern too large | Size > 256KB | Reject with error | Immediate | - |
| Invalid command | JSON parse failure | Send error response | Immediate | - |
| Watchdog timeout | Task blocked > 5 seconds | Hardware reset | 5 seconds | All LEDs off→on |
| Power brownout | Voltage < 3.0V detected | Save state, controlled shutdown | 100ms | Fade to black |
| Flash wear | Write failures | Read-only mode | Immediate | White pulse |
| Memory leak | Heap < 20KB available | Emergency restart | 3 seconds | Red pulse |
| CPU overload | Idle < 10% for 10 sec | Skip frames, reduce FPS to 30 | Immediate | - |
| Corrupt index | JSON parse fails | Rebuild from file scan | 5 seconds | - |

### Automatic Recovery Procedures

#### Heap Defragmentation Recovery
```c
void heap_recovery_procedure() {
    // 1. Stop non-critical tasks
    effect_engine_pause();

    // 2. Free all caches
    pattern_cache_clear();
    json_buffer_free();

    // 3. Force garbage collection
    heap_caps_malloc_extmem_enable(0);

    // 4. If still fragmented, controlled restart
    if (esp_get_free_heap_size() < HEAP_MIN_THRESHOLD) {
        save_state_to_nvs();
        esp_restart();
    }
}
```

#### Network Recovery Sequence
```c
typedef struct {
    uint32_t retry_count;
    uint32_t backoff_ms;
} wifi_recovery_state_t;

void wifi_recovery_procedure(wifi_recovery_state_t* state) {
    state->retry_count++;
    state->backoff_ms = MIN(1000 * (1 << state->retry_count), 16000);

    // Attempt reconnect after backoff
    vTaskDelay(state->backoff_ms / portTICK_PERIOD_MS);
    esp_wifi_connect();
}
```

#### Storage Recovery Options
```c
typedef enum {
    STORAGE_RECOVERY_DELETE_OLDEST,
    STORAGE_RECOVERY_DELETE_LARGEST,
    STORAGE_RECOVERY_DELETE_CORRUPTED,
    STORAGE_RECOVERY_FORMAT
} storage_recovery_action_t;

esp_err_t storage_recovery_procedure(storage_recovery_action_t action) {
    switch(action) {
        case STORAGE_RECOVERY_DELETE_OLDEST:
            return storage_delete_oldest_pattern();
        case STORAGE_RECOVERY_DELETE_LARGEST:
            return storage_delete_largest_pattern();
        case STORAGE_RECOVERY_DELETE_CORRUPTED:
            return storage_scan_and_clean();
        case STORAGE_RECOVERY_FORMAT:
            return storage_format_partition();
    }
}
```

## 1.6 Performance Budgets

### CPU Utilization Targets
```
Core 0 (Protocol & Storage):
  WiFi Stack        : 15% baseline
  HTTP Server       : 10% average, 30% during upload
  WebSocket Handler : 5% average, 15% peak
  Storage I/O       : 5% average, 40% during write
  Available         : 35% headroom

Core 1 (Effects & Output):
  Effect Engine     : 30% average, 60% during transitions
  LED Output        : 15% constant (DMA + RMT)
  Pattern Cache     : 5% during load
  System Tasks      : 10% (FreeRTOS, timers)
  Available         : 40% headroom
```

### Timing Guarantees
```
LED Frame Rate      : 60 FPS ± 1 FPS (16.67ms ± 0.3ms)
WebSocket Response  : < 50ms for commands
HTTP Upload         : > 100 KB/s throughput
Pattern Switch      : < 100ms to first frame
Parameter Update    : < 2 frames latency (33ms)
Boot to Ready       : < 5 seconds
Factory Reset       : < 10 seconds
```

### Memory Watermarks
```
Heap Free Minimum   : 20KB (trigger recovery below)
Stack Free Minimum  : 512 bytes per task
Largest Free Block  : 8KB (prevent severe fragmentation)
Pattern Cache Hit   : > 90% during normal operation
```

## 1.7 Module Communication

### Inter-Module Message Passing
```c
// Queue definitions
QueueHandle_t ws_to_effect_queue;   // 16 messages, 64 bytes each
QueueHandle_t effect_to_led_queue;  // 4 frames, 960 bytes each
QueueHandle_t storage_event_queue;  // 8 events, 32 bytes each

// Message structures
typedef struct {
    enum { CMD_PLAY, CMD_STOP, CMD_PARAM, CMD_TRANSITION } type;
    union {
        struct { char pattern_id[37]; uint32_t start_ms; } play;
        struct { char param[32]; float value; } param;
        struct { uint8_t type; uint32_t duration_ms; } transition;
    } data;
} effect_command_t;

typedef struct {
    rgb_t pixels[320];
    uint32_t timestamp_ms;
    uint8_t brightness;
} led_frame_t;
```

### Synchronization Primitives
```c
// Mutexes
SemaphoreHandle_t storage_mutex;     // Protect filesystem access
SemaphoreHandle_t pattern_cache_mutex;  // Protect cache updates
SemaphoreHandle_t ws_client_list_mutex; // Protect client list

// Events
EventGroupHandle_t system_events;
#define EVENT_WIFI_CONNECTED    (1 << 0)
#define EVENT_STORAGE_READY     (1 << 1)
#define EVENT_PATTERN_LOADED    (1 << 2)
#define EVENT_ERROR_OCCURRED    (1 << 3)
```

## 1.8 Build Configuration

### Required sdkconfig settings
```ini
# System
CONFIG_FREERTOS_HZ=1000
CONFIG_FREERTOS_USE_TRACE_FACILITY=y
CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y

# Memory
CONFIG_SPIRAM_SUPPORT=n  # No external RAM on base model
CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768

# Network
CONFIG_LWIP_MAX_SOCKETS=8
CONFIG_LWIP_TCP_MAXRTX=6
CONFIG_LWIP_TCP_SYNMAXRTX=3

# HTTP Server
CONFIG_HTTPD_MAX_REQ_HDR_LEN=1024
CONFIG_HTTPD_MAX_URI_LEN=512
CONFIG_HTTPD_WS_SUPPORT=y

# Optimization
CONFIG_COMPILER_OPTIMIZATION_LEVEL_RELEASE=y
CONFIG_BOOTLOADER_COMPILER_OPTIMIZATION_SIZE=y
```

---

*End of firmware_architecture.md - Version 1.0.0*