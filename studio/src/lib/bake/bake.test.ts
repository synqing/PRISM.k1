import { describe, it, expect } from 'vitest';
import { bakeProjectToPrism } from './bake';

describe('bakeProjectToPrism palette header policy', () => {
  it('omits palette header when embedPaletteHeader=false', async () => {
    const simple = { color: '#112233', ledCount: 10 } as any;
    const res = await bakeProjectToPrism(simple, 0.1, 10, 10, ['#ff0000','#00ff00'], false);
    const bytes = res.bytes;
    // header byte 49 = palette count
    expect(bytes[49]).toBe(0);
  });
});

