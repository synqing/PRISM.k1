import { describe, it, expect } from 'vitest';
import { evaluateFrame } from './runtime';
import type { Graph } from './types';

const makeLut = () => {
  const lut = new Uint8Array(256*3);
  for (let i=0;i<256;i++){ lut[i*3]=i; lut[i*3+1]=0; lut[i*3+2]=255-i; }
  return lut;
};

describe('motion examples', () => {
  it('Wave: AngleField -> SinOsc -> PaletteMap -> ToK1', () => {
    const g: Graph = {
      nodes: {
        angle: { id: 'angle', kind: 'AngleField', params: {}, inputs: {} },
        osc: { id: 'osc', kind: 'SinOsc', params: { freq: 0.5, spatial: 1.0, phase: 0.0 }, inputs: {} },
        mul: { id: 'mul', kind: 'Multiply', params: {}, inputs: { a: 'angle', b: 'osc' } },
        pal: { id: 'pal', kind: 'PaletteMap', params: {}, inputs: { src: 'mul' } },
        out: { id: 'out', kind: 'ToK1', params: {}, inputs: { src: 'pal' } },
      },
      order: ['angle','osc','mul','pal','out'],
      ledCount: 320,
    };
    const frameA = evaluateFrame(g, 0, { lut: makeLut() });
    const frameB = evaluateFrame(g, 0.2, { lut: makeLut() });
    expect(frameA.length).toBe(320*3);
    // content should change over time
    expect(Array.from(frameA)).not.toEqual(Array.from(frameB));
  });

  it('Sinelon: PhaseAccum -> CenterOutMirror -> Fade -> PaletteMap -> ToK1', () => {
    const g: Graph = {
      nodes: {
        phase: { id: 'phase', kind: 'PhaseAccum', params: { speed: 0.4, offset: 0 }, inputs: {} },
        mirror: { id: 'mirror', kind: 'CenterOutMirror', params: {}, inputs: { src: 'phase' } },
        fade: { id: 'fade', kind: 'Fade', params: { amount: 0.9 }, inputs: { src: 'mirror' } },
        pal: { id: 'pal', kind: 'PaletteMap', params: {}, inputs: { src: 'fade' } },
        out: { id: 'out', kind: 'ToK1', params: {}, inputs: { src: 'pal' } },
      },
      order: ['phase','mirror','fade','pal','out'],
      ledCount: 320,
    };
    const frame = evaluateFrame(g, 0.1, { lut: makeLut() });
    // basic invariants
    expect(frame.length).toBe(320*3);
  });

  it('Ripple: DistCenter + Ring + Fade + PaletteMap', () => {
    const g: Graph = {
      nodes: {
        dist: { id: 'dist', kind: 'DistCenter', params: {}, inputs: {} },
        ring: { id: 'ring', kind: 'Ring', params: { radius: 0.0, width: 0.08 }, inputs: { src: 'dist' } },
        fade: { id: 'fade', kind: 'Fade', params: { amount: 0.9 }, inputs: { src: 'ring' } },
        pal: { id: 'pal', kind: 'PaletteMap', params: {}, inputs: { src: 'fade' } },
        out: { id: 'out', kind: 'ToK1', params: {}, inputs: { src: 'pal' } },
      },
      order: ['dist','ring','fade','pal','out'],
      ledCount: 320,
    };
    const frame = evaluateFrame(g, 0.0, { lut: makeLut() });
    expect(frame.length).toBe(320*3);
    // center should be brighter than far edge due to ring params
    const centerBright = frame[160*3];
    const edgeBright = frame[0];
    expect(centerBright).toBeGreaterThanOrEqual(edgeBright);
  });

  it('Interference: Sum of two SinOsc fields then PaletteMap', () => {
    const g: Graph = {
      nodes: {
        sinA: { id: 'sinA', kind: 'SinOsc', params: { freq: 0.2, spatial: 1.0, phase: 0.0 }, inputs: {} },
        sinB: { id: 'sinB', kind: 'SinOsc', params: { freq: 0.33, spatial: 2.0, phase: 0.1 }, inputs: {} },
        add: { id: 'add', kind: 'Add', params: {}, inputs: { a: 'sinA', b: 'sinB' } },
        pal: { id: 'pal', kind: 'PaletteMap', params: {}, inputs: { src: 'add' } },
        out: { id: 'out', kind: 'ToK1', params: {}, inputs: { src: 'pal' } },
      },
      order: ['sinA','sinB','add','pal','out'],
      ledCount: 320,
    };
    const f0 = evaluateFrame(g, 0.0, { lut: makeLut() });
    const f1 = evaluateFrame(g, 0.5, { lut: makeLut() });
    expect(f0.length).toBe(320*3);
    expect(Array.from(f0)).not.toEqual(Array.from(f1));
  });

  it('Confetti: Impulse at center, mirrored and faded', () => {
    const g: Graph = {
      nodes: {
        imp: { id: 'imp', kind: 'Impulse', params: { rate: 5 }, inputs: {} },
        mir: { id: 'mir', kind: 'CenterOutMirror', params: {}, inputs: { src: 'imp' } },
        fade: { id: 'fade', kind: 'Fade', params: { amount: 0.8 }, inputs: { src: 'mir' } },
        pal: { id: 'pal', kind: 'PaletteMap', params: {}, inputs: { src: 'fade' } },
        out: { id: 'out', kind: 'ToK1', params: {}, inputs: { src: 'pal' } },
      },
      order: ['imp','mir','fade','pal','out'],
      ledCount: 320,
    };
    const f0 = evaluateFrame(g, 0.0, { lut: makeLut() });
    const center0 = f0[160*3];
    const f1 = evaluateFrame(g, 0.25, { lut: makeLut() });
    const center1 = f1[160*3];
    expect(center0).not.toBe(center1);
  });
});
