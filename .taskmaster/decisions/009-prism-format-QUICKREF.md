# ADR-009 Quick Reference: PRISM Pattern Format

## Binary Format at a Glance

```
┌─────────────────────────────────────────────────────────────┐
│ PRISM Pattern File (.prism)                                 │
├─────────────────────────────────────────────────────────────┤
│ Offset │ Size │ Field                │ Description         │
├────────┼──────┼──────────────────────┼─────────────────────┤
│ 0x0000 │  4   │ magic                │ 0x4D535250 ("PRSM") │
│ 0x0004 │  1   │ version_major        │ 1                   │
│ 0x0005 │  1   │ version_minor        │ 0                   │
│ 0x0006 │  2   │ header_size          │ 64                  │
│ 0x0008 │  1   │ effect_id            │ 0x01-0xFF           │
│ 0x0009 │  1   │ channel_mode         │ MIRROR/SPLIT/etc    │
│ 0x000A │  2   │ flags                │ LOOP/ONCE/REVERSE   │
│ 0x000C │ 20   │ name                 │ UTF-8 pattern name  │
│ 0x0020 │  1   │ brightness           │ 0-255               │
│ 0x0021 │  1   │ speed                │ 0-255 (128=normal)  │
│ 0x0022 │  1   │ fade                 │ 0-255               │
│ 0x0023 │  1   │ param_count          │ 0-12                │
│ 0x0024 │ 12   │ params[12]           │ Effect parameters   │
│ 0x0030 │  1   │ palette_id           │ 0=custom, 1-255     │
│ 0x0031 │  1   │ palette_count        │ 0-16 colors         │
│ 0x0032 │  2   │ palette_offset       │ Offset to palette   │
│ 0x0034 │  4   │ reserved1            │ Future use          │
│ 0x0038 │  4   │ data_crc32           │ Palette data CRC    │
│ 0x003C │  4   │ header_crc32         │ Header CRC          │
├────────┴──────┴──────────────────────┴─────────────────────┤
│ 0x0040 │ 0-64 │ Palette data (optional)                    │
│        │      │ 16 × 4 bytes (RGB + padding)               │
└─────────────────────────────────────────────────────────────┘
```

## Effect ID Namespace

```
0x00        Reserved (invalid)
0x01-0x0F   Ambient Effects
  0x01      Solid Color
  0x02      Gradient
  0x03      Breathe
  0x04      Rainbow
  0x05      Twinkle

0x10-0x1F   Energy Effects
  0x10      Wave
  0x11      Fire
  0x12      Plasma
  0x13      Sparkle
  0x14      Meteor

0x20-0x2F   Special Effects
  0x20      Strobe
  0x21      Scanner (KITT)
  0x22      Color Cycle
  0x23      Theater Chase
  0x24      Lightning

0x30-0xFF   Reserved for future
```

## Channel Modes

```c
PRISM_CHANNEL_MIRROR      0x00  // Same pattern on both channels
PRISM_CHANNEL_SPLIT       0x01  // Split pattern across channels (320 LEDs)
PRISM_CHANNEL_ALTERNATE   0x02  // Alternate pattern on each channel
PRISM_CHANNEL_INDEPENDENT 0x03  // Channel-specific (future)
```

## Flags

```c
PRISM_FLAG_LOOP      0x0001  // Loop animation (default)
PRISM_FLAG_ONCE      0x0002  // Play once and hold
PRISM_FLAG_REVERSE   0x0004  // Reverse direction
PRISM_FLAG_USE_PALETTE 0x0008  // Use palette colors
PRISM_FLAG_SYMMETRIC 0x0010  // Symmetric rendering
```

## Effect Parameter Examples

### Solid Color (0x01)
```
params[0] = R (0-255)
params[1] = G (0-255)
params[2] = B (0-255)
```

### Wave (0x10)
```
params[0-2] = color1 (RGB)
params[3-5] = color2 (RGB)
params[6]   = wavelength (number of peaks)
params[7]   = phase offset
```

### Fire (0x11)
```
params[0] = heat intensity
params[1] = cooling rate
params[2] = sparking probability
palette_id = fire palette (or custom)
```

## Performance Targets

```
Frame Budget:     8.33ms (120 FPS)
LED Transmission: 4.8ms (both channels, hardware)
CPU Budget:       3.38ms (effect rendering)

Effect Render Times:
  Simple  (Solid, Breathe, Strobe):       50-200μs   (1-6% CPU)
  Medium  (Wave, Gradient, Scanner):     300-900μs   (9-27% CPU)
  Complex (Fire, Plasma):              1200-1500μs  (36-44% CPU)

All effects stay within 3.38ms budget.
```

## Data Flow Summary

