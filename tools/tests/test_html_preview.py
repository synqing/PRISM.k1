import io
import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from tools.previews import html_preview, terminal_preview


class HtmlPreviewTests(unittest.TestCase):
    def test_convert_plain_strips_ansi(self) -> None:
        ansi_text = "\033[38;2;255;0;0mRED\033[0m"
        html = html_preview.convert_plain(ansi_text)
        self.assertEqual(html, "<pre>RED</pre>")

    def test_assemble_html_uses_converter(self) -> None:
        frames = [[(255, 0, 0)] * 3]
        config = terminal_preview.PreviewConfig(width=3)
        with mock.patch("tools.previews.html_preview.convert_with_ansi2html", return_value="<span>ok</span>"):
            html = html_preview.assemble_html(frames, title="Test", config=config)
        self.assertIn("<span>ok</span>", html)
        self.assertIn("Test", html)

    def test_assemble_html_fallback_to_plain(self) -> None:
        frames = [[(255, 0, 0)] * 3]
        config = terminal_preview.PreviewConfig(width=3)
        with mock.patch("tools.previews.html_preview.convert_with_ansi2html", side_effect=RuntimeError("missing")),\
             mock.patch("tools.previews.html_preview.convert_with_aha", side_effect=RuntimeError("missing")):
            html = html_preview.assemble_html(frames, title="Fallback", config=config)
        self.assertIn("<pre>", html)
        self.assertIn("Fallback", html)

    def test_cli_writes_html_file(self) -> None:
        frames = [[(255, 255, 255)] * 2]
        payload = {"data": {"frames": frames}, "meta": {"fps": 10}}
        with tempfile.TemporaryDirectory() as tmp:
            input_path = Path(tmp) / "frames.json"
            output_path = Path(tmp) / "preview.html"
            input_path.write_text(json.dumps(payload), encoding="utf-8")
            with mock.patch("tools.previews.html_preview.convert_with_ansi2html", return_value="<span>frame</span>"):
                html_preview.main([
                    "--input", str(input_path),
                    "--output", str(output_path),
                    "--width", "2",
                ])
            content = output_path.read_text(encoding="utf-8")
        self.assertIn("meta:", content)
        self.assertIn("<span>frame</span>", content)


if __name__ == "__main__":
    unittest.main()
