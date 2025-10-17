---
title: Research Session
query: "Design a minimal node-graph store (nodes, pins, params) with undo/redo and hybrid autosave. Compare zundo vs custom temporal (memory/latency), propose TypeScript types, store shape, actions/selectors, and an autosave policy. Include risks, benchmarks (1k ops undo/redo), test plan, and acceptance criteria."
date: 10/17/2025
time: 11:42:58 AM
timestamp: 2025-10-17T03:42:58.423Z
exchanges: 1
---

# Research Session

## Initial Query

**Question:** Design a minimal node-graph store (nodes, pins, params) with undo/redo and hybrid autosave. Compare zundo vs custom temporal (memory/latency), propose TypeScript types, store shape, actions/selectors, and an autosave policy. Include risks, benchmarks (1k ops undo/redo), test plan, and acceptance criteria.

**Response:**

**Overview**

This proposes a minimal, fast node-graph store for PRISM Studio that matches the existing zustand patterns in studio/src/stores/project.ts (temporal history via zundo + markDirty/touch + hybrid autosave) and the simpler timeline store. It covers:

- zundo vs custom temporal history tradeoffs (memory, latency, complexity)
- TypeScript types for nodes/pins/params
- Store shape, actions, and selectors
- Undo/redo API and gesture-friendly grouping
- Hybrid autosave policy aligned with the project store
- Risks, benchmark design (1k ops undo/redo), test plan, and acceptance criteria

The recommended starting point is to use zundo’s temporal because it’s already in use in project.ts and device.ts, provides adequate performance with partialize/equality, and keeps implementation time low. A custom patch-based history is outlined as a future optimization if we outgrow zundo’s snapshot costs.


**zundo vs Custom Temporal**

- zundo temporal
  - Pros
    - Already used in project and device stores for consistent developer ergonomics.
    - Minimal implementation: plug-in with `limit`, `partialize`, `equality`.
    - Works with immer producers and devtools out-of-the-box.
    - Has pause/resume semantics to avoid flooding history during drags (API available in modern zundo).
  - Cons
    - Memory: history is snapshots of the partialized state; O(historyLimit × sizeOf(partializedState)).
    - Latency: per-commit serialization/copy of partialized keys; mitigated by keeping partialized scope tight (e.g., nodes only) and enabling equality to drop redundant frames.
    - Granularity: operation-level grouping requires explicit pause/resume (e.g., drag).

- Custom temporal (patch-based ring buffer)
  - Pros
    - Memory: store compact op records and inverse ops, typically O(historyLimit × averagePatchSize), much smaller than full snapshots for large graphs.
    - Latency: avoid deep copies; apply/revert small, targeted patches; constant-time ring buffer.
    - Flexible grouping: batch small ops naturally (coalesce move events).
  - Cons
    - Complexity: every mutating action must define an inverse; easy to miss edge-cases (e.g., connect cycles, cascading deletes).
    - Debuggability: devtools time-travel doesn’t see “full state” unless we reconstruct.
    - Effort: non-trivial effort to achieve parity with zundo; higher maintenance.

Recommendation: Start with zundo + careful partialize/equality + grouping for gestures. Revisit patch-based history only if benchmarks show history memory or latency becomes a bottleneck (e.g., 10k+ node graphs with long drags) or if we need per-op coalescing beyond what pause/resume gives us.


**TypeScript Types (minimal, strongly typed)**

```ts
// studio/src/stores/graph.types.ts
export type NodeKind =
  | 'Noise2D' | 'Gradient' | 'Solid'
  | 'Rotate' | 'Scale' | 'Mirror'
  | 'Add' | 'Multiply'
  | 'PaletteMap' | 'HueShift' | 'Brightness'
  | 'ToK1';

export type ParamValue = number | string;

export interface GraphNode {
  id: string;
  kind: NodeKind;
  // Keyed by param name. Keep it open to enable node-specific params.
  params: Record<string, ParamValue>;
  // Keyed by input pin name; value is upstream node id or null (disconnected).
  // For minimalism, we allow single upstream per input pin.
  inputs: Record<string, string | null>;
}

// Optional, UI-only extras kept out of history via partialize:
export interface NodeLayout {
  x: number;
  y: number;
}

export interface GraphUI {
  // Track per-node position, expanded state, etc. Not included in undo/redo.
  layout: Record<string, NodeLayout>;
  selection: string[]; // maintain selection order for multi-edit
}

export interface Graph {
  nodes: Record<string, GraphNode>;
  // Stable node order for deterministic rendering and serialization.
  order: string[];
  // Monotonic revision for change tracking (bumped on each edit).
  revision: number;
}

export type PinRef = { nodeId: string; pin: string };
```

