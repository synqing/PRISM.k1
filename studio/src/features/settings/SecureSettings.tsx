import React from 'react';
import { setSecret, getSecret, deleteSecret } from '../../lib/secure';

export default function SecureSettings({ showLogger, setShowLogger }: { showLogger?: boolean; setShowLogger?: (v: boolean) => void }) {
  const [key, setKey] = React.useState('device_token');
  const [value, setValue] = React.useState('');
  const [loaded, setLoaded] = React.useState<string | null>(null);
  const [status, setStatus] = React.useState<string>('');

  const onSave = async () => {
    await setSecret(key, value);
    setStatus('Saved');
  };
  const onLoad = async () => {
    const v = await getSecret(key);
    setLoaded(v);
    setStatus(v != null ? 'Loaded' : 'Not found');
  };
  const onDelete = async () => {
    await deleteSecret(key);
    setStatus('Deleted');
  };

  return (
    <section id="secure-settings" style={{ padding: 16, border: '1px solid #333', borderRadius: 8 }}>
      <h2>Secure Credentials</h2>
      <div style={{ marginBottom: 8 }}>
        <label>
          <input type="checkbox" checked={!!showLogger} onChange={(e) => setShowLogger && setShowLogger(e.target.checked)} /> Show Logger Panel
        </label>
      </div>
      <div style={{ display: 'flex', gap: 8, alignItems: 'center' }}>
        <label>
          Key:&nbsp;
          <input value={key} onChange={(e) => setKey(e.target.value)} />
        </label>
        <label>
          Value:&nbsp;
          <input value={value} onChange={(e) => setValue(e.target.value)} />
        </label>
        <button onClick={onSave}>Save Secret</button>
        <button onClick={onLoad}>Load Secret</button>
        <button onClick={onDelete}>Delete Secret</button>
        {status && <span style={{ opacity: 0.8 }}>{status}</span>}
      </div>
      {loaded !== null && (
        <p style={{ marginTop: 8, opacity: 0.8 }}>Loaded: {String(loaded)}</p>
      )}
    </section>
  );
}
