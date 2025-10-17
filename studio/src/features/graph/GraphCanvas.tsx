import React from 'react';
import { useGraphStore, withHistoryBatch } from '../../stores/graph';
import { NodeDefinitions } from '../../lib/graph/nodeDefinitions';

export default function GraphCanvas() {
  const graph = useGraphStore((s) => s.graph);
  const addNode = useGraphStore((s) => s.addNode);
  const removeNode = useGraphStore((s) => s.removeNode);
  const layout = useGraphStore((s) => s.layout);

  const handleAdd = (kind: keyof typeof NodeDefinitions) => {
    const id = `${kind}-${Math.floor(Math.random()*1e6)}`;
    withHistoryBatch(() => {
      addNode({ id, kind, params: {}, inputs: {} }, { x: 40 + Object.keys(graph.nodes).length * 120, y: 40 });
    });
  };

  return (
    <div style={{ border: '1px solid #333', borderRadius: 8, padding: 8, marginBottom: 16 }}>
      <div style={{ display: 'flex', gap: 8, flexWrap: 'wrap', marginBottom: 8 }}>
        {Object.keys(NodeDefinitions).slice(0, 6).map((k) => (
          <button key={k} onClick={() => handleAdd(k as any)}>{k}</button>
        ))}
      </div>
      <div style={{ position: 'relative', height: 220, background: '#111', borderRadius: 6, overflow: 'hidden' }}>
        {Object.values(graph.nodes).map((n) => (
          <div key={n.id} style={{ position: 'absolute', left: (layout[n.id]?.x ?? 20), top: (layout[n.id]?.y ?? 20), background: '#2d3436', color: '#fff', padding: '6px 8px', borderRadius: 6 }}>
            <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
              <strong>{n.kind}</strong>
              <button onClick={() => removeNode(n.id)} style={{ marginLeft: 8 }}>×</button>
            </div>
          </div>
        ))}
        {Object.keys(graph.nodes).length === 0 && (
          <div style={{ position: 'absolute', left: '50%', top: '50%', transform: 'translate(-50%,-50%)', color: '#888' }}>Add nodes to start building your effect</div>
        )}
      </div>
    </div>
  );
}

