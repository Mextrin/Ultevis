import mediapipe as mp
import cv2
from collections import deque
from pathlib import Path
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from mediapipe.tasks.python.vision import RunningMode
from keyboard_utils import KeyboardZone, PINCH_THRESHOLD, KEYBOARD_ZONES


_GESTURE_MODEL_PATH = Path(__file__).with_name("gesture_recognizer.task")
if not _GESTURE_MODEL_PATH.exists():
    raise FileNotFoundError(f"MediaPipe gesture recognizer model not found: {_GESTURE_MODEL_PATH}")

_ClassifierOptions = mp.tasks.components.processors.ClassifierOptions
_keyboard_gesture_options = vision.GestureRecognizerOptions(
    base_options=python.BaseOptions(model_asset_path=str(_GESTURE_MODEL_PATH)),
    running_mode=RunningMode.VIDEO,
    num_hands=2,
    min_hand_detection_confidence=0.5,
    min_tracking_confidence=0.5,
    canned_gesture_classifier_options=_ClassifierOptions(score_threshold=0.55),
)
recognizer = vision.GestureRecognizer.create_from_options(_keyboard_gesture_options)

# Per-hand debounce: require N consecutive frames classifying the same thumb gesture
# before we flip the boolean. Stops single-frame misclassifications from bumping octaves.
_THUMB_DEBOUNCE_FRAMES = 3
_THUMB_MIN_SCORE = 0.6
_thumb_history: dict[str, deque] = {
    "Left": deque(maxlen=_THUMB_DEBOUNCE_FRAMES),
    "Right": deque(maxlen=_THUMB_DEBOUNCE_FRAMES),
}


def _stable_thumb_state(label: str, gesture_name: str, score: float) -> tuple[bool, bool]:
    history = _thumb_history.setdefault(label, deque(maxlen=_THUMB_DEBOUNCE_FRAMES))
    tag = gesture_name if score >= _THUMB_MIN_SCORE and gesture_name in ("Thumb_Up", "Thumb_Down") else "_"
    history.append(tag)
    if len(history) < _THUMB_DEBOUNCE_FRAMES:
        return False, False
    first = history[0]
    if any(item != first for item in history):
        return False, False
    return first == "Thumb_Up", first == "Thumb_Down"


def _reset_thumb_history(label: str) -> None:
    _thumb_history.setdefault(label, deque(maxlen=_THUMB_DEBOUNCE_FRAMES)).clear()

# --- State Tracking ---
# Keep track of which notes are currently being played by each hand
left_hand_notes = {}  # {note: True}
right_hand_notes = {} # {note: True}
# --------------------

def detect_keyboard_hands(detection_result):
    keyboard_payload = {
        "instrument": "keyboard", # Optimization tag for C++!
        "rightHandVisible": False,
        "leftHandVisible": False,
        "rightHandX": 0.0,
        "rightHandY": 0.0,
        "leftHandX": 0.0,
        "leftHandY": 0.0,
        "rightPinch": False,
        "leftPinch": False,
        "rightThumbUp": False,
        "rightThumbDown": False,
        "leftThumbUp": False,
        "leftThumbDown": False,
    }

    if not detection_result.handedness:
        for label in ("Left", "Right"):
            _reset_thumb_history(label)
        return keyboard_payload

    gestures_per_hand = getattr(detection_result, "gestures", None) or []

    processed_labels = set()
    seen_labels = set()
    for idx, (hand_landmarks, handedness) in enumerate(zip(detection_result.hand_landmarks, detection_result.handedness)):
        label = handedness[0].category_name
        if label in processed_labels:
            continue

        processed_labels.add(label)
        seen_labels.add(label)
        thumb_tip = hand_landmarks[4]
        index_tip = hand_landmarks[8]

        # Calculate distance between thumb and index finger
        pinch_distance = ((thumb_tip.x - index_tip.x) ** 2 + (thumb_tip.y - index_tip.y) ** 2) ** 0.5
        is_pinching = pinch_distance < PINCH_THRESHOLD

        gesture_name = ""
        gesture_score = 0.0
        if idx < len(gestures_per_hand) and gestures_per_hand[idx]:
            top = gestures_per_hand[idx][0]
            gesture_name = top.category_name or ""
            gesture_score = float(top.score or 0.0)
        thumb_up, thumb_down = _stable_thumb_state(label, gesture_name, gesture_score)

        hand_center_x = sum(landmark.x for landmark in hand_landmarks) / len(hand_landmarks)
        hand_center_y = sum(landmark.y for landmark in hand_landmarks) / len(hand_landmarks)

        # The camera feed is mirrored for display, so QML hit-tests use display-space X.
        display_x = max(0.0, min(1.0, 1.0 - hand_center_x))
        display_y = max(0.0, min(1.0, hand_center_y))

        if label == "Right":
            keyboard_payload["rightHandVisible"] = True
            keyboard_payload["rightHandX"] = display_x
            keyboard_payload["rightHandY"] = display_y
            keyboard_payload["rightPinch"] = is_pinching
            keyboard_payload["rightThumbUp"] = thumb_up
            keyboard_payload["rightThumbDown"] = thumb_down
        else:
            keyboard_payload["leftHandVisible"] = True
            keyboard_payload["leftHandX"] = display_x
            keyboard_payload["leftHandY"] = display_y
            keyboard_payload["leftPinch"] = is_pinching
            keyboard_payload["leftThumbUp"] = thumb_up
            keyboard_payload["leftThumbDown"] = thumb_down

    # If a hand disappeared this frame, clear its debounce so a re-entry
    # has to re-confirm a gesture from scratch instead of triggering on stale state.
    for label in ("Left", "Right"):
        if label not in seen_labels:
            _reset_thumb_history(label)

    return keyboard_payload


