# Light Guide Plate (LGP) Visual Perception Analysis
## Research Date: 2025-10-16

## Physical Properties of Dual Edge-Lit LGP

### Light Propagation Mechanics

**Edge-Lit LGP Behavior:**
1. Light enters from edge(s)
2. Total internal reflection carries light through acrylic
3. Surface extraction features (dots, textures, or roughness) scatter light out toward viewer
4. Brightness decreases with distance from LED source (inverse square + material losses)

**Dual Edge Configuration (Top + Bottom):**
```
Brightness Profile (Cross-Section View):

Single Edge-Lit:
Intensity
  100% │██████░░░░░░░      (Strong near edge, fades toward opposite)
       │
     0 └────────────────
        EDGE          OPPOSITE

Dual Edge-Lit (Top + Bottom):
Intensity
  100% │████████████       (Top edge contribution)
       │████████████       (Bottom edge contribution)
       │████████████       (ADDITIVE overlap in center)
     0 └────────────────
        TOP    CENTER    BOTTOM
```

**Critical Insight:** Dual edge-lit LGPs create **additive light mixing** across the entire surface. The viewer does NOT see distinct "top" and "bottom" zones—they see a **unified glowing surface** with varying intensity distributions.

---

## Scenario Analysis: Visual Perception Outcomes

### Scenario 1: MIRROR Mode (Both Channels Same Pattern)

**Configuration:**
- CH1 (bottom): Rainbow wave left→right
- CH2 (top): Rainbow wave left→right (identical timing/color)

**Optical Result:**
```
User Perception: UNIFIED GLOWING SURFACE

Visual Appearance:
┌─────────────────────────────────────┐
│  Rainbow wave propagates L→R        │
│  BRIGHTER than single channel       │
│  Smooth vertical gradient           │
│  (slight top/bottom emphasis        │
│   but NOT distinct bands)           │
└─────────────────────────────────────┘
```

**Answer: C** - Blended/averaged output with **enhanced brightness**

**Perceptual Characteristics:**
- **Not** two separate bands (LGP diffusion prevents hard boundaries)
- **Not** perfectly uniform (brightness peaks near edges)
- **IS** unified surface with gentle vertical gradient (stronger at edges, softer in middle)
- **Enhanced brightness:** Dual sources = ~1.5-1.8x perceived brightness vs single edge

**Design Implication:** MIRROR mode creates **reinforced patterns** with improved brightness uniformity.

---

### Scenario 2: SPLIT Mode (Different Patterns)

**Configuration:**
- CH1 (bottom): Fire effect (warm: red/orange/yellow)
- CH2 (top): Wave effect (cool: blue/cyan/white)

**Optical Result:**
```
User Perception: VERTICAL COLOR GRADIENT

Visual Appearance (looking at LGP surface):
┌─────────────────────────────────────┐
│ Cool blues/cyans (top-emphasized)   │ ← CH2 dominates
│ ╔═══════════════════════════════╗   │
│ ║  Blended purple/teal zone     ║   │ ← Additive color mixing
│ ╚═══════════════════════════════╝   │
│ Warm reds/oranges (bottom-emphasized)│ ← CH1 dominates
└─────────────────────────────────────┘
```

**Answer: B** - Blended colors in middle, distinct color temperature zones at edges

**Perceptual Characteristics:**
- **Top ~40% of LGP:** Cool colors dominate (CH2 stronger influence)
- **Middle ~20%:** Additive color blending (purple, teal, complex hues)
- **Bottom ~40%:** Warm colors dominate (CH1 stronger influence)
- **Temporal dynamics:** If patterns animate at different rates, creates **layered motion perception**

**Design Implication:** SPLIT mode enables **vertical color zoning** but NOT hard boundaries. Think "gradient layers" not "split screen."

---

## Motion Class Redefinition for Dual Edge-Lit LGP

### Problem: Current Motion Classes Are 1D Linear

**Current definitions assume:**
- LEDs arranged in visible line
- Motion = physical LED progression
- User directly sees LED positions

**Reality of LGP:**
- LEDs hidden at edges
- User sees diffused surface illumination
- Motion = **perceived light propagation** across 2D surface

### Proposed Motion Class Semantic Shift

**FROM:** "LED index progression direction"
**TO:** "Perceived surface light flow direction"

