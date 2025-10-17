import { create } from 'zustand';
import { devtools } from 'zustand/middleware';
import { immer } from 'zustand/middleware/immer';
import { temporal } from 'zundo';
import { save, open } from '@tauri-apps/plugin-dialog';
import { readTextFile, writeTextFile } from '@tauri-apps/plugin-fs';
import { throttle } from 'lodash-es';
import type { Project, Track, Clip } from '../lib/projectSchema';
import { ProjectSchema, createEmptyProject } from '../lib/projectSchema';

type ProjectState = {
  project: Project;
  filePath: string | null;
  isDirty: boolean;
  recentFiles: string[];
  setName: (name: string) => void;
  touch: () => void;
  addTrack: (track: Omit<Track, 'clips'> & { clips?: Clip[] }) => void;
  addClip: (clip: Clip) => void;
  replace: (data: unknown) => void; // load project from serialized data
  newProject: () => void;
  saveProject: () => Promise<void>;
  saveProjectAs: () => Promise<void>;
  openProject: () => Promise<void>;
  saveProjectToPath: (path: string) => Promise<void>;
  openProjectFromPath: (path: string) => Promise<void>;
  addRecent: (path: string) => void;
  clearRecent: () => void;
  setLastBake: (stats: {
    fps: number; ledCount: number; frames: number; payloadSize: number; totalSize: number;
    ttflMs?: number; startedAt?: number | null; finishedAt?: number | null; throughputBps?: number;
    geometryId?: string; palettePolicy?: 'host_lut' | 'device_blend';
  }) => void;
};

