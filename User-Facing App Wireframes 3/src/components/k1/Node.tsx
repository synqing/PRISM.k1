import { useRef } from 'react';
import { MoreVertical } from 'lucide-react';
import { PORT_COLORS, CATEGORY_COLORS, type NodeData, type Port } from './types';

interface NodeProps {
  data: NodeData;
  selected?: boolean;
  onSelect?: () => void;
  onDelete?: () => void;
  onPortMouseDown?: (portId: string, isOutput: boolean, event: React.MouseEvent) => void;
}

export function Node({ 
  data, 
  selected, 
  onSelect, 
  onDelete,
  onPortMouseDown,
}: NodeProps) {
  const nodeRef = useRef<HTMLDivElement>(null);
  
  const nodeWidth = data.compact ? 180 : 240;

  const handlePortMouseDown = (port: Port, isOutput: boolean, e: React.MouseEvent) => {
    e.stopPropagation();
    e.preventDefault();
    onPortMouseDown?.(port.id, isOutput, e);
  };

  return (
    <div
      ref={nodeRef}
      data-node-id={data.id}
      className={`
        absolute rounded-lg
        border transition-all duration-150
        glass-node glass-corners glass-sparkle tech-spotlight corner-notch frosted-texture
        ${selected ? 'border-[var(--k1-accent)] shadow-[0_0_0_4px_rgba(110,231,243,0.5)] glow-accent' : ''}
        ${data.compact ? 'min-w-[180px]' : 'min-w-[240px]'}
        cursor-move select-none hover-lift
      `}
      style={{
        left: data.position.x,
        top: data.position.y,
      }}
      onClick={onSelect}
    >
      {/* Header */}
      <div className="flex items-center justify-between px-3 py-2 border-b border-[rgba(255,255,255,0.08)] bg-gradient-to-b from-white/5 to-transparent relative z-10">
        <div className="flex items-center gap-2 flex-1 min-w-0">
          <div className={`w-6 h-6 rounded flex items-center justify-center flex-shrink-0 ${CATEGORY_COLORS[data.category]} shadow-k1-sm`}>
            <span className="text-sm">{data.icon}</span>
          </div>
          <span className="text-sm truncate">{data.title}</span>
        </div>
        <button
          className="h-6 w-6 p-0 hover:bg-white/10 rounded flex items-center justify-center transition-colors"
          onClick={(e) => {
            e.stopPropagation();
            onDelete?.();
          }}
        >
          <MoreVertical className="w-3 h-3" />
        </button>
      </div>

      {/* Body with Ports */}
      <div className="px-3 py-3 relative">
        {/* Input Ports */}
        {data.inputs.length > 0 && (
          <div className="space-y-2 mb-3">
            {data.inputs.map((port) => (
              <div key={port.id} className="flex items-center gap-2 group/port">
                <button
                  data-node-id={data.id}
                  data-port-id={port.id}
                  data-is-output="false"
                  className={`
                    w-4 h-4 rounded-full border-2 border-black/30
                    cursor-pointer hover:scale-150 transition-all duration-150
                    group-hover/port:ring-4 group-hover/port:ring-white/30
                    relative z-10 shadow-k1-sm
                    hover:shadow-[0_0_16px_rgba(255,255,255,0.4)]
                  `}
                  style={{
                    backgroundColor: PORT_COLORS[port.type],
                    marginLeft: -20,
                    boxShadow: `0 0 8px ${PORT_COLORS[port.type]}66`,
                  }}
                  onMouseDown={(e) => handlePortMouseDown(port, false, e)}
                  title={`Input: ${port.label} (${port.type})`}
                  type="button"
                />
                <span className="text-xs text-[var(--k1-text-dim)] group-hover/port:text-[var(--k1-text)]">
                  {port.label}
                </span>
              </div>
            ))}
          </div>
        )}

        {/* Parameters */}
        {!data.compact && data.parameters && data.parameters.length > 0 && (
          <div className="space-y-1.5 text-xs text-[var(--k1-text-dim)] mb-3">
            {data.parameters.slice(0, 2).map((param) => (
              <div key={param.id} className="flex items-center justify-between">
                <span>{param.label}</span>
                <span className="font-mono text-[10px]">{String(param.value)}</span>
              </div>
            ))}
            {data.parameters.length > 2 && (
              <div className="text-[10px] text-[var(--k1-text-dim)]/50">
                +{data.parameters.length - 2} more
              </div>
            )}
          </div>
        )}

        {/* Output Ports */}
        {data.outputs.length > 0 && (
          <div className="space-y-2">
            {data.outputs.map((port) => (
              <div key={port.id} className="flex items-center justify-end gap-2 group/port">
                <span className="text-xs text-[var(--k1-text-dim)] group-hover/port:text-[var(--k1-text)]">
                  {port.label}
                </span>
                <button
                  data-node-id={data.id}
                  data-port-id={port.id}
                  data-is-output="true"
                  className={`
                    w-4 h-4 rounded-full border-2 border-black/30
                    cursor-pointer hover:scale-150 transition-all duration-150
                    group-hover/port:ring-4 group-hover/port:ring-white/30
                    relative z-10 shadow-k1-sm
                    hover:shadow-[0_0_16px_rgba(255,255,255,0.4)]
                  `}
                  style={{
                    backgroundColor: PORT_COLORS[port.type],
                    marginRight: -20,
                    boxShadow: `0 0 8px ${PORT_COLORS[port.type]}66`,
                  }}
                  onMouseDown={(e) => handlePortMouseDown(port, true, e)}
                  title={`Output: ${port.label} (${port.type})`}
                  type="button"
                />
              </div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
}
