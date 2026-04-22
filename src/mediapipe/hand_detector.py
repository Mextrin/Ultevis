import os
import sys
import struct
from time import monotonic
import cv2
import numpy as np
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
HEADLESS = os.environ.get("ULTEVIS_HEADLESS", "0").strip() == "1"

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
    DrumZone("Crash",         49, 0.02, 0.09, 0.25, 0.34),
    DrumZone("High-Tom",      48, 0.31, 0.14, 0.47, 0.36),
    DrumZone("Low-Tom",       45, 0.53, 0.14, 0.69, 0.36),
    DrumZone("Ride",          51, 0.75, 0.09, 0.98, 0.34),
    DrumZone("Closed Hi-Hat", 42, 0.02, 0.38, 0.23, 0.50),
    DrumZone("Open Hi-Hat",   46, 0.02, 0.52, 0.23, 0.64),
    DrumZone("Snare",         38, 0.29, 0.49, 0.48, 0.68),
    DrumZone("Floor Tom",     41, 0.77, 0.49, 0.98, 0.71),
    DrumZone("Kick",          36, 0.31, 0.75, 0.69, 0.96),
)

DEFAULT_DRUM_VELOCITY = 110

# Minimum seconds between hits on the same zone — prevents boundary jitter
# from firing the same drum repeatedly.
DRUM_COOLDOWN_S = 0.30
_zone_last_hit: dict[str, float] = {}


def zone_is_ready(name: str) -> bool:
    now = monotonic()
    if now - _zone_last_hit.get(name, 0.0) >= DRUM_COOLDOWN_S:
        _zone_last_hit[name] = now
        return True
    return False


def compute_hand_center(hand_landmarks) -> tuple[float, float]:
    xs = [lm.x for lm in hand_landmarks]
    ys = [lm.y for lm in hand_landmarks]
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
        fill_color = (0, 120, 255) if zone.name in active_zone_names else (45, 45, 45)
        cv2.rectangle(overlay, (left, top), (right, bottom), fill_color, -1)
    cv2.addWeighted(overlay, 0.18, frame, 0.82, 0.0, frame)
    for zone in DRUM_ZONES:
        left, top, right, bottom = zone.pixel_rect(width, height)
        color = (0, 180, 255) if zone.name in active_zone_names else (110, 110, 110)
        cv2.rectangle(frame, (left, top), (right, bottom), color, 2)
        cv2.putText(frame, zone.name, (left + 12, bottom - 12),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.55, color, 2)


# ---------------------------------------------------------------------------
# Headless frame source — C++ pipes RGB888 frames over stdin.
# Protocol: 8-byte header (width, height as little-endian uint32) then
# width * height * 3 bytes of packed RGB data.
# ---------------------------------------------------------------------------
def read_stdin_frame() -> tuple[np.ndarray | None, np.ndarray | None]:
    header = sys.stdin.buffer.read(8)
    if len(header) < 8:
        return None, None
    w, h = struct.unpack("<II", header)
    n = w * h * 3
    data = bytearray()
    while len(data) < n:
        chunk = sys.stdin.buffer.read(n - len(data))
        if not chunk:
            return None, None
        data.extend(chunk)
    rgb = np.frombuffer(data, dtype=np.uint8).reshape((h, w, 3))
    bgr = cv2.cvtColor(rgb, cv2.COLOR_RGB2BGR)
    return bgr, rgb


# ---------------------------------------------------------------------------
# MediaPipe gesture recognizer
# ---------------------------------------------------------------------------
MODEL_PATH = Path(__file__).with_name("gesture_recognizer.task")
if not MODEL_PATH.exists():
    raise FileNotFoundError(f"Model not found: {MODEL_PATH}")

recognizer = vision.GestureRecognizer.create_from_options(
    vision.GestureRecognizerOptions(
        base_options=python.BaseOptions(model_asset_path=str(MODEL_PATH)),
        num_hands=2,
    )
)

# ---------------------------------------------------------------------------
# Camera setup (non-headless only)
# ---------------------------------------------------------------------------
if not HEADLESS:
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        raise RuntimeError("Unable to open webcam at index 0. Check camera permissions.")
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

previous_zone_by_hand: dict[str, str | None] = {"Left": None, "Right": None}

# ---------------------------------------------------------------------------
# Main loop
# ---------------------------------------------------------------------------

# In headless mode request the first frame from C++ immediately.
if HEADLESS:
    sys.stdout.buffer.write(b'R')
    sys.stdout.buffer.flush()