def draw_keyboard_zones(frame, active_zone_names: set[str]) -> None:
    overlay = frame.copy()
    height, width = frame.shape[:2]

    # Sort keys to draw white keys first, then black keys
    sorted_zones = sorted(KEYBOARD_ZONES, key=lambda z: "#" in z.name)

    for zone in sorted_zones:
        left, top, right, bottom = zone.pixel_rect(width, height)
        is_active = zone.name in active_zone_names
        is_black_key = "#" in zone.name

        if is_black_key:
            fill_color = (0, 120, 255) if is_active else (0, 0, 0)
            border_color = (0, 180, 255) if is_active else (80, 80, 80)
        else:
            fill_color = (0, 120, 255) if is_active else (255, 255, 255)
            border_color = (0, 180, 255) if is_active else (150, 150, 150)

        cv2.rectangle(overlay, (left, top), (right, bottom), fill_color, -1)
        cv2.rectangle(overlay, (left, top), (right, bottom), border_color, 2)

    cv2.addWeighted(overlay, 0.3, frame, 0.7, 0.0, frame)


def find_key_zone(x: float, y: float) -> KeyboardZone | None:
    # Prioritize black keys in hit detection
    sorted_zones = sorted(KEYBOARD_ZONES, key=lambda z: "#" in z.name, reverse=True)
    for zone in sorted_zones:
        if zone.contains(x, y):
            return zone
    return None


def detect_key_strokes(detection_result):
    global left_hand_notes, right_hand_notes

    keyboard_payload = {
        "instrument": "keyboard",
        "rightHandVisible": False,
        "leftHandVisible": False,
        "notesOn": [],
        "notesOff": [],
    }

    active_zone_names: set[str] = set()
    displayed_hand_positions: dict[str, tuple[float, float]] = {}

    current_left_notes = {}
    current_right_notes = {}

    if detection_result.handedness:
        processed_labels = set()
        for hand_landmarks, handedness in zip(detection_result.hand_landmarks, detection_result.handedness):
            label = handedness[0].category_name
            if label in processed_labels:
                continue
            processed_labels.add(label)

            # Define all finger tips
            finger_tips = {
                "thumb": hand_landmarks[4],
                "index": hand_landmarks[8],
                "middle": hand_landmarks[12],
                "ring": hand_landmarks[16],
                "pinky": hand_landmarks[20],
            }

            # Set primary hand position using the index finger
            display_x = 1.0 - finger_tips["index"].x
            display_y = finger_tips["index"].y
            displayed_hand_positions[label] = (display_x, display_y)

            # Set visibility and finger coordinates in the payload
            prefix = "right" if label == "Right" else "left"
            keyboard_payload[f"{prefix}HandVisible"] = True
            for name, tip in finger_tips.items():
                keyboard_payload[f"{prefix}_{name}_X"] = 1.0 - tip.x
                keyboard_payload[f"{prefix}_{name}_Y"] = tip.y

            # Check for key presses with each finger
            for tip in finger_tips.values():
                mirrored_x = 1.0 - tip.x
                zone = find_key_zone(mirrored_x, tip.y)
            
                if zone:
                    active_zone_names.add(zone.name)
                    if label == "Right":
                        current_right_notes[zone.note] = True
                    else:
                        current_left_notes[zone.note] = True

    # Determine which notes to turn ON
    new_right_notes = [note for note in current_right_notes if note not in right_hand_notes]
    new_left_notes = [note for note in current_left_notes if note not in left_hand_notes]

    new_on = new_right_notes + new_left_notes
    keyboard_payload["notesOn"] = " ".join(map(str, new_on))

    # Determine which notes to turn OFF
    stopped_right_notes = [note for note in right_hand_notes if note not in current_right_notes]
    stopped_left_notes = [note for note in left_hand_notes if note not in current_left_notes]

    new_off = stopped_right_notes + stopped_left_notes
    keyboard_payload["notesOff"] = " ".join(map(str, new_off))

    # Update the state for the next frame
    right_hand_notes = current_right_notes
    left_hand_notes = current_left_notes

    return keyboard_payload, active_zone_names, displayed_hand_positions
