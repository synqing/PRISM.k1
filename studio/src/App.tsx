import React from 'react';
import { useProjectStore } from './stores/project';
import SecureSettings from './features/settings/SecureSettings';
import DevicePanel from './features/devices/DevicePanel';
import LoggerPanel from './features/logger/LoggerPanel';
import { revealItemInDir, openPath } from '@tauri-apps/plugin-opener';

export default function App() {
  const project = useProjectStore((s) => s.project);
  const isDirty = useProjectStore((s) => s.isDirty);
  const saveProject = useProjectStore((s) => s.saveProject);
  const saveProjectAs = useProjectStore((s) => s.saveProjectAs);
  const openProject = useProjectStore((s) => s.openProject);
  const newProject = useProjectStore((s) => s.newProject);
  const setName = useProjectStore((s) => s.setName);
  const recent = useProjectStore((s) => s.recentFiles);
  const [showLogger, setShowLogger] = React.useState<boolean>(() => {
    try { return JSON.parse(localStorage.getItem('prism.showLogger') || 'false'); } catch { return false; }
  });

  const revealPath = async (p: string) => {
    const w: any = window as any;
    if (!w.__TAURI_INTERNALS__) return;
    const idx1 = p.lastIndexOf('/');
    const idx2 = p.lastIndexOf('\\\\');
    const idx = Math.max(idx1, idx2);
    const dir = idx > 0 ? p.slice(0, idx) : p;
    try { await revealItemInDir(p); } catch { try { await openPath(dir); } catch { /* noop */ } }
  };

  // Keyboard shortcuts: Cmd/Ctrl+Z and Shift+Cmd/Ctrl+Z
  React.useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      const isMod = e.metaKey || e.ctrlKey;
      if (!isMod) return;
      if (e.key.toLowerCase() === 'z' && !e.shiftKey) {
        e.preventDefault();
        // undo
        (useProjectStore.temporal.getState().undo)();
      } else if ((e.key.toLowerCase() === 'z' && e.shiftKey) || (e.key.toLowerCase() === 'y')) {
        e.preventDefault();
        // redo
        (useProjectStore.temporal.getState().redo)();
      } else if (e.key.toLowerCase() === 's' && !e.shiftKey) {
        e.preventDefault();
        // save
        saveProject();
      } else if (e.key.toLowerCase() === 's' && e.shiftKey) {
        e.preventDefault();
        // save as
        saveProjectAs();
      }
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, []);

  // Dev-only E2E helpers
  React.useEffect(() => {
    if (import.meta.env.DEV) {
      // @ts-ignore test hook
      (window as any).__e2e = {
        saveTo: (path: string) => useProjectStore.getState().saveProjectToPath(path),
        openFrom: (path: string) => useProjectStore.getState().openProjectFromPath(path),
        setName: (n: string) => useProjectStore.getState().setName(n),
        getProjectName: () => useProjectStore.getState().project.name,
      };
    }
  }, []);

  // Listen for native menu actions and map to store
  React.useEffect(() => {
    let unlisten: (() => void) | undefined;
    (async () => {
      if ((window as any).__TAURI_INTERNALS__) {
        try {
          const { listen } = await import('@tauri-apps/api/event');
          unlisten = await listen<string>('menu:action', (e) => {
            switch (e.payload) {
              case 'undo':
                (useProjectStore.temporal.getState().undo)();
                break;
              case 'redo':
                (useProjectStore.temporal.getState().redo)();
                break;
              case 'save':
                saveProject();
                break;
              case 'saveAs':
                saveProjectAs();
                break;
              case 'open':
                openProject();
                break;
              case 'new':
                newProject();
                break;
              case 'clearRecent':
                useProjectStore.getState().clearRecent();
                break;
              case 'settings': {
                const el = document.getElementById('secure-settings');
                if (el) el.scrollIntoView({ behavior: 'smooth', block: 'start' });
                break;
              }
            }
          });
          // open recent handler
          await listen<string>('menu:openRecent', (e) => {
            if (!e.payload) return;
            useProjectStore.getState().openProjectFromPath(e.payload);
          });
          await listen<string>('menu:revealRecent', (e) => {
            if (!e.payload) return;
            revealPath(e.payload);
          });
        } catch {
          // ignore when not in Tauri runtime
        }
      }
    })();
    return () => {
      try { unlisten && unlisten(); } catch { /* noop */ }
    };
  }, [saveProject, saveProjectAs, openProject, newProject]);

  // Push initial recent list to native menu at startup
  React.useEffect(() => {
    (async () => {
      if ((window as any).__TAURI_INTERNALS__) {
        try {
          const { emit } = await import('@tauri-apps/api/event');
          await emit('recent:update', JSON.stringify(useProjectStore.getState().recentFiles));
        } catch { /* noop */ }
      }
    })();
  }, []);

  return (
    <div>
      <nav style={{ padding: 12, borderBottom: '1px solid #333', display: 'flex', gap: 8, alignItems: 'center' }}>
        <button onClick={newProject}>New</button>
        <button onClick={openProject}>Open</button>
        <button onClick={saveProject}>Save</button>
        <button onClick={saveProjectAs}>Save As…</button>
        {isDirty && <span title="Unsaved changes" style={{ color: '#f80' }}>•</span>}
        <span style={{ marginLeft: 16, opacity: 0.8 }}>Recent:</span>
        {recent.slice(0, 3).map((p) => (
          <span key={p} title={p}>
            <button onClick={() => useProjectStore.getState().openProjectFromPath(p)}>
              {p.split('/').pop()}
            </button>
            <button onClick={() => revealPath(p)} title="Open containing folder" style={{ marginLeft: 4 }}>Reveal</button>
          </span>
        ))}
      </nav>
      <main style={{ padding: 24 }}>
        <h1>PRISM Studio</h1>
        <p>Project: {project.name}</p>
        <div style={{ marginTop: 12 }}>
          <label>
            Rename project:&nbsp;
            <input value={project.name} onChange={(e) => setName(e.target.value)} />
          </label>
        </div>

        <DevicePanel />
      </main>
      <div style={{ padding: 24 }}>
        <SecureSettings showLogger={showLogger} setShowLogger={(v: boolean) => { setShowLogger(v); try { localStorage.setItem('prism.showLogger', JSON.stringify(v)); } catch { /* noop */ } }} />
        {showLogger && <LoggerPanel />}
      </div>
    </div>
  );
}
