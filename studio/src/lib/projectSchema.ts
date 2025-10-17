import { z } from 'zod';

// Canon-aligned enums (ADR-003/008 for LED and temporal; ADR-010 for sync)
export const MotionEnum = z.enum(['LEFT', 'RIGHT', 'CENTER', 'EDGE', 'STATIC']);
export type Motion = z.infer<typeof MotionEnum>;

export const SyncModeEnum = z.enum(['SYNC', 'OFFSET', 'PROGRESSIVE', 'WAVE', 'CUSTOM']);
export type SyncMode = z.infer<typeof SyncModeEnum>;

// Device configuration (align with CANON: led_count=320, fps=120)
export const DeviceSchema = z.object({
  ledCount: z.number().int().positive().default(320),
  fps: z.number().int().positive().default(120),
});
export type Device = z.infer<typeof DeviceSchema>;

// Sync configuration with mode-specific parameters
export const WaveformEnum = z.enum(['sine', 'triangle', 'sawtooth']);
export type Waveform = z.infer<typeof WaveformEnum>;

export const SyncConfigSchema = z.object({
  mode: SyncModeEnum,
  // OFFSET
  offsetMs: z.number().int().min(0).max(500).optional(),
  // PROGRESSIVE
  progressiveStartMs: z.number().int().min(0).max(500).optional(),
  progressiveEndMs: z.number().int().min(0).max(500).optional(),
  // WAVE
  waveAmplitudeMs: z.number().int().min(0).max(500).optional(),
  wavePeriod: z.number().int().min(1).max(10).optional(), // cycles across 160 LEDs
  wavePhaseDeg: z.number().int().min(0).max(359).optional(),
  waveform: WaveformEnum.optional(),
  // CUSTOM (per-LED delay for one edge of 160)
  customDelayMs: z.array(z.number().int().min(0).max(500)).optional(),
}).superRefine((val, ctx) => {
  switch (val.mode) {
    case 'SYNC':
      break;
    case 'OFFSET':
      if (typeof val.offsetMs !== 'number') ctx.addIssue({ code: z.ZodIssueCode.custom, message: 'offsetMs required for OFFSET' });
      break;
    case 'PROGRESSIVE':
      if (typeof val.progressiveStartMs !== 'number' || typeof val.progressiveEndMs !== 'number') {
        ctx.addIssue({ code: z.ZodIssueCode.custom, message: 'progressiveStartMs and progressiveEndMs required for PROGRESSIVE' });
      }
      break;
    case 'WAVE':
      if (typeof val.waveAmplitudeMs !== 'number' || typeof val.wavePeriod !== 'number') {
        ctx.addIssue({ code: z.ZodIssueCode.custom, message: 'waveAmplitudeMs and wavePeriod required for WAVE' });
      }
      if (typeof val.wavePhaseDeg !== 'number') {
        ctx.addIssue({ code: z.ZodIssueCode.custom, message: 'wavePhaseDeg required for WAVE' });
      }
      if (typeof val.waveform !== 'string') {
        ctx.addIssue({ code: z.ZodIssueCode.custom, message: 'waveform required for WAVE' });
      }
      break;
    case 'CUSTOM':
      if (!Array.isArray(val.customDelayMs)) {
        ctx.addIssue({ code: z.ZodIssueCode.custom, message: 'customDelayMs required for CUSTOM' });
      } else if (val.customDelayMs.length !== 160) {
        ctx.addIssue({ code: z.ZodIssueCode.custom, message: 'customDelayMs must have 160 entries (edge LED count)' });
      }
      break;
  }
});
export type SyncConfig = z.infer<typeof SyncConfigSchema>;

// Palette (array of #RRGGBB hex strings)
export const ColorHex = z.string().regex(/^#([0-9a-fA-F]{6})$/, 'Expected #RRGGBB color');
export const PaletteSchema = z.array(ColorHex).min(1);
export type Palette = z.infer<typeof PaletteSchema>;

export const ClipSchema = z.object({
  id: z.string().uuid(),
  trackId: z.string(),
  name: z.string().min(1),
  start: z.number().int().nonnegative(), // ms
  duration: z.number().int().nonnegative(), // ms
  effect: z.string().optional(),
  params: z.record(z.any()).default({}),
});

export type Clip = z.infer<typeof ClipSchema>;

export const TrackSchema = z.object({
  id: z.string(),
  name: z.string().min(1),
  clips: z.array(ClipSchema).default([]),
});

export type Track = z.infer<typeof TrackSchema>;

export const ProjectSchema = z.object({
  version: z.literal(1),
  id: z.string(),
  name: z.string().min(1),
  createdAt: z.string(), // ISO
  updatedAt: z.string(), // ISO
  bpm: z.number().positive().default(120),
  tracks: z.array(TrackSchema).default([]),
  // New meta describing device + temporal behavior
  device: DeviceSchema.default({ ledCount: 320, fps: 120 }),
  motion: MotionEnum.default('STATIC'),
  sync: SyncConfigSchema.default({ mode: 'SYNC' }),
  palette: PaletteSchema.default(['#ffffff']),
  description: z.string().optional(),
  lastBake: z
    .object({
      fps: z.number().int().positive(),
      ledCount: z.number().int().positive(),
      frames: z.number().int().nonnegative(),
      payloadSize: z.number().int().nonnegative(),
      totalSize: z.number().int().nonnegative(),
      ttflMs: z.number().int().nonnegative().optional(),
      startedAt: z.number().int().nonnegative().optional(),
      finishedAt: z.number().int().nonnegative().optional(),
      throughputBps: z.number().int().nonnegative().optional(),
    })
    .optional(),
});

export type Project = z.infer<typeof ProjectSchema>;

export function createEmptyProject(name = 'Untitled Project'): Project {
  const now = new Date().toISOString();
  return {
    version: 1,
    id: crypto.randomUUID(),
    name,
    createdAt: now,
    updatedAt: now,
    bpm: 120,
    tracks: [],
    device: { ledCount: 320, fps: 120 },
    motion: 'STATIC',
    sync: { mode: 'SYNC' },
    palette: ['#ffffff'],
    lastBake: undefined,
  };
}
