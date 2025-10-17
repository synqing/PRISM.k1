#!/usr/bin/env python3
import argparse, sys, base64, time

try:
    import serial  # pyserial
except Exception as e:
    print("pyserial required: pip install pyserial", file=sys.stderr)
    sys.exit(2)

def send_line(ser, line):
    ser.write((line+"\n").encode('ascii'))
    ser.flush()
    resp = ser.readline().decode('ascii', errors='ignore').strip()
    return resp

def do_upload(ser, name, data, chunk=1024):
    import zlib
    crc = zlib.crc32(data) & 0xFFFFFFFF
    print(f"BEGIN {name} {len(data)} 0x{crc:08X}")
    r = send_line(ser, f"BEGIN {name} {len(data)} {crc:08X}")
    if not r.startswith("OK"): raise SystemExit(f"BEGIN failed: {r}")
    off = 0
    while off < len(data):
        blk = data[off:off+chunk]
        b64 = base64.b64encode(blk).decode('ascii')
        r = send_line(ser, f"DATA {off} {b64}")
        if not r.startswith("OK"): raise SystemExit(f"DATA failed @ {off}: {r}")
        off += len(blk)
    r = send_line(ser, "END")
    if not r.startswith("OK"): raise SystemExit(f"END failed: {r}")
    print("Upload complete")

def main():
    ap = argparse.ArgumentParser(description="PRISM UART test CLI")
    ap.add_argument('--port', required=True)
    ap.add_argument('--baud', type=int, default=115200)
    sub = ap.add_subparsers(dest='cmd', required=True)
    sub.add_parser('status')
    p_play = sub.add_parser('play'); p_play.add_argument('name')
    sub.add_parser('stop')
    p_b = sub.add_parser('b'); p_b.add_argument('target', type=int); p_b.add_argument('ms', type=int)
    p_g = sub.add_parser('g'); p_g.add_argument('gx', type=int); p_g.add_argument('ms', type=int)
    p_up = sub.add_parser('upload'); p_up.add_argument('file'); p_up.add_argument('--name')
    args = ap.parse_args()

    with serial.Serial(args.port, args.baud, timeout=2) as ser:
        time.sleep(0.1)
        if args.cmd == 'status':
            print(send_line(ser, 'STATUS'))
        elif args.cmd == 'play':
            print(send_line(ser, f'PLAY {args.name}'))
        elif args.cmd == 'stop':
            print(send_line(ser, 'STOP'))
        elif args.cmd == 'b':
            print(send_line(ser, f'B {args.target} {args.ms}'))
        elif args.cmd == 'g':
            print(send_line(ser, f'G {args.gx} {args.ms}'))
        elif args.cmd == 'upload':
            data = open(args.file, 'rb').read()
            name = args.name or 'baked'
            do_upload(ser, name, data)

if __name__ == '__main__':
    main()

