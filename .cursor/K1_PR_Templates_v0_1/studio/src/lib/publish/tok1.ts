// studio/src/lib/publish/tok1.ts (PR TEMPLATE)
export type PublishMode =
  | { mode: "clip"; fps: number; seconds: number }
  | { mode: "program" };

export interface ToK1Params {
  target: string;
  geometry: string; // e.g., "K1_LGP_v1"
  color: { gamma: number; brightnessLimit: number; whiteBalance: [number,number,number] };
  publish: PublishMode;
}
