#!/usr/bin/env ts-node
/**
 * Mock WS device for local Studio/testing.
 * - Listens on ws://0.0.0.0:<PORT>/ (default 8081)
 * - Implements TLV framing per SoT: [TYPE:1][LEN_BE:2][PAYLOAD][CRC_BE:4]
 * - STATUS (0x30): returns minimal schema with maxChunk=4089 and optional caps
 * - PUT_BEGIN (0x10): parses, allocs buffer, echoes ack
 * - PUT_DATA (0x11): writes bytes into buffer
 * - PUT_END (0x12): recomputes CRC and echoes ack; validates empty payload
 */
import { WebSocketServer } from 'ws';

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

type Session = { name: string; size: number; crc: number; buf: Uint8Array; received: number } | null;

const port = Number(process.env.MOCK_WS_PORT || 8081);
const caps = Number(process.env.MOCK_CAPS || 0);

const wss = new WebSocketServer({ port });
console.log(`[mock-ws] listening on ws://0.0.0.0:${port}/`);

wss.on('connection', (ws) => {
  console.log('[mock-ws] client connected');
  let sess: Session = null;
  ws.on('message', (data: Buffer) => {
    const buf = new Uint8Array(data);
    if (buf.length < 7) return;
    const typ = buf[0]; const len = (buf[1] << 8) | buf[2];
    const payload = buf.slice(3, 3 + len);
    const rcrc = (buf[3+len] << 24) | (buf[4+len] << 16) | (buf[5+len] << 8) | buf[6+len];
    const calc = crc32(buf.slice(0, 3 + len));
    if (rcrc !== calc) {
      console.warn('[mock-ws] CRC mismatch, dropping');
      return;
    }
    if (typ === 0x30) {
      // STATUS: [verlen_le:u32][ver][ledCount_le:u16][avail_le:u32][maxChunk_le:u16][caps_le:u32?]
      const ver = new TextEncoder().encode('mock-1.0.0');
      const payload = new Uint8Array(4 + ver.length + 2 + 4 + 2 + 4);
      const dv = new DataView(payload.buffer);
      dv.setUint32(0, ver.length, true); payload.set(ver, 4);
      let off = 4 + ver.length; dv.setUint16(off, 320, true); off += 2;
      dv.setUint32(off, 1024*1024, true); off += 4; // avail
      dv.setUint16(off, 4089, true); off += 2; // maxChunk
      dv.setUint32(off, caps >>> 0, true);
      ws.send(buildTLV(0x30, payload));
    } else if (typ === 0x10) {
      // PUT_BEGIN: [name_len][name][size_be][crc_be]
      const nameLen = payload[0]; const name = new TextDecoder().decode(payload.slice(1, 1+nameLen));
      const dv = new DataView(payload.buffer, payload.byteOffset);
      const size = dv.getUint32(1+nameLen, false); const crc = dv.getUint32(1+nameLen+4, false);
      sess = { name, size, crc, buf: new Uint8Array(size), received: 0 };
      ws.send(buildTLV(0x30, new Uint8Array())); // ack with empty STATUS
    } else if (typ === 0x11) {
      if (!sess) return;
      const dv = new DataView(payload.buffer, payload.byteOffset);
      const off = dv.getUint32(0, false);
      const data = payload.slice(4);
      if (off + data.length <= (sess as any).size) {
        (sess as any).buf.set(data, off);
        if (off + data.length > (sess as any).received) (sess as any).received = off + data.length;
      }
    } else if (typ === 0x12) {
      if (!sess) return;
      if (payload.length !== 0) console.warn('[mock-ws] PUT_END payload should be empty');
      const c = crc32((sess as any).buf);
      if (c !== (sess as any).crc || (sess as any).received !== (sess as any).size) {
        console.warn('[mock-ws] CRC or size mismatch');
      }
      ws.send(buildTLV(0x30, new Uint8Array())); // final ack
      sess = null;
    }
  });
  ws.on('close', () => console.log('[mock-ws] client disconnected'));
});

process.on('SIGINT', () => { console.log('bye'); process.exit(0); });

