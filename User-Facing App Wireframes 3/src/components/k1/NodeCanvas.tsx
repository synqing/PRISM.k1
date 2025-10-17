import { useRef, useState, useEffect, useCallback } from 'react';
import { Node } from './Node';
import { toast } from 'sonner@2.0.3';
import { type NodeData, type Wire, PORT_COLORS } from './types';

interface NodeCanvasProps {
  nodes: NodeData[];
  wires: Wire[];
  selectedNodeId?: string;
  onNodeSelect?: (nodeId: string) => void;
  onNodeMove?: (nodeId: string, position: { x: number; y: number }) => void;
  onNodeDelete?: (nodeId: string) => void;
  onWireCreate?: (wire: Omit<Wire, 'id'>) => void;
  onWireDelete?: (wireId: string) => void;
}

export function NodeCanvas({
  nodes,
  wires,
  selectedNodeId,
  onNodeSelect,
  onNodeMove,
  onNodeDelete,
  onWireCreate,
  onWireDelete,
}: NodeCanvasProps) {
  const canvasRef = useRef<HTMLDivElement>(null);
  const [pan, setPan] = useState({ x: 0, y: 0 });
  const [zoom, setZoom] = useState(1);
  const [isPanning, setIsPanning] = useState(false);
  const [panStart, setPanStart] = useState({ x: 0, y: 0 });
  const [draggedNode, setDraggedNode] = useState<string | null>(null);
  const [dragOffset, setDragOffset] = useState({ x: 0, y: 0 });
  const [connectingPort, setConnectingPort] = useState<{
    nodeId: string;
    portId: string;
    isOutput: boolean;
    type: string;
  } | null>(null);
  const [mousePos, setMousePos] = useState({ x: 0, y: 0 });
  const [snapToGrid, setSnapToGrid] = useState(true);
  const rafRef = useRef<number>();
  
  const GRID_SIZE = 20;

  // Snap position to grid
  const snapPosition = useCallback((x: number, y: number) => {
    if (!snapToGrid) return { x, y };
    return {
      x: Math.round(x / GRID_SIZE) * GRID_SIZE,
      y: Math.round(y / GRID_SIZE) * GRID_SIZE,
    };
  }, [snapToGrid]);

  // Pan with space + drag + keyboard shortcuts
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Don't trigger shortcuts when typing
      if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement) {
        return;
      }

      if (e.code === 'Space' && !isPanning) {
        e.preventDefault();
        setIsPanning(true);
      }
      
      // Toggle snap to grid (G key)
      if (e.key === 'g' || e.key === 'G') {
        e.preventDefault();
        setSnapToGrid(prev => {
          const newSnap = !prev;
          toast.success(`Snap to grid ${newSnap ? 'ON' : 'OFF'}`);
          return newSnap;
        });
      }

      // Cancel wire connection (Escape)
      if (e.key === 'Escape' && connectingPort) {
        e.preventDefault();
        setConnectingPort(null);
        toast.info('Connection cancelled');
      }
    };

    const handleKeyUp = (e: KeyboardEvent) => {
      if (e.code === 'Space') {
        setIsPanning(false);
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);

    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
    };
  }, [isPanning, connectingPort]);

  const handleMouseDown = (e: React.MouseEvent) => {
    if (isPanning || e.button === 1) {
      e.preventDefault();
      setPanStart({ x: e.clientX - pan.x, y: e.clientY - pan.y });
    }
  };

  const handleMouseMove = useCallback((e: React.MouseEvent) => {
    const rect = canvasRef.current?.getBoundingClientRect();
    if (rect) {
      setMousePos({
        x: (e.clientX - rect.left - pan.x) / zoom,
        y: (e.clientY - rect.top - pan.y) / zoom,
      });
    }

    if (isPanning || e.buttons === 4) {
      // Use RAF for smooth panning
      if (rafRef.current) {
        cancelAnimationFrame(rafRef.current);
      }
      rafRef.current = requestAnimationFrame(() => {
        setPan({
          x: e.clientX - panStart.x,
          y: e.clientY - panStart.y,
        });
      });
    }

    if (draggedNode && !connectingPort) {
      const node = nodes.find((n) => n.id === draggedNode);
      if (node && rect) {
        let newX = (e.clientX - rect.left - pan.x) / zoom - dragOffset.x;
        let newY = (e.clientY - rect.top - pan.y) / zoom - dragOffset.y;
        
        // Snap to grid if enabled
        const snapped = snapPosition(newX, newY);
        
        // Use RAF for smooth dragging
        if (rafRef.current) {
          cancelAnimationFrame(rafRef.current);
        }
        rafRef.current = requestAnimationFrame(() => {
          onNodeMove?.(draggedNode, snapped);
        });
      }
    }
  }, [isPanning, panStart, draggedNode, connectingPort, nodes, pan, zoom, dragOffset, onNodeMove, snapPosition]);

  const handleMouseUp = useCallback(() => {
    setDraggedNode(null);
    // Clean up RAF
    if (rafRef.current) {
      cancelAnimationFrame(rafRef.current);
    }
  }, []);

  const handleWheel = (e: React.WheelEvent) => {
    if (e.metaKey || e.ctrlKey) {
      e.preventDefault();
      const delta = -e.deltaY * 0.001;
      setZoom((prev) => Math.max(0.25, Math.min(2, prev + delta)));
    }
  };

  const handleNodeMouseDown = (nodeId: string, e: React.MouseEvent) => {
    e.stopPropagation();
    if (!isPanning && !connectingPort) {
      const node = nodes.find((n) => n.id === nodeId);
      if (node) {
        const rect = canvasRef.current?.getBoundingClientRect();
        if (rect) {
          const clickX = (e.clientX - rect.left - pan.x) / zoom;
          const clickY = (e.clientY - rect.top - pan.y) / zoom;
          setDragOffset({
            x: clickX - node.position.x,
            y: clickY - node.position.y,
          });
        }
      }
      setDraggedNode(nodeId);
      onNodeSelect?.(nodeId);
    }
  };

  // Type compatibility matrix - what can connect to what
  const areTypesCompatible = (fromType: string, toType: string): boolean => {
    // Output ports accept anything
    if (toType === 'output') return true;
    
    // Exact match always works
    if (fromType === toType) return true;
    
    // Field can be used as scalar (sample the field)
    if (fromType === 'field' && toType === 'scalar') return true;
    
    // Scalar can expand to field
    if (fromType === 'scalar' && toType === 'field') return true;
    
    // Field can be used to modulate color
    if (fromType === 'field' && toType === 'color') return true;
    
    // Scalar can control color parameters
    if (fromType === 'scalar' && toType === 'color') return true;
    
    return false;
  };

  const handlePortMouseDown = (nodeId: string, portId: string, isOutput: boolean, e: React.MouseEvent) => {
    e.stopPropagation();
    e.preventDefault();

    const node = nodes.find((n) => n.id === nodeId);
    if (!node) return;

    const portList = isOutput ? node.outputs : node.inputs;
    const port = portList.find((p) => p.id === portId);
    if (!port) return;

    if (connectingPort) {
      // Complete connection - only allow output -> input
      if (connectingPort.isOutput !== isOutput) {
        const fromType = connectingPort.isOutput ? connectingPort.type : port.type;
        const toType = connectingPort.isOutput ? port.type : connectingPort.type;
        
        // Check type compatibility
        if (!areTypesCompatible(fromType, toType)) {
          toast.error(`Cannot connect ${fromType} to ${toType} (incompatible types)`);
          setConnectingPort(null);
          return;
        }

        const from = connectingPort.isOutput
          ? { nodeId: connectingPort.nodeId, portId: connectingPort.portId }
          : { nodeId, portId };
        const to = connectingPort.isOutput
          ? { nodeId, portId }
          : { nodeId: connectingPort.nodeId, portId: connectingPort.portId };

        const wireType = connectingPort.isOutput ? connectingPort.type : port.type;

        onWireCreate?.({
          from,
          to,
          type: wireType as any,
        });
      } else {
        toast.error(`Cannot connect ${isOutput ? 'output' : 'input'} to ${isOutput ? 'output' : 'input'}`);
      }
      setConnectingPort(null);
    } else {
      // Start connection
      setConnectingPort({
        nodeId,
        portId,
        isOutput,
        type: port.type,
      });
    }
  };

  // Get port center position using actual DOM measurement
  const getPortPosition = (nodeId: string, portId: string, isOutput: boolean): { x: number; y: number } => {
    const portElement = document.querySelector(
      `[data-node-id="${nodeId}"][data-port-id="${portId}"][data-is-output="${isOutput}"]`
    ) as HTMLElement;

    if (portElement) {
      const rect = portElement.getBoundingClientRect();
      const canvasRect = canvasRef.current?.getBoundingClientRect();
      
      if (canvasRect) {
        // Get center of port in canvas coordinates
        const centerX = (rect.left + rect.width / 2 - canvasRect.left - pan.x) / zoom;
        const centerY = (rect.top + rect.height / 2 - canvasRect.top - pan.y) / zoom;
        return { x: centerX, y: centerY };
      }
    }

    // Fallback to calculation if DOM element not found
    const node = nodes.find((n) => n.id === nodeId);
    if (!node) return { x: 0, y: 0 };

    const nodeWidth = node.compact ? 180 : 240;
    const portList = isOutput ? node.outputs : node.inputs;
    const portIndex = portList.findIndex((p) => p.id === portId);
    
    // Port centers are at the node edge after accounting for margin
    const x = isOutput ? node.position.x + nodeWidth : node.position.x;
    
    // Y calculation: header(40px) + padding(12px) + portHeight/2(8px) + portIndex * (portHeight(16px) + gap(8px))
    const y = node.position.x + 40 + 12 + 8 + (portIndex * 24);

    return { x, y };
  };

  // Generate improved wire path with better bezier curve
  const getWirePath = useCallback((from: { x: number; y: number }, to: { x: number; y: number }) => {
    const dx = to.x - from.x;
    const dy = to.y - from.y;
    const distance = Math.sqrt(dx * dx + dy * dy);
    
    // Adaptive control point offset based on distance and direction
    const baseOffset = Math.min(distance * 0.4, 100);
    const verticalInfluence = Math.abs(dy) / Math.max(distance, 1);
    const offset = baseOffset * (0.7 + verticalInfluence * 0.3);

    return `M ${from.x} ${from.y} C ${from.x + offset} ${from.y}, ${to.x - offset} ${to.y}, ${to.x} ${to.y}`;
  }, []);

  const handleCanvasClick = (e: React.MouseEvent) => {
    if (connectingPort && e.target === e.currentTarget) {
      setConnectingPort(null);
    }
  };

  // Cleanup RAF on unmount
  useEffect(() => {
    return () => {
      if (rafRef.current) {
        cancelAnimationFrame(rafRef.current);
      }
    };
  }, []);

  return (
    <div
      ref={canvasRef}
      className="relative w-full h-full overflow-hidden canvas-grid bg-[var(--k1-bg)]"
      onMouseDown={handleMouseDown}
      onMouseMove={handleMouseMove}
      onMouseUp={handleMouseUp}
      onWheel={handleWheel}
      onClick={handleCanvasClick}
      style={{ cursor: isPanning ? 'grab' : 'default' }}
    >
      {/* Transform container */}
      <div
        style={{
          transform: `translate(${pan.x}px, ${pan.y}px) scale(${zoom})`,
          transformOrigin: '0 0',
          position: 'absolute',
          inset: 0,
        }}
      >
        {/* SVG for wires */}
        <svg
          className="absolute inset-0 pointer-events-none"
          style={{
            width: '100%',
            height: '100%',
            overflow: 'visible',
          }}
        >
          <defs>
            <filter id="glow">
              <feGaussianBlur stdDeviation="2" result="coloredBlur" />
              <feMerge>
                <feMergeNode in="coloredBlur" />
                <feMergeNode in="SourceGraphic" />
              </feMerge>
            </filter>
            
            <filter id="glow-strong">
              <feGaussianBlur stdDeviation="3" result="coloredBlur" />
              <feMerge>
                <feMergeNode in="coloredBlur" />
                <feMergeNode in="SourceGraphic" />
              </feMerge>
            </filter>
            
            {/* Animated gradient for data flow */}
            <linearGradient id="flowGradient" x1="0%" y1="0%" x2="100%" y2="0%">
              <stop offset="0%" stopColor="white" stopOpacity="0">
                <animate attributeName="offset" values="-0.5;1" dur="1.5s" repeatCount="indefinite" />
              </stop>
              <stop offset="50%" stopColor="white" stopOpacity="0.6">
                <animate attributeName="offset" values="0;1.5" dur="1.5s" repeatCount="indefinite" />
              </stop>
              <stop offset="100%" stopColor="white" stopOpacity="0">
                <animate attributeName="offset" values="0.5;2" dur="1.5s" repeatCount="indefinite" />
              </stop>
            </linearGradient>
          </defs>

          {/* Existing wires */}
          {wires.map((wire) => {
            const from = getPortPosition(wire.from.nodeId, wire.from.portId, true);
            const to = getPortPosition(wire.to.nodeId, wire.to.portId, false);
            const path = getWirePath(from, to);
            const dx = to.x - from.x;
            const distance = Math.sqrt(dx * dx + (to.y - from.y) ** 2);
            const thickness = Math.min(2 + distance / 200, 3.5);

            return (
              <g key={wire.id} className="wire-group">
                {/* Invisible thick clickable area */}
                <path
                  d={path}
                  stroke="transparent"
                  strokeWidth="14"
                  fill="none"
                  onClick={(e) => {
                    e.stopPropagation();
                    onWireDelete?.(wire.id);
                  }}
                  className="cursor-pointer"
                  style={{ pointerEvents: 'stroke' }}
                />
                {/* Main wire with adaptive thickness */}
                <path
                  d={path}
                  stroke={PORT_COLORS[wire.type]}
                  strokeWidth={thickness}
                  fill="none"
                  filter="url(#glow)"
                  style={{ pointerEvents: 'none' }}
                  className="transition-all hover:opacity-80"
                />
                {/* Animated flow overlay */}
                <path
                  d={path}
                  stroke="url(#flowGradient)"
                  strokeWidth={thickness + 0.5}
                  fill="none"
                  style={{ pointerEvents: 'none', mixBlendMode: 'screen' }}
                  opacity="0.7"
                />
              </g>
            );
          })}

          {/* Active connection preview with animation */}
          {connectingPort && (
            <g>
              <path
                d={getWirePath(
                  getPortPosition(connectingPort.nodeId, connectingPort.portId, connectingPort.isOutput),
                  mousePos
                )}
                stroke="var(--k1-accent)"
                strokeWidth="3.5"
                strokeDasharray="8 4"
                fill="none"
                opacity="0.5"
                filter="url(#glow-strong)"
              >
                <animate attributeName="stroke-dashoffset" values="0;-12" dur="0.8s" repeatCount="indefinite" />
              </path>
              <path
                d={getWirePath(
                  getPortPosition(connectingPort.nodeId, connectingPort.portId, connectingPort.isOutput),
                  mousePos
                )}
                stroke="var(--k1-accent)"
                strokeWidth="1.5"
                fill="none"
                opacity="1"
              />
            </g>
          )}
        </svg>

        {/* Nodes */}
        {nodes.map((node) => (
          <div
            key={node.id}
            onMouseDown={(e) => handleNodeMouseDown(node.id, e)}
            style={{ position: 'absolute' }}
          >
            <Node
              data={node}
              selected={node.id === selectedNodeId}
              onSelect={() => onNodeSelect?.(node.id)}
              onDelete={() => onNodeDelete?.(node.id)}
              onPortMouseDown={(portId, isOutput, e) =>
                handlePortMouseDown(node.id, portId, isOutput, e)
              }
            />
          </div>
        ))}
      </div>

      {/* Canvas Info HUD */}
      <div className="absolute bottom-4 right-4 flex flex-col gap-2 items-end pointer-events-none">
        <div className="px-3 py-1.5 glass-panel glass-corners frosted-texture rounded-lg text-xs font-mono">
          {Math.round(zoom * 100)}%
        </div>
        <div className={`px-3 py-1.5 glass-panel frosted-texture rounded-lg text-xs font-mono transition-all ${
          snapToGrid ? 'text-[var(--k1-accent)] border-[var(--k1-accent)]/30' : 'text-[var(--k1-text-dim)]'
        }`}>
          {snapToGrid ? '◈ Snap ON' : '◇ Snap OFF'}
        </div>
      </div>

      {/* Instructions */}
      <div className="absolute bottom-4 left-4 px-3 py-2 glass-panel glass-corners glass-sparkle frosted-texture rounded-lg text-[10px] text-[var(--k1-text-dim)] space-y-0.5 pointer-events-none">
        <div className="flex items-center gap-2">
          <kbd className="px-1 py-0.5 bg-[var(--k1-bg)] rounded text-[9px]">Space</kbd>
          <span>+ drag to pan</span>
        </div>
        <div className="flex items-center gap-2">
          <kbd className="px-1 py-0.5 bg-[var(--k1-bg)] rounded text-[9px]">⌘</kbd>
          <span>+ scroll to zoom</span>
        </div>
        <div className="flex items-center gap-2">
          <kbd className="px-1 py-0.5 bg-[var(--k1-bg)] rounded text-[9px]">G</kbd>
          <span>Toggle snap</span>
        </div>
        <div className="pt-1 border-t border-[rgba(255,255,255,0.08)] mt-1">
          <div className="text-[var(--k1-accent)]">Click output → input to wire</div>
          <div className="mt-1">Click wire to delete • Esc to cancel</div>
        </div>
      </div>

      {/* Connection Status */}
      {connectingPort && (
        <div className="absolute top-4 left-1/2 -translate-x-1/2 px-4 py-2 bg-[var(--k1-accent)] text-black rounded-lg shadow-k1-lg font-semibold pointer-events-none" style={{ boxShadow: '0 0 30px rgba(110, 231, 243, 0.8), 0 12px 40px rgba(0, 0, 0, 0.6)' }}>
          ⚡ Click {connectingPort.isOutput ? 'INPUT' : 'OUTPUT'} port to complete
        </div>
      )}
    </div>
  );
}
