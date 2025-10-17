import type { Sampler, CompileContext } from './types';
import { buildOklchLut } from '../color/oklchLut';

export type NodeCompiler = (inputs: Record<string, Sampler>, params: Record<string, any>, ctx: CompileContext) => Sampler;

export const Registry: Record<string, NodeCompiler> = {
  AngleField: () => {
    return (i) => { const v = Math.round(iNorm(i) * 255); return [v,v,v]; };
  },
  RadiusField: () => {
    return (i, _t) => {
      const center = 160; const maxd = Math.max(1, center-1);
      const d = Math.abs(i - center);
      const v = Math.max(0, Math.min(255, Math.round((d / maxd) * 255)));
      return [v,v,v];
    };
  },
  SinOsc: (_inputs, params) => {
    const freq = Number(params.freq ?? 0.5); // Hz
    const spatial = Number(params.spatial ?? 1.0); // cycles along strip
    const phase0 = Number(params.phase ?? 0);
    return (i, t) => {
      const x = iNorm(i);
      const phase = phase0 + freq * t + spatial * x;
      const s = Math.sin(2*Math.PI*phase);
      const v = Math.max(0, Math.min(255, Math.round((s*0.5+0.5)*255)));
      return [v,v,v];
    };
  },
  PhaseAccum: (_inputs, params) => {
    const speed = Number(params.speed ?? 0.2);
    const offset = Number(params.offset ?? 0);
    return (_i, t) => {
      const f = ((speed * t + offset) % 1 + 1) % 1;
      const v = Math.round(f * 255);
      return [v,v,v];
    };
  },
  DistCenter: () => {
    return (i) => {
      const center = 160; const maxd = Math.max(1, center-1);
      const d = Math.abs(i - center);
      const v = Math.max(0, Math.min(255, Math.round((d / maxd) * 255)));
      return [v,v,v];
    };
  },
  Ring: (inputs, params) => {
    const src = inputs.src ?? (()=>[0,0,0]);
    const radius = Number(params.radius ?? 0.3);
    const width = Math.max(0.001, Number(params.width ?? 0.05));
    return (i,t) => {
      const center = 160; const maxd = Math.max(1, center-1);
      const d = Math.abs(i - center) / maxd; // 0..1
      const inBand = Math.abs(d - radius) <= width;
      const [r,g,b] = src(i,t);
      const mask = inBand ? 255 : 0;
      return [Math.round(r*mask/255), Math.round(g*mask/255), Math.round(b*mask/255)];
    };
  },
  Fade: (inputs, params) => {
    const src = inputs.src ?? (()=>[0,0,0]);
    const amount = Math.max(0, Math.min(1, Number(params.amount ?? 0.8)));
    return (i,t) => {
      const [r,g,b] = src(i,t);
      return [Math.round(r*amount), Math.round(g*amount), Math.round(b*amount)];
    };
  },
  CenterOutMirror: (inputs) => {
    const src = inputs.src ?? (()=>[0,0,0]);
    return (i,t) => {
      const center = 160; const mirror = (2*center - 1) - i; // mirror index around center boundary
      const A = src(i,t);
      const B = src(Math.max(0, Math.min(mirror, 319)), t);
      return [Math.round((A[0]+B[0])/2), Math.round((A[1]+B[1])/2), Math.round((A[2]+B[2])/2)];
    };
  },
  Impulse: (_inputs, params) => {
    const rate = Math.max(0, Number(params.rate ?? 1.0)); // impulses per second
    let lastK = -1;
    return (i, t) => {
      const k = Math.floor(t * rate);
      const isHit = k !== lastK;
      if (i === 160 && isHit) {
        lastK = k;
        return [255,255,255];
      }
      return [0,0,0];
    };
  },
  Noise2D: (_inputs, params) => {
    // Deterministic 2D value noise using integer hash; seedable
    const seed = Number(params.seed ?? 1337) | 0;
    const scale = Number(params.scale ?? 16); // larger = smoother
    const amp = Number(params.amplitude ?? 255);
    function hash(x: number, y: number) {
      // 32-bit integer mixing for deterministic pseudo-noise
      let n = (x|0) * 374761393 ^ (y|0) * 668265263 ^ (seed|0) * 1274126177;
      n = (n ^ (n >>> 13)) | 0;
      n = (n * 1274126177) | 0;
      return ((n ^ (n >>> 16)) >>> 0) / 0xFFFFFFFF;
    }
    return (i, tSec) => {
      const x = Math.floor(i / Math.max(1, scale));
      const y = Math.floor((tSec * 60) / Math.max(1, scale));
      const v = hash(x, y);
      const c = Math.max(0, Math.min(255, Math.round(v * amp)));
      return [c, c, c];
    };
  },
  Solid: (_inputs, params) => {
    const hex = (params.color as string) ?? '#ffffff';
    const m = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    const r = m ? parseInt(m[1], 16) : 255;
    const g = m ? parseInt(m[2], 16) : 255;
    const b = m ? parseInt(m[3], 16) : 255;
    return () => [r, g, b];
  },
  Gradient: (_inputs, params) => {
    const c0 = (params.c0 as string) ?? '#000000';
    const c1 = (params.c1 as string) ?? '#ffffff';
    const speed = Number(params.speed ?? 0); // cycles per second across strip
    // Precompute 256*RGB8 OKLCH gradient LUT for perceptual smoothness
    const lut = buildOklchLut([c0, c1], { gamma: 2.2, gamut: 'clip' });
    return (i, tSec) => {
      let t = iNorm(i);
      if (speed !== 0) {
        const shift = (speed * tSec) % 1;
        t = t + shift;
        if (t > 1) t = t - Math.floor(t);
      }
      const idx = Math.max(0, Math.min(255, Math.round(t * 255)));
      const o = idx * 3;
      return [lut[o], lut[o + 1], lut[o + 2]];
    };
  },
  Brightness: (inputs, params) => {
    const src = inputs.src ?? (()=>[255,255,255]);
    const scale = Math.max(0, Math.min(255, Number(params.amount ?? 255))) / 255;
    return (i,t) => {
      const [r,g,b] = src(i,t);
      return [Math.round(r*scale), Math.round(g*scale), Math.round(b*scale)];
    };
  },
  HueShift: (inputs, params) => {
    const src = inputs.src ?? (()=>[255,255,255]);
    const deg = Number(params.deg ?? 0);
    const rate = Number(params.rate ?? 0); // degrees per second
    return (i,t) => rgbShiftHue(src(i,t), deg + rate * t);
  },
  Add: (inputs) => {
    const a = inputs.a ?? (()=>[0,0,0]);
    const b = inputs.b ?? (()=>[0,0,0]);
    return (i,t) => {
      const A = a(i,t), B=b(i,t);
      return [
        Math.min(255, A[0]+B[0]),
        Math.min(255, A[1]+B[1]),
        Math.min(255, A[2]+B[2])
      ];
    };
  },
  Multiply: (inputs) => {
    const a = inputs.a ?? (()=>[255,255,255]);
    const b = inputs.b ?? (()=>[255,255,255]);
    return (i,t) => {
      const A = a(i,t), B=b(i,t);
      return [
        Math.round(A[0]*B[0]/255),
        Math.round(A[1]*B[1]/255),
        Math.round(A[2]*B[2]/255)
      ];
    };
  },
  PaletteMap: (inputs, _params, ctx) => {
    const src = inputs.src ?? (()=>[0,0,0]);
    const lut = ctx.lut;
    if (!lut) return src;
    return (i,t) => {
      const [r,g,b] = src(i,t);
      // Luminance-based index (sRGB luma approximation)
      const lum = 0.2126 * r + 0.7152 * g + 0.0722 * b;
      const idx = Math.max(0, Math.min(255, Math.round(lum)));
      const o = idx*3;
      return [lut[o], lut[o+1], lut[o+2]];
    };
  },
  ToK1: (inputs) => inputs.src ?? (()=>[0,0,0])
};

