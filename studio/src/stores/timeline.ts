import { create } from 'zustand';
import { devtools } from 'zustand/middleware';
import { immer } from 'zustand/middleware/immer';

export type TimelineState = {
  zoom: number; // pixels per second
  offset: number; // seconds, timeline start offset
  playhead: number; // seconds
  playing: boolean;
  bpm: number;
  // actions
  setZoom: (z: number) => void;
  setOffset: (s: number) => void;
  setBpm: (b: number) => void;
  togglePlay: () => void;
  setPlayhead: (s: number) => void;
  tick: (dtSec: number) => void;
};

export const useTimeline = create<TimelineState>()(
  devtools(
    immer((set) => ({
      zoom: 120, // 120 px / second
      offset: 0,
      playhead: 0,
      playing: false,
      bpm: 120,
      setZoom: (z) => set((s) => { s.zoom = Math.max(10, Math.min(2000, z)); }, false, 'timeline/setZoom'),
      setOffset: (o) => set((s) => { s.offset = Math.max(0, o); }, false, 'timeline/setOffset'),
      setBpm: (b) => set((s) => { s.bpm = Math.max(20, Math.min(300, b)); }, false, 'timeline/setBpm'),
      togglePlay: () => set((s) => { s.playing = !s.playing; }, false, 'timeline/togglePlay'),
      setPlayhead: (p) => set((s) => { s.playhead = Math.max(0, p); }, false, 'timeline/setPlayhead'),
      tick: (dt) => set((s) => { if (s.playing) s.playhead = Math.max(0, s.playhead + dt); }, false, 'timeline/tick'),
    }))
  )
);

export function secToPx(sec: number, zoom: number) { return sec * zoom; }
export function pxToSec(px: number, zoom: number) { return px / zoom; }

export function beatLengthSec(bpm: number) { return 60 / bpm; }
