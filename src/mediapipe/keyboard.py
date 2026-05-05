from __future__ import annotations

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
# Keep track of which notes are currently being played by each hand, split by keyboard
right_hand_top_notes = {}
right_hand_bottom_notes = {}
left_hand_top_notes = {}
left_hand_bottom_notes = {}

# Keep track of the last finger-to-wrist Y-distance to detect a "press"
last_finger_wrist_dist = {} # {(hand_label, finger_name): distance}
latched_finger_zones = {} # {(hand_label, finger_name): KeyboardZone}
finger_press_armed = {} # {(hand_label, finger_name): bool}

# Threshold for detecting a "press" (finger moving towards wrist)
# A negative value means the finger is moving closer to the wrist.
PRESS_DISTANCE_THRESHOLD = 0.015 

# Add this new constant
REPRESS_DISTANCE_THRESHOLD = 0.005

# Threshold for detecting a "lift" to allow re-pressing the same key
# A positive value means the finger is moving away from the wrist.
LIFT_DISTANCE_THRESHOLD = -0.01
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

        # Calculate distance between thumb and index finger
        pinch_distance = ((thumb_tip.x - index_tip.x) ** 2 + (thumb_tip.y - index_tip.y) ** 2) ** 0.5
        is_pinching = pinch_distance < PINCH_THRESHOLD

        gesture_categories = []
        if idx < len(gestures_per_hand) and gestures_per_hand[idx]:
            gesture_categories = gestures_per_hand[idx]
        thumb_up, thumb_down = _thumb_tracker.update(label, gesture_categories)

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
            _thumb_tracker.reset(label)

    return keyboard_payload


def draw_keyboard_zones(frame, active_zone_names: set[str]) -> None:
    return


def find_key_zone(x: float, y: float) -> KeyboardZone | None:
    # Prioritize black keys in hit detection
    sorted_zones = sorted(KEYBOARD_ZONES, key=lambda z: "#" in z.name, reverse=True)
    for zone in sorted_zones:
        if zone.contains(x, y):
            return zone
    return None


def detect_key_strokes(detection_result):
    global right_hand_top_notes, right_hand_bottom_notes, left_hand_top_notes, left_hand_bottom_notes
    global last_finger_wrist_dist, latched_finger_zones, finger_press_armed

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
    
    # This set is now ONLY for visual feedback on the frame of the press
    active_zone_names: set[str] = set()
    displayed_hand_positions: dict[str, tuple[float, float]] = {}
    
    current_right_top_notes = {}
    current_right_bottom_notes = {}
    current_left_top_notes = {}
    current_left_bottom_notes = {}
    current_finger_distances = {}
    seen_finger_ids: set[tuple[str, str]] = set()

    if detection_result.handedness:
        processed_labels = set()
        for hand_landmarks, handedness in zip(detection_result.hand_landmarks, detection_result.handedness):
            label = handedness[0].category_name
            if label in processed_labels:
                continue
            processed_labels.add(label)

            wrist_landmark = hand_landmarks[0]
            finger_tips = {
                "thumb": hand_landmarks[4], "index": hand_landmarks[8],
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
                
                finger_id = (label, name)
                seen_finger_ids.add(finger_id)
                
                current_dist = tip.y - wrist_landmark.y
                current_finger_distances[finger_id] = current_dist
                
                last_dist = last_finger_wrist_dist.get(finger_id, current_dist)
                dist_velocity = current_dist - last_dist

                mirrored_x = 1.0 - tip.x
                zone = find_key_zone(mirrored_x, tip.y)
                latched_zone = latched_finger_zones.get(finger_id)
                finger_lifted = dist_velocity < LIFT_DISTANCE_THRESHOLD
                zone_matches_latch = (
                    latched_zone is not None
                    and zone is not None
                    and zone.name == latched_zone.name
                    and zone.note == latched_zone.note
                )

                if latched_zone is not None and (finger_lifted or not zone_matches_latch):
                    latched_finger_zones.pop(finger_id, None)
                    latched_zone = None
                    zone_matches_latch = False

                active_zone = latched_zone if zone_matches_latch else None

                # Rearm only after the finger actually lifts or leaves the key area.
                if active_zone is None and (zone is None or finger_lifted):
                    finger_press_armed[finger_id] = True

                if active_zone is None and zone is not None:
                    finger_pressed = finger_press_armed.get(finger_id, True) and dist_velocity > PRESS_DISTANCE_THRESHOLD
                    if finger_pressed:
                        latched_finger_zones[finger_id] = zone
                        finger_press_armed[finger_id] = False
                        active_zone = zone
                        active_zone_names.add(zone.name)

                if active_zone is not None:
                    is_top = active_zone.name.endswith("+")
                    if label == "Right":
                        (current_right_top_notes if is_top else current_right_bottom_notes)[active_zone.note] = True
                    else:
                        (current_left_top_notes if is_top else current_left_bottom_notes)[active_zone.note] = True

    lost_finger_ids = set(latched_finger_zones) - seen_finger_ids
    for finger_id in lost_finger_ids:
        latched_finger_zones.pop(finger_id, None)
        finger_press_armed.pop(finger_id, None)

    for finger_id in set(last_finger_wrist_dist) - seen_finger_ids:
        last_finger_wrist_dist.pop(finger_id, None)

    # Update the last known distances for the next frame
    last_finger_wrist_dist = current_finger_distances

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
