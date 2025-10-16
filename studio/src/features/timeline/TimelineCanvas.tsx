import React from 'react';
import { setupDprCanvas, resizeIfNeeded } from '../../lib/canvas';
import { useTimeline, secToPx, beatLengthSec } from '../../stores/timeline';
import { useProjectStore } from '../../stores/project';
import shallow from 'zustand/shallow';

function useRaf(callback: (dt: number) => void) {
  const ref = React.useRef(0);
  const last = React.useRef<number | null>(null);
  React.useEffect(() => {
    const loop = (t: number) => {
      if (last.current == null) last.current = t;
      const dt = Math.max(0, (t - last.current) / 1000);
      last.current = t;
      callback(dt);
      ref.current = requestAnimationFrame(loop);
    };
    ref.current = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(ref.current);
  }, [callback]);
}

export default function TimelineCanvas() {
  const canvasRef = React.useRef<HTMLCanvasElement | null>(null);
  const state = useTimeline();
  const tracks = useProjectStore((s) => s.project.tracks, shallow);

  // Offscreen static layer buffer for grid + tracks/clips
  const bufferRef = React.useRef<ReturnType<typeof setupDprCanvas> | null>(null);
  const bufferKey = React.useRef<string>('');
  const prevStateRef = React.useRef<{ zoom: number; offset: number; bpm: number; w: number; h: number } | null>(null);

  React.useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if (e.key === ' ') { e.preventDefault(); state.togglePlay(); }
      if (e.key === '+') state.setZoom(state.zoom * 1.1);
      if (e.key === '-') state.setZoom(state.zoom / 1.1);
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [state]);

  const onPointerDown = (e: React.PointerEvent) => {
    const el = canvasRef.current; if (!el) return;
    (e.target as Element).setPointerCapture(e.pointerId);
    const rect = el.getBoundingClientRect();
    const x = e.clientX - rect.left;
    state.setPlayhead(state.offset + x / state.zoom);
  };
  const onWheel = (e: React.WheelEvent) => {
    e.preventDefault();
    const f = e.deltaY < 0 ? 1.1 : 1/1.1;
    state.setZoom(state.zoom * f);
  };

  useRaf((dt) => {
    state.tick(dt);
    const cvs = canvasRef.current; if (!cvs) return;
    const dc = setupDprCanvas(cvs);
    resizeIfNeeded(dc);
    // Rebuild static layer if needed
    maybeRebuildStatic(dc, state, tracks, bufferRef, bufferKey, prevStateRef);
    // Blit static and draw dynamic overlays (playhead)
    if (bufferRef.current) {
      dc.ctx.drawImage(bufferRef.current.canvas, 0, 0);
    }
    drawPlayhead(dc.ctx, state);
  });

  return (
    <div style={{ border: '1px solid #333', borderRadius: 8, overflow: 'hidden' }}>
      <canvas
        ref={canvasRef}
        style={{ width: '100%', height: 240, display: 'block', background: '#0c0c0e' }}
        role="application"
        aria-label="Timeline canvas"
        tabIndex={0}
        onPointerDown={onPointerDown}
        onWheel={onWheel}
      />
      <div style={{ display: 'flex', gap: 12, padding: 8, fontSize: 12, color: '#bbb' }}>
        <span>{state.playing ? 'Playing' : 'Paused'}</span>
        <span>t={state.playhead.toFixed(2)}s</span>
        <span>zoom={state.zoom.toFixed(0)} px/s</span>
        <span>bpm={state.bpm}</span>
      </div>
    </div>
  );
}

function drawPlayhead(ctx: CanvasRenderingContext2D, state: ReturnType<typeof useTimeline.getState>) {
  const { height } = ctx.canvas;
  ctx.save();
  ctx.strokeStyle = '#ffae00';
  ctx.lineWidth = 2;
  const x = Math.floor(secToPx(state.playhead - state.offset, state.zoom)) + 0.5;
  ctx.beginPath();
  ctx.moveTo(x, 0);
  ctx.lineTo(x, height);
  ctx.stroke();
  ctx.restore();
}

function drawGrid(ctx: CanvasRenderingContext2D, state: ReturnType<typeof useTimeline.getState>) {
  const { width, height } = ctx.canvas;
  const secPerBeat = beatLengthSec(state.bpm);
  const majorEvery = 4; // bars of 4 beats
  ctx.save();
  ctx.fillStyle = '#111417';
  ctx.fillRect(0, 0, width, height);
  // vertical ticks
  ctx.strokeStyle = '#242a30';
  ctx.lineWidth = 1;
  const startSec = state.offset;
  const endSec = state.offset + width / state.zoom;
  // find first beat index
  const firstBeat = Math.floor(startSec / secPerBeat);
  for (let bi = firstBeat; ; bi++) {
    const beatSec = bi * secPerBeat;
    if (beatSec > endSec) break;
    const x = Math.floor(secToPx(beatSec - state.offset, state.zoom)) + 0.5;
    const isMajor = bi % majorEvery === 0;
    ctx.globalAlpha = isMajor ? 0.8 : 0.4;
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, height);
    ctx.stroke();
  }
  ctx.restore();
}

