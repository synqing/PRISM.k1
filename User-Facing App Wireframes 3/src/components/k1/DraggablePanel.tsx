import { useRef, useState, useEffect, useCallback, ReactNode } from 'react';
import { GripVertical } from 'lucide-react';

interface DraggablePanelProps {
  children: ReactNode;
  defaultPosition: { x: number; y: number };
  width: number;
  height: string;
  title?: string;
  className?: string;
}

export function DraggablePanel({
  children,
  defaultPosition,
  width,
  height,
  title,
  className = '',
}: DraggablePanelProps) {
  const panelRef = useRef<HTMLDivElement>(null);
  const [position, setPosition] = useState(defaultPosition);
  const [isDragging, setIsDragging] = useState(false);
  const [dragOffset, setDragOffset] = useState({ x: 0, y: 0 });
  const rafRef = useRef<number>();

  const handleMouseDown = useCallback((e: React.MouseEvent) => {
    if (!panelRef.current) return;
    
    const rect = panelRef.current.getBoundingClientRect();
    setDragOffset({
      x: e.clientX - rect.left,
      y: e.clientY - rect.top,
    });
    setIsDragging(true);
    
    // Prevent text selection while dragging
    e.preventDefault();
  }, []);

  const handleMouseMove = useCallback((e: MouseEvent) => {
    if (!isDragging) return;

    // Cancel any pending RAF
    if (rafRef.current) {
      cancelAnimationFrame(rafRef.current);
    }

    rafRef.current = requestAnimationFrame(() => {
      const newX = e.clientX - dragOffset.x;
      const newY = e.clientY - dragOffset.y;

      // Constrain to viewport
      const maxX = window.innerWidth - width;
      const maxY = window.innerHeight - 100; // Leave some space at bottom

      setPosition({
        x: Math.max(0, Math.min(newX, maxX)),
        y: Math.max(0, Math.min(newY, maxY)),
      });
    });
  }, [isDragging, dragOffset, width]);

  const handleMouseUp = useCallback(() => {
    setIsDragging(false);
    if (rafRef.current) {
      cancelAnimationFrame(rafRef.current);
    }
  }, []);

  // Attach global mouse listeners when dragging
  useEffect(() => {
    if (isDragging) {
      document.addEventListener('mousemove', handleMouseMove);
      document.addEventListener('mouseup', handleMouseUp);
    }

    return () => {
      document.removeEventListener('mousemove', handleMouseMove);
      document.removeEventListener('mouseup', handleMouseUp);
      if (rafRef.current) {
        cancelAnimationFrame(rafRef.current);
      }
    };
  }, [isDragging, handleMouseMove, handleMouseUp]);

  return (
    <div
      ref={panelRef}
      className={`absolute rounded-xl overflow-hidden ${className}`}
      style={{
        left: position.x,
        top: position.y,
        width: width,
        height: height,
        zIndex: isDragging ? 40 : 30,
        transition: isDragging ? 'none' : 'box-shadow 0.2s ease',
      }}
    >
      {/* Glass Background Layer with Heavy Opacity */}
      <div className="absolute inset-0 rounded-xl pointer-events-none" style={{
        background: 'rgba(26, 31, 43, 0.15)',
        backdropFilter: 'blur(60px) saturate(180%)',
        WebkitBackdropFilter: 'blur(60px) saturate(180%)',
        border: '1px solid rgba(255, 255, 255, 0.1)',
        boxShadow: '0 8px 32px rgba(0, 0, 0, 0.4), inset 0 1px 0 rgba(255, 255, 255, 0.1)'
      }} />
      
      {/* Top-left gradient overlay */}
      <div className="absolute inset-0 rounded-xl pointer-events-none" style={{
        background: 'linear-gradient(135deg, rgba(110, 231, 243, 0.08) 0%, transparent 50%)',
      }} />
      
      {/* Drag Handle Header */}
      <div
        className="absolute top-0 left-0 right-0 h-8 flex items-center justify-between px-3 cursor-grab active:cursor-grabbing z-50 hover:bg-white/5 transition-colors"
        onMouseDown={handleMouseDown}
        style={{ 
          backdropFilter: 'blur(8px)',
          borderBottom: '1px solid rgba(255,255,255,0.08)',
        }}
      >
        <div className="flex items-center gap-2 text-xs text-[var(--k1-text-dim)] select-none pointer-events-none">
          <GripVertical className="w-3.5 h-3.5 opacity-40" />
          {title && <span className="font-medium">{title}</span>}
        </div>
        
        {/* Visual indicator when dragging */}
        {isDragging && (
          <div className="text-[10px] text-[var(--k1-accent)] font-mono pointer-events-none">
            {Math.round(position.x)}, {Math.round(position.y)}
          </div>
        )}
      </div>

      {/* Panel Content with top padding for drag handle */}
      <div className="relative w-full h-full" style={{ paddingTop: '32px' }}>
        {children}
      </div>
    </div>
  );
}
