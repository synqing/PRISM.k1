import { describe, it, expect } from 'vitest';
import { encodeIR } from './exportIR';

describe('IR v0.1 encoder', () => {
  it('encodes header and sections', () => {
    const pal = new Uint8Array(256*3);
    for (let i=0;i<256;i++){ pal[i*3]=i; pal[i*3+1]=0; pal[i*3+2]=255-i; }
    const ops = [new Uint8Array([0x02, 0x00])]; // dummy op bytes
    const bytes = encodeIR({ constants: [0.5, 1.0], palettes: [pal], ops });
    expect(bytes[0]).toBe('K'.charCodeAt(0));
    expect(bytes[1]).toBe('1'.charCodeAt(0));
    expect(bytes[2]).toBe('I'.charCodeAt(0));
    expect(bytes[3]).toBe('R'.charCodeAt(0));
    // version little-endian at 4..5
    expect(bytes[4]).toBe(1);
    // npal at 10..11 should be 1
    expect(bytes[10]).toBe(1);
    // length should be header(12) + const(8) + palette(768) + ops(2)
    expect(bytes.length).toBe(12 + 2*4 + 256*3 + 2);
  });
});

