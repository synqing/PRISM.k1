#!/usr/bin/env python3
"""
Generate a minimal .prism binary matching the packer header v1 used in Studio.

Usage:
  python tools/tests/gen_fixture_prism.py --fps 120 --led 320 --frames 1 --name baked --color #1ec8ff > /tmp/out.prism
"""
import argparse, sys, zlib

def hex_to_rgb(hexs: str):
    s = hexs.lstrip('#')
    if len(s) != 6:
        return (255,255,255)
    return (int(s[0:2],16), int(s[2:4],16), int(s[4:6],16))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--fps', type=int, default=120)
    ap.add_argument('--led', type=int, default=320)
    ap.add_argument('--frames', type=int, default=1)
    ap.add_argument('--name', type=str, default='baked')
    ap.add_argument('--color', type=str, default='#1ec8ff')
    ap.add_argument('--palette', type=str, default='')  # comma-separated hex
    ap.add_argument('--compress', action='store_true')
    args = ap.parse_args()

    # Build header 64 bytes
    header = bytearray(64)
    # Magic 'PRSM' LE
    header[0:4] = (0x4D,0x53,0x52,0x50)
    header[4] = 1
    header[5] = 0
    header[6:8] = (64).to_bytes(2,'little')
    # effect/channel/flags
    header[8] = 0x01
    header[9] = 0x00
    header[10:12] = (0x0001).to_bytes(2,'little')
    # name
    name = (args.name or 'baked')[:20].encode('utf-8')
    header[12:12+len(name)] = name
    # brightness/speed/fade/param_count
    header[32] = 255
    spd = max(0,min(255, round((args.fps/120)*128)))
    header[33] = spd
    header[34] = 0
    header[35] = 0
    # palette blank; data_crc32=0
    # header_crc32 over 0..59
    hcrc = zlib.crc32(bytes(header[0:60])) & 0xFFFFFFFF
    header[60:64] = hcrc.to_bytes(4,'little')

    # Optional palette
    palette_hex = [p for p in (args.palette.split(',') if args.palette else []) if p]
    palette_count = min(16, len(palette_hex))
    if palette_count > 0:
        # set count and offset, and data_crc32
        header[49] = palette_count & 0xFF
        header[50:52] = (64).to_bytes(2,'little')
    # header_crc32 over 0..59
    hcrc = zlib.crc32(bytes(header[0:60])) & 0xFFFFFFFF
    header[60:64] = hcrc.to_bytes(4,'little')

    palette_block = bytearray()
    if palette_count > 0:
        for i in range(palette_count):
            r,g,b = hex_to_rgb(palette_hex[i])
            palette_block += bytes([r,g,b,0])
        dcrc = zlib.crc32(bytes(palette_block)) & 0xFFFFFFFF
        header[56:60] = dcrc.to_bytes(4,'little')

    # Payload: frames * (led*3) solid color
    r,g,b = hex_to_rgb(args.color)
    frame = bytes([r,g,b]) * args.led
    payload_raw = frame * args.frames
    if args.compress:
        # XOR delta vs zero + simple RLE (count,value)
        data = bytearray()
        for b in payload_raw:
            data.append(b ^ 0)
        # RLE
        enc = bytearray()
        i=0
        n=len(data)
        while i<n:
            val=data[i]
            cnt=1
            while i+cnt<n and data[i+cnt]==val and cnt<255:
                cnt+=1
            enc += bytes([cnt,val])
            i+=cnt
        payload = bytes(enc)
    else:
        payload = payload_raw
    out = header + palette_block + payload
    sys.stdout.buffer.write(out)

if __name__ == '__main__':
    main()
