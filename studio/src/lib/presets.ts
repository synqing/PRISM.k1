export type BakePreset = {
  name: string;
  seconds: number;
  fps: number;
  color: string;
  palette: string[];
  useGradient?: boolean;
  grad?: { c0: string; c1: string; speed: number };
  useHueShift?: boolean;
  hue?: { deg: number; rate: number };
};

export const PRESETS: BakePreset[] = [
  {
    name: 'Warm Drift',
    seconds: 2,
    fps: 120,
    color: '#ff7a59',
    palette: ['#ff6b35','#f7931e','#fdc830','#f37335'],
    useGradient: true,
    grad: { c0: '#661400', c1: '#ffae00', speed: 0.2 },
    useHueShift: true,
    hue: { deg: 0, rate: 15 },
  },
  {
    name: 'Cool Wave',
    seconds: 2,
    fps: 120,
    color: '#1ec8ff',
    palette: ['#00d4ff','#1ec8ff','#a1f0ff','#e0fbff'],
    useGradient: true,
    grad: { c0: '#001133', c1: '#1ec8ff', speed: 0.35 },
    useHueShift: false,
  },
  {
    name: 'Solid Teal',
    seconds: 1,
    fps: 120,
    color: '#00e5c3',
    palette: [],
  }
];

