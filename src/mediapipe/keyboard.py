from dataclasses import dataclass

import mediapipe as mp
import cv2

PINCH_THRESHOLD = 0.05

# Key MIDI notes (C4 = 60)
# Top Octave (C5-B5)
TOP_OCTAVE_NOTES = {
    "C+": 72, "C#+": 73, "D+": 74, "D#+": 75, "E+": 76, "F+": 77, "F#+": 78,
    "G+": 79, "G#+": 80, "A+": 81, "A#+": 82, "B+": 83, "C++": 84
}

# Bottom Octave (C4-B4)
BOTTOM_OCTAVE_NOTES = {
    "C-": 60, "C#-": 61, "D-": 62, "D#-": 63, "E-": 64, "F-": 65, "F#-": 66,
    "G-": 67, "G#-": 68, "A-": 69, "A#-": 70, "B-": 71, "C--": 72
}

# Define the layout for the keyboard zones
WHITE_KEY_WIDTH = 1.0 / 8
BLACK_KEY_WIDTH = WHITE_KEY_WIDTH * 0.6

# Top Octave
TOP_OCTAVE_Y_START = 0.2
TOP_OCTAVE_WHITE_Y_END = 0.6
TOP_OCTAVE_BLACK_Y_END = 0.5

# Bottom Octave
BOTTOM_OCTAVE_Y_START = 0.6
BOTTOM_OCTAVE_WHITE_Y_END = 1
BOTTOM_OCTAVE_BLACK_Y_END = 0.9


@dataclass(frozen=True)
class KeyboardZone:
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

