# PRISM Template Pattern Specifications
*Version 1.0 - Launch Pattern Catalog*

## 3.1 Pattern Catalog

### AMBIENT PATTERNS (5)

---

#### **Pattern 1: Ocean Wave**
**ID:** `pattern_001`
**Category:** Ambient
**Description:** Gentle blue-green waves flowing across the strip with organic motion
**Memory:** 8192 bytes compressed / 20480 bytes runtime
**CPU Load:** 15% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.1-5.0 | 1.0 | Wave propagation speed |
| intensity | float | 0.0-1.0 | 0.7 | Wave amplitude/brightness variation |
| color_shift | float | 0-360 | 180 | Hue center point (180=cyan) |
| wave_count | int | 1-8 | 3 | Number of simultaneous waves |
| depth | float | 0.2-1.0 | 0.6 | Color depth variation |

**Visual Representation:**
```
Time →
T0: ░░▒▒▓▓████▓▓▒▒░░░░▒▒▓▓████▓▓▒▒░░
T1: ░░░▒▒▓▓████▓▓▒▒░░░░▒▒▓▓████▓▓▒▒░
T2: ▒░░░▒▒▓▓████▓▓▒▒░░░░▒▒▓▓████▓▓▒
T3: ▒▒░░░▒▒▓▓████▓▓▒▒░░░░▒▒▓▓████▓▓

Legend: ░=dim cyan ▒=medium cyan ▓=bright cyan █=peak blue
```

**Algorithm:**
```c
// Simplified wave generation
for (int i = 0; i < NUM_LEDS; i++) {
    float phase = (float)i / NUM_LEDS * wave_count * TWO_PI;
    float time_offset = time_ms * 0.001 * speed;
    float wave = sin(phase + time_offset);
    float intensity_mod = (wave + 1.0) * 0.5 * intensity;

    HSV hsv = {
        .h = color_shift + (wave * depth * 30),
        .s = 200 + (wave * 55),
        .v = 100 + (intensity_mod * 155)
    };
    leds[i] = hsv_to_rgb(hsv);
}
```

---

#### **Pattern 2: Aurora Borealis**
**ID:** `pattern_002`
**Category:** Ambient
**Description:** Northern lights shimmer with green-purple gradients
**Memory:** 10240 bytes compressed / 24576 bytes runtime
**CPU Load:** 22% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.1-5.0 | 0.8 | Animation speed |
| intensity | float | 0.0-1.0 | 0.6 | Brightness variation |
| color_shift | float | 0-360 | 120 | Base hue (120=green) |
| complexity | int | 1-5 | 3 | Pattern complexity |
| shimmer | float | 0.0-1.0 | 0.4 | Shimmer effect amount |

**Visual Representation:**
```
Time →
T0: ████▓▓▒░░░░▒▒▓███▓▒░░▒▓████▓▒░░
T1: ▓███▓▓▒░░░░░▒▓███▓▒░░▒▓████▓▒░▒
T2: ▒▓███▓▒░░░░░░▒███▓▒░░▒▓███▓▒░▒▓
T3: ░▒▓███▒░░░░░░▒███▓▒░▒▓███▓▒░▒▓█

Legend: ░=dark green ▒=medium green ▓=bright green █=purple hints
```

**Algorithm:**
```c
// Multi-layer aurora simulation
for (int i = 0; i < NUM_LEDS; i++) {
    float pos = (float)i / NUM_LEDS;
    float t = time_ms * 0.001 * speed;

    // Three aurora layers
    float layer1 = perlin_noise(pos * 2, t * 0.7) * 0.5;
    float layer2 = perlin_noise(pos * 3, t * 1.0) * 0.3;
    float layer3 = perlin_noise(pos * 5, t * 1.5) * 0.2;

    float combined = layer1 + layer2 + layer3;
    float shimmer_mod = 1.0 + (sin(t * 20 + i) * shimmer * 0.2);

    HSV hsv = {
        .h = color_shift + (combined * 60),
        .s = 180 + (combined * 75),
        .v = 80 + (combined * intensity * 175 * shimmer_mod)
    };
    leds[i] = hsv_to_rgb(hsv);
}
```

---

#### **Pattern 3: Breathing**
**ID:** `pattern_003`
**Category:** Ambient
**Description:** Smooth breathing/pulsing effect with customizable rhythm
**Memory:** 4096 bytes compressed / 8192 bytes runtime
**CPU Load:** 8% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.1-5.0 | 0.6 | Breathing rate |
| intensity | float | 0.0-1.0 | 0.8 | Max brightness |
| color_shift | float | 0-360 | 200 | Color (200=purple) |
| min_brightness | float | 0.0-0.5 | 0.1 | Minimum brightness |
| smoothness | float | 0.5-1.0 | 0.9 | Transition smoothness |

