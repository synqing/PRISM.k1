import json
import tempfile
import unittest
from pathlib import Path
from typing import List

from tools import prism_packaging
from tools.parser_testbed import builder


class PrismPackagingTests(unittest.TestCase):
    def create_payload(self, tmp: Path, led_count=8, frames=3) -> Path:
        data = {
            "meta": {
                "fps": 30,
                "duration": frames / 30,
                "palette": ["#ff0000", "#00ff00"],
            },
            "data": {
                "frames": [
                    [[i * 10 % 256, j * 5 % 256, (i + j) % 256] for i in range(led_count)]
                    for j in range(frames)
                ]
            },
        }
        path = tmp / "payload.json"
        path.write_text(json.dumps(data), encoding="utf-8")
        return path

    def _slice_header(self, blob: bytes) -> bytes:
        base_size = builder.BASE_SIZE
        meta_size = builder.META_SIZE
        extra_len = int.from_bytes(blob[base_size + meta_size : base_size + meta_size + 2], "little")
        total = base_size + meta_size + 2 + extra_len
        return blob[:total]

    def test_package_roundtrip_and_stats(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            tmp = Path(tmpdir)
            json_path = self.create_payload(tmp, led_count=4, frames=2)
            out_path = tmp / "output.prism"
            report_path = tmp / "report.json"
            stats = prism_packaging.write_package(json_path, out_path, report_path)
            content = out_path.read_bytes()
            self.assertTrue(report_path.exists())
            report = json.loads(report_path.read_text(encoding="utf-8"))

        self.assertGreater(len(content), builder.BASE_SIZE)
        header_blob = self._slice_header(content)
        parsed = builder.parse_header_blob(header_blob)
        self.assertEqual(parsed.base.magic, b"PRSM")
        self.assertEqual(parsed.base.led_count, 4)
        self.assertEqual(parsed.base.frame_count, 2)
        self.assertEqual(parsed.extra_fields.get("ramp_space"), "hsluv")

        payload_crc = int(stats["payload_crc32"], 16)
        stored_crc = int.from_bytes(content[-4:], "little")
        self.assertEqual(payload_crc, stored_crc)

        self.assertEqual(stats["roundtrip_hash"], report["roundtrip_hash"])
        self.assertEqual(stats["palette_size"], len(stats["palette"]))
        self.assertGreaterEqual(stats["palette_size"], 2)
        self.assertAlmostEqual(stats["fps"], 30)

    def test_rle_and_delta_usage(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            tmp = Path(tmpdir)
            frames = [
                [[0, 0, 0]] * 16,
                [[0, 0, 0]] * 8 + [[255, 255, 255]] * 8,
                [[0, 0, 0]] * 8 + [[255, 255, 255]] * 8,
            ]
            payload = {
                "meta": {"fps": 24, "palette": ["#000000", "#ffffff"]},
                "data": {"frames": frames},
            }
            json_path = tmp / "frames.json"
            json_path.write_text(json.dumps(payload), encoding="utf-8")
            out_path = tmp / "encoded.prism"
            stats = prism_packaging.write_package(json_path, out_path, None)

        self.assertTrue(any(frame_stat["rle"] for frame_stat in stats["frames"]))
        self.assertTrue(any(frame_stat["delta"] for frame_stat in stats["frames"]))

    def test_validation_rejects_large_palette(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            tmp = Path(tmpdir)
            frames: List[List[List[int]]] = []
            frame: List[List[int]] = []
            for i in range(70):
                colour = [i % 256, (i * 2) % 256, (i * 3) % 256]
                frame.append(colour)
            frames.append(frame)
            payload = {"data": {"frames": frames}}
            path = tmp / "payload.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            out_path = tmp / "file.prism"
            with self.assertRaises(ValueError):
                prism_packaging.write_package(path, out_path, None)


if __name__ == "__main__":
    unittest.main()
