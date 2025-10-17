import { describe, it, expect } from 'vitest';
import { topoSort, compileGraph, evaluateFrame } from './runtime';
import type { Graph } from './types';

describe('graph runtime', () => {
  it('topoSort orders DAG and throws on cycles', () => {
    const g: Graph = {
      nodes: {
        a: { id: 'a', kind: 'Solid', params: { color: '#000000' }, inputs: {} },
        b: { id: 'b', kind: 'Solid', params: { color: '#ffffff' }, inputs: {} },
        c: { id: 'c', kind: 'Add', params: {}, inputs: { a: 'a', b: 'b' } },
      },
      order: [],
    };
    const order = topoSort(g.nodes);
    expect(order.indexOf('a')).toBeLessThan(order.indexOf('c'));
    expect(order.indexOf('b')).toBeLessThan(order.indexOf('c'));
    // Introduce a cycle
    g.nodes['a'].inputs = { src: 'c' } as any;
    expect(() => topoSort(g.nodes)).toThrow();
  });

  it('compileGraph and evaluateFrame produce frame bytes', () => {
    const g: Graph = {
      nodes: {
        solid: { id: 'solid', kind: 'Solid', params: { color: '#112233' }, inputs: {} },
        out: { id: 'out', kind: 'ToK1', params: {}, inputs: { src: 'solid' } },
      },
      order: ['solid','out'],
      ledCount: 8,
    };
    const sampler = compileGraph(g, {});
    const rgb = sampler(0, 0);
    expect(rgb).toEqual([0x11,0x22,0x33]);
    const frame = evaluateFrame(g, 0, {});
    expect(frame.length).toBe(8*3);
  });
});

