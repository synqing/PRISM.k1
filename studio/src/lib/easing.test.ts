import { describe, it, expect } from 'vitest';
import { Easings, cubicBezier } from './easing';

describe('Easings', () => {
  it('linear identity', () => {
    expect(Easings.linear(0)).toBe(0);
    expect(Easings.linear(0.5)).toBe(0.5);
    expect(Easings.linear(1)).toBe(1);
  });
  it('easeInQuad monotonic', () => {
    const f = Easings.easeInQuad;
    expect(f(0)).toBe(0);
    // t^2 monotonic; pick 0.7 which yields 0.49
    expect(f(0.7)).toBeGreaterThan(0.4);
    expect(f(1)).toBe(1);
  });
  it('cubic-bezier maps endpoints', () => {
    const f = cubicBezier(0.25, 0.1, 0.25, 1.0);
    expect(f(0)).toBeCloseTo(0, 5);
    expect(f(1)).toBeCloseTo(1, 5);
  });
});
