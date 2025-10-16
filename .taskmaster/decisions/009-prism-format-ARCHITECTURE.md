# ADR-009 Architecture Diagram: PRISM Effect System

## System Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          PRISM.K1 Effect System                              │
│                     (ESP32-S3 @ 240MHz, 512KB RAM, 8MB Flash)               │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 1: Network Layer (WebSocket Binary Protocol)                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  WebSocket Client → [TLV Frame] → protocol_parser.c                         │
│                                                                              │
│  PUT_BEGIN (0x10)  →  Allocate buffer, validate metadata                    │
│  PUT_DATA (0x11)   →  Stream chunks, accumulate CRC32                       │
│  PUT_END (0x12)    →  Validate CRC32 → Write to storage                     │
│  CONTROL (0x20)    →  PLAY/STOP/PAUSE commands                              │
│                                                                              │
└────────────────────────────┬────────────────────────────────────────────────┘
                             ↓
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 2: Storage Layer (LittleFS on Flash)                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  /littlefs/patterns/                                                         │
│    ├── fire_warm.prism       (96 bytes)                                     │
│    ├── ocean_wave.prism      (128 bytes)                                    │
│    ├── rainbow_pulse.prism   (64 bytes)                                     │
│    └── ... (15+ patterns, ~100KB total)                                     │
│                                                                              │
│  Storage API: template_storage_write(), template_storage_read()             │
│  Format: Binary .prism files (64-256KB max per ADR-004)                     │
│                                                                              │
└────────────────────────────┬────────────────────────────────────────────────┘
                             ↓ CONTROL PLAY command
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 3: Pattern Management (prism_executor.c)                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌────────────────────────────────────────────────────────────┐             │
│  │ prism_executor_state_t (singleton)                         │             │
│  ├────────────────────────────────────────────────────────────┤             │
│  │ current_pattern: prism_pattern_t (128 bytes)               │             │
│  │   ├── header (64 bytes)                                    │             │
│  │   │   ├── magic, version, effect_id                        │             │
│  │   │   ├── brightness, speed, fade                          │             │
│  │   │   ├── params[12] (effect-specific)                     │             │
│  │   │   └── header_crc32, data_crc32                         │             │
│  │   └── palette[16] (64 bytes, RGB24)                        │             │
│  │                                                             │             │
│  │ effect: prism_effect_descriptor_t*                         │             │
│  │   ├── effect_id, name, render function pointer            │             │
│  │   └── param_count, estimated_us                            │             │
│  │                                                             │             │
│  │ frame_counter: uint32_t (animation timing)                 │             │
│  │ is_playing: bool                                            │             │
│  └────────────────────────────────────────────────────────────┘             │
│                                                                              │
│  Operations:                                                                 │
│    prism_executor_load_pattern(name) → Read → Parse → Validate → Lookup    │
│    prism_executor_play() → Start rendering loop                             │
│    prism_executor_render_frame() → Call effect render function              │
│                                                                              │
└────────────────────────────┬────────────────────────────────────────────────┘
                             ↓ 120 FPS render loop
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 4: Effect Library (prism_effects_*.c)                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌──────────────────────────────────────────────────────────┐               │
│  │ Effect Registry (prism_effect_registry[])                │               │
│  ├──────────────────────────────────────────────────────────┤               │
│  │ 0x01  Solid Color    →  prism_effect_solid_render()     │ ~50μs         │
│  │ 0x02  Gradient       →  prism_effect_gradient_render()  │ ~300μs        │
│  │ 0x03  Breathe        →  prism_effect_breathe_render()   │ ~200μs        │
│  │ 0x04  Rainbow        →  prism_effect_rainbow_render()   │ ~600μs        │
│  │ 0x05  Twinkle        →  prism_effect_twinkle_render()   │ ~800μs        │
│  │ 0x10  Wave           →  prism_effect_wave_render()      │ ~800μs        │
│  │ 0x11  Fire           →  prism_effect_fire_render()      │ ~1200μs       │
│  │ 0x12  Plasma         →  prism_effect_plasma_render()    │ ~1500μs       │
│  │ 0x13  Sparkle        →  prism_effect_sparkle_render()   │ ~700μs        │
│  │ 0x14  Meteor         →  prism_effect_meteor_render()    │ ~900μs        │
│  │ 0x20  Strobe         →  prism_effect_strobe_render()    │ ~100μs        │
│  │ 0x21  Scanner        →  prism_effect_scanner_render()   │ ~600μs        │
│  │ 0x22  Color Cycle    →  prism_effect_colorcycle_render()│ ~400μs        │
│  │ 0x23  Theater Chase  →  prism_effect_theater_render()   │ ~300μs        │
│  │ 0x24  Lightning      →  prism_effect_lightning_render() │ ~500μs        │
│  └──────────────────────────────────────────────────────────┘               │
│                                                                              │
│  Render Function Signature:                                                 │
│    esp_err_t render(pattern, frame_counter, frame_ch1, frame_ch2)           │
│                                                                              │
│  Constraints:                                                                │
│    - MUST complete within 3.38ms (CPU budget)                               │
│    - MUST NOT allocate heap memory                                          │
│    - Generate 960 bytes GRB24 data (2×480 bytes)                            │
│                                                                              │
└────────────────────────────┬────────────────────────────────────────────────┘
                             ↓ Uses math utilities
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 5: Math Utilities (prism_math.c)                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  FastLED-inspired 8-bit integer math (no floating point)                    │
│                                                                              │
│  sin8(theta)            →  Fast sine lookup table (256 bytes)               │
│  scale8(i, scale)       →  Fast 8-bit scaling: (i * scale) / 256            │
│  scale8_video(i, scale) →  Video scaling: (i * scale) / 255                 │
│  blend8(a, b, amount)   →  Interpolate: a*(1-amount) + b*amount             │
│  dim8_raw(i)            →  Dim brightness: i * (256-amount) / 256           │
│  brighten8_raw(i)       →  Brighten: i * (256+amount) / 256                 │
│  color_from_palette()   →  Interpolated palette lookup                      │
│  hsv2rgb_rainbow()      →  HSV to RGB conversion                            │
│  random8()              →  Fast PRNG for effects                            │
│                                                                              │
│  Memory: ~2KB code + 256 bytes lookup tables                                │
│                                                                              │
└────────────────────────────┬────────────────────────────────────────────────┘
                             ↓ Generates GRB24 frames
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 6: Playback Engine (led_playback.c)                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  playback_task() [FreeRTOS Task, Priority 10, Core 0]                       │
│                                                                              │
│  while (running) {                                                           │
│    // Render frame using pattern executor                                   │
│    prism_executor_render_frame(frame_ch1, frame_ch2);  ← 3.38ms budget      │
│                                                                              │
│    // Submit to LED driver                                                  │
│    led_driver_submit_frames(frame_ch1, frame_ch2);                          │
│                                                                              │
│    // Wait for next frame (8.33ms @ 120 FPS)                                │
│    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(LED_FRAME_TIME_MS));           │
│  }                                                                           │
│                                                                              │
│  Frame buffers: 2×480 bytes (static allocation)                             │
│                                                                              │
└────────────────────────────┬────────────────────────────────────────────────┘
                             ↓ Submit GRB24 frames
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 7: LED Driver (led_driver.c)                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  Dual-channel RMT peripheral (hardware WS2812B encoding)                    │
│                                                                              │
│  ┌──────────────────────────────────────────────────────────┐               │
│  │ Channel 1 (GPIO 9)      Channel 2 (GPIO 10)              │               │
│  ├──────────────────────────────────────────────────────────┤               │
│  │ 160 LEDs (480 bytes)    160 LEDs (480 bytes)             │               │
│  │ Front buffer ←─────┐    Front buffer ←─────┐             │               │
│  │ Back buffer        │    Back buffer        │             │               │
│  └────────────────────┼────────────────────────┼─────────────┘               │
│                       │                        │                             │
│                       │ Double buffering       │                             │
│                       │ (swap on frame ready)  │                             │
│                       └────────────────────────┘                             │
│                                                                              │
│  RMT Encoding: GRB24 → WS2812B timing (10MHz clock)                         │
│    Bit 0: 0.4μs high, 0.6μs low                                             │
│    Bit 1: 0.7μs high, 0.6μs low                                             │
│    Reset: 50μs low                                                           │
│                                                                              │
│  Transmission time: 4.8ms (both channels in parallel)                       │
│                                                                              │
└────────────────────────────┬────────────────────────────────────────────────┘
                             ↓ Hardware output
