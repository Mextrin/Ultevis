from __future__ import annotations

import math
import mediapipe as mp
import cv2
from pathlib import Path
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from mediapipe.tasks.python.vision import RunningMode
from keyboard_gestures import ThumbGestureTracker
from keyboard_utils import KeyboardZone, PINCH_THRESHOLD, KEYBOARD_ZONES


_GESTURE_MODEL_PATH = Path(__file__).with_name("gesture_recognizer.task")
if not _GESTURE_MODEL_PATH.exists():
    raise FileNotFoundError(f"MediaPipe gesture recognizer model not found: {_GESTURE_MODEL_PATH}")

_ClassifierOptions = mp.tasks.components.processors.ClassifierOptions
_keyboard_gesture_options = vision.GestureRecognizerOptions(
    base_options=python.BaseOptions(model_asset_path=str(_GESTURE_MODEL_PATH)),
    running_mode=RunningMode.VIDEO,
    num_hands=2,
    min_hand_detection_confidence=0.7,
    min_tracking_confidence=0.7,
    canned_gesture_classifier_options=_ClassifierOptions(score_threshold=0.35),
)
recognizer = vision.GestureRecognizer.create_from_options(_keyboard_gesture_options)

_thumb_tracker = ThumbGestureTracker()

# --- State Tracking ---
right_hand_top_notes = {}
right_hand_bottom_notes = {}
left_hand_top_notes = {}
left_hand_bottom_notes = {}

# Keep track of individual finger states for holding notes and preventing flickering
active_finger_presses = {} # {(hand_label, finger_name): bool}

# --- NEW KINEMATIC BEND THRESHOLDS ---
# We calculate the ratio of the direct distance (Wrist to Tip) vs the path (Wrist -> Knuckle -> Tip).
# 1.0 means perfectly straight. Lower numbers mean the knuckle is bent.
# This topological approach makes the trigger 100% immune to hand rotation.
PRESS_ON_BEND_RATIO = 0.85  # Must bend knuckle significantly to trigger (roughly 65 degrees)
PRESS_OFF_BEND_RATIO = 0.91 # Must straighten hand to release (roughly 45 degrees)

FINGER_DIRECTION_LANDMARKS = {
    "thumb": (2, 4),
    "index": (5, 8),
    "middle": (9, 12),
    "ring": (13, 16),
    "pinky": (17, 20),
}


def dist3d(p1, p2) -> float:
    """Helper to safely calculate true 3D distance between two landmarks."""
    return ((p1.x - p2.x)**2 + (p1.y - p2.y)**2 + (p1.z - p2.z)**2)**0.5


