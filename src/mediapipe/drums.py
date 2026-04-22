import os
import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
import socket
import json
from dataclasses import dataclass
from pathlib import Path

UDP_IP = "127.0.0.1"
UDP_PORT = 5005
CAMERA_MODE = os.environ.get("ULTEVIS_CAMERA_MODE", "theremin").strip().lower()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def draw_frame (frame):
    display_frame = cv2.flip(frame, 1)
    frame_height, frame_width = display_frame.shape[:2]

    return display_frame, frame_height, frame_width

@dataclass(frozen=True)
class DrumZone:
    name: str
    note: int
    left: float
    top: float
    right: float
    bottom: float

    def contains(self, x: float, y: float) -> bool:
        return self.left <= x <= self.right and self.top <= y <= self.bottom

    def pixel_rect(self, width: int, height: int) -> tuple[int, int, int, int]:
        return (
            int(self.left * width),
            int(self.top * height),
            int(self.right * width),
            int(self.bottom * height),
        )


DRUM_ZONES = (
    DrumZone("Crash", 49, 0.02, 0.09, 0.25, 0.34),
    DrumZone("High-Tom", 48, 0.31, 0.14, 0.47, 0.36),
    DrumZone("Low-Tom", 45, 0.53, 0.14, 0.69, 0.36),
    DrumZone("Ride", 51, 0.75, 0.09, 0.98, 0.34),
    DrumZone("Closed Hi-Hat", 42, 0.02, 0.38, 0.23, 0.50),
    DrumZone("Open Hi-Hat", 46, 0.02, 0.52, 0.23, 0.64),
    DrumZone("Snare", 38, 0.29, 0.49, 0.48, 0.68),
    DrumZone("Floor Tom", 41, 0.77, 0.49, 0.98, 0.71),
    DrumZone("Kick", 36, 0.31, 0.75, 0.69, 0.96),
)

DEFAULT_DRUM_VELOCITY = 110



def compute_hand_center(hand_landmarks) -> tuple[float, float]:
    xs = [landmark.x for landmark in hand_landmarks]
    ys = [landmark.y for landmark in hand_landmarks]
    return sum(xs) / len(xs), sum(ys) / len(ys)


def find_zone(x: float, y: float) -> DrumZone | None:
    for zone in DRUM_ZONES:
        if zone.contains(x, y):
            return zone
    return None


def draw_drum_zones(frame, active_zone_names: set[str]) -> None:
    overlay = frame.copy()
    height, width = frame.shape[:2]

    for zone in DRUM_ZONES:
        left, top, right, bottom = zone.pixel_rect(width, height)
        is_active = zone.name in active_zone_names
        fill_color = (0, 120, 255) if is_active else (45, 45, 45)

        cv2.rectangle(overlay, (left, top), (right, bottom), fill_color, -1)

    cv2.addWeighted(overlay, 0.18, frame, 0.82, 0.0, frame)

    for zone in DRUM_ZONES:
        left, top, right, bottom = zone.pixel_rect(width, height)
        is_active = zone.name in active_zone_names
        border_color = (0, 180, 255) if is_active else (110, 110, 110)

        cv2.rectangle(frame, (left, top), (right, bottom), border_color, 2)
        cv2.putText(
            frame,
            zone.name,
            (left + 12, bottom - 12),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.55,
            border_color,
            2,
        )

# Initialize GestureRecognizer
MODEL_PATH = Path(__file__).with_name("gesture_recognizer.task")
if not MODEL_PATH.exists():
    raise FileNotFoundError(f"MediaPipe gesture recognizer model not found: {MODEL_PATH}")

base_options = python.BaseOptions(model_asset_path=str(MODEL_PATH))
options = vision.GestureRecognizerOptions(base_options=base_options, num_hands=2)
recognizer = vision.GestureRecognizer.create_from_options(options)

# --- THIS IS THE FIX ---
# This dictionary MUST be global to persist state between frames.
previous_zone_by_hand = {"Left": None, "Right": None}
# --- END OF FIX ---


def drum_detect (recognition_result): 
    # These are now local to the function, created fresh for each frame
    active_zone_names: set[str] = set()
    displayed_hand_positions: dict[str, tuple[float, float]] = {}
    detected_labels = set()

    drum_payload = {
        "rightHandVisible": False,
        "leftHandVisible": False,
        "rightHandX": 0.0,
        "rightHandY": 0.0,
        "leftHandX": 0.0,
        "leftHandY": 0.0,
        "rightGesture": "None",
        "leftGesture": "None",
        "leftDrumHit": False,
        "leftDrumType": 38,
        "leftDrumVelocity": DEFAULT_DRUM_VELOCITY,
        "rightDrumHit": False,
        "rightDrumType": 42,
        "rightDrumVelocity": DEFAULT_DRUM_VELOCITY,
    }
      
    if recognition_result.handedness:
        for i, (hand_landmarks, handedness, gestures) in enumerate(zip(
            recognition_result.hand_landmarks, 
            recognition_result.handedness,
            recognition_result.gestures
        )):
            label = handedness[0].category_name  # "Left" or "Right"
            detected_labels.add(label)

            # Get default gesture from the model
            gesture_name = gestures[0].category_name if gestures else "None"

            X_RIGHT_MIN = 0.0 
            X_RIGHT_MAX = 0.67 

            if label == "Right":
                drum_payload["rightHandVisible"] = True
                drum_payload["rightGesture"] = gesture_name
            else:
                drum_payload["leftHandVisible"] = True
                drum_payload["leftGesture"] = gesture_name

            if CAMERA_MODE == "drums":
                hand_center_x, hand_center_y = compute_hand_center(hand_landmarks)
                display_x = 1.0 - hand_center_x
                display_y = hand_center_y
                displayed_hand_positions[label] = (display_x, display_y)

                zone = find_zone(display_x, display_y)
                previous_zone_name = previous_zone_by_hand.get(label)
                current_zone_name = zone.name if zone is not None else None

                if zone is not None:
                    active_zone_names.add(zone.name)
                    if current_zone_name != previous_zone_name:
                        prefix = "right" if label == "Right" else "left"
                        drum_payload[f"{prefix}DrumHit"] = True
                        drum_payload[f"{prefix}DrumType"] = zone.note
                        drum_payload[f"{prefix}DrumVelocity"] = DEFAULT_DRUM_VELOCITY

                previous_zone_by_hand[label] = current_zone_name

    for label in previous_zone_by_hand:
        if label not in detected_labels:
            previous_zone_by_hand[label] = None
    
    return drum_payload, active_zone_names, displayed_hand_positions



def get_drum_hit_coordinates(display_frame, frame_height, frame_width, active_zone_names, displayed_hand_positions):
    draw_drum_zones(display_frame, active_zone_names)
    for label, position in displayed_hand_positions.items():
        center_x = int(position[0] * frame_width)
        center_y = int(position[1] * frame_height)
        cv2.circle(display_frame, center=(center_x, center_y), radius=10, color=(0, 180, 255), thickness=-1)
        cv2.putText(
            display_frame,
            label,
            (center_x + 10, center_y - 10),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.5,
            (0, 180, 255),
            2,
        )