KEYBOARD_ZONES = (
    # --- Top Octave ---
    # White Keys
    KeyboardZone("C+", TOP_OCTAVE_NOTES["C+"], 0, TOP_OCTAVE_Y_START, WHITE_KEY_WIDTH, TOP_OCTAVE_WHITE_Y_END),
    KeyboardZone("D+", TOP_OCTAVE_NOTES["D+"], WHITE_KEY_WIDTH, TOP_OCTAVE_Y_START, WHITE_KEY_WIDTH * 2, TOP_OCTAVE_WHITE_Y_END),
    KeyboardZone("E+", TOP_OCTAVE_NOTES["E+"], WHITE_KEY_WIDTH * 2, TOP_OCTAVE_Y_START, WHITE_KEY_WIDTH * 3, TOP_OCTAVE_WHITE_Y_END),
    KeyboardZone("F+", TOP_OCTAVE_NOTES["F+"], WHITE_KEY_WIDTH * 3, TOP_OCTAVE_Y_START, WHITE_KEY_WIDTH * 4, TOP_OCTAVE_WHITE_Y_END),
    KeyboardZone("G+", TOP_OCTAVE_NOTES["G+"], WHITE_KEY_WIDTH * 4, TOP_OCTAVE_Y_START, WHITE_KEY_WIDTH * 5, TOP_OCTAVE_WHITE_Y_END),
    KeyboardZone("A+", TOP_OCTAVE_NOTES["A+"], WHITE_KEY_WIDTH * 5, TOP_OCTAVE_Y_START, WHITE_KEY_WIDTH * 6, TOP_OCTAVE_WHITE_Y_END),
    KeyboardZone("B+", TOP_OCTAVE_NOTES["B+"], WHITE_KEY_WIDTH * 6, TOP_OCTAVE_Y_START, WHITE_KEY_WIDTH * 7, TOP_OCTAVE_WHITE_Y_END),
    KeyboardZone("C++", TOP_OCTAVE_NOTES["C++"], WHITE_KEY_WIDTH * 7, TOP_OCTAVE_Y_START, 1.0, TOP_OCTAVE_WHITE_Y_END),  # C6 (optional extension)
    # Black Keys
    KeyboardZone("C#+", TOP_OCTAVE_NOTES["C#+"], WHITE_KEY_WIDTH - (BLACK_KEY_WIDTH / 2), TOP_OCTAVE_Y_START, WHITE_KEY_WIDTH + (BLACK_KEY_WIDTH / 2), TOP_OCTAVE_BLACK_Y_END),
    KeyboardZone("D#+", TOP_OCTAVE_NOTES["D#+"], (WHITE_KEY_WIDTH * 2) - (BLACK_KEY_WIDTH / 2), TOP_OCTAVE_Y_START, (WHITE_KEY_WIDTH * 2) + (BLACK_KEY_WIDTH / 2), TOP_OCTAVE_BLACK_Y_END),
    KeyboardZone("F#+", TOP_OCTAVE_NOTES["F#+"], (WHITE_KEY_WIDTH * 4) - (BLACK_KEY_WIDTH / 2), TOP_OCTAVE_Y_START, (WHITE_KEY_WIDTH * 4) + (BLACK_KEY_WIDTH / 2), TOP_OCTAVE_BLACK_Y_END),
    KeyboardZone("G#+", TOP_OCTAVE_NOTES["G#+"], (WHITE_KEY_WIDTH * 5) - (BLACK_KEY_WIDTH / 2), TOP_OCTAVE_Y_START, (WHITE_KEY_WIDTH * 5) + (BLACK_KEY_WIDTH / 2), TOP_OCTAVE_BLACK_Y_END),
    KeyboardZone("A#+", TOP_OCTAVE_NOTES["A#+"], (WHITE_KEY_WIDTH * 6) - (BLACK_KEY_WIDTH / 2), TOP_OCTAVE_Y_START, (WHITE_KEY_WIDTH * 6) + (BLACK_KEY_WIDTH / 2), TOP_OCTAVE_BLACK_Y_END),

    # --- Bottom Octave ---
    # White Keys
    KeyboardZone("C-", BOTTOM_OCTAVE_NOTES["C-"], 0, BOTTOM_OCTAVE_Y_START, WHITE_KEY_WIDTH, BOTTOM_OCTAVE_WHITE_Y_END),
    KeyboardZone("D-", BOTTOM_OCTAVE_NOTES["D-"], WHITE_KEY_WIDTH, BOTTOM_OCTAVE_Y_START, WHITE_KEY_WIDTH * 2, BOTTOM_OCTAVE_WHITE_Y_END),
    KeyboardZone("E-", BOTTOM_OCTAVE_NOTES["E-"], WHITE_KEY_WIDTH * 2, BOTTOM_OCTAVE_Y_START, WHITE_KEY_WIDTH * 3, BOTTOM_OCTAVE_WHITE_Y_END),
    KeyboardZone("F-", BOTTOM_OCTAVE_NOTES["F-"], WHITE_KEY_WIDTH * 3, BOTTOM_OCTAVE_Y_START, WHITE_KEY_WIDTH * 4, BOTTOM_OCTAVE_WHITE_Y_END),
    KeyboardZone("G-", BOTTOM_OCTAVE_NOTES["G-"], WHITE_KEY_WIDTH * 4, BOTTOM_OCTAVE_Y_START, WHITE_KEY_WIDTH * 5, BOTTOM_OCTAVE_WHITE_Y_END),
    KeyboardZone("A-", BOTTOM_OCTAVE_NOTES["A-"], WHITE_KEY_WIDTH * 5, BOTTOM_OCTAVE_Y_START, WHITE_KEY_WIDTH * 6, BOTTOM_OCTAVE_WHITE_Y_END),
    KeyboardZone("B-", BOTTOM_OCTAVE_NOTES["B-"], WHITE_KEY_WIDTH * 6, BOTTOM_OCTAVE_Y_START, WHITE_KEY_WIDTH * 7, BOTTOM_OCTAVE_WHITE_Y_END),
    KeyboardZone("C--", BOTTOM_OCTAVE_NOTES["C--"], WHITE_KEY_WIDTH * 7, BOTTOM_OCTAVE_Y_START, 1.0, BOTTOM_OCTAVE_WHITE_Y_END),  # C5 (optional extension)
    # Black Keys
    KeyboardZone("C#-", BOTTOM_OCTAVE_NOTES["C#-"], WHITE_KEY_WIDTH - (BLACK_KEY_WIDTH / 2), BOTTOM_OCTAVE_Y_START, WHITE_KEY_WIDTH + (BLACK_KEY_WIDTH / 2), BOTTOM_OCTAVE_BLACK_Y_END),
    KeyboardZone("D#-", BOTTOM_OCTAVE_NOTES["D#-"], (WHITE_KEY_WIDTH * 2) - (BLACK_KEY_WIDTH / 2), BOTTOM_OCTAVE_Y_START, (WHITE_KEY_WIDTH * 2) + (BLACK_KEY_WIDTH / 2), BOTTOM_OCTAVE_BLACK_Y_END),
    KeyboardZone("F#-", BOTTOM_OCTAVE_NOTES["F#-"], (WHITE_KEY_WIDTH * 4) - (BLACK_KEY_WIDTH / 2), BOTTOM_OCTAVE_Y_START, (WHITE_KEY_WIDTH * 4) + (BLACK_KEY_WIDTH / 2), BOTTOM_OCTAVE_BLACK_Y_END),
    KeyboardZone("G#-", BOTTOM_OCTAVE_NOTES["G#-"], (WHITE_KEY_WIDTH * 5) - (BLACK_KEY_WIDTH / 2), BOTTOM_OCTAVE_Y_START, (WHITE_KEY_WIDTH * 5) + (BLACK_KEY_WIDTH / 2), BOTTOM_OCTAVE_BLACK_Y_END),
    KeyboardZone("A#-", BOTTOM_OCTAVE_NOTES["A#-"], (WHITE_KEY_WIDTH * 6) - (BLACK_KEY_WIDTH / 2), BOTTOM_OCTAVE_Y_START, (WHITE_KEY_WIDTH * 6) + (BLACK_KEY_WIDTH / 2), BOTTOM_OCTAVE_BLACK_Y_END),
)

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


