import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from mediapipe.tasks.python.vision import RunningMode
from mediapipe.tasks.python.vision import face_landmarker as face_landmarker_module
from dataclasses import dataclass
from pathlib import Path

PINCH_THRESHOLD = 0.05
DEFAULT_DRUM_VELOCITY = 110

JAW_OPEN_BLENDSHAPE_INDEX = int(face_landmarker_module.Blendshapes.JAW_OPEN)
MOUTH_OPEN_ON_THRESHOLD = 0.25
MOUTH_OPEN_OFF_THRESHOLD = 0.15

FACE_MODEL_PATH = Path(__file__).with_name("face_landmarker.task")
MODEL_PATH = Path(__file__).with_name("gesture_recognizer.task")

face_landmarker = None
active_triggered_zones = set()
previous_mouth_open = False


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
    DrumZone("Closed Hi-Hat", 42, 0.02, 0.56, 0.28, 0.71),
    DrumZone("Open Hi-Hat", 46, 0.02, 0.36, 0.28, 0.52),
    DrumZone("Snare", 38, 0.29, 0.49, 0.48, 0.68),
    DrumZone("Floor Tom", 43, 0.77, 0.49, 0.98, 0.71),
    DrumZone("Kick", 36, 0.31, 0.75, 0.69, 0.96),
)


def get_face_landmarker():
    global face_landmarker

    if face_landmarker is not None:
        return face_landmarker

    if not FACE_MODEL_PATH.exists():
        raise FileNotFoundError(f"MediaPipe face landmarker model not found: {FACE_MODEL_PATH}")

    face_options = face_landmarker_module.FaceLandmarkerOptions(
        base_options=python.BaseOptions(model_asset_path=str(FACE_MODEL_PATH)),
        running_mode=RunningMode.IMAGE,
        num_faces=1,
        min_face_detection_confidence=0.5,
        min_face_presence_confidence=0.5,
        min_tracking_confidence=0.5,
        output_face_blendshapes=True,
    )

    face_landmarker = face_landmarker_module.FaceLandmarker.create_from_options(face_options)
    return face_landmarker


def is_pinch(hand_landmarks) -> bool:
    thumb_tip = hand_landmarks[4]
    index_tip = hand_landmarks[8]

    distance = (
        (thumb_tip.x - index_tip.x) ** 2 +
        (thumb_tip.y - index_tip.y) ** 2
    ) ** 0.5

    return distance < PINCH_THRESHOLD


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


def detect_mouth_open(mp_image, was_open: bool) -> bool:
    landmarker = get_face_landmarker()
    face_result = landmarker.detect(mp_image)

    if not face_result.face_blendshapes:
        return False

    jaw_open_score = 0.0

    for blendshape in face_result.face_blendshapes[0]:
        if blendshape.index == JAW_OPEN_BLENDSHAPE_INDEX or blendshape.category_name in ("jawOpen", "JAW_OPEN"):
            jaw_open_score = blendshape.score
            break

    threshold = MOUTH_OPEN_OFF_THRESHOLD if was_open else MOUTH_OPEN_ON_THRESHOLD
    return jaw_open_score >= threshold


if not MODEL_PATH.exists():
    raise FileNotFoundError(f"MediaPipe gesture recognizer model not found: {MODEL_PATH}")

base_options = python.BaseOptions(model_asset_path=str(MODEL_PATH))
ClassifierOptions = mp.tasks.components.processors.ClassifierOptions

options = vision.GestureRecognizerOptions(
    base_options=base_options,
    num_hands=2,
    min_hand_detection_confidence=0.1,
    min_tracking_confidence=0.1,
    canned_gesture_classifier_options=ClassifierOptions(score_threshold=0.1),
)

recognizer = vision.GestureRecognizer.create_from_options(options)


def drum_detect(recognition_result, mp_image=None):
    global active_triggered_zones
    global previous_mouth_open

    active_zone_names: set[str] = set()
    displayed_hand_positions: dict[str, tuple[float, float]] = {}
    processed_labels = set()

    drum_payload = {
        "instrument": "drums",
        "rightHandVisible": False,
        "leftHandVisible": False,
        "rightHandX": 0.0,
        "rightHandY": 0.0,
        "leftHandX": 0.0,
        "leftHandY": 0.0,
        "leftDrumHit": False,
        "leftDrumType": 38,
        "leftDrumVelocity": DEFAULT_DRUM_VELOCITY,
        "rightDrumHit": False,
        "rightDrumType": 42,
        "rightDrumVelocity": DEFAULT_DRUM_VELOCITY,
        "mouthKickHit": False,
    }

    if mp_image is not None:
        is_mouth_open = detect_mouth_open(mp_image, previous_mouth_open)

        if is_mouth_open and not previous_mouth_open:
            drum_payload["mouthKickHit"] = True

        previous_mouth_open = is_mouth_open

    hands_in_zones = {}

    if recognition_result.handedness:
        for hand_landmarks, handedness, gestures in zip(
            recognition_result.hand_landmarks,
            recognition_result.handedness,
            recognition_result.gestures,
        ):
            label = handedness[0].category_name

            if label in processed_labels:
                continue

            processed_labels.add(label)

            pinch = is_pinch(hand_landmarks)

            index_tip = hand_landmarks[8]
            display_x = 1.0 - index_tip.x
            display_y = index_tip.y

            displayed_hand_positions[label] = (display_x, display_y)

            if label == "Right":
                drum_payload["rightHandVisible"] = True
                drum_payload["rightHandX"] = display_x
                drum_payload["rightHandY"] = display_y
            else:
                drum_payload["leftHandVisible"] = True
                drum_payload["leftHandX"] = display_x
                drum_payload["leftHandY"] = display_y

            zone = find_zone(display_x, display_y)

            if zone is not None:
                hands_in_zones[zone.name] = (pinch, label, zone)

    zones_to_remove = set()

    for zone_name in active_triggered_zones:
        if zone_name not in hands_in_zones:
            zones_to_remove.add(zone_name)
        else:
            pinch, _, _ = hands_in_zones[zone_name]

            if not pinch:
                zones_to_remove.add(zone_name)

    active_triggered_zones.difference_update(zones_to_remove)

    for zone_name, (pinch, label, zone) in hands_in_zones.items():
        if pinch:
            active_zone_names.add(zone_name)

            if zone_name not in active_triggered_zones:
                prefix = "right" if label == "Right" else "left"

                drum_payload[f"{prefix}DrumHit"] = True
                drum_payload[f"{prefix}DrumType"] = zone.note
                drum_payload[f"{prefix}DrumVelocity"] = DEFAULT_DRUM_VELOCITY

                active_triggered_zones.add(zone_name)

    return drum_payload, active_zone_names, displayed_hand_positions


def get_drum_hit_coordinates(
    display_frame,
    frame_height,
    frame_width,
    active_zone_names,
    displayed_hand_positions,
):
    draw_drum_zones(display_frame, active_zone_names)

    for _, position in displayed_hand_positions.items():
        center_x = int(position[0] * frame_width)
        center_y = int(position[1] * frame_height)

        cv2.circle(
            display_frame,
            center=(center_x, center_y),
            radius=10,
            color=(0, 180, 255),
            thickness=-1,
        )