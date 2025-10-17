import { evaluateGraphOnce } from '../graph/evaluator';
import type { Graph as GraphTyped, GraphNode } from '../graph/types';
import { packPrism } from './packPrism';
import { buildOklchLut, paletteHexToRgbBytes } from '../color/oklchLut';

type SimpleGraph = { color?: string; ledCount?: number };

function buildGraph(color: string, ledCount: number, usePalette: boolean): GraphTyped {
  const nodes: Record<string, GraphNode> = {};
  const solid: GraphNode = { id: 'solid', kind: 'Solid', params: { color }, inputs: {} };
  const endId = usePalette ? 'pal' : 'out';
  nodes['solid'] = solid;
  if (usePalette) {
    nodes['pal'] = { id: 'pal', kind: 'PaletteMap', params: {}, inputs: { src: 'solid' } };
    nodes['out'] = { id: 'out', kind: 'ToK1', params: {}, inputs: { src: 'pal' } };
  } else {
    nodes['out'] = { id: 'out', kind: 'ToK1', params: {}, inputs: { src: 'solid' } };
  }
  return { nodes, order: ['solid'].concat(usePalette ? ['pal'] : []).concat(['out']), ledCount };
}

export async function bakeProjectToPrism(graph: GraphTyped | SimpleGraph, seconds = 1, fps = 120, ledCount = 320, paletteStops?: string[]) {
  const frames = [] as Uint8Array[];
  const total = Math.max(1, Math.floor(seconds * fps));
  // Build LUT/palette header if requested
  let paletteRgb: Uint8Array | undefined;
  let lut: Uint8Array | undefined;
  if (paletteStops && paletteStops.length > 0) {
    // Build LUT (for future evaluator use) and pack palette header section now
    // Note: Evaluator will use LUT directly when we add indexed output
    lut = buildOklchLut(paletteStops, { gamma: 2.2, gamut: 'clip' });
    paletteRgb = paletteHexToRgbBytes(paletteStops);
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
    frames.push(evaluateGraphOnce(g, ctx, t));
  }
  return packPrism({ frames, fps, ledCount, name: 'baked', paletteRgb });
}