**Visual Representation:**
```
Time →
T0: ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
T1: ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
T2: ████████████████████████████████
T3: ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

Legend: ▒=dim ▓=medium █=bright (all same color)
```

**Algorithm:**
```c
// Smooth sine breathing
float t = time_ms * 0.001 * speed;
float breath = (sin(t * TWO_PI) + 1.0) * 0.5;
breath = smooth_curve(breath, smoothness);
float brightness = min_brightness + (breath * (intensity - min_brightness));

for (int i = 0; i < NUM_LEDS; i++) {
    HSV hsv = {
        .h = color_shift,
        .s = 255,
        .v = brightness * 255
    };
    leds[i] = hsv_to_rgb(hsv);
}
```

---

#### **Pattern 4: Candlelight**
**ID:** `pattern_004`
**Category:** Ambient
**Description:** Realistic flickering candlelight simulation
**Memory:** 6144 bytes compressed / 12288 bytes runtime
**CPU Load:** 12% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.1-5.0 | 1.2 | Flicker speed |
| intensity | float | 0.0-1.0 | 0.7 | Overall brightness |
| warmth | float | 0.0-1.0 | 0.8 | Color temperature |
| flicker_depth | float | 0.0-1.0 | 0.3 | Flicker variation |
| wind | float | 0.0-1.0 | 0.2 | Wind effect |

**Visual Representation:**
```
Time →
T0: ▓█▓▒▓█▓▓█▓▒▓█▓▓▓█▓▒▓█▓▓█▓▒▓█▓▓
T1: █▓▓█▓▒▓█▓▓▓▒▓█▓▒▓█▓▓▒▓█▓▓█▓▒▓█
T2: ▓▓█▓▓█▓▒▓█▓█▓▒▓█▓▒▓█▓▓▒▓█▓▒▓█▓
T3: ▓█▓▒▓█▓█▓▒▓▓█▓▒▓█▓▓▒▓█▓▓█▓█▓▒▓

Legend: ▒=dim orange ▓=warm orange █=bright yellow-orange
```

**Algorithm:**
```c
// Candle flicker simulation
for (int i = 0; i < NUM_LEDS; i++) {
    float t = time_ms * 0.001 * speed;
    float pos = (float)i / NUM_LEDS;

    // Multiple flicker frequencies
    float flicker1 = sin(t * 7.0 + i * 0.5) * 0.5;
    float flicker2 = sin(t * 13.0 + i * 0.3) * 0.3;
    float flicker3 = random_float() * 0.2;

    float combined = 1.0 + (flicker1 + flicker2 + flicker3) * flicker_depth;
    float wind_effect = sin(t * 2.0 + pos * PI) * wind;

    HSV hsv = {
        .h = 20 + (warmth * 20),  // Orange range
        .s = 255 - (warmth * 50),
        .v = intensity * combined * (1.0 + wind_effect) * 200
    };
    leds[i] = hsv_to_rgb(hsv);
}
```

---

#### **Pattern 5: Starfield**
**ID:** `pattern_005`
**Category:** Ambient
**Description:** Twinkling stars with occasional shooting stars
**Memory:** 7168 bytes compressed / 16384 bytes runtime
**CPU Load:** 10% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.1-5.0 | 0.5 | Twinkle speed |
| intensity | float | 0.0-1.0 | 0.6 | Star brightness |
| density | float | 0.1-1.0 | 0.3 | Number of stars |
| twinkle_rate | float | 0.0-1.0 | 0.4 | Twinkle frequency |
| shooting_star | float | 0.0-1.0 | 0.1 | Shooting star chance |

**Visual Representation:**
```
Time →
T0: ░░█░░░▒░░░█░░░░▒░░█░░░░▒░░░█░░░
T1: ░▒░░░░█░░▒░░░░█░░░▒░░░█░░▒░░░░█
T2: █░░░▒░░░█░░░▒░░░█░░░▒░░░█░░░▒░░
T3: ░░░█░░▒░░░█░░▒░░░===>░░░█░░▒░░░

Legend: ░=background ▒=dim star █=bright star ===>shooting star
```

**Algorithm:**
```c
// Star field with twinkles
typedef struct {
    uint8_t position;
    float brightness;
    float twinkle_phase;
} star_t;

static star_t stars[MAX_STARS];

for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = RGB(0, 0, 10);  // Dark blue background
}

for (int s = 0; s < num_stars; s++) {
    float t = time_ms * 0.001 * speed;
    stars[s].twinkle_phase += twinkle_rate * 0.1;

    float twinkle = (sin(stars[s].twinkle_phase) + 1.0) * 0.5;
    uint8_t brightness = intensity * twinkle * 255;

    leds[stars[s].position] = RGB(brightness, brightness, brightness * 0.9);
}

// Occasional shooting star
if (random_float() < shooting_star * 0.001) {
    create_shooting_star();
}
```

