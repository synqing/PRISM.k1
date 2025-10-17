#!/usr/bin/env node
/**
 * Minimal CLI helper for uploading .prism patterns and issuing control frames.
 *
 * Usage examples:
 *   node scripts/upload-pattern.mjs --host 192.168.4.1 --file ./pattern.prism --play
 *   node scripts/upload-pattern.mjs --url ws://prism-k1.local/ws --play --pattern breathe
 *   node scripts/upload-pattern.mjs --host 192.168.4.1 --stop
 */

import fs from 'node:fs/promises';
import path from 'node:path';
import process from 'node:process';
import WebSocket from 'ws';
import crc32 from 'crc-32';

const TLV_TYPE_PUT_BEGIN = 0x10;
const TLV_TYPE_PUT_DATA = 0x11;
const TLV_TYPE_PUT_END = 0x12;
const TLV_TYPE_CONTROL = 0x20;
const TLV_TYPE_STATUS = 0x30;
const TLV_TYPE_ERROR = 0x40;

const CONTROL_CMD_PLAY = 0x01;
const CONTROL_CMD_STOP = 0x02;

const DEFAULT_CHUNK_SIZE = 1024;
const ACK_TIMEOUT_MS = 5000;

async function main() {
  const options = parseArgs(process.argv.slice(2));

  if (!options.url) {
    const host = options.host ?? '192.168.4.1';
    const port = options.port ?? 80;
    options.url = `ws://${host}:${port}/ws`;
  }

  if (!options.file && !options.play && !options.stop) {
    printHelp();
    process.exit(1);
  }

  const patternFromFile = options.file
    ? sanitizePatternId(path.basename(options.file, path.extname(options.file)))
    : undefined;
  const patternName = sanitizePatternId(options.pattern ?? patternFromFile ?? '');

  if ((options.play || options.autoPlay) && !patternName) {
    console.error('A pattern identifier is required for --play (use --pattern=NAME).');
    process.exit(1);
  }

  const ws = new WebSocket(options.url);
  ws.binaryType = 'arraybuffer';

  const pending = [];

  ws.on('message', (data) => {
    const buffer = Buffer.from(data);
    const frame = decodeFrame(buffer);
    if (!frame) {
      return;
    }

    if (pending.length > 0) {
      const entry = pending.shift();
      clearTimeout(entry.timer);
      if (frame.type === TLV_TYPE_ERROR) {
        const code = frame.payload[0] ?? 0xff;
        const message = frame.payload.length > 1 ? frame.payload.slice(1).toString('utf8') : '';
        entry.reject(new Error(`device error 0x${code.toString(16)} ${describeError(code)} ${message}`.trim()));
      } else {
        entry.resolve(frame);
      }
    } else {
      logFrame(frame);
    }
  });

  const openPromise = new Promise((resolve, reject) => {
    ws.once('open', resolve);
    ws.once('error', reject);
  });

  try {
    await openPromise;
  } catch (err) {
    console.error('Failed to open WebSocket:', err);
    process.exit(1);
  }

  try {
    if (options.file) {
      await uploadPattern(ws, pending, options.file, patternName, options.chunkSize ?? DEFAULT_CHUNK_SIZE);
    }

    if (options.play || options.autoPlay) {
      await sendPlay(ws, pending, patternName);
    }

    if (options.stop) {
      await sendStop(ws, pending);
    }

    console.log('All operations completed.');
  } catch (err) {
    console.error('Operation failed:', err.message ?? err);
    process.exitCode = 1;
  } finally {
    ws.close();
  }
}