---

### Scenario 3: CENTER_ORIGIN with Dual Edge

**Original Concept (1D LED strip thinking):**
```
LEDs: [0...79][80...159]
Motion: 79←0  |  80→159  (splits at center, expands outward)
```

**With Dual Edge-Lit LGP:**
```
Configuration:
- CH1 (bottom): CENTER_ORIGIN
- CH2 (top): CENTER_ORIGIN

Physical Reality:
Bottom Edge: LED 79 ────→ LED 0    |    LED 80 ────→ LED 159
Top Edge:    LED 79 ────→ LED 0    |    LED 80 ────→ LED 159

User Perception:
┌─────────────────────────────────────┐
│ ←──────────  ║  ──────────→        │ (horizontal spreading)
│ ←──────────  ║  ──────────→        │ (both edges synchronize)
│ ←──────────  ║  ──────────→        │ (user sees L+R motion)
└─────────────────────────────────────┘
         CENTER (LED 79-80)
```

**Answer: A** - Horizontal spreading (left ← center → right)

**Why NOT vertical spreading:**
- Both edges move in same horizontal direction
- No vertical differential to create up/down motion
- LGP diffusion reinforces horizontal symmetry

---

### Scenario 4: Creating Vertical Motion Perception

**Asymmetric Motion Attempt:**
```
Configuration:
- CH1 (bottom): LEFT_ORIGIN (LED 0 → LED 159, L→R)
- CH2 (top): RIGHT_ORIGIN (LED 159 → LED 0, R→L)

Physical Reality:
Bottom Edge: LED 0 ──────────────→ LED 159
Top Edge:    LED 159 ←────────────  LED 0

User Perception:
┌─────────────────────────────────────┐
│ ←───────────────────────────────    │ (top: right-to-left)
│     ╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱           │ (DIAGONAL shear)
│        ╱╱╱╱╱╱╱╱╱╱╱╱╱╱              │ (visual confusion)
│ ────────────────────────────────→   │ (bottom: left-to-right)
└─────────────────────────────────────┘
```

**Answer: C** - Visual confusion (diagonal shear effect)

**Why this fails:**
- Opposing horizontal motion creates **temporal phase mismatch**
- Center convergence point shifts over time (dynamic diagonal)
- NOT perceived as vertical motion—perceived as **competing motion**

**To Create True Vertical Motion:**
You need **temporal sequencing**, not spatial reversal:

```
VERTICAL_RISE Configuration:
- CH1 (bottom): Fire at T=0ms
- CH2 (top): Fire at T=100ms
- Both: Same motion class (e.g., LEFT_ORIGIN)

Result:
Bottom lights first → Top lights 100ms later → Perception of "rising"
```

**Key Insight:** Vertical motion requires **time delays between channels**, not different motion classes.

---

## Recommended Motion Class Definitions for LGP

### Reframe: "Surface Light Flow" Instead of "LED Index"

| Motion Class | Physical LED Behavior | Perceived Surface Flow | Use Case |
|--------------|----------------------|------------------------|----------|
| **LEFT_ORIGIN** | LED 0 → LED 159 | Light sweeps left edge → right edge | Horizontal wipe L→R |
| **RIGHT_ORIGIN** | LED 159 → LED 0 | Light sweeps right edge → left edge | Horizontal wipe R→L |
| **CENTER_ORIGIN** | LEDs 79+80 expand outward | Light blooms from center → edges | Radial expand (horizontal) |
| **EDGE_ORIGIN** | LEDs 0+159 converge inward | Light sweeps from edges → center | Radial collapse (horizontal) |
| **STATIC** | All LEDs same state | Uniform surface glow (or texture) | Ambient/background |

**Removed:** References to "top", "bottom", "vertical" from motion class semantics.

**Why:** Motion classes define **per-channel** behavior. Vertical dynamics are created by **channel coordination** + **timing**.

---

## Channel Coordination Modes: Beyond MIRROR/SPLIT

### Mode 1: MIRROR (Synchronized Reinforcement)
```
Configuration:
- CH1 = CH2 (same pattern, same timing)

Visual Result:
┌─────────────────────────────────────┐
│  Unified pattern                    │
│  Enhanced brightness                │
│  Smooth vertical uniformity         │
└─────────────────────────────────────┘

Use Cases:
- Maximum brightness mode
- Simple, bold patterns
- High-visibility effects (alerts, notifications)
```

