export type EasingId =
  | 'linear'
  | 'easeInQuad' | 'easeOutQuad' | 'easeInOutQuad'
  | 'easeInCubic' | 'easeOutCubic' | 'easeInOutCubic'
  | 'easeInQuart' | 'easeOutQuart' | 'easeInOutQuart'
  | 'easeInQuint' | 'easeOutQuint' | 'easeInOutQuint'
  | 'easeInSine' | 'easeOutSine' | 'easeInOutSine'
  | 'easeInExpo' | 'easeOutExpo' | 'easeInOutExpo'
  | 'easeInCirc' | 'easeOutCirc' | 'easeInOutCirc';

export type EasingFn = (t: number) => number;

export const clamp01 = (t: number) => (t < 0 ? 0 : t > 1 ? 1 : t);

export const Easings: Record<EasingId, EasingFn> = {
  linear: (t) => t,

  easeInQuad: (t) => t * t,
  easeOutQuad: (t) => t * (2 - t),
  easeInOutQuad: (t) => (t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t),

  easeInCubic: (t) => t * t * t,
  easeOutCubic: (t) => --t * t * t + 1,
  easeInOutCubic: (t) => (t < 0.5 ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1),

  easeInQuart: (t) => t * t * t * t,
  easeOutQuart: (t) => 1 - --t * t * t * t,
  easeInOutQuart: (t) => (t < 0.5 ? 8 * t * t * t * t : 1 - 8 * --t * t * t * t),

  easeInQuint: (t) => t * t * t * t * t,
  easeOutQuint: (t) => 1 + --t * t * t * t * t,
  easeInOutQuint: (t) => (t < 0.5 ? 16 * t * t * t * t * t : 1 + 16 * --t * t * t * t * t),

  easeInSine: (t) => 1 - Math.cos((t * Math.PI) / 2),
  easeOutSine: (t) => Math.sin((t * Math.PI) / 2),
  easeInOutSine: (t) => 0.5 * (1 - Math.cos(Math.PI * t)),

  easeInExpo: (t) => (t === 0 ? 0 : Math.pow(2, 10 * (t - 1))),
  easeOutExpo: (t) => (t === 1 ? 1 : 1 - Math.pow(2, -10 * t)),
  easeInOutExpo: (t) =>
    t === 0
      ? 0
      : t === 1
      ? 1
      : t < 0.5
      ? Math.pow(2, 10 * (2 * t - 1)) / 2
      : (2 - Math.pow(2, -10 * (2 * t - 1))) / 2,

  easeInCirc: (t) => 1 - Math.sqrt(1 - t * t),
  easeOutCirc: (t) => Math.sqrt(1 - (t - 1) * (t - 1)),
  easeInOutCirc: (t) => (t < 0.5 ? (1 - Math.sqrt(1 - (2 * t) ** 2)) / 2 : (1 + Math.sqrt(1 - (2 * t - 1) ** 2)) / 2),
};

export function cubicBezier(p0x: number, p0y: number, p1x: number, p1y: number): EasingFn {
  // Using a simple approximation by binary search to invert x(t) → t
  // Control points are (0,0), (p0x,p0y), (p1x,p1y), (1,1)
  const cx = 3 * p0x;
  const bx = 3 * (p1x - p0x) - cx;
  const ax = 1 - cx - bx;
  const cy = 3 * p0y;
  const by = 3 * (p1y - p0y) - cy;
  const ay = 1 - cy - by;
  const sampleX = (t: number) => ((ax * t + bx) * t + cx) * t;
  const sampleY = (t: number) => ((ay * t + by) * t + cy) * t;
  return (x) => {
    x = clamp01(x);
    let t0 = 0, t1 = 1, t = x;
    for (let i = 0; i < 16; i++) {
      const x2 = sampleX(t);
      const dx = x2 - x;
      if (Math.abs(dx) < 1e-5) break;
      if (dx > 0) t1 = t; else t0 = t;
      t = (t0 + t1) / 2;
    }
    return clamp01(sampleY(t));
  };
}

export function getEasing(id: EasingId | EasingFn): EasingFn {
  if (typeof id === 'function') return id;
  return Easings[id];
}

