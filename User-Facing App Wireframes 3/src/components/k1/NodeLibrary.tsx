import { Search, Zap, Grid3x3, Wand2, Palette, GitMerge, MonitorPlay } from 'lucide-react';
import { Input } from '../ui/input';
import { ScrollArea } from '../ui/scroll-area';
import { useState } from 'react';
import type { NodeCategory } from './types';

interface NodeTemplate {
  id: string;
  title: string;
  category: NodeCategory;
  icon: string;
  description: string;
}

const NODE_TEMPLATES: NodeTemplate[] = [
  // Generators
  { id: 'gradient', title: 'Gradient', category: 'generator', icon: '🌈', description: 'Linear color gradient' },
  { id: 'noise', title: 'Noise', category: 'generator', icon: '⚡', description: 'Perlin noise field' },
  { id: 'wave', title: 'Wave', category: 'generator', icon: '〰️', description: 'Sine wave pattern' },
  { id: 'solid', title: 'Solid Color', category: 'generator', icon: '⬛', description: 'Uniform color' },
  
  // Spatial
  { id: 'position', title: 'Position', category: 'spatial', icon: '📍', description: 'LED position data' },
  { id: 'mirror', title: 'Mirror', category: 'spatial', icon: '🪞', description: 'Mirror across axis' },
  { id: 'rotate', title: 'Rotate', category: 'spatial', icon: '🔄', description: 'Rotate pattern' },
  { id: 'scale', title: 'Scale', category: 'spatial', icon: '📏', description: 'Scale pattern' },
  
  // Modifiers
  { id: 'multiply', title: 'Multiply', category: 'modifier', icon: '✖️', description: 'Multiply values' },
  { id: 'add', title: 'Add', category: 'modifier', icon: '➕', description: 'Add values' },
  { id: 'clamp', title: 'Clamp', category: 'modifier', icon: '📊', description: 'Limit range' },
  { id: 'invert', title: 'Invert', category: 'modifier', icon: '🔃', description: 'Invert values' },
  
  // Color
  { id: 'hue-shift', title: 'Hue Shift', category: 'color', icon: '🎨', description: 'Shift hue' },
  { id: 'saturation', title: 'Saturation', category: 'color', icon: '💧', description: 'Adjust saturation' },
  { id: 'brightness', title: 'Brightness', category: 'color', icon: '☀️', description: 'Adjust brightness' },
  { id: 'colorize', title: 'Colorize', category: 'color', icon: '🖌️', description: 'Apply color tint' },
  
  // Combine
  { id: 'blend', title: 'Blend', category: 'combine', icon: '🔀', description: 'Blend two inputs' },
  { id: 'mask', title: 'Mask', category: 'combine', icon: '🎭', description: 'Mask with field' },
  
  // Output
  { id: 'k1-output', title: 'K1 Output', category: 'output', icon: '📤', description: 'Send to device' },
];

const CATEGORY_INFO: Record<NodeCategory, { icon: React.ReactNode; label: string; color: string }> = {
  generator: { icon: <Zap className="w-4 h-4" />, label: 'Generators', color: 'text-purple-400' },
  spatial: { icon: <Grid3x3 className="w-4 h-4" />, label: 'Spatial', color: 'text-cyan-400' },
  modifier: { icon: <Wand2 className="w-4 h-4" />, label: 'Modifiers', color: 'text-amber-400' },
  color: { icon: <Palette className="w-4 h-4" />, label: 'Color', color: 'text-pink-400' },
  combine: { icon: <GitMerge className="w-4 h-4" />, label: 'Combine', color: 'text-blue-400' },
  output: { icon: <MonitorPlay className="w-4 h-4" />, label: 'Output', color: 'text-green-400' },
};

interface NodeLibraryProps {
  onAddNode?: (templateId: string) => void;
}