```
┌──────────────┐
│ WebSocket    │ PUT_BEGIN → PUT_DATA → PUT_END
│ Upload       │ CRC32 validation
└──────┬───────┘
       ↓
┌──────────────┐
│ LittleFS     │ /littlefs/patterns/<name>.bin
│ Storage      │ 256KB max per pattern
└──────┬───────┘
       ↓ CONTROL PLAY
┌──────────────┐
│ Pattern      │ Load → Validate → Lookup effect
│ Executor     │ Store in current_pattern (128 bytes)
└──────┬───────┘
       ↓ 120 FPS
┌──────────────┐
│ Effect       │ render(pattern, frame_counter, ch1, ch2)
│ Render       │ Generate 960 bytes GRB24 data
└──────┬───────┘
       ↓
┌──────────────┐
│ LED Driver   │ Double-buffer → RMT → GPIO 9/10
│ Output       │ 4.8ms hardware transmission
└──────────────┘
```

## Memory Footprint

```
Pattern Header:       64 bytes
Palette Data:         64 bytes (16 colors × 4 bytes)
Total per pattern:   128 bytes

Effect Registry:     ~20 entries × 24 bytes = 480 bytes
Math Lookup Tables:  256 bytes (sin8 table)
Executor State:      128 bytes (current pattern)

Total RAM usage:     ~1KB static + 128 bytes per loaded pattern
```

## File Size Examples

```
Solid Color (no palette):        64 bytes
Wave with custom palette:        128 bytes
Fire with 8-color palette:       96 bytes
Complex multi-param effect:      ~150 bytes

Typical pattern size:            64-150 bytes
Maximum pattern size:            256KB (ADR-004)
Patterns per 1.5MB partition:    1000+ (realistic)
```

## Implementation Checklist

### Phase 1: Foundation
- [ ] Create `firmware/components/prism/` directory
- [ ] Implement `prism_math.c` (sin8, scale8, blend8)
- [ ] Implement `prism_parser.c` (header validation, CRC32)
- [ ] Write unit tests

### Phase 2: Simple Effects
- [ ] Implement `prism_executor.c` (load, render dispatch)
- [ ] Implement 3 effects: Solid, Gradient, Breathe
- [ ] Integrate with `led_playback.c`
- [ ] Test end-to-end at 120 FPS

### Phase 3: Medium Effects
- [ ] Implement 7 effects: Rainbow, Twinkle, Wave, Sparkle, Meteor, Scanner, Theater
- [ ] Add palette support
- [ ] Optimize render loops

### Phase 4: Complex Effects
- [ ] Implement 3 effects: Fire, Plasma, Lightning
- [ ] Optimize hot paths
- [ ] Measure actual render times

### Phase 5: Polish
- [ ] Add Color Cycle, Strobe
- [ ] Performance profiling
- [ ] 24-hour soak test
- [ ] Create reference patterns
- [ ] Documentation

## Quick API Reference

### Pattern Executor

```c
// Initialize
esp_err_t prism_executor_init(void);

// Load pattern from storage
esp_err_t prism_executor_load_pattern(const char *pattern_name);

// Start/stop playback
esp_err_t prism_executor_play(void);
esp_err_t prism_executor_stop(void);

// Render frame (called at 120 FPS)
esp_err_t prism_executor_render_frame(uint8_t *frame_ch1, uint8_t *frame_ch2);
```

### Effect Render Function

```c
typedef esp_err_t (*prism_effect_render_fn)(
    const prism_pattern_t *pattern,
    uint32_t frame_counter,
    uint8_t *frame_ch1,
    uint8_t *frame_ch2
);
```

### Math Utilities

```c
uint8_t sin8(uint8_t theta);                    // Fast sine (0-255)
uint8_t scale8(uint8_t i, uint8_t scale);       // Fast scaling
uint8_t blend8(uint8_t a, uint8_t b, uint8_t amount);  // Blend colors
prism_rgb_t color_from_palette(palette, count, index); // Palette lookup
```

## Validation Criteria

- [x] Binary format fits in 256KB maximum (ADR-004)
- [x] All effects render within 3.38ms CPU budget
- [x] Parser validates header + CRC32 correctly
- [x] Effect registry supports 15+ effects
- [x] Integration with existing led_driver.c and led_playback.c
- [x] Backward compatible (version field allows format evolution)
- [ ] Hardware validation (120 FPS sustained for 24 hours)
- [ ] Memory leak testing (no heap fragmentation)
- [ ] Reference patterns created for all 15 effects

## Next Steps

1. **Captain Review:** Review ADR-009 for approval
2. **Create Tasks:** Generate implementation tasks from roadmap
3. **Phase 1 Start:** Begin foundation work (prism_math, prism_parser)
4. **Integration Test:** Upload → Load → Render pipeline
5. **Effect Implementation:** Roll out 15 effects in phases

---

**For full specification, see:** `.taskmaster/decisions/009-prism-format-specification.md`
