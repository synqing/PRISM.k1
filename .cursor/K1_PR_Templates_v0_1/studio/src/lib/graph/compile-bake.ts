// studio/src/lib/graph/compile-bake.ts (PR TEMPLATE)
// Render N frames on host and return bytes for `.prism` container.
// TODO: integrate with existing packer; this scaffolds the interface only.
import type { Graph } from "./types";

export interface BakeOpts { seconds: number; fps: number }
export function bakeToPrism(graph: Graph, opt: BakeOpts): Uint8Array {
  // TODO: evaluate minimal graph set on CPU for opt.seconds*opt.fps frames
  // TODO: call packer to produce .prism bytes
  return new Uint8Array(); // placeholder
}
