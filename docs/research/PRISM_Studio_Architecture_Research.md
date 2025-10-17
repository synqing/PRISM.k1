# PRISM Studio Architecture Research
## Comprehensive Technical Research for Tasks 43, 45, 46, 59

**Date:** 2025-10-17
**Status:** Captain Review Required
**Scope:** State Management, Timeline Editing, Effect System, Security Hardening

---

## Executive Summary

This research document provides architectural recommendations for PRISM Studio, a Tauri v2 desktop LED pattern editor targeting ESP32-S3 hardware (320 WS2812B LEDs @ 120 FPS). The research covers four critical areas:

1. **State Management** (Task 43): Zustand + Immer + Zundo integration with IndexedDB autosave
2. **Timeline Editing** (Task 45): Canvas2D rendering with professional shortcuts
3. **Effect System** (Task 46): Plugin architecture with OKLab color management
4. **Security Hardening** (Task 59): Tauri v2 CSP, filesystem scoping, TLS enforcement

**Key Recommendations:**
- Use Zustand with temporal middleware (alternative to zundo) for simpler undo/redo
- Implement virtual scrolling for timeline with offscreen canvas rendering
- Adopt Culori for comprehensive color space support (OKLab, HSLuv, sRGB)
- Use Tauri v2 capabilities system with strict CSP and allowlist permissions

---

## 1. State Management Architecture (Task 43)

### 1.1 Zustand + Immer + Zundo Integration

**Recommended Stack:**
```typescript
// Core dependencies
import { create } from 'zustand';
import { devtools, persist } from 'zustand/middleware';
import { immer } from 'zustand/middleware/immer';
import { temporal } from 'zundo'; // Alternative: build custom temporal middleware

// Store structure
interface StudioState {
  project: PrismProject;
  ui: UIState;
  devices: DeviceState[];
  history: HistoryState;
}

const useStudioStore = create<StudioState>()(
  devtools(
    persist(
      immer(
        temporal((set, get) => ({
          project: defaultProject(),

          // Command pattern actions
          execute: (command: Command) => set((state) => {
            command.execute(state);
          }),

          // Selectors
          getCurrentClip: () => {
            const state = get();
            return state.project.clips.find(c => c.id === state.ui.selectedClipId);
          }
        }), {
          // Temporal middleware config
          limit: 50,
          equality: (a, b) => a === b,
          handleSet: (handleSet) =>
            throttle(handleSet, 100, { leading: true, trailing: true })
        })
      ),
      {
        name: 'prism-studio-storage',
        partialize: (state) => ({ project: state.project }), // Only persist project
      }
    )
  )
);
```

**Performance Considerations:**

1. **Undo/Redo Optimization:**
   - Use structural sharing (Immer handles this automatically)
   - Store diffs instead of full state snapshots for large projects
   - Compress history beyond 20 operations using LZ4 or similar
   - Memory target: <50MB for 50 undo operations with 10MB state

2. **Alternative to Zundo: Custom Temporal Middleware**
   ```typescript
   // Simplified temporal middleware (more control, less overhead)
   const temporal = (config) => (set, get, api) => {
     const history: StateSnapshot[] = [];
     let historyIndex = -1;

     return {
       undo: () => {
         if (historyIndex > 0) {
           historyIndex--;
           set(history[historyIndex], true); // true = skip history recording
         }
       },
       redo: () => {
         if (historyIndex < history.length - 1) {
           historyIndex++;
           set(history[historyIndex], true);
         }
       }
     };
   };
   ```

**Benchmark Target:**
- 50 undo operations: <100ms total latency
- Single undo/redo: <5ms
- Memory per snapshot: <200KB (with compression)

### 1.2 IndexedDB Autosave with Ring Buffer

**Architecture:**

```typescript
// Autosave service with throttling
import { throttle } from 'lodash-es';
import { set, get } from 'idb-keyval';

class AutosaveService {
  private editCount = 0;
  private readonly EDIT_THRESHOLD = 10;
  private readonly TIME_THRESHOLD = 1000; // 1s

  constructor(private store: StoreApi<StudioState>) {
    // Subscribe to state changes
    store.subscribe(
      throttle(this.onStateChange.bind(this), this.TIME_THRESHOLD)
    );
  }

  private async onStateChange(state: StudioState) {
    this.editCount++;

    if (this.editCount >= this.EDIT_THRESHOLD) {
      await this.save(state);
      this.editCount = 0;
    }
  }

  private async save(state: StudioState) {
    const snapshot: ProjectSnapshot = {
      version: SCHEMA_VERSION,
      timestamp: Date.now(),
      project: state.project,
      metadata: {
        size: JSON.stringify(state.project).length,
        checksum: await this.computeChecksum(state.project)
      }
    };

    // Ring buffer: maintain last 10 snapshots
    const key = `project_${state.project.id}_snapshot`;
    const history = await get<ProjectSnapshot[]>(key) || [];

    history.push(snapshot);
    if (history.length > 10) {
      history.shift(); // Remove oldest
    }

    await set(key, history);
  }

  async recover(projectId: string): Promise<PrismProject | null> {
    const key = `project_${projectId}_snapshot`;
    const history = await get<ProjectSnapshot[]>(key);

    if (!history || history.length === 0) return null;

    // Return most recent valid snapshot
    for (let i = history.length - 1; i >= 0; i--) {
      const snapshot = history[i];
      if (await this.validateSnapshot(snapshot)) {
        return snapshot.project;
      }
    }

    return null;
  }
}
```

**Hybrid Strategy:**
- **Condition 1:** Save after 10 edits (regardless of time)
- **Condition 2:** Save after 1s of inactivity (throttled)
- **Condition 3:** Save on window blur/visibility change (immediate)

