#!/usr/bin/env ts-node
/**
 * Mock PRISM device WebSocket server for local testing.
 * Accepts TLV frames: PUT_BEGIN (0x10), PUT_DATA (0x11), PUT_END (0x12), CONTROL (0x20), STATUS (0x30).
 * Replies with simple ACK: echoes back the same TLV frame with a valid CRC.
 */
import { WebSocketServer } from 'ws';

function crc32(bytes: Uint8Array): number {
  const table = (crc32 as any)._t || ((crc32 as any)._t = (() => {
    const t = new Uint32Array(256);
    for (let i = 0; i < 256; i++) {
      let c = i;
      for (let k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1);
      t[i] = c >>> 0;
    }
    return t;
  })());
  let crc = 0xFFFFFFFF;
  for (let i = 0; i < bytes.length; i++) crc = table[(crc ^ bytes[i]) & 0xFF] ^ (crc >>> 8);
  return (crc ^ 0xFFFFFFFF) >>> 0;
}

function ack(frame: Uint8Array): Uint8Array {
  const len = 3 + (frame[1] << 8 | frame[2]) + 4;
  const data = frame.slice(0, len - 4);
  const c = crc32(data);
  const out = new Uint8Array(len);
  out.set(data, 0);
  out[len - 4] = (c >>> 24) & 0xFF; out[len - 3] = (c >>> 16) & 0xFF; out[len - 2] = (c >>> 8) & 0xFF; out[len - 1] = c & 0xFF;
  return out;
}

const port = Number(process.env.MOCK_WS_PORT || 8081);
const wss = new WebSocketServer({ port });
console.log(`[mock_ws_device] listening on ws://127.0.0.1:${port}/`);
wss.on('connection', (ws) => {
  ws.on('message', (msg: Buffer) => {
    const buf = new Uint8Array(msg);
    // naive TLV check
    if (buf.length < 7) return;
    const type = buf[0];
    if (type === 0x30) {
      // STATUS: reply with minimal payload encoding maxChunk=4089
      const payload = new Uint8Array([0,0,0,0, 0,0, 0,0,0,0, 0x0F,0xF9]); // vlen=0, ledCount=0, avail=0, maxChunk=4089
      const frame = new Uint8Array(3 + payload.length + 4);
      frame[0] = 0x30; frame[1] = 0; frame[2] = payload.length;
      frame.set(payload, 3);
      const c = crc32(frame.slice(0, 3 + payload.length));
      const off = 3 + payload.length; frame[off] = (c >>> 24) & 0xFF; frame[off+1] = (c >>> 16) & 0xFF; frame[off+2] = (c >>> 8) & 0xFF; frame[off+3] = c & 0xFF;
      ws.send(frame);
      return;
    }
    ws.send(ack(buf));
  });
});

