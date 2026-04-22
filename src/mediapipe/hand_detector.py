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

cap = cv2.VideoCapture(0)
if not cap.isOpened():
    raise RuntimeError("Unable to open webcam at index 0. Check camera permissions.")

previous_zone_by_hand = {"Left": None, "Right": None}

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break

    # Convert to MediaPipe format
    image = mp.Image(image_format=mp.ImageFormat.SRGB, data=cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))
    
    recognition_result = recognizer.recognize(image)

    payload = {
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

    active_zone_names: set[str] = set()
    displayed_hand_positions: dict[str, tuple[float, float]] = {}
    detected_labels = set()

    if recognition_result.handedness:
        for i, (hand_landmarks, handedness, gestures) in enumerate(zip(
            recognition_result.hand_landmarks, 
            recognition_result.handedness,
            recognition_result.gestures
        )):
            label = handedness[0].category_name  # "Left" or "Right"
            detected_labels.add(label)
            thumb_tip = hand_landmarks[4]  # Thumb tip
            index_tip = hand_landmarks[8]  # Index finger tip
            
            # Get default gesture from the model
            gesture_name = gestures[0].category_name if gestures else "None"
            
            # Calculate distance for custom Pinch gesture
            distance = ((thumb_tip.x - index_tip.x)**2 + (thumb_tip.y - index_tip.y)**2)**0.5
            PINCH_THRESHOLD = 0.05
            
            # Override model gesture if pinch is detected
            if distance < PINCH_THRESHOLD:
                gesture_name = "Pinch"

            X_RIGHT_MIN = 0.0 
            X_RIGHT_MAX = 0.67 

            if label == "Right":
                constrained_x_right = max(X_RIGHT_MIN, min(index_tip.x, X_RIGHT_MAX))
                payload["rightHandVisible"] = True
                payload["rightHandX"] = constrained_x_right
                payload["rightHandY"] = index_tip.y
                payload["rightGesture"] = gesture_name
            else:
                payload["leftHandVisible"] = True
                payload["leftHandX"] = index_tip.x
                payload["leftHandY"] = index_tip.y
                payload["leftGesture"] = gesture_name

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
                        payload[f"{prefix}DrumHit"] = True
                        payload[f"{prefix}DrumType"] = zone.note
                        payload[f"{prefix}DrumVelocity"] = DEFAULT_DRUM_VELOCITY

                previous_zone_by_hand[label] = current_zone_name

    for label in previous_zone_by_hand:
        if label not in detected_labels:
            previous_zone_by_hand[label] = None

    sock.sendto(json.dumps(payload).encode(), (UDP_IP, UDP_PORT))

    display_frame = cv2.flip(frame, 1)
    frame_height, frame_width = display_frame.shape[:2]

    if CAMERA_MODE == "drums":
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
        cv2.putText(
            display_frame,
            "Drum mode: move a hand into a zone to trigger a hit",
            (10, frame_height - 18),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.65,
            (0, 180, 255),
            2,
        )
        window_title = "MediaPipe Drum Zone Tracker"
    else:
        if recognition_result.handedness:
            for hand_landmarks in recognition_result.hand_landmarks:
                index_tip = hand_landmarks[8]
                center_x = int((1 - index_tip.x) * frame_width)
                center_y = int(index_tip.y * frame_height)
                cv2.circle(display_frame, center=(center_x, center_y), radius=10, color=(0, 255, 0), thickness=-1)

        cv2.putText(display_frame,
            f"L: {payload['leftHandVisible']}, LeftX: {payload['leftHandX']:.2f}, LeftY: {payload['leftHandY']:.2f}, Gest: {payload['leftGesture']}",
            (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2
        )
        cv2.putText(display_frame,
            f"R: {payload['rightHandVisible']}, RightX: {payload['rightHandX']:.2f}, RightY: {payload['rightHandY']:.2f}, Gest: {payload['rightGesture']}",
            (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2
        )
        window_title = "MediaPipe Gesture Tracker"

    cv2.imshow(window_title, display_frame)

    if cv2.waitKey(1) == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