Note: This respects the requested minimal `GraphNode` shape while adding a separate UI bucket so we can partialize history to graph data only. Connections are embedded in per-node `inputs`, so we do not need a separate edge list.


**Store Shape**

```ts
// studio/src/stores/graph.ts
import { create } from 'zustand';
import { devtools } from 'zustand/middleware';
import { immer } from 'zustand/middleware/immer';
import { temporal } from 'zundo';
import { throttle } from 'lodash-es';
import type { Graph, GraphNode, NodeLayout } from './graph.types';
import { useProjectStore } from './project'; // to touch() and piggyback autosave

type GraphState = {
  graph: Graph;
  ui: {
    layout: Record<string, NodeLayout>;
    selection: string[];
  };
  isDirty: boolean;

  // CRUD
  addNode: (node: GraphNode, pos?: NodeLayout) => void;
  removeNode: (id: string) => void;
  setParam: (id: string, key: string, value: number | string) => void;

  // Wiring
  connect: (toNodeId: string, toPin: string, fromNodeId: string) => void;
  disconnect: (toNodeId: string, toPin: string) => void;

  // UI-only
  setNodePosition: (id: string, pos: NodeLayout) => void;
  setSelection: (ids: string[]) => void;

  // Utilities
  resetGraph: (g?: Partial<Graph>) => void;
};

export const useGraphStore = create<GraphState>()(
  devtools(
    temporal(
      immer((set, get) => {
        // Autosave scaffold, mirroring studio/src/stores/project.ts
        let editCount = 0;
        let lastSaveAt = 0;
        let idleTimer: any | null = null;

        const doSaveIfNeeded = async () => {
          const { isDirty } = get();
          if (!isDirty) return;
          // Piggyback project save; graph is part of project persistence surface.
          try {
            useProjectStore.getState().touch();
            await useProjectStore.getState().saveProject();
          } catch {
            // optional: fallback draft to localStorage here
          }
          set({ isDirty: false });
          editCount = 0;
          lastSaveAt = Date.now();
        };

        const scheduleIdleFlush = () => {
          if (idleTimer) clearTimeout(idleTimer);
          idleTimer = setTimeout(() => { void doSaveIfNeeded(); }, 2000);
        };

        const autosave = throttle(async () => {
          editCount += 1;
          const now = Date.now();
          const sinceLast = now - lastSaveAt;
          if (editCount >= 10 || sinceLast >= 250) {
            await doSaveIfNeeded();
          }
          scheduleIdleFlush();
        }, 250, { leading: true, trailing: true });

        const markDirty = <T extends any[]>(fn: (...args: T) => void, name?: string) =>
          (...args: T) => {
            fn(...args);
            set(s => { s.isDirty = true; s.graph.revision++; }, false, name ?? 'graph/dirty');
            // Touch project so its recent/save UI reacts immediately.
            useProjectStore.getState().touch();
            void autosave();
          };

        const initial: GraphState = {
          graph: { nodes: {}, order: [], revision: 0 },
          ui: { layout: {}, selection: [] },
          isDirty: false,

          addNode: markDirty((node: GraphNode, pos?: NodeLayout) =>
            set(s => {
              if (s.graph.nodes[node.id]) throw new Error('DUPLICATE_NODE_ID');
              s.graph.nodes[node.id] = node;
              s.graph.order.push(node.id);
              if (pos) s.ui.layout[node.id] = pos;
            }, false, 'graph/addNode')),

          removeNode: markDirty((id: string) =>
            set(s => {
              if (!s.graph.nodes[id]) return;
              // remove inbound references to id
              for (const n of Object.values(s.graph.nodes)) {
                for (const pin of Object.keys(n.inputs)) {
                  if (n.inputs[pin] === id) n.inputs[pin] = null;
                }
              }
              delete s.graph.nodes[id];
              s.graph.order = s.graph.order.filter(x => x !== id);
              delete s.ui.layout[id];
              s.ui.selection = s.ui.selection.filter(x => x !== id);
            }, false, 'graph/removeNode')),

          setParam: markDirty((id, key, value) =>
            set(s => {
              const n = s.graph.nodes[id]; if (!n) throw new Error('NODE_NOT_FOUND');
              n.params[key] = value;
            }, false, 'graph/setParam')),

          connect: markDirty((toNodeId, toPin, fromNodeId) =>
            set(s => {
              const to = s.graph.nodes[toNodeId]; if (!to) throw new Error('NODE_NOT_FOUND');
              if (!s.graph.nodes[fromNodeId]) throw new Error('NODE_NOT_FOUND');
              if (!(toPin in to.inputs)) throw new Error('PIN_NOT_FOUND');
              // cycle guard (simple): disallow if from depends on to
              if (dependsOn(s.graph.nodes, fromNodeId, toNodeId)) throw new Error('CYCLE');
              to.inputs[toPin] = fromNodeId;
            }, false, 'graph/connect')),

          disconnect: markDirty((toNodeId, toPin) =>
            set(s => {
              const to = s.graph.nodes[toNodeId]; if (!to) throw new Error('NODE_NOT_FOUND');
              if (!(toPin in to.inputs)) throw new Error('PIN_NOT_FOUND');
              to.inputs[toPin] = null;
            }, false, 'graph/disconnect')),

          setNodePosition: (id, pos) =>
            set(s => { s.ui.layout[id] = pos; }, false, 'graph/setNodePosition'),

          setSelection: (ids) =>
            set(s => { s.ui.selection = ids; }, false, 'graph/setSelection'),

          resetGraph: markDirty((g?: Partial<Graph>) =>
            set(s => {
              s.graph = { nodes: g?.nodes ?? {}, order: g?.order ?? [], revision: (s.graph.revision + 1) };
              s.ui.layout = {};
              s.ui.selection = [];
            }, false, 'graph/resetGraph')),
        };

        return initial;
      }),
      // Temporal config: only track the graph data in history (not UI).
      {
        limit: 100,
        partialize: (state: GraphState) => ({ graph: state.graph }),
        equality: (a, b) => (a as any).graph === (b as any).graph,
      }
    )
  )
);

// Simple DFS based cycle guard used above
function dependsOn(nodes: Record<string, GraphNode>, start: string, target: string): boolean {
  const seen = new Set<string>();
  const stack = [start];
  while (stack.length) {
    const id = stack.pop()!;
    if (id === target) return true;
    if (seen.has(id)) continue;
    seen.add(id);
    const n = nodes[id]; if (!n) continue;
    // For minimalism assume every node has a single output; all consumers are encoded in other nodes.
    // We walk "up" edges by chasing inputs of the current node to their sources.
    for (const src of Object.values(n.inputs)) {
      if (src) stack.push(src);
    }
  }
  return false;
}

export const { undo: graphUndo, redo: graphRedo, clear: graphHistoryClear } =
  (useGraphStore as any).temporal.getState();
```