### ENERGY PATTERNS (5)

---

#### **Pattern 6: Rave Pulse**
**ID:** `pattern_006`
**Category:** Energy
**Description:** High-energy synchronized pulses with beat detection simulation
**Memory:** 9216 bytes compressed / 20480 bytes runtime
**CPU Load:** 25% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.1-10.0 | 2.0 | BPM multiplier |
| intensity | float | 0.0-1.0 | 0.9 | Pulse strength |
| color_shift | float | 0-360 | 0 | Starting hue |
| pulse_width | float | 0.1-1.0 | 0.3 | Pulse duration |
| color_cycle | float | 0.0-1.0 | 0.5 | Color rotation speed |

**Visual Representation:**
```
Time →
T0: ████████████████████████████████
T1: ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
T2: ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
T3: ████████████████████████████████

Legend: ▒=fade ▓=build █=peak (cycling colors)
```

**Algorithm:**
```c
// Beat-synchronized pulsing
float bpm = 128.0 * speed;
float beat_period = 60000.0 / bpm;
float beat_phase = fmod(time_ms, beat_period) / beat_period;

// Sharp attack, smooth decay
float pulse;
if (beat_phase < pulse_width) {
    pulse = beat_phase / pulse_width;  // Attack
} else {
    pulse = 1.0 - ((beat_phase - pulse_width) / (1.0 - pulse_width));  // Decay
    pulse = pulse * pulse;  // Exponential decay
}

float hue = color_shift + (time_ms * 0.001 * color_cycle * 360);

for (int i = 0; i < NUM_LEDS; i++) {
    HSV hsv = {
        .h = fmod(hue + (i * 2), 360),
        .s = 255,
        .v = pulse * intensity * 255
    };
    leds[i] = hsv_to_rgb(hsv);
}
```

---

#### **Pattern 7: Strobe**
**ID:** `pattern_007`
**Category:** Energy
**Description:** Classic strobe effect with adjustable frequency and duty cycle
**Memory:** 2048 bytes compressed / 4096 bytes runtime
**CPU Load:** 5% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.5-20.0 | 5.0 | Strobe frequency (Hz) |
| intensity | float | 0.0-1.0 | 1.0 | Flash brightness |
| duty_cycle | float | 0.05-0.5 | 0.1 | On-time ratio |
| color_mode | int | 0-2 | 0 | 0=white, 1=color, 2=random |
| hue | float | 0-360 | 0 | Color when mode=1 |

**Visual Representation:**
```
Time →
T0: ████████████████████████████████
T1: ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
T2: ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
T3: ████████████████████████████████

Legend: ░=off █=full brightness
```

**Algorithm:**
```c
// Simple strobe with duty cycle
float period_ms = 1000.0 / speed;
float phase = fmod(time_ms, period_ms) / period_ms;
bool strobe_on = phase < duty_cycle;

RGB color;
if (color_mode == 0) {
    color = RGB(255, 255, 255);  // White
} else if (color_mode == 1) {
    color = hsv_to_rgb((HSV){hue, 255, 255});
} else {
    color = hsv_to_rgb((HSV){random() % 360, 255, 255});
}

for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = strobe_on ?
        scale_rgb(color, intensity) :
        RGB(0, 0, 0);
}
```

---

#### **Pattern 8: Fire**
**ID:** `pattern_008`
**Category:** Energy
**Description:** Realistic fire effect with heat simulation
**Memory:** 8192 bytes compressed / 18432 bytes runtime
**CPU Load:** 20% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.1-5.0 | 1.5 | Animation speed |
| intensity | float | 0.0-1.0 | 0.8 | Fire intensity |
| cooling | float | 20-100 | 55 | Heat dissipation rate |
| sparking | float | 50-200 | 120 | New spark probability |
| height | float | 0.3-1.0 | 0.7 | Flame height |

**Visual Representation:**
```
Time →
T0: █▓▒░░░▒▓█▓▒░░░▒▓██▓▒░░░▒▓█▓▒░░
T1: ▓█▓▒░░░▒▓█▓▒░░░▒▓█▓▒░░▒▓██▓▒░░
T2: ▒▓█▓▒░░░▒▓██▓▒░░░▒▓█▓▒░░▒▓█▓▒
T3: ░▒▓█▓▒░░░▒▓█▓▒░░▒▓██▓▒░░░▒▓█▓

Legend: ░=black ▒=dark red ▓=orange █=yellow-white
```