export function NodeLibrary({ onAddNode }: NodeLibraryProps) {
  const [search, setSearch] = useState('');
  const [selectedCategory, setSelectedCategory] = useState<NodeCategory | 'all'>('all');

  const filteredNodes = NODE_TEMPLATES.filter((node) => {
    const matchesSearch = node.title.toLowerCase().includes(search.toLowerCase()) ||
      node.description.toLowerCase().includes(search.toLowerCase());
    const matchesCategory = selectedCategory === 'all' || node.category === selectedCategory;
    return matchesSearch && matchesCategory;
  });

  const groupedNodes = filteredNodes.reduce((acc, node) => {
    if (!acc[node.category]) {
      acc[node.category] = [];
    }
    acc[node.category].push(node);
    return acc;
  }, {} as Record<NodeCategory, NodeTemplate[]>);

  return (
    <div className="w-full h-full flex flex-col bg-transparent">
      {/* Header */}
      <div className="p-4 border-b border-[rgba(255,255,255,0.12)] flex-shrink-0">
        <h2 className="mb-3">Node Library</h2>
        <div className="relative">
          <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-[var(--k1-text-dim)]" />
          <Input
            placeholder="Search nodes..."
            value={search}
            onChange={(e) => setSearch(e.target.value)}
            className="pl-9 bg-[var(--k1-bg)] border-[var(--k1-border)]"
          />
        </div>
      </div>

      {/* Category Filter */}
      <div className="p-2 border-b border-[rgba(255,255,255,0.08)] flex gap-1 flex-wrap bg-black/10">
        <button
          onClick={() => setSelectedCategory('all')}
          className={`px-2 py-1 rounded text-xs transition-colors ${
            selectedCategory === 'all'
              ? 'bg-[var(--k1-accent)] text-black'
              : 'bg-[var(--k1-bg)] text-[var(--k1-text-dim)] hover:text-[var(--k1-text)]'
          }`}
        >
          All
        </button>
        {Object.entries(CATEGORY_INFO).map(([category, info]) => (
          <button
            key={category}
            onClick={() => setSelectedCategory(category as NodeCategory)}
            className={`px-2 py-1 rounded text-xs transition-colors flex items-center gap-1 ${
              selectedCategory === category
                ? `bg-[var(--k1-accent)] text-black`
                : `bg-[var(--k1-bg)] hover:text-[var(--k1-text)] ${info.color}`
            }`}
          >
            {info.icon}
            {info.label}
          </button>
        ))}
      </div>

      {/* Node List */}
      <ScrollArea className="flex-1">
        <div className="p-3 space-y-4">
          {Object.entries(groupedNodes).map(([category, nodes]) => (
            <div key={category}>
              <div className="flex items-center gap-2 mb-2 px-1">
                <span className={`${CATEGORY_INFO[category as NodeCategory].color}`}>
                  {CATEGORY_INFO[category as NodeCategory].icon}
                </span>
                <h3 className="text-xs uppercase tracking-wide text-[var(--k1-text-dim)]">
                  {CATEGORY_INFO[category as NodeCategory].label}
                </h3>
              </div>
              <div className="space-y-1">
                {nodes.map((node) => (
                  <button
                    key={node.id}
                    onClick={() => onAddNode?.(node.id)}
                    className="w-full p-2 rounded border border-[rgba(255,255,255,0.08)] hover:border-[var(--k1-accent)]/50 transition-all text-left group relative overflow-hidden bg-gradient-to-br from-white/5 to-transparent hover:from-white/10 backdrop-blur-sm hover-lift"
                  >
                    <div className="flex items-center gap-2 mb-1">
                      <span className="text-lg">{node.icon}</span>
                      <span className="text-sm group-hover:text-[var(--k1-accent)] transition-colors">
                        {node.title}
                      </span>
                    </div>
                    <p className="text-[10px] text-[var(--k1-text-dim)] ml-7">{node.description}</p>
                  </button>
                ))}
              </div>
            </div>
          ))}
          
          {filteredNodes.length === 0 && (
            <div className="text-center py-8 text-[var(--k1-text-dim)]">
              <p className="text-sm">No nodes found</p>
              <p className="text-xs mt-1">Try a different search or category</p>
            </div>
          )}
        </div>
      </ScrollArea>
    </div>
  );
}
