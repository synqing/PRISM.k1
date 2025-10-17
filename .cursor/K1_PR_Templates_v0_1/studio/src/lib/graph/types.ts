// studio/src/lib/graph/types.ts (PR TEMPLATE)
export type NodeID = string;
export type Socket = "scalar" | "color" | "image";

export interface ParamSpec {
  name: string;
  kind: "scalar" | "angle" | "int" | "vec2";
  default: number | number[];
  min?: number;
  max?: number;
}

export interface NodeDef {
  type: string;
  inputs: { [name: string]: Socket };
  outputs: { [name: string]: Socket };
  params: ParamSpec[];
}

export interface NodeInstance {
  id: NodeID;
  type: string;
  params: { [k: string]: number | number[] };
}

export interface Link {
  from: { node: NodeID; socket: string };
  to:   { node: NodeID; socket: string };
}

export interface Graph {
  version: 1;
  nodes: NodeInstance[];
  links: Link[];
  macros: { Energy: number; Decay: number; HueShift: number; Spread: number };
  palettes?: { [name: string]: string }; // base64 256*RGB8 LUTs
}
