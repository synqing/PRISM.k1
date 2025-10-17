// Minimal IR v0.1 exporter scaffold (not yet used by device upload)
// Binary: 'K1IR' magic, version=1, nops, nconst, npal; then const float32s, palettes, ops

export type IRProgram = {
  constants: number[];
  palettes: Uint8Array[]; // each 256*3
  ops: Uint8Array[];      // raw op payloads per spec
};

export function encodeIR(prog: IRProgram): Uint8Array {
  const enc = new TextEncoder();
  const header = new Uint8Array(12);
  header.set(enc.encode('K1IR'), 0);
  const dv = new DataView(header.buffer);
  dv.setUint16(4, 1, true); // version
  dv.setUint16(6, prog.ops.length, true);
  dv.setUint16(8, prog.constants.length, true);
  dv.setUint16(10, prog.palettes.length, true);
  // constants
  const constBuf = new ArrayBuffer(prog.constants.length * 4);
  const constDV = new DataView(constBuf);
  prog.constants.forEach((c, i) => constDV.setFloat32(i*4, c, true));
  const constBytes = new Uint8Array(constBuf);
  // palettes
  const palBytes = prog.palettes.length ? prog.palettes.reduce((a,b)=>{
    const out = new Uint8Array(a.length + b.length); out.set(a,0); out.set(b,a.length); return out;
  }, new Uint8Array(0)) : new Uint8Array(0);
  // ops (already encoded)
  const opsBytes = prog.ops.length ? prog.ops.reduce((a,b)=>{ const out=new Uint8Array(a.length+b.length); out.set(a,0); out.set(b,a.length); return out; }, new Uint8Array(0)) : new Uint8Array(0);
  const out = new Uint8Array(header.length + constBytes.length + palBytes.length + opsBytes.length);
  let off = 0; out.set(header, off); off += header.length; out.set(constBytes, off); off += constBytes.length; out.set(palBytes, off); off += palBytes.length; out.set(opsBytes, off);
  return out;
}

// Placeholder compiler: returns empty program for now
// Minimal graph→IR scaffold: encode presence of Angle/Radius/SinOsc and a single PALETTEMAP
import type { Graph } from '../graph/types';

export function compileGraphToIR(_graph?: Graph, paletteLut?: Uint8Array): Uint8Array {
  const constants: number[] = [];
  const palettes: Uint8Array[] = [];
  const ops: Uint8Array[] = [];
  // Always write a RADIUS op (dst r0) as a placeholder
  ops.push(new Uint8Array([0x02, 0x00])); // RADIUS dst=0
  // If a palette is provided, add one palette bank and PALETTEMAP
  if (paletteLut && paletteLut.length === 256*3) {
    palettes.push(paletteLut);
    ops.push(new Uint8Array([0x0A, 0x00, 0x00])); // PALETTEMAP pal=0, a=0 (dst implied)
  }
  return encodeIR({ constants, palettes, ops });
}