while True:
    if HEADLESS:
        frame, frame_rgb = read_stdin_frame()
        if frame is None:
            break
    else:
        ret, frame = cap.read()
        if not ret:
            break
        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

    image = mp.Image(image_format=mp.ImageFormat.SRGB, data=frame_rgb)
    result = recognizer.recognize(image)

    payload = {
        "rightHandVisible": False, "leftHandVisible": False,
        "rightHandX": 0.0, "rightHandY": 0.0,
        "leftHandX":  0.0, "leftHandY":  0.0,
        "rightGesture": "None", "leftGesture": "None",
        "leftDrumHit":  False, "leftDrumType":  38, "leftDrumVelocity":  DEFAULT_DRUM_VELOCITY,
        "rightDrumHit": False, "rightDrumType": 42, "rightDrumVelocity": DEFAULT_DRUM_VELOCITY,
    }

    active_zone_names: set[str] = set()
    displayed_hand_positions: dict[str, tuple[float, float]] = {}
    detected_labels: set[str] = set()

    if result.handedness:
        for hand_landmarks, handedness, gestures in zip(
            result.hand_landmarks, result.handedness, result.gestures
        ):
            label = handedness[0].category_name
            detected_labels.add(label)
            thumb_tip = hand_landmarks[4]
            index_tip = hand_landmarks[8]

            gesture_name = gestures[0].category_name if gestures else "None"
            dist = ((thumb_tip.x - index_tip.x) ** 2 + (thumb_tip.y - index_tip.y) ** 2) ** 0.5
            if dist < 0.05:
                gesture_name = "Pinch"

            if label == "Right":
                X_MAX = 0.67  # guide line boundary in MediaPipe x space
                payload["rightHandVisible"] = True
                if index_tip.x <= X_MAX:
                    payload["rightHandX"] = index_tip.x / X_MAX  # normalise to 0–1
                else:
                    payload["rightHandX"] = 1.0  # pin to highest pitch when past the line
                payload["rightHandY"]       = index_tip.y
                payload["rightGesture"]     = gesture_name
            else:
                payload["leftHandVisible"] = True
                payload["leftHandX"]       = index_tip.x
                payload["leftHandY"]       = index_tip.y
                payload["leftGesture"]     = gesture_name

            if CAMERA_MODE == "drums":
                cx, cy = compute_hand_center(hand_landmarks)
                dx, dy = 1.0 - cx, cy
                displayed_hand_positions[label] = (dx, dy)
                zone = find_zone(dx, dy)
                prev_name = previous_zone_by_hand.get(label)
                curr_name = zone.name if zone else None
                if zone:
                    active_zone_names.add(zone.name)
                    if curr_name != prev_name and zone_is_ready(zone.name):
                        prefix = "right" if label == "Right" else "left"
                        payload[f"{prefix}DrumHit"]      = True
                        payload[f"{prefix}DrumType"]     = zone.note
                        payload[f"{prefix}DrumVelocity"] = DEFAULT_DRUM_VELOCITY
                previous_zone_by_hand[label] = curr_name

    for label in previous_zone_by_hand:
        if label not in detected_labels:
            previous_zone_by_hand[label] = None

    sock.sendto(json.dumps(payload).encode(), (UDP_IP, UDP_PORT))

    # Signal C++ that we're ready for the next frame (pull protocol).
    if HEADLESS:
        sys.stdout.buffer.write(b'R')
        sys.stdout.buffer.flush()
    else:
        display = cv2.flip(frame, 1)
        fh, fw = display.shape[:2]
        if CAMERA_MODE == "drums":
            draw_drum_zones(display, active_zone_names)
            for lbl, (px, py) in displayed_hand_positions.items():
                cx2, cy2 = int(px * fw), int(py * fh)
                cv2.circle(display, (cx2, cy2), 10, (0, 180, 255), -1)
                cv2.putText(display, lbl, (cx2 + 10, cy2 - 10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 180, 255), 2)
            cv2.putText(display, "Drum mode: move hand into zone to hit",
                        (10, fh - 18), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0, 180, 255), 2)
            title = "MediaPipe Drum Zone Tracker"
        else:
            for hl in result.hand_landmarks:
                tip = hl[8]
                cv2.circle(display, (int((1 - tip.x) * fw), int(tip.y * fh)),
                           10, (0, 255, 0), -1)
            cv2.putText(display,
                f"L:{payload['leftHandVisible']} x={payload['leftHandX']:.2f} "
                f"y={payload['leftHandY']:.2f} g={payload['leftGesture']}",
                (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
            cv2.putText(display,
                f"R:{payload['rightHandVisible']} x={payload['rightHandX']:.2f} "
                f"y={payload['rightHandY']:.2f} g={payload['rightGesture']}",
                (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
            title = "MediaPipe Gesture Tracker"
        cv2.imshow(title, display)
        if cv2.waitKey(1) == ord('q'):
            break

if not HEADLESS:
    cap.release()
    cv2.destroyAllWindows()