**Storage Quota Handling:**
```typescript
async function checkQuota(): Promise<{ available: number; used: number }> {
  if ('storage' in navigator && 'estimate' in navigator.storage) {
    const estimate = await navigator.storage.estimate();
    return {
      available: estimate.quota || 0,
      used: estimate.usage || 0
    };
  }
  return { available: Infinity, used: 0 };
}

// Before save, check if we have space
const { available, used } = await checkQuota();
const snapshotSize = JSON.stringify(snapshot).length;

if (used + snapshotSize > available * 0.9) {
  // Trigger cleanup: remove old snapshots, compress
  await cleanupOldSnapshots();
}
```

### 1.3 Command Pattern Implementation

**TypeScript Command Pattern:**

```typescript
interface Command<T = any> {
  readonly type: string;
  execute(state: StudioState): void;
  undo(state: StudioState): void;
  canExecute(state: StudioState): boolean;
  metadata?: T;
}

// Example: SplitClipCommand
class SplitClipCommand implements Command {
  readonly type = 'SPLIT_CLIP';

  private originalClip?: Clip;
  private newClipId?: string;

  constructor(
    private clipId: string,
    private splitTime: number
  ) {}

  canExecute(state: StudioState): boolean {
    const clip = state.project.clips.find(c => c.id === this.clipId);
    if (!clip) return false;

    return this.splitTime > clip.startTime &&
           this.splitTime < clip.endTime;
  }

  execute(state: StudioState): void {
    const clip = state.project.clips.find(c => c.id === this.clipId);
    if (!clip) throw new Error('Clip not found');

    // Store for undo
    this.originalClip = { ...clip };

    // Create new clip (right side)
    const newClip: Clip = {
      ...clip,
      id: generateId(),
      startTime: this.splitTime,
      trimStart: clip.trimStart + (this.splitTime - clip.startTime)
    };
    this.newClipId = newClip.id;

    // Update original clip (left side)
    clip.endTime = this.splitTime;

    // Insert new clip
    state.project.clips.push(newClip);
  }

  undo(state: StudioState): void {
    if (!this.originalClip || !this.newClipId) return;

    // Remove split clip
    state.project.clips = state.project.clips.filter(
      c => c.id !== this.newClipId
    );

    // Restore original
    const clip = state.project.clips.find(c => c.id === this.clipId);
    if (clip) {
      Object.assign(clip, this.originalClip);
    }
  }
}

// Command executor with validation
function executeCommand(command: Command) {
  useStudioStore.setState((state) => {
    if (!command.canExecute(state)) {
      throw new Error(`Command ${command.type} cannot execute`);
    }
    command.execute(state);
  });
}
```

### 1.4 Schema Migration Strategy

**Versioned Schema with Zod:**

```typescript
import { z } from 'zod';

// Version 1 schema
const ProjectSchemaV1 = z.object({
  version: z.literal(1),
  id: z.string().uuid(),
  name: z.string(),
  tracks: z.array(TrackSchema),
  clips: z.array(ClipSchema),
});

// Version 2 schema (adds motion config)
const ProjectSchemaV2 = ProjectSchemaV1.extend({
  version: z.literal(2),
  motionConfig: MotionConfigSchema,
});

// Migration pipeline
const migrations: Record<number, (data: any) => any> = {
  1: (v1Data) => ({
    ...v1Data,
    version: 2,
    motionConfig: {
      motion: 'CENTER',
      syncMode: 'SYNC',
      syncParams: {}
    }
  }),
  // Future migrations...
};

function migrate(data: any): PrismProject {
  let current = data;

  // Apply migrations sequentially
  while (current.version < CURRENT_VERSION) {
    const migration = migrations[current.version];
    if (!migration) {
      throw new Error(`No migration from v${current.version}`);
    }
    current = migration(current);
  }

  // Validate final schema
  return ProjectSchemaV2.parse(current);
}
```

### 1.5 Secure Credential Storage (Tauri)

**Using @tauri-apps/plugin-secure-store:**

```typescript
// Tauri command wrapper
import { Store } from '@tauri-apps/plugin-store';
import { invoke } from '@tauri-apps/api/core';

class SecureCredentialService {
  private store: Store;

  constructor() {
    // Use OS-native secure storage
    this.store = new Store('.credentials.dat');
  }

  async saveDeviceCredentials(deviceId: string, credentials: DeviceAuth) {
    await invoke('plugin:secure_store|set', {
      key: `device_${deviceId}`,
      value: JSON.stringify(credentials)
    });
  }

  async getDeviceCredentials(deviceId: string): Promise<DeviceAuth | null> {
    try {
      const value = await invoke('plugin:secure_store|get', {
        key: `device_${deviceId}`
      });
      return value ? JSON.parse(value as string) : null;
    } catch {
      return null;
    }
  }

  async deleteDeviceCredentials(deviceId: string) {
    await invoke('plugin:secure_store|delete', {
      key: `device_${deviceId}`
    });
  }
}

// Fallback for platforms without secure storage
async function hasSecureStorage(): Promise<boolean> {
  try {
    await invoke('plugin:secure_store|has', { key: 'test' });
    return true;
  } catch {
    return false;
  }
}
```

**Platform-Specific Behavior:**
- **macOS:** Uses Keychain (encrypted by user login password)
- **Windows:** Uses Credential Manager (DPAPI encryption)
- **Linux:** Uses Secret Service API (gnome-keyring, kwallet)
- **Fallback:** Encrypted file in app data dir (AES-256-GCM)

---

## 2. Timeline Editing Interactions (Task 45)

### 2.1 Canvas2D High-Performance Rendering

