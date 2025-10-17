import type { Graph } from './types';
import { evaluateFrame } from './runtime';
import type { Graph } from './types';

// Streaming evaluation API: invokes callback per frame
export async function evaluateGraph(graph: Graph, seconds: number, fps: number, cb: (frame: Uint8Array, index: number) => void, ctx?: { lut?: Uint8Array }) {
  const total = Math.max(1, Math.round(seconds * fps));
  for (let i=0;i<total;i++){
    const t = i / fps;
    const frame = evaluateFrame(graph, t, ctx ?? {});
    cb(frame, i);
    // Yield to event loop to keep UI responsive
    await new Promise(r => setTimeout(r, 0));
  }
}

export function evaluateGraphOnce(graph: Graph, ctx?: { lut?: Uint8Array }, t: number = 0): Uint8Array {
  return evaluateFrame(graph, t, ctx);
}
