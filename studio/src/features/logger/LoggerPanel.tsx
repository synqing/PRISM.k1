import React from 'react';
import { subscribe, clearLogs, getLogs } from '../../lib/logger';
import type { LogEntry } from '../../lib/logger';

export default function LoggerPanel() {
  const [logs, setLogs] = React.useState<LogEntry[]>(getLogs());
  React.useEffect(() => {
    const unsub = subscribe(setLogs);
    return () => { unsub && unsub(); };
  }, []);
  return (
    <section style={{ padding: 16, border: '1px solid #333', borderRadius: 8, marginTop: 16 }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
        <h3 style={{ margin: 0 }}>Logger</h3>
        <button onClick={clearLogs} style={{ marginLeft: 'auto' }}>Clear</button>
      </div>
      <div style={{ maxHeight: 240, overflow: 'auto', marginTop: 8, fontFamily: 'ui-monospace, SFMono-Regular, Menlo, monospace', fontSize: 12 }}>
        {logs.length === 0 ? (
          <div style={{ opacity: 0.7 }}>No logs yet.</div>
        ) : (
          logs.map((l) => (
            <div key={l.id} style={{ opacity: 0.9 }}>
              <span style={{ color: '#7f8c8d' }}>{new Date(l.ts).toLocaleTimeString()} </span>
              <strong>[{l.kind}]</strong> {l.msg}
            </div>
          ))
        )}
      </div>
    </section>
  );
}
