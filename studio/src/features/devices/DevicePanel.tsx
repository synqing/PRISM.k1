import React from 'react';
import { invoke } from '@tauri-apps/api/core';
import * as fs from '@tauri-apps/plugin-fs';
import { revealItemInDir, openPath } from '@tauri-apps/plugin-opener';
import { log } from '../../lib/logger';

type DeviceInfo = {
  name: string;
  host: string;
  port: number;
};

type ConnectState = Record<string, string>; // host: "CONNECTED" | error

type StatusInfo = {
  version: string;
  ledCount: number;
  storageAvailable: number;
  maxChunk: number;
  templateCount: number;
} | null;

type PatternItem = { id: string; size: number; mtime: number };

export default function DevicePanel() {
  const [devices, setDevices] = React.useState<DeviceInfo[]>([]);
  const [loading, setLoading] = React.useState(false);
  const [connectState, setConnectState] = React.useState<ConnectState>({});
  const [active, setActive] = React.useState<DeviceInfo | null>(null);
  const [statusInfo, setStatusInfo] = React.useState<StatusInfo>(null);
  const [patterns, setPatterns] = React.useState<PatternItem[]>([]);
  const [latencyMs, setLatencyMs] = React.useState<number | null>(null);
  const [netHealth, setNetHealth] = React.useState<'good'|'ok'|'poor'|'unknown'>('unknown');
  const [toasts, setToasts] = React.useState<{ id: number; text: string; kind: 'info'|'error' }[]>([]);
  const [exported, setExported] = React.useState<Record<string, string>>({});
  const [ctx, setCtx] = React.useState<{ open: boolean; x: number; y: number; id: string | null }>(() => ({ open: false, x: 0, y: 0, id: null }));
  const ctxRef = React.useRef<HTMLDivElement | null>(null);
  const [collision, setCollision] = React.useState<{ id: string; path: string; device: DeviceInfo } | null>(null);
  const w: any = typeof window !== 'undefined' ? window : {};
  const forceFake = !!w.__E2E_FORCE_FAKE__;
  const hasTauri = !!w.__TAURI_INTERNALS__;

  const scan = async () => {
    setLoading(true);
    try {
      if (!hasTauri || forceFake) {
        // Fallback for web-only: show a stub device
        setDevices([{ name: 'prism-k1', host: 'prism-k1.local', port: 80 }]);
      } else {
        const list = await invoke<DeviceInfo[]>('device_discover');
        setDevices(list);
      }
    } catch (e) {
      console.error(e);
      setDevices([]);
    } finally {
      setLoading(false);
    }
  };

  const connect = async (d: DeviceInfo) => {
    if (forceFake) {
      setConnectState((s) => ({ ...s, [d.host]: 'CONNECTED' }));
      setActive(d);
      setStatusInfo({ version: 'fake-1.0.0', ledCount: 320, storageAvailable: 1024*1024, maxChunk: 4089, templateCount: 5 });
      setPatterns([
        { id: 'waves', size: 10240, mtime: 1700000000 },
        { id: 'sparkle', size: 5120, mtime: 1700000500 },
      ]);
      return;
    }
    try {
      const res = await invoke<string>('device_connect', { host: d.host, port: d.port });
      setConnectState((s) => ({ ...s, [d.host]: res }));
      if (res === 'CONNECTED') {
        setActive(d);
        await fetchStatus(d);
      }
      log('info', `connect ${d.host}: ${res}`);
    } catch (e: any) {
      setConnectState((s) => ({ ...s, [d.host]: String(e) }));
      log('error', `connect ${d.host} failed: ${String(e)}`);
    }
  };

  const status = async (d: DeviceInfo) => {
    try {
      await fetchStatus(d, false);
      addToast(`Status refreshed for ${d.host}`, 'info');
    } catch (e: any) {
      addToast(`Status failed: ${String(e)}`, 'error');
    }
  };

  const fetchStatus = async (d: DeviceInfo, notify = false) => {
    if (forceFake) return;
    const t0 = performance.now();
    const res = await invoke<any>('device_status', { host: d.host });
    const t1 = performance.now();
    setLatencyMs(t1 - t0);
    const lat = t1 - t0;
    setNetHealth(lat <= 50 ? 'good' : lat <= 150 ? 'ok' : 'poor');
    setStatusInfo(res as StatusInfo);
    if (notify) log('info', `status ${d.host}: ${JSON.stringify(res)}`);
  };

  const listPatterns = async (d: DeviceInfo) => {
    if (forceFake) return; // already set on connect in fake mode
    try {
      const res = await invoke<any>('device_list', { host: d.host });
      const items: PatternItem[] = Array.isArray(res?.items) ? res.items : [];
      setPatterns(items);
      setActive(d);
      addToast(`Loaded ${items.length} patterns from ${d.host}`, 'info');
      log('info', `list ${d.host}: ${items.length} items`);
    } catch (e: any) {
      addToast(`List failed: ${String(e)}`, 'error');
      log('error', `list ${d.host} failed: ${String(e)}`);
    }
  };

  const deletePattern = async (d: DeviceInfo, id: string) => {
    // confirm
    setConfirm({ msg: `Delete '${id}' from ${d.host}?`, onConfirm: async () => {
      await doDeletePattern(d, id);
    }});
  };

  const doDeletePattern = async (d: DeviceInfo, id: string) => {
    if (forceFake) {
      setPatterns((ps) => ps.filter((p) => p.id !== id));
      return;
    }
    try {
      await invoke<boolean>('device_delete', { host: d.host, name: id });
      setPatterns((ps) => ps.filter((p) => p.id !== id));
      addToast(`Deleted '${id}'`, 'info');
      log('info', `delete ${d.host}:${id}`);
    } catch (e: any) {
      addToast(`Delete failed: ${String(e)}`, 'error');
      log('error', `delete ${d.host}:${id} failed: ${String(e)}`);
    }
  };

  const exportPattern = async (d: DeviceInfo, id: string) => {
    if (forceFake) {
      addToast('Export not available in fake mode', 'error');
      return;
    }
    try {
      const { save } = await import('@tauri-apps/plugin-dialog');
      const target = await save({ defaultPath: `${id}.bin`, filters: [{ name: 'PRISM Pattern', extensions: ['bin'] }] });
      if (!target) return;
      try {
        const exists = await ((fs as any).exists ? (fs as any).exists(target) : false);
        if (exists) {
          setCollision({ id, path: target, device: d });
          return;
        }
      } catch { /* noop */ }
      await invoke<boolean>('device_export', { host: d.host, id, path: target });
      setExported((m) => ({ ...m, [id]: target }));
      addToast(`Exported '${id}'`, 'info');
    } catch (e: any) {
      addToast(`Export failed: ${String(e)}`, 'error');
    }
  };

  const onRowContextMenu = (e: React.MouseEvent, id: string) => {
    e.preventDefault();
    setCtx({ open: true, x: e.clientX, y: e.clientY, id });
  };

  React.useEffect(() => {
    const onGlobal = (e: MouseEvent) => {
      if (!ctx.open) return;
      const el = ctxRef.current;
      if (el && el.contains(e.target as Node)) return;
      setCtx((c) => ({ ...c, open: false }));
    };
    window.addEventListener('click', onGlobal);
    window.addEventListener('contextmenu', onGlobal);
    return () => {
      window.removeEventListener('click', onGlobal);
      window.removeEventListener('contextmenu', onGlobal);
    };
  }, [ctx.open]);

  const revealExported = async (id: string) => {
    const p = exported[id];
    if (!p) { addToast('Reveal available after export', 'error'); return; }
    try { await revealItemInDir(p); } catch {
      try {
        const idx1 = p.lastIndexOf('/');
        const idx2 = p.lastIndexOf('\\\\');
        const dir = (idx1 >= 0 || idx2 >= 0) ? p.slice(0, Math.max(idx1, idx2)) : p;
        await openPath(dir);
      } catch { /* noop */ }
    }
  };

  const deleteAll = async (d: DeviceInfo) => {
    setConfirm({ msg: `Delete ALL patterns from ${d.host}?`, onConfirm: async () => {
      await doDeleteAll(d);
    }});
  };

  const doDeleteAll = async (d: DeviceInfo) => {
    const ids = patterns.map((p) => p.id);
    if (ids.length === 0) return;
    if (forceFake) {
      setPatterns([]);
      addToast('Deleted all (fake)', 'info');
      return;
    }
    try {
      for (const id of ids) {
        try { await invoke<boolean>('device_delete', { host: d.host, name: id }); } catch { /* noop */ }
      }
      setPatterns([]);
      addToast('Deleted all patterns', 'info');
      log('info', `delete-all ${d.host}: ${ids.length} items`);
    } catch (e: any) {
      addToast(`Delete all failed: ${String(e)}`, 'error');
      log('error', `delete-all ${d.host} failed: ${String(e)}`);
    }
  };

  const addToast = (text: string, kind: 'info'|'error' = 'info') => {
    const id = Date.now() + Math.random();
    setToasts((t) => [...t, { id, text, kind }]);
    setTimeout(() => setToasts((t) => t.filter((x) => x.id !== id)), 3500);
  };

  React.useEffect(() => {
    // Auto-scan on mount for convenience
    scan();
  }, []);

  // Periodic latency ping via STATUS every 5s when connected
  React.useEffect(() => {
    if (!active || forceFake) return;
    const id = setInterval(() => { fetchStatus(active).catch(() => { /* noop */ }); }, 5000);
    return () => clearInterval(id);
  }, [active, forceFake]);

  const fmtBytes = (n?: number | null) => {
    if (n == null) return '-';
    if (n >= 1024*1024) return `${(n/(1024*1024)).toFixed(1)} MiB`;
    if (n >= 1024) return `${(n/1024).toFixed(1)} KiB`;
    return `${n} B`;
  };

  const healthColor = netHealth === 'good' ? '#2ecc71' : netHealth === 'ok' ? '#f1c40f' : netHealth === 'poor' ? '#e74c3c' : '#7f8c8d';
  const [confirm, setConfirm] = React.useState<{ msg: string; onConfirm: () => Promise<void> } | null>(null);
  const confirmSticky = confirm || (collision ? { msg: `File exists: ${collision.path}. Overwrite or Auto‑Suffix?`, onConfirm: async () => {} } : null);

  const handleCollisionAction = async (action: 'overwrite' | 'suffix' | 'cancel') => {
    if (!collision) return;
    const { id, path, device } = collision;
    if (action === 'cancel') { setCollision(null); return; }
    let target = path;
    if (action === 'suffix') {
      target = await nextSuffixPath(path);
    }
    try {
      await invoke<boolean>('device_export', { host: device.host, id, path: target });
      setExported((m) => ({ ...m, [id]: target }));
      addToast(`Exported '${id}'`, 'info');
    } catch (e: any) {
      addToast(`Export failed: ${String(e)}`, 'error');
    } finally {
      setCollision(null);
    }
  };

  const nextSuffixPath = async (p: string) => {
    const idx1 = p.lastIndexOf('/');
    const idx2 = p.lastIndexOf('\\\\');
    const dir = (idx1 >= 0 || idx2 >= 0) ? p.slice(0, Math.max(idx1, idx2)) : '';
    const name = (idx1 >= 0 || idx2 >= 0) ? p.slice(Math.max(idx1, idx2) + 1) : p;
    const dot = name.lastIndexOf('.');
    const base = dot >= 0 ? name.slice(0, dot) : name;
    const ext = dot >= 0 ? name.slice(dot) : '';
    for (let i = 1; i < 100; i++) {
      const cand = `${dir ? dir + '/' : ''}${base} (${i})${ext}`;
      try {
        const exists = await ((fs as any).exists ? (fs as any).exists(cand) : false);
        if (!exists) return cand;
      } catch { return cand; }
    }
    return `${dir ? dir + '/' : ''}${base}-${Date.now()}${ext}`;
  };

  return (
    <section style={{ padding: 16, border: '1px solid #333', borderRadius: 8, marginTop: 24 }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
        <h2 style={{ margin: 0 }}>Devices</h2>
        <button onClick={scan} disabled={loading}>{loading ? 'Scanning…' : 'Scan'}</button>
        <span style={{ marginLeft: 'auto', fontSize: 12, opacity: 0.85 }}>
          Network:&nbsp;
          <span style={{ color: healthColor }}>{netHealth.toUpperCase()}</span>
          {latencyMs != null && <span> ({Math.round(latencyMs)} ms)</span>}
        </span>
      </div>
      {devices.length === 0 ? (
        <p style={{ opacity: 0.75, marginTop: 12 }}>No devices found yet.</p>
      ) : (
        <table style={{ width: '100%', marginTop: 12, borderCollapse: 'collapse' }}>
          <thead>
            <tr>
              <th style={{ textAlign: 'left', borderBottom: '1px solid #333', paddingBottom: 6 }}>Name</th>
              <th style={{ textAlign: 'left', borderBottom: '1px solid #333', paddingBottom: 6 }}>Host</th>
              <th style={{ textAlign: 'left', borderBottom: '1px solid #333', paddingBottom: 6 }}>Port</th>
              <th style={{ textAlign: 'left', borderBottom: '1px solid #333', paddingBottom: 6 }}>Actions</th>
              <th style={{ textAlign: 'left', borderBottom: '1px solid #333', paddingBottom: 6 }}>State</th>
            </tr>
          </thead>
          <tbody>
            {devices.map((d) => (
              <tr key={`${d.host}:${d.port}`}>
                <td style={{ padding: '6px 0' }}>{d.name}</td>
                <td style={{ padding: '6px 0' }}>{d.host}</td>
                <td style={{ padding: '6px 0' }}>{d.port}</td>
                <td style={{ padding: '6px 0' }}>
                  <button onClick={() => connect(d)}>Connect</button>
                  <button onClick={() => status(d)} style={{ marginLeft: 6 }}>Status</button>
                  <button onClick={() => listPatterns(d)} style={{ marginLeft: 6 }}>List</button>
                </td>
                <td style={{ padding: '6px 0' }}>{connectState[d.host] || '-'}</td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
      {(active || patterns.length > 0) && (
        <div style={{ marginTop: 16 }}>
          <h3 style={{ marginTop: 0 }}>Device: {active ? active.host : 'n/a'}</h3>
          {statusInfo ? (
            <div style={{ display: 'flex', gap: 16, flexWrap: 'wrap', fontSize: 14, opacity: 0.9 }}>
              <div>Version: <strong>{statusInfo.version}</strong></div>
              <div>LEDs: <strong>{statusInfo.ledCount}</strong></div>
              <div>Storage Free: <strong>{fmtBytes(statusInfo.storageAvailable)}</strong></div>
              <div>Max Chunk: <strong>{statusInfo.maxChunk}</strong></div>
              <div>Templates: <strong>{statusInfo.templateCount}</strong></div>
            </div>
          ) : (
            <div style={{ opacity: 0.7 }}>No status yet.</div>
          )}

          {patterns.length > 0 && (
            <div style={{ marginTop: 12 }}>
              <h4 style={{ margin: '8px 0' }}>Patterns</h4>
              <div style={{ display: 'flex', gap: 8, marginBottom: 8 }}>
                <button onClick={() => active ? listPatterns(active) : undefined}>Refresh</button>
                <button onClick={() => active ? deleteAll(active) : undefined} disabled={!active || connectState[active.host] !== 'CONNECTED'}>Delete All</button>
              </div>
              <table style={{ width: '100%', borderCollapse: 'collapse' }}>
                <thead>
                  <tr>
                    <th style={{ textAlign: 'left', borderBottom: '1px solid #333', paddingBottom: 6 }}>ID</th>
                    <th style={{ textAlign: 'left', borderBottom: '1px solid #333', paddingBottom: 6 }}>Size</th>
                    <th style={{ textAlign: 'left', borderBottom: '1px solid #333', paddingBottom: 6 }}>Modified</th>
                    <th style={{ textAlign: 'left', borderBottom: '1px solid #333', paddingBottom: 6 }}>Actions</th>
                  </tr>
                </thead>
                <tbody>
                  {patterns.map((p) => (
                    <tr key={p.id} onContextMenu={(e) => onRowContextMenu(e, p.id)}>
                      <td style={{ padding: '6px 0' }}>{p.id}</td>
                      <td style={{ padding: '6px 0' }}>{fmtBytes(p.size)}</td>
                      <td style={{ padding: '6px 0' }}>{new Date(p.mtime * 1000).toLocaleString()}</td>
                      <td style={{ padding: '6px 0' }}>
                        <button onClick={() => active && deletePattern(active, p.id)} disabled={!active || connectState[active.host] !== 'CONNECTED'}>Delete</button>
                        <button onClick={() => active && exportPattern(active, p.id)} style={{ marginLeft: 6 }} disabled={!active}>Export</button>
                        {exported[p.id] ? (
                          <button onClick={() => addToast('Revealing exported file from Recent section', 'info')} style={{ marginLeft: 6 }}>Reveal</button>
                        ) : (
                          <button onClick={() => addToast('Reveal available after export', 'error')} style={{ marginLeft: 6 }}>Reveal</button>
                        )}
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      )}

      {/* Toasts */}
      <div style={{ position: 'fixed', right: 16, bottom: 16, display: 'flex', flexDirection: 'column', gap: 8, zIndex: 50 }}>
        {toasts.map((t) => (
          <div key={t.id} style={{ background: t.kind === 'error' ? '#e74c3c' : '#2d3436', color: '#fff', padding: '8px 12px', borderRadius: 6, boxShadow: '0 2px 6px rgba(0,0,0,0.3)', maxWidth: 360 }}>
            {t.text}
          </div>
        ))}
      </div>
      {ctx.open && ctx.id && (
        <div ref={ctxRef} style={{ position: 'fixed', top: ctx.y, left: ctx.x, background: '#2d3436', color: '#fff', padding: 6, borderRadius: 6, boxShadow: '0 2px 6px rgba(0,0,0,0.3)', zIndex: 60 }}>
          <div>
            <button onClick={() => { active && exportPattern(active, ctx.id!); setCtx((c)=>({ ...c, open: false })); }} disabled={!active}>Export</button>
          </div>
          <div style={{ marginTop: 6 }}>
            <button onClick={() => { revealExported(ctx.id!); setCtx((c)=>({ ...c, open: false })); }}>Reveal</button>
          </div>
          <div style={{ marginTop: 6 }}>
            <button onClick={() => { active && deletePattern(active, ctx.id!); setCtx((c)=>({ ...c, open: false })); }} disabled={!active || (active && connectState[active.host] !== 'CONNECTED')}>Delete</button>
          </div>
        </div>
      )}
      {confirmSticky && (
        <div style={{ position: 'fixed', left: '50%', transform: 'translateX(-50%)', bottom: 16, background: '#2d3436', color: '#fff', padding: '10px 14px', borderRadius: 6, display: 'flex', gap: 8, alignItems: 'center', boxShadow: '0 2px 6px rgba(0,0,0,0.3)', zIndex: 60 }}>
          <span>{confirm ? confirm.msg : confirmSticky.msg}</span>
          {confirm ? (
            <>
              <button onClick={async () => { const c = confirm; setConfirm(null); c && (await c.onConfirm()); }}>Confirm</button>
              <button onClick={() => setConfirm(null)}>Cancel</button>
            </>
          ) : (
            <>
              <button onClick={() => handleCollisionAction('overwrite')}>Overwrite</button>
              <button onClick={() => handleCollisionAction('suffix')}>Auto‑Suffix</button>
              <button onClick={() => handleCollisionAction('cancel')}>Cancel</button>
            </>
          )}
        </div>
      )}
    </section>
  );
}
