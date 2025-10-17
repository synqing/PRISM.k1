#!/usr/bin/env python3
import struct
import argparse
import sys


def migrate_v10_to_v11(input_path, output_path):
    """Migrate .prism v1.0 (64 bytes) to v1.1 (70+ bytes)"""
    with open(input_path, 'rb') as f:
        v10_data = f.read()

    if len(v10_data) < 64:
        raise ValueError("File too small to be valid .prism v1.0")

    # Parse v1.0 header
    magic, version = struct.unpack_from('<4sH', v10_data, 0)

    if magic != b'PRSM':
        raise ValueError("Invalid magic number")

    # Create v1.1 header (copy v1.0 header base)
    v11_data = bytearray(v10_data[:64])

    # Update version to 0x0101 (v1.1)
    struct.pack_into('<H', v11_data, 4, 0x0101)

    # Append v1.1 temporal fields (defaults)
    # Layout example only; adjust to actual spec as needed.
    v11_data.extend([
        0x01,  # version minor copy / reserved
        0x04,  # motion: STATIC
        0x00,  # sync: SYNC
        0x00,  # reserved
        0x00, 0x00,  # delay_ms
        0x00, 0x00,  # progressive_start_ms
        0x00, 0x00,  # progressive_end_ms
        0x00, 0x00,  # wave_amplitude_ms
        0x00, 0x00,  # wave_frequency_hz
        0x00, 0x00   # wave_phase_deg
    ])

    # Append LED payload
    v11_data.extend(v10_data[64:])

    # TODO: Recalculate CRC if present in trailer

    with open(output_path, 'wb') as f:
        f.write(v11_data)

    print(f"Migrated {input_path} → {output_path}")


def main(argv=None):
    parser = argparse.ArgumentParser(description='Migrate .prism v1.0 files to v1.1 format')
    parser.add_argument('input', help='Input .prism v1.0 file')
    parser.add_argument('output', help='Output .prism v1.1 file')
    args = parser.parse_args(argv)

    migrate_v10_to_v11(args.input, args.output)


if __name__ == '__main__':
    main()

