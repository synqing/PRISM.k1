/**
 * K1-Lightwave Animation Engine
 * Handles temporal sequencing and LGP light propagation
 */

export type AnimationPattern = 
  | 'radialBloom'
  | 'risingWave'
  | 'horizontalChase'
  | 'triangleShape'
  | 'dualZone'
  | 'breathe';

// ADR-010: Motion Direction Enum
export type MotionDirection = 
  | 'left'      // LED 0 → 159 (left-to-right sweep)
  | 'right'     // LED 159 → 0 (right-to-left sweep)
  | 'center'    // LEDs 79-80 → edges (radial bloom)
  | 'edge'      // Edges → center (radial collapse)
  | 'static';   // No propagation (solid/ambient)

// ADR-010: Sync Mode Enum
export type SyncMode = 
  | 'sync'        // Both edges fire simultaneously
  | 'offset'      // CH2 delayed by fixed time
  | 'progressive' // Delay varies linearly (triangles/wedges)
  | 'wave'        // Sinusoidal delay pattern
  | 'custom';     // Per-LED timing map

export type MotionOrigin = 'center' | 'left' | 'right' | 'bottom' | 'top';

export type WaveformType = 'sin' | 'triangle' | 'sawtooth';

export interface AnimationConfig {
  pattern: AnimationPattern;
  speed: number;           // 1-255, affects propagation speed
  
  // ADR-010: Motion Architecture
  motionDirection: MotionDirection;
  syncMode: SyncMode;
  
  // Sync Mode Parameters
  offsetDelayMs: number;          // OFFSET: fixed delay (0-500ms)
  progressiveStartMs: number;     // PROGRESSIVE: start delay (0-500ms)
  progressiveEndMs: number;       // PROGRESSIVE: end delay (0-500ms)
  waveAmplitudeMs: number;        // WAVE: amplitude (0-500ms)
  wavePeriod: number;             // WAVE: cycles (0.5-4.0)
  wavePhaseDeg: number;           // WAVE: phase (0-359°)
  waveform: WaveformType;         // WAVE: waveform type
  customDelayMap: number[];       // CUSTOM: 160 delay values
  
  // Legacy (for backward compatibility)
  motionOrigin: MotionOrigin;
  
  // Colors
  ch1Colors: string[];     // Bottom edge colors
  ch2Colors: string[];     // Top edge colors
  
  // Legacy temporal offset
  temporalOffset: number;  // ms delay between top/bottom (60-180ms)
}

export interface LobeState {
  position: number;        // 0.0 (left) to 1.0 (right)
  intensity: number;       // 0.0 to 1.0
  radius: number;          // Size of bloom
  color: { r: number; g: number; b: number };
}

export interface ChannelState {
  lobes: LobeState[];
}

export interface AnimationFrame {
  timestamp: number;       // ms
  ch1: ChannelState;       // Bottom edge
  ch2: ChannelState;       // Top edge
}

/**
 * Calculate surface color at given position using LGP physics
 */
export function calculateSurfaceColor(
  positionX: number,        // 0.0 (left) to 1.0 (right)
  positionY: number,        // 0.0 (bottom) to 1.0 (top)
  ch1Lobes: LobeState[],    // Bottom edge lobes
  ch2Lobes: LobeState[]     // Top edge lobes
): { r: number; g: number; b: number } {
  // Brightness falloff from edges (edges ~20% brighter than center)
  const bottomInfluence = Math.pow(1.0 - positionY, 1.5) * 1.2;
  const topInfluence = Math.pow(positionY, 1.5) * 1.2;
  
  let r = 0, g = 0, b = 0;
  
  // Accumulate contributions from bottom lobes
  for (const lobe of ch1Lobes) {
    const distance = Math.abs(positionX - lobe.position);
    const contribution = Math.exp(-distance * distance / (lobe.radius * lobe.radius)) * lobe.intensity;
    const weight = contribution * bottomInfluence;
    
    r += lobe.color.r * weight;
    g += lobe.color.g * weight;
    b += lobe.color.b * weight;
  }
  
  // Accumulate contributions from top lobes
  for (const lobe of ch2Lobes) {
    const distance = Math.abs(positionX - lobe.position);
    const contribution = Math.exp(-distance * distance / (lobe.radius * lobe.radius)) * lobe.intensity;
    const weight = contribution * topInfluence;
    
    r += lobe.color.r * weight;
    g += lobe.color.g * weight;
    b += lobe.color.b * weight;
  }
  
  // Clamp to valid range
  return {
    r: Math.min(1.0, r),
    g: Math.min(1.0, g),
    b: Math.min(1.0, b)
  };
}

