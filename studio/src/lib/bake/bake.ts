import { evaluateGraphOnce } from '../graph/evaluator';
import type { Graph as GraphTyped, GraphNode } from '../graph/types';
import { packPrism } from './packPrism';
import { buildOklchLut, paletteHexToRgbBytes } from '../color/oklchLut';

type SimpleGraph = { color?: string; ledCount?: number };

function buildGraph(color: string, ledCount: number, usePalette: boolean): GraphTyped {
  const nodes: Record<string, GraphNode> = {};
  const solid: GraphNode = { id: 'solid', kind: 'Solid', params: { color }, inputs: {} };
  nodes['solid'] = solid;
  if (usePalette) {
    nodes['pal'] = { id: 'pal', kind: 'PaletteMap', params: {}, inputs: { src: 'solid' } };
    nodes['out'] = { id: 'out', kind: 'ToK1', params: {}, inputs: { src: 'pal' } };
  } else {
    nodes['out'] = { id: 'out', kind: 'ToK1', params: {}, inputs: { src: 'solid' } };
  }
  return { nodes, order: ['solid'].concat(usePalette ? ['pal'] : []).concat(['out']), ledCount };
}

export async function bakeProjectToPrism(
  graph: GraphTyped | SimpleGraph,
  seconds = 1,
  fps = 120,
  ledCount = 320,
  paletteStops?: string[],
  embedPaletteHeader: boolean = true,
  brightnessCap?: number,
) {
  const frames = [] as Uint8Array[];
  const total = Math.max(1, Math.floor(seconds * fps));
  // Build LUT/palette header if requested
  let paletteRgb: Uint8Array | undefined;
  let lut: Uint8Array | undefined;
  if (paletteStops && paletteStops.length > 0) {
    // Build LUT (for future evaluator use) and pack palette header section now
    // Note: Evaluator will use LUT directly when we add indexed output
    lut = buildOklchLut(paletteStops, { gamma: 2.2, gamut: 'clip' });
    if (embedPaletteHeader) {
      paletteRgb = paletteHexToRgbBytes(paletteStops);
    }
  }
  const ctx = lut ? { lut } : {};
  // Normalize graph
  let g: GraphTyped;
  if ((graph as any).nodes) {
    g = graph as GraphTyped;
    g.ledCount = ledCount;
  } else {
    const simple = graph as SimpleGraph;
    g = buildGraph(simple.color ?? '#1ec8ff', ledCount, !!paletteRgb);
  }
  for (let i = 0; i < total; i++) {
    // time-based evaluation: t in seconds
    const t = i / fps;
    let frame = evaluateGraphOnce(g, ctx, t);
    // Optional brightness cap (peak-based) in pre-export
    if (typeof brightnessCap === 'number' && isFinite(brightnessCap) && brightnessCap >= 0 && brightnessCap <= 255) {
      let peak = 0;
      for (let j = 0; j < frame.length; j++) if (frame[j] > peak) peak = frame[j];
      if (peak > brightnessCap && peak > 0) {
        const scale = brightnessCap / peak;
        const out = new Uint8Array(frame.length);
        for (let j = 0; j < frame.length; j++) out[j] = Math.min(255, Math.round(frame[j] * scale));
        frame = out;
      }
    }
    frames.push(frame);
  }
  return packPrism({ frames, fps, ledCount, name: 'baked', paletteRgb });
}