function iNorm(i: number, total = 320) { return total>1 ? i/(total-1) : 0; }

function rgbShiftHue([r,g,b]: [number,number,number], deg: number): [number,number,number] {
  // quick & dirty RGB hue rotation via matrix (approx)
  const rad = (deg*Math.PI)/180;
  const cosA = Math.cos(rad), sinA = Math.sin(rad);
  const lumR=0.213, lumG=0.715, lumB=0.072;
  const a00 = lumR + (1-lumR)*cosA + (-lumR)*sinA;
  const a01 = lumG + (-lumG)*cosA + (-lumG)*sinA;
  const a02 = lumB + (-lumB)*cosA + (1-lumB)*sinA;
  const a10 = lumR + (-lumR)*cosA + 0.143*sinA;
  const a11 = lumG + (1-lumG)*cosA + 0.140*sinA;
  const a12 = lumB + (-lumB)*cosA + (-0.283)*sinA;
  const a20 = lumR + (-lumR)*cosA + (-(1-lumR))*sinA;
  const a21 = lumG + (-lumG)*cosA + lumG*sinA;
  const a22 = lumB + (1-lumB)*cosA + lumB*sinA;
  const rr = clamp(Math.round(a00*r + a01*g + a02*b));
  const gg = clamp(Math.round(a10*r + a11*g + a12*b));
  const bb = clamp(Math.round(a20*r + a21*g + a22*b));
  return [rr,gg,bb];
}

function clamp(n: number){ return n<0?0:n>255?255:n; }
