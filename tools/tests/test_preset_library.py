import unittest

from tools import preset_library


class PresetLibraryTests(unittest.TestCase):
    def test_library_has_minimum_entries(self) -> None:
        self.assertGreaterEqual(len(preset_library.PRESET_LIBRARY), 20)

    def test_preset_ids_are_unique(self) -> None:
        ids = [preset.preset_id for preset in preset_library.PRESET_LIBRARY]
        self.assertEqual(len(ids), len(set(ids)))


if __name__ == "__main__":
    unittest.main()
