import { describe, it, expect } from 'vitest';
import { Registry } from './registry';

const dummy = () => [0,0,0] as [number,number,number];

describe('graph registry samplers', () => {
  it('Solid returns constant color', () => {
    const s = Registry.Solid({}, { color: '#123456' }, {});
    expect(s(0,0)).toEqual([0x12,0x34,0x56]);
    expect(s(10,1)).toEqual([0x12,0x34,0x56]);
  });

  it('Gradient interpolates and animates with speed', () => {
    const g = Registry.Gradient({}, { c0: '#000000', c1: '#ffffff', speed: 0.5 }, {});
    const a = g(0, 0); // near 0 -> dark
    const b = g(319, 0); // near 1 -> bright
    expect(a[0]).toBeLessThan(b[0]);
    // at time shift, mid should change
    const mid0 = g(160, 0);
    const mid1 = g(160, 1); // shifted by speed*1
    // Values differ over time
    expect(mid0[0]).not.toBe(mid1[0]);
  });

  it('Brightness scales outputs', () => {
    const s = () => [200, 150, 100] as [number,number,number];
    const br = Registry.Brightness({ src: s }, { amount: 128 }, {});
    expect(br(0,0)).toEqual([100, 75, 50]);
  });

  it('HueShift rotates hue over time', () => {
    const s = () => [200, 50, 50] as [number,number,number];
    const hs = Registry.HueShift({ src: s }, { deg: 0, rate: 30 }, {});
    const c0 = hs(0,0); const c1 = hs(0,1);
    // Should change
    expect(c0[0] !== c1[0] || c0[1] !== c1[1] || c0[2] !== c1[2]).toBe(true);
  });

  it('PaletteMap maps via LUT', () => {
    // With luminance-based indexing, low-luma colors map to low indices
    const lut = new Uint8Array(256*3);
    // Encode gradient: index -> [index, 0, 255-index]
    for (let i=0;i<256;i++){ lut[i*3]=i; lut[i*3+1]=0; lut[i*3+2]=255-i; }
    const dark = () => [10, 10, 10] as [number,number,number];
    const pmDark = Registry.PaletteMap({ src: dark }, {}, { lut });
    const [r1,g1,b1] = pmDark(0,0);
    expect(g1).toBe(0);
    expect(b1).toBeGreaterThan(r1);

    const bright = () => [240, 240, 240] as [number,number,number];
    const pmBright = Registry.PaletteMap({ src: bright }, {}, { lut });
    const [r2,g2,b2] = pmBright(0,0);
    expect(g2).toBe(0);
    expect(r2).toBeGreaterThan(b2);
  });

  it('Noise2D is deterministic and within [0,255]', () => {
    const n1 = Registry.Noise2D({}, { seed: 42, scale: 8, amplitude: 255 }, {});
    const n2 = Registry.Noise2D({}, { seed: 42, scale: 8, amplitude: 255 }, {});
    const a = Array.from({length: 5}, (_,k) => n1(k, 0.5));
    const b = Array.from({length: 5}, (_,k) => n2(k, 0.5));
    expect(a).toEqual(b);
    for (const [r,g,b3] of a) {
      expect(r).toBeGreaterThanOrEqual(0);
      expect(r).toBeLessThanOrEqual(255);
      expect(r).toBe(g);
      expect(g).toBe(b3);
    }
  });

  it('AngleField increases monotonically across strip', () => {
    const a = Registry.AngleField({}, {}, {});
    const v0 = a(0,0)[0];
    const v1 = a(160,0)[0];
    const v2 = a(319,0)[0];
    expect(v0).toBeLessThanOrEqual(v1);
    expect(v1).toBeLessThanOrEqual(v2);
  });

  it('RadiusField is minimal at center and larger at edges', () => {
    const r = Registry.RadiusField({}, {}, {});
    const center = r(160,0)[0];
    const edge = r(0,0)[0];
    expect(center).toBeLessThan(edge);
  });

  it('SinOsc varies over time', () => {
    const s = Registry.SinOsc({}, { freq: 1.0, spatial: 0.0, phase: 0 }, {});
    const a = s(10, 0)[0];
    const b = s(10, 0.25)[0];
    expect(a).not.toBe(b);
  });

  it('Ring masks near radius', () => {
    // Use a solid white source to test masking
    const white = () => [255,255,255] as [number,number,number];
    const ring = Registry.Ring({ src: white }, { radius: 0.0, width: 0.1 }, {});
    const center = ring(160,0);
    const far = ring(319,0);
    expect(center[0]).toBeGreaterThanOrEqual(200);
    expect(far[0]).toBe(0);
  });

  it('CenterOutMirror averages symmetric samples', () => {
    const grad = (i:number)=> [i%256, i%256, i%256] as [number,number,number];
    const mir = Registry.CenterOutMirror({ src: grad as any }, {}, {});
    const a = mir(100,0);
    const b = mir(219,0); // symmetric around center=160 -> 2*160-1=319; 319-100=219
    expect(a).toEqual(b);
  });
});