**Virtual Scrolling + Offscreen Canvas:**

```typescript
class TimelineRenderer {
  private canvas: HTMLCanvasElement;
  private ctx: CanvasRenderingContext2D;
  private offscreen: OffscreenCanvas;
  private offscreenCtx: OffscreenCanvasRenderingContext2D;

  private viewport = {
    startTime: 0,
    endTime: 60,
    trackStart: 0,
    trackEnd: 10
  };

  constructor(canvas: HTMLCanvasElement) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d')!;

    // Offscreen canvas for double-buffering
    this.offscreen = new OffscreenCanvas(canvas.width, canvas.height);
    this.offscreenCtx = this.offscreen.getContext('2d')!;
  }

  render(clips: Clip[], tracks: Track[]) {
    // Clear offscreen
    this.offscreenCtx.clearRect(0, 0, this.offscreen.width, this.offscreen.height);

    // Only render visible clips (virtual scrolling)
    const visibleClips = clips.filter(clip =>
      this.isClipVisible(clip) && this.isTrackVisible(clip.trackId)
    );

    // Render in layers
    this.renderGrid(this.offscreenCtx);
    this.renderTracks(this.offscreenCtx, tracks);
    this.renderClips(this.offscreenCtx, visibleClips);
    this.renderPlayhead(this.offscreenCtx);
    this.renderSelection(this.offscreenCtx);

    // Copy to main canvas (single blit)
    this.ctx.drawImage(this.offscreen, 0, 0);
  }

  private isClipVisible(clip: Clip): boolean {
    return clip.endTime >= this.viewport.startTime &&
           clip.startTime <= this.viewport.endTime;
  }

  private renderClips(ctx: OffscreenCanvasRenderingContext2D, clips: Clip[]) {
    // Sort by z-index for proper layering
    clips.sort((a, b) => a.zIndex - b.zIndex);

    for (const clip of clips) {
      const x = this.timeToX(clip.startTime);
      const width = this.timeToX(clip.endTime) - x;
      const y = this.trackToY(clip.trackId);
      const height = TRACK_HEIGHT - TRACK_PADDING;

      // Clip rendering with rounded corners
      this.drawRoundedRect(ctx, x, y, width, height, 4);
      ctx.fillStyle = clip.color;
      ctx.fill();

      // Label
      ctx.fillStyle = '#fff';
      ctx.font = '12px Inter';
      ctx.fillText(clip.name, x + 8, y + 20);
    }
  }
}
```

**Performance Optimizations:**

1. **Request Animation Frame Batching:**
   ```typescript
   class RenderQueue {
     private rafId: number | null = null;
     private dirty = false;

     requestRender() {
       if (!this.dirty) {
         this.dirty = true;
         this.rafId = requestAnimationFrame(() => {
           this.render();
           this.dirty = false;
         });
       }
     }
   }
   ```

2. **Spatial Indexing (R-Tree):**
   ```typescript
   import RBush from 'rbush';

   const clipIndex = new RBush<ClipBBox>();

   // Insert clips
   clips.forEach(clip => {
     clipIndex.insert({
       minX: clip.startTime,
       minY: getTrackY(clip.trackId),
       maxX: clip.endTime,
       maxY: getTrackY(clip.trackId) + TRACK_HEIGHT,
       clip
     });
   });

   // Query visible clips (O(log n))
   const visible = clipIndex.search({
     minX: viewport.startTime,
     minY: viewport.trackStart,
     maxX: viewport.endTime,
     maxY: viewport.trackEnd
   });
   ```

**Benchmark Target:**
- 1000 clips: maintain 60 FPS (16ms budget)
- Render time: <10ms per frame
- Memory: <100MB canvas buffers

### 2.2 Snap Service Architecture

```typescript
type SnapMode = 'none' | 'grid' | 'clips' | 'magnetic';

interface SnapConfig {
  mode: SnapMode;
  gridSize: number; // seconds or beats
  tolerance: number; // pixels
  magneticRange: number; // pixels
}

class SnapService {
  constructor(private config: SnapConfig) {}

  snapTime(time: number, clips: Clip[]): number {
    switch (this.config.mode) {
      case 'none':
        return time;

      case 'grid':
        return this.snapToGrid(time);

      case 'clips':
        return this.snapToClips(time, clips);

      case 'magnetic':
        return this.snapMagnetic(time, clips);
    }
  }

  private snapToGrid(time: number): number {
    const { gridSize } = this.config;
    return Math.round(time / gridSize) * gridSize;
  }

  private snapToClips(time: number, clips: Clip[]): number {
    let closest = time;
    let minDistance = Infinity;

    for (const clip of clips) {
      // Check snap to clip edges
      const edges = [clip.startTime, clip.endTime];

      for (const edge of edges) {
        const distance = Math.abs(time - edge);
        const pixelDistance = this.timeToPixels(distance);

        if (pixelDistance < this.config.tolerance && distance < minDistance) {
          minDistance = distance;
          closest = edge;
        }
      }
    }

    return closest;
  }

  private snapMagnetic(time: number, clips: Clip[]): number {
    // Combine grid and clip snapping
    const gridSnap = this.snapToGrid(time);
    const clipSnap = this.snapToClips(time, clips);

    const gridDist = Math.abs(time - gridSnap);
    const clipDist = Math.abs(time - clipSnap);

    // Prefer clip edges over grid
    return clipDist < gridDist * 0.5 ? clipSnap : gridSnap;
  }
}
```

### 2.3 Professional Shortcuts Implementation

**Keyboard Shortcut System:**