**Pros:**
- Brightest output
- Most uniform appearance
- Simple to understand

**Cons:**
- No vertical dynamics
- Less visual complexity

---

### Mode 2: SPLIT (Independent Zones)
```
Configuration:
- CH1 = Pattern A
- CH2 = Pattern B (different pattern/colors)

Visual Result:
┌─────────────────────────────────────┐
│ Pattern B (top-dominant)            │ ← Cool colors
│ ╔═══════════════════════════════╗   │
│ ║  Color blend zone             ║   │ ← Additive mixing
│ ╚═══════════════════════════════╝   │
│ Pattern A (bottom-dominant)         │ ← Warm colors
└─────────────────────────────────────┘

Use Cases:
- Dual-theme displays (e.g., temp indicator: cold top, hot bottom)
- Layered ambience
- Complex visual storytelling
```

**Pros:**
- Vertical color zoning
- Artistic complexity
- Dual information display

**Cons:**
- Potential visual conflict if patterns clash
- Reduced clarity for single-message displays

---

### Mode 3: GRADIENT (Coordinated Blend)
```
Configuration:
- CH1 = Color A (e.g., pure red)
- CH2 = Color B (e.g., pure blue)
- Both: STATIC motion class

Visual Result:
┌─────────────────────────────────────┐
│ ████████████████████ Pure Blue     │ ← CH2 (top)
│ ████████████████████ Purple        │
│ ████████████████████ Purple-Pink   │ ← Blend zone
│ ████████████████████ Pink          │
│ ████████████████████ Pure Red      │ ← CH1 (bottom)
└─────────────────────────────────────┘

Use Cases:
- Sunrise/sunset effects
- Temperature visualization
- Smooth mood lighting
```

**Pros:**
- Natural vertical color transition
- Elegant, subtle
- Works with LGP optics (smooth blending)

**Cons:**
- Static (unless animated)
- Requires complementary colors for best effect

---

### Mode 4: SEQUENCED (Temporal Offset)
```
Configuration:
- CH1 = Pattern A at T=0ms
- CH2 = Pattern A at T=150ms (delayed)
- Same pattern, time-shifted

Visual Result (over time):
T=0ms:   Bottom glows first
T=75ms:  Bottom bright, top faint
T=150ms: Top and bottom equal (MIRROR state)
T=225ms: Top bright, bottom fading
T=300ms: Top glows alone

Perception: Rising/falling wave effect

Use Cases:
- Vertical motion illusion
- Breathing effects (rise and fall)
- Attention-grabbing animations
```

**Pros:**
- Creates vertical motion perception
- Elegant temporal dynamics
- Works WITH LGP additive properties

**Cons:**
- More complex to program
- Requires timing parameters

---

### Mode 5: INVERSE (Mirrored Color/Brightness)
```
Configuration:
- CH1 = Fire effect (warm palette)
- CH2 = Fire effect INVERTED (cool palette, or brightness-inverted)

Visual Result:
┌─────────────────────────────────────┐
│ Cool fire (blue flames)             │ ← CH2 (inverted palette)
│ ╔═══════════════════════════════╗   │
│ ║  Neutral/purple flames        ║   │ ← Blend creates balance
│ ╚═══════════════════════════════╝   │
│ Warm fire (red/orange flames)       │ ← CH1 (normal palette)
└─────────────────────────────────────┘

Use Cases:
- Yin-yang effects
- Balanced contrast
- Artistic duality
```

**Pros:**
- Visually striking
- Balanced brightness across surface
- Clear top/bottom differentiation

**Cons:**
- Requires palette-inversion logic
- May be too busy for some contexts

---

## Design Recommendations: Motion + Mode Combinations

### Most Pleasing Visual Effects

**Recommendation 1: MIRROR + CENTER_ORIGIN**
```
Why: Creates radial "bloom" effect
- Both channels expand from center
- Brightness reinforcement
- Clear, intuitive motion
- Works perfectly with LGP additive properties

Example: Power-on animation, success feedback
```

