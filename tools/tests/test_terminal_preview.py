import io
import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from tools.previews import terminal_preview


class TerminalPreviewTests(unittest.TestCase):
    def test_apply_preview_mapping_adjusts_values(self) -> None:
        mapped = terminal_preview.apply_preview_mapping(
            (255, 128, 0),
            gamma=terminal_preview.DEFAULT_GAMMA,
            brightness=terminal_preview.DEFAULT_BRIGHTNESS,
            saturation=terminal_preview.DEFAULT_SATURATION,
        )
        self.assertTrue(all(0 <= c <= 255 for c in mapped))
        self.assertLess(mapped[1], 200)  # saturation/brightness reduce green intensity

    def test_downsample_frame_reduces_width(self) -> None:
        frame = [(i, i, i) for i in range(100)]
        downsampled = terminal_preview.downsample_frame(frame, width=10)
        self.assertEqual(len(downsampled), 10)
        self.assertTrue(all(isinstance(pixel, tuple) for pixel in downsampled))

    def test_render_frame_outputs_ansi(self) -> None:
        frame = [(255, 0, 0)] * 10
        config = terminal_preview.PreviewConfig(width=5, block="#")
        rendered = terminal_preview.render_frame(frame, config=config)
        self.assertIn("\033[38;2;", rendered)
        self.assertIn("#", rendered)

    def test_render_terminal_static(self) -> None:
        frame = [(255, 255, 255)] * 10
        buffer = io.StringIO()
        config = terminal_preview.PreviewConfig(width=5)
        terminal_preview.render_terminal(
            [frame],
            fps=10,
            loop=False,
            static=True,
            config=config,
            output=buffer,
            sleep_fn=lambda _: None,
        )
        output = buffer.getvalue()
        self.assertIn("\033[2J\033[H", output)
        self.assertTrue(output.endswith("\n"))

    def test_render_terminal_animation_calls_sleep(self) -> None:
        frame = [(255, 0, 0)] * 10
        config = terminal_preview.PreviewConfig(width=5)
        sleep_mock = mock.Mock()
        terminal_preview.render_terminal(
            [frame, frame],
            fps=20,
            loop=False,
            static=False,
            config=config,
            output=io.StringIO(),
            sleep_fn=sleep_mock,
        )
        sleep_mock.assert_called()

    def test_load_frames_supports_rgb(self) -> None:
        payload = {
            "version": "1.0",
            "meta": {"fps": 10},
            "data": {"rgb": [[0, 0, 0], [255, 255, 255]]},
        }
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "palette.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            frames, meta = terminal_preview.load_frames(path)
        self.assertEqual(len(frames), 1)
        self.assertEqual(meta["fps"], 10)
        self.assertEqual(len(frames[0]), 2)


if __name__ == "__main__":
    unittest.main()
