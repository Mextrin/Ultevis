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
    global right_hand_top_notes, right_hand_bottom_notes, left_hand_top_notes, left_hand_bottom_notes, last_finger_wrist_dist

    keyboard_payload = {
        "instrument": "keyboard",
        "rightHandVisible": False,
        "leftHandVisible": False,
        "topNotesOn": "",
        "topNotesOff": "",
        "bottomNotesOn": "",
        "bottomNotesOff": "",
    }
    
    # This set is now ONLY for visual feedback on the frame of the press
    active_zone_names: set[str] = set()
    displayed_hand_positions: dict[str, tuple[float, float]] = {}
    
    current_right_top_notes = {}
    current_right_bottom_notes = {}
    current_left_top_notes = {}
    current_left_bottom_notes = {}
    current_finger_distances = {}

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
                
                current_dist = tip.y - wrist_landmark.y
                current_finger_distances[finger_id] = current_dist
                
                last_dist = last_finger_wrist_dist.get(finger_id, current_dist)
                dist_velocity = current_dist - last_dist

                mirrored_x = 1.0 - tip.x
                zone = find_key_zone(mirrored_x, tip.y)

                # Determine if the note for this finger is currently "on"
                is_top = zone.name.endswith("+") if zone else False
                is_note_on = (label == "Right" and zone and zone.note in (right_hand_top_notes if is_top else right_hand_bottom_notes)) or \
                             (label == "Left"  and zone and zone.note in (left_hand_top_notes  if is_top else left_hand_bottom_notes))

                if zone:
                    #press
                    finger_pressed = dist_velocity > PRESS_DISTANCE_THRESHOLD
                    # The 'finger_lifted' and 'finger_repressed' variables are no longer needed here.

                    # We only care about a new press. If a press is detected, the note is 'on' for this frame.
                    # If it's not detected, the note is 'off'. Simple as that.
                    if finger_pressed:
                        active_zone_names.add(zone.name)
                        if label == "Right":
                            (current_right_top_notes if is_top else current_right_bottom_notes)[zone.note] = True
                        else:
                            (current_left_top_notes if is_top else current_left_bottom_notes)[zone.note] = True
                
                # REMOVE THE ENTIRE 'elif is_note_on:' BLOCK

    # Update the last known distances for the next frame
    last_finger_wrist_dist = current_finger_distances

    # Determine which notes to turn ON
    top_on    = [n for n in current_right_top_notes    if n not in right_hand_top_notes] + \
                [n for n in current_left_top_notes     if n not in left_hand_top_notes]
    bottom_on = [n for n in current_right_bottom_notes if n not in right_hand_bottom_notes] + \
                [n for n in current_left_bottom_notes  if n not in left_hand_bottom_notes]
    if top_on:    keyboard_payload["topNotesOn"]    = " ".join(map(str, top_on))
    if bottom_on: keyboard_payload["bottomNotesOn"] = " ".join(map(str, bottom_on))

    # Determine which notes to turn OFF
    top_off    = [n for n in right_hand_top_notes    if n not in current_right_top_notes] + \
                 [n for n in left_hand_top_notes     if n not in current_left_top_notes]
    bottom_off = [n for n in right_hand_bottom_notes if n not in current_right_bottom_notes] + \
                 [n for n in left_hand_bottom_notes  if n not in current_left_bottom_notes]
    if top_off:    keyboard_payload["topNotesOff"]    = " ".join(map(str, top_off))
    if bottom_off: keyboard_payload["bottomNotesOff"] = " ".join(map(str, bottom_off))

    # Update state for next frame
    right_hand_top_notes    = current_right_top_notes
    right_hand_bottom_notes = current_right_bottom_notes
    left_hand_top_notes     = current_left_top_notes
    left_hand_bottom_notes  = current_left_bottom_notes
    
    return keyboard_payload, active_zone_names, displayed_hand_positions