**Recommendation 2: GRADIENT + STATIC**
```
Why: Leverages LGP blending naturally
- Smooth vertical color transition
- No motion complexity
- Elegant ambient effect

Example: Sunrise simulation, temperature display
```

**Recommendation 3: SEQUENCED + LEFT_ORIGIN**
```
Why: Creates rising sweep effect
- Bottom fires first (T=0)
- Top fires 150ms later
- Both sweep L→R
- Perception: Rising wave

Example: Loading progress, level-up animation
```

---

### Most Intuitive User Perception

**For Single Message (e.g., notification):**
- **MIRROR mode** with any motion class
- User sees one unified pattern
- Maximum clarity

**For Dual Information (e.g., status indicators):**
- **SPLIT mode** with complementary colors
- Top = one state, bottom = another
- Clear vertical zoning

**For Ambient/Mood:**
- **GRADIENT mode** with slow color shifts
- Natural, calming
- Background-appropriate

---

### Most Powerful Artistic Control

**Layered Complexity:**
```
SPLIT mode + Different motion classes
- CH1: EDGE_ORIGIN (converging)
- CH2: CENTER_ORIGIN (expanding)
- Creates opposing flow dynamics
- Visually complex but intentional
```

**Temporal Choreography:**
```
SEQUENCED mode + Variable delays
- CH1: Fire at T=0
- CH2: Fire at T=100, 200, 150, 180... (variable)
- Creates organic, unpredictable vertical motion
- Requires timing parameter array
```

**Color Storytelling:**
```
INVERSE mode + Animated palettes
- CH1: Warm fire (anger, energy)
- CH2: Cool fire (calm, control)
- Blend zone = emotional balance
- Can shift over time (palette interpolation)
```

---

## Critical Architectural Decisions

### 1. Motion Class Semantics

**RECOMMENDATION:** Decouple motion classes from physical channel assignment.

**Motion classes should define:**
- Horizontal propagation direction (on the 1D LED strip)
- Timing/sequencing within that strip
- NOT vertical behavior

**Vertical behavior should be defined by:**
- Channel coordination mode (MIRROR, SPLIT, SEQUENCED, etc.)
- Inter-channel timing offsets
- Color palette relationships

**Implementation:**
```c
typedef enum {
    MOTION_STATIC,         // No propagation
    MOTION_LEFT_ORIGIN,    // LED 0 → LED 159
    MOTION_RIGHT_ORIGIN,   // LED 159 → LED 0
    MOTION_CENTER_ORIGIN,  // LED 79-80 → edges
    MOTION_EDGE_ORIGIN,    // LED 0+159 → center
} motion_class_t;

typedef enum {
    CHANNEL_MODE_MIRROR,     // CH1 = CH2 (same pattern)
    CHANNEL_MODE_SPLIT,      // CH1 ≠ CH2 (independent patterns)
    CHANNEL_MODE_GRADIENT,   // CH1 + CH2 = color blend (static)
    CHANNEL_MODE_SEQUENCED,  // CH1 → CH2 (time offset)
    CHANNEL_MODE_INVERSE,    // CH2 = inverse(CH1)
} channel_mode_t;

typedef struct {
    motion_class_t motion;
    channel_mode_t ch_mode;
    uint16_t ch2_delay_ms;  // For SEQUENCED mode
} pattern_config_t;
```

---

### 2. User-Facing Nomenclature

**Avoid technical terms** like "CH1", "CH2", "edge-lit"

**Use perceptual language:**
- "Surface flow" instead of "motion class"
- "Vertical dynamics" instead of "channel mode"
- "Bloom", "sweep", "rise", "pulse" instead of "CENTER_ORIGIN", etc.

**Example User-Facing Options:**
```
Pattern: Fire
Surface Flow: Sweeps left to right
Vertical Style: Unified (bright)

Pattern: Rainbow
Surface Flow: Blooms from center
Vertical Style: Rising (bottom first, then top)

Pattern: Ocean Wave
Surface Flow: Sweeps right to left
Vertical Style: Dual zone (warm bottom, cool top)
```

---

### 3. Default Behaviors

**Safe defaults for new users:**
- MIRROR mode (most intuitive)
- CENTER_ORIGIN or STATIC motion (visually balanced)
- Medium brightness