export const useProjectStore = create<ProjectState>()(
  devtools(
    temporal(
      immer((set, get) => {
        // helper to mark dirty and trigger autosave
        const markDirty = <T extends any[]>(fn: (...args: T) => void) =>
          (...args: T) => {
            fn(...args);
            set({ isDirty: true });
            autosave();
          };

        // Autosave: hybrid throttle (>=250ms) with idle flush at 2s and edit-count threshold (10)
        let editCount = 0;
        let lastSaveAt = 0;
        let idleTimer: any | null = null;
        const doSaveIfNeeded = async () => {
          const { isDirty, filePath } = get();
          if (!isDirty || !filePath) return;
          await get().saveProject();
          editCount = 0;
          lastSaveAt = Date.now();
        };
        const scheduleIdleFlush = () => {
          if (idleTimer) clearTimeout(idleTimer);
          idleTimer = setTimeout(() => {
            void doSaveIfNeeded();
          }, 2000); // idle flush at 2s
        };
        const autosave = throttle(async () => {
          editCount += 1;
          const now = Date.now();
          const sinceLast = now - lastSaveAt;
          // Trigger on edit threshold or if at least 250ms since last throttle edge
          if (editCount >= 10 || sinceLast >= 250) {
            await doSaveIfNeeded();
          }
          scheduleIdleFlush();
        }, 250, { leading: true, trailing: true });

        const loadRecent = (): string[] => {
          if (typeof window === 'undefined') return [];
          try {
            return JSON.parse(localStorage.getItem('prism.recent') || '[]');
          } catch { /* noop */
            return [];
          }
        };

        return {
          project: createEmptyProject('Untitled Project'),
          filePath: null,
          isDirty: false,
          recentFiles: loadRecent(),

          setName: markDirty((name: string) =>
            set((s) => {
              s.project.name = name;
              s.project.updatedAt = new Date().toISOString();
            }, false, 'project/setName')
          ),
          touch: markDirty(() =>
            set((s) => {
              s.project.updatedAt = new Date().toISOString();
            }, false, 'project/touch')
          ),
          addTrack: markDirty((track: Omit<Track, 'clips'> & { clips?: Clip[] }) =>
            set((s) => {
              s.project.tracks.push({ ...track, clips: track.clips ?? [] });
              s.project.updatedAt = new Date().toISOString();
            }, false, 'project/addTrack')
          ),
          addClip: markDirty((clip: Clip) =>
            set((s) => {
              const t = s.project.tracks.find((t) => t.id === clip.trackId);
              if (!t) throw new Error(`Track not found: ${clip.trackId}`);
              t.clips.push(clip);
              s.project.updatedAt = new Date().toISOString();
            }, false, 'project/addClip')
          ),
          replace: (data: unknown) =>
            set((s) => {
              const parsed = ProjectSchema.parse(data);
              s.project = parsed;
              s.isDirty = false;
            }, false, 'project/replace'),

          newProject: () =>
            set(() => ({
              project: createEmptyProject('Untitled Project'),
              filePath: null,
              isDirty: false,
            }), false, 'project/new'),

          saveProject: async () => {
            const { project, filePath } = get();
            const path =
              filePath ||
              (await save({
                filters: [{ name: 'PRISM Project', extensions: ['prismproj'] }],
                defaultPath: `${project.name}.prismproj`,
              }));
            if (!path) return;
            await get().saveProjectToPath(path);
          },

          saveProjectAs: async () => {
            const { project } = get();
            const path = await save({
              filters: [{ name: 'PRISM Project', extensions: ['prismproj'] }],
              defaultPath: `${project.name}.prismproj`,
            });
            if (!path) return;
            await get().saveProjectToPath(path);
          },

          openProject: async () => {
            const path = await open({
              multiple: false,
              filters: [{ name: 'PRISM Project', extensions: ['prismproj', 'json'] }],
            });
            if (!path || Array.isArray(path)) return;
            await get().openProjectFromPath(path);
          },
          saveProjectToPath: async (path: string) => {
            const { project } = get();
            const json = serializeProject(project);
            if (typeof window !== 'undefined' && !(window as any).__TAURI_INTERNALS__) {
              try { localStorage.setItem(`fs:${path}`, json); } catch { /* noop */ }
            } else {
              await writeTextFile(path, json);
            }
            set((s) => {
              s.filePath = path;
              s.isDirty = false;
              s.recentFiles = addRecentLocal(s.recentFiles, path);
            }, false, 'project/saveToPath');
          },
          openProjectFromPath: async (path: string) => {
            let json: string | null = null;
            if (typeof window !== 'undefined' && !(window as any).__TAURI_INTERNALS__) {
              try { json = localStorage.getItem(`fs:${path}`); } catch { /* noop */ }
              if (!json) throw new Error('Mock file not found in localStorage');
            } else {
              json = await readTextFile(path);
            }
            const parsed = deserializeProject(json);
            set((s) => {
              s.project = parsed;
              s.filePath = path;
              s.isDirty = false;
              s.recentFiles = addRecentLocal(s.recentFiles, path);
            }, false, 'project/openFromPath');
          },
          addRecent: (path: string) =>
            set((s) => {
              s.recentFiles = addRecentLocal(s.recentFiles, path);
            }, false, 'project/addRecent'),

          clearRecent: () =>
            set((s) => {
              s.recentFiles = [];
              if (typeof window !== 'undefined') {
                try { localStorage.setItem('prism.recent', JSON.stringify([])); } catch { /* noop */ }
                (async () => {
                  try {
                    if ((window as any).__TAURI_INTERNALS__) {
                      const { emit } = await import('@tauri-apps/api/event');
                      await emit('recent:update', JSON.stringify([]));
                    }
                  } catch { /* noop */ }
                })();
              }
            }, false, 'project/clearRecent'),

          setLastBake: (stats) =>
            set((s) => {
              s.project.lastBake = {
                fps: stats.fps,
                ledCount: stats.ledCount,
                frames: stats.frames,
                payloadSize: stats.payloadSize,
                totalSize: stats.totalSize,
                ttflMs: stats.ttflMs ?? undefined,
                startedAt: stats.startedAt ?? undefined as any,
                finishedAt: stats.finishedAt ?? undefined as any,
                throughputBps: stats.throughputBps ?? undefined,
                geometryId: stats.geometryId ?? undefined,
                palettePolicy: stats.palettePolicy ?? undefined,
              } as any;
              s.project.updatedAt = new Date().toISOString();
              s.isDirty = true;
            }, false, 'project/setLastBake'),
        };
      }),
      {
        limit: 50,
        partialize: (state) => ({ project: state.project }),
        equality: (a, b) => (a as any).project === (b as any).project,
      }
    )
  )
);

export function serializeProject(p: Project): string {
  return JSON.stringify(p);
}

export function deserializeProject(json: string): Project {
  return ProjectSchema.parse(JSON.parse(json));
}

export const { undo, redo, clear } = useProjectStore.temporal.getState();

function addRecentLocal(list: string[], path: string): string[] {
  const next = [path, ...list.filter((p) => p !== path)].slice(0, 5);
  if (typeof window !== 'undefined') {
    try { localStorage.setItem('prism.recent', JSON.stringify(next)); } catch { /* noop */ }
    // Notify Tauri backend to update the native menu
    if ((window as any).__TAURI_INTERNALS__) {
      (async () => {
        try {
          const { emit } = await import('@tauri-apps/api/event');
          await emit('recent:update', JSON.stringify(next));
        } catch { /* noop */ }
      })();
    }
  }
  return next;
}