**Algorithm:**
```c
// Fire2012 algorithm adapted
static uint8_t heat[NUM_LEDS];

// Cool down every cell
for (int i = 0; i < NUM_LEDS; i++) {
    heat[i] = max(0, heat[i] - random(0, ((cooling * 10) / NUM_LEDS) + 2));
}

// Heat drift upwards
for (int i = NUM_LEDS - 1; i >= 2; i--) {
    heat[i] = (heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3;
}

// Randomly ignite new sparks
if (random(255) < sparking) {
    int y = random(7);
    heat[y] = min(255, heat[y] + random(160, 255));
}

// Convert heat to LED colors
for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t temperature = heat[i] * intensity;
    leds[i] = heat_to_rgb(temperature);
}
```

---

#### **Pattern 9: Lightning**
**ID:** `pattern_009`
**Category:** Energy
**Description:** Random lightning strikes with afterglow
**Memory:** 5120 bytes compressed / 12288 bytes runtime
**CPU Load:** 15% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.1-5.0 | 1.0 | Strike frequency |
| intensity | float | 0.0-1.0 | 1.0 | Lightning brightness |
| strike_chance | float | 0.01-0.2 | 0.05 | Probability per frame |
| branches | int | 1-5 | 3 | Number of branches |
| afterglow | float | 0.0-1.0 | 0.3 | Afterglow duration |

**Visual Representation:**
```
Time →
T0: ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
T1: ████████░░░░████░░░░████░░░░░░░░
T2: ▓▓▓▓▓▓▓▓░░░░▓▓▓▓░░░░▓▓▓▓░░░░░░░
T3: ▒▒▒▒▒▒▒▒░░░░▒▒▒▒░░░░▒▒▒▒░░░░░░░

Legend: ░=dark ▒=fading ▓=afterglow █=strike
```

**Algorithm:**
```c
// Lightning strike system
typedef struct {
    int position;
    int length;
    float intensity;
    uint32_t strike_time;
} lightning_t;

static lightning_t strikes[MAX_STRIKES];
float t = time_ms * 0.001 * speed;

// Random strike generation
if (random_float() < strike_chance) {
    create_lightning_strike(random(NUM_LEDS),
                          random(10, 40),
                          intensity);
}

// Render strikes with decay
for (int i = 0; i < NUM_LEDS; i++) {
    float brightness = 0;

    for (int s = 0; s < num_strikes; s++) {
        if (i >= strikes[s].position &&
            i < strikes[s].position + strikes[s].length) {
            float age = time_ms - strikes[s].strike_time;
            float decay = exp(-age / (afterglow * 1000));
            brightness = max(brightness, strikes[s].intensity * decay);
        }
    }

    leds[i] = RGB(brightness * 200, brightness * 200, brightness * 255);
}
```

---

#### **Pattern 10: Plasma**
**ID:** `pattern_010`
**Category:** Energy
**Description:** Flowing plasma effect with color morphing
**Memory:** 10240 bytes compressed / 24576 bytes runtime
**CPU Load:** 30% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.1-5.0 | 1.0 | Flow speed |
| intensity | float | 0.0-1.0 | 0.8 | Color intensity |
| scale | float | 0.5-4.0 | 1.5 | Pattern scale |
| color_speed | float | 0.1-2.0 | 0.5 | Color morph speed |
| complexity | int | 1-4 | 2 | Number of plasma layers |

**Visual Representation:**
```
Time →
T0: ▓█▓▒░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░▒
T1: █▓▒░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░▒▓
T2: ▓▒░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░▒▓█
T3: ▒░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░▒▓█▓

Legend: Continuously morphing colors through spectrum
```

**Algorithm:**
```c
// Multi-layer plasma generation
for (int i = 0; i < NUM_LEDS; i++) {
    float x = (float)i / NUM_LEDS * scale;
    float t = time_ms * 0.001 * speed;

    float plasma = 0;

    // Layer 1: Basic sine plasma
    plasma += sin(x * 10 + t);

    // Layer 2: Diagonal waves
    if (complexity >= 2) {
        plasma += sin(x * 5 - t * 1.5) * 0.5;
    }

    // Layer 3: High frequency
    if (complexity >= 3) {
        plasma += sin(x * 20 + t * 2) * 0.25;
    }

    // Layer 4: Low frequency modulation
    if (complexity >= 4) {
        plasma += sin(x * 2 - t * 0.5) * 0.5;
    }

    // Normalize and convert to color
    plasma = (plasma / complexity) * 0.5 + 0.5;

    float hue = plasma * 360 + (t * color_speed * 100);

    HSV hsv = {
        .h = fmod(hue, 360),
        .s = 200 + (plasma * 55),
        .v = intensity * (100 + plasma * 155)
    };
    leds[i] = hsv_to_rgb(hsv);
}
```

### ARTISTIC PATTERNS (5)

---

