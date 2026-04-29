from collections import Counter, deque
from dataclasses import dataclass
from typing import Iterable, Protocol


THUMB_UP = "Thumb_Up"
THUMB_DOWN = "Thumb_Down"
NO_THUMB_GESTURE = "_"
THUMB_GESTURES = (THUMB_UP, THUMB_DOWN)


class GestureCategory(Protocol):
    category_name: str | None
    score: float | None


@dataclass(frozen=True)
class ThumbGestureConfig:
    min_score: float = 0.45
    window_size: int = 3
    min_votes: int = 2

    def __post_init__(self) -> None:
        if not 0.0 <= self.min_score <= 1.0:
            raise ValueError("min_score must be between 0 and 1")
        if self.window_size <= 0:
            raise ValueError("window_size must be positive")
        if self.min_votes <= 0 or self.min_votes > self.window_size:
            raise ValueError("min_votes must be in the range 1..window_size")


DEFAULT_THUMB_GESTURE_CONFIG = ThumbGestureConfig()


def select_thumb_gesture(
    gesture_categories: Iterable[GestureCategory],
    min_score: float = DEFAULT_THUMB_GESTURE_CONFIG.min_score,
) -> str:
    """Return the highest-confidence thumb gesture from a MediaPipe category list."""
    best_name = NO_THUMB_GESTURE
    best_score = min_score

    for category in gesture_categories:
        name = getattr(category, "category_name", None) or ""
        if name not in THUMB_GESTURES:
            continue

        score = float(getattr(category, "score", 0.0) or 0.0)
        if score >= best_score:
            best_name = name
            best_score = score

    return best_name


class ThumbGestureTracker:
    """Small per-hand smoother for thumb gestures.

    A 2-of-3 vote is intentionally less strict than requiring every frame to
    agree, which makes thumb-down usable when MediaPipe drops a frame.
    """

    def __init__(self, config: ThumbGestureConfig = DEFAULT_THUMB_GESTURE_CONFIG) -> None:
        self.config = config
        self._history: dict[str, deque[str]] = {}

    def update(self, label: str, gesture_categories: Iterable[GestureCategory]) -> tuple[bool, bool]:
        tag = select_thumb_gesture(gesture_categories, self.config.min_score)
        history = self._history.setdefault(label, deque(maxlen=self.config.window_size))
        history.append(tag)

        active_tag = self._stable_tag(history, tag)
        return active_tag == THUMB_UP, active_tag == THUMB_DOWN

    def reset(self, label: str) -> None:
        self._history.setdefault(label, deque(maxlen=self.config.window_size)).clear()

    def reset_all(self, labels: Iterable[str]) -> None:
        for label in labels:
            self.reset(label)

    def _stable_tag(self, history: deque[str], current_tag: str) -> str:
        if len(history) < self.config.min_votes:
            return NO_THUMB_GESTURE

        counts = Counter(item for item in history if item in THUMB_GESTURES)

        if current_tag in THUMB_GESTURES:
            return current_tag if counts[current_tag] >= self.config.min_votes else NO_THUMB_GESTURE

        for gesture_name in THUMB_GESTURES:
            if counts[gesture_name] >= self.config.min_votes:
                return gesture_name

        return NO_THUMB_GESTURE
