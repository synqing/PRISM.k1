# Firmware PRs (ESP-IDF C++)

## PR #A — WebSocket + TLV base
- `/ws` endpoint (esp_http_server with WS enabled)
- `proto/tlv.h/.cpp` (frame parse, CRC-32, dispatcher)

## PR #B — LED driver (dual-edge WS281x via RMT)
- `led/led_ws281x.h/.cpp` (two RMT TX channels, 160 px each)

## PR #C — Geometry + Palette + Renderer
- `render/geometry.h/.cpp` (profile load, accessors)
- `color/palette.h/.cpp` (256xRGB8 banks)
- `render/renderer.h/.cpp` (edge physics, uniformity, gamma-last)

## PR #D — Program IR runtime
- `prog/program.h/.cpp` (load IR bytes, per-pixel eval skeleton)

## PR #E — App wiring
- `main/app.cpp` (FreeRTOS tasks: render loop, control loop)
