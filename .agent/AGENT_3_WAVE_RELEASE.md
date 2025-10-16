# AGENT 3: WAVE → Profile → Release Pipeline
**Mission: Sinusoidal Patterns & Production Release**

## Your Sequential Pipeline (19 subtasks)

```
Task 17: WAVE Mode (6 subtasks) → LUT-Based Waveforms
   ↓
Task 18: WAVE Profiling (5 subtasks) → Performance Optimization
   ↓
Task 20: Docs/Migration/Release (8 subtasks) → Production Ready
```

**Total: 19 subtasks, ~4-6 hours**

---

## 📋 WORKFLOW

```bash
git pull --rebase
task-master show <task-id>
# implement
git add . && git commit -m "feat(task-X): subtask X.Y" && git push
task-master set-status --id=X.Y --status=done
```

---

## 🎯 TASK 17: WAVE Mode with LUTs (6 subtasks)

**Goal:** Sinusoidal delay patterns via lookup tables
**Location:** `firmware/components/playback/`

### Research:
```bash
task-master show 17
```

### Key Concepts:
- 256-byte sin8 table (aligned(64) for cache)
- Triangle/sawtooth waveform evaluators
- Phase calculation: amplitude, frequency, offset
- <0.5μs execution target

---

### Subtask 17.1: Generate sin8 lookup table

**Create:** `firmware/components/playback/prism_wave_tables.h`

```c
#ifndef PRISM_WAVE_TABLES_H
#define PRISM_WAVE_TABLES_H

#include <stdint.h>

// 256-entry sine table (0-255 mapped to 0-2π)
// Output: 0-255 representing 0.0-1.0 range
extern const uint8_t sin8_table[256] __attribute__((aligned(64)));

// Fast triangle wave (branchless)
static inline uint8_t triangle8(uint8_t phase) {
    uint8_t folded = phase ^ ((phase & 0x80) ? 0xFF : 0x00);
    return folded << 1;
}

// Fast sawtooth wave
static inline uint8_t sawtooth8(uint8_t phase) {
    return phase;
}

#endif
```

**Create:** `firmware/components/playback/prism_wave_tables.c`

```c
#include "prism_wave_tables.h"

// Pre-computed 8-bit sine table (generated offline)
const uint8_t sin8_table[256] __attribute__((aligned(64))) = {
    128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 161, 164, 167, 170, 173,
    176, 178, 181, 184, 187, 189, 192, 194, 197, 199, 201, 204, 206, 208, 210, 212,
    214, 216, 218, 220, 221, 223, 224, 226, 227, 228, 229, 230, 231, 232, 233, 233,
    234, 234, 235, 235, 235, 235, 235, 235, 235, 234, 234, 233, 233, 232, 231, 230,
    229, 228, 227, 226, 224, 223, 221, 220, 218, 216, 214, 212, 210, 208, 206, 204,
    201, 199, 197, 194, 192, 189, 187, 184, 181, 178, 176, 173, 170, 167, 164, 161,
    158, 155, 152, 149, 146, 143, 140, 137, 134, 131, 128, 124, 121, 118, 115, 112,
    109, 106, 103, 100, 97, 94, 91, 88, 85, 82, 79, 77, 74, 71, 68, 66,
    63, 61, 58, 56, 54, 51, 49, 47, 45, 43, 41, 39, 37, 35, 34, 32,
    31, 29, 28, 27, 26, 25, 24, 23, 22, 22, 21, 21, 20, 20, 20, 20,
    20, 20, 20, 21, 21, 22, 22, 23, 24, 25, 26, 27, 28, 29, 31, 32,
    34, 35, 37, 39, 41, 43, 45, 47, 49, 51, 54, 56, 58, 61, 63, 66,
    68, 71, 74, 77, 79, 82, 85, 88, 91, 94, 97, 100, 103, 106, 109, 112,
    115, 118, 121, 124, 128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 161,
    164, 167, 170, 173, 176, 178, 181, 184, 187, 189, 192, 194, 197, 199, 201, 204,
    206, 208, 210, 212, 214, 216, 218, 220, 221, 223, 224, 226, 227, 228, 229, 230
};
```

**Commit:** `feat(task-17): complete subtask 17.1 - generate sin8 LUT`

---

### Subtask 17.2: Implement waveform evaluators

Already done in 17.1 (triangle8, sawtooth8 inline functions)

**Commit:** `feat(task-17): complete subtask 17.2 - add waveform evaluators`

---

### Subtask 17.3: Phase calculation engine

**Add to** `prism_temporal.c`:

```c
// Calculate wave delay for LED index
static inline uint16_t calculate_wave_delay(size_t led_index,
                                             const prism_sync_params_t *params) {
    // Phase: map LED index to 0-255
    const uint8_t phase = (uint8_t)((led_index * 256) / 160);

    // Look up sine value
    const uint8_t sine_val = sin8_table[phase];

    // Scale to delay range: base ± amplitude
    const int32_t delay = params->delay_ms +
                          ((sine_val - 128) * params->wave_amplitude_ms) / 128;

    // Clamp to uint16_t range
    return (uint16_t)((delay < 0) ? 0 : ((delay > 65535) ? 65535 : delay));
}
```

**Commit:** `feat(task-17): complete subtask 17.3 - add phase calculation`

---

### Subtask 17.4: WAVE LUT memory allocation

**Add to** `prism_temporal.c`:

```c
// Static delay table for WAVE mode (allocated once)
static uint16_t wave_delay_table[160] __attribute__((aligned(64)));

// Initialize WAVE delay table from parameters
static void init_wave_table(const prism_sync_params_t *params) {
    for (size_t i = 0; i < 160; i++) {
        wave_delay_table[i] = calculate_wave_delay(i, params);
    }
}
```

**Commit:** `feat(task-17): complete subtask 17.4 - allocate WAVE LUT memory`

---

### Subtask 17.5: Optimize calculate_ch2_frame for WAVE

**Add WAVE case to** `calculate_ch2_frame`:

```c
case PRISM_SYNC_WAVE: {
    // Initialize wave table if needed
    if (ctx->delay_table == NULL) {
        init_wave_table(&ctx->params);
        // NOTE: In production, delay_table should point to wave_delay_table
        // For now, use the static table directly
    }

    const uint32_t now_ms = prism_frame_time_ms(ctx, 16);

    // Apply wave delays
    for (size_t i = 0; i < led_count; ++i) {
        const uint16_t delay = wave_delay_table[i];
        ch2_frame[i] = (now_ms >= delay) ? ch1_frame[i] : 0;
    }
    break;
}
```

**Commit:** `feat(task-17): complete subtask 17.5 - optimize WAVE mode`

---

### Subtask 17.6: WAVE mode test suite

**Add to** `test_prism_temporal.c`:

```c
TEST_CASE("WAVE mode generates sinusoidal delays", "[temporal][wave]") {
    prism_temporal_ctx_t ctx;
    uint16_t ch1[160], ch2[160];

    prism_motion_init(&ctx, ch1, ch2, 160);
    ctx.sync_mode = PRISM_SYNC_WAVE;
    ctx.params.delay_ms = 100;
    ctx.params.wave_amplitude_ms = 50;

    // Initialize wave table
    init_wave_table(&ctx.params);

    // Verify delays vary sinusoidally
    TEST_ASSERT_GREATER_THAN(0, wave_delay_table[0]);
    TEST_ASSERT_GREATER_THAN(wave_delay_table[0], wave_delay_table[40]);
    TEST_ASSERT_GREATER_THAN(wave_delay_table[40], wave_delay_table[80]);
}
```

**Commit:** `test(task-17): complete subtask 17.6 - add WAVE tests`

**Mark complete:** `task-master set-status --id=17 --status=done`

---

## 🎯 TASK 18: WAVE Profiling (5 subtasks)

**Goal:** Sub-microsecond performance validation
**Location:** `firmware/components/playback/`

### Research:
```bash
task-master show 18
```

### Targets:
- LUT path median <1μs
- Cache profiling with PMU counters
- No heap fragmentation

---

### Subtask 18.1: CONFIG_PRISM_PROFILE_TEMPORAL framework

**Create:** `firmware/components/playback/Kconfig`

```kconfig
menu "PRISM Playback Configuration"

config PRISM_PROFILE_TEMPORAL
    bool "Enable temporal profiling"
    default n
    help
        Enable cycle-accurate profiling of temporal calculations.
        Adds ~1KB code size. Disable in production.

endmenu
```

**Add to** `prism_temporal.c`:

```c
#ifdef CONFIG_PRISM_PROFILE_TEMPORAL

typedef struct {
    uint32_t total_cycles;
    uint32_t min_cycles;
    uint32_t max_cycles;
    uint32_t samples;
} wave_prof_accum_t;

static wave_prof_accum_t prof_data[5];  // One per sync mode

static inline uint32_t wave_profiler_begin(void) {
    return esp_cpu_get_cycle_count();
}

static inline void wave_profiler_end(uint32_t start, prism_sync_mode_t mode) {
    uint32_t end = esp_cpu_get_cycle_count();
    uint32_t cycles = end - start;

    if (cycles < prof_data[mode].min_cycles || prof_data[mode].samples == 0) {
        prof_data[mode].min_cycles = cycles;
    }
    if (cycles > prof_data[mode].max_cycles) {
        prof_data[mode].max_cycles = cycles;
    }
    prof_data[mode].total_cycles += cycles;
    prof_data[mode].samples++;
}

#else
#define wave_profiler_begin() 0
#define wave_profiler_end(start, mode) do {} while(0)
#endif
```