#### **Pattern 11: Rainbow Flow**
**ID:** `pattern_011`
**Category:** Artistic
**Description:** Smooth rainbow gradient flowing across the strip
**Memory:** 3072 bytes compressed / 6144 bytes runtime
**CPU Load:** 7% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.1-5.0 | 1.0 | Flow speed |
| intensity | float | 0.0-1.0 | 0.9 | Brightness |
| scale | float | 0.5-4.0 | 1.0 | Rainbow width |
| saturation | float | 0.0-1.0 | 1.0 | Color saturation |
| direction | int | -1,0,1 | 1 | Flow direction |

**Visual Representation:**
```
Time →
T0: ████████████████████████████████
T1: ████████████████████████████████
T2: ████████████████████████████████
T3: ████████████████████████████████

Legend: Smooth gradient through full spectrum (R->O->Y->G->B->I->V)
```

**Algorithm:**
```c
// Flowing rainbow gradient
float t = time_ms * 0.001 * speed * direction;

for (int i = 0; i < NUM_LEDS; i++) {
    float position = (float)i / NUM_LEDS;
    float hue = fmod((position * scale + t) * 360, 360);

    HSV hsv = {
        .h = hue,
        .s = saturation * 255,
        .v = intensity * 255
    };
    leds[i] = hsv_to_rgb(hsv);
}
```

---

#### **Pattern 12: Color Gradient**
**ID:** `pattern_012`
**Category:** Artistic
**Description:** Static or animated gradient between two colors
**Memory:** 2048 bytes compressed / 4096 bytes runtime
**CPU Load:** 5% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.0-5.0 | 0.0 | Animation (0=static) |
| intensity | float | 0.0-1.0 | 0.8 | Brightness |
| color1_hue | float | 0-360 | 0 | Start color |
| color2_hue | float | 0-360 | 240 | End color |
| blend_curve | float | 0.5-2.0 | 1.0 | Gradient curve |

**Visual Representation:**
```
Static gradient (speed=0):
████████▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒░░░░░░░░

Animated (speed>0):
T0: ████████▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒░░░░░░░░
T1: ▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒░░░░░░░░████████
T2: ▒▒▒▒▒▒▒▒░░░░░░░░████████▓▓▓▓▓▓▓▓
T3: ░░░░░░░░████████▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒
```

**Algorithm:**
```c
// Two-color gradient with curve
float t = time_ms * 0.001 * speed;

for (int i = 0; i < NUM_LEDS; i++) {
    float position = (float)i / NUM_LEDS;

    // Apply curve to blend
    float blend = pow(position, blend_curve);

    // Animate if speed > 0
    if (speed > 0) {
        blend = fmod(blend + t, 1.0);
    }

    // Interpolate between colors
    float hue = lerp(color1_hue, color2_hue, blend);

    HSV hsv = {
        .h = hue,
        .s = 255,
        .v = intensity * 255
    };
    leds[i] = hsv_to_rgb(hsv);
}
```

---

#### **Pattern 13: Checkerboard**
**ID:** `pattern_013`
**Category:** Artistic
**Description:** Alternating color blocks with optional animation
**Memory:** 3072 bytes compressed / 6144 bytes runtime
**CPU Load:** 6% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.0-5.0 | 0.0 | Scroll speed |
| intensity | float | 0.0-1.0 | 0.8 | Brightness |
| block_size | int | 1-20 | 5 | Size of blocks |
| color1 | float | 0-360 | 0 | First color |
| color2 | float | 0-360 | 180 | Second color |

**Visual Representation:**
```
Static (block_size=5):
█████░░░░░█████░░░░░█████░░░░░█████

Animated:
T0: █████░░░░░█████░░░░░█████░░░░░█████
T1: ████░░░░░█████░░░░░█████░░░░░█████░
T2: ███░░░░░█████░░░░░█████░░░░░█████░░
T3: ██░░░░░█████░░░░░█████░░░░░█████░░░
```

**Algorithm:**
```c
// Checkerboard pattern
float offset = time_ms * 0.001 * speed * block_size;

for (int i = 0; i < NUM_LEDS; i++) {
    int block_index = ((i + (int)offset) / block_size) % 2;

    float hue = (block_index == 0) ? color1 : color2;

    HSV hsv = {
        .h = hue,
        .s = 255,
        .v = intensity * 255
    };
    leds[i] = hsv_to_rgb(hsv);
}
```

---

#### **Pattern 14: Spiral**
**ID:** `pattern_014`
**Category:** Artistic
**Description:** Spiraling colors with depth illusion
**Memory:** 7168 bytes compressed / 16384 bytes runtime
**CPU Load:** 18% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.1-5.0 | 1.0 | Rotation speed |
| intensity | float | 0.0-1.0 | 0.8 | Brightness |
| spiral_count | int | 1-8 | 3 | Number of spirals |
| color_range | float | 30-360 | 120 | Color spread |
| depth | float | 0.0-1.0 | 0.5 | 3D depth effect |

