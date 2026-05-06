"""
guitar.py — Hand tracking for Guitar mode.

Strum detection and neck-position control are handled entirely in QML
(pinch rising-edge inside the strum zone, and thumb up/down via
OctaveGestureHoldController), so Python only needs to track:

  • Hand visibility + positions
  • Pinch state (hysteresis, same as drums.py)
  • Thumb-up / thumb-down gestures (same thresholds as keyboard.py)
"""

# ── Thresholds ────────────────────────────────────────────────────────────────
PINCH_ON_THRESHOLD     = 0.035
PINCH_OFF_THRESHOLD    = 0.05
PINCH_RELEASE_VELOCITY = 0.008
STRUM_THRESHOLD = 0.06
STRUM_COOLDOWN_FRAMES = 3
THUMB_SCORE_THRESHOLD  = 0.60

# ── Module-level hysteresis state ─────────────────────────────────────────────
previous_pinch_distances = {"Left": 1.0, "Right": 1.0}
active_pinches = {"Left": False, "Right": False}
cooldown_frames = 0
last_right_y = None


# ── Helpers ───────────────────────────────────────────────────────────────────

def is_pinch(label: str, hand_landmarks) -> bool:
    global previous_pinch_distances, active_pinches

    thumb_tip = hand_landmarks[4]
    index_tip = hand_landmarks[8]

    current_dist = (
        (thumb_tip.x - index_tip.x) ** 2 +
        (thumb_tip.y - index_tip.y) ** 2
    ) ** 0.5

    prev_dist = previous_pinch_distances.get(label, 1.0)
    currently_pinched = active_pinches.get(label, False)

    if currently_pinched and (current_dist - prev_dist > PINCH_RELEASE_VELOCITY):
        currently_pinched = False

    elif currently_pinched and current_dist > PINCH_OFF_THRESHOLD:
        currently_pinched = False

    elif not currently_pinched and current_dist < PINCH_ON_THRESHOLD:
        currently_pinched = True

    previous_pinch_distances[label] = current_dist
    active_pinches[label] = currently_pinched

    return currently_pinched


def gesture_score(gesture_categories, name: str) -> float:
    for cat in gesture_categories or []:
        if getattr(cat, "category_name", "") == name:
            return float(getattr(cat, "score", 0.0))
    return 0.0


def thumb_up(gesture_categories) -> bool:
    return gesture_score(gesture_categories, "Thumb_Up") >= THUMB_SCORE_THRESHOLD


def thumb_down(gesture_categories) -> bool:
    return gesture_score(gesture_categories, "Thumb_Down") >= THUMB_SCORE_THRESHOLD


# ── Main detection ────────────────────────────────────────────────────────────

def detect_guitar_hands(detection_result):
    global active_pinches, previous_pinch_distances, last_right_y, cooldown_frames 

    payload = {
        "instrument": "guitar",
        "leftHandVisible": False,
        "rightHandVisible": False,
        "leftHandX": 0.5,
        "leftHandY": 0.5,
        "rightHandX": 0.5,
        "rightHandY": 0.5,
        "leftPinch": False,
        "rightPinch": False,
        "rightThumbUp": False,
        "rightThumbDown": False,
        "guitarStrumDirection": "down",
    }

    active_zone_names = set()
    displayed_hand_positions = {}

    if not detection_result.handedness:
        last_right_y = None
        active_pinches["Left"] = False
        previous_pinch_distances["Left"] = 1.0
        active_pinches["Right"] = False
        previous_pinch_distances["Right"] = 1.0
        return payload, active_zone_names, displayed_hand_positions
    
    gestures_per_hand = getattr(detection_result, "gestures", None) or []

    processed_labels = set()
    for idx, (hand_landmarks, handedness) in enumerate(zip(detection_result.hand_landmarks, detection_result.handedness)):
        label = handedness[0].category_name
        if label in processed_labels:
            continue
        processed_labels.add(label)

        index_tip = hand_landmarks[8]
        
        display_x = 1.0 - index_tip.x
        display_y = index_tip.y
        displayed_hand_positions[label] = (display_x, display_y)

        gesture_categories = []
        if idx < len(gestures_per_hand) and gestures_per_hand[idx]:
            gesture_categories = gestures_per_hand[idx]

        if label == "Right":
            payload["rightHandVisible"] = True
            payload["rightHandX"] = display_x
            payload["rightHandY"] = display_y
            payload["rightPinch"] = is_pinch(label, hand_landmarks)
            payload["rightThumbUp"]     = thumb_up(gesture_categories)
            payload["rightThumbDown"]   = thumb_down(gesture_categories)

            current_y = display_y

            if last_right_y is not None:
                velocity = current_y - last_right_y

                if velocity > STRUM_THRESHOLD and cooldown_frames == 0:
                    payload["guitarStrumHit"] = True
                    payload["guitarStrumDirection"] = "down"
                    cooldown_frames = STRUM_COOLDOWN_FRAMES
                elif velocity < -STRUM_THRESHOLD and cooldown_frames == 0:
                    payload["guitarStrumHit"] = True
                    payload["guitarStrumDirection"] = "up"
                    cooldown_frames = STRUM_COOLDOWN_FRAMES

            last_right_y = current_y

        elif label == "Left":
            payload["leftHandVisible"] = True
            payload["leftHandX"] = display_x
            payload["leftHandY"] = display_y
            payload["leftPinch"] = is_pinch(label, hand_landmarks)

    if "Right" not in processed_labels:
        last_right_y = None
        active_pinches["Right"] = False
        previous_pinch_distances["Right"] = 1.0

    if "Left" not in processed_labels:
        active_pinches["Left"] = False
        previous_pinch_distances["Left"] = 1.0


    return payload, active_zone_names, displayed_hand_positions