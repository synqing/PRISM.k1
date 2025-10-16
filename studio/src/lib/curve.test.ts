import { describe, it, expect } from 'vitest';
import { evaluateCurve, insertKeyframe } from './curve';
import type { Keyframe } from './curve';

describe('Curve evaluation', () => {
  const kfs: Keyframe[] = [
    { time: 0, value: 0, easing: 'linear' },
    { time: 1, value: 1, easing: 'linear' },
  ];

  it('evaluates in-range linearly', () => {
    expect(evaluateCurve(kfs, 0)).toBeCloseTo(0, 6);
    expect(evaluateCurve(kfs, 0.25)).toBeCloseTo(0.25, 6);
    expect(evaluateCurve(kfs, 0.5)).toBeCloseTo(0.5, 6);
    expect(evaluateCurve(kfs, 1)).toBeCloseTo(1, 6);
  });

  it('clamps by default', () => {
    expect(evaluateCurve(kfs, -1)).toBeCloseTo(0, 6);
    expect(evaluateCurve(kfs, 2)).toBeCloseTo(1, 6);
  });

  it('loops when requested', () => {
    expect(evaluateCurve(kfs, 1.25, { extrapolate: 'loop' })).toBeCloseTo(0.25, 6);
  });

  it('supports easing on first keyframe', () => {
    const q: Keyframe[] = [
      { time: 0, value: 0, easing: 'easeInQuad' },
      { time: 1, value: 1 },
    ];
    const v = evaluateCurve(q, 0.7);
    expect(v).toBeGreaterThan(0.4);
  });

  it('insertKeyframe returns sorted frames', () => {
    const withMid = insertKeyframe(kfs, { time: 0.5, value: 0.75 });
    expect(withMid[1].time).toBe(0.5);
  });
});
