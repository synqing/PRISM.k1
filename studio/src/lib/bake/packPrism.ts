// Minimal .prism packer (placeholder, simplified header)

export type PackInput = {
  frames: Uint8Array[]; // each frame is RGB bytes for ledCount
  fps: number;
  ledCount: number;
  name?: string;
  // Optional custom palette: up to 16 colors
  paletteHex?: string[]; // e.g., ["#ff0000", "#00ff00", ...]
  paletteRgb?: Uint8Array; // length = 3 * count
};

export type PackResult = { bytes: Uint8Array; stats: { frames: number; size: number; fps: number; ledCount: number } };

function crc32(bytes: Uint8Array): number {
  // simple crc32fast-less fallback (tiny): table-based
  const table = (crc32 as any)._t || ((crc32 as any)._t = (function(){
    const t = new Uint32Array(256);
    for (let i=0;i<256;i++){
      let c=i;
      for (let k=0;k<8;k++) c = (c & 1)? (0xEDB88320 ^ (c>>>1)) : (c>>>1);
      t[i]=c>>>0;
    }
    return t;
  })());
  let crc = 0xFFFFFFFF;
  for (let i=0;i<bytes.length;i++) crc = table[(crc ^ bytes[i]) & 0xFF] ^ (crc >>> 8);
  return (crc ^ 0xFFFFFFFF) >>> 0;
}

export function packPrism(input: PackInput): PackResult {
  const { frames, fps, ledCount } = input;
  // Build optional palette block
  let paletteCount = 0;
  let paletteBlock = new Uint8Array(0);
  if (input.paletteRgb && input.paletteRgb.length >= 3) {
    paletteCount = Math.min(16, Math.floor(input.paletteRgb.length / 3));
    paletteBlock = new Uint8Array(paletteCount * 4);
    for (let i = 0; i < paletteCount; i++) {
      paletteBlock[i*4+0] = input.paletteRgb[i*3+0] ?? 0;
      paletteBlock[i*4+1] = input.paletteRgb[i*3+1] ?? 0;
      paletteBlock[i*4+2] = input.paletteRgb[i*3+2] ?? 0;
      paletteBlock[i*4+3] = 0;
    }
  } else if (input.paletteHex && input.paletteHex.length > 0) {
    const count = Math.min(16, input.paletteHex.length);
    paletteCount = count;
    paletteBlock = new Uint8Array(count * 4);
    const hexToRgb = (hex: string): [number, number, number] => {
      const m = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
      if (!m) return [255,255,255];
      return [parseInt(m[1],16), parseInt(m[2],16), parseInt(m[3],16)];
    };
    for (let i=0;i<count;i++){
      const [r,g,b] = hexToRgb(input.paletteHex[i]);
      paletteBlock[i*4+0] = r; paletteBlock[i*4+1] = g; paletteBlock[i*4+2] = b; paletteBlock[i*4+3] = 0;
    }
  }

  const payloadLen = frames.reduce((a, f) => a + f.length, 0);
  // ADR-009 header parity (64 bytes)
  const header = new Uint8Array(64);
  const dv = new DataView(header.buffer);
  // Magic 'PRSM' LE
  dv.setUint32(0, 0x4D535250, true);
  // version_major=1, version_minor=0, header_size=64
  header[4] = 1; header[5] = 0; dv.setUint16(6, 64, true);
  // effect_id (solid), channel_mode=mirror, flags=LOOP
  header[8] = 0x01; header[9] = 0x00; dv.setUint16(10, 0x0001, true);
  // name (20 bytes, null-padded)
  const nameBytes = new TextEncoder().encode((input.name ?? 'baked').slice(0, 20));
  header.set(nameBytes, 12);
  // brightness/speed/fade/param_count/params[12]
  header[32] = 255; // brightness
  header[33] = Math.max(0, Math.min(255, Math.round((fps/120)*128))); // speed approx
  header[34] = 0; // fade
  header[35] = 0; // param_count
  // palette_id/count/offset/reserved1
  header[48] = 0; header[49] = paletteCount & 0xFF;
  const paletteOffset = paletteCount > 0 ? 64 : 0;
  dv.setUint16(50, paletteOffset, true);
  dv.setUint32(52, 0, true);
  // data_crc32 (palette) if present
  const paletteCrc = paletteCount > 0 ? crc32(paletteBlock) : 0;
  dv.setUint32(56, paletteCrc, true);
  // Compute header CRC32 over bytes 0..59 and write at 60
  const hcrc = crc32(header.subarray(0, 60));
  dv.setUint32(60, hcrc, true);

  // Assemble file: header + optional palette + payload frames
  const bytes = new Uint8Array(64 + paletteBlock.length + payloadLen);
  let off = 0;
  bytes.set(header, off); off += 64;
  if (paletteBlock.length) { bytes.set(paletteBlock, off); off += paletteBlock.length; }
  for (const f of frames) { bytes.set(f, off); off += f.length; }
  // Guard: device max pattern size 256KB (262144 bytes)
  if (bytes.length > 262144) {
    throw new Error(`PATTERN_MAX_SIZE_EXCEEDED: ${bytes.length} > 262144`);
  }
  return { bytes, stats: { frames: frames.length, size: bytes.length, fps, ledCount } };
}

// Experimental: XOR-delta + simple RLE (count,value) pairs over entire payload
function xorDelta(frames: Uint8Array[]): Uint8Array {
  if (frames.length === 0) return new Uint8Array(0);
  const total = frames.reduce((a, f) => a + f.length, 0);
  const out = new Uint8Array(total);
  let off = 0;
  const zero = 0;
  for (let i=0;i<frames.length;i++){
    const prevVal = i===0 ? zero : 0; // using zero baseline for simplicity here
    const frm = frames[i];
    for (let j=0;j<frm.length;j++) out[off++] = frm[j] ^ (prevVal as number);
  }
  return out;
}

function rleEncode(bytes: Uint8Array): Uint8Array {
  const out: number[] = [];
  let i = 0;
  while (i < bytes.length) {
    const val = bytes[i];
    let count = 1;
    while (i + count < bytes.length && bytes[i + count] === val && count < 255) count++;
    out.push(count, val);
    i += count;
  }
  return new Uint8Array(out);
}

export function packPrismCompressed(input: PackInput): PackResult {
  const { frames, fps, ledCount } = input;
  // header (64) + optional palette as in packPrism
  const base = packPrism({ ...input, frames: [] });
  const header = base.bytes.subarray(0, 64);
  const paletteCount = header[49];
  const paletteLen = paletteCount * 4;
  const paletteBlock = base.bytes.subarray(64, 64 + paletteLen);
  const payloadDelta = xorDelta(frames);
  const payloadRle = rleEncode(payloadDelta);
  const bytes = new Uint8Array(64 + paletteLen + payloadRle.length);
  bytes.set(header, 0);
  if (paletteLen) bytes.set(paletteBlock, 64);
  bytes.set(payloadRle, 64 + paletteLen);
  return { bytes, stats: { frames: frames.length, size: bytes.length, fps, ledCount } };
}