**Commit:** `feat(task-18): complete subtask 18.1 - add profiling framework`

---

### Subtask 18.2-18.5: Profiling & optimization

Reference full research:
```bash
task-master show 18
```

Implement:
- esp_cpu_get_cycle_count instrumentation
- PMU cache profiling
- DRAM_ATTR placement validation
- Validation harness with <1μs assertion

**Commit each subtask**

**Mark complete:** `task-master set-status --id=18 --status=done`

---

## 🎯 TASK 20: Docs/Migration/Release (8 subtasks)

**Goal:** Production-ready release
**Location:** `docs/`, `tools/`, firmware

### Research:
```bash
task-master show 20
```

---

### Subtask 20.1: v1.0→v1.1 migration CLI

**Create:** `tools/migrate_prism.py`

```python
#!/usr/bin/env python3
import struct
import argparse

def migrate_v10_to_v11(input_path, output_path):
    """Migrate .prism v1.0 (64 bytes) to v1.1 (70 bytes)"""
    with open(input_path, 'rb') as f:
        v10_data = f.read()

    if len(v10_data) < 64:
        raise ValueError("File too small to be valid .prism v1.0")

    # Parse v1.0 header
    magic, version, led_count = struct.unpack_from('<4sHH', v10_data, 0)

    if magic != b'PRSM':
        raise ValueError("Invalid magic number")

    # Create v1.1 header
    v11_data = bytearray(v10_data[:64])  # Copy v1.0 header

    # Update version
    struct.pack_into('<H', v11_data, 4, 0x0101)

    # Add v1.1 motion/sync fields (default to SYNC/STATIC)
    v11_data.extend([
        0x01,  # version
        0x04,  # motion: STATIC
        0x00,  # sync: SYNC
        0x00,  # reserved
        0x00, 0x00,  # delay_ms
        0x00, 0x00,  # progressive_start_ms
        0x00, 0x00,  # progressive_end_ms
        0x00, 0x00,  # wave_amplitude_ms
        0x00, 0x00,  # wave_frequency_hz
        0x00, 0x00   # wave_phase_deg
    ])

    # Append LED payload
    v11_data.extend(v10_data[64:])

    # Recalculate CRC (TODO)

    with open(output_path, 'wb') as f:
        f.write(v11_data)

    print(f"Migrated {input_path} → {output_path}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('input', help='Input .prism v1.0 file')
    parser.add_argument('output', help='Output .prism v1.1 file')
    args = parser.parse_args()

    migrate_v10_to_v11(args.input, args.output)
```

**Commit:** `feat(task-20): complete subtask 20.1 - migration CLI`

---

### Subtask 20.2: User manual

**Create:** `docs/user-manual.md`

```markdown
# PRISM K1 User Manual v1.1

## Temporal Sequencing Overview

PRISM K1 firmware v1.1 introduces **temporal sequencing** - the ability to
stagger LED timing between the top and bottom edges of the Light Guide Plate.

### What is Temporal Sequencing?

By introducing small time delays (60-200ms) between when LEDs illuminate on
each edge, your brain perceives geometric shapes and motion through the
**phi phenomenon** - the same effect used in animated signs.

### Sync Modes

**SYNC** - Both edges illuminate simultaneously (like v1.0)

**OFFSET** - Top edge delayed by fixed time (rising/falling curtain)

**PROGRESSIVE** - Delay varies linearly (triangles, wedges, diamonds)

**WAVE** - Sinusoidal delays create organic flowing motion

**CUSTOM** - Expert mode: design your own per-LED timing curves

### Motion Direction

Controls how patterns propagate across the 160 LEDs:

- **LEFT**: Moves 0→159 (left-to-right)
- **RIGHT**: Moves 159→0 (right-to-left)
- **CENTER**: Radiates from middle outward
- **EDGE**: Converges from edges inward
- **STATIC**: No propagation

...
```

**Commit:** `docs(task-20): complete subtask 20.2 - user manual`

---

### Subtasks 20.3-20.8:

Full details:
```bash
task-master show 20
```

Deliverables:
- 5 video tutorials
- 24-hour soak test (3 units)
- 20+ pattern preset library
- CHANGELOG.md
- v1.1 firmware deployment + validation

**Commit each subtask**

**Mark complete:** `task-master set-status --id=20 --status=done`

---

## 🎉 MISSION ACCOMPLISHED!

**Total: 19 subtasks - WAVE mode, profiling, and production release complete!**

Final checks:
```bash
cd firmware
idf.py build
idf.py unity
```

**Congratulations! ADR-010 is PRODUCTION READY! 🚀🎉**
