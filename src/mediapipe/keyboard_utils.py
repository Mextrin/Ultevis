from dataclasses import dataclass
PINCH_THRESHOLD = 0.05

WHITE_KEYS_PER_KEYBOARD = 14
WHITE_KEY_WIDTH = 1.0 / WHITE_KEYS_PER_KEYBOARD
BLACK_KEY_WIDTH = WHITE_KEY_WIDTH * 0.65

WHITE_KEY_STEPS = (
    ("C", 0), ("D", 2), ("E", 4),
    ("F", 5), ("G", 7), ("A", 9), ("B", 11),
    ("C2", 12), ("D2", 14), ("E2", 16),
    ("F2", 17), ("G2", 19), ("A2", 21), ("B2", 23),
)

BLACK_KEY_STEPS = (
    ("C#", 1, 1), ("D#", 3, 2),
    ("F#", 6, 4), ("G#", 8, 5), ("A#", 10, 6),
    ("C#2", 13, 8), ("D#2", 15, 9),
    ("F#2", 18, 11), ("G#2", 20, 12), ("A#2", 22, 13),
)

TOP_OCTAVE_Y_START = 0.12
TOP_OCTAVE_WHITE_Y_END = 0.42
TOP_OCTAVE_BLACK_Y_END = 0.28

BOTTOM_OCTAVE_Y_START = 0.55
BOTTOM_OCTAVE_WHITE_Y_END = 0.85
BOTTOM_OCTAVE_BLACK_Y_END = 0.71


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

def _zone_name(note_name: str, suffix: str) -> str:
    return f"{note_name}{suffix}"


def _build_keyboard_zones(
    suffix: str,
    base_note: int,
    y_start: float,
    white_y_end: float,
    black_y_end: float,
) -> tuple[KeyboardZone, ...]:
    zones: list[KeyboardZone] = []

    for index, (note_name, semitone_offset) in enumerate(WHITE_KEY_STEPS):
        zones.append(
            KeyboardZone(
                _zone_name(note_name, suffix),
                base_note + semitone_offset,
                WHITE_KEY_WIDTH * index,
                y_start,
                WHITE_KEY_WIDTH * (index + 1),
                white_y_end,
            )
        )

    for note_name, semitone_offset, boundary_index in BLACK_KEY_STEPS:
        center_x = WHITE_KEY_WIDTH * boundary_index
        zones.append(
            KeyboardZone(
                _zone_name(note_name, suffix),
                base_note + semitone_offset,
                center_x - (BLACK_KEY_WIDTH / 2),
                y_start,
                center_x + (BLACK_KEY_WIDTH / 2),
                black_y_end,
            )
        )

    return tuple(zones)


KEYBOARD_ZONES = (
    *_build_keyboard_zones("+", 72, TOP_OCTAVE_Y_START, TOP_OCTAVE_WHITE_Y_END, TOP_OCTAVE_BLACK_Y_END),
    *_build_keyboard_zones("-", 60, BOTTOM_OCTAVE_Y_START, BOTTOM_OCTAVE_WHITE_Y_END, BOTTOM_OCTAVE_BLACK_Y_END),
)
