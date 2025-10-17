import type { Sampler, CompileContext } from './types';

export type NodeCompiler = (inputs: Record<string, Sampler>, params: Record<string, any>, ctx: CompileContext) => Sampler;

export const Registry: Record<string, NodeCompiler> = {
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
    const h2r = (h: string) => {
      const m = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(h);
      return m ? [parseInt(m[1], 16), parseInt(m[2], 16), parseInt(m[3], 16)] : [255,255,255];
    };
    const a = h2r(c0); const b = h2r(c1);
    return (i, tSec) => {
      // shift gradient position over time by speed
      let t = iNorm(i);
      if (speed !== 0) {
        const shift = (speed * tSec) % 1;
        t = (t + shift);
        t = t - Math.floor(t); // wrap [0,1)
      }
      return [
        Math.round(a[0]*(1-t)+b[0]*t),
        Math.round(a[1]*(1-t)+b[1]*t),
        Math.round(a[2]*(1-t)+b[2]*t),
      ];
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
