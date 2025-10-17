import { Settings, Trash2 } from 'lucide-react';
import { Button } from '../ui/button';
import { Slider } from '../ui/slider';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '../ui/select';
import { Switch } from '../ui/switch';
import { Label } from '../ui/label';
import { ScrollArea } from '../ui/scroll-area';
import { Badge } from '../ui/badge';
import type { NodeData, NodeParameter } from './types';
import { CATEGORY_COLORS } from './types';

interface NodeInspectorProps {
  node: NodeData | null;
  onParameterChange?: (nodeId: string, parameterId: string, value: number | string | boolean) => void;
  onDeleteNode?: (nodeId: string) => void;
}

export function NodeInspector({ node, onParameterChange, onDeleteNode }: NodeInspectorProps) {
  if (!node) {
    return (
      <div className="w-full h-full flex flex-col bg-transparent">
        <div className="p-4 border-b border-[rgba(255,255,255,0.12)] flex-shrink-0">
          <h2>Inspector</h2>
        </div>
        <div className="flex-1 flex items-center justify-center p-8">
          <div className="text-center text-[var(--k1-text-dim)]">
            <Settings className="w-12 h-12 mx-auto mb-3 opacity-30" />
            <p className="text-sm">No node selected</p>
            <p className="text-xs mt-1">Select a node to edit properties</p>
          </div>
        </div>
      </div>
    );
  }

  const handleParameterChange = (param: NodeParameter, value: number | string | boolean) => {
    onParameterChange?.(node.id, param.id, value);
  };

  return (
    <div className="w-full h-full flex flex-col bg-transparent">
      {/* Header */}
      <div className="p-4 border-b border-[rgba(255,255,255,0.12)] flex-shrink-0">
        <div className="flex items-center justify-between mb-3">
          <h2>Inspector</h2>
          <Button
            variant="ghost"
            size="sm"
            onClick={() => onDeleteNode?.(node.id)}
            className="h-8 px-2 text-[var(--k1-error)] hover:text-[var(--k1-error)] hover:bg-[var(--k1-error)]/10"
          >
            <Trash2 className="w-4 h-4 mr-1" />
            Delete
          </Button>
        </div>
        
        {/* Node Info */}
        <div className="space-y-2">
          <div className="flex items-center gap-2">
            <div className={`w-8 h-8 rounded flex items-center justify-center ${CATEGORY_COLORS[node.category]}`}>
              <span className="text-lg">{node.icon}</span>
            </div>
            <div className="flex-1 min-w-0">
              <h3 className="truncate">{node.title}</h3>
              <p className="text-xs text-[var(--k1-text-dim)] capitalize">{node.category}</p>
            </div>
          </div>
          
          {/* Port Summary */}
          <div className="flex gap-4 text-xs">
            <div>
              <span className="text-[var(--k1-text-dim)]">Inputs: </span>
              <span className="font-mono">{node.inputs.length}</span>
            </div>
            <div>
              <span className="text-[var(--k1-text-dim)]">Outputs: </span>
              <span className="font-mono">{node.outputs.length}</span>
            </div>
          </div>
        </div>
      </div>

      {/* Parameters */}
      <ScrollArea className="flex-1">
        <div className="p-4 space-y-4">
          {node.parameters && node.parameters.length > 0 ? (
            <>
              <div className="flex items-center gap-2 mb-2">
                <Settings className="w-4 h-4 text-[var(--k1-text-dim)]" />
                <h3 className="text-xs uppercase tracking-wide text-[var(--k1-text-dim)]">
                  Parameters
                </h3>
              </div>

              {node.parameters.map((param) => (
                <div key={param.id} className="space-y-2">
                  <div className="flex items-center justify-between">
                    <Label className="text-sm">{param.label}</Label>
                    {param.type === 'slider' || param.type === 'number' ? (
                      <Badge variant="outline" className="font-mono text-xs">
                        {param.value}
                      </Badge>
                    ) : null}
                  </div>

                  {param.type === 'slider' && (
                    <Slider
                      value={[param.value as number]}
                      onValueChange={([value]) => handleParameterChange(param, value)}
                      min={param.min ?? 0}
                      max={param.max ?? 100}
                      step={param.step ?? 1}
                      className="py-2"
                    />
                  )}

                  {param.type === 'number' && (
                    <input
                      type="number"
                      value={param.value as number}
                      onChange={(e) => handleParameterChange(param, parseFloat(e.target.value))}
                      min={param.min}
                      max={param.max}
                      step={param.step ?? 1}
                      className="w-full px-3 py-2 bg-[var(--k1-bg)] border border-[var(--k1-border)] rounded text-sm font-mono"
                    />
                  )}

                  {param.type === 'select' && param.options && (
                    <Select
                      value={param.value as string}
                      onValueChange={(value) => handleParameterChange(param, value)}
                    >
                      <SelectTrigger className="bg-[var(--k1-bg)] border-[var(--k1-border)]">
                        <SelectValue />
                      </SelectTrigger>
                      <SelectContent>
                        {param.options.map((option) => (
                          <SelectItem key={option} value={option}>
                            {option}
                          </SelectItem>
                        ))}
                      </SelectContent>
                    </Select>
                  )}

                  {param.type === 'toggle' && (
                    <div className="flex items-center justify-between py-1">
                      <Switch
                        checked={param.value as boolean}
                        onCheckedChange={(checked) => handleParameterChange(param, checked)}
                      />
                      <span className="text-xs text-[var(--k1-text-dim)]">
                        {param.value ? 'Enabled' : 'Disabled'}
                      </span>
                    </div>
                  )}

                  {/* Range display for sliders */}
                  {param.type === 'slider' && (param.min !== undefined || param.max !== undefined) && (
                    <div className="flex justify-between text-[10px] text-[var(--k1-text-dim)] font-mono">
                      <span>{param.min ?? 0}</span>
                      <span>{param.max ?? 100}</span>
                    </div>
                  )}
                </div>
              ))}
            </>
          ) : (
            <div className="text-center py-8 text-[var(--k1-text-dim)]">
              <p className="text-sm">No parameters</p>
              <p className="text-xs mt-1">This node has no adjustable properties</p>
            </div>
          )}
        </div>
      </ScrollArea>

      {/* Ports Info */}
      {(node.inputs.length > 0 || node.outputs.length > 0) && (
        <div className="p-4 border-t border-[rgba(255,255,255,0.08)] space-y-3 bg-black/10">
          {node.inputs.length > 0 && (
            <div>
              <h4 className="text-xs uppercase tracking-wide text-[var(--k1-text-dim)] mb-2">
                Input Ports
              </h4>
              <div className="space-y-1">
                {node.inputs.map((port) => (
                  <div key={port.id} className="flex items-center gap-2 text-xs">
                    <div
                      className="w-2 h-2 rounded-full"
                      style={{ backgroundColor: `var(--port-${port.type})` }}
                    />
                    <span className="flex-1">{port.label}</span>
                    <span className="text-[var(--k1-text-dim)] font-mono text-[10px]">
                      {port.type}
                    </span>
                  </div>
                ))}
              </div>
            </div>
          )}

          {node.outputs.length > 0 && (
            <div>
              <h4 className="text-xs uppercase tracking-wide text-[var(--k1-text-dim)] mb-2">
                Output Ports
              </h4>
              <div className="space-y-1">
                {node.outputs.map((port) => (
                  <div key={port.id} className="flex items-center gap-2 text-xs">
                    <div
                      className="w-2 h-2 rounded-full"
                      style={{ backgroundColor: `var(--port-${port.type})` }}
                    />
                    <span className="flex-1">{port.label}</span>
                    <span className="text-[var(--k1-text-dim)] font-mono text-[10px]">
                      {port.type}
                    </span>
                  </div>
                ))}
              </div>
            </div>
          )}
        </div>
      )}
    </div>
  );
}