Key points:

- history partialize only includes `graph` (nodes/order/revision); UI (layout, selection) is excluded from history for performance and UX (moving nodes shouldn’t fill undo by default). If you want position in history for keyboard-nudges, you can flip this by including positions in `graph` or temporarily pausing history during drags and recording a single entry at pointer-up.
- markDirty mirrors project.ts behavior and piggybacks the project autosave to persist changes consistently with the rest of the app.
- dependsOn prevents simple cycles on connect; it’s cheap because we walk only up edges via inputs. For richer validation (port arity, types) add a NodeDefinitions map and check rules in connect.

Gesture policy with zundo: During drag, either:
- don’t call `setNodePosition` through markDirty (it updates only UI and not tracked in history), or
- if you later include positions in history: call `useGraphStore.temporal.getState().pause()` on pointerdown, update positions on pointermove, and `resume()` once on pointerup to push a single grouped state (zundo provides pause/resume in current versions).


**Actions & Selectors**

Actions above cover the minimal requirements:

- addNode(node, pos?): add a node + optional layout position
- removeNode(id): remove node and clean inbound references
- setParam(id, key, value): update param
- connect(toNodeId, toPin, fromNodeId)
- disconnect(toNodeId, toPin)
- resetGraph()

Useful selectors (to add as plain functions, not storing in state, to avoid re-renders):

