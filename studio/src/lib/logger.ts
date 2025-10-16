export type LogEntry = { id: number; ts: number; kind: 'info'|'error'; msg: string };

let LOGS: LogEntry[] = [];
const subs = new Set<(logs: LogEntry[]) => void>();

export function log(kind: 'info'|'error', msg: string) {
  const entry: LogEntry = { id: Date.now() + Math.random(), ts: Date.now(), kind, msg };
  LOGS = [...LOGS, entry].slice(-500);
  subs.forEach((fn) => fn(LOGS));
}

export function subscribe(cb: (logs: LogEntry[]) => void) {
  subs.add(cb);
  cb(LOGS);
  return () => subs.delete(cb);
}

export function clearLogs() {
  LOGS = [];
  subs.forEach((fn) => fn(LOGS));
}

export function getLogs() { return LOGS; }