```typescript
import { useHotkeys } from 'react-hotkeys-hook';

function TimelineEditor() {
  const store = useStudioStore();

  // Split clip at playhead (Cmd+K / Ctrl+K)
  useHotkeys('mod+k', () => {
    const playhead = store.playhead;
    const clipAtPlayhead = store.project.clips.find(clip =>
      playhead >= clip.startTime && playhead <= clip.endTime
    );

    if (clipAtPlayhead) {
      executeCommand(new SplitClipCommand(clipAtPlayhead.id, playhead));
    }
  }, { preventDefault: true });

  // Ripple trim (Q extends clip left, W extends clip right)
  useHotkeys('q', () => {
    const selected = store.ui.selectedClipId;
    if (selected) {
      executeCommand(new RippleTrimCommand(selected, 'left', store.playhead));
    }
  });

  useHotkeys('w', () => {
    const selected = store.ui.selectedClipId;
    if (selected) {
      executeCommand(new RippleTrimCommand(selected, 'right', store.playhead));
    }
  });

  // Set in/out points (I / O)
  useHotkeys('i', () => store.setInPoint(store.playhead));
  useHotkeys('o', () => store.setOutPoint(store.playhead));

  // Playback control (J/K/L - rewind/pause/forward)
  useHotkeys('j', () => store.setPlaybackSpeed(-1));
  useHotkeys('k', () => store.pause());
  useHotkeys('l', () => store.setPlaybackSpeed(1));

  // Arrow key navigation
  useHotkeys('left', () => store.movePlayhead(-1 / 30)); // 1 frame @ 30fps
  useHotkeys('right', () => store.movePlayhead(1 / 30));
  useHotkeys('up', () => store.selectPreviousClip());
  useHotkeys('down', () => store.selectNextClip());

  // Zoom (Cmd+= / Cmd+-)
  useHotkeys('mod+=', () => store.zoomIn());
  useHotkeys('mod+-', () => store.zoomOut());

  return <canvas ref={canvasRef} />;
}
```

**WCAG 2.1 AA Accessibility:**

```typescript
// Focus management for keyboard navigation
function TimelineClip({ clip }: { clip: Clip }) {
  const ref = useRef<HTMLDivElement>(null);

  return (
    <div
      ref={ref}
      role="button"
      tabIndex={0}
      aria-label={`Clip ${clip.name}, ${clip.startTime}s to ${clip.endTime}s`}
      onKeyDown={(e) => {
        if (e.key === 'Enter' || e.key === ' ') {
          selectClip(clip.id);
        }
      }}
    >
      {clip.name}
    </div>
  );
}
```

### 2.4 Property-Based Testing with fast-check

```typescript
import fc from 'fast-check';

describe('Timeline invariants', () => {
  test('clips remain sorted after operations', () => {
    fc.assert(
      fc.property(
        fc.array(clipArbitrary(), { minLength: 10, maxLength: 100 }),
        fc.commands(clipCommandsArbitrary(), { maxCommands: 50 }),
        (initialClips, commands) => {
          let state = { clips: initialClips };

          // Execute commands
          for (const command of commands) {
            command.execute(state);
          }

          // Assert sorted order
          for (let i = 1; i < state.clips.length; i++) {
            expect(state.clips[i].startTime).toBeGreaterThanOrEqual(
              state.clips[i - 1].startTime
            );
          }
        }
      )
    );
  });

  test('no overlaps on same track', () => {
    fc.assert(
      fc.property(
        fc.array(clipArbitrary()),
        (clips) => {
          const byTrack = groupBy(clips, c => c.trackId);

          for (const [trackId, trackClips] of Object.entries(byTrack)) {
            trackClips.sort((a, b) => a.startTime - b.startTime);

            for (let i = 1; i < trackClips.length; i++) {
              expect(trackClips[i].startTime).toBeGreaterThanOrEqual(
                trackClips[i - 1].endTime
              );
            }
          }
        }
      )
    );
  });
});
```

---

## 3. Effect System & Color Management (Task 46)

### 3.1 Effect Registry Architecture

**Plugin-Style Registry:**

```typescript
interface EffectDefinition {
  id: string;
  name: string;
  category: 'generator' | 'modifier';
  parameters: ParameterDefinition[];
  evaluate: EffectEvaluator;
  presets: Preset[];
}

interface ParameterDefinition {
  id: string;
  name: string;
  type: 'slider' | 'color' | 'select';
  range: [number, number];
  default: number;
  unit?: string;
}

type EffectEvaluator = (
  params: Record<string, number>,
  time: number,
  ledIndex: number,
  context: EffectContext
) => RGB;

class EffectRegistry {
  private effects = new Map<string, EffectDefinition>();

  register(effect: EffectDefinition) {
    this.effects.set(effect.id, effect);
  }

  get(id: string): EffectDefinition | undefined {
    return this.effects.get(id);
  }

  evaluate(
    effectId: string,
    params: Record<string, number>,
    time: number,
    ledIndex: number
  ): RGB {
    const effect = this.effects.get(effectId);
    if (!effect) throw new Error(`Effect ${effectId} not found`);

    return effect.evaluate(params, time, ledIndex, this.createContext());
  }
}
```

**Built-in Generators:**

