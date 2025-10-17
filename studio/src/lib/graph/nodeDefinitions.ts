import type { NodeKind } from '../../stores/graph';

export type PinDef = { required?: boolean; arity?: 1 | 'many' };
export type ParamDef = { type: 'number' | 'string' | 'color'; min?: number; max?: number; enum?: string[] };

export const NodeDefinitions: Record<NodeKind, { inputs: Record<string, PinDef>; params: Record<string, ParamDef> }> = {
  AngleField: { inputs: {}, params: {} },
  RadiusField: { inputs: {}, params: {} },
  SinOsc: { inputs: {}, params: { freq: { type: 'number', min: 0, max: 10 }, spatial: { type: 'number', min: 0, max: 10 }, phase: { type: 'number', min: 0, max: 1 } } },
  PhaseAccum: { inputs: {}, params: { speed: { type: 'number', min: -10, max: 10 }, offset: { type: 'number', min: 0, max: 1 } } },
  DistCenter: { inputs: {}, params: {} },
  Ring: { inputs: { src: { required: false } }, params: { radius: { type: 'number', min: 0, max: 1 }, width: { type: 'number', min: 0.001, max: 0.5 } } },
  Fade: { inputs: { src: { required: true } }, params: { amount: { type: 'number', min: 0, max: 1 } } },
  CenterOutMirror: { inputs: { src: { required: true } }, params: {} },
  Solid: { inputs: {}, params: { color: { type: 'color' } } },
  Gradient: { inputs: {}, params: { c0: { type: 'color' }, c1: { type: 'color' }, speed: { type: 'number', min: -5, max: 5 } } },
  Brightness: { inputs: { src: { required: true } }, params: { amount: { type: 'number', min: 0, max: 255 } } },
  HueShift: { inputs: { src: { required: true } }, params: { deg: { type: 'number', min: -180, max: 180 }, rate: { type: 'number', min: -360, max: 360 } } },
  Add: { inputs: { a: { required: true }, b: { required: true } }, params: {} },
  Multiply: { inputs: { a: { required: true }, b: { required: true } }, params: {} },
  PaletteMap: { inputs: { src: { required: true } }, params: {} },
  Noise2D: { inputs: {}, params: { seed: { type: 'number' }, scale: { type: 'number', min: 1, max: 64 }, amplitude: { type: 'number', min: 0, max: 255 } } },
  Rotate: { inputs: { src: { required: true } }, params: { deg: { type: 'number', min: -180, max: 180 } } },
  Scale: { inputs: { src: { required: true } }, params: { factor: { type: 'number', min: 0.1, max: 10 } } },
  Mirror: { inputs: { src: { required: true } }, params: {} },
  ToK1: { inputs: { src: { required: true } }, params: {} },
};
