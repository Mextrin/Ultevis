import mediapipe as mp

PINCH_THRESHOLD = 0.04
STRUM_THRESHOLD = 0.06 

last_right_y = None
cooldown_frames = 0 

def detect_guitar_hands(detection_result):
    global last_right_y, cooldown_frames

    payload = {
        "instrument": "guitar",
        "leftHandVisible": False,
        "rightHandVisible": False,
        "leftHandX": 0.5,
        "leftHandY": 0.5,
        "leftPinch": False,
        "guitarStrumHit": False
    }

    active_zone_names = set()
    displayed_hand_positions = {}

    if not detection_result.handedness:
        last_right_y = None
        return payload, active_zone_names, displayed_hand_positions

    processed_labels = set()

    # Decrease cooldown every frame
    if cooldown_frames > 0:
        cooldown_frames -= 1

    for hand_landmarks, handedness in zip(detection_result.hand_landmarks, detection_result.handedness):
        label = handedness[0].category_name
        if label in processed_labels:
            continue
        processed_labels.add(label)

        # Mirrored X for display
        display_x = 1.0 - hand_landmarks[9].x # Middle finger base
        display_y = hand_landmarks[9].y
        displayed_hand_positions[label] = (display_x, display_y)

        if label == "Right":
            payload["rightHandVisible"] = True
            current_y = hand_landmarks[9].y

            if last_right_y is not None:
                velocity = current_y - last_right_y
                
                # If moving down rapidly and not on cooldown
                if velocity > STRUM_THRESHOLD and cooldown_frames == 0:
                    payload["guitarStrumHit"] = True
                    cooldown_frames = 5 # Wait 5 frames before allowing another strum
            
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

    return payload, active_zone_names, displayed_hand_positions