// placeholder mapping retained for future refactors
function maybeRebuildStatic(
  onscreen: ReturnType<typeof setupDprCanvas>,
  state: ReturnType<typeof useTimeline.getState>,
  tracks: ReturnType<typeof useProjectStore.getState>['project']['tracks'],
  bufferRef: React.MutableRefObject<ReturnType<typeof setupDprCanvas> | null>,
  keyRef: React.MutableRefObject<string>,
  prevRef: React.MutableRefObject<{ zoom: number; offset: number; bpm: number; w: number; h: number } | null>
) {
  const key = buildStaticKey(state, tracks, onscreen.canvas.width, onscreen.canvas.height);
  // If only offset changed and geometry stable, do a scroll blit redraw for the revealed strip
  const prev = prevRef.current;
  const onlyPan = prev && prev.zoom === state.zoom && prev.bpm === state.bpm && prev.w === onscreen.canvas.width && prev.h === onscreen.canvas.height && keyRef.current !== key;
  if (onlyPan && bufferRef.current) {
    const dxPx = Math.round((prev!.offset - state.offset) * state.zoom);
    const dc = bufferRef.current;
    const ctx = dc.ctx;
    const { width, height } = dc.canvas;
    if (Math.abs(dxPx) < width) {
      // Scroll existing buffer content
      ctx.save();
      if (dxPx !== 0) {
        // drawImage self copy
        if (dxPx > 0) {
          // content moves right: copy left part to right
          ctx.drawImage(dc.canvas, 0, 0, width - dxPx, height, dxPx, 0, width - dxPx, height);
          // redraw newly exposed left strip
          ctx.beginPath();
          ctx.rect(0, 0, dxPx, height);
          ctx.clip();
          ctx.clearRect(0, 0, dxPx, height);
          ctx.restore();
          ctx.save();
          ctx.beginPath(); ctx.rect(0, 0, dxPx, height); ctx.clip();
          drawGrid(ctx, state);
          drawTracks(ctx, state, tracks);
        } else {
          // content moves left: copy right part to left
          const s = -dxPx;
          ctx.drawImage(dc.canvas, s, 0, width - s, height, 0, 0, width - s, height);
          // redraw newly exposed right strip
          ctx.beginPath(); ctx.rect(width - s, 0, s, height); ctx.clip();
          ctx.clearRect(width - s, 0, s, height);
          ctx.restore();
          ctx.save();
          ctx.beginPath(); ctx.rect(width - s, 0, s, height); ctx.clip();
          drawGrid(ctx, state);
          drawTracks(ctx, state, tracks);
        }
      }
      ctx.restore();
      keyRef.current = key;
      prevRef.current = { zoom: state.zoom, offset: state.offset, bpm: state.bpm, w: onscreen.canvas.width, h: onscreen.canvas.height };
      return;
    }
  }
  if (!bufferRef.current || keyRef.current !== key) {
    // (Re)create buffer canvas at DPR size
    const buf = document.createElement('canvas');
    buf.style.width = `${onscreen.canvas.width / onscreen.dpr}px`;
    buf.style.height = `${onscreen.canvas.height / onscreen.dpr}px`;
    const dc = setupDprCanvas(buf);
    dc.canvas.width = onscreen.canvas.width;
    dc.canvas.height = onscreen.canvas.height;
    dc.ctx.setTransform(onscreen.dpr, 0, 0, onscreen.dpr, 0, 0);
    // draw grid + clips
    drawGrid(dc.ctx, state);
    drawTracks(dc.ctx, state, tracks);
    bufferRef.current = dc;
    keyRef.current = key;
    prevRef.current = { zoom: state.zoom, offset: state.offset, bpm: state.bpm, w: onscreen.canvas.width, h: onscreen.canvas.height };
  }
}

export function buildStaticKey(
  state: ReturnType<typeof useTimeline.getState>,
  tracks: ReturnType<typeof useProjectStore.getState>['project']['tracks'],
  w: number,
  h: number
) {
  // Simplified hash: zoom|offset|bpm|size|tracks-min-signature
  let sig = `${state.zoom.toFixed(2)}|${state.offset.toFixed(2)}|${state.bpm}|${w}x${h}`;
  // capture clip layout without heavy stringify
  for (const t of tracks) {
    sig += `|t:${t.id}`;
    for (const c of t.clips) sig += `,c:${c.id}:${c.start}:${c.duration}`;
  }
  return sig;
}

function drawTracks(
  ctx: CanvasRenderingContext2D,
  state: ReturnType<typeof useTimeline.getState>,
  tracks: ReturnType<typeof useProjectStore.getState>['project']['tracks']
) {
  const { width } = ctx.canvas;
  const rowH = 28;
  const gap = 8;
  let y = 0;
  for (const t of tracks) {
    // track background
    ctx.fillStyle = '#0f1115';
    ctx.fillRect(0, y, width, rowH);
    // draw clips
    for (const c of t.clips) {
      const startSec = c.start / 1000;
      const durSec = c.duration / 1000;
      const x = secToPx(startSec - state.offset, state.zoom);
      const w = Math.max(1, durSec * state.zoom);
      // LOD thresholds
      if (w < 2) {
        ctx.fillStyle = '#3a7bd5';
        ctx.fillRect(Math.floor(x), y + 2, 1, rowH - 4);
        continue;
      }
      ctx.fillStyle = '#2c7be5';
      ctx.fillRect(Math.floor(x), y + 4, Math.floor(w), rowH - 8);
      if (w > 80) {
        ctx.fillStyle = '#d6e4ff';
        ctx.font = '12px system-ui, sans-serif';
        ctx.textBaseline = 'middle';
        ctx.save();
        ctx.beginPath();
        ctx.rect(Math.floor(x) + 2, y + 4, Math.floor(w) - 4, rowH - 8);
        ctx.clip();
        ctx.fillText(c.name ?? c.id, Math.floor(x) + 6, y + rowH / 2);
        ctx.restore();
      }
    }
    y += rowH + gap;
  }
}