┌─────────────────────────────────────────────────────────────────────────────┐
│ Hardware: 2×160 WS2812B LEDs (320 total)                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  GPIO 9  ───────→  [LED 0] [LED 1] ... [LED 159]  (Channel 1)               │
│  GPIO 10 ───────→  [LED 0] [LED 1] ... [LED 159]  (Channel 2)               │
│                                                                              │
│  Update rate: 120 FPS (8.33ms frame period)                                 │
│  Color depth: 24-bit (8-bit per channel, GRB order)                         │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Component Dependency Graph

```
┌──────────────────┐
│ protocol_parser  │ ← WebSocket TLV protocol
└────────┬─────────┘
         │ uses
         ↓
┌──────────────────┐     ┌──────────────────┐
│ prism_executor   │────→│ prism_parser     │ ← Validate .prism format
└────────┬─────────┘     └──────────────────┘
         │ uses
         ↓
┌──────────────────┐     ┌──────────────────┐
│ prism_effects    │────→│ prism_math       │ ← Math utilities
└────────┬─────────┘     └──────────────────┘
         │ generates
         ↓
┌──────────────────┐
│ led_playback     │ ← Render loop (120 FPS)
└────────┬─────────┘
         │ submits
         ↓
┌──────────────────┐
│ led_driver       │ ← RMT hardware
└──────────────────┘
```

