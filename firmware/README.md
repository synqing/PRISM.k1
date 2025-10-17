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

### 4. Optional: 4 MB Flash (Single Factory) for Bring-up

For boards with 4 MB flash, use the provided single-factory partition table (no OTA) to maximize app space during bring-up:

```bash
idf.py set-target esp32s3
idf.py -T partitions-4mb.csv build
idf.py -p /dev/cu.usbserial-0001 flash monitor
```

You can also set this in menuconfig: Partition Table → Custom, and point to `partitions-4mb.csv`.

### UART Test Mode (No Wi‑Fi Required)

A UART test mode mirrors upload/control flows without sockets. It feeds the same protocol parser used by WS/TLV.

- Enable: `idf.py menuconfig → PRISM Network/Test → Enable UART test mode` (CONFIG_PRISM_UART_TEST)
- UART: 115200‑8N1 on UART0 (USB serial)
- Commands (one per line):
  - `STATUS`
  - `PLAY <name>` / `STOP`
  - `B <target_u8> <ms_u16>` (brightness ramp)
  - `G <gamma_x100_u16> <ms_u16>` (gamma ramp)
  - `BEGIN <name> <size_u32> <crc32_hex>`
  - `DATA <offset_u32> <base64>`
  - `END`

Host CLI (requires `pyserial`):

```bash
python3 tools/serial/prism_serial_cli.py --port /dev/tty.usbserial-0001 status
python3 tools/serial/prism_serial_cli.py --port /dev/tty.usbserial-0001 upload /path/to/baked.prism
python3 tools/serial/prism_serial_cli.py --port /dev/tty.usbserial-0001 play baked
```

Uploads enforce the 256 KB pattern size and CRC32 parity (empty PUT_END semantics per SoT).
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
