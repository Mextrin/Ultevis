import sys
import unittest
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT / "src" / "mediapipe"))

from keyboard_utils import (
    BOTTOM_OCTAVE_Y_START,
    KEYBOARD_ZONES,
    TOP_OCTAVE_Y_START,
    WHITE_KEYS_PER_KEYBOARD,
)


class KeyboardLayoutTests(unittest.TestCase):
    def test_each_keyboard_displays_ten_white_keys(self) -> None:
        white_zones = [zone for zone in KEYBOARD_ZONES if "#" not in zone.name]
        top_white_zones = [zone for zone in white_zones if zone.top == TOP_OCTAVE_Y_START]
        bottom_white_zones = [zone for zone in white_zones if zone.top == BOTTOM_OCTAVE_Y_START]

        self.assertEqual(len(top_white_zones), WHITE_KEYS_PER_KEYBOARD)
        self.assertEqual(len(bottom_white_zones), WHITE_KEYS_PER_KEYBOARD)

    def test_white_keys_fill_the_full_keyboard_width(self) -> None:
        for y_start in (TOP_OCTAVE_Y_START, BOTTOM_OCTAVE_Y_START):
            zones = [zone for zone in KEYBOARD_ZONES if "#" not in zone.name and zone.top == y_start]

            self.assertAlmostEqual(zones[0].left, 0.0)
            self.assertAlmostEqual(zones[-1].right, 1.0)
            for current_zone, next_zone in zip(zones, zones[1:]):
                self.assertAlmostEqual(current_zone.right, next_zone.left)


if __name__ == "__main__":
    unittest.main()
