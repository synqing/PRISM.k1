import { create } from 'zustand';
import { devtools } from 'zustand/middleware';
import { immer } from 'zustand/middleware/immer';
import { temporal } from 'zundo';

export type NodeKind = 'Noise2D' | 'Gradient' | 'Solid' | 'Rotate' | 'Scale' | 'Mirror' | 'Add' | 'Multiply' | 'PaletteMap' | 'HueShift' | 'Brightness' | 'ToK1';
export type ParamValue = number | string;

export interface GraphNode { id: string; kind: NodeKind; params: Record<string, ParamValue>; inputs: Record<string, string | null>; }
export type GraphStateData = { nodes: Record<string, GraphNode>; order: string[]; revision: number; outputGeometry?: { width: number; height: number } };

type GraphState = {
  graph: GraphStateData;
  layout: Record<string, { x: number; y: number }>;
  selection: string[];
  addNode: (node: GraphNode, pos?: { x: number; y: number }) => void;
  removeNode: (id: string) => void;
  updateNodeParams: (id: string, key: string, value: ParamValue) => void;
  connectPins: (toNodeId: string, toPin: string, fromNodeId: string) => void;
  setOutputGeometry: (w: number, h: number) => void;
};

function dependsOn(nodes: Record<string, GraphNode>, start: string, target: string): boolean {
  const seen = new Set<string>();
  const stack = [start];
  while (stack.length) {
    const id = stack.pop()!;
    if (id === target) return true;
    if (seen.has(id)) continue; seen.add(id);
    const n = nodes[id]; if (!n) continue;
    for (const src of Object.values(n.inputs||{})) if (src) stack.push(src);
  }
  return false;
}

export const useGraphStore = create<GraphState>()(
  devtools(
    temporal(
      immer((set, _get) => ({
        graph: { nodes: {}, order: [], revision: 0 },
        layout: {},
        selection: [],

        addNode: (node, pos) => set((s) => {
          s.graph.nodes[node.id] = node;
          s.graph.order.push(node.id);
          s.graph.revision++;
          if (pos) s.layout[node.id] = pos;
        }, false, 'graph/addNode'),

        removeNode: (id) => set((s) => {
          delete s.graph.nodes[id];
          s.graph.order = s.graph.order.filter((x) => x !== id);
          for (const n of Object.values(s.graph.nodes)) {
            for (const [pin, src] of Object.entries(n.inputs||{})) if (src === id) n.inputs[pin] = null;
          }
          s.graph.revision++;
          delete s.layout[id];
          s.selection = s.selection.filter((x) => x !== id);
        }, false, 'graph/removeNode'),

        updateNodeParams: (id, key, value) => set((s) => {
          const n = s.graph.nodes[id]; if (!n) return;
          n.params[key] = value;
          s.graph.revision++;
        }, false, 'graph/updateNodeParams'),

        connectPins: (toNodeId, toPin, fromNodeId) => set((s) => {
          if (!s.graph.nodes[toNodeId] || !s.graph.nodes[fromNodeId]) return;
          if (toNodeId === fromNodeId) return;
          // prevent cycles
          const clone = JSON.parse(JSON.stringify(s.graph.nodes)) as Record<string, GraphNode>;
          clone[toNodeId].inputs[toPin] = fromNodeId;
          if (dependsOn(clone, fromNodeId, toNodeId)) return;
          s.graph.nodes[toNodeId].inputs[toPin] = fromNodeId;
          s.graph.revision++;
        }, false, 'graph/connectPins'),

        setOutputGeometry: (w, h) => set((s) => {
          s.graph.outputGeometry = { width: w, height: h };
          s.graph.revision++;
        }, false, 'graph/setOutputGeometry'),
      })),
      {
        limit: 50,
        partialize: (s) => ({ graph: s.graph }),
        equality: (a, b) => (a as any).graph?.revision === (b as any).graph?.revision,
      }
    )
  )
);

export const { undo: graphUndo, redo: graphRedo, clear: graphHistoryClear, pause: graphHistoryPause, resume: graphHistoryResume } = (useGraphStore as any).temporal.getState();

export function withHistoryBatch(fn: () => void) {
  graphHistoryPause(); try { fn(); } finally { graphHistoryResume(); }
}