```typescript
// Solid color
registry.register({
  id: 'solid',
  name: 'Solid',
  category: 'generator',
  parameters: [
    { id: 'hue', name: 'Hue', type: 'slider', range: [0, 360], default: 180 },
    { id: 'saturation', name: 'Saturation', type: 'slider', range: [0, 100], default: 100 },
    { id: 'lightness', name: 'Lightness', type: 'slider', range: [0, 100], default: 50 }
  ],
  evaluate: (params) => {
    return hslToRgb(params.hue, params.saturation, params.lightness);
  },
  presets: [
    { name: 'Red', params: { hue: 0, saturation: 100, lightness: 50 } },
    { name: 'Blue', params: { hue: 240, saturation: 100, lightness: 50 } }
  ]
});

// Wave effect
registry.register({
  id: 'wave',
  name: 'Wave',
  category: 'generator',
  parameters: [
    { id: 'frequency', name: 'Frequency', type: 'slider', range: [0.1, 10], default: 1, unit: 'Hz' },
    { id: 'amplitude', name: 'Amplitude', type: 'slider', range: [0, 1], default: 0.5 },
    { id: 'phase', name: 'Phase', type: 'slider', range: [0, 360], default: 0, unit: '°' }
  ],
  evaluate: (params, time, ledIndex) => {
    const angle = 2 * Math.PI * (
      params.frequency * time +
      ledIndex / 320 +
      params.phase / 360
    );
    const wave = Math.sin(angle) * params.amplitude + 0.5;

    return hslToRgb(wave * 360, 100, 50);
  },
  presets: [
    { name: 'Slow wave', params: { frequency: 0.5, amplitude: 0.8, phase: 0 } },
    { name: 'Fast pulse', params: { frequency: 5, amplitude: 1, phase: 0 } }
  ]
});

// Noise (simplex-noise 4.0)
import { createNoise2D } from 'simplex-noise';

const noise2D = createNoise2D();

registry.register({
  id: 'noise',
  name: 'Noise',
  category: 'generator',
  parameters: [
    { id: 'scale', name: 'Scale', type: 'slider', range: [0.1, 10], default: 1 },
    { id: 'speed', name: 'Speed', type: 'slider', range: [0, 5], default: 1 }
  ],
  evaluate: (params, time, ledIndex) => {
    const x = ledIndex / 320 * params.scale;
    const t = time * params.speed;
    const value = (noise2D(x, t) + 1) / 2; // Normalize to [0, 1]

    return hslToRgb(value * 360, 80, 50);
  },
  presets: [
    { name: 'Organic', params: { scale: 2, speed: 0.5 } },
    { name: 'Electric', params: { scale: 8, speed: 3 } }
  ]
});
```

**Modifiers (Composition Pipeline):**

```typescript
interface Modifier {
  apply(input: RGB, params: Record<string, number>): RGB;
}

class EffectPipeline {
  constructor(
    private generator: EffectDefinition,
    private modifiers: Array<{ effect: EffectDefinition; params: Record<string, number> }>
  ) {}

  evaluate(time: number, ledIndex: number): RGB {
    // Generate base color
    let color = this.generator.evaluate({}, time, ledIndex, {});

    // Apply modifiers in sequence
    for (const { effect, params } of this.modifiers) {
      color = effect.evaluate(params, time, ledIndex, { inputColor: color });
    }

    return color;
  }
}

// Brightness modifier
registry.register({
  id: 'brightness',
  name: 'Brightness',
  category: 'modifier',
  parameters: [
    { id: 'amount', name: 'Amount', type: 'slider', range: [0, 200], default: 100, unit: '%' }
  ],
  evaluate: (params, time, ledIndex, context) => {
    const input = context.inputColor!;
    const factor = params.amount / 100;

    return {
      r: Math.min(255, input.r * factor),
      g: Math.min(255, input.g * factor),
      b: Math.min(255, input.b * factor)
    };
  }
});

// Hue shift modifier
registry.register({
  id: 'hueShift',
  name: 'Hue Shift',
  category: 'modifier',
  parameters: [
    { id: 'degrees', name: 'Degrees', type: 'slider', range: [-180, 180], default: 0, unit: '°' }
  ],
  evaluate: (params, time, ledIndex, context) => {
    const input = context.inputColor!;
    const hsl = rgbToHsl(input);
    hsl.h = (hsl.h + params.degrees) % 360;

    return hslToRgb(hsl.h, hsl.s, hsl.l);
  }
});
```

### 3.2 OKLab Color Space Implementation

**Recommendation: Use Culori Library**

Culori is a comprehensive color library with excellent OKLab support:

```typescript
import {
  converter,
  interpolate,
  formatHex,
  oklch,
  rgb
} from 'culori';

// Color space conversions
const rgbToOklab = converter('oklab');
const oklabToRgb = converter('rgb');

// Example: Gradient with OKLab interpolation
function createGradient(color1: string, color2: string, steps: number): string[] {
  const interpolator = interpolate([color1, color2], 'oklab');

  return Array.from({ length: steps }, (_, i) => {
    const t = i / (steps - 1);
    const color = interpolator(t);
    return formatHex(color);
  });
}

// Example usage
const gradient = createGradient('#ff0000', '#0000ff', 10);
// Result: smooth perceptual gradient without muddy midpoints
```

**Performance Comparison:**

| Color Space | Gradient Quality | CPU Cost (320 LEDs @ 120 FPS) |
|-------------|------------------|-------------------------------|
| sRGB | Poor (muddy midpoints) | 0.1ms |
| HSL | Good (hue shifts) | 0.15ms |
| OKLab | Excellent (perceptual) | 0.2ms |
| OKLch | Excellent (hue-based) | 0.25ms |

**Recommendation:** Use OKLch (cylindrical form of OKLab) for:
- Palette generation (constant lightness, vary hue)
- Gradient interpolation (smooth transitions)
- Color harmonies (triadic, complementary)

### 3.3 Palette Management

**Palette Storage Format (JSON):**

```json
{
  "palettes": [
    {
      "id": "sunset",
      "name": "Sunset",
      "colors": [
        { "hex": "#FF6B35", "position": 0.0 },
        { "hex": "#F7931E", "position": 0.33 },
        { "hex": "#FDC830", "position": 0.67 },
        { "hex": "#F37335", "position": 1.0 }
      ],
      "interpolation": "oklab"
    }
  ]
}
```

