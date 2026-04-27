import mediapipe as mp

PINCH_THRESHOLD = 0.05

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
    }

    if not detection_result.handedness:
        return keyboard_payload

    processed_labels = set()
    for hand_landmarks, handedness in zip(detection_result.hand_landmarks, detection_result.handedness):
        label = handedness[0].category_name
        if label in processed_labels:
            continue

        processed_labels.add(label)
        thumb_tip = hand_landmarks[4]
        index_tip = hand_landmarks[8]
        
        # Calculate distance between thumb and index finger
        pinch_distance = ((thumb_tip.x - index_tip.x) ** 2 + (thumb_tip.y - index_tip.y) ** 2) ** 0.5
        is_pinching = pinch_distance < PINCH_THRESHOLD

        # The camera feed is mirrored for display, so QML hit-tests use display-space X.
        display_x = max(0.0, min(1.0, 1.0 - index_tip.x))
        display_y = max(0.0, min(1.0, index_tip.y))

        if label == "Right":
            keyboard_payload["rightHandVisible"] = True
            keyboard_payload["rightHandX"] = display_x
            keyboard_payload["rightHandY"] = display_y
            keyboard_payload["rightPinch"] = is_pinching
        else:
            keyboard_payload["leftHandVisible"] = True
            keyboard_payload["leftHandX"] = display_x
            keyboard_payload["leftHandY"] = display_y
            keyboard_payload["leftPinch"] = is_pinching

    return keyboard_payload