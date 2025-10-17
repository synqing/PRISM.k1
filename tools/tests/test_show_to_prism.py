import json
import math
import tempfile
import unittest
from pathlib import Path

from tools import show_to_prism


class ShowToPrismTests(unittest.TestCase):
    def setUp(self) -> None:
        self.palette = ["#ff0000", "#00ff00", "#0000ff"]

    def test_palette_sampler_bounds(self) -> None:
        sampler = show_to_prism.build_palette_sampler(self.palette, "hsv")
        for value in [0.0, 0.25, 0.5, 0.75, 1.0]:
            r, g, b = sampler.sample(value)
            self.assertTrue(0 <= r <= 255)
            self.assertTrue(0 <= g <= 255)
            self.assertTrue(0 <= b <= 255)

    def test_sine_show_deterministic(self) -> None:
        sampler = show_to_prism.build_palette_sampler(self.palette, "hsv")
        params = show_to_prism.collect_params(
            show_to_prism.build_cli_parser().parse_args([
                "--show", "sine",
                "--palette", ",".join(self.palette),
                "--led-count", "4",
                "--duration", "1",
                "--fps", "2",
                "--output", "out.json",
            ])
        )
        frames_a, _ = show_to_prism.generate_show(
            "sine",
            palette=sampler,
            led_count=4,
            fps=2,
            duration=1,
            seed_numeric=None,
            params=params,
        )
        frames_b, _ = show_to_prism.generate_show(
            "sine",
            palette=sampler,
            led_count=4,
            fps=2,
            duration=1,
            seed_numeric=None,
            params=params,
        )
        self.assertEqual(frames_a, frames_b)
        self.assertEqual(len(frames_a), 2)
        self.assertEqual(len(frames_a[0]), 4)

    def test_sine_show_seed_changes_phase(self) -> None:
        sampler = show_to_prism.build_palette_sampler(self.palette, "hsv")
        params = {
            "wave_amplitude": 0.35,
            "wave_frequency": 1.0,
            "wave_speed": 0.5,
            "wave_direction": 1.0,
            "wave_phase": 0.0,
            "noise_scale": 1.0,
            "noise_speed": 0.4,
            "noise_octaves": 2,
            "noise_persistence": 0.5,
            "noise_lacunarity": 2.0,
            "flow_field_scale": 1.0,
            "flow_step_size": 0.2,
            "flow_speed": 0.3,
            "flow_curl": 0.5,
            "flow_octaves": 2,
            "flow_persistence": 0.6,
            "flow_lacunarity": 2.0,
        }
        frames_seed_1, _ = show_to_prism.generate_show(
            "sine",
            palette=sampler,
            led_count=6,
            fps=4,
            duration=0.5,
            seed_numeric=123,
            params=params,
        )
        frames_seed_2, _ = show_to_prism.generate_show(
            "sine",
            palette=sampler,
            led_count=6,
            fps=4,
            duration=0.5,
            seed_numeric=456,
            params=params,
        )
        self.assertNotEqual(frames_seed_1, frames_seed_2)

    def test_noise_show_range(self) -> None:
        sampler = show_to_prism.build_palette_sampler(self.palette, "hsv")
        params = {
            "wave_amplitude": 0.4,
            "wave_frequency": 1.0,
            "wave_speed": 0.5,
            "wave_direction": 1.0,
            "wave_phase": 0.0,
            "noise_scale": 1.2,
            "noise_speed": 0.4,
            "noise_octaves": 2,
            "noise_persistence": 0.5,
            "noise_lacunarity": 2.0,
            "flow_field_scale": 1.0,
            "flow_step_size": 0.2,
            "flow_speed": 0.3,
            "flow_curl": 0.5,
            "flow_octaves": 2,
            "flow_persistence": 0.6,
            "flow_lacunarity": 2.0,
        }
        frames, _ = show_to_prism.generate_show(
            "noise",
            palette=sampler,
            led_count=5,
            fps=5,
            duration=0.5,
            seed_numeric=123,
            params=params,
        )
        for frame in frames:
            for pixel in frame:
                self.assertTrue(all(0 <= channel <= 255 for channel in pixel))

    def test_cli_generates_output(self) -> None:
        payload = None
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "show.json"
            csv_path = Path(tmp) / "show.csv"
            args = [
                "--show", "sine",
                "--palette", ",".join(self.palette),
                "--led-count", "6",
                "--duration", "0.5",
                "--fps", "4",
                "--output", str(output),
                "--ramp-space", "hsv",
                "--csv", str(csv_path),
                "--seed", "42",
            ]
            show_to_prism.main(args)
            payload = json.loads(output.read_text(encoding="utf-8"))
            csv_content = csv_path.read_text(encoding="utf-8")
        self.assertIn("frames", payload["data"])
        meta = payload["meta"]
        self.assertEqual(meta["show_type"], "sine")
        self.assertIn("show_params", meta)
        self.assertEqual(set(meta["show_params"].keys()), {"wave_amplitude", "wave_frequency", "wave_speed", "wave_direction", "wave_phase"})
        self.assertEqual(meta["show_params"]["wave_amplitude"], 0.45)
        self.assertTrue(csv_content.startswith("# meta: "))

    def test_validation_rejects_invalid_amplitude(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "show.json"
            with self.assertRaises(SystemExit):
                show_to_prism.main([
                    "--show", "sine",
                    "--palette", ",".join(self.palette),
                    "--led-count", "5",
                    "--duration", "1",
                    "--fps", "24",
                    "--wave-amplitude", "1.5",
                    "--output", str(output),
                ])


if __name__ == "__main__":
    unittest.main()