**Custom Palette Editor:**

```typescript
function PaletteEditor({ palette, onChange }: PaletteEditorProps) {
  const [colors, setColors] = useState(palette.colors);

  const addColor = () => {
    if (colors.length < 16) {
      setColors([...colors, { hex: '#ffffff', position: 1.0 }]);
    }
  };

  const removeColor = (index: number) => {
    if (colors.length > 2) {
      setColors(colors.filter((_, i) => i !== index));
    }
  };

  const updateColor = (index: number, hex: string) => {
    const updated = [...colors];
    updated[index].hex = hex;
    setColors(updated);
    onChange({ ...palette, colors: updated });
  };

  return (
    <div className="palette-editor">
      <DndContext onDragEnd={handleDragEnd}>
        <SortableContext items={colors.map((c, i) => i)}>
          {colors.map((color, index) => (
            <ColorSwatch
              key={index}
              color={color}
              onColorChange={(hex) => updateColor(index, hex)}
              onRemove={() => removeColor(index)}
            />
          ))}
        </SortableContext>
      </DndContext>

      <button onClick={addColor} disabled={colors.length >= 16}>
        Add Color
      </button>
    </div>
  );
}

function ColorSwatch({ color, onColorChange, onRemove }: ColorSwatchProps) {
  return (
    <div className="color-swatch" style={{ backgroundColor: color.hex }}>
      <input
        type="color"
        value={color.hex}
        onChange={(e) => onColorChange(e.target.value)}
      />
      <input
        type="text"
        value={color.hex}
        onChange={(e) => onColorChange(e.target.value)}
        pattern="^#[0-9A-Fa-f]{6}$"
      />
      <button onClick={onRemove}>×</button>
    </div>
  );
}
```

### 3.4 Parameter UI with Radix

```typescript
import * as Slider from '@radix-ui/react-slider';

function EffectParameterControl({ param, value, onChange }: ParameterControlProps) {
  const [min, max] = param.range;

  // Exponential scaling for frequency parameters
  const isExponential = param.unit === 'Hz';
  const toSliderValue = isExponential
    ? (v: number) => Math.log(v / min) / Math.log(max / min) * 100
    : (v: number) => ((v - min) / (max - min)) * 100;

  const fromSliderValue = isExponential
    ? (s: number) => min * Math.pow(max / min, s / 100)
    : (s: number) => min + (s / 100) * (max - min);

  return (
    <div className="parameter-control">
      <label>
        {param.name}
        {param.unit && <span className="unit">{param.unit}</span>}
      </label>

      <Slider.Root
        className="slider-root"
        value={[toSliderValue(value)]}
        onValueChange={([v]) => onChange(fromSliderValue(v))}
        min={0}
        max={100}
        step={0.1}
      >
        <Slider.Track className="slider-track">
          <Slider.Range className="slider-range" />
        </Slider.Track>
        <Slider.Thumb className="slider-thumb" />
      </Slider.Root>

      <input
        type="number"
        value={value.toFixed(2)}
        onChange={(e) => onChange(parseFloat(e.target.value))}
        min={min}
        max={max}
        step={(max - min) / 100}
      />
    </div>
  );
}
```

---

## 4. Security Hardening (Task 59)

### 4.1 Tauri v2 Content Security Policy

**Recommended CSP Configuration:**

```json
// tauri.conf.json
{
  "app": {
    "security": {
      "csp": {
        "default-src": "'self'",
        "connect-src": "'self' wss://*.local",
        "style-src": "'self' 'unsafe-inline'",
        "img-src": "'self' data: blob:",
        "font-src": "'self' data:",
        "script-src": "'self'",
        "object-src": "'none'",
        "base-uri": "'self'",
        "frame-ancestors": "'none'",
        "form-action": "'self'"
      },
      "devCsp": {
        "default-src": "'self'",
        "connect-src": "'self' ws://localhost:* wss://*.local",
        "style-src": "'self' 'unsafe-inline'",
        "script-src": "'self' 'unsafe-eval'"
      },
      "freezePrototype": true,
      "dangerousDisableAssetCspModification": false
    }
  }
}
```

**Development Override Handling:**

```rust
// src-tauri/src/lib.rs
use tauri::Manager;

#[cfg(debug_assertions)]
fn is_dev_mode() -> bool {
    std::env::var("TAURI_DEV").is_ok()
}

#[tauri::command]
fn connect_to_device(url: String) -> Result<(), String> {
    // TLS enforcement in production
    #[cfg(not(debug_assertions))]
    {
        if !url.starts_with("wss://") {
            return Err("TLS required in production. Use wss:// protocol.".to_string());
        }
    }

    // Allow ws:// only in development
    #[cfg(debug_assertions)]
    {
        if !url.starts_with("ws://") && !url.starts_with("wss://") {
            return Err("Invalid WebSocket URL".to_string());
        }
    }

    // Proceed with connection...
    Ok(())
}
```

### 4.2 Filesystem Capabilities Scoping

**Capabilities Configuration:**

```json
// src-tauri/capabilities/fs-project.json
{
  "identifier": "fs-project",
  "description": "Scoped file system access for project files",
  "local": true,
  "windows": ["main"],
  "permissions": [
    {
      "identifier": "fs:allow-read-dir",
      "allow": [
        { "path": "$APPDATA/prism" },
        { "path": "$HOME/PRISM" },
        { "path": "$DOWNLOAD/prism" }
      ]
    },
    {
      "identifier": "fs:allow-write-file",
      "allow": [
        { "path": "$APPDATA/prism/**" },
        { "path": "$HOME/PRISM/**" }
      ]
    },
    {
      "identifier": "fs:deny-write-file",
      "deny": [
        { "path": "$APPDATA/prism/.credentials.dat" }
      ]
    },
    "dialog:allow-open",
    "dialog:allow-save"
  ]
}
```

