import { describe, it, expect } from 'vitest';
import { evaluateFrame } from './runtime';

describe('graph performance', () => {
  it('evaluates <=16.7ms per frame at 320 LEDs for Solid', () => {
    const g: any = { ledCount: 320, nodes: { solid: { id: 'solid', kind: 'Solid', params: { color: '#123456' }, inputs: {} }, out: { id: 'out', kind: 'ToK1', params: {}, inputs: { src: 'solid' } } }, order: ['solid','out'] };
    const t0 = performance.now();
    const n = 60;
    for (let i=0;i<n;i++) evaluateFrame(g, i/120, {});
    const dt = (performance.now() - t0) / n;
    // Allow headroom to avoid flakiness in some environments
    expect(dt).toBeLessThan(8);
  });
});