function parseArgs(argv) {
  const opts = {
    chunkSize: DEFAULT_CHUNK_SIZE,
    autoPlay: false,
    play: false,
    stop: false,
  };

  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i];
    if (!arg) continue;

    if (arg === '--host' && i + 1 < argv.length) {
      opts.host = argv[++i];
      continue;
    }
    if (arg.startsWith('--host=')) {
      opts.host = arg.slice(7);
      continue;
    }
    if (arg === '--port' && i + 1 < argv.length) {
      opts.port = Number(argv[++i]);
      continue;
    }
    if (arg.startsWith('--port=')) {
      opts.port = Number(arg.slice(7));
      continue;
    }
    if (arg === '--url' && i + 1 < argv.length) {
      opts.url = argv[++i];
      continue;
    }
    if (arg.startsWith('--url=')) {
      opts.url = arg.slice(6);
      continue;
    }
    if ((arg === '--file' || arg === '--upload') && i + 1 < argv.length) {
      opts.file = argv[++i];
      continue;
    }
    if (arg.startsWith('--file=') || arg.startsWith('--upload=')) {
      const [, value] = arg.split('=');
      opts.file = value;
      continue;
    }
    if (arg === '--pattern' && i + 1 < argv.length) {
      opts.pattern = argv[++i];
      continue;
    }
    if (arg.startsWith('--pattern=')) {
      opts.pattern = arg.slice(10);
      continue;
    }
    if (arg === '--play') {
      opts.play = true;
      continue;
    }
    if (arg === '--stop') {
      opts.stop = true;
      continue;
    }
    if (arg === '--chunk' && i + 1 < argv.length) {
      opts.chunkSize = Number(argv[++i]);
      continue;
    }
    if (arg.startsWith('--chunk=')) {
      opts.chunkSize = Number(arg.slice(8));
      continue;
    }
    if (arg === '--no-play') {
      opts.play = false;
      continue;
    }
    if (arg === '--play-after-upload') {
      opts.autoPlay = true;
      continue;
    }
    if (arg === '--help' || arg === '-h') {
      printHelp();
      process.exit(0);
    }
  }

  return opts;
}

function printHelp() {
  console.log(`Usage:
  node scripts/upload-pattern.mjs --host <ip> --file <pattern.prism> [--pattern name] [--play]
  node scripts/upload-pattern.mjs --url ws://device.local/ws --play --pattern name
  node scripts/upload-pattern.mjs --host <ip> --stop

Options:
  --host <ip>                Device hostname or IP (default 192.168.4.1)
  --port <port>              HTTP/WebSocket port (default 80)
  --url <ws://...>           Override WebSocket URL
  --file, --upload <path>    Upload a .prism file
  --pattern <id>             Override pattern identifier (defaults to filename)
  --play                     Send CONTROL PLAY after upload (or standalone with --pattern)
  --stop                     Send CONTROL STOP
  --chunk <bytes>            Chunk size for PUT_DATA (default 1024)
`);
}

function sanitizePatternId(input) {
  const src = input ?? '';
  const out = [];
  let dotSeen = false;
  for (let i = 0; i < src.length; i += 1) {
    const c = src[i];
    if (c === '.' && !dotSeen) {
      dotSeen = true;
      continue;
    }
    if (c === '/' || c === '\\') {
      continue;
    }
    if (/^[A-Za-z0-9]$/.test(c)) {
      out.push(c.toLowerCase());
    } else if (c === '-' || c === '_') {
      out.push(c);
    } else {
      out.push('_');
    }
  }
  if (out.length === 0) {
    return 'pattern';
  }
  return out.join('').slice(0, 63);
}

function buildFrame(type, payload = Buffer.alloc(0)) {
  const frame = Buffer.alloc(1 + 2 + payload.length + 4);
  frame[0] = type;
  frame.writeUInt16BE(payload.length, 1);
  payload.copy(frame, 3);
  const crc = crc32.buf(frame.slice(0, 3 + payload.length)) >>> 0;
  frame.writeUInt32BE(crc, 3 + payload.length);
  return frame;
}

function decodeFrame(buffer) {
  if (buffer.length < 7) {
    console.warn('Ignoring short frame from device');
    return null;
  }
  const type = buffer[0];
  const length = buffer.readUInt16BE(1);
  if (buffer.length !== 1 + 2 + length + 4) {
    console.warn('Ignoring frame with mismatched length');
    return null;
  }
  const payload = buffer.slice(3, 3 + length);
  const crc = buffer.readUInt32BE(3 + length);
  const calc = crc32.buf(buffer.slice(0, 3 + length)) >>> 0;
  if (crc !== calc) {
    console.warn('Ignoring frame with invalid CRC32');
    return null;
  }
  return { type, payload };
}

