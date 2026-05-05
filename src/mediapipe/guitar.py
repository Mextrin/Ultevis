from collections import deque

PINCH_THRESHOLD = 0.04
STRUM_THRESHOLD = 0.06
STRUM_COOLDOWN_FRAMES = 5
FIST_SCORE_THRESHOLD = 0.45
FIST_WINDOW_SIZE = 4
FIST_MIN_VOTES = 2

last_right_y = None
cooldown_frames = 0
closed_fist_history = deque(maxlen=FIST_WINDOW_SIZE)


def gesture_matches_closed_fist(gesture_categories):
    for category in gesture_categories or []:
        if getattr(category, "category_name", "") != "Closed_Fist":
            continue

        if float(getattr(category, "score", 0.0)) >= FIST_SCORE_THRESHOLD:
            return True

    return False


def update_closed_fist_state(gesture_categories):
    is_closed_fist = gesture_matches_closed_fist(gesture_categories)

    if not is_closed_fist:
        closed_fist_history.clear()
        return False

    closed_fist_history.append(True)
    return sum(closed_fist_history) >= FIST_MIN_VOTES


def reset_closed_fist_state():
    closed_fist_history.clear()


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
        "guitarStrumDirection": "down"
    }

    active_zone_names = set()
    displayed_hand_positions = {}

    if not detection_result.handedness:
        last_right_y = None
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

        # Mirrored X for display
        display_x = 1.0 - hand_landmarks[9].x # Middle finger base
        display_y = hand_landmarks[9].y
        displayed_hand_positions[label] = (display_x, display_y)

        gesture_categories = []
        if idx < len(gestures_per_hand) and gestures_per_hand[idx]:
            gesture_categories = gestures_per_hand[idx]

        if label == "Right":
            payload["rightHandVisible"] = True
            current_y = hand_landmarks[9].y
            closed_fist_active = update_closed_fist_state(gesture_categories)

            if closed_fist_active:
                last_right_y = None
                cooldown_frames = 0
                continue

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

            thumb_tip = hand_landmarks[4]
            index_tip = hand_landmarks[8]
            pinch_dist = ((thumb_tip.x - index_tip.x)**2 + (thumb_tip.y - index_tip.y)**2)**0.5
            payload["leftPinch"] = pinch_dist < PINCH_THRESHOLD

    if "Right" not in processed_labels:
        last_right_y = None
        reset_closed_fist_state()

    return payload, active_zone_names, displayed_hand_positions
