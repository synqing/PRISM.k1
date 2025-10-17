// GeometryProfile schema/types for K1_LGP_v1 (app-side)

export interface GeometryPixel {
  u: number; // 0..1
  v: number; // 0..1
  edgeW0: number; // 0..1 energy from edge 0
  edgeW1: number; // 0..1 energy from edge 1
  gain: number; // per-pixel uniformity gain
}

export interface GeometryProfile {
  schema: 1;
  id: string; // e.g., K1_LGP_v1
  faceplate: string;
  revision: string;
  signature?: string; // optional sha256-hex
  gamma: number; // e.g., 2.2
  whiteBalance: [number, number, number];
  maxBrightness: number; // 0..1
  pixels: GeometryPixel[];
}

// Placeholder sample profile (not used at runtime yet)
export const K1_LGP_V1_SAMPLE: GeometryProfile = {
  schema: 1,
  id: 'K1_LGP_v1',
  faceplate: 'StdClear_2mm_v1',
  revision: 'r1.0',
  gamma: 2.2,
  whiteBalance: [1.0, 0.98, 1.04],
  maxBrightness: 0.85,
  pixels: Array.from({ length: 320 }, (_, i) => ({
    u: i / 319,
    v: 0.5,
    edgeW0: i < 160 ? 1 : 0,
    edgeW1: i >= 160 ? 1 : 0,
    gain: 1.0,
  })),
};