function logFrame(frame) {
  if (frame.type === TLV_TYPE_STATUS) {
    const status = frame.payload[0] ?? 0;
    const message = frame.payload.length > 1 ? frame.payload.slice(1).toString('utf8') : '';
    console.log(`STATUS 0x${status.toString(16)} ${message}`.trim());
  } else if (frame.type === TLV_TYPE_ERROR) {
    const code = frame.payload[0] ?? 0xff;
    const message = frame.payload.length > 1 ? frame.payload.slice(1).toString('utf8') : '';
    console.warn(`ERROR 0x${code.toString(16)} ${describeError(code)} ${message}`.trim());
  } else {
    console.log(`Frame type=0x${frame.type.toString(16)} payload=${frame.payload.toString('hex')}`);
  }
}

function describeError(code) {
  switch (code) {
    case 0x00:
      return 'ok';
    case 0x01:
      return 'max-clients';
    case 0x02:
      return 'overflow';
    case 0x03:
      return 'invalid';
    case 0x04:
      return 'storage-full';
    case 0x05:
      return 'not-found';
    default:
      return 'error';
  }
}

function sendFrame(ws, pending, type, payload, expectAck = true, label = 'frame') {
  return new Promise((resolve, reject) => {
    const frame = buildFrame(type, payload);
    let timer;
    const entry = {
      resolve,
      reject,
      label,
      timer: null,
    };

    if (expectAck) {
      timer = setTimeout(() => {
        const idx = pending.indexOf(entry);
        if (idx >= 0) {
          pending.splice(idx, 1);
        }
        reject(new Error(`Timeout waiting for acknowledgment (${label})`));
      }, ACK_TIMEOUT_MS);
      entry.timer = timer;
      pending.push(entry);
    }

    ws.send(frame, (err) => {
      if (err) {
        if (expectAck) {
          const idx = pending.indexOf(entry);
          if (idx >= 0) {
            clearTimeout(entry.timer);
            pending.splice(idx, 1);
          }
        }
        reject(err);
      } else if (!expectAck) {
        resolve();
      }
    });
  });
}

async function uploadPattern(ws, pending, filePath, patternName, chunkSize) {
  console.log(`Uploading '${filePath}' as pattern '${patternName}' ...`);
  const data = await fs.readFile(filePath);
  const size = data.length;
  const crc = crc32.buf(data) >>> 0;

  if (patternName.length === 0 || patternName.length >= 64) {
    throw new Error('Pattern identifier must be between 1 and 63 characters after normalization');
  }

  const nameBuf = Buffer.from(patternName, 'ascii');
  const beginPayload = Buffer.alloc(1 + nameBuf.length + 4 + 4);
  beginPayload[0] = nameBuf.length;
  nameBuf.copy(beginPayload, 1);
  beginPayload.writeUInt32BE(size, 1 + nameBuf.length);
  beginPayload.writeUInt32BE(crc >>> 0, 1 + nameBuf.length + 4);

  await sendFrame(ws, pending, TLV_TYPE_PUT_BEGIN, beginPayload, true, 'PUT_BEGIN');
  console.log('PUT_BEGIN acknowledged');

  for (let offset = 0; offset < size; offset += chunkSize) {
    const chunk = data.slice(offset, Math.min(offset + chunkSize, size));
    const payload = Buffer.alloc(4 + chunk.length);
    payload.writeUInt32BE(offset, 0);
    chunk.copy(payload, 4);
    await sendFrame(ws, pending, TLV_TYPE_PUT_DATA, payload, true, `PUT_DATA offset=${offset}`);
  }
  console.log('PUT_DATA complete');

  await sendFrame(ws, pending, TLV_TYPE_PUT_END, Buffer.alloc(0), true, 'PUT_END');
  console.log('PUT_END acknowledged');
}

async function sendPlay(ws, pending, patternName) {
  console.log(`Sending PLAY for pattern '${patternName}' ...`);
  const nameBuf = Buffer.from(patternName, 'ascii');
  const payload = Buffer.alloc(2 + nameBuf.length);
  payload[0] = CONTROL_CMD_PLAY;
  payload[1] = nameBuf.length;
  nameBuf.copy(payload, 2);
  await sendFrame(ws, pending, TLV_TYPE_CONTROL, payload, true, 'CONTROL_PLAY');
  console.log('PLAY command acknowledged');
}

async function sendStop(ws, pending) {
  console.log('Sending STOP ...');
  const payload = Buffer.from([CONTROL_CMD_STOP]);
  await sendFrame(ws, pending, TLV_TYPE_CONTROL, payload, true, 'CONTROL_STOP');
  console.log('STOP command acknowledged');
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
