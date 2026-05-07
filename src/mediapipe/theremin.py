import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from mediapipe.tasks.python.vision import RunningMode
from collections import deque
from pathlib import Path


MODEL_PATH = Path(__file__).with_name("hand_landmarker.task")
if not MODEL_PATH.exists():
    raise FileNotFoundError(f"MediaPipe hand landmarker model not found: {MODEL_PATH}")

hand_base_options = python.BaseOptions(model_asset_path=str(MODEL_PATH))
options = vision.HandLandmarkerOptions(base_options=hand_base_options,running_mode=RunningMode.VIDEO,num_hands=2)
recognizer = vision.HandLandmarker.create_from_options(options)

TRAIL_LENGTH = 50
TRAIL_MAX_THICKNESS = 20
RIGHT_TRAIL_COLOR = (20, 100, 255)   # orange
LEFT_TRAIL_COLOR  = (20, 100, 255)   # orange

left_trail:  deque[tuple[int, int]] = deque(maxlen=TRAIL_LENGTH)
right_trail: deque[tuple[int, int]] = deque(maxlen=TRAIL_LENGTH)

MAX_TRAIL_JUMP_PX = 80


def append_trail(trail: deque, pt: tuple[int, int]) -> None:
    if trail:
        dx = pt[0] - trail[-1][0]
        dy = pt[1] - trail[-1][1]
        if dx * dx + dy * dy > MAX_TRAIL_JUMP_PX ** 2:
            trail.clear()
    trail.append(pt)


def detect_hands(detection_result): 
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
            label = handedness[0].category_name 
            thumb_tip = hand_landmarks[4]  
            index_tip = hand_landmarks[8]  

            distance = ((thumb_tip.x - index_tip.x)**2 + (thumb_tip.y - index_tip.y)**2)**0.5
            PINCH_THRESHOLD = 0.05  

            if distance < PINCH_THRESHOLD:
                X_RIGHT_MIN = 0.0 
                X_RIGHT_MAX = 0.67

                if label == "Right":
                    X_RIGHT_MIN = 0.0
                    X_RIGHT_MAX = 0.67
                    X_RIGHT_KILL = 0.80
                    if X_RIGHT_MIN <= index_tip.x <= X_RIGHT_KILL:
                        theremin_payload["rightHandVisible"] = True
                        theremin_payload["rightHandX"] = min(index_tip.x, X_RIGHT_MAX)
                        theremin_payload["rightHandY"] = index_tip.y
                else:
                    X_LEFT_MIN = 0.67
                    X_LEFT_MAX = 1.0
                    X_LEFT_KILL = 0.53
                    if X_LEFT_KILL <= index_tip.x <= X_LEFT_MAX:
                        theremin_payload["leftHandVisible"] = True
                        theremin_payload["leftHandX"] = max(index_tip.x, X_LEFT_KILL)
                        theremin_payload["leftHandY"] = index_tip.y

    return theremin_payload
   

def draw_trail(frame, trail: deque, color: tuple[int, int, int]) -> None:
    points = list(trail)
    if len(points) < 2:
        return

    glow_overlay = frame.copy()
    glow_overlay[:] = 0 
    n = len(points)
    for i in range(1, n):
        alpha = i / n 

        glow_thickness = max(2, int(alpha * TRAIL_MAX_THICKNESS * 1.5))
        cv2.line(glow_overlay, points[i - 1], points[i], color, glow_thickness, lineType=cv2.LINE_AA)

        core_thickness = max(1, int(alpha * TRAIL_MAX_THICKNESS * 0.4))
        core_color = tuple(min(255, c + 150) for c in color)
        cv2.line(glow_overlay, points[i - 1], points[i], core_color, core_thickness, lineType=cv2.LINE_AA)

    blur_kernel_size = max(1, int(TRAIL_MAX_THICKNESS * 0.5) | 1)
    blurred_glow = cv2.GaussianBlur(glow_overlay, (blur_kernel_size, blur_kernel_size), 0)

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

            trail = right_trail if label == "Right" else left_trail
            append_trail(trail, pt)

            distance = ((thumb_tip.x - index_tip.x)**2 + (thumb_tip.y - index_tip.y)**2)**0.5
            PINCH_THRESHOLD = 0.05
            if distance < PINCH_THRESHOLD:
                color = RIGHT_TRAIL_COLOR if label == "Right" else LEFT_TRAIL_COLOR
                cv2.circle(display_frame, center=pt, radius=20, color=color, thickness=-1)

    if "Right" not in seen_labels:
        right_trail.clear()
    if "Left" not in seen_labels:
        left_trail.clear()

    draw_trail(display_frame, right_trail, RIGHT_TRAIL_COLOR)
    draw_trail(display_frame, left_trail, LEFT_TRAIL_COLOR)



def add_theremin_text(display_frame, theremin_payload):
    cv2.putText(display_frame,
        f"L: {theremin_payload['leftHandVisible']}, LeftX: {theremin_payload['leftHandX']:.2f}, LeftY: {theremin_payload['leftHandY']:.2f}",
        (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2
    )
    cv2.putText(display_frame,
        f"R: {theremin_payload['rightHandVisible']}, RightX: {theremin_payload['rightHandX']:.2f}, RightY: {theremin_payload['rightHandY']:.2f}",
        (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2
    )