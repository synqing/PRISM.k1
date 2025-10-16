import { describe, it, expect } from 'vitest';
import { buildStaticKey } from './TimelineCanvas';

const state = { zoom: 100, offset: 0, bpm: 120 } as any;
const tracks = [
  { id: 't1', name: 'Track 1', clips: [{ id: 'c1', trackId: 't1', name: 'Clip 1', start: 0, duration: 1000 }] },
];

describe('buildStaticKey', () => {
  it('changes when zoom changes', () => {
    const a = buildStaticKey(state, tracks as any, 800, 240);
    const b = buildStaticKey({ ...state, zoom: 200 } as any, tracks as any, 800, 240);
    expect(a).not.toBe(b);
  });
  it('changes when offset changes', () => {
    const a = buildStaticKey(state, tracks as any, 800, 240);
    const b = buildStaticKey({ ...state, offset: 1 } as any, tracks as any, 800, 240);
    expect(a).not.toBe(b);
  });
  it('changes when clip layout changes', () => {
    const a = buildStaticKey(state, tracks as any, 800, 240);
    const altered = [{ id: 't1', name: 'Track 1', clips: [{ id: 'c1', trackId: 't1', name: 'Clip 1', start: 500, duration: 1000 }] }];
    const b = buildStaticKey(state, altered as any, 800, 240);
    expect(a).not.toBe(b);
  });
});

