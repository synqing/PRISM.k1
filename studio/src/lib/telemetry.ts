export type TelemetryEvent = {
  ts: number;
  phase: string;
  bytesSent?: number;
  totalBytes?: number;
  percent?: number;
  bytesPerSec?: number;
  ttflMs?: number;
};

export async function appendTelemetry(e: TelemetryEvent) {
  try {
    const w: any = window as any;
    if (!w.__TAURI_INTERNALS__) return; // no-op in web mode
    const { writeTextFile, exists, mkdir } = await import('@tauri-apps/plugin-fs');
    const line = JSON.stringify(e) + '\n';
    const dir = 'logs';
    try { if (!await (exists as any)(dir)) await (mkdir as any)(dir); } catch {}
    const path = 'logs/upload.jsonl';
    // Append by read+concat if append not available
    try {
      const { readTextFile } = await import('@tauri-apps/plugin-fs');
      let prev = '';
      try { prev = await (readTextFile as any)(path); } catch { prev = ''; }
      await (writeTextFile as any)(path, prev + line);
    } catch {
      await (writeTextFile as any)(path, line);
    }
  } catch { /* ignore */ }
}