def draw_keyboard_zones(frame, active_zone_names: set[str]) -> None:
    overlay = frame.copy()
    height, width = frame.shape[:2]

    # Sort keys to draw white keys first, then black keys
    sorted_zones = sorted(KEYBOARD_ZONES, key=lambda z: "#" in z.name)

    for zone in sorted_zones:
        left, top, right, bottom = zone.pixel_rect(width, height)
        is_active = zone.name in active_zone_names
        is_black_key = "#" in zone.name

        if is_black_key:
            fill_color = (0, 120, 255) if is_active else (0, 0, 0)
            border_color = (0, 180, 255) if is_active else (80, 80, 80)
        else:
            fill_color = (0, 120, 255) if is_active else (255, 255, 255)
            border_color = (0, 180, 255) if is_active else (150, 150, 150)

        cv2.rectangle(overlay, (left, top), (right, bottom), fill_color, -1)
        cv2.rectangle(overlay, (left, top), (right, bottom), border_color, 2)

    cv2.addWeighted(overlay, 0.3, frame, 0.7, 0.0, frame)


def find_key_zone(x: float, y: float) -> KeyboardZone | None:
    # Prioritize black keys in hit detection
    sorted_zones = sorted(KEYBOARD_ZONES, key=lambda z: "#" in z.name, reverse=True)
    for zone in sorted_zones:
        if zone.contains(x, y):
            return zone
    return None

# --- State Tracking ---
# Keep track of which notes are currently being played by each hand
left_hand_notes = {}  # {note: True}
right_hand_notes = {} # {note: True}
# --------------------

def detect_key_strokes(detection_result):
    global left_hand_notes, right_hand_notes

    keyboard_payload = {
        "instrument": "keyboard",
        "rightHandVisible": False,
        "leftHandVisible": False,
        "rightHandX": 0.0,
        "rightHandY": 0.0,
        "leftHandX": 0.0,
        "leftHandY": 0.0,
        "notesOn": [],
        "notesOff": [],
    }
    
    active_zone_names: set[str] = set()
    displayed_hand_positions: dict[str, tuple[float, float]] = {}
    
    current_left_notes = {}
    current_right_notes = {}

    if detection_result.handedness:
        processed_labels = set()
        for hand_landmarks, handedness in zip(detection_result.hand_landmarks, detection_result.handedness):
            label = handedness[0].category_name
            if label in processed_labels:
                continue
            processed_labels.add(label)

            # We'll use the index finger tip for hit detection
            index_tip = hand_landmarks[8]
            
            display_x = 1.0 - index_tip.x
            display_y = index_tip.y
            
            displayed_hand_positions[label] = (display_x, display_y)

            if label == "Right":
                keyboard_payload["rightHandVisible"] = True
                keyboard_payload["rightHandX"] = display_x
                keyboard_payload["rightHandY"] = display_y
            else:
                keyboard_payload["leftHandVisible"] = True
                keyboard_payload["leftHandX"] = display_x
                keyboard_payload["leftHandY"] = display_y

            zone = find_key_zone(display_x, display_y)
            
            # If a hand is in a zone, mark the note as active for that hand
            if zone:
                active_zone_names.add(zone.name)
                if label == "Right":
                    current_right_notes[zone.note] = True
                else:
                    current_left_notes[zone.note] = True

    # Determine which notes to turn ON
    new_right_notes = [note for note in current_right_notes if note not in right_hand_notes]
    new_left_notes = [note for note in current_left_notes if note not in left_hand_notes]
    keyboard_payload["notesOn"] = new_right_notes + new_left_notes

    # Determine which notes to turn OFF
    stopped_right_notes = [note for note in right_hand_notes if note not in current_right_notes]
    stopped_left_notes = [note for note in left_hand_notes if note not in current_left_notes]
    keyboard_payload["notesOff"] = stopped_right_notes + stopped_left_notes

    # Update the state for the next frame
    right_hand_notes = current_right_notes
    left_hand_notes = current_left_notes
    
    return keyboard_payload, active_zone_names, displayed_hand_positions

