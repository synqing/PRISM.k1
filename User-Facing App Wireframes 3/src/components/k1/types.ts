// Core type definitions for the K1 Node System

export type PortType = 'scalar' | 'field' | 'color' | 'output';

export interface Port {
  id: string;
  label: string;
  type: PortType;
}

export interface NodeParameter {
  id: string;
  label: string;
  type: 'slider' | 'select' | 'number' | 'toggle';
  value: number | string | boolean;
  min?: number;
  max?: number;
  step?: number;
  options?: string[];
}

export type NodeCategory = 'generator' | 'spatial' | 'modifier' | 'color' | 'combine' | 'output';

export interface NodeData {
  id: string;
  title: string;
  category: NodeCategory;
  icon: string;
  inputs: Port[];
  outputs: Port[];
  parameters?: NodeParameter[];
  position: { x: number; y: number };
  compact?: boolean;
}

export interface Wire {
  id: string;
  from: { nodeId: string; portId: string };
  to: { nodeId: string; portId: string };
  type: PortType;
}

export const PORT_COLORS: Record<PortType, string> = {
  scalar: '#F59E0B',
  field: '#22D3EE',
  color: '#F472B6',
  output: '#34D399',
};

export const CATEGORY_COLORS: Record<NodeCategory, string> = {
  generator: 'bg-purple-500/10 text-purple-400',
  spatial: 'bg-cyan-500/10 text-cyan-400',
  modifier: 'bg-amber-500/10 text-amber-400',
  color: 'bg-pink-500/10 text-pink-400',
  combine: 'bg-blue-500/10 text-blue-400',
  output: 'bg-green-500/10 text-green-400',
};
