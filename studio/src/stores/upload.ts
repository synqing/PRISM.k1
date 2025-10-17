import { create } from 'zustand';
import { listen } from '@tauri-apps/api/event';
import { appendTelemetry } from '../lib/telemetry';

export type UploadPhase = 'idle' | 'prepare' | 'stream' | 'finalizing' | 'done' | 'error' | 'cancelled';

type UploadState = {
  phase: UploadPhase;
  percent: number;
  bytesSent: number;
  totalBytes: number;
  bytesPerSec: number;
  error: string | null;
  startedAt: number | null;
  finishedAt: number | null;
  cancel: () => Promise<void>;
  reset: () => void;
  subscribe: () => void;
};

export const useUploadStore = create<UploadState>((set, get) => ({
  phase: 'idle', percent: 0, bytesSent: 0, totalBytes: 0, bytesPerSec: 0,
  error: null, startedAt: null, finishedAt: null,
  cancel: async () => {
    try { const { invoke } = await import('@tauri-apps/api/core'); await invoke('device_upload_cancel'); } catch {}
    set({ phase: 'cancelled', finishedAt: Date.now() });
  },
  reset: () => set({ phase: 'idle', percent: 0, bytesSent: 0, totalBytes: 0, bytesPerSec: 0, error: null, startedAt: null, finishedAt: null }),
  subscribe: () => {
    // idempotent attach (simple)
    listen('upload:progress', (e) => {
      const p = e.payload as any;
      const nextPhase = (p.phase as UploadPhase) ?? 'stream';
      const now = Date.now();
      // If we've reached a terminal cancelled state, ignore subsequent events
      if (get().phase === 'cancelled') return;
      if (nextPhase === 'finalizing') {
        set({ phase: 'done', percent: 100, bytesSent: p.bytesSent ?? get().bytesSent, totalBytes: p.totalBytes ?? get().totalBytes, bytesPerSec: p.bytesPerSec ?? get().bytesPerSec, finishedAt: now });
        void appendTelemetry({ ts: now, phase: 'finalizing', bytesSent: p.bytesSent, totalBytes: p.totalBytes, percent: 100, bytesPerSec: p.bytesPerSec });
      } else if (nextPhase === 'done') {
        set({ phase: 'done', finishedAt: now });
        const ttflMs = (get().startedAt && get().finishedAt) ? (get().finishedAt! - get().startedAt!) : undefined;
        void appendTelemetry({ ts: now, phase: 'done', ttflMs });
      } else if (nextPhase === 'cancelled') {
        set({ phase: 'cancelled', finishedAt: now });
        void appendTelemetry({ ts: now, phase: 'cancelled' });
      } else {
        set({
          phase: nextPhase,
          bytesSent: p.bytesSent ?? 0,
          totalBytes: p.totalBytes ?? 0,
          percent: p.percent ?? 0,
          bytesPerSec: p.bytesPerSec ?? 0,
          startedAt: get().startedAt ?? now,
        });
        void appendTelemetry({ ts: now, phase: nextPhase, bytesSent: p.bytesSent, totalBytes: p.totalBytes, percent: p.percent, bytesPerSec: p.bytesPerSec });
      }
    });
    return;
  },
}));
