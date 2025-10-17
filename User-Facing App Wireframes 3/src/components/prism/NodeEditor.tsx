import { useState } from "react";
import { Button } from "../ui/button";
import { ScrollArea } from "../ui/scroll-area";
import { Input } from "../ui/input";
import { Slider } from "../ui/slider";
import { X, Plus, Zap } from "lucide-react";

const MOCK_NODE_LIBRARY = [
  { category: 'Generators', items: ['Noise', 'Gradient', 'Solid Color', 'Wave'] },
  { category: 'Modifiers', items: ['Blur', 'HSV Adjust', 'Brightness', 'Saturation'] },
  { category: 'Combiners', items: ['Mix', 'Add', 'Multiply', 'Screen'] },
  { category: 'Output', items: ['LED Output'] },
];

interface Node {
  id: string;
  type: string;
  x: number;
  y: number;
  inputs: number;
  outputs: number;
}

const MOCK_NODES: Node[] = [
  { id: 'n1', type: 'Noise', x: 100, y: 100, inputs: 0, outputs: 1 },
  { id: 'n2', type: 'HSV Adjust', x: 300, y: 100, inputs: 1, outputs: 1 },
  { id: 'n3', type: 'Gradient', x: 100, y: 250, inputs: 0, outputs: 1 },
  { id: 'n4', type: 'Mix', x: 500, y: 150, inputs: 2, outputs: 1 },
  { id: 'n5', type: 'LED Output', x: 700, y: 150, inputs: 1, outputs: 0 },
];

interface NodeEditorProps {
  onClose?: () => void;
}

export function NodeEditor({ onClose }: NodeEditorProps) {
  const [nodes] = useState(MOCK_NODES);

  return (
    <div className="fixed inset-0 z-50 bg-background">
      {/* Header */}
      <div className="h-14 border-b border-border flex items-center justify-between px-4">
        <div className="flex items-center gap-4">
          <h2 className="text-lg">Node Editor (Advanced)</h2>
          <Button variant="outline" size="sm" className="gap-2">
            <Plus className="w-4 h-4" />
            Add Node
          </Button>
        </div>
        <div className="flex items-center gap-2">
          <Button className="gap-2">
            <Zap className="w-4 h-4" />
            Bake to Clip
          </Button>
          <Button variant="ghost" size="sm" onClick={onClose}>
            <X className="w-4 h-4" />
          </Button>
        </div>
      </div>

      <div className="flex h-[calc(100vh-3.5rem)]">
        {/* Mini Library */}
        <div className="w-64 border-r border-border bg-card">
          <ScrollArea className="h-full p-4">
            <div className="space-y-4">
              {MOCK_NODE_LIBRARY.map((category) => (
                <div key={category.category} className="space-y-2">
                  <h4 className="text-xs text-muted-foreground uppercase tracking-wider">
                    {category.category}
                  </h4>
                  <div className="space-y-1">
                    {category.items.map((item) => (
                      <button
                        key={item}
                        className="w-full text-left px-3 py-2 text-sm rounded hover:bg-accent/10 transition-colors border border-transparent hover:border-border"
                      >
                        {item}
                      </button>
                    ))}
                  </div>
                </div>
              ))}
            </div>
          </ScrollArea>
        </div>

        {/* Graph Area */}
        <div className="flex-1 relative bg-muted/20 overflow-hidden">
          {/* Grid Background */}
          <svg className="absolute inset-0 w-full h-full pointer-events-none" style={{ opacity: 0.1 }}>
            <defs>
              <pattern id="grid" width="20" height="20" patternUnits="userSpaceOnUse">
                <path d="M 20 0 L 0 0 0 20" fill="none" stroke="currentColor" strokeWidth="0.5" />
              </pattern>
            </defs>
            <rect width="100%" height="100%" fill="url(#grid)" />
          </svg>

          {/* Nodes */}
          <div className="absolute inset-0">
            {nodes.map((node) => (
              <div
                key={node.id}
                className="absolute bg-card border-2 border-border rounded-lg shadow-lg"
                style={{ left: node.x, top: node.y, width: 160 }}
              >
                {/* Node Header */}
                <div className="px-3 py-2 border-b border-border bg-muted/50">
                  <div className="text-sm font-medium">{node.type}</div>
                </div>

                {/* Input Ports */}
                {node.inputs > 0 && (
                  <div className="py-2">
                    {Array.from({ length: node.inputs }).map((_, i) => (
                      <div key={i} className="flex items-center gap-2 px-3 py-1">
                        <div className="w-3 h-3 rounded-full border-2 border-primary bg-background" />
                        <span className="text-xs text-muted-foreground">In {i + 1}</span>
                      </div>
                    ))}
                  </div>
                )}

                {/* Output Ports */}
                {node.outputs > 0 && (
                  <div className="py-2">
                    {Array.from({ length: node.outputs }).map((_, i) => (
                      <div key={i} className="flex items-center justify-end gap-2 px-3 py-1">
                        <span className="text-xs text-muted-foreground">Out {i + 1}</span>
                        <div className="w-3 h-3 rounded-full border-2 border-accent bg-background" />
                      </div>
                    ))}
                  </div>
                )}
              </div>
            ))}

            {/* Connection Lines */}
            <svg className="absolute inset-0 w-full h-full pointer-events-none">
              <path
                d="M 260 115 Q 280 115, 300 115"
                stroke="currentColor"
                strokeWidth="2"
                fill="none"
                className="text-primary"
              />
              <path
                d="M 260 265 Q 380 265, 500 180"
                stroke="currentColor"
                strokeWidth="2"
                fill="none"
                className="text-primary"
              />
              <path
                d="M 660 165 Q 680 165, 700 165"
                stroke="currentColor"
                strokeWidth="2"
                fill="none"
                className="text-accent"
              />
            </svg>
          </div>
        </div>

        {/* Macro Knobs Panel */}
        <div className="w-80 border-l border-border bg-card">
          <div className="p-4 border-b border-border">
            <h3 className="font-medium">Macro Controls</h3>
          </div>
          <ScrollArea className="h-[calc(100%-3.5rem)] p-4">
            <div className="space-y-4">
              {['Speed', 'Intensity', 'Color Shift', 'Density'].map((param) => (
                <div key={param} className="space-y-2">
                  <div className="flex items-center justify-between">
                    <span className="text-sm">{param}</span>
                    <Input type="number" defaultValue="50" className="w-16 h-7 text-sm" />
                  </div>
                  <Slider defaultValue={[50]} max={100} step={1} />
                </div>
              ))}
            </div>
          </ScrollArea>
        </div>
      </div>
    </div>
  );
}