```ts
// studio/src/stores/graph.selectors.ts
import { GraphNode } from './graph.types';
import { useGraphStore } from './graph';

// List nodes in order (stable)
export const useNodeList = () =>
  useGraphStore(s => s.graph.order.map(id => s.graph.nodes[id]));

// Build edges list for rendering (toPin is known via inputs key)
export function getEdges(nodes: Record<string, GraphNode>) {
  const edges: { from: string; to: string; toPin: string }[] = [];
  for (const [toId, n] of Object.entries(nodes)) {
    for (const [pin, src] of Object.entries(n.inputs)) {
      if (src) edges.push({ from: src, to: toId, toPin: pin });
    }
  }
  return edges;
}

// Topological order (for evaluator; will throw on cycles)
export function topoSort(nodes: Record<string, GraphNode>): string[] {
  const inDeg: Record<string, number> = {};
  const adj: Record<string, string[]> = {};
  Object.keys(nodes).forEach(id => { inDeg[id] = 0; adj[id] = []; });
  for (const [to, n] of Object.entries(nodes)) {
    for (const src of Object.values(n.inputs)) {
      if (src) { inDeg[to]++; adj[src].push(to); }
    }
  }
  const q = Object.keys(nodes).filter(id => inDeg[id] === 0);
  const order: string[] = [];
  while (q.length) {
    const u = q.shift()!;
    order.push(u);
    for (const v of adj[u]) {
      if (--inDeg[v] === 0) q.push(v);
    }
  }
  if (order.length !== Object.keys(nodes).length) throw new Error('CYCLE_DETECTED');
  return order;
}

// Can connect check: disallow cycles and ensure input exists
export function canConnect(nodes: Record<string, GraphNode>, to: string, toPin: string, from: string) {
  const t = nodes[to]; const f = nodes[from];
  if (!t || !f) return { ok: false, reason: 'NODE_NOT_FOUND' };
  if (!(toPin in t.inputs)) return { ok: false, reason: 'PIN_NOT_FOUND' };
  if (to === from) return { ok: false, reason: 'SELF_LOOP' };
  if (dependsOn(nodes, from, to)) return { ok: false, reason: 'CYCLE' };
  return { ok: true as const };
}
```

GraphCanvas.tsx can consume `useNodeList()`, `getEdges(store.graph.nodes)`, and `setNodePosition`, while wiring UI-only actions outside history (or using pause/resume to group).


**Autosave Policy (Hybrid, aligned with project.ts)**

- Trigger conditions (same as studio/src/stores/project.ts)
  - Throttle window: 250 ms leading + trailing
  - Idle flush: 2 seconds without edits
  - Edit-count threshold: 10 edits since last save
- Behavior
  - On any mutating action wrapped in markDirty:
    - set `isDirty = true` and bump `graph.revision`
    - call `useProjectStore.getState().touch()` so project timestamps update and recent UI reacts
    - schedule autosave: if current project has `filePath`, call `saveProject()` to persist both project and graph together; otherwise, rely on idle flush to prompt or fallback to localStorage draft in development mode (optional).
- Rationale
  - Minimizes disk and Tauri IPC churn while still protecting user work against crashes.
  - Keeps graph persistence cohesive with the project file; avoids new storage surfaces.

Note: If you later move the graph into the `Project` schema (e.g., `project.graph`), make sure project.ts `partialize` in zundo remains limited (it currently partializes `{ project }`). Add graph only after benchmarking because it will then be part of project history snapshots too.


**Undo/Redo Integration**

- Export `{ undo, redo, clear }` from `useGraphStore.temporal.getState()` similar to project.ts and device.ts.
- History scope: `partialize: (state) => ({ graph: state.graph })`
- History limit: `100` initially; adjust to 50–200 after benchmarks.
- Gesture grouping: For position drags:
  - Keep positions outside history (recommended minimal path) OR
  - On pointerdown: `pause()`; on pointerup: `resume()` to record a single frame.

Optional: Add “batch” utility:

```ts
export function withHistoryBatch<T>(fn: () => T): T {
  const t = (useGraphStore as any).temporal.getState();
  t.pause();
  try { return fn(); }
  finally { t.resume(); }
}
```