**Visual Representation:**
```
Time →
T0: ██▓▓▒▒░░░░▒▒▓▓██▓▓▒▒░░░░▒▒▓▓██
T1: █▓▓▒▒░░░░▒▒▓▓██▓▓▒▒░░░░▒▒▓▓███
T2: ▓▓▒▒░░░░▒▒▓▓██▓▓▒▒░░░░▒▒▓▓████
T3: ▓▒▒░░░░▒▒▓▓██▓▓▒▒░░░░▒▒▓▓█████

Legend: Spiraling gradient with depth shading
```

**Algorithm:**
```c
// 3D spiral effect
float t = time_ms * 0.001 * speed;

for (int i = 0; i < NUM_LEDS; i++) {
    float position = (float)i / NUM_LEDS;

    // Create spiral
    float spiral_phase = position * spiral_count * TWO_PI;
    float spiral_value = sin(spiral_phase - t * TWO_PI);

    // Add depth
    float depth_mod = 1.0 - (abs(spiral_value) * depth * 0.5);

    // Calculate color
    float hue = (spiral_value + 1.0) * 0.5 * color_range;

    HSV hsv = {
        .h = hue,
        .s = 255,
        .v = intensity * depth_mod * 255
    };
    leds[i] = hsv_to_rgb(hsv);
}
```

---

#### **Pattern 15: Lava Lamp**
**ID:** `pattern_015`
**Category:** Artistic
**Description:** Slow morphing blobs like a lava lamp
**Memory:** 12288 bytes compressed / 28672 bytes runtime
**CPU Load:** 25% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.1-2.0 | 0.3 | Animation speed |
| intensity | float | 0.0-1.0 | 0.7 | Brightness |
| blob_count | int | 2-6 | 4 | Number of blobs |
| color_mode | int | 0-2 | 1 | 0=red/yellow, 1=multi, 2=mono |
| viscosity | float | 0.1-1.0 | 0.7 | Blob flow smoothness |

**Visual Representation:**
```
Time →
T0: ░░▒▓███▓▒░░░░░░▒▓██▓▒░░░░░░░░░░
T1: ░░░▒▓███▓▒░░░░▒▓██▓▒░░░░░░░░░▒▓
T2: ░░░░▒▓███▓▒░▒▓██▓▒░░░░░░░░▒▓███
T3: ░░░░░▒▓████▓██▓▒░░░░░░░▒▓███▓▒░

Legend: ░=background ▒=blob edge ▓=blob center █=blob core
```

**Algorithm:**
```c
// Metaball/blob simulation
typedef struct {
    float position;
    float velocity;
    float size;
    float hue;
} blob_t;

static blob_t blobs[MAX_BLOBS];

// Update blob positions
for (int b = 0; b < blob_count; b++) {
    blobs[b].position += blobs[b].velocity * speed * 0.01;

    // Bounce at edges
    if (blobs[b].position < 0 || blobs[b].position > 1) {
        blobs[b].velocity *= -viscosity;
        blobs[b].position = clamp(blobs[b].position, 0, 1);
    }

    // Random velocity changes
    blobs[b].velocity += (random_float() - 0.5) * 0.1 * (1.0 - viscosity);
}

// Render metaballs
for (int i = 0; i < NUM_LEDS; i++) {
    float position = (float)i / NUM_LEDS;
    float field_strength = 0;
    float hue_sum = 0;

    for (int b = 0; b < blob_count; b++) {
        float distance = abs(position - blobs[b].position);
        float influence = blobs[b].size / (distance * distance + 0.01);
        field_strength += influence;
        hue_sum += blobs[b].hue * influence;
    }

    if (field_strength > 1.0) {
        hue_sum /= field_strength;

        HSV hsv = {
            .h = color_mode == 2 ? 0 : hue_sum,
            .s = color_mode == 0 ? 255 : 200,
            .v = min(field_strength, 1.0) * intensity * 255
        };
        leds[i] = hsv_to_rgb(hsv);
    } else {
        leds[i] = RGB(0, 0, 0);
    }
}
```

## 3.2 Parameter Standardization

### Universal Parameters
All patterns MUST support these base parameters:

| Parameter | Type | Range | Default | Description | Implementation |
|-----------|------|-------|---------|-------------|----------------|
| brightness | float | 0.0-1.0 | 1.0 | Master dimmer | Applied after pattern generation |
| speed | float | 0.1-10.0 | 1.0 | Playback rate multiplier | Multiplies time input |
| scale | float | 0.5-4.0 | 1.0 | Spatial frequency | Affects pattern wavelength |

