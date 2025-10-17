export type NodeKind =
  | 'Solid'
  | 'Gradient'
  | 'Brightness'
  | 'HueShift'
  | 'Add'
  | 'Multiply'
  | 'PaletteMap'
  | 'AngleField'
  | 'RadiusField'
  | 'SinOsc'
  | 'PhaseAccum'
  | 'DistCenter'
  | 'Ring'
  | 'Fade'
  | 'CenterOutMirror'
  | 'Impulse'
  | 'Noise2D'
  | 'ToK1';

export type ParamValue = number | string;

export interface GraphNode {
  id: string;
  kind: NodeKind;
  params: Record<string, ParamValue>;
  inputs: Record<string, string | null>; // pin -> upstream node id
}

export interface Graph {
  nodes: Record<string, GraphNode>;
  order: string[]; // stable order (optional; topo will be derived if empty)
  ledCount?: number;
}

export type Sampler = (i: number, t: number) => [number, number, number]; // ledIndex->RGB

export interface CompileContext {
  lut?: Uint8Array; // 256*3 palette lookup (optional)
}
