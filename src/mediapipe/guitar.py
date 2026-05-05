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

THUMB_SCORE_THRESHOLD  = 0.60

# ── Module-level hysteresis state ─────────────────────────────────────────────
_prev_pinch_dist = {"Left": 1.0, "Right": 1.0}
_pinch_active    = {"Left": False, "Right": False}


# ── Helpers ───────────────────────────────────────────────────────────────────

def _is_pinch(label: str, hand_landmarks) -> bool:
    """Hysteresis-based pinch — identical to drums.py."""
    thumb_tip = hand_landmarks[4]
    index_tip = hand_landmarks[8]
    dist = ((thumb_tip.x - index_tip.x) ** 2 +
            (thumb_tip.y - index_tip.y) ** 2) ** 0.5

    prev_dist = _prev_pinch_dist.get(label, 1.0)
    active    = _pinch_active.get(label, False)

    if active and (dist - prev_dist > PINCH_RELEASE_VELOCITY):
        active = False
    elif active and dist > PINCH_OFF_THRESHOLD:
        active = False
    elif not active and dist < PINCH_ON_THRESHOLD:
        active = True

    _prev_pinch_dist[label] = dist
    _pinch_active[label]    = active
    return active


def _gesture_score(gesture_categories, name: str) -> float:
    for cat in gesture_categories or []:
        if getattr(cat, "category_name", "") == name:
            return float(getattr(cat, "score", 0.0))
    return 0.0


def _thumb_up(gesture_categories) -> bool:
    return _gesture_score(gesture_categories, "Thumb_Up") >= THUMB_SCORE_THRESHOLD


def _thumb_down(gesture_categories) -> bool:
    return _gesture_score(gesture_categories, "Thumb_Down") >= THUMB_SCORE_THRESHOLD


# ── Main detection ────────────────────────────────────────────────────────────

def detect_guitar_hands(detection_result):
    payload = {
        "instrument":      "guitar",
        "leftHandVisible":  False,
        "rightHandVisible": False,
        "leftHandX":        0.5,
        "leftHandY":        0.5,
        "rightHandX":       0.5,
        "rightHandY":       0.5,
        "leftPinch":        False,
        "rightPinch":       False,
        "leftThumbUp":      False,
        "leftThumbDown":    False,
        "rightThumbUp":     False,
        "rightThumbDown":   False,
    }

    active_zone_names        = set()
    displayed_hand_positions = {}

    if not detection_result.handedness:
        _pinch_active["Left"]  = False
        _pinch_active["Right"] = False
        return payload, active_zone_names, displayed_hand_positions

    gestures_per_hand = getattr(detection_result, "gestures", None) or []
    processed_labels  = set()

    for idx, (hand_landmarks, handedness) in enumerate(
        zip(detection_result.hand_landmarks, detection_result.handedness)
    ):
        label = handedness[0].category_name
        if label in processed_labels:
            continue
        processed_labels.add(label)

        # Mirror X so coordinates match the flipped camera display
        display_x = 1.0 - hand_landmarks[9].x
        display_y = hand_landmarks[9].y
        displayed_hand_positions[label] = (display_x, display_y)

        gesture_categories = []
        if idx < len(gestures_per_hand) and gestures_per_hand[idx]:
            gesture_categories = gestures_per_hand[idx]

        is_pinching = _is_pinch(label, hand_landmarks)

        if label == "Left":
            payload["leftHandVisible"] = True
            payload["leftHandX"]       = display_x
            payload["leftHandY"]       = display_y
            payload["leftPinch"]       = is_pinching
            payload["leftThumbUp"]     = _thumb_up(gesture_categories)
            payload["leftThumbDown"]   = _thumb_down(gesture_categories)

        elif label == "Right":
            payload["rightHandVisible"] = True
            payload["rightHandX"]       = display_x
            payload["rightHandY"]       = display_y
            payload["rightPinch"]       = is_pinching
            payload["rightThumbUp"]     = _thumb_up(gesture_categories)
            payload["rightThumbDown"]   = _thumb_down(gesture_categories)

    # Clear pinch state for hands that left the frame
    if "Left"  not in processed_labels:
        _pinch_active["Left"]  = False
    if "Right" not in processed_labels:
        _pinch_active["Right"] = False

    return payload, active_zone_names, displayed_hand_positions