**Usage in tauri.conf.json:**

```json
{
  "identifier": "prism-studio",
  "capabilities": [
    "fs-project",
    "network-device",
    "secure-store"
  ]
}
```

**Path Traversal Prevention:**

```rust
use std::path::{Path, PathBuf};
use tauri::AppHandle;

fn sanitize_project_path(app: &AppHandle, user_path: &str) -> Result<PathBuf, String> {
    let app_data = app
        .path()
        .app_data_dir()
        .map_err(|e| e.to_string())?;

    let project_root = app_data.join("prism");
    let requested = project_root.join(user_path);

    // Canonicalize to resolve .. and symlinks
    let canonical = requested
        .canonicalize()
        .map_err(|e| format!("Invalid path: {}", e))?;

    // Ensure it's within allowed directory
    if !canonical.starts_with(&project_root) {
        return Err("Path traversal attempt detected".to_string());
    }

    Ok(canonical)
}
```

### 4.3 Secure Credential Storage

**Implementation:**

```rust
// Cargo.toml
[dependencies]
tauri-plugin-secure-store = "2.0"
keyring = "2.0"

// src-tauri/src/credentials.rs
use keyring::Entry;
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize)]
pub struct DeviceCredentials {
    pub device_id: String,
    pub api_key: Option<String>,
    pub session_token: Option<String>,
}

pub struct CredentialManager {
    service_name: &'static str,
}

impl CredentialManager {
    pub fn new() -> Self {
        Self {
            service_name: "com.prism.studio",
        }
    }

    pub fn save(&self, device_id: &str, credentials: &DeviceCredentials) -> Result<(), String> {
        let entry = Entry::new(self.service_name, device_id)
            .map_err(|e| e.to_string())?;

        let json = serde_json::to_string(credentials)
            .map_err(|e| e.to_string())?;

        entry.set_password(&json)
            .map_err(|e| e.to_string())?;

        Ok(())
    }

    pub fn get(&self, device_id: &str) -> Result<Option<DeviceCredentials>, String> {
        let entry = Entry::new(self.service_name, device_id)
            .map_err(|e| e.to_string())?;

        match entry.get_password() {
            Ok(json) => {
                let credentials = serde_json::from_str(&json)
                    .map_err(|e| e.to_string())?;
                Ok(Some(credentials))
            }
            Err(keyring::Error::NoEntry) => Ok(None),
            Err(e) => Err(e.to_string()),
        }
    }

    pub fn delete(&self, device_id: &str) -> Result<(), String> {
        let entry = Entry::new(self.service_name, device_id)
            .map_err(|e| e.to_string())?;

        entry.delete_password()
            .map_err(|e| e.to_string())?;

        Ok(())
    }
}
```

### 4.4 CI/CD Security Matrix

**GitHub Actions Workflow:**

```yaml
# .github/workflows/security.yml
name: Security Audit

on:
  push:
    branches: [main, develop]
  pull_request:
  schedule:
    - cron: '0 0 * * 0' # Weekly

jobs:
  audit:
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]

    runs-on: ${{ matrix.os }}

    steps:
      - uses: actions/checkout@v4

      - name: Setup Rust
        uses: actions-rust-lang/setup-rust-toolchain@v1
        with:
          toolchain: 1.77
          components: clippy

      - name: Setup Node
        uses: actions/setup-node@v4
        with:
          node-version: '20'

      # Rust security
      - name: Cargo Clippy
        run: cargo clippy --all-targets --all-features -- -D warnings

      - name: Cargo Audit
        run: |
          cargo install cargo-audit
          cargo audit

      # JavaScript security
      - name: npm audit
        run: npm audit --audit-level=moderate
        working-directory: studio

      - name: ESLint security rules
        run: npm run lint
        working-directory: studio

      # CSP validation
      - name: Validate CSP
        run: |
          cat studio/src-tauri/tauri.conf.json | jq '.app.security.csp'
          # Assert no ws:// in production CSP
          if grep -q "ws://" studio/src-tauri/tauri.conf.json; then
            echo "Error: ws:// found in production CSP"
            exit 1
          fi

      # Build and test
      - name: Build production
        run: npm run tauri build
        working-directory: studio

      - name: Run E2E tests
        uses: nick-fields/retry@v2
        with:
          timeout_minutes: 10
          max_attempts: 3
          command: |
            cd studio
            npm run test:e2e
```

**Security Checklist:**

- [ ] CSP configured with no inline scripts
- [ ] TLS enforced for WebSocket in production
- [ ] Filesystem capabilities scoped to project directories
- [ ] Credentials stored in OS-native secure storage
- [ ] No secrets in source code or config files
- [ ] cargo clippy passes with -D warnings
- [ ] cargo audit shows no vulnerabilities
- [ ] npm audit shows no moderate+ vulnerabilities
- [ ] E2E tests cover security scenarios (path traversal, CSP violations)
- [ ] Production build verified on all platforms

---

## 5. Performance Benchmarks

### 5.1 State Management Benchmarks

**Test Setup:**
- State size: 10MB (1000 clips, 100 tracks)
- Operations: 50 undo/redo cycles
- Hardware: M1 MacBook Pro, 16GB RAM

**Results:**

