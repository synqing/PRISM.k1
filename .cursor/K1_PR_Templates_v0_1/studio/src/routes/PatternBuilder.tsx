// studio/src/routes/PatternBuilder.tsx (PR TEMPLATE)
import React from "react";
import { NODE_LIBRARY } from "../lib/graph/library";
import type { Graph } from "../lib/graph/types";

export default function PatternBuilder() {
  // TODO: load/save graph state; basic canvas; property pane; Publish split-button
  return (
    <div className="flex h-full">
      <aside className="w-64 border-r p-3">
        <h2 className="font-bold">Nodes</h2>
        <ul>{Object.keys(NODE_LIBRARY).map(k => <li key={k}>{k}</li>)}</ul>
      </aside>
      <main className="flex-1 p-3">
        <div className="mb-3 flex gap-2">
          <button>Publish: Clip</button>
          <button>Publish: Program</button>
        </div>
        <div className="border rounded h-[70vh]">Canvas (TODO)</div>
      </main>
      <aside className="w-72 border-l p-3">
        <h2 className="font-bold">Properties</h2>
        <div>TODO</div>
      </aside>
    </div>
  );
}
