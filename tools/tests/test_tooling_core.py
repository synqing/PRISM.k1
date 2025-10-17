import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path

from tools import tooling_core


class ToolingCoreTests(unittest.TestCase):
    def test_write_json_produces_deterministic_payload(self) -> None:
        payload = {"rgb": [[1, 2, 3], [4, 5, 6]]}
        meta_seed = {"source": "test"}
        extra_meta = {
            "label": "unit-test",
            "tool": "test_suite",
            "generated_at": "2025-01-01T00:00:00Z",
            "software_version": "test-suite/1.0.0",
            "seed": "0x1",
            "seed_numeric": 1,
        }

        with tempfile.TemporaryDirectory() as tmp:
            out_path = Path(tmp) / "sample.json"
            tooling_core.write_json(out_path, payload, meta_seed, extra_meta=extra_meta)
            content = out_path.read_text(encoding="utf-8")

        self.assertTrue(content.endswith("\n"))
        data = json.loads(content)
        self.assertEqual(data["version"], tooling_core.DEFAULT_VERSION)
        self.assertEqual(data["data"], payload)
        self.assertEqual(data["meta"]["source"], "test")
        self.assertEqual(data["meta"]["label"], "unit-test")
        self.assertEqual(data["meta"]["tool"], "test_suite")
        self.assertEqual(data["meta"]["generated_at"], "2025-01-01T00:00:00Z")
        self.assertEqual(data["meta"]["software_version"], "test-suite/1.0.0")
        self.assertEqual(data["meta"]["seed"], "0x1")
        self.assertEqual(data["meta"]["seed_numeric"], 1)

    def test_write_csv_includes_metadata_and_rows(self) -> None:
        rows = [(1, 2, 3), (4, 5, 6)]
        headers = ["r", "g", "b"]
        meta_seed = {"intent": "csv-test"}
        extra_meta = {
            "tool": "test_suite",
            "generated_at": "2025-01-01T00:00:00Z",
            "software_version": "test-suite/1.0.0",
        }

        with tempfile.TemporaryDirectory() as tmp:
            out_path = Path(tmp) / "sample.csv"
            tooling_core.write_csv(
                out_path,
                rows,
                headers,
                meta_seed,
                extra_meta=extra_meta,
            )
            lines = out_path.read_text(encoding="utf-8").splitlines()

        self.assertGreaterEqual(len(lines), 3)
        self.assertTrue(lines[0].startswith("# meta: {"))
        meta_payload = json.loads(lines[0][8:])
        self.assertEqual(meta_payload["intent"], "csv-test")
        self.assertEqual(meta_payload["tool"], "test_suite")
        self.assertEqual(meta_payload["generated_at"], "2025-01-01T00:00:00Z")
        self.assertEqual(meta_payload["software_version"], "test-suite/1.0.0")
        self.assertEqual(lines[1], "r,g,b")
        self.assertEqual(lines[2], "1,2,3")
        self.assertEqual(lines[3], "4,5,6")

    def test_build_parser_requires_output(self) -> None:
        parser = tooling_core.build_parser("Test Parser")
        with self.assertRaises(SystemExit):
            with contextlib.redirect_stderr(io.StringIO()):
                parser.parse_args([])
        args = parser.parse_args(["--output", "dummy.json"])
        self.assertEqual(args.output, "dummy.json")

    def test_load_metadata_roundtrip(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "meta.json"
            path.write_text('{"foo": "bar"}', encoding="utf-8")
            data = tooling_core.load_metadata(path)
        self.assertEqual(data, {"foo": "bar"})


if __name__ == "__main__":
    unittest.main()
