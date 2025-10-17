import { useState, useEffect } from 'react';
import { NodeLibrary } from './NodeLibrary';
import { NodeCanvas } from './NodeCanvas';
import { NodeInspector } from './NodeInspector';
import { K1Toolbar } from './K1Toolbar';
import { DraggablePanel } from './DraggablePanel';
import type { NodeData, Wire, Port } from './types';
import { toast } from 'sonner@2.0.3';

// Helper to create sample nodes
function createNode(
  id: string,
  templateId: string,
  position: { x: number; y: number }
): NodeData {
  const templates: Record<string, Partial<NodeData>> = {
    gradient: {
      title: 'Gradient',
      category: 'generator',
      icon: '🌈',
      inputs: [],
      outputs: [
        { id: 'output', label: 'Field', type: 'field' },
      ],
      parameters: [
        { id: 'angle', label: 'Angle', type: 'slider', value: 0, min: 0, max: 360, step: 1 },
        { id: 'start', label: 'Start', type: 'slider', value: 0, min: 0, max: 100, step: 1 },
        { id: 'end', label: 'End', type: 'slider', value: 100, min: 0, max: 100, step: 1 },
      ],
    },
    noise: {
      title: 'Noise',
      category: 'generator',
      icon: '⚡',
      inputs: [],
      outputs: [
        { id: 'output', label: 'Field', type: 'field' },
      ],
      parameters: [
        { id: 'scale', label: 'Scale', type: 'slider', value: 10, min: 1, max: 100, step: 1 },
        { id: 'speed', label: 'Speed', type: 'slider', value: 1, min: 0, max: 10, step: 0.1 },
        { id: 'octaves', label: 'Octaves', type: 'number', value: 3, min: 1, max: 8, step: 1 },
      ],
    },
    'hue-shift': {
      title: 'Hue Shift',
      category: 'color',
      icon: '🎨',
      inputs: [
        { id: 'input', label: 'Color', type: 'color' },
        { id: 'amount', label: 'Amount', type: 'scalar' },
      ],
      outputs: [
        { id: 'output', label: 'Color', type: 'color' },
      ],
      parameters: [
        { id: 'hue', label: 'Hue Shift', type: 'slider', value: 0, min: -180, max: 180, step: 1 },
      ],
    },
    blend: {
      title: 'Blend',
      category: 'combine',
      icon: '🔀',
      inputs: [
        { id: 'a', label: 'A', type: 'color' },
        { id: 'b', label: 'B', type: 'color' },
        { id: 'mix', label: 'Mix', type: 'scalar' },
      ],
      outputs: [
        { id: 'output', label: 'Result', type: 'color' },
      ],
      parameters: [
        { id: 'blend', label: 'Blend', type: 'slider', value: 50, min: 0, max: 100, step: 1 },
        { id: 'mode', label: 'Mode', type: 'select', value: 'mix', options: ['mix', 'add', 'multiply', 'screen'] },
      ],
    },
    'k1-output': {
      title: 'K1 Output',
      category: 'output',
      icon: '📤',
      inputs: [
        { id: 'color', label: 'Color', type: 'color' },
      ],
      outputs: [],
      parameters: [
        { id: 'brightness', label: 'Brightness', type: 'slider', value: 100, min: 0, max: 100, step: 1 },
        { id: 'edge', label: 'Edge', type: 'select', value: 'both', options: ['both', 'left', 'right'] },
      ],
    },
  };

  const template = templates[templateId] || {
    title: templateId,
    category: 'generator' as const,
    icon: '❓',
    inputs: [],
    outputs: [{ id: 'output', label: 'Output', type: 'output' as const }],
    parameters: [],
  };

  return {
    id,
    position,
    compact: false,
    ...template,
  } as NodeData;
}