## Memory Layout (Static Allocation)

```
┌─────────────────────────────────────────────────────────────┐
│ ESP32-S3 Internal SRAM (~400KB available)                   │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ Pattern Executor State                                  │  │
│ │   current_pattern: 128 bytes                            │  │
│ │   effect descriptor pointer: 4 bytes                    │  │
│ │   frame_counter: 4 bytes                                │  │
│ │   mutex: 80 bytes                                       │  │
│ │ Total: ~220 bytes                                       │  │
│ └────────────────────────────────────────────────────────┘  │
│                                                              │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ Effect Registry (const, in flash)                       │  │
│ │   15 descriptors × 24 bytes = 360 bytes                 │  │
│ └────────────────────────────────────────────────────────┘  │
│                                                              │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ Math Lookup Tables (const, in flash)                    │  │
│ │   sin8 table: 256 bytes                                 │  │
│ └────────────────────────────────────────────────────────┘  │
│                                                              │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ Playback Task Stack                                     │  │
│ │   Frame buffers: 2×480 bytes = 960 bytes (static)      │  │
│ │   Stack: 4KB                                            │  │
│ │ Total: ~5KB                                             │  │
│ └────────────────────────────────────────────────────────┘  │
│                                                              │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ LED Driver State                                        │  │
│ │   Double buffers: 4×480 bytes = 1920 bytes (static)    │  │
│ │   State: ~200 bytes                                     │  │
│ │ Total: ~2KB                                             │  │
│ └────────────────────────────────────────────────────────┘  │
│                                                              │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ Effect State (per-effect persistent data)               │  │
│ │   Fire heat map: 160 bytes                              │  │
│ │   Plasma phase: ~64 bytes                               │  │
│ │   Other: ~200 bytes                                     │  │
│ │ Total: ~500 bytes                                       │  │
│ └────────────────────────────────────────────────────────┘  │
│                                                              │
│ Total PRISM system footprint: ~8KB SRAM + 4KB code         │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Timing Diagram (Single Frame @ 120 FPS)

```
Frame Period: 8.33ms (120 FPS)
├────────────────────────────────────────────────────────────┤

0ms                                                         8.33ms
│                                                              │
├──────────┬────────────────────────┬───────────────────────┬─┤
│          │                        │                       │ │
│  Wait    │   Effect Render        │   LED Transmission    │ │
│ (idle)   │  (CPU computation)     │   (RMT hardware)      │ │
│          │                        │                       │ │
│  <var>   │    0.05-1.5ms          │      4.8ms            │ │
│          │   (budget: 3.38ms)     │    (fixed, async)     │ │
│          │                        │                       │ │
└──────────┴────────────────────────┴───────────────────────┴─┘

