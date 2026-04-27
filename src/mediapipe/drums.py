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
    DrumZone("Crash", 54, 0.02, 0.09, 0.25, 0.34),
    DrumZone("High-Tom", 48, 0.31, 0.14, 0.47, 0.36),
    DrumZone("Low-Tom", 45, 0.53, 0.14, 0.69, 0.36),
    DrumZone("Ride", 60, 0.75, 0.09, 0.98, 0.34),
    DrumZone("Closed Hi-Hat", 42, 0.02, 0.38, 0.23, 0.50),
    DrumZone("Open Hi-Hat", 46, 0.02, 0.52, 0.23, 0.64),
    DrumZone("Snare", 38, 0.29, 0.49, 0.48, 0.68),
    DrumZone("Floor Tom", 43, 0.77, 0.49, 0.98, 0.71),
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

# --- THE FIX ---
# Instead of tracking hands, we track which ZONES have been hit.
# This completely eliminates Left/Right handedness flickering!
active_triggered_zones = set()
# ---------------


def drum_detect(recognition_result): 
    global active_triggered_zones

    active_zone_names: set[str] = set()
    displayed_hand_positions: dict[str, tuple[float, float]] = {}
    detected_labels = set()
    processed_labels = set()

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

    # Map out which zones currently have hands inside them
    hands_in_zones = {}
      
    if recognition_result.handedness:
        for i, (hand_landmarks, handedness, gestures) in enumerate(zip(
            recognition_result.hand_landmarks, 
            recognition_result.handedness,
            recognition_result.gestures
        )):
            label = handedness[0].category_name  
            
            # Prevent MediaPipe's "Two Right Hands" ghost glitch
            if label in processed_labels:
                continue
            
            processed_labels.add(label)
            detected_labels.add(label)

            gesture_name = gestures[0].category_name if gestures else "None"

            if label == "Right":
                drum_payload["rightHandVisible"] = True
                drum_payload["rightGesture"] = gesture_name
            else:
                drum_payload["leftHandVisible"] = True
                drum_payload["leftGesture"] = gesture_name

            hand_center_x, hand_center_y = compute_hand_center(hand_landmarks)
            display_x = 1.0 - hand_center_x
            display_y = hand_center_y
            displayed_hand_positions[label] = (display_x, display_y)

            zone = find_zone(display_x, display_y)
            if zone is not None:
                hands_in_zones[zone.name] = (gesture_name, label, zone)

    # 1. RESET MEMORY: 
    # If a hand leaves a zone, or explicitly makes a Closed_Fist, reset the zone so it can be hit again.
    # Notice we DO NOT reset if the gesture is "None" (which ignores the 1-frame camera flickers!)
    zones_to_remove = set()
    for zone_name in active_triggered_zones:
        if zone_name not in hands_in_zones:
            zones_to_remove.add(zone_name)
        else:
            gesture_name, _, _ = hands_in_zones[zone_name]
            if gesture_name == "Closed_Fist":
                zones_to_remove.add(zone_name)

    active_triggered_zones.difference_update(zones_to_remove)

    # 2. TRIGGER HITS:
    # If a hand with an Open_Palm is in a zone that hasn't been triggered yet, hit it!
    for zone_name, (gesture_name, label, zone) in hands_in_zones.items():
        if gesture_name == "Open_Palm":
            active_zone_names.add(zone_name)
            
            if zone_name not in active_triggered_zones:
                prefix = "right" if label == "Right" else "left"
                drum_payload[f"{prefix}DrumHit"] = True
                drum_payload[f"{prefix}DrumType"] = zone.note
                drum_payload[f"{prefix}DrumVelocity"] = DEFAULT_DRUM_VELOCITY
                
                # Mark zone as active so it doesn't trigger again until reset
                active_triggered_zones.add(zone_name)
                
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