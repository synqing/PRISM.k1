import React from 'react';
import { invoke } from '@tauri-apps/api/core';
import * as fs from '@tauri-apps/plugin-fs';
import { revealItemInDir, openPath } from '@tauri-apps/plugin-opener';
import { log } from '../../lib/logger';
import { compileGraphToIR } from '../../lib/ir/exportIR';
import { appendTelemetry } from '../../lib/telemetry';
import ProgressPanel from './ProgressPanel';
import { useUploadStore } from '../../stores/upload';
import { mapUploadError } from '../../lib/uploadErrors';
import { CAP_PALETTE, CAP_COMPRESS, CAP_RESUME, CAP_EVENTS, CAP_PROGRAM } from '../../lib/protocol';
import { PRESETS } from '../../lib/presets';
import { useProjectStore } from '../../stores/project';

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
  caps?: number;
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
  const upload = useUploadStore();
  React.useEffect(() => {
    if (upload.phase === 'cancelled') {
      addToast('Upload cancelled', 'error');
    }
  }, [upload.phase]);

  React.useEffect(() => {
    if (upload.phase === 'done') {
      const startedAt = upload.startedAt;
      const finishedAt = upload.finishedAt;
      const ttflMs = startedAt && finishedAt ? (finishedAt - startedAt) : undefined;
      const palette = paletteStr.split(',').map(s => s.trim()).filter(Boolean);
      const totalSize = upload.totalBytes || 0;
      const payloadSize = Math.max(0, totalSize - 64 - Math.min(16, palette.length) * 4);
      useProjectStore.getState().setLastBake({
        fps,
        ledCount: 320,
        frames: Math.max(1, Math.round(seconds * fps)),
        payloadSize,
        totalSize,
        ttflMs,
        startedAt: startedAt ?? null,
        finishedAt: finishedAt ?? null,
        throughputBps: upload.bytesPerSec,
        geometryId,
        palettePolicy: devicePaletteBlend ? 'device_blend' : 'host_lut',
      });
      // Append telemetry snapshot with publish metadata
      void appendTelemetry({ ts: Date.now(), phase: 'publish', bytesPerSec: upload.bytesPerSec, totalBytes: totalSize, percent: 100, ttflMs, });
    }
  }, [upload.phase]);

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

  // Keyboard shortcuts: mod+P (play), mod+S (stop)
  React.useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      const mod = e.metaKey || e.ctrlKey;
      if (!mod) return;
      if (e.key.toLowerCase() === 'p') {
        e.preventDefault();
        if (active) { invoke('device_control_play', { host: active.host, name: 'baked' }).catch(()=>{}); }
      } else if (e.key.toLowerCase() === 's') {
        e.preventDefault();
        if (active) { invoke('device_control_stop', { host: active.host }).catch(()=>{}); }
      }
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [active]);

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
  // Bake UI controls
  const [seconds, setSeconds] = React.useState<number>(1);
  const [fps, setFps] = React.useState<number>(120);
  const [color, setColor] = React.useState<string>('#1ec8ff');
  const [paletteStr, setPaletteStr] = React.useState<string>('#ff6b35,#f7931e,#fdc830,#f37335');
  const [devicePaletteBlend, setDevicePaletteBlend] = React.useState<boolean>(() => {
    try { return JSON.parse(localStorage.getItem('prism.palette.deviceBlend') || 'false'); } catch { return false; }
  });
  const [geometryId, setGeometryId] = React.useState<string>(() => {
    try { return localStorage.getItem('prism.geometry.id') || 'K1_LGP_v1'; } catch { return 'K1_LGP_v1'; }
  });
  // Node controls
  const [useGradient, setUseGradient] = React.useState<boolean>(false);
  const [gradC0, setGradC0] = React.useState<string>('#000000');
  const [gradC1, setGradC1] = React.useState<string>('#ffffff');
  const [gradSpeed, setGradSpeed] = React.useState<number>(0.25);
  const [useHueShift, setUseHueShift] = React.useState<boolean>(false);
  const [hueDeg, setHueDeg] = React.useState<number>(0);
  const [hueRate, setHueRate] = React.useState<number>(30);
  const [publishMode, setPublishMode] = React.useState<'clip'|'program'>(() => {
    try { return (localStorage.getItem('prism.publish.mode') as any) || 'clip'; } catch { return 'clip'; }
  });
  const [capByBrightness, setCapByBrightness] = React.useState<boolean>(() => {
    try { return JSON.parse(localStorage.getItem('prism.bake.capByBrightness') || 'false'); } catch { return false; }
  });
  // Playback controls
  const [brightness, setBrightness] = React.useState<number>(255);
  const [gammaX100, setGammaX100] = React.useState<number>(220);
  const rampTimer = React.useRef<any>(null);
  const sendBrightness = React.useCallback(async (val: number) => {
    if (!active) return;
    try {
      await invoke('device_control_brightness', { host: active.host, target: Math.max(0, Math.min(255, Math.round(val))), durationMs: 150 });
    } catch (e) { log('error', `brightness failed: ${String(e)}`); }
  }, [active]);
  const sendGamma = React.useCallback(async (val: number) => {
    if (!active) return;
    try {
      await invoke('device_control_gamma', { host: active.host, gammaX100: Math.max(50, Math.min(500, Math.round(val))), durationMs: 150 });
    } catch (e) { log('error', `gamma failed: ${String(e)}`); }
  }, [active]);
  const throttledSend = (kind: 'b'|'g', val: number) => {
    if (rampTimer.current) clearTimeout(rampTimer.current);
    rampTimer.current = setTimeout(() => { kind==='b' ? sendBrightness(val) : sendGamma(val); }, 150);
  };

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

  function renderCapBadge(label: string, on: boolean, color: string) {
    return (
      <span title={on ? `${label} supported` : `${label} not supported`} style={{
        fontSize: 11,
        padding: '2px 6px',
        borderRadius: 12,
        border: `1px solid ${on ? color : '#444'}`,
        color: on ? color : '#777',
        background: on ? 'transparent' : 'transparent',
      }}>{label}</span>
    );
  }

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
      {/* Capability badges when STATUS.caps present */}
      {statusInfo && typeof statusInfo.caps === 'number' && (
        <div style={{ marginTop: 8, display: 'flex', gap: 6, flexWrap: 'wrap' }}>
          {renderCapBadge('Palette', (statusInfo.caps & CAP_PALETTE) !== 0, '#8e44ad')}
          {renderCapBadge('Resume', (statusInfo.caps & CAP_RESUME) !== 0, '#16a085')}
          {renderCapBadge('Events', (statusInfo.caps & CAP_EVENTS) !== 0, '#2980b9')}
          {renderCapBadge('Compress', (statusInfo.caps & CAP_COMPRESS) !== 0, '#7f8c8d')}
          {renderCapBadge('Program', (statusInfo.caps & CAP_PROGRAM) !== 0, '#27ae60')}
        </div>
      )}
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
              {typeof (statusInfo as any).caps === 'number' && (
                <div title="Device capabilities" style={{ display: 'flex', gap: 6, alignItems: 'center' }}>
                  Caps:
                  <span style={{ marginLeft: 6, padding: '2px 6px', border: '1px solid #333', borderRadius: 4, opacity: ((statusInfo as any).caps & CAP_PALETTE) ? 1 : 0.4 }}>Palette</span>
                </div>
              )}
            </div>
          ) : (
            <div style={{ opacity: 0.7 }}>No status yet.</div>
          )}

          {patterns.length > 0 && (
            <div style={{ marginTop: 12 }}>
              <h4 style={{ margin: '8px 0' }}>Patterns</h4>
              <div style={{ display: 'flex', gap: 6, flexWrap: 'wrap', marginBottom: 8 }}>
                {PRESETS.map((p) => (
                  <button key={p.name} onClick={() => {
                    setSeconds(p.seconds); setFps(p.fps); setColor(p.color); setPaletteStr(p.palette.join(','));
                    setUseGradient(!!p.useGradient);
                    if (p.useGradient && p.grad) { setGradC0(p.grad.c0); setGradC1(p.grad.c1); setGradSpeed(p.grad.speed); }
                    setUseHueShift(!!p.useHueShift);
                    if (p.useHueShift && p.hue) { setHueDeg(p.hue.deg); setHueRate(p.hue.rate); }
                    if (active) {
                      try {
                        const v = { seconds: p.seconds, fps: p.fps, color: p.color, paletteStr: p.palette.join(','), useGradient: !!p.useGradient, grad: p.grad || null, useHueShift: !!p.useHueShift, hue: p.hue || null };
                        localStorage.setItem(`prism.preset.${active.host}`, JSON.stringify(v));
                      } catch { /* ignore */ void 0; }
                    }
                  }}>{p.name}</button>
                ))}
              </div>
              <div style={{ display: 'flex', gap: 8, marginBottom: 8, flexWrap: 'wrap', alignItems: 'center' }}>
                <button onClick={() => active ? listPatterns(active) : undefined}>Refresh</button>
                <button onClick={() => active ? deleteAll(active) : undefined} disabled={!active || connectState[active.host] !== 'CONNECTED'}>Delete All</button>
                {/* Publish mode */}
                <div style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
                  <label style={{ fontSize: 12, opacity: 0.9 }}>Publish
                    <select value={publishMode} onChange={e => { const v = (e.target as HTMLSelectElement).value as any; setPublishMode(v); try { localStorage.setItem('prism.publish.mode', v); } catch { /* ignore */ void 0; } }} style={{ marginLeft: 4 }}>
                      <option value="clip">Clip (.prism)</option>
                      <option value="program" disabled={!statusInfo || typeof statusInfo.caps !== 'number' || (statusInfo.caps & CAP_PROGRAM) === 0}>Program (IR v0.1)</option>
                    </select>
                  </label>
                </div>
                {/* Bake & Upload with controls */}
                <button disabled={!active || upload.phase === 'stream' || upload.phase === 'finalizing' || upload.phase === 'cancelled'} onClick={async () => {
                  if (!active) { addToast('No device', 'error'); return; }
                  try {
                    const { invoke } = await import('@tauri-apps/api/core');
                    // Bake N frames + pack, then upload
                    const { bakeProjectToPrism } = await import('../../lib/bake/bake');
                    const palette = paletteStr.split(',').map(s => s.trim()).filter(Boolean);
                    // Build a simple graph based on node controls
                    const nodes: any = {};
                    if (useGradient) {
                      nodes['grad'] = { id: 'grad', kind: 'Gradient', params: { c0: gradC0, c1: gradC1, speed: gradSpeed }, inputs: {} };
                    } else {
                      nodes['solid'] = { id: 'solid', kind: 'Solid', params: { color }, inputs: {} };
                    }
                    let lastId = useGradient ? 'grad' : 'solid';
                    if (useHueShift) {
                      nodes['hue'] = { id: 'hue', kind: 'HueShift', params: { deg: hueDeg, rate: hueRate }, inputs: { src: lastId } };
                      lastId = 'hue';
                    }
                    // PaletteMap optional (if palette provided)
                    const usePalette = palette.length > 0;
                    if (usePalette) {
                      nodes['pal'] = { id: 'pal', kind: 'PaletteMap', params: {}, inputs: { src: lastId } };
                      lastId = 'pal';
                    }
                    nodes['out'] = { id: 'out', kind: 'ToK1', params: {}, inputs: { src: lastId } };
                    const order = Object.keys(nodes);
                    const graph = { nodes, order, ledCount: 320 };
                    // Compute embedPaletteHeader policy
                    const caps = statusInfo?.caps ?? 0;
                    const embedPaletteHeader = !(devicePaletteBlend && (caps & CAP_PALETTE));
                    // Clip or Program selection (program currently behind firmware; fallback to clip)
                    let bytes: Uint8Array;
                    if (publishMode === 'program') {
                      try {
                        await compileGraphToIR(
                          undefined,
                          embedPaletteHeader
                            ? undefined
                            : (usePalette
                                ? (await import('../../lib/color/oklchLut')).paletteHexToRgbBytes(palette)
                                : undefined)
                        );
                        // Fallback: until firmware supports programs, upload as clip instead
                        const res = await bakeProjectToPrism(graph as any, seconds, fps, 320, palette, embedPaletteHeader, capByBrightness ? brightness : undefined);
                        bytes = res.bytes;
                        addToast('Program mode requires firmware support; publishing clip fallback', 'info');
                      } catch {
                        const res = await bakeProjectToPrism(graph as any, seconds, fps, 320, palette, embedPaletteHeader);
                        bytes = res.bytes;
                      }
                    } else {
                      const res = await bakeProjectToPrism(graph as any, seconds, fps, 320, palette, embedPaletteHeader, capByBrightness ? brightness : undefined);
                      bytes = res.bytes;
                    }
                    await invoke('device_upload', { host: active.host, name: 'baked', bytes: Array.from(bytes) });
                    // Auto-PLAY the uploaded pattern id
                    try { await invoke('device_control_play', { host: active.host, name: 'baked' }); } catch { /* ignore */ void 0; }
                    addToast(`Bake & Upload started (${seconds}s @${fps} FPS)`, 'info');
                  } catch (e:any) { addToast(mapUploadError(String(e)), 'error'); }
                }}>Bake & Upload</button>
                <label style={{ fontSize: 12, opacity: 0.9 }}>Sec
                  <input type="number" min={0.1} step={0.1} value={seconds} onChange={e => setSeconds(Math.max(0.1, parseFloat(e.target.value)||1))} style={{ width: 64, marginLeft: 4 }} />
                </label>
                <label title="If enabled, scales baked pixels so no channel exceeds the current brightness slider." style={{ fontSize: 12, opacity: 0.9, display: 'flex', alignItems: 'center', gap: 4 }}>
                  <input type="checkbox" checked={capByBrightness} onChange={e => { setCapByBrightness(e.target.checked); try { localStorage.setItem('prism.bake.capByBrightness', JSON.stringify(e.target.checked)); } catch { /* ignore */ void 0; } }} /> Cap payload by brightness
                </label>
                <label style={{ fontSize: 12, opacity: 0.9 }}>FPS
                  <input type="number" min={1} step={1} value={fps} onChange={e => { setFps(Math.max(1, parseInt(e.target.value)||120)); if (upload.phase==='cancelled') upload.reset(); }} style={{ width: 64, marginLeft: 4 }} />
                </label>
                <label style={{ fontSize: 12, opacity: 0.9 }}>Color
                  <input type="color" value={color} onChange={e => { setColor(e.target.value); if (upload.phase==='cancelled') upload.reset(); }} style={{ marginLeft: 4 }} />
                </label>
                <label style={{ fontSize: 12, opacity: 0.9, display: 'flex', alignItems: 'center' }}>Palette
                  <input type="text" value={paletteStr} onChange={e => { setPaletteStr(e.target.value); if (upload.phase==='cancelled') upload.reset(); }} placeholder="#ff0000,#00ff00" style={{ width: 240, marginLeft: 4 }} />
                </label>
                <label title="If enabled, prefer on-device palette blending when supported" style={{ fontSize: 12, opacity: 0.9, display: 'flex', alignItems: 'center', gap: 4 }}>
                  <input type="checkbox" checked={devicePaletteBlend} disabled={!statusInfo || (statusInfo && typeof statusInfo.caps === 'number' && (statusInfo.caps & CAP_PALETTE) === 0)} onChange={e => { setDevicePaletteBlend(e.target.checked); try { localStorage.setItem('prism.palette.deviceBlend', JSON.stringify(e.target.checked)); } catch { /* ignore */ void 0; } }} /> Device palette blend
                </label>
                <label style={{ fontSize: 12, opacity: 0.9, display: 'flex', alignItems: 'center', gap: 4 }}>Geometry
                  <select value={geometryId} onChange={e => { setGeometryId(e.target.value); try { localStorage.setItem('prism.geometry.id', e.target.value); } catch { /* ignore */ void 0; } }} style={{ marginLeft: 4 }}>
                    <option value="K1_LGP_v1">K1_LGP_v1</option>
                  </select>
                </label>
                <label style={{ fontSize: 12, opacity: 0.9 }}>
                  <input type="checkbox" checked={useGradient} onChange={e => { setUseGradient(e.target.checked); if (upload.phase==='cancelled') upload.reset(); }} /> Gradient
                </label>
                {useGradient && (
                  <>
                    <label style={{ fontSize: 12, opacity: 0.9 }}>C0
                      <input type="color" value={gradC0} onChange={e => setGradC0(e.target.value)} style={{ marginLeft: 4 }} />
                    </label>
                    <label style={{ fontSize: 12, opacity: 0.9 }}>C1
                      <input type="color" value={gradC1} onChange={e => setGradC1(e.target.value)} style={{ marginLeft: 4 }} />
                    </label>
                    <label style={{ fontSize: 12, opacity: 0.9 }}>Speed
                      <input type="number" min={-5} step={0.05} value={gradSpeed} onChange={e => setGradSpeed(parseFloat(e.target.value)||0)} style={{ width: 80, marginLeft: 4 }} />
                    </label>
                  </>
                )}
                <label style={{ fontSize: 12, opacity: 0.9 }}>
                  <input type="checkbox" checked={useHueShift} onChange={e => setUseHueShift(e.target.checked)} /> Hue Shift
                </label>
                {useHueShift && (
                  <>
                    <label style={{ fontSize: 12, opacity: 0.9 }}>Deg
                      <input type="number" min={-180} max={180} step={1} value={hueDeg} onChange={e => setHueDeg(parseFloat(e.target.value)||0)} style={{ width: 80, marginLeft: 4 }} />
                    </label>
                    <label style={{ fontSize: 12, opacity: 0.9 }}>Rate
                      <input type="number" min={-360} max={360} step={1} value={hueRate} onChange={e => setHueRate(parseFloat(e.target.value)||0)} style={{ width: 80, marginLeft: 4 }} />
                    </label>
                  </>
                )}
                <div style={{ display: 'flex', gap: 8, alignItems: 'center' }}>
                  <label style={{ fontSize: 12, opacity: 0.9 }}>Brightness
                    <input type="range" min={0} max={255} value={brightness} onChange={(e)=>{ const v = parseInt(e.target.value)||0; setBrightness(v); throttledSend('b', v); }} style={{ marginLeft: 6 }} />
                  </label>
                  <label style={{ fontSize: 12, opacity: 0.9 }}>Gamma
                    <input type="range" min={100} max={300} value={gammaX100} onChange={(e)=>{ const v = parseInt(e.target.value)||220; setGammaX100(v); throttledSend('g', v); }} style={{ marginLeft: 6 }} />
                  </label>
                  <button onClick={async ()=>{ if (active) { try { await invoke('device_control_play', { host: active.host, name: 'baked' }); addToast('PLAY', 'info'); } catch (e:any) { addToast(mapUploadError(String(e)), 'error'); } } }} disabled={!active}>Play</button>
                  <button onClick={async ()=>{ if (active) { try { await invoke('device_control_stop', { host: active.host }); addToast('STOP', 'info'); } catch (e:any) { addToast(mapUploadError(String(e)), 'error'); } } }} disabled={!active}>Stop</button>
                </div>
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
          <ProgressPanel />
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