**Advanced options for power users:**
- SPLIT mode
- SEQUENCED mode with custom delays
- Per-channel pattern assignment

---

## Visual Diagrams: Key Concepts

### Dual Edge-Lit LGP Light Distribution

```
Side View (cross-section):

  Viewer
    ↑
    │
┌───┴───┐ ← LGP surface (what user sees)
│▓▓▓▓▓▓▓│ ← Diffused light output
│░░░░░░░│ ← Internal light propagation
│███████│ ← Acrylic substrate
│░░░░░░░│
│▓▓▓▓▓▓▓│
└───────┘
 ↑     ↑
CH2   CH1
(top) (bottom)

Brightness Profile (perceived):
 100% ┼───┐     ┌───  (edges slightly brighter)
      │   └─────┘      (center slightly dimmer)
   0% └──────────────
     CH2  MID  CH1
```

### Motion Perception Examples

```
MIRROR Mode + CENTER_ORIGIN:
T=0:     ║          (center bright)
T=100:  ─║─         (expanding)
T=200: ──║──        (wider)
T=300: ───║───       (full width)

User sees: Radial bloom (horizontal)


SEQUENCED Mode + LEFT_ORIGIN:
T=0:    │              (bottom left starts)
T=50:   ─│             (bottom sweeps right)
T=100:  ──│            (bottom continues)
        │              (top left starts)
T=150:  ───│           (bottom completes)
        ─│             (top sweeps right)
T=200:  ────           (bottom full)
        ──│            (top continues)
T=250:  ────           (both full)
        ───│

User sees: Rising left-to-right sweep


SPLIT Mode + Different Patterns:
CH1 (bottom): ≈≈≈≈≈≈  (wave, cool colors)
CH2 (top):    ╱╱╱╱╱╱  (fire, warm colors)

User sees:
┌─────────────────┐
│ ╱╱╱ Warm ╱╱╱    │ ← Top-dominant fire
│ ≈≈≈ Purple ≈≈≈  │ ← Blend zone (red+blue)
│ ≈≈≈ Cool ≈≈≈    │ ← Bottom-dominant wave
└─────────────────┘
```

---

## Final Recommendations Summary

### Architecture Decisions

1. **Motion classes = horizontal propagation** (within each channel)
2. **Channel modes = vertical relationships** (between channels)
3. **Timing offsets = vertical motion perception** (SEQUENCED mode)

### Default Configuration

```c
// Recommended default for new patterns:
pattern_config_t default_config = {
    .motion = MOTION_CENTER_ORIGIN,    // Balanced, intuitive
    .ch_mode = CHANNEL_MODE_MIRROR,    // Unified, bright
    .ch2_delay_ms = 0                  // Synchronous
};
```

### Power User Features

- SPLIT mode for independent top/bottom patterns
- SEQUENCED mode with adjustable delays (0-500ms range recommended)
- INVERSE mode for advanced color effects
- Per-channel palette assignment

### User Interface Recommendations

**Simple mode:**
- Pattern selector (Fire, Rainbow, Wave, etc.)
- "Brightness" slider
- "Speed" slider

**Advanced mode:**
- Pattern selector (with per-channel option)
- "Surface flow" dropdown (Left sweep, Right sweep, Center bloom, Edge collapse)
- "Vertical style" dropdown (Unified, Dual zone, Rising, Falling)
- "Rise/fall speed" slider (for SEQUENCED mode)

---

## Conclusion

The dual edge-lit LGP configuration creates a **unified glowing surface** with **additive light blending**, NOT separate top/bottom zones. Motion classes should define **horizontal propagation** within each channel, while **channel coordination modes** define vertical relationships.

The most powerful architectural approach:
- **Motion classes:** Horizontal semantics only
- **Channel modes:** Vertical coordination strategy
- **Timing offsets:** Vertical motion perception
- **User-facing language:** Perceptual, not technical

This approach provides:
- Intuitive defaults (MIRROR mode)
- Powerful artistic control (SPLIT, SEQUENCED, INVERSE modes)
- Clear mental model (horizontal × vertical = 2D surface dynamics)

**Next Steps:**
1. Implement channel_mode_t enum and logic
2. Add ch2_delay_ms timing parameter
3. Create user-facing "Vertical Style" UI options
4. Test with real hardware to validate perceptual predictions