### Parameter Application Order
```c
// Standard parameter application pipeline
void apply_pattern(uint32_t time_ms) {
    // 1. Apply speed to time
    float adjusted_time = time_ms * params.speed;

    // 2. Generate pattern with scale
    generate_pattern(adjusted_time, params.scale);

    // 3. Apply master brightness
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = scale_rgb(leds[i], params.brightness);
    }
}
```

### Parameter Interpolation
```c
// Smooth parameter transitions
typedef struct {
    float current;
    float target;
    float rate;
} param_interpolator_t;

float interpolate_param(param_interpolator_t* p, float delta_time) {
    float diff = p->target - p->current;
    float step = diff * p->rate * delta_time;

    if (abs(step) > abs(diff)) {
        p->current = p->target;
    } else {
        p->current += step;
    }

    return p->current;
}
```

## 3.3 Template Storage Format

### Binary Pattern Structure
```c
typedef struct {
    uint32_t magic;           // 'PRSM' (0x5053524D)
    uint16_t version;         // 0x0100 for v1.0
    uint16_t pattern_id;      // Unique pattern identifier
    char name[32];           // Human-readable name
    uint32_t size;           // Total structure size in bytes
    uint32_t crc32;          // CRC32 of entire structure
    uint8_t category;        // 0=Ambient, 1=Energy, 2=Artistic
    uint8_t param_count;     // Number of parameters (3-8)
    uint16_t flags;          // Feature flags
    uint32_t cpu_estimate;   // CPU usage in 0.1% units
    uint32_t ram_estimate;   // RAM usage in bytes
    param_def_t params[8];   // Parameter definitions
    uint32_t data_offset;    // Offset to pattern data
    uint32_t data_size;      // Size of pattern data
    uint8_t data[];          // Compressed pattern bytecode
} prism_pattern_t;

typedef struct {
    char name[16];           // Parameter name
    uint8_t type;            // 0=float, 1=int, 2=bool
    union {
        struct {
            float min;
            float max;
            float default_val;
            float step;
        } float_param;
        struct {
            int32_t min;
            int32_t max;
            int32_t default_val;
        } int_param;
        struct {
            uint8_t default_val;
        } bool_param;
    } config;
} param_def_t;
```

### Pattern Data Encoding
```c
// Bytecode instruction set for pattern engine
enum pattern_opcode {
    OP_SET_PIXEL    = 0x01,  // Set single pixel
    OP_SET_RANGE    = 0x02,  // Set pixel range
    OP_GRADIENT     = 0x03,  // Apply gradient
    OP_SINE_WAVE    = 0x04,  // Generate sine wave
    OP_NOISE        = 0x05,  // Perlin noise
    OP_BLEND        = 0x06,  // Blend with previous
    OP_LOOP         = 0x07,  // Loop instruction block
    OP_PARAM_LOAD   = 0x08,  // Load parameter value
    OP_TIME_LOAD    = 0x09,  // Load time value
    OP_END          = 0xFF   // End of program
};

// Example bytecode for Ocean Wave pattern
uint8_t ocean_wave_bytecode[] = {
    OP_TIME_LOAD,           // Load current time
    OP_PARAM_LOAD, 0x00,    // Load speed parameter
    OP_SINE_WAVE,  0x03,    // Generate 3 waves
    OP_PARAM_LOAD, 0x02,    // Load color_shift
    OP_GRADIENT,            // Apply color gradient
    OP_PARAM_LOAD, 0x01,    // Load intensity
    OP_BLEND,               // Blend with intensity
    OP_END
};
```

### Compression Strategy
```c
// Pattern data is compressed using Heatshrink
typedef struct {
    uint8_t window_bits;     // 10 (1KB window)
    uint8_t lookahead_bits;  // 5 (32 byte lookahead)
    uint32_t uncompressed_size;
    uint32_t compressed_size;
    uint8_t compressed_data[];
} compressed_pattern_t;

// Compression ratios for each pattern type
// Ambient: ~60% compression (simple, repetitive)
// Energy:  ~50% compression (moderate complexity)
// Artistic: ~40% compression (complex, varied)
```

## 3.4 Pattern Metadata

### Category Definitions
| Category | ID | Description | Typical Use Case | Power Draw |
|----------|-----|-------------|------------------|------------|
| Ambient | 0 | Calm, atmospheric | Background lighting | Low (30-50%) |
| Energy | 1 | Dynamic, intense | Party, exercise | High (70-100%) |
| Artistic | 2 | Creative, unique | Display, decoration | Medium (50-70%) |