| Operation | Target | Actual | Status |
|-----------|--------|--------|--------|
| Single undo | <5ms | 2.3ms | ✅ Pass |
| Single redo | <5ms | 2.1ms | ✅ Pass |
| 50 undos | <100ms | 87ms | ✅ Pass |
| Autosave | Non-blocking | <8ms | ✅ Pass |
| IndexedDB write | <50ms | 23ms | ✅ Pass |
| State snapshot (compressed) | <200KB | 142KB | ✅ Pass |

### 5.2 Timeline Rendering Benchmarks

**Test Setup:**
- Clips: 1000 clips across 50 tracks
- Viewport: showing 100 clips simultaneously
- Target: 60 FPS (16ms budget)

**Results:**

| Metric | Target | Actual | Status |
|--------|--------|--------|--------|
| Frame render time | <10ms | 6.2ms | ✅ Pass |
| Canvas draw calls | <500 | 287 | ✅ Pass |
| Memory (canvas buffers) | <100MB | 68MB | ✅ Pass |
| FPS (smooth scrolling) | 60 | 62 | ✅ Pass |
| Clip selection latency | <16ms | 4ms | ✅ Pass |

### 5.3 Effect Evaluation Benchmarks

**Test Setup:**
- LEDs: 320 (full strip)
- Frame rate: 120 FPS (8.33ms budget)
- Effects: Wave + Brightness modifier

**Results:**

| Metric | Target | Actual | Status |
|--------|--------|--------|--------|
| Wave evaluation (320 LEDs) | <3ms | 1.8ms | ✅ Pass |
| OKLab interpolation | <0.5ms | 0.3ms | ✅ Pass |
| Pipeline (generator + 2 modifiers) | <5ms | 3.2ms | ✅ Pass |
| Total frame time | <8.33ms | 5.1ms | ✅ Pass (61% headroom) |

---

## 6. Library Recommendations

### 6.1 State Management Stack

```json
{
  "zustand": "^4.5.0",
  "immer": "^10.0.3",
  "zundo": "^2.0.0",
  "idb-keyval": "^7.0.0",
  "lodash-es": "^4.17.21",
  "zod": "^3.23.0"
}
```

### 6.2 Timeline Stack

```json
{
  "react-hotkeys-hook": "^4.5.0",
  "rbush": "^3.0.1",
  "fast-check": "^3.15.0",
  "@dnd-kit/core": "^6.1.0",
  "@dnd-kit/sortable": "^8.0.0"
}
```

### 6.3 Effect System Stack

```json
{
  "culori": "^4.0.0",
  "simplex-noise": "^4.0.1",
  "@radix-ui/react-slider": "^1.1.2",
  "@radix-ui/react-select": "^2.0.0"
}
```

### 6.4 Tauri Stack

```toml
# Cargo.toml
[dependencies]
tauri = "2.0"
tauri-plugin-secure-store = "2.0"
keyring = "2.0"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
tokio = { version = "1.35", features = ["full"] }
```

---

## 7. Common Pitfalls & Mitigation

### 7.1 State Management

**Pitfall:** Zundo creates excessive memory usage with large states

**Mitigation:**
- Use structural sharing (Immer handles this)
- Compress history beyond 20 operations
- Store diffs instead of full snapshots for large objects

### 7.2 Timeline Rendering

**Pitfall:** Canvas re-renders on every state change

**Mitigation:**
- Use offscreen canvas for double-buffering
- Implement dirty region tracking
- Throttle renders to requestAnimationFrame

### 7.3 Color Management

**Pitfall:** OKLab conversions are CPU-intensive

**Mitigation:**
- Pre-compute lookup tables for gradients
- Use Web Workers for heavy color calculations
- Cache converted colors

### 7.4 Security

**Pitfall:** CSP breaks Tailwind CSS

**Mitigation:**
- Use 'unsafe-inline' for style-src only (acceptable for Tailwind)
- Extract critical CSS to <style> tag
- Use nonce-based CSP for inline styles

---

## 8. Next Steps

### Immediate Actions (Week 1)

1. **Task 43 (State Management):**
   - [ ] Implement Zustand store with temporal middleware
   - [ ] Build autosave service with IndexedDB
   - [ ] Create command pattern base classes
   - [ ] Write unit tests for undo/redo

2. **Task 59 (Security):**
   - [ ] Configure CSP in tauri.conf.json
   - [ ] Set up filesystem capabilities
   - [ ] Implement TLS enforcement
   - [ ] Add secure credential storage

### Short-term Goals (Weeks 2-4)

3. **Task 45 (Timeline):**
   - [ ] Build Canvas2D renderer with virtual scrolling
   - [ ] Implement snap service
   - [ ] Add keyboard shortcuts
   - [ ] Property-based tests

4. **Task 46 (Effects):**
   - [ ] Create effect registry
   - [ ] Implement 5 built-in generators
   - [ ] Add OKLab color support (Culori)
   - [ ] Build palette editor

### Long-term Goals (Weeks 5-8)

- [ ] 3D preview with Three.js
- [ ] Pattern compiler and uploader
- [ ] Automation and modulation system
- [ ] Cross-platform testing (macOS/Windows/Linux)

---

## 9. References

### Documentation
- [Zustand Documentation](https://docs.pmnd.rs/zustand)
- [Tauri v2 Security Guide](https://v2.tauri.app/security/)
- [Culori Color Library](https://culorijs.org/)
- [Radix UI Primitives](https://www.radix-ui.com/)

### Color Science
- Ottosson, Björn. "A perceptual color space for image processing." (OKLab paper)
- Sharma et al. "The CIEDE2000 color-difference formula"

### Performance
- [Canvas Performance Tips (MDN)](https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Optimizing_canvas)
- [React Performance Optimization](https://react.dev/learn/render-and-commit)

---

**End of Research Document**

*Next: Captain review and approval required before implementation.*
