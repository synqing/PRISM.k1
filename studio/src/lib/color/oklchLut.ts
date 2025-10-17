// OKLCH LUT builder using Culori v4
// API: buildOklchLut takes array of hex stops and returns 256*RGB8 LUT

import { interpolate, converter } from 'culori';

export type LutOptions = {
  gamma?: number; // default 2.2
  gamut?: 'clip' | 'preserve-hue' | 'compress'; // add compress
};

const rgbConv = converter('rgb');

function clamp01(x: number) { return x < 0 ? 0 : x > 1 ? 1 : x; }

function applyGamma01(v: number, gamma: number) {
  // v in [0,1]
  return Math.pow(clamp01(v), 1 / gamma);
}

// Simple in-module cache keyed by palette+options
let _lutCache: Map<string, Uint8Array> = new Map();
function keyFor(stops: string[], options: LutOptions) {
  return JSON.stringify({ stops, gamma: options.gamma ?? 2.2, gamut: options.gamut ?? 'clip' });
}

export function buildOklchLut(stops: string[], options: LutOptions = {}): Uint8Array {
  const gamma = options.gamma ?? 2.2;
  const gamut = options.gamut ?? 'clip';
  const colors = (stops && stops.length > 0) ? stops : ['#ffffff', '#000000'];
  const cacheKey = keyFor(colors, options);
  const cached = _lutCache.get(cacheKey);
  if (cached) return cached;
  const lut = new Uint8Array(256 * 3);

  // Culori interpolator in OKLCH space
  const interp = interpolate(colors, 'oklch');

  for (let i = 0; i < 256; i++) {
    const t = i / 255;
    const col = interp(t);
    // Convert to sRGB; culori returns channel values in 0..1 (possibly out of gamut)
    // Apply basic gamut strategy
    let rgb: any = rgbConv(col);
    if (!rgb) {
      // Fallback to white if conversion fails
      rgb = { r: 1, g: 1, b: 1 };
    }
    if (gamut === 'clip') {
      rgb = { r: clamp01(rgb.r), g: clamp01(rgb.g), b: clamp01(rgb.b) };
    } else if (gamut === 'preserve-hue') {
      // naive preserve-hue via channel normalization to [0,1]
      rgb = { r: clamp01(rgb.r), g: clamp01(rgb.g), b: clamp01(rgb.b) };
      // (For full fidelity, implement chroma reduction in OKLCH prior to conversion.)
    } else if (gamut === 'compress') {
      // Very naive compression: scale channels down uniformly if any exceeds 1
      const maxc = Math.max(Math.abs(rgb.r), Math.abs(rgb.g), Math.abs(rgb.b), 1);
      rgb = { r: clamp01(rgb.r / maxc), g: clamp01(rgb.g / maxc), b: clamp01(rgb.b / maxc) };
    }
    const rr = Math.round(applyGamma01(rgb.r, gamma) * 255);
    const gg = Math.round(applyGamma01(rgb.g, gamma) * 255);
    const bb = Math.round(applyGamma01(rgb.b, gamma) * 255);
    lut[i * 3 + 0] = rr;
    lut[i * 3 + 1] = gg;
    lut[i * 3 + 2] = bb;
  }
  _lutCache.set(cacheKey, lut);
  return lut;
}

export function paletteHexToRgbBytes(hex: string[]): Uint8Array {
  const clampByte = (n: number) => Math.max(0, Math.min(255, Math.round(n)));
  const out = new Uint8Array(Math.min(16, hex.length) * 3);
  for (let i = 0; i < Math.min(16, hex.length); i++) {
    const conv = rgbConv(hex[i]);
    const r = conv ? clampByte(conv.r * 255) : 255;
    const g = conv ? clampByte(conv.g * 255) : 255;
    const b = conv ? clampByte(conv.b * 255) : 255;
    out[i*3+0] = r; out[i*3+1] = g; out[i*3+2] = b;
  }
  return out;
}