Breakdown:
  - Effect render: 0.05ms (Solid) to 1.5ms (Plasma) - MUST stay < 3.38ms
  - LED transmission: 4.8ms (both channels in parallel, hardware)
  - Remaining time: Idle / other tasks

CPU budget: 3.38ms = 40.56% of frame time
Hardware transmission: 4.8ms = 57.6% of frame time (no CPU usage)
Total frame time: 8.33ms
```

## File Structure

```
firmware/components/prism/
├── CMakeLists.txt                      # Build configuration
│
├── include/                            # Public headers
│   ├── prism_pattern.h                 # Format structures (128 LOC)
│   ├── prism_parser.h                  # Parser API (80 LOC)
│   ├── prism_executor.h                # Executor API (100 LOC)
│   ├── prism_effects.h                 # Effect registry (120 LOC)
│   └── prism_math.h                    # Math utilities (80 LOC)
│
├── prism_parser.c                      # Format parser (600 LOC)
│   ├── prism_parser_validate()         # Validate header + CRC32
│   ├── prism_parser_load_palette()     # Load custom palette
│   └── prism_parser_check_version()    # Version compatibility
│
├── prism_executor.c                    # Pattern executor (400 LOC)
│   ├── prism_executor_init()           # Initialize executor
│   ├── prism_executor_load_pattern()   # Load from storage
│   ├── prism_executor_play()           # Start playback
│   ├── prism_executor_stop()           # Stop playback
│   └── prism_executor_render_frame()   # Render next frame
│
├── prism_math.c                        # Math utilities (500 LOC)
│   ├── sin8(), cos8()                  # Fast trig (lookup table)
│   ├── scale8(), scale8_video()        # Fast scaling
│   ├── blend8(), lerp8()               # Color blending
│   ├── color_from_palette()            # Palette interpolation
│   ├── hsv2rgb_rainbow()               # HSV conversion
│   └── random8()                       # PRNG for effects
│
├── prism_effects_ambient.c             # Ambient effects (400 LOC)
│   ├── prism_effect_solid_render()     # 0x01 Solid Color
│   ├── prism_effect_gradient_render()  # 0x02 Gradient
│   ├── prism_effect_breathe_render()   # 0x03 Breathe
│   ├── prism_effect_rainbow_render()   # 0x04 Rainbow
│   └── prism_effect_twinkle_render()   # 0x05 Twinkle
│
├── prism_effects_energy.c              # Energy effects (600 LOC)
│   ├── prism_effect_wave_render()      # 0x10 Wave
│   ├── prism_effect_fire_render()      # 0x11 Fire
│   ├── prism_effect_plasma_render()    # 0x12 Plasma
│   ├── prism_effect_sparkle_render()   # 0x13 Sparkle
│   └── prism_effect_meteor_render()    # 0x14 Meteor
│
├── prism_effects_special.c             # Special effects (500 LOC)
│   ├── prism_effect_strobe_render()    # 0x20 Strobe
│   ├── prism_effect_scanner_render()   # 0x21 Scanner
│   ├── prism_effect_colorcycle_render()# 0x22 Color Cycle
│   ├── prism_effect_theater_render()   # 0x23 Theater Chase
│   └── prism_effect_lightning_render() # 0x24 Lightning
│
└── test/                               # Unit tests
    ├── test_prism_parser.c             # Parser tests
    ├── test_prism_executor.c           # Executor tests
    └── test_prism_effects.c            # Effect render tests

Total: ~3,400 LOC (lines of code)
```

## Performance Characteristics

### CPU Usage per Frame

```
Effect Category    │ Min Time │ Max Time │ Avg Time │ CPU %  │
───────────────────┼──────────┼──────────┼──────────┼────────┤
Simple Effects     │   50μs   │  200μs   │  133μs   │  1-6%  │
Medium Effects     │  300μs   │  900μs   │  600μs   │  9-27% │
Complex Effects    │ 1200μs   │ 1500μs   │ 1350μs   │ 36-44% │
───────────────────┴──────────┴──────────┴──────────┴────────┘

