import sys
import unittest
from pathlib import Path
from types import SimpleNamespace


PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT / "src" / "mediapipe"))

from keyboard_gestures import THUMB_DOWN, THUMB_UP, ThumbGestureTracker, select_thumb_gesture


def category(name: str, score: float) -> SimpleNamespace:
    return SimpleNamespace(category_name=name, score=score)


class ThumbGestureSelectionTests(unittest.TestCase):
    def test_selects_thumb_down_from_full_category_list(self) -> None:
        result = select_thumb_gesture([
            category("None", 0.92),
            category(THUMB_DOWN, 0.46),
        ])

        self.assertEqual(result, THUMB_DOWN)

    def test_thumb_up_and_down_use_the_same_threshold(self) -> None:
        self.assertEqual(select_thumb_gesture([category(THUMB_UP, 0.45)]), THUMB_UP)
        self.assertEqual(select_thumb_gesture([category(THUMB_DOWN, 0.45)]), THUMB_DOWN)

    def test_rejects_thumb_candidates_below_threshold(self) -> None:
        self.assertEqual(select_thumb_gesture([category(THUMB_DOWN, 0.44)]), "_")


class ThumbGestureTrackerTests(unittest.TestCase):
    def test_two_of_three_frames_stabilizes_thumb_down_with_one_drop(self) -> None:
        tracker = ThumbGestureTracker()

        self.assertEqual(tracker.update("Right", [category(THUMB_DOWN, 0.55)]), (False, False))
        self.assertEqual(tracker.update("Right", []), (False, False))
        self.assertEqual(tracker.update("Right", [category(THUMB_DOWN, 0.50)]), (False, True))

    def test_direction_change_resets_until_new_direction_is_stable(self) -> None:
        tracker = ThumbGestureTracker()

        tracker.update("Left", [category(THUMB_DOWN, 0.55)])
        tracker.update("Left", [category(THUMB_DOWN, 0.55)])
        self.assertEqual(tracker.update("Left", [category(THUMB_DOWN, 0.55)]), (False, True))

        self.assertEqual(tracker.update("Left", [category(THUMB_UP, 0.55)]), (False, False))
        self.assertEqual(tracker.update("Left", [category(THUMB_UP, 0.55)]), (True, False))

    def test_reset_clears_stale_hold_state(self) -> None:
        tracker = ThumbGestureTracker()

        tracker.update("Right", [category(THUMB_UP, 0.55)])
        self.assertEqual(tracker.update("Right", [category(THUMB_UP, 0.55)]), (True, False))
        tracker.reset("Right")

        self.assertEqual(tracker.update("Right", [category(THUMB_UP, 0.55)]), (False, False))


if __name__ == "__main__":
    unittest.main()
