# K1-Lightwave Animation System

## 🎯 Overview

Complete temporal sequencing animation system for K1-Lightwave dual-edge LGP displays. Implements physically accurate light propagation with real-time editing capabilities.

---

## 🏗️ Architecture

### Core Components

1. **K1AnimationEngine.ts** - Physics engine
   - Temporal sequencing calculations
   - LGP surface color blending (additive)
   - 6 pattern generators (bloom, wave, chase, triangle, dual-zone, breathe)
   - Preset configurations

2. **K1DeviceAnimated.tsx** - Animated device component
   - Canvas-based rendering (pixel-perfect)
   - Real-time frame generation
   - External/internal time control
   - Dual-edge light simulation

3. **AnimationPlayer.tsx** - Playback controls
   - Timeline scrubbing
   - Play/pause/reset
   - Speed control (1-255)
   - Pattern switching

4. **LivePatternEditor.tsx** - Real-time editor
   - Color palette editor (CH1/CH2)
   - Motion parameters (origin, sync mode)
   - Temporal offset control (0-300ms)
   - Phi phenomenon guidance

5. **PatternStudio.tsx** - Full production environment
   - 3-panel layout (editor | preview | export)
   - Live preview updates
   - Export to .prism format

---

## 🌊 Animation Patterns

### 1. Radial Bloom
- **Description**: Light expands from center outward
- **Sync**: Synchronized (both edges identical)
- **Use case**: Ambient backgrounds, breathing effects
- **Preset colors**: Warm bottom (orange/red), Cool top (purple/blue)

### 2. Rising Wave
- **Description**: Horizontal sweep with temporal offset
- **Sync**: Offset (150ms delay between top/bottom)
- **Use case**: Directional indicators, progress animations
- **Key feature**: Creates perceived upward motion via phi phenomenon

### 3. Horizontal Chase
- **Description**: Sequential lobes moving left-to-right
- **Sync**: Synchronized or offset
- **Use case**: Loading indicators, marquee effects
- **Preset**: 4 lobes per edge, 0.12 radius

### 4. Triangle Shape
- **Description**: Progressive delay creates geometric shapes
- **Sync**: Progressive (0-20% delay gradient)
- **Use case**: Dynamic transitions, geometric art
- **Key feature**: Diagonal edge from temporal sequencing

### 5. Dual Zone
- **Description**: Independent patterns on each channel
- **Sync**: Split (CH1 = fire, CH2 = wave)
- **Use case**: Contrasting effects, temperature visualization
- **Blending**: Purple/teal in mid-panel additive zone

### 6. Breathe
- **Description**: All lobes pulse in sync
- **Sync**: Synchronized
- **Use case**: Idle states, sleep mode, notifications
- **Timing**: Slow cycle (60 speed default)

---

## 🎨 LGP Physics Model

### Brightness Distribution

```
Vertical Profile:
100% ┼───┐     ┌───  ← Edges (top/bottom)
     │   └─────┘     ← Center (softer)
  0% └──────────────
    TOP   MID  BOTTOM
```

Formula: `influence = pow(distance_from_edge, 1.5) * 1.2`

### Additive Blending

Bottom (CH1) and top (CH2) lights blend in mid-panel:

```
Surface color = Σ(CH1_lobes * bottom_influence) + 
                Σ(CH2_lobes * top_influence)
```

Each lobe contributes via Gaussian falloff:
```
contribution = exp(-distance² / radius²) * intensity
```

---

## ⚡ Temporal Sequencing

### Phi Phenomenon Range

**Optimal delays**: 60-150ms  
Creates perceived geometric shapes:
- **80ms**: Quick, snappy triangles
- **120ms**: Medium, smooth diamonds  
- **180ms**: Slow, flowing waves

### Sync Modes

1. **Synchronized** - Top and bottom fire together (offset = 0)
2. **Offset** - Fixed delay between channels (60-300ms)
3. **Progressive** - Delay varies across horizontal axis (creates diagonals)
4. **Split** - Independent patterns per channel

---

## 🎯 Usage Examples

### Basic Animation

