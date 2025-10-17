import React from 'react';
import { useUploadStore } from '../../stores/upload';

export default function ProgressPanel() {
  const s = useUploadStore();
  React.useEffect(() => { s.subscribe(); }, []);
  if (s.phase === 'idle') return null;
  const eta = s.bytesPerSec > 0 && s.totalBytes > 0 ? Math.max(0, (s.totalBytes - s.bytesSent) / s.bytesPerSec) : 0;
  return (
    <div style={{ marginTop: 12, border: '1px solid #333', borderRadius: 6, padding: 8 }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: 6 }}>
        <strong>Upload</strong>
        <span>{s.phase.toUpperCase()}</span>
      </div>
      <div style={{ height: 6, background: '#222', borderRadius: 4 }}>
        <div style={{ width: `${Math.min(100, Math.max(0, s.percent)).toFixed(1)}%`, height: '100%', background: '#4caf50', borderRadius: 4 }} />
      </div>
      <div style={{ display: 'flex', gap: 12, fontSize: 12, opacity: 0.9, marginTop: 6 }}>
        <span>{s.percent.toFixed(1)}%</span>
        <span>{s.bytesSent}/{s.totalBytes} bytes</span>
        <span>{s.bytesPerSec} B/s (EMA)</span>
        <span>ETA: {isFinite(eta) ? `${Math.round(eta)}s` : '-'}</span>
        {s.phase === 'done' && s.startedAt && s.finishedAt && (
          <span>TTFL: {Math.round((s.finishedAt - s.startedAt)/1000)}s</span>
        )}
        {s.phase === 'stream' && <button onClick={() => s.cancel()} style={{ marginLeft: 'auto' }}>Cancel</button>}
        {(s.phase === 'cancelled' || s.phase === 'error') && <button onClick={() => s.reset()} style={{ marginLeft: 'auto' }}>Reset</button>}
      </div>
      {s.error && <div style={{ color: '#e74c3c', marginTop: 6 }}>{s.error}</div>}
    </div>
  );
}
