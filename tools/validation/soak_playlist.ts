#!/usr/bin/env ts-node
/**
 * Soak test: upload a generated .prism payload N times to a mock or real device.
 * Usage: MOCK_WS_PORT=8081 HOST=127.0.0.1:8081 ts-node tools/validation/soak_playlist.ts
 */
import WebSocket from 'ws';

function crc32(bytes: Uint8Array): number {
  const table = (crc32 as any)._t || ((crc32 as any)._t = (() => {
    const t = new Uint32Array(256);
    for (let i = 0; i < 256; i++) { let c = i; for (let k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1); t[i] = c >>> 0; }
    return t;
  })());
  let crc = 0xFFFFFFFF; for (let i = 0; i < bytes.length; i++) crc = table[(crc ^ bytes[i]) & 0xFF] ^ (crc >>> 8);
  return (crc ^ 0xFFFFFFFF) >>> 0;
}

function buildTLV(typ: number, payload: Uint8Array): Uint8Array {
  const frame = new Uint8Array(3 + payload.length + 4);
  frame[0] = typ; frame[1] = (payload.length >> 8) & 0xFF; frame[2] = payload.length & 0xFF;
  frame.set(payload, 3);
  const c = crc32(frame.slice(0, 3 + payload.length));
  const off = 3 + payload.length; frame[off] = (c >>> 24) & 0xFF; frame[off+1] = (c >>> 16) & 0xFF; frame[off+2] = (c >>> 8) & 0xFF; frame[off+3] = c & 0xFF;
  return frame;
}

async function uploadOnce(host: string, name: string, bytes: Uint8Array): Promise<void> {
  return new Promise((resolve, reject) => {
    const ws = new WebSocket(`ws://${host}/`);
    ws.on('open', () => {
      // STATUS
      ws.send(buildTLV(0x30, new Uint8Array()));
      // BEGIN
      const begin = new Uint8Array(1 + name.length + 8);
      begin[0] = name.length; begin.set(new TextEncoder().encode(name), 1);
      const dv = new DataView(begin.buffer);
      dv.setUint32(1 + name.length, bytes.length, false);
      dv.setUint32(1 + name.length + 4, crc32(bytes), false);
      ws.send(buildTLV(0x10, begin));
      // DATA
      const max = 4089; let off = 0;
      while (off < bytes.length) {
        const n = Math.min(max, bytes.length - off);
        const pl = new Uint8Array(4 + n);
        const dv2 = new DataView(pl.buffer); dv2.setUint32(0, off, false);
        pl.set(bytes.subarray(off, off + n), 4);
        ws.send(buildTLV(0x11, pl));
        off += n;
      }
      // END
      ws.send(buildTLV(0x12, new Uint8Array()));
    });
    ws.on('message', () => {/* ignore */});
    ws.on('error', (e) => reject(e));
    ws.on('close', () => resolve());
  });
}

async function main() {
  const host = process.env.HOST || '127.0.0.1:8081';
  const runs = Number(process.env.RUNS || 50);
  const led = Number(process.env.LED || 320);
  const frames = Number(process.env.FRAMES || 1);
  const color = [0x1e, 0xc8, 0xff];
  const frame = new Uint8Array(led*3); for (let i=0;i<led;i++){ frame[i*3]=color[0]; frame[i*3+1]=color[1]; frame[i*3+2]=color[2]; }
  const payload = new Uint8Array(frame.length * frames); for (let f=0;f<frames;f++) payload.set(frame, f*frame.length);
  for (let i=0;i<runs;i++){
    await uploadOnce(host, `baked-${i}`, payload);
    process.stdout.write(`.`);
  }
  console.log(`\nSoak complete: ${runs} uploads.`);
}

main().catch((e) => { console.error(e); process.exit(1); });

