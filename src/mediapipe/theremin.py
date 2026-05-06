import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from mediapipe.tasks.python.vision import RunningMode
from collections import deque
from pathlib import Path


# Create hand detector with new API
MODEL_PATH = Path(__file__).with_name("hand_landmarker.task")
if not MODEL_PATH.exists():
    raise FileNotFoundError(f"MediaPipe hand landmarker model not found: {MODEL_PATH}")

hand_base_options = python.BaseOptions(model_asset_path=str(MODEL_PATH))
options = vision.HandLandmarkerOptions(base_options=hand_base_options,running_mode=RunningMode.VIDEO,num_hands=2)
recognizer = vision.HandLandmarker.create_from_options(options)

_TRAIL_LENGTH = 50
_TRAIL_MAX_THICKNESS = 20
# (B, G, R)
_RIGHT_TRAIL_COLOR = (20, 100, 255)   # orange
_LEFT_TRAIL_COLOR  = (20, 100, 255)   # orange

_left_trail:  deque[tuple[int, int]] = deque(maxlen=_TRAIL_LENGTH)
_right_trail: deque[tuple[int, int]] = deque(maxlen=_TRAIL_LENGTH)

# If a hand jumps further than this in one frame it's a label-swap glitch, not real movement
_MAX_TRAIL_JUMP_PX = 80


def _append_trail(trail: deque, pt: tuple[int, int]) -> None:
    if trail:
        dx = pt[0] - trail[-1][0]
        dy = pt[1] - trail[-1][1]
        if dx * dx + dy * dy > _MAX_TRAIL_JUMP_PX ** 2:
            trail.clear()
    trail.append(pt)


def detect_hands(detection_result): 
    # 1. Create a fresh, default payload for every frame.
    theremin_payload = {
        "instrument": "theremin",
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
                X_RIGHT_MIN = 0.0 ### X right hand limit
                X_RIGHT_MAX = 0.67 ###

                if label == "Right":
                    X_RIGHT_MIN = 0.0
                    X_RIGHT_MAX = 0.67
                    X_RIGHT_KILL = 0.80

                    if X_RIGHT_MIN <= index_tip.x <= X_RIGHT_KILL:
                        # Sound stays on until 0.80,
                        # but pitch/control value is clamped at 0.67.
                        theremin_payload["rightHandVisible"] = True
                        theremin_payload["rightHandX"] = min(index_tip.x, X_RIGHT_MAX)
                        theremin_payload["rightHandY"] = index_tip.y

                else:
                    theremin_payload["leftHandVisible"] = True
                    theremin_payload["leftHandX"] = index_tip.x
                    theremin_payload["leftHandY"] = index_tip.y

    # 2. Return the newly created payload, which is either updated or still in its default state.
    return theremin_payload
   

def _draw_trail(frame, trail: deque, color: tuple[int, int, int]) -> None:
    points = list(trail)
    if len(points) < 2:
        return

    # Create a blank, black overlay by copying the frame structure and zeroing it out.
    glow_overlay = frame.copy()
    glow_overlay[:] = 0  # Set all pixels to black

    n = len(points)
    for i in range(1, n):
        alpha = i / n  # 0 = oldest, 1 = newest

        # 1. Draw the thick "glow" line
        glow_thickness = max(2, int(alpha * _TRAIL_MAX_THICKNESS * 1.5))
        cv2.line(glow_overlay, points[i - 1], points[i], color, glow_thickness, lineType=cv2.LINE_AA)

        # 2. Draw the thin, bright "core" line directly on top of the glow
        core_thickness = max(1, int(alpha * _TRAIL_MAX_THICKNESS * 0.4))
        core_color = tuple(min(255, c + 150) for c in color)
        cv2.line(glow_overlay, points[i - 1], points[i], core_color, core_thickness, lineType=cv2.LINE_AA)

    # 3. Apply Gaussian Blur to the combined glow/core overlay
    blur_kernel_size = max(1, int(_TRAIL_MAX_THICKNESS * 0.5) | 1)
    blurred_glow = cv2.GaussianBlur(glow_overlay, (blur_kernel_size, blur_kernel_size), 0)

    # 4. Add the blurred effect layer to the original frame
    cv2.add(frame, blurred_glow, dst=frame)


def draw_circle(display_frame, frame_height, frame_width, detection_result):
    seen_labels: set[str] = set()

    if detection_result.handedness:
        for hand_landmarks, handedness in zip(
            detection_result.hand_landmarks, detection_result.handedness
        ):
            label = handedness[0].category_name
            seen_labels.add(label)
            thumb_tip = hand_landmarks[4]
            index_tip = hand_landmarks[8]

            center_x = int((1 - index_tip.x) * frame_width)
            center_y = int(index_tip.y * frame_height)
            pt = (center_x, center_y)

            trail = _right_trail if label == "Right" else _left_trail
            _append_trail(trail, pt)

            distance = ((thumb_tip.x - index_tip.x)**2 + (thumb_tip.y - index_tip.y)**2)**0.5
            PINCH_THRESHOLD = 0.05
            if distance < PINCH_THRESHOLD:
                color = _RIGHT_TRAIL_COLOR if label == "Right" else _LEFT_TRAIL_COLOR
                cv2.circle(display_frame, center=pt, radius=20, color=color, thickness=-1)

    # Clear trails for hands that left the frame
    if "Right" not in seen_labels:
        _right_trail.clear()
    if "Left" not in seen_labels:
        _left_trail.clear()

    # Draw trails beneath the circles
    _draw_trail(display_frame, _right_trail, _RIGHT_TRAIL_COLOR)
    _draw_trail(display_frame, _left_trail, _LEFT_TRAIL_COLOR)



def add_theremin_text(display_frame, theremin_payload):
    cv2.putText(display_frame,
        f"L: {theremin_payload['leftHandVisible']}, LeftX: {theremin_payload['leftHandX']:.2f}, LeftY: {theremin_payload['leftHandY']:.2f}",
        (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2
    )
    cv2.putText(display_frame,
        f"R: {theremin_payload['rightHandVisible']}, RightX: {theremin_payload['rightHandX']:.2f}, RightY: {theremin_payload['rightHandY']:.2f}",
        (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2
    )