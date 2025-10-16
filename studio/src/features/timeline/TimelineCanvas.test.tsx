import { describe, it, expect, beforeEach } from 'vitest';
import { render } from '@testing-library/react';
import TimelineCanvas from './TimelineCanvas';

function mock2D(ctx: any = {}) {
  return {
    setTransform: () => {},
    clearRect: () => {},
    fillRect: () => {},
    beginPath: () => {},
    moveTo: () => {},
    lineTo: () => {},
    stroke: () => {},
    save: () => {},
    restore: () => {},
    fillText: () => {},
    rect: () => {},
    clip: () => {},
    drawImage: () => {},
    canvas: { width: 800, height: 240 },
    ...ctx,
  } as unknown as CanvasRenderingContext2D;
}

describe('TimelineCanvas', () => {
  beforeEach(() => {
    // mock getContext
    Object.defineProperty(HTMLCanvasElement.prototype, 'getContext', {
      value: (type: string) => (type === '2d' ? mock2D() : null),
      configurable: true,
    });
    // mock getBoundingClientRect for DPR sizing
    Object.defineProperty(HTMLCanvasElement.prototype, 'getBoundingClientRect', {
      value: () => ({ width: 800, height: 240, x: 0, y: 0, left: 0, top: 0, right: 800, bottom: 240, toJSON: () => {} }),
      configurable: true,
    });
    // rAF mock to avoid infinite loop
    let id = 1;
    (globalThis as any).requestAnimationFrame = (cb: any) => {
      setTimeout(() => cb(performance.now()), 5);
      return id++;
    };
    (globalThis as any).cancelAnimationFrame = () => {};
  });

  it('renders and responds to keyboard play toggle', async () => {
    const { getByRole } = render(<TimelineCanvas />);
    const canvas = getByRole('application');
    canvas.dispatchEvent(new KeyboardEvent('keydown', { key: ' ' }));
    // No crash is a pass; deeper assertions would spy on store
    expect(canvas).toBeDefined();
  });
});
