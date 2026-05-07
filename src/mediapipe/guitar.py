"""
guitar.py — Hand tracking for Guitar mode.

Python handles:
  • Hand visibility + positions
  • Pinch state (hysteresis, same as drums.py)
  • Thumb-up / thumb-down gestures (same thresholds as keyboard.py)
  • Right-hand strum detection (frame-level Y-displacement with direction tracking)
"""

PINCH_ON_THRESHOLD     = 0.065
PINCH_OFF_THRESHOLD    = 0.12
PINCH_RELEASE_VELOCITY = 0.04
THUMB_SCORE_THRESHOLD  = 0.60

STRUM_THRESHOLD     = 0.07
STRUM_COOLDOWN      = 5

previous_pinch_distances = {"Left": 1.0, "Right": 1.0}
active_pinches = {"Left": False, "Right": False}

strum_anchor_y       = -1.0   
strum_direction      =  0     
strum_prev_y         = -1.0
strum_cooldown       =  0     
strum_needs_reversal = False 


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


def detect_strum(right_y: float, pinched: bool) -> tuple[bool, bool]:
    """Return (strum_fired, is_down_strum). Mutates module-level strum state."""
    global strum_anchor_y, strum_direction, strum_prev_y, strum_cooldown, strum_needs_reversal

    if strum_cooldown > 0:
        strum_cooldown -= 1

    if not pinched:
        strum_anchor_y       = -1.0
        strum_direction      =  0
        strum_prev_y         = -1.0
        strum_needs_reversal = False
        return False, False

    if strum_anchor_y == -1.0 or strum_prev_y == -1.0:
        strum_anchor_y  = right_y
        strum_prev_y    = right_y
        strum_direction = 0
        return False, False

    if strum_direction == 1 and right_y < strum_prev_y:
        strum_anchor_y       = strum_prev_y
        strum_direction      = -1
        strum_needs_reversal = False
    elif strum_direction == -1 and right_y > strum_prev_y:
        strum_anchor_y       = strum_prev_y
        strum_direction      = 1
        strum_needs_reversal = False
    elif strum_direction == 0:
        strum_direction = 1 if right_y > strum_prev_y else -1

    strum_prev_y = right_y

    if (abs(right_y - strum_anchor_y) > STRUM_THRESHOLD
            and strum_cooldown == 0
            and not strum_needs_reversal):
        is_down               = right_y > strum_anchor_y
        strum_anchor_y       = right_y
        strum_cooldown       = STRUM_COOLDOWN
        strum_needs_reversal = True
        return True, is_down

    return False, False


# ── Main detection ────────────────────────────────────────────────────────────

def detect_guitar_hands(detection_result):
    global active_pinches, previous_pinch_distances

    # Proactively reset state for hands that are not visible in this frame.
    # This prevents stale state from carrying over from previous frames.
    visible_labels = {h[0].category_name for h in detection_result.handedness}
    for hand in ["Left", "Right"]:
        if hand not in visible_labels:
            active_pinches[hand] = False
            previous_pinch_distances[hand] = 1.0

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
        "strumDetected": False,
        "strumIsDown": False,
    }

    active_zone_names = set()
    displayed_hand_positions = {}

    if not detection_result.handedness:
        return payload, active_zone_names, displayed_hand_positions
    
    gestures_per_hand = getattr(detection_result, "gestures", None) or []

    processed_labels = set()
    for idx, (hand_landmarks, handedness) in enumerate(zip(detection_result.hand_landmarks, detection_result.handedness)):
        label = handedness[0].category_name
        if label in processed_labels:
            continue
        processed_labels.add(label)

        thumb_tip = hand_landmarks[4]
        index_tip = hand_landmarks[8]
        
        display_x = 1.0 - ((thumb_tip.x + index_tip.x) / 2.0)
        display_y = (thumb_tip.y + index_tip.y) / 2.0

        displayed_hand_positions[label] = (display_x, display_y)

        gesture_categories = []
        if idx < len(gestures_per_hand) and gestures_per_hand[idx]:
            gesture_categories = gestures_per_hand[idx]

        if label == "Right":
            payload["rightHandVisible"] = True
            payload["rightHandX"] = display_x
            payload["rightHandY"] = display_y
            r_pinch = is_pinch(label, hand_landmarks)
            payload["rightPinch"]     = r_pinch
            payload["rightThumbUp"]   = thumb_up(gesture_categories)
            payload["rightThumbDown"] = thumb_down(gesture_categories)

            strum_fired, strum_down = detect_strum(display_y, r_pinch)
            if strum_fired:
                payload["strumDetected"] = True
                payload["strumIsDown"]   = strum_down

        elif label == "Left":
            payload["leftHandVisible"] = True
            payload["leftHandX"] = display_x
            payload["leftHandY"] = display_y
            payload["leftPinch"] = is_pinch(label, hand_landmarks)

    return payload, active_zone_names, displayed_hand_positions