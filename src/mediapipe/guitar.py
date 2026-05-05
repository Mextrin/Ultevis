from collections import deque

import mediapipe as mp

PINCH_THRESHOLD = 0.04
STRUM_THRESHOLD = 0.06
FIST_SCORE_THRESHOLD = 0.45
FIST_GESTURE_NAMES = {"Closed_Fist", "Closed Fist"}
FIST_WINDOW_SIZE = 4
FIST_MIN_VOTES = 2
DOWNSTRUM = "down"
UPSTRUM = "up"

last_right_y = None
cooldown_frames = 0
_fist_history: dict[str, deque[bool]] = {}


def gesture_matches_closed_fist(gesture_categories) -> bool:
    if not gesture_categories:
        return False

    strongest = max(gesture_categories, key=lambda category: getattr(category, "score", 0.0))
    return (
        getattr(strongest, "category_name", "") in FIST_GESTURE_NAMES
        and getattr(strongest, "score", 0.0) >= FIST_SCORE_THRESHOLD
    )


def update_closed_fist_state(label, gesture_categories) -> bool:
    history = _fist_history.setdefault(label, deque(maxlen=FIST_WINDOW_SIZE))
    history.append(gesture_matches_closed_fist(gesture_categories))

    if len(history) < FIST_MIN_VOTES:
        return False

    return sum(history) >= FIST_MIN_VOTES


def reset_closed_fist_state(label) -> None:
    _fist_history.setdefault(label, deque(maxlen=FIST_WINDOW_SIZE)).clear()

def detect_guitar_hands(detection_result):
    global last_right_y, cooldown_frames

    payload = {
        "instrument": "guitar",
        "leftHandVisible": False,
        "rightHandVisible": False,
        "leftHandX": 0.5,
        "leftHandY": 0.5,
        "leftPinch": False,
        "guitarStrumHit": False,
        "guitarStrumDirection": DOWNSTRUM,
    }

    active_zone_names = set()
    displayed_hand_positions = {}

    if not detection_result.handedness:
        last_right_y = None
        reset_closed_fist_state("Right")
        reset_closed_fist_state("Left")
        return payload, active_zone_names, displayed_hand_positions

    processed_labels = set()
    gestures_per_hand = getattr(detection_result, "gestures", None) or []

    # Decrease cooldown every frame
    if cooldown_frames > 0:
        cooldown_frames -= 1

    for idx, (hand_landmarks, handedness) in enumerate(zip(detection_result.hand_landmarks, detection_result.handedness)):
        label = handedness[0].category_name
        if label in processed_labels:
            continue
        processed_labels.add(label)

        gesture_categories = []
        if idx < len(gestures_per_hand) and gestures_per_hand[idx]:
            gesture_categories = gestures_per_hand[idx]

        # Mirrored X for display
        display_x = 1.0 - hand_landmarks[9].x # Middle finger base
        display_y = hand_landmarks[9].y
        displayed_hand_positions[label] = (display_x, display_y)

        if label == "Right":
            payload["rightHandVisible"] = True
            current_y = hand_landmarks[9].y

            if update_closed_fist_state("Right", gesture_categories):
                last_right_y = None
                cooldown_frames = 0
                continue

            if last_right_y is not None and cooldown_frames == 0:
                velocity = current_y - last_right_y

                if velocity > STRUM_THRESHOLD:
                    payload["guitarStrumHit"] = True
                    payload["guitarStrumDirection"] = DOWNSTRUM
                    cooldown_frames = 5 # Wait 5 frames before allowing another strum
                elif velocity < -STRUM_THRESHOLD:
                    payload["guitarStrumHit"] = True
                    payload["guitarStrumDirection"] = UPSTRUM
                    cooldown_frames = 5

            last_right_y = current_y

        elif label == "Left":
            payload["leftHandVisible"] = True
            payload["leftHandX"] = display_x
            payload["leftHandY"] = display_y
            update_closed_fist_state("Left", gesture_categories)

            thumb_tip = hand_landmarks[4]
            index_tip = hand_landmarks[8]
            pinch_dist = ((thumb_tip.x - index_tip.x)**2 + (thumb_tip.y - index_tip.y)**2)**0.5
            payload["leftPinch"] = pinch_dist < PINCH_THRESHOLD

    if "Right" not in processed_labels:
        last_right_y = None
        reset_closed_fist_state("Right")
    if "Left" not in processed_labels:
        reset_closed_fist_state("Left")

    return payload, active_zone_names, displayed_hand_positions