export function LightLab() {
  const [nodes, setNodes] = useState<NodeData[]>([
    createNode('node-1', 'gradient', { x: 100, y: 100 }),
    createNode('node-2', 'hue-shift', { x: 400, y: 150 }),
    createNode('node-3', 'k1-output', { x: 700, y: 180 }),
  ]);

  const [wires, setWires] = useState<Wire[]>([]);
  const [selectedNodeId, setSelectedNodeId] = useState<string | null>('node-1');
  const [playing, setPlaying] = useState(false);
  const [clipboard, setClipboard] = useState<NodeData | null>(null);

  const handleAddNode = (templateId: string) => {
    const newNode = createNode(
      `node-${Date.now()}`,
      templateId,
      { x: 300, y: 200 + nodes.length * 50 }
    );
    setNodes([...nodes, newNode]);
    setSelectedNodeId(newNode.id);
    toast.success(`Added ${newNode.title} node`);
  };

  const handleNodeMove = (nodeId: string, position: { x: number; y: number }) => {
    setNodes((prev) =>
      prev.map((node) => (node.id === nodeId ? { ...node, position } : node))
    );
  };

  const handleNodeDelete = (nodeId: string) => {
    setNodes((prev) => prev.filter((node) => node.id !== nodeId));
    setWires((prev) =>
      prev.filter((wire) => wire.from.nodeId !== nodeId && wire.to.nodeId !== nodeId)
    );
    if (selectedNodeId === nodeId) {
      setSelectedNodeId(null);
    }
    toast.success('Node deleted');
  };

  const handleWireCreate = (wire: Omit<Wire, 'id'>) => {
    const fromNode = nodes.find((n) => n.id === wire.from.nodeId);
    const fromPort = fromNode?.outputs.find((p) => p.id === wire.from.portId);

    if (fromPort) {
      const newWire: Wire = {
        id: `wire-${Date.now()}`,
        ...wire,
        type: fromPort.type,
      };
      setWires([...wires, newWire]);
      toast.success('Connection created');
    }
  };

  const handleWireDelete = (wireId: string) => {
    setWires((prev) => prev.filter((wire) => wire.id !== wireId));
    toast.success('Connection deleted');
  };

  const handleParameterChange = (
    nodeId: string,
    parameterId: string,
    value: number | string | boolean
  ) => {
    setNodes((prev) =>
      prev.map((node) =>
        node.id === nodeId
          ? {
              ...node,
              parameters: node.parameters?.map((param) =>
                param.id === parameterId ? { ...param, value } : param
              ),
            }
          : node
      )
    );
  };

  // Auto-arrange nodes in a clean grid layout
  const handleAutoArrange = () => {
    const SPACING_X = 300;
    const SPACING_Y = 200;
    const START_X = 100;
    const START_Y = 100;

    // Group nodes by category
    const generators = nodes.filter(n => n.category === 'generator');
    const spatial = nodes.filter(n => n.category === 'spatial');
    const modifiers = nodes.filter(n => n.category === 'modifier');
    const color = nodes.filter(n => n.category === 'color');
    const combine = nodes.filter(n => n.category === 'combine');
    const output = nodes.filter(n => n.category === 'output');

    const groups = [generators, spatial, modifiers, color, combine, output].filter(g => g.length > 0);
    
    const arranged: NodeData[] = [];
    let columnIndex = 0;

    groups.forEach((group, groupIdx) => {
      group.forEach((node, idx) => {
        arranged.push({
          ...node,
          position: {
            x: START_X + columnIndex * SPACING_X,
            y: START_Y + idx * SPACING_Y,
          },
        });
      });
      columnIndex++;
    });

    setNodes(arranged);
    toast.success('Nodes arranged by category');
  };

  const selectedNode = nodes.find((n) => n.id === selectedNodeId) || null;

  // Keyboard Shortcuts
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Skip if typing in input/textarea
      if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement) {
        return;
      }

      const isMod = e.metaKey || e.ctrlKey;

      // Delete selected node (Delete/Backspace)
      if ((e.key === 'Delete' || e.key === 'Backspace') && selectedNodeId) {
        e.preventDefault();
        handleNodeDelete(selectedNodeId);
      }

      // Duplicate node (Cmd/Ctrl + D)
      if (isMod && e.key === 'd' && selectedNodeId) {
        e.preventDefault();
        const node = nodes.find((n) => n.id === selectedNodeId);
        if (node) {
          const newNode = {
            ...node,
            id: `node-${Date.now()}`,
            position: { x: node.position.x + 50, y: node.position.y + 50 },
          };
          setNodes([...nodes, newNode]);
          setSelectedNodeId(newNode.id);
          toast.success(`Duplicated ${node.title}`);
        }
      }

      // Copy node (Cmd/Ctrl + C)
      if (isMod && e.key === 'c' && selectedNodeId) {
        const node = nodes.find((n) => n.id === selectedNodeId);
        if (node) {
          setClipboard(node);
          toast.success('Node copied to clipboard');
        }
      }

      // Paste node (Cmd/Ctrl + V)
      if (isMod && e.key === 'v' && clipboard) {
        e.preventDefault();
        const newNode = {
          ...clipboard,
          id: `node-${Date.now()}`,
          position: { x: clipboard.position.x + 50, y: clipboard.position.y + 50 },
        };
        setNodes([...nodes, newNode]);
        setSelectedNodeId(newNode.id);
        toast.success(`Pasted ${clipboard.title}`);
      }

      // Select all nodes (Cmd/Ctrl + A)
      if (isMod && e.key === 'a') {
        e.preventDefault();
        toast.info('Multi-select coming soon');
      }

      // Toggle compact mode (C key)
      if (e.key === 'c' && !isMod && selectedNodeId) {
        e.preventDefault();
        setNodes((prev) =>
          prev.map((node) =>
            node.id === selectedNodeId ? { ...node, compact: !node.compact } : node
          )
        );
        toast.success('Toggled compact mode');
      }

      // Auto-arrange nodes (Shift + A)
      if (e.shiftKey && e.key === 'A') {
        e.preventDefault();
        handleAutoArrange();
      }

      // Toggle playback (Space when not on canvas)
      if (e.key === ' ' && e.target === document.body) {
        e.preventDefault();
        setPlaying(!playing);
      }

      // Deselect (Escape)
      if (e.key === 'Escape') {
        setSelectedNodeId(null);
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [selectedNodeId, nodes, clipboard, playing]);

  return (
    <div className="h-screen flex bg-[var(--k1-bg)] relative overflow-hidden">
      {/* VIVID Gradient Background - Makes glass visible */}
      <div className="absolute inset-0 pointer-events-none opacity-60">
        <div className="absolute top-0 right-0 w-[1000px] h-[1000px] bg-gradient-to-bl from-[var(--k1-accent)]/40 via-[var(--k1-accent-2)]/30 to-transparent blur-[100px] animate-pulse" style={{ animationDuration: '8s' }} />
        <div className="absolute bottom-0 left-0 w-[900px] h-[900px] bg-gradient-to-tr from-[var(--k1-accent-2)]/35 via-purple-500/25 to-transparent blur-[100px] animate-pulse" style={{ animationDuration: '12s', animationDelay: '2s' }} />
        <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-[600px] h-[600px] bg-gradient-to-br from-pink-500/20 via-orange-500/20 to-transparent blur-[120px] animate-pulse" style={{ animationDuration: '15s', animationDelay: '4s' }} />
      </div>

      {/* Full Canvas with Toolbar */}
      <div className="flex-1 flex flex-col relative z-0">
        {/* Toolbar */}
        <K1Toolbar
          playing={playing}
          onPlayPause={() => setPlaying(!playing)}
          onReset={() => toast.info('Reset pattern')}
          onSave={() => toast.success('Pattern saved')}
          onExport={() => toast.success('Exported to K1 device')}
          onImport={() => toast.info('Import pattern')}
          onFullscreen={() => document.documentElement.requestFullscreen()}
          onSettings={() => toast.info('Settings')}
          nodeCount={nodes.length}
          wireCount={wires.length}
        />

        {/* Canvas */}
        <div className="flex-1">
          <NodeCanvas
            nodes={nodes}
            wires={wires}
            selectedNodeId={selectedNodeId || undefined}
            onNodeSelect={setSelectedNodeId}
            onNodeMove={handleNodeMove}
            onNodeDelete={handleNodeDelete}
            onWireCreate={handleWireCreate}
            onWireDelete={handleWireDelete}
          />
        </div>
      </div>

      {/* Floating Left: Node Library */}
      <DraggablePanel
        defaultPosition={{ x: 16, y: 64 }}
        width={280}
        height="500px"
        title="Node Library"
      >
        <NodeLibrary onAddNode={handleAddNode} />
      </DraggablePanel>

      {/* Floating Right: Inspector */}
      <DraggablePanel
        defaultPosition={{ x: typeof window !== 'undefined' ? window.innerWidth - 360 : 1100, y: 64 }}
        width={340}
        height="400px"
        title="Inspector"
      >
        <NodeInspector
          node={selectedNode}
          onParameterChange={handleParameterChange}
          onDeleteNode={handleNodeDelete}
        />
      </DraggablePanel>
    </div>
  );
}