**Benchmarks (plan for 1k ops undo/redo)**

Goal: validate latency and memory are acceptable on typical dev hardware (M1/Pro, 16GB) and CI runners.

- Setup (vitest)
  - Create graphs with sizes N = 25, 100, 500 nodes; each node uses 2–3 inputs; ensure acyclicity.
  - Perform 1,000 operations consisting of:
    - 600 param edits (setParam on random nodes/keys)
    - 200 connect/disconnect ops across valid pins
    - 200 add/removeNode pairs on throwaway nodes (to simulate structural churn)
  - Measure:
    - Commit latency: total time of 1,000 ops, report mean and p95 per op
    - Undo latency: call `undo()` 1,000 times, measure total and per op
    - Redo latency: call `redo()` 1,000 times, measure
    - Memory: delta of `process.memoryUsage().heapUsed` before/after 1k ops and after undo (approximate)
- Expected ranges (guidance, not hard-coded; verify empirically)
  - zundo (with partialize to `graph` only and equality): 
    - Param-only ops: sub-millisecond per op typical; p95 < 2 ms
    - Connect/disconnect: ~1–2 ms per op at N=100; p95 < 4 ms
    - Add/removeNode: ~2–5 ms per op at N=100; p95 < 8 ms
    - Undo/redo: same order as apply; group by diskless operations (no autosave within benchmark)
  - Memory (N=100, limit=100): Expect < 20–40 MB peak heap overhead during 1k mixed ops.
- Artifacts
  - Save benchmark numbers into console and a JSON under `studio/test-results/.last-graph-bench.json` if desired for trend watching (optional in CI).

Example vitest snippet:

```ts
// studio/src/stores/graph.bench.test.ts
import { describe, it, expect } from 'vitest';
import { useGraphStore, graphUndo, graphRedo } from './graph';

function measure(fn: () => void) {
  const t0 = performance.now();
  fn();
  return performance.now() - t0;
}

describe('graph temporal performance (1k ops)', () => {
  it('1k mixed ops and undo/redo under threshold', () => {
    const store = useGraphStore.getState();
    // seed graph of ~100 nodes...
    // perform 1000 ops...
    const applyMs = measure(() => {
      // ...for loop triggering setParam/connect/disconnect/add/remove
    });
    const undoMs = measure(() => { for (let i = 0; i < 1000; i++) graphUndo(); });
    const redoMs = measure(() => { for (let i = 0; i < 1000; i++) graphRedo(); });

    // Guidance thresholds (tune after first run)
    expect(applyMs).toBeLessThan(2500); // ~2.5 ms/op avg guard
    expect(undoMs).toBeLessThan(2500);
    expect(redoMs).toBeLessThan(2500);
  });
});
```

Note: Keep autosave disabled during perf tests (e.g., expose a test flag or stub out `saveProject`) so file I/O doesn’t skew numbers.


**Test Plan**

- Unit tests (Vitest) in studio/src/stores/graph.test.ts
  - Default graph seeding
    - Initializes with empty `nodes`, `order`, `revision = 0`, `isDirty = false`
  - CRUD flows
    - addNode adds node to `nodes` and `order`; invalid duplicate ID throws
    - removeNode removes node, cleans inbound references, updates order/selection/layout
    - setParam sets correct value and increments `revision`
  - Pin wiring
    - connect sets `to.inputs[pin] = fromId`, rejects missing nodes/pins
    - connect rejects cycles (dependsOn test)
    - disconnect clears input pin
  - Undo/Redo
    - After each CRUD/wiring, `undo` reverts state to previous snapshot
    - Redo reapplies; `clear` empties history
    - Ensure UI-only actions (setNodePosition, setSelection) don’t end up in history by default
  - Autosave coupling
    - After mutating actions, `isDirty = true`; `useProjectStore.touch` called (stub with spy)
    - Hybrid autosave: throttle and idle flush paths call `saveProject` when filePath present (spy)
  - Topological sort selector
    - Returns correct order on DAG; throws on cycles
  - Serialization (optional)
    - Round-trip `graph` portion to/from JSON for future project embedding

- React tests (GraphCanvas hooks later in Task 2)
  - Add node via store, wire pins, verify DOM renders nodes/edges
  - Drag node: position updates, not in history unless explicitly grouped