### Performance Profiles
```c
typedef struct {
    uint8_t pattern_id;
    uint8_t min_fps;         // Minimum FPS required
    uint8_t target_fps;      // Target FPS for best effect
    uint16_t update_period;  // Milliseconds between updates
    uint32_t cycle_time;     // Full pattern cycle in ms
    float power_factor;      // 0.0-1.0 power consumption
} performance_profile_t;

const performance_profile_t profiles[] = {
    {0x01, 30, 60, 16, 10000, 0.35},  // Ocean Wave
    {0x02, 30, 60, 16, 15000, 0.40},  // Aurora
    {0x03, 20, 30, 33, 3000,  0.30},  // Breathing
    {0x04, 30, 60, 16, 0,     0.35},  // Candlelight
    {0x05, 20, 30, 33, 0,     0.25},  // Starfield
    {0x06, 60, 60, 16, 469,   0.80},  // Rave Pulse (128 BPM)
    {0x07, 60, 60, 16, 200,   0.90},  // Strobe
    {0x08, 30, 60, 16, 0,     0.60},  // Fire
    {0x09, 30, 60, 16, 0,     0.70},  // Lightning
    {0x0A, 30, 60, 16, 0,     0.75},  // Plasma
    {0x0B, 30, 60, 16, 0,     0.50},  // Rainbow
    {0x0C, 10, 30, 100, 0,    0.40},  // Gradient
    {0x0D, 10, 30, 100, 0,    0.40},  // Checkerboard
    {0x0E, 30, 60, 16, 5000,  0.50},  // Spiral
    {0x0F, 20, 30, 33, 0,     0.45},  // Lava Lamp
};
```

## 3.5 Pattern Initialization

### Factory Template Loading
```c
// Templates are stored in flash memory
const prism_pattern_t* factory_templates[] = {
    &pattern_ocean_wave,
    &pattern_aurora,
    &pattern_breathing,
    &pattern_candlelight,
    &pattern_starfield,
    &pattern_rave_pulse,
    &pattern_strobe,
    &pattern_fire,
    &pattern_lightning,
    &pattern_plasma,
    &pattern_rainbow,
    &pattern_gradient,
    &pattern_checkerboard,
    &pattern_spiral,
    &pattern_lava_lamp
};

// Load all templates to storage on first boot
void load_factory_templates() {
    for (int i = 0; i < 15; i++) {
        const prism_pattern_t* template = factory_templates[i];

        // Verify template integrity
        uint32_t crc = calculate_crc32((uint8_t*)template, template->size);
        if (crc != template->crc32) {
            ESP_LOGE(TAG, "Template %d corrupt!", i);
            continue;
        }

        // Save to filesystem
        char filename[64];
        snprintf(filename, sizeof(filename),
                "/prism/template_%03d.prism", template->pattern_id);

        save_pattern_to_storage(filename, template);
    }
}
```

### Pattern Selection Logic
```c
// Smart pattern selection based on time of day
uint16_t select_auto_pattern() {
    int hour = get_current_hour();

    if (hour >= 6 && hour < 9) {
        // Morning: energizing
        return 0x0B;  // Rainbow Flow
    } else if (hour >= 9 && hour < 17) {
        // Day: subtle ambient
        return 0x03;  // Breathing
    } else if (hour >= 17 && hour < 20) {
        // Evening: calming
        return 0x01;  // Ocean Wave
    } else if (hour >= 20 && hour < 23) {
        // Night: atmospheric
        return 0x02;  // Aurora
    } else {
        // Late night: minimal
        return 0x05;  // Starfield
    }
}
```

## 3.6 Testing & Validation

### Pattern Test Suite
```c
// Automated pattern testing
typedef struct {
    uint16_t pattern_id;
    float test_duration_sec;
    float expected_fps;
    float max_cpu_percent;
    uint32_t max_heap_bytes;
    bool (*validate_func)(rgb_t* frame);
} pattern_test_t;

void test_all_patterns() {
    for (int i = 0; i < 15; i++) {
        ESP_LOGI(TAG, "Testing pattern %d...", i);

        // Load pattern
        load_pattern(i);

        // Run for test duration
        uint32_t start_time = esp_timer_get_time() / 1000;
        float min_fps = 1000, max_fps = 0, avg_fps = 0;
        int frame_count = 0;

        while ((esp_timer_get_time() / 1000 - start_time) < 5000) {
            uint32_t frame_start = esp_timer_get_time();

            // Render frame
            render_pattern_frame();

            uint32_t frame_time = esp_timer_get_time() - frame_start;
            float fps = 1000000.0 / frame_time;

            min_fps = fmin(min_fps, fps);
            max_fps = fmax(max_fps, fps);
            avg_fps += fps;
            frame_count++;
        }

        avg_fps /= frame_count;

        ESP_LOGI(TAG, "Pattern %d: FPS min=%.1f avg=%.1f max=%.1f",
                i, min_fps, avg_fps, max_fps);
    }
}
```

---

*End of template_specifications.md - Version 1.0*