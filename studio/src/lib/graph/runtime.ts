import type { Graph, GraphNode, Sampler, CompileContext } from './types';
import { Registry } from './registry';
import { NodeDefinitions } from './nodeDefinitions';

export function topoSort(nodes: Record<string, GraphNode>): string[] {
  const inDeg: Record<string, number> = {}; const adj: Record<string, string[]> = {};
  Object.keys(nodes).forEach(id => { inDeg[id]=0; adj[id]=[]; });
  for (const [to,n] of Object.entries(nodes)) {
    for (const src of Object.values(n.inputs)) if (src){ inDeg[to]++; adj[src].push(to); }
  }
  const q = Object.keys(nodes).filter(id=>inDeg[id]===0);
  const order: string[] = [];
  while (q.length){ const u = q.shift()!; order.push(u); for (const v of adj[u]) { if (--inDeg[v]===0) q.push(v); } }
  if (order.length !== Object.keys(nodes).length) throw new Error('CYCLE_DETECTED');
  return order;
}

// simple compile cache keyed by id+params JSON to avoid re-building identical nodes
const _compileCache: Map<string, Sampler> = new Map();

function clampParams(kind: string, params: Record<string, any>): Record<string, any> {
  const def = (NodeDefinitions as any)[kind];
  if (!def) return params;
  const out: Record<string, any> = { ...params };
  for (const [k, spec] of Object.entries<any>(def.params || {})) {
    const v = out[k];
    if (v == null) continue;
    if (spec.type === 'number') {
      const min = spec.min ?? -Infinity;
      const max = spec.max ?? Infinity;
      const num = Number(v);
      out[k] = isFinite(num) ? Math.min(max, Math.max(min, num)) : v;
    }
  }
  return out;
}

export function compileGraph(g: Graph, ctx: CompileContext = {}): Sampler {
  const order = (g.order && g.order.length) ? g.order : topoSort(g.nodes);
  const compiled: Record<string, Sampler> = {};
  for (const id of order) {
    const n = g.nodes[id];
    const comp = Registry[n.kind];
    const inputs: Record<string, Sampler> = {};
    for (const [pin, src] of Object.entries(n.inputs||{})) inputs[pin] = src ? (compiled[src] ?? (()=>[0,0,0])) : (()=>[0,0,0]);
    const params = clampParams(n.kind, n.params||{});
    const cacheKey = `${id}:${n.kind}:${JSON.stringify(params)}`;
    let sampler = _compileCache.get(cacheKey);
    if (!sampler) {
      sampler = comp ? comp(inputs, params, ctx) : (()=>[0,0,0]);
      _compileCache.set(cacheKey, sampler);
    }
    compiled[id] = sampler;
  }
  // Output node is last ToK1 if present, else last in order
  const outId = order.reverse().find(id => g.nodes[id].kind === 'ToK1') ?? order[0];
  return compiled[outId] ?? (()=>[0,0,0]);
}

export function evaluateFrame(g: Graph, t: number, ctx: CompileContext = {}): Uint8Array {
  const ledCount = g.ledCount ?? 320;
  const sampler = compileGraph(g, ctx);
  const frame = new Uint8Array(ledCount*3);
  for (let i=0;i<ledCount;i++){
    const [r,gc,b] = sampler(i, t);
    frame[i*3+0]=r; frame[i*3+1]=gc; frame[i*3+2]=b;
  }
  return frame;
}