- Performance tests (separate suite)
  - graph.bench.test.ts as above, skipped by default on CI if too flaky; use `--run` in local or CI nightly

- E2E (Playwright)
  - Smoke: graph persists after reload because project autosave saved recent changes; add 1 node and a connection, reload, assert state


**Acceptance Criteria**

- Functionality
  - Can create, update, wire, and delete nodes using store actions
  - Undo/redo available via exported `graphUndo`/`graphRedo`; history limit enforced
  - UI-only state (layout, selection) not included in history by default (or grouped when enabled)
  - Cycle creation is prevented at connect time; topoSort throws on cycles

- Persistence
  - Graph edits mark the project dirty and trigger hybrid autosave via project.ts policy:
    - Throttle: 250 ms, leading + trailing
    - Idle flush: 2 s
    - Edit-count threshold: 10
  - If project has a `filePath`, graph changes are persisted with project saves

- Performance (initial thresholds; adjust after first run)
  - 1,000 mixed ops apply in under 2.5 s total (avg ≤ 2.5 ms/op) on a typical dev machine
  - Undo 1,000 ops in under 2.5 s; redo under 2.5 s
  - No visible UI jank during typical graph editing (subjective, later verified in Task 2 testing)

- Developer ergonomics
  - API mirrors patterns in studio/src/stores/project.ts and studio/src/stores/device.ts
  - devtools integration works; temporal state export works


**Risks & Mitigations**

- Snapshot memory growth (zundo)
  - Mitigate with `partialize` to graph only, low `limit` (50–100), equality drop of duplicate frames
  - Consider grouping operations (pause/resume) during drags
  - If large graphs or long sessions show high RSS, switch to patch-based temporal

- Gesture flooding of history
  - Keep layout out of history, or pause/resume during gestures
  - Add batch helper (`withHistoryBatch`) for multi-step actions (e.g., paste multiple nodes)

- Cycle detection correctness
  - Current check prevents obvious “from depends on to” for a single connect op; still possible to create cycles via other sequences if logic has holes. Backstop with topoSort throwing in evaluator and surface as UI error.

- Autosave coupling
  - Double-saving risk if both project and graph try to write: we piggyback graph autosave onto project.saveProject; do not implement separate disk writes in graph store.
  - In tests, stub project.saveProject to avoid I/O.

- Undo across autosave boundaries
  - History is in-memory; autosave does not alter history. It’s safe. After reload, the saved project restores the latest committed state (not history). This is consistent with project.ts behavior.

- Param type drift
  - params allow number|string; validating per NodeKind would be future work (NodeDefinitions) to prevent invalid values.


**Future: Patch-Based Temporal (if needed)**

If benchmarks or UX demand finer-grained coalescing or lower memory:

- Implement a ring buffer of `Op` records with `do` and `undo` closures (or structured inverse data).
- Provide `beginGroup()/endGroup()` to coalesce gesture streams.
- Keep devtools snapshotting as an optional diagnostic (e.g., periodic checkpoints) to retain some time travel.

This adds complexity and test burden; defer until we have evidence zundo is insufficient.


**How This Fits Tasks**

- Task 1 (Establish Node Graph State Management)
  - Implement `studio/src/stores/graph.ts` as shown, following devtools/immer/temporal patterns of project/device stores.
  - Add unit tests in studio/src/stores/graph.test.ts mirroring existing test style in project.test.ts and device.test.ts.
  - Expose `graphUndo`,`graphRedo` for UI/keybindings.

- Task 2 (Visual Node Graph Editor)
  - GraphCanvas binds to store actions/selectors and renders nodes/edges; use UI-only layout/selection to avoid history flood; add gesture grouping if positions become part of history.
  - Validate wiring and nodes persist after reload via project autosave (Playwright smoke).

- Task 7–10 (Bake/Upload/Perf)
  - topoSort and stable order feed the evaluator (Task 3, not shown here).
  - Stable autosave ensures graphs persist for Bake & Upload flows and CI perf validation.

If you want, I can translate the snippets into concrete files (graph.ts, graph.types.ts, graph.selectors.ts, graph.test.ts) and wire the tests to run with Vitest next.


---

*Generated by Task Master Research Command*  
*Timestamp: 2025-10-17T03:42:58.423Z*
