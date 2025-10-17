# PRISM K1 - Unity Test App

Minimal ESP-IDF application that runs all Unity tests defined under `../components/tests`.

## Why this app?

- Keeps the test runtime focused: boots into Unity and runs tests immediately
- Avoids starting the full PRISM main app threads during testing
- Easier to flash/monitor just for unit test cycles

## Quick Start

Using the helper script:

```bash
# Build only
firmware/tools/run_unity_tests.sh

# Flash and monitor (replace with your serial device)
firmware/tools/run_unity_tests.sh /dev/tty.usbserial-XXXX
```

Manual steps:

```bash
cd firmware/test_all_app
idf.py build
idf.py -p /dev/tty.usbserial-XXXX flash monitor
```

You should see Unity test output and a final PASS/FAIL summary.

## Requirements

- ESP-IDF environment set up (IDF_PATH, toolchain installed)
- Target: ESP32-S3 (matches main project)
- Serial permissions for your development board

## What gets included

This app sets `EXTRA_COMPONENT_DIRS ../components`, so it picks up:
- All components under `firmware/components`
- The tests component at `firmware/components/tests` (Unity test sources)

## Tips

- If you add new tests under `components/tests`, they will be picked up automatically on next build
- Use `idf.py fullclean` in this directory if you change toolchain/IDF versions

