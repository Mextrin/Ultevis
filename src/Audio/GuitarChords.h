#pragma once

#include "../Core/GlobalState.h"
#include <array>

namespace airchestra {

// Helper for base MIDI notes of standard tuning: E2, A2, D3, G3, B3, E4
constexpr int GUITAR_STRINGS[6] = { 40, 45, 50, 55, 59, 64 };

// Standard Barre Shape Offsets (-1 = muted string)
struct BarreShape {
    int offsets[6];
};

// Standard E-Shape (Root on 6th String)
constexpr BarreShape E_SHAPES[10] = {
    {0, 2, 2, 1, 0, 0},    // 0: Major
    {0, 2, 2, 0, 0, 0},    // 1: Minor
    {0, 2, 0, 1, 0, 0},    // 2: Dom7
    {0, 2, 1, 1, 0, 0},    // 3: Maj7
    {0, 2, 0, 0, 0, 0},    // 4: Min7
    {0, 2, 4, -1, 0, 0},   // 5: Sus2
    {0, 2, 2, 2, 0, 0},    // 6: Sus4
    {0, 1, 2, 0, -1, -1},  // 7: Diminished
    {0, 1, 0, 0, -1, -1},  // 8: Min7b5
    {0, 3, 2, 1, -1, -1}   // 9: Augmented
};

// Standard A-Shape (Root on 5th String)
constexpr BarreShape A_SHAPES[10] = {
    {-1, 0, 2, 2, 2, 0},   // 0: Major
    {-1, 0, 2, 2, 1, 0},   // 1: Minor
    {-1, 0, 2, 0, 2, 0},   // 2: Dom7
    {-1, 0, 2, 1, 2, 0},   // 3: Maj7
    {-1, 0, 2, 0, 1, 0},   // 4: Min7
    {-1, 0, 2, 2, 0, 0},   // 5: Sus2
    {-1, 0, 2, 2, 3, 0},   // 6: Sus4
    {-1, 0, 1, 2, 1, -1},  // 7: Diminished
    {-1, 0, 1, 0, 1, -1},  // 8: Min7b5
    {-1, 0, 3, 2, 2, -1}   // 9: Augmented
};

// Standard D-Shape (Root on 4th String)
constexpr BarreShape D_SHAPES[10] = {
    {-1, -1, 0, 2, 3, 2},  // 0: Major
    {-1, -1, 0, 2, 3, 1},  // 1: Minor
    {-1, -1, 0, 2, 1, 2},  // 2: Dom7
    {-1, -1, 0, 2, 2, 2},  // 3: Maj7
    {-1, -1, 0, 2, 1, 1},  // 4: Min7
    {-1, -1, 0, 2, 3, 0},  // 5: Sus2
    {-1, -1, 0, 2, 3, 3},  // 6: Sus4
    {-1, -1, 0, 1, 3, 1},  // 7: Diminished
    {-1, -1, 0, 1, 1, 1},  // 8: Min7b5
    {-1, -1, 0, 3, 3, 2}   // 9: Augmented
};

enum ShapeType { SHAPE_E, SHAPE_A, SHAPE_D };

// Mathematically applies a chord shape to a specific fret
constexpr std::array<int, 6> applyShape(ShapeType type, int quality, int fret) {
    std::array<int, 6> res = {-1, -1, -1, -1, -1, -1};
    const BarreShape* shapeSet = (type == SHAPE_E) ? E_SHAPES : ((type == SHAPE_A) ? A_SHAPES : D_SHAPES);
    for(int s = 0; s < 6; ++s) {
        int offset = shapeSet[quality].offsets[s];
        if(offset != -1) {
            res[s] = GUITAR_STRINGS[s] + fret + offset;
        }
    }
    return res;
}

// Compile-time database holding [Root][Quality][Voicing][String]
struct GuitarChordDb {
    int chords[7][10][3][6];

