#pragma once

#include "../Core/GlobalState.h"
#include <array>

namespace airchestra {

constexpr int GUITAR_STRINGS[6] = { 40, 45, 50, 55, 59, 64 };

struct BarreShape {
    int offsets[6];
};

constexpr BarreShape E_SHAPES[7] = {
    {0, 2, 2, 1, 0, 0},    // 0: Major
    {0, 2, 2, 0, 0, 0},    // 1: Minor
    {0, 2, 0, 1, 0, 0},    // 2: Dom7
    {0, 2, 1, 1, 0, 0},    // 3: Maj7
    {0, 2, 0, 0, 0, 0},    // 4: Min7
    {0, 2, 4, -1, 0, 0},   // 5: Sus2
    {0, 2, 2, 2, 0, 0}     // 6: Sus4
};

constexpr BarreShape A_SHAPES[7] = {
    {-1, 0, 2, 2, 2, 0},   // 0: Major
    {-1, 0, 2, 2, 1, 0},   // 1: Minor
    {-1, 0, 2, 0, 2, 0},   // 2: Dom7
    {-1, 0, 2, 1, 2, 0},   // 3: Maj7
    {-1, 0, 2, 0, 1, 0},   // 4: Min7
    {-1, 0, 2, 2, 0, 0},   // 5: Sus2
    {-1, 0, 2, 2, 3, 0}    // 6: Sus4
};

constexpr BarreShape D_SHAPES[7] = {
    {-1, -1, 0, 2, 3, 2},  // 0: Major
    {-1, -1, 0, 2, 3, 1},  // 1: Minor
    {-1, -1, 0, 2, 1, 2},  // 2: Dom7
    {-1, -1, 0, 2, 2, 2},  // 3: Maj7
    {-1, -1, 0, 2, 1, 1},  // 4: Min7
    {-1, -1, 0, 2, 3, 0},  // 5: Sus2
    {-1, -1, 0, 2, 3, 3}   // 6: Sus4
};

enum ShapeType { SHAPE_E, SHAPE_A, SHAPE_D };

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

// Support for 12 Roots (C through B)
struct GuitarChordDb {
    int chords[12][7][3][6];

    constexpr GuitarChordDb() : chords{} {
        // Base Open Voicings (mapped to the 7 natural roots)
        int origC[7][6] = { { -1, 48, 52, 55, 60, 64 }, { -1, 48, 55, 60, 63, 67 }, { -1, 48, 52, 55, 58, 64 }, { -1, 48, 52, 55, 59, 64 }, { -1, 48, 55, 58, 63, 67 }, { -1, 48, 55, 60, 62, 67 }, { -1, 48, 55, 60, 65, 67 } };
        int origD[7][6] = { { -1, -1, 50, 57, 62, 66 }, { -1, -1, 50, 57, 62, 65 }, { -1, -1, 50, 57, 60, 66 }, { -1, -1, 50, 57, 61, 66 }, { -1, -1, 50, 57, 60, 65 }, { -1, -1, 50, 57, 62, 64 }, { -1, -1, 50, 57, 62, 67 } };
        int origE[7][6] = { { 40, 47, 52, 56, 59, 64 }, { 40, 47, 52, 55, 59, 64 }, { 40, 47, 50, 56, 59, 64 }, { 40, 47, 51, 56, 59, 64 }, { 40, 47, 52, 55, 62, 64 }, { 40, 47, 54, -1, 59, 64 }, { 40, 47, 52, 57, 59, 64 } };
        int origF[7][6] = { { 41, 48, 53, 57, 60, 65 }, { 41, 48, 53, 56, 60, 65 }, { 41, 48, 51, 57, 60, 65 }, { -1, -1, 53, 57, 60, 64 }, { 41, 48, 51, 56, 60, 65 }, { 41, 48, 53, 55, 60, 65 }, { 41, 48, 53, 58, 60, 65 } };
        int origG[7][6] = { { 43, 47, 50, 55, 59, 67 }, { 43, 50, 55, 58, 62, 67 }, { 43, 47, 50, 55, 59, 65 }, { 43, 47, 50, 55, 59, 66 }, { 43, 50, 53, 58, 62, 67 }, { 43, 45, 50, 55, 62, 67 }, { 43, 48, 50, 55, 60, 67 } };
        int origA[7][6] = { { -1, 45, 52, 57, 61, 64 }, { -1, 45, 52, 57, 60, 64 }, { -1, 45, 52, 55, 61, 64 }, { -1, 45, 52, 56, 61, 64 }, { -1, 45, 52, 55, 60, 64 }, { -1, 45, 52, 57, 59, 64 }, { -1, 45, 52, 57, 62, 64 } };
        int origB[7][6] = { { -1, 47, 54, 59, 63, 66 }, { -1, 47, 54, 59, 62, 66 }, { -1, 47, 51, 57, 59, 66 }, { -1, 47, 54, 58, 63, 66 }, { -1, 47, 54, 57, 62, 66 }, { -1, 47, 54, 59, 61, 66 }, { -1, 47, 54, 59, 64, 66 } };
        int (*origs[7])[6] = {origC, origD, origE, origF, origG, origA, origB};

        // Maps all 12 roots to their natural base and a 'sharp' flag
        int baseRoot[12] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};
        bool isSharp[12] = {false, true, false, true, false, false, true, false, true, false, true, false};

        ShapeType types[12][2] = {
            {SHAPE_E, SHAPE_A}, {SHAPE_E, SHAPE_A}, {SHAPE_A, SHAPE_E}, {SHAPE_A, SHAPE_E}, 
            {SHAPE_A, SHAPE_D}, {SHAPE_A, SHAPE_D}, {SHAPE_A, SHAPE_E}, {SHAPE_E, SHAPE_A}, 
            {SHAPE_E, SHAPE_A}, {SHAPE_E, SHAPE_A}, {SHAPE_E, SHAPE_A}, {SHAPE_A, SHAPE_E}
        };

        int frets[12][2] = {
            {8, 15}, {9, 16}, {5, 10}, {6, 11}, {7, 14}, {8, 15}, 
            {9, 14}, {3, 10}, {4, 11}, {5, 12}, {6, 13}, {2, 7}
        };

        for(int r = 0; r < 12; ++r) {
            for(int v = 0; v < 3; ++v) {
                for(int q = 0; q < 7; ++q) {
                    if (v == 0) {
                        int br = baseRoot[r];
                        for(int s = 0; s < 6; ++s) {
                            int note = origs[br][q][s];
                            // Synthesize sharp chords by moving the fretted notes up 1 fret
                            if (note != -1 && isSharp[r]) note += 1;
                            chords[r][q][v][s] = note;
                        }
                    } else {
                        auto c = applyShape(types[r][v-1], q, frets[r][v-1]);
                        for(int s = 0; s < 6; ++s) chords[r][q][v][s] = c[s];
                    }
                }
            }
        }
    }
};

constexpr GuitarChordDb CHORD_DB;

inline const int* getGuitarChord(GuitarChordRoot root, GuitarChordQuality quality, int voicing = 0)
{
    return CHORD_DB.chords[static_cast<int>(root)][static_cast<int>(quality)][voicing];
}

} // namespace airchestra