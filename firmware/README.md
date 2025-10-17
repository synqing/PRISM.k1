# PRISM K1 Firmware

ESP32-S3 firmware for PRISM LED controller.

## Features

- **WebSocket Communication**: Binary protocol for real-time control
- **Pattern Storage**: LittleFS with .prism format support
- **Template System**: 15 pre-built patterns for instant use
- **Hot Cache**: <100ms pattern switching
- **WiFi Management**: AP mode for setup, STA for operation
- **60-Second Setup**: Template-first experience

## Requirements

- ESP-IDF v5.x
- ESP32-S3 with 8MB flash
- Python 3.8+

## Quick Start

### 1. Install ESP-IDF

```bash
# macOS
brew install cmake ninja python3
mkdir -p ~/esp
cd ~/esp
git clone -b v5.3.0 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3
. ./export.sh
```

### 2. Build Firmware

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
```

### 3. Flash to Device

```bash
# Find your device port (usually /dev/cu.usbserial-* on macOS)
idf.py -p /dev/cu.usbserial-0001 flash monitor
```

## Project Structure

```
firmware/
├── main/               # Main application
├── components/
│   ├── core/          # System initialization, memory, errors
│   ├── network/       # WebSocket, WiFi, protocol parser
│   ├── storage/       # LittleFS, pattern format, cache
│   ├── playback/      # LED driver, effects, animation
│   └── templates/     # Built-in patterns
├── partitions.csv     # Flash partition table
└── sdkconfig.defaults # Default configuration
```

## Configuration

### Partition Table

| Partition | Size | Purpose |
|-----------|------|---------|
| nvs | 24KB | WiFi credentials, settings |
| factory | 2MB | Firmware application |
| storage | 1.5MB | Pattern storage (LittleFS) |
| coredump | 64KB | Crash dumps |

### Memory Budget

- **Heap Usage**: <150KB at runtime
- **Stack Sizes**:
  - Main: 8KB
  - Network: 4KB
  - Playback: 4KB
  - WebSocket: 6KB

## Development

### Monitor Output

```bash
idf.py monitor
```

### Clean Build

```bash
idf.py fullclean
idf.py build
```

### Component Testing

```bash
# Test individual component
cd components/network
idf.py build
```

## WebSocket Protocol

See [websocket_protocol.md](../docs/websocket_protocol.md)

## Storage Format

See [storage_layout.md](../docs/storage_layout.md)

## Templates

See [template_catalog.md](../docs/template_catalog.md)

## Profiling & Metrics (Developers)

Temporal profiling can be enabled for WAVE mode to validate cycle budget and cache behavior.

- Enable: `idf.py menuconfig → Components → PRISM Playback → Enable temporal profiling`
- Counters: toggle D$/I$ hit/miss and instruction count under the same menu
- Console logs: once per second (min/max/avg cycles, D$/I$, IPC when enabled)
- Accessors: `playback_get_wave_metrics()` returns a structured snapshot
- CLI: run `prism_metrics` to print a snapshot (enable in Kconfig)
- HTTP endpoints (when enabled):
  - `/metrics/wave` (JSON)
  - `/metrics` (Prometheus exposition)
  - `/metrics.csv` (CSV)
- Optional push: periodically POST JSON snapshots to a configured URL

See docs/phase3/wave_performance.md for full details.