CPU Budget: 3380μs (3.38ms)
All effects stay within budget with 2-3× safety margin
```

### Memory Usage

```
Component              │ Static RAM │ Heap (max) │ Flash  │
───────────────────────┼────────────┼────────────┼────────┤
Pattern Executor       │    220 B   │     0 B    │  2 KB  │
Effect Registry        │      0 B   │     0 B    │ 360 B  │
Math Utilities         │      0 B   │     0 B    │  2 KB  │
Effect State           │    500 B   │     0 B    │    -   │
Playback Buffers       │    960 B   │     0 B    │    -   │
LED Driver Buffers     │   1920 B   │     0 B    │    -   │
───────────────────────┼────────────┼────────────┼────────┤
Total                  │   ~4 KB    │     0 B    │ ~20 KB │
───────────────────────┴────────────┴────────────┴────────┘

Note: Pattern upload allocates heap temporarily (up to 256KB)
```

## Integration Points

### 1. Upload Flow (protocol_parser.c → storage)

```c
// In handle_put_end() after CRC validation:
esp_err_t ret = template_storage_write(
    g_upload_session.filename,
    g_upload_session.upload_buffer,
    g_upload_session.expected_size
);

// ADD: Optional validation of .prism format
ret = prism_parser_validate(
    g_upload_session.upload_buffer,
    g_upload_session.expected_size,
    NULL  // Don't need to load full pattern yet
);
```

### 2. Playback Control (protocol_parser.c → executor)

```c
// In handle_control() CONTROL_CMD_PLAY:
// REPLACE:
extern esp_err_t playback_play_builtin(uint16_t effect_id, ...);
esp_err_t ret = playback_play_builtin(0x0040, NULL, 0);

// WITH:
extern esp_err_t prism_executor_load_pattern(const char *pattern_name);
extern esp_err_t prism_executor_play(void);

esp_err_t ret = prism_executor_load_pattern(pattern_name);
if (ret == ESP_OK) {
    ret = prism_executor_play();
}
```

### 3. Render Loop (led_playback.c → executor)

```c
// In playback_task():
// REPLACE:
switch (s_pb.effect_id) {
    case EFFECT_WAVE_SINGLE: { /* ... */ }
    case EFFECT_PALETTE_CYCLE: { /* ... */ }
}

// WITH:
esp_err_t ret = prism_executor_render_frame(frame_ch1, frame_ch2);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Frame render failed");
}
```

## Testing Strategy

### Unit Tests

```
test_prism_parser.c:
  ✓ Parse valid header
  ✓ Reject invalid magic number
  ✓ Validate CRC32 (header + data)
  ✓ Load custom palette
  ✓ Check version compatibility

test_prism_executor.c:
  ✓ Load pattern from storage
  ✓ Lookup effect descriptor
  ✓ Play/stop state transitions
  ✓ Render frame dispatch

test_prism_effects.c:
  ✓ All 15 effects render correctly
  ✓ Verify GRB output format
  ✓ Measure render times
```

### Integration Tests

```
test_upload_to_render.c:
  ✓ Upload .prism via WebSocket
  ✓ Validate and store
  ✓ Load and play
  ✓ Render 1000 frames
  ✓ Verify 120 FPS sustained

test_effect_gallery.c:
  ✓ Load all 15 reference patterns
  ✓ Render each for 10 seconds
  ✓ Measure CPU usage
  ✓ Check for memory leaks
```

### Hardware Tests

```
test_24hour_soak.c:
  ✓ Run for 24 hours continuously
  ✓ Cycle through all effects
  ✓ Monitor frame drops
  ✓ Check heap fragmentation
  ✓ Verify visual output
```

## Next Steps

1. **Review this ADR** with Captain for approval
2. **Generate implementation tasks** using Taskmaster
3. **Phase 1:** Implement foundation (parser + math utilities)
4. **Phase 2:** Implement 3 simple effects + integration
5. **Phase 3-5:** Roll out remaining effects in phases
6. **Validation:** 24-hour soak test on hardware

---

**See also:**
- `009-prism-format-specification.md` - Full specification
- `009-prism-format-QUICKREF.md` - Quick reference guide
