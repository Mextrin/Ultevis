import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from mediapipe.tasks.python.vision import RunningMode 
import socket
import json
from pathlib import Path

# Create hand detector with new API
MODEL_PATH = Path(__file__).with_name("hand_landmarker.task")
if not MODEL_PATH.exists():
    raise FileNotFoundError(f"MediaPipe hand landmarker model not found: {MODEL_PATH}")

hand_base_options = python.BaseOptions(model_asset_path=str(MODEL_PATH))
options = vision.HandLandmarkerOptions(base_options=hand_base_options,running_mode=RunningMode.VIDEO,num_hands=2)
recognizer = vision.HandLandmarker.create_from_options(options)

def detect_hands(detection_result): 
    # 1. Create a fresh, default payload for every frame.
    theremin_payload = {
        "rightHandVisible": False,
        "leftHandVisible": False,
        "rightHandX": 0.0,
        "rightHandY": 0.0,
        "leftHandX": 0.0,
        "leftHandY": 0.0,
    }

    if detection_result.handedness:
        for i, (hand_landmarks, handedness) in enumerate(zip(
            detection_result.hand_landmarks, detection_result.handedness
        )):
            label = handedness[0].category_name  # "Left" or "Right"
            thumb_tip = hand_landmarks[4]  # Thumb tip is landmark 4
            index_tip = hand_landmarks[8]  # Index finger tip is landmark 8

            # Calculate distance between thumb and index tips
            distance = ((thumb_tip.x - index_tip.x)**2 + (thumb_tip.y - index_tip.y)**2)**0.5
            PINCH_THRESHOLD = 0.05  # Adjust this value to tune sensitivity

            # Only record position if thumb and index are touching (pinching)
            if distance < PINCH_THRESHOLD:
                X_RIGHT_MIN = 0.0 # X right hand limit
                X_RIGHT_MAX = 0.67 

                if label == "Right":
                    # Constrain the value between X_MIN and X_MAX
                    constrained_x_right = max(X_RIGHT_MIN, min(index_tip.x, X_RIGHT_MAX))
                    theremin_payload["rightHandVisible"] = True
                    theremin_payload["rightHandX"] = constrained_x_right
                    theremin_payload["rightHandY"] = index_tip.y
                else:
                    theremin_payload["leftHandVisible"] = True
                    theremin_payload["leftHandX"] = index_tip.x
                    theremin_payload["leftHandY"] = index_tip.y

    # 2. Return the newly created payload, which is either updated or still in its default state.
    return theremin_payload