def detect_keyboard_hands(detection_result):
    keyboard_payload = {
        "instrument": "keyboard", 
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
        _thumb_tracker.reset_all(("Left", "Right"))
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

        pinch_distance = ((thumb_tip.x - index_tip.x) ** 2 + (thumb_tip.y - index_tip.y) ** 2) ** 0.5
        is_pinching = pinch_distance < PINCH_THRESHOLD

        gesture_categories = []
        if idx < len(gestures_per_hand) and gestures_per_hand[idx]:
            gesture_categories = gestures_per_hand[idx]
        thumb_up, thumb_down = _thumb_tracker.update(label, gesture_categories)

        hand_center_x = sum(landmark.x for landmark in hand_landmarks) / len(hand_landmarks)
        hand_center_y = sum(landmark.y for landmark in hand_landmarks) / len(hand_landmarks)

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

    for label in ("Left", "Right"):
        if label not in seen_labels:
            _thumb_tracker.reset(label)

    return keyboard_payload


def draw_keyboard_zones(frame, active_zone_names: set[str]) -> None:
    return


def find_key_zone(x: float, y: float) -> KeyboardZone | None:
    sorted_zones = sorted(KEYBOARD_ZONES, key=lambda z: "#" in z.name, reverse=True)
    for zone in sorted_zones:
        if zone.contains(x, y):
            return zone
    return None


def is_finger_pressed(hand_label: str, finger_name: str, hand_landmarks, tip_y: float) -> bool:
    """Calculates if a finger is pressed using dynamic, perspective-aware skeleton topology."""
    global active_finger_presses

    base_idx, tip_idx = FINGER_DIRECTION_LANDMARKS[finger_name]
    wrist = hand_landmarks[0]
    base = hand_landmarks[base_idx]
    tip = hand_landmarks[tip_idx]

    w_b = dist3d(wrist, base)
    b_t = dist3d(base, tip)
    w_t = dist3d(wrist, tip)

    path_len = w_b + b_t
    if path_len < 1e-6:
        return False

    ratio = w_t / path_len
    
    on_threshold = PRESS_ON_BEND_RATIO
    off_threshold = PRESS_OFF_BEND_RATIO

    if finger_name == "pinky":
        on_threshold += 0.035
        off_threshold += 0.035
    
    if tip_y < 0.5:
        on_threshold += 0.05
        off_threshold += 0.05

    finger_id = (hand_label, finger_name)
    is_pressed = active_finger_presses.get(finger_id, False)

    if is_pressed:
        if ratio > off_threshold:
            is_pressed = False
    else:
        if ratio < on_threshold:
            is_pressed = True

    active_finger_presses[finger_id] = is_pressed
    return is_pressed


def detect_key_strokes(detection_result):
    global right_hand_top_notes, right_hand_bottom_notes, left_hand_top_notes, left_hand_bottom_notes

    keyboard_payload = {
        "instrument": "keyboard",
        "rightHandVisible": False,
        "leftHandVisible": False,
        "leftTopNotesOn": "",
        "leftTopNotesOff": "",
        "rightTopNotesOn": "",
        "rightTopNotesOff": "",
        "leftBottomNotesOn": "",
        "leftBottomNotesOff": "",
        "rightBottomNotesOn": "",
        "rightBottomNotesOff": "",
    }
    
    active_zone_names: set[str] = set()
    displayed_hand_positions: dict[str, tuple[float, float]] = {}
    
    current_right_top_notes = {}
    current_right_bottom_notes = {}
    current_left_top_notes = {}
    current_left_bottom_notes = {}

    if detection_result.handedness:
        processed_labels = set()
        for hand_landmarks, handedness in zip(detection_result.hand_landmarks, detection_result.handedness):
            label = handedness[0].category_name
            if label in processed_labels:
                continue
            processed_labels.add(label)

            finger_tips = {
                "index": hand_landmarks[8],
                "middle": hand_landmarks[12], "ring": hand_landmarks[16], "pinky": hand_landmarks[20],
            }

            display_x = 1.0 - finger_tips["index"].x
            display_y = finger_tips["index"].y
            displayed_hand_positions[label] = (display_x, display_y)

            prefix = "right" if label == "Right" else "left"
            keyboard_payload[f"{prefix}HandVisible"] = True
            
            for name, tip in finger_tips.items():
                keyboard_payload[f"{prefix}_{name}_X"] = 1.0 - tip.x
                keyboard_payload[f"{prefix}_{name}_Y"] = tip.y
                
                mirrored_x = 1.0 - tip.x
                zone = find_key_zone(mirrored_x, tip.y)

                is_top = zone.name.endswith("+") if zone else False

                if zone:
                    if is_finger_pressed(label, name, hand_landmarks, tip.y):
                        active_zone_names.add(zone.name)
                        if label == "Right":
                            (current_right_top_notes if is_top else current_right_bottom_notes)[zone.note] = True
                        else:
                            (current_left_top_notes if is_top else current_left_bottom_notes)[zone.note] = True

    # Determine which notes to turn ON
    right_top_on = [n for n in current_right_top_notes if n not in right_hand_top_notes]
    left_top_on = [n for n in current_left_top_notes if n not in left_hand_top_notes]
    right_bottom_on = [n for n in current_right_bottom_notes if n not in right_hand_bottom_notes]
    left_bottom_on = [n for n in current_left_bottom_notes if n not in left_hand_bottom_notes]
    if right_top_on:
        keyboard_payload["rightTopNotesOn"] = " ".join(map(str, right_top_on))
    if left_top_on:
        keyboard_payload["leftTopNotesOn"] = " ".join(map(str, left_top_on))
    if right_bottom_on:
        keyboard_payload["rightBottomNotesOn"] = " ".join(map(str, right_bottom_on))
    if left_bottom_on:
        keyboard_payload["leftBottomNotesOn"] = " ".join(map(str, left_bottom_on))

    # Determine which notes to turn OFF
    right_top_off = [n for n in right_hand_top_notes if n not in current_right_top_notes]
    left_top_off = [n for n in left_hand_top_notes if n not in current_left_top_notes]
    right_bottom_off = [n for n in right_hand_bottom_notes if n not in current_right_bottom_notes]
    left_bottom_off = [n for n in left_hand_bottom_notes if n not in current_left_bottom_notes]
    if right_top_off:
        keyboard_payload["rightTopNotesOff"] = " ".join(map(str, right_top_off))
    if left_top_off:
        keyboard_payload["leftTopNotesOff"] = " ".join(map(str, left_top_off))
    if right_bottom_off:
        keyboard_payload["rightBottomNotesOff"] = " ".join(map(str, right_bottom_off))
    if left_bottom_off:
        keyboard_payload["leftBottomNotesOff"] = " ".join(map(str, left_bottom_off))

    # Update state for next frame
    right_hand_top_notes    = current_right_top_notes
    right_hand_bottom_notes = current_right_bottom_notes
    left_hand_top_notes     = current_left_top_notes
    left_hand_bottom_notes  = current_left_bottom_notes
    
    return keyboard_payload, active_zone_names, displayed_hand_positions