```tsx
<K1DeviceAnimated
  heightVariant="h60"
  pattern="risingWave"
  isPlaying={true}
/>
```

### Custom Configuration

```tsx
const customConfig = {
  pattern: 'radialBloom',
  speed: 150,
  syncMode: 'offset',
  motionOrigin: 'center',
  ch1Colors: ['#FF4500', '#FFA500'],
  ch2Colors: ['#00CED1', '#7B68EE'],
  temporalOffset: 120
};

<K1DeviceAnimated
  pattern="radialBloom"
  config={customConfig}
/>
```

### With Timeline Control

```tsx
const [time, setTime] = useState(0);
const [playing, setPlaying] = useState(true);

<K1DeviceAnimated
  pattern="horizontalChase"
  isPlaying={playing}
  currentTime={time}
  onTimeUpdate={setTime}
/>

<AnimationPlayer
  pattern="horizontalChase"
  isPlaying={playing}
  onPlayPause={() => setPlaying(!playing)}
  currentTime={time}
  onTimeChange={setTime}
  duration={4000}
/>
```

---

## 🔧 Technical Specifications

### Performance

- **Rendering**: Canvas 2D (pixel-by-pixel)
- **Resolution**: 1320×240px @ 1px = 0.25mm
- **Target FPS**: 60fps (30fps acceptable during scrubbing)
- **Frame generation**: <5ms per frame
- **Blur simulation**: 8px CSS filter (frosted acrylic effect)

### LED Configuration

- **CH1 (Bottom)**: 160 LEDs along bottom edge
- **CH2 (Top)**: 160 LEDs along top edge
- **Total**: 320 RGB LEDs (WS2812B)
- **Hidden**: LEDs not visible to user, only diffused surface

### Export Format

- **Duration**: 4 seconds @ 60fps = 240 frames
- **Size**: ~42 KB (estimated .prism binary)
- **Compression**: Frame-based with delta encoding
- **Storage**: Fits K1 device (256 KB total capacity)

---

## 📐 Design Tokens

### Color Palettes

**Warm (CH1 Bottom)**:
- `#FF4500` - Red-Orange
- `#FF6B35` - Orange
- `#FFA500` - Golden Orange
- `#FFC64D` - Yellow-Orange

**Cool (CH2 Top)**:
- `#7B68EE` - Medium Slate Blue
- `#5B8CFF` - Cornflower Blue
- `#00CED1` - Dark Turquoise
- `#1E90FF` - Dodger Blue

**Blended (Mid-Panel)**:
- `#A06BFF` - Purple (warm+cool)
- `#7AD0FF` - Light Sky Blue
- `#6BE6C1` - Turquoise (additive)

### Timing Presets

- **Micro**: 80ms (quick geometric shapes)
- **Fast**: 120ms (optimal phi phenomenon)
- **Base**: 150ms (smooth waves)
- **Slow**: 180-200ms (flowing motion)

---

## 🚀 Next Steps

### To Complete Prototype:

1. ✅ **Animation System** - DONE
2. ✅ **Live Pattern Editor** - DONE
3. ✅ **Timeline Player** - DONE
4. ⏳ **Keyboard Shortcuts** - Space/J/K/L playback controls
5. ⏳ **Pattern Compiler** - Generate .prism binary from timeline
6. ⏳ **Device Sync** - Upload patterns via WebSocket
7. ⏳ **Pattern Library** - Save/load from IndexedDB
8. ⏳ **Export System** - PNG thumbnails, .prism files
9. ⏳ **Cue List Manager** - Multi-pattern sequencing
10. ⏳ **Mobile View** - Touch-friendly timeline

### Suggested Priorities:

**High**: Keyboard shortcuts, Pattern compiler, Device sync  
**Medium**: Library system, Export  
**Low**: Cue list, Mobile responsive

---

## 📚 References

- **ADR-010**: K1 firmware protocol specification
- **Phi Phenomenon**: 60-150ms optimal range for motion perception
- **LGP Physics**: Total internal reflection + extraction features
- **Temporal Sequencing**: Audi dynamic turn signal inspiration

---

Built with React, Canvas 2D, and lots of ✨ temporal magic ✨
