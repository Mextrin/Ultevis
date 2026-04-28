from dataclasses import dataclass
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
TOP_OCTAVE_Y_START = 0.12
TOP_OCTAVE_WHITE_Y_END = 0.55
TOP_OCTAVE_BLACK_Y_END = 0.32

# Bottom Octave
BOTTOM_OCTAVE_Y_START = 0.6
BOTTOM_OCTAVE_WHITE_Y_END = 1
BOTTOM_OCTAVE_BLACK_Y_END = 0.8


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