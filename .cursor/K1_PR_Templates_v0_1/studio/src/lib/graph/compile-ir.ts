// studio/src/lib/graph/compile-ir.ts (PR TEMPLATE)
// Compile graph → IR v0.1 bytes (see spec/IR_v0_1.md)
import type { Graph } from "./types";
export interface IRBlob { bytes: Uint8Array }

export function compileToIR(graph: Graph): IRBlob {
  // TODO: topo-sort DAG, constant-fold, emit ops + constants + palettes
  return { bytes: new Uint8Array() };
}
