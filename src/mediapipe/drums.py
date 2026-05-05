from __future__ import annotations

import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from mediapipe.tasks.python.vision import RunningMode
from mediapipe.tasks.python.vision import face_landmarker as face_landmarker_module
from dataclasses import dataclass
from pathlib import Path

PINCH_ON_THRESHOLD = 0.035
PINCH_OFF_THRESHOLD = 0.05
PINCH_RELEASE_VELOCITY = 0.008

previous_pinch_distances = {"Left": 1.0, "Right": 1.0}
active_pinches = {"Left": False, "Right": False}

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
        center_x = (self.left + self.right) / 2.0
        center_y = (self.top + self.bottom) / 2.0
        raw_radius_x = (self.right - self.left) / 2.0
        raw_radius_y = (self.bottom - self.top) / 2.0

        if self.name == "Kick":
            radius_x = raw_radius_x
            radius_y = raw_radius_y
        else:
            radius = min(raw_radius_x, raw_radius_y) * 1.1
            radius_x = radius
            radius_y = radius

        if radius_x <= 0.0 or radius_y <= 0.0:
            return False

        dx = (x - center_x) / radius_x
        dy = (y - center_y) / radius_y
        return (dx * dx) + (dy * dy) <= 1.0

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
    DrumZone("Closed Hi-Hat", 42, 0.02, 0.60, 0.28, 0.75),
    DrumZone("Open Hi-Hat", 46, 0.02, 0.40, 0.28, 0.56),
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
        running_mode=RunningMode.VIDEO,
        num_faces=1,
        min_face_detection_confidence=0.5,
        min_face_presence_confidence=0.5,
        min_tracking_confidence=0.5,
        output_face_blendshapes=True,
    )

    face_landmarker = face_landmarker_module.FaceLandmarker.create_from_options(face_options)
    return face_landmarker


def is_pinch(label: str, hand_landmarks) -> bool:
    global previous_pinch_distances, active_pinches

    thumb_tip = hand_landmarks[4]
    index_tip = hand_landmarks[8]

    current_dist = (
        (thumb_tip.x - index_tip.x) ** 2 +
        (thumb_tip.y - index_tip.y) ** 2
    ) ** 0.5

    prev_dist = previous_pinch_distances.get(label, 1.0)
    currently_pinched = active_pinches.get(label, False)

    if currently_pinched and (current_dist - prev_dist > PINCH_RELEASE_VELOCITY):
        currently_pinched = False

    elif currently_pinched and current_dist > PINCH_OFF_THRESHOLD:
        currently_pinched = False

    elif not currently_pinched and current_dist < PINCH_ON_THRESHOLD:
        currently_pinched = True

    previous_pinch_distances[label] = current_dist
    active_pinches[label] = currently_pinched

    return currently_pinched


def find_zone(x: float, y: float) -> DrumZone | None:
    for zone in DRUM_ZONES:
        if zone.contains(x, y):
            return zone
    return None


def draw_drum_zones(frame, active_zone_names: set[str]) -> None:
    return


def detect_mouth_open(mp_image, was_open: bool, timestamp_ms: int) -> bool:
    landmarker = get_face_landmarker()
    face_result = landmarker.detect_for_video(mp_image, timestamp_ms)

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
    running_mode=RunningMode.VIDEO,
    num_hands=2,
    min_hand_detection_confidence=0.1,
    min_tracking_confidence=0.1,
    canned_gesture_classifier_options=ClassifierOptions(score_threshold=0.1),
)

recognizer = vision.GestureRecognizer.create_from_options(options)


def drum_detect(recognition_result, mp_image=None, timestamp_ms=0):
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
        "rightPinch": False,
        "leftPinch": False,
        "leftDrumHit": False,
        "leftDrumType": 38,
        "rightDrumHit": False,
        "rightDrumType": 42,
        "mouthKickHit": False,
    }

    if mp_image is not None:
        is_mouth_open = detect_mouth_open(mp_image, previous_mouth_open, timestamp_ms)

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

            pinch = is_pinch(label, hand_landmarks)

            index_tip = hand_landmarks[8]
            display_x = 1.0 - index_tip.x
            display_y = index_tip.y

            displayed_hand_positions[label] = (display_x, display_y)

            if label == "Right":
                drum_payload["rightHandVisible"] = True
                drum_payload["rightHandX"] = display_x
                drum_payload["rightHandY"] = display_y
                drum_payload["rightPinch"] = pinch
            else:
                drum_payload["leftHandVisible"] = True
                drum_payload["leftHandX"] = display_x
                drum_payload["leftHandY"] = display_y
                drum_payload["leftPinch"] = pinch

            zone = find_zone(display_x, display_y)

            if zone is not None:
                hand_zone_key = f"{label}_{zone.name}"
                hands_in_zones[hand_zone_key] = (pinch, label, zone)

    zones_to_remove = set()

    for hand_zone_key in active_triggered_zones:
        if hand_zone_key not in hands_in_zones:
            zones_to_remove.add(hand_zone_key)
        else:
            pinch, _, _ = hands_in_zones[hand_zone_key]

            if not pinch:
                zones_to_remove.add(hand_zone_key)

    active_triggered_zones.difference_update(zones_to_remove)

    for hand_zone_key, (pinch, label, zone) in hands_in_zones.items():
        if pinch:
            # We just add the zone.name here so the QML UI still lights up properly!
            active_zone_names.add(zone.name) 

            # Check if THIS specific hand has already triggered THIS drum
            if hand_zone_key not in active_triggered_zones:
                prefix = "right" if label == "Right" else "left"

                drum_payload[f"{prefix}DrumHit"] = True
                drum_payload[f"{prefix}DrumType"] = zone.note

                active_triggered_zones.add(hand_zone_key)

    return drum_payload, active_zone_names, displayed_hand_positions


def get_drum_hit_coordinates(
    display_frame,
    frame_height,
    frame_width,
    active_zone_names,
    displayed_hand_positions,
):
    return
