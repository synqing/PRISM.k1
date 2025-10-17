import type { EasingId } from './easing';
import { getEasing } from './easing';

export type Keyframe = {
  time: number; // seconds
  value: number; // normalized 0..1 or domain value
  easing?: EasingId; // default linear
};

export type Extrapolate = 'clamp' | 'loop' | 'extend';

export function sortKeyframes(kfs: Keyframe[]): Keyframe[] {
  return [...kfs].sort((a, b) => a.time - b.time);
}

export function evaluateCurve(
  keyframes: Keyframe[],
  t: number,
  opts: { extrapolate?: Extrapolate } = {}
): number {
  const ex = opts.extrapolate ?? 'clamp';
  if (keyframes.length === 0) return 0;
  const kfs = sortKeyframes(keyframes);
  const t0 = kfs[0].time;
  const t1 = kfs[kfs.length - 1].time;

  // Extrapolation
  if (t <= t0) {
    if (ex === 'loop' && t1 > t0) {
      const dur = t1 - t0;
      const tt = ((t - t0) % dur + dur) % dur + t0;
      return evaluateCurve(kfs, tt, { extrapolate: 'clamp' });
    }
    return kfs[0].value;
  }
  if (t >= t1) {
    if (ex === 'loop' && t1 > t0) {
      const dur = t1 - t0;
      const tt = ((t - t0) % dur + dur) % dur + t0;
      return evaluateCurve(kfs, tt, { extrapolate: 'clamp' });
    }
    if (ex === 'extend') {
      // linear extrapolation using last segment slope
      if (kfs.length >= 2) {
        const a = kfs[kfs.length - 2];
        const b = kfs[kfs.length - 1];
        const dt = b.time - a.time || 1e-6;
        const dv = b.value - a.value;
        const slope = dv / dt;
        return b.value + slope * (t - b.time);
      }
    }
    return kfs[kfs.length - 1].value;
  }

  // Find segment
  let i = 0;
  while (i < kfs.length - 1 && !(t >= kfs[i].time && t <= kfs[i + 1].time)) i++;
  const a = kfs[i];
  const b = kfs[i + 1];
  const span = b.time - a.time || 1e-6;
  const nt = (t - a.time) / span; // 0..1
  const easing = getEasing(a.easing ?? 'linear');
  const kt = easing(nt);
  return a.value + (b.value - a.value) * kt;
}

export function insertKeyframe(kfs: Keyframe[], k: Keyframe): Keyframe[] {
  return sortKeyframes([...kfs, k]);
}

export function removeKeyframe(kfs: Keyframe[], time: number): Keyframe[] {
  return kfs.filter((k) => k.time !== time);
}