/**
 * Parse hex color to RGB
 */
function hexToRgb(hex: string): { r: number; g: number; b: number } {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  return result ? {
    r: parseInt(result[1], 16) / 255,
    g: parseInt(result[2], 16) / 255,
    b: parseInt(result[3], 16) / 255
  } : { r: 1, g: 1, b: 1 };
}

/**
 * Generate animation frame for given timestamp
 */
export function generateAnimationFrame(
  config: AnimationConfig,
  timestamp: number // ms since animation start
): AnimationFrame {
  const { pattern, speed, syncMode, motionOrigin, ch1Colors, ch2Colors, temporalOffset } = config;
  
  // Speed affects cycle duration (higher = faster)
  // Speed 100 = ~1.5s cycle, Speed 200 = ~0.75s cycle
  const cycleDuration = 2000 / (speed / 100);
  const normalizedTime = (timestamp % cycleDuration) / cycleDuration; // 0.0 to 1.0
  
  let ch1: ChannelState = { lobes: [] };
  let ch2: ChannelState = { lobes: [] };
  
  switch (pattern) {
    case 'radialBloom':
      ch1 = generateRadialBloom(normalizedTime, ch1Colors, 0);
      ch2 = syncMode === 'synchronized' 
        ? generateRadialBloom(normalizedTime, ch2Colors, 0)
        : generateRadialBloom(normalizedTime, ch2Colors, temporalOffset / cycleDuration);
      break;
      
    case 'risingWave':
      // Bottom leads, top follows with offset
      ch1 = generateHorizontalWave(normalizedTime, ch1Colors, motionOrigin, 0);
      ch2 = generateHorizontalWave(normalizedTime, ch2Colors, motionOrigin, temporalOffset / cycleDuration);
      break;
      
    case 'horizontalChase':
      ch1 = generateHorizontalChase(normalizedTime, ch1Colors, motionOrigin);
      ch2 = syncMode === 'synchronized'
        ? generateHorizontalChase(normalizedTime, ch2Colors, motionOrigin)
        : generateHorizontalChase(normalizedTime, ch2Colors, motionOrigin, temporalOffset / cycleDuration);
      break;
      
    case 'triangleShape':
      // Progressive delay creates triangle
      ch1 = generateProgressiveWave(normalizedTime, ch1Colors, 0);
      ch2 = generateProgressiveWave(normalizedTime, ch2Colors, 0.2); // 20% max delay
      break;
      
    case 'dualZone':
      // Independent patterns on each channel
      ch1 = generateFireEffect(normalizedTime, ch1Colors);
      ch2 = generateWaveEffect(normalizedTime, ch2Colors);
      break;
      
    case 'breathe':
      ch1 = generateBreathe(normalizedTime, ch1Colors);
      ch2 = generateBreathe(normalizedTime, ch2Colors);
      break;
  }
  
  return { timestamp, ch1, ch2 };
}

/**
 * Pattern: Radial Bloom (expands from center)
 */
