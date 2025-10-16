# Template Catalog Specification

**Version:** 1.0
**Purpose:** Define 15-20 pre-built patterns for 60-second experience
**Target:** 80% of users will use templates without creating custom patterns

## Template Categories

### Category Distribution
- **Ambient (5-7 patterns):** Calm, atmospheric, background-friendly
- **Energy (5-7 patterns):** Dynamic, reactive, party-focused
- **Special (5-6 patterns):** Seasonal, unique, showcase effects

## Template Specifications

### AMBIENT CATEGORY

#### 1. Breathing Glow
**ID:** 0x01
**Description:** Gentle pulsing like peaceful breathing
**Parameters:**
- Speed: 0.15 (slow, 4-second cycle)
- Color: Warm white (#FFE5B4)
- Depth: 0.7 (70% brightness variation)
- Smoothness: 0.9 (very smooth transitions)

**Implementation:**
```c
void breathing_glow(float t, led_t* leds, int count) {
    float brightness = 0.3f + 0.7f * (sinf(t * 0.15f) * 0.5f + 0.5f);
    for (int i = 0; i < count; i++) {
        leds[i] = color_warm_white * brightness;
    }
}
```

#### 2. Ocean Waves
**ID:** 0x02
**Description:** Rolling waves of blue and cyan
**Parameters:**
- Speed: 0.25 (medium flow)
- Palette: Ocean (deep blue → cyan → white foam)
- Wave Count: 3
- Direction: Forward

**Implementation:**
```c
void ocean_waves(float t, led_t* leds, int count) {
    for (int i = 0; i < count; i++) {
        float phase = (float)i / count + t * 0.25f;
        float wave = sinf(phase * PI * 3) * 0.5f + 0.5f;
        leds[i] = palette_ocean.interpolate(wave);
    }
}
```

#### 3. Sunset Fade
**ID:** 0x03
**Description:** Slow transition through sunset colors
**Parameters:**
- Duration: 60 seconds per cycle
- Palette: Sunset (red → orange → purple → blue)
- Shimmer: 0.1 (subtle variation)

#### 4. Forest Canopy
**ID:** 0x04
**Description:** Dappled green light through leaves
**Parameters:**
- Base Color: Forest green (#228B22)
- Variation: 0.3 (30% random brightness)
- Spot Size: 5 LEDs
- Movement: 0.05 (very slow drift)

#### 5. Candlelight
**ID:** 0x05
**Description:** Realistic flickering candle simulation
**Parameters:**
- Color Temperature: 1850K (candle warm)
- Flicker Rate: 0.4 (natural variation)
- Wind Effect: 0.2 (occasional dimming)

### ENERGY CATEGORY

#### 6. Pulse Drive
**ID:** 0x06
**Description:** Rhythmic pulses traveling down strip
**Parameters:**
- BPM: 128 (dance tempo)
- Color: Electric blue (#00FFFF)
- Decay: 0.3 (trail length)
- Intensity: 0.8

**Implementation:**
```c
void pulse_drive(float t, led_t* leds, int count, float bpm) {
    float beat_phase = fmodf(t * (bpm / 60.0f), 1.0f);
    int pulse_pos = (int)(beat_phase * count);

    for (int i = 0; i < count; i++) {
        float distance = abs(i - pulse_pos) / (float)count;
        float intensity = expf(-distance * 10) * 0.8f;
        leds[i] = color_electric_blue * intensity;
    }
}
```

#### 7. Rainbow Runner
**ID:** 0x07
**Description:** Full spectrum colors racing along strip
**Parameters:**
- Speed: 1.0 (one cycle per second)
- Saturation: 1.0 (vivid colors)
- Segments: 1 (full rainbow visible)

#### 8. Strobe Party
**ID:** 0x08
**Description:** Club-style strobe with color changes
**Parameters:**
- Flash Rate: 10Hz
- Color Mode: Random per flash
- Intensity: 1.0 (full brightness)
- Duty Cycle: 0.1 (10% on time)

#### 9. Fire Effect
**ID:** 0x09
**Description:** Realistic fire animation
**Parameters:**
- Heat: 0.7 (flame temperature)
- Sparking: 0.4 (new flame probability)
- Cooling: 0.05 (fade rate)
- Palette: Fire (black → red → orange → yellow → white)

#### 10. Lightning Storm
**ID:** 0x0A
**Description:** Random lightning strikes
**Parameters:**
- Strike Rate: 0.2 (strikes per second)
- Brightness: 1.0 (blinding flash)
- Fork Probability: 0.3
- Afterglow: 0.5 seconds

### SPECIAL CATEGORY

#### 11. Christmas Twinkle
**ID:** 0x0B
**Description:** Red, green, and white twinkling
**Parameters:**
- Colors: Christmas (red, green, gold, white)
- Twinkle Rate: 0.5 (changes per second)
- Pattern: Random
- Fade Time: 0.3 seconds

#### 12. Valentine Hearts
**ID:** 0x0C
**Description:** Pink and red heart pulses
**Parameters:**
- Colors: Love (pink, red, white)
- Pulse Rate: 72 BPM (heartbeat)
- Spread: Groups of 5 LEDs

#### 13. Halloween Spooky
**ID:** 0x0D
**Description:** Orange and purple with random blackouts
**Parameters:**
- Colors: Halloween (orange, purple, green)
- Blackout Rate: 0.1 (occasional darkness)
- Creep Speed: 0.1 (slow color drift)

#### 14. Meteor Shower
**ID:** 0x0E
**Description:** Shooting stars across the strip
**Parameters:**
- Meteor Rate: 0.3 per second
- Trail Length: 8 LEDs
- Speed: 2.0 (fast movement)
- Color: White with blue tint

#### 15. Matrix Rain
**ID:** 0x0F
**Description:** Green digital rain effect
**Parameters:**
- Drop Rate: 0.5 per second
- Color: Matrix green (#00FF00)
- Fade Rate: 0.2
- Glitch Probability: 0.05

## Extended Templates (16-20)

#### 16. Aurora Borealis
**ID:** 0x10
**Description:** Northern lights simulation
**Parameters:**
- Colors: Aurora (green, blue, purple, pink)
- Wave Speed: 0.1 (very slow)
- Intensity Variation: 0.5

#### 17. Lava Lamp
**ID:** 0x11
**Description:** Slow morphing blobs of color
**Parameters:**
- Blob Count: 3
- Speed: 0.05 (hypnotic slow)
- Colors: Psychedelic palette

#### 18. Sparkle Celebration
**ID:** 0x12
**Description:** Random white sparkles on color base
**Parameters:**
- Base Color: User selectable
- Sparkle Rate: 5 per second
- Sparkle Brightness: 1.0

#### 19. Color Wash
**ID:** 0x13
**Description:** Solid colors with smooth transitions
**Parameters:**
- Transition Time: 3 seconds
- Color Sequence: User definable
- Hold Time: 2 seconds per color

#### 20. Audio Reactive (Future)
**ID:** 0x14
**Description:** Responds to music/sound
**Parameters:**
- Sensitivity: 0.5
- Mode: Spectrum/Beat/Volume
- Color Map: Frequency-based

## Template Metadata Structure

```c
typedef struct {
    uint8_t id;                    // Template ID
    char name[32];                 // Display name
    char description[64];          // User description
    uint8_t category;              // 0=Ambient, 1=Energy, 2=Special
    uint8_t complexity;            // CPU usage (1-5)
    uint16_t default_duration_ms;  // Default cycle time
    uint8_t param_count;          // Number of parameters

    struct {
        char name[16];             // Parameter name
        float default_value;       // Default (0-1)
        float min, max;           // Range
        uint8_t type;             // 0=float, 1=color, 2=enum
    } params[6];

    uint32_t flags;               // Feature flags
} template_metadata_t;
```

## Template Storage Format

Templates are compiled into firmware, not stored as .prism files:

```c
// templates/template_patterns.c
const template_function_t template_functions[] = {
    [0x01] = breathing_glow,
    [0x02] = ocean_waves,
    [0x03] = sunset_fade,
    // ...
};

const template_metadata_t template_metadata[] = {
    [0x01] = {
        .id = 0x01,
        .name = "Breathing Glow",
        .description = "Gentle pulsing like peaceful breathing",
        .category = CATEGORY_AMBIENT,
        .complexity = 1,
        .default_duration_ms = 4000,
        .param_count = 4,
        .params = {
            {"Speed", 0.15, 0.05, 1.0, PARAM_FLOAT},
            {"Depth", 0.7, 0.1, 1.0, PARAM_FLOAT},
            {"Color", 0, 0, 15, PARAM_COLOR},
            {"Smooth", 0.9, 0.0, 1.0, PARAM_FLOAT}
        }
    },
    // ...
};
```

## Performance Requirements

### CPU Usage (at 60 FPS, 150 LEDs)
- Ambient patterns: <10% CPU
- Energy patterns: <25% CPU
- Special patterns: <30% CPU

### Memory Usage
- Per template: <1KB RAM (parameters + state)
- All templates loaded: <20KB RAM total

### Response Time
- Template selection: <50ms
- Parameter change: <16ms (next frame)
- Template preview: <100ms

## User Experience Flow

### First Boot Experience
1. Device powers on → Shows "Breathing Glow" (safe default)
2. User connects via app → Sees template gallery
3. User taps template → Instant preview on LEDs
4. User adjusts slider → Real-time parameter update
5. User saves → Template becomes active pattern

### Template Selection UI
```
┌─────────────────────────────────┐
│  AMBIENT          [Tab Bar]     │
│  ┌────┐ ┌────┐ ┌────┐          │
│  │Breath│Ocean│Sunset│          │
│  │ 😌  │ 🌊  │ 🌅  │          │
│  └────┘ └────┘ └────┘          │
│  ┌────┐ ┌────┐                 │
│  │Forest│Candle│                │
│  │ 🌳  │ 🕯️  │                │
│  └────┘ └────┘                 │
│                                 │
│  [Speed: ═════○════]            │
│  [Depth: ═══════○══]            │
│  [Color: (color picker)]        │
│                                 │
│  [Apply]  [Save As...]          │
└─────────────────────────────────┘
```

## Testing Criteria

### Visual Quality
- Smooth transitions (no stepping at 60 FPS)
- Appropriate brightness levels
- Color accuracy on actual LEDs
- No flicker or artifacts

### Performance
- All templates run at 60 FPS minimum
- Parameter changes apply immediately
- No memory leaks after hours of use
- Switching templates takes <100ms

### User Testing
- 90% of users find desired effect in <30 seconds
- 80% successfully adjust parameters
- 95% achieve "first LED" in <60 seconds
- Average satisfaction rating >4.5/5

## Implementation Priority

### Phase 1 (MVP - Day 1-3)
1. Breathing Glow (simplest, safest default)
2. Ocean Waves (shows movement)
3. Pulse Drive (energy demonstration)
4. Rainbow Runner (colorful showcase)
5. Fire Effect (complex animation)

### Phase 2 (Polish - Day 4-5)
6. Sunset Fade
7. Candlelight
8. Strobe Party
9. Christmas Twinkle
10. Meteor Shower

### Phase 3 (Complete - Day 6-7)
11. Forest Canopy
12. Lightning Storm
13. Valentine Hearts
14. Halloween Spooky
15. Matrix Rain

### Future (Post-Launch)
16-20: Based on user feedback and requests

## Success Metrics

- **Adoption Rate:** >80% of users use templates
- **Satisfaction:** >4.5/5 star rating
- **Retention:** Users keep using after 1 week
- **Variety:** Each template used by >10% of users
- **Performance:** Zero complaints about lag/stutter