    constexpr GuitarChordDb() : chords{} {
        // 1. Maintain your original hand-crafted open chords for Voicing 1
        int origC[7][6] = { { -1, 48, 52, 55, 60, 64 }, { -1, 48, 55, 60, 63, 67 }, { -1, 48, 52, 58, 60, 64 }, { -1, 48, 52, 55, 59, 64 }, { -1, 48, 55, 58, 63, 67 }, { -1, 48, 55, 60, 62, 67 }, { -1, 48, 55, 60, 65, 67 } };
        int origD[7][6] = { { -1, -1, 50, 57, 62, 66 }, { -1, -1, 50, 57, 62, 65 }, { -1, -1, 50, 57, 60, 66 }, { -1, -1, 50, 57, 61, 66 }, { -1, -1, 50, 57, 60, 65 }, { -1, -1, 50, 57, 62, 64 }, { -1, -1, 50, 57, 62, 67 } };
        int origE[7][6] = { { 40, 47, 52, 56, 59, 64 }, { 40, 47, 52, 55, 59, 64 }, { 40, 47, 50, 56, 59, 64 }, { 40, 47, 51, 56, 59, 64 }, { 40, 47, 52, 55, 62, 64 }, { 40, 47, 54, 56, 59, 64 }, { 40, 47, 52, 57, 59, 64 } };
        int origF[7][6] = { { 41, 48, 53, 57, 60, 65 }, { 41, 48, 53, 56, 60, 65 }, { 41, 48, 51, 57, 60, 65 }, { -1, -1, 53, 57, 60, 64 }, { 41, 48, 51, 56, 60, 65 }, { 41, 48, 53, 55, 60, 65 }, { 41, 48, 53, 58, 60, 65 } };
        int origG[7][6] = { { 43, 47, 50, 55, 59, 67 }, { 43, 50, 55, 58, 62, 67 }, { 43, 47, 50, 55, 59, 65 }, { 43, 47, 50, 55, 59, 66 }, { 43, 50, 53, 58, 62, 67 }, { 43, 45, 50, 55, 62, 67 }, { 43, 48, 50, 55, 60, 67 } };
        int origA[7][6] = { { -1, 45, 52, 57, 61, 64 }, { -1, 45, 52, 57, 60, 64 }, { -1, 45, 52, 55, 61, 64 }, { -1, 45, 52, 56, 61, 64 }, { -1, 45, 52, 55, 60, 64 }, { -1, 45, 52, 57, 59, 64 }, { -1, 45, 52, 57, 62, 64 } };
        int origB[7][6] = { { -1, 47, 54, 59, 63, 66 }, { -1, 47, 54, 59, 62, 66 }, { -1, 47, 51, 57, 59, 66 }, { -1, 47, 54, 58, 63, 66 }, { -1, 47, 54, 57, 62, 66 }, { -1, 47, 54, 59, 61, 66 }, { -1, 47, 54, 59, 64, 66 } };

        int (*origs[7])[6] = {origC, origD, origE, origF, origG, origA, origB};

        // 2. Map standard shapes to specifically hit the desired fret zones
        ShapeType types[7][3] = {
            {SHAPE_A, SHAPE_E, SHAPE_A}, // C
            {SHAPE_D, SHAPE_E, SHAPE_A}, // D
            {SHAPE_E, SHAPE_A, SHAPE_D}, // E
            {SHAPE_E, SHAPE_A, SHAPE_D}, // F
            {SHAPE_E, SHAPE_A, SHAPE_E}, // G
            {SHAPE_A, SHAPE_D, SHAPE_E}, // A
            {SHAPE_A, SHAPE_E, SHAPE_A}  // B
        };

        // Define the target fret positions for [Voicing 1(Open-7)][Voicing 2(7-15)][Voicing 3(15-21)]
        int frets[7][3] = {
            {3, 8, 15},  // C
            {0, 10, 17}, // D
            {0, 7, 14},  // E
            {1, 8, 15},  // F
            {3, 10, 15}, // G
            {0, 7, 17},  // A
            {2, 7, 14}   // B
        };

        // 3. Compile-time Loop to build the 1,260 element table
        for(int r = 0; r < 7; ++r) {
            for(int v = 0; v < 3; ++v) {
                for(int q = 0; q < 10; ++q) {
                    
                    // Retain your custom voicings for the first 7 qualities of Voicing 1
                    if (v == 0 && q < 7) {
                        for(int s = 0; s < 6; ++s) chords[r][q][v][s] = origs[r][q][s];
                    } else {
                        // Dynamically compute the rest using perfectly transposed standard shapes
                        auto c = applyShape(types[r][v], q, frets[r][v]);
                        for(int s = 0; s < 6; ++s) chords[r][q][v][s] = c[s];
                    }
                    
                }
            }
        }
    }
};

// Instantiate the database once at compile-time
constexpr GuitarChordDb CHORD_DB;

// Now accepts `voicing` as a parameter (defaulting to 0/Open)
inline const int* getGuitarChord(GuitarChordRoot root, GuitarChordQuality quality, int voicing = 0)
{
    return CHORD_DB.chords[
        static_cast<int>(root)
    ][
        static_cast<int>(quality)
    ][
        voicing
    ];
}

} // namespace airchestra