function generateRadialBloom(
  time: number,
  colors: string[],
  phaseOffset: number
): ChannelState {
  const t = (time + phaseOffset) % 1.0;
  const expansion = Math.sin(t * Math.PI); // 0 → 1 → 0
  
  return {
    lobes: [{
      position: 0.5, // Center
      intensity: 0.8 + expansion * 0.2,
      radius: 0.2 + expansion * 0.3,
      color: hexToRgb(colors[0] || '#FFFFFF')
    }]
  };
}

/**
 * Pattern: Horizontal Wave (sweeps left to right)
 */
function generateHorizontalWave(
  time: number,
  colors: string[],
  origin: MotionOrigin,
  phaseOffset: number
): ChannelState {
  const t = (time + phaseOffset) % 1.0;
  const position = origin === 'left' ? t : 1.0 - t;
  
  return {
    lobes: colors.map((color, i) => ({
      position: (position + i * 0.25) % 1.0,
      intensity: 0.9,
      radius: 0.15,
      color: hexToRgb(color)
    }))
  };
}

/**
 * Pattern: Horizontal Chase (sequential lobes)
 */
function generateHorizontalChase(
  time: number,
  colors: string[],
  origin: MotionOrigin,
  phaseOffset: number = 0
): ChannelState {
  const t = (time + phaseOffset) % 1.0;
  const numLobes = 4;
  
  return {
    lobes: Array.from({ length: numLobes }, (_, i) => {
      const lobePhase = (t + i / numLobes) % 1.0;
      const position = origin === 'left' ? lobePhase : 1.0 - lobePhase;
      
      return {
        position,
        intensity: 0.7 + Math.sin(lobePhase * Math.PI) * 0.3,
        radius: 0.12,
        color: hexToRgb(colors[i % colors.length])
      };
    })
  };
}

/**
 * Pattern: Progressive Wave (creates triangle shape)
 */
function generateProgressiveWave(
  time: number,
  colors: string[],
  maxDelay: number
): ChannelState {
  const numLobes = 8;
  
  return {
    lobes: Array.from({ length: numLobes }, (_, i) => {
      const delay = (i / numLobes) * maxDelay;
      const t = (time - delay + 1.0) % 1.0;
      
      return {
        position: i / numLobes,
        intensity: 0.6 + Math.sin(t * Math.PI * 2) * 0.4,
        radius: 0.08,
        color: hexToRgb(colors[i % colors.length])
      };
    })
  };
}

/**
 * Pattern: Fire Effect (flickering warm colors)
 */
function generateFireEffect(
  time: number,
  colors: string[]
): ChannelState {
  const numLobes = 6;
  
  return {
    lobes: Array.from({ length: numLobes }, (_, i) => ({
      position: i / (numLobes - 1),
      intensity: 0.5 + Math.random() * 0.5, // Flicker
      radius: 0.1 + Math.random() * 0.05,
      color: hexToRgb(colors[Math.floor(Math.random() * colors.length)])
    }))
  };
}

/**
 * Pattern: Wave Effect (flowing cool colors)
 */
function generateWaveEffect(
  time: number,
  colors: string[]
): ChannelState {
  const numLobes = 5;
  
  return {
    lobes: Array.from({ length: numLobes }, (_, i) => {
      const phase = (time + i / numLobes) % 1.0;
      
      return {
        position: i / (numLobes - 1),
        intensity: 0.6 + Math.sin(phase * Math.PI * 2) * 0.4,
        radius: 0.12,
        color: hexToRgb(colors[i % colors.length])
      };
    })
  };
}

/**
 * Pattern: Breathe (all lobes pulse in sync)
 */
function generateBreathe(
  time: number,
  colors: string[]
): ChannelState {
  const pulse = Math.sin(time * Math.PI * 2) * 0.5 + 0.5; // 0 to 1
  
  return {
    lobes: colors.map((color, i) => ({
      position: (i + 0.5) / colors.length,
      intensity: 0.4 + pulse * 0.6,
      radius: 0.15 + pulse * 0.1,
      color: hexToRgb(color)
    }))
  };
}

/**
 * Default configurations for each pattern
 */
export const PATTERN_PRESETS: Record<AnimationPattern, AnimationConfig> = {
  radialBloom: {
    pattern: 'radialBloom',
    speed: 100,
    motionDirection: 'center',
    syncMode: 'sync',
    offsetDelayMs: 0,
    progressiveStartMs: 0,
    progressiveEndMs: 0,
    waveAmplitudeMs: 0,
    wavePeriod: 1,
    wavePhaseDeg: 0,
    waveform: 'sin',
    customDelayMap: [],
    motionOrigin: 'center',
    ch1Colors: ['#FF4500', '#FF6B35', '#FFA500'],
    ch2Colors: ['#7B68EE', '#5B8CFF', '#00CED1'],
    temporalOffset: 0
  },
  risingWave: {
    pattern: 'risingWave',
    speed: 120,
    motionDirection: 'left',
    syncMode: 'offset',
    offsetDelayMs: 150,
    progressiveStartMs: 0,
    progressiveEndMs: 0,
    waveAmplitudeMs: 0,
    wavePeriod: 1,
    wavePhaseDeg: 0,
    waveform: 'sin',
    customDelayMap: [],
    motionOrigin: 'left',
    ch1Colors: ['#FF4500', '#FF6B35'],
    ch2Colors: ['#00CED1', '#1E90FF'],
    temporalOffset: 150
  },
  horizontalChase: {
    pattern: 'horizontalChase',
    speed: 150,
    motionDirection: 'left',
    syncMode: 'sync',
    offsetDelayMs: 0,
    progressiveStartMs: 0,
    progressiveEndMs: 0,
    waveAmplitudeMs: 0,
    wavePeriod: 1,
    wavePhaseDeg: 0,
    waveform: 'sin',
    customDelayMap: [],
    motionOrigin: 'left',
    ch1Colors: ['#FF4500', '#FF6B35', '#FFA500', '#FFC64D'],
    ch2Colors: ['#7B68EE', '#5B8CFF', '#00CED1', '#1E90FF'],
    temporalOffset: 80
  },
  triangleShape: {
    pattern: 'triangleShape',
    speed: 100,
    motionDirection: 'left',
    syncMode: 'progressive',
    offsetDelayMs: 0,
    progressiveStartMs: 200,
    progressiveEndMs: 0,
    waveAmplitudeMs: 0,
    wavePeriod: 1,
    wavePhaseDeg: 0,
    waveform: 'sin',
    customDelayMap: [],
    motionOrigin: 'left',
    ch1Colors: ['#FF6B35', '#FFA500'],
    ch2Colors: ['#5B8CFF', '#00CED1'],
    temporalOffset: 200
  },
  dualZone: {
    pattern: 'dualZone',
    speed: 80,
    motionDirection: 'center',
    syncMode: 'sync',
    offsetDelayMs: 0,
    progressiveStartMs: 0,
    progressiveEndMs: 0,
    waveAmplitudeMs: 0,
    wavePeriod: 1,
    wavePhaseDeg: 0,
    waveform: 'sin',
    customDelayMap: [],
    motionOrigin: 'center',
    ch1Colors: ['#FF4500', '#FF6B35', '#FFA500'],
    ch2Colors: ['#00CED1', '#1E90FF', '#7B68EE'],
    temporalOffset: 0
  },
  breathe: {
    pattern: 'breathe',
    speed: 60,
    motionDirection: 'center',
    syncMode: 'sync',
    offsetDelayMs: 0,
    progressiveStartMs: 0,
    progressiveEndMs: 0,
    waveAmplitudeMs: 0,
    wavePeriod: 1,
    wavePhaseDeg: 0,
    waveform: 'sin',
    customDelayMap: [],
    motionOrigin: 'center',
    ch1Colors: ['#FF4500', '#FF6B35'],
    ch2Colors: ['#7B68EE', '#00CED1'],
    temporalOffset: 0
  }
};
