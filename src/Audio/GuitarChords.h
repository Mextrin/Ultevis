#pragma once

#include <string>
#include <map>

namespace airchestra {

// Represents a single guitar chord.
// The array holds 6 MIDI notes, representing the strings from Lowest (E) to Highest (e).
// A value of -1 means the string is muted/not played in this chord.
struct GuitarChord {
    int strings[6];
};

// Standard Guitar Tuning (Standard E): 
// Low E (40), A (45), D (50), G (55), B (59), High E (64)

const std::map<std::string, GuitarChord> CHORD_LIBRARY = {
    // --- MAJOR CHORDS ---

    // C Major (x 3 2 0 1 0)
    {"C_Major", {{ -1, 48, 52, 55, 60, 64 }}},

    // D Major (x x 0 2 3 2)
    {"D_Major", {{ -1, -1, 50, 57, 62, 66 }}},
    
    // E Major (0 2 2 1 0 0)
    {"E_Major", {{ 40, 47, 52, 56, 59, 64 }}},

    // F Major (Barre: 1 3 3 2 1 1)
    {"F_Major", {{ 41, 48, 53, 57, 60, 65 }}},

    // G Major (3 2 0 0 0 3)
    {"G_Major", {{ 43, 47, 50, 55, 59, 67 }}},
    
    // A Major (x 0 2 2 2 0)
    {"A_Major", {{ -1, 45, 52, 57, 61, 64 }}},
    
    // B Major (x 2 4 4 4 2)
    {"B_Major", {{ -1, 47, 54, 59, 63, 66 }}},

    // --- MINOR CHORDS ---

    // C Minor (Barre: x 3 5 5 4 3)
    {"C_Minor", {{ -1, 48, 55, 60, 63, 67 }}},

    // D Minor (x x 0 2 3 1)
    {"D_Minor", {{ -1, -1, 50, 57, 62, 65 }}},

    // E Minor (0 2 2 0 0 0)
    {"E_Minor", {{ 40, 47, 52, 55, 59, 64 }}},

    // F Minor (Barre: 1 3 3 1 1 1)
    {"F_Minor", {{ 41, 48, 53, 56, 60, 65 }}},

    // G Minor (Barre: 3 5 5 3 3 3)
    {"G_Minor", {{ 43, 50, 55, 58, 62, 67 }}},
    
    // A Minor (x 0 2 2 1 0)
    {"A_Minor", {{ -1, 45, 52, 57, 60, 64 }}},
    
    // B Minor (Barre: x 2 4 4 3 2)
    {"B_Minor", {{ -1, 47, 54, 59, 62, 66 }}},


    // --- 7TH CHORDS ---

    // C7 (x 3 2 3 1 0)
    {"C7", {{ -1, 48, 52, 58, 60, 64 }}},

    // D7 (x x 0 2 1 2)
    {"D7", {{ -1, -1, 50, 57, 60, 66 }}},

    // E7 (0 2 0 1 0 0)
    {"E7", {{ 40, 47, 50, 56, 59, 64 }}},

    // F7 (1 3 1 2 1 1)
    {"F7", {{ 41, 48, 51, 57, 60, 65 }}},

    // G7 (3 2 0 0 0 1)
    {"G7", {{ 43, 47, 50, 55, 59, 65 }}},
    
    // A7 (x 0 2 0 2 0)
    {"A7", {{ -1, 45, 52, 55, 61, 64 }}},

    // B7 (x 2 1 2 0 2)
    {"B7", {{ -1, 47, 51, 57, 59, 66 }}},

    // --- Maj7 CHORDS ---

    // C Maj7 (x 3 2 0 0 0)
    {"C_Maj7", {{ -1, 48, 52, 55, 59, 64 }}},

    // D Maj7 (x x 0 2 2 2)
    {"D_Maj7", {{ -1, -1, 50, 57, 61, 66 }}},
    
    // E Maj7 (0 2 1 1 0 0)
    {"E_Maj7", {{ 40, 47, 51, 56, 59, 64 }}},

    // F Maj7 (x x 3 2 1 0)
    {"F_Maj7", {{ -1, -1, 53, 57, 60, 64 }}},

    // G Maj7 (3 2 0 0 0 2)
    {"G_Maj7", {{ 43, 47, 50, 55, 59, 66 }}},
    
    // A Maj7 (x 0 2 1 2 0)
    {"A_Maj7", {{ -1, 45, 52, 56, 61, 64 }}},
    
    // B Maj7 (x 2 4 3 4 2)
    {"B_Maj7", {{ -1, 47, 54, 58, 63, 66 }}},

    // -- Min7 CHORDS --- 

    // C Minor 7 (Barre: x 3 5 3 4 3)
    {"C_Min7", {{ -1, 48, 55, 58, 63, 67 }}},

    // D Minor 7 (x x 0 2 1 1)
    {"D_Min7", {{ -1, -1, 50, 57, 60, 65 }}},

    // E Minor 7 (0 2 2 0 3 0)
    {"E_Min7", {{ 40, 47, 52, 55, 62, 64 }}},

    // F Minor 7 (Barre: 1 3 1 1 1 1)
    {"F_Min7", {{ 41, 48, 51, 56, 60, 65 }}},

    // G Minor 7 (Barre: 3 5 3 3 3 3)
    {"G_Min7", {{ 43, 50, 53, 58, 62, 67 }}},
    
    // A Minor 7 (x 0 2 0 1 0)
    {"A_Min7", {{ -1, 45, 52, 55, 60, 64 }}},
    
    // B Minor 7 (Barre: x 2 4 2 3 2)
    {"B_Min7", {{ -1, 47, 54, 57, 62, 66 }}},

    // --- SUS2 CHORDS ---

    // C sus2 (Barre: x 3 5 5 3 3)
    {"C_sus2", {{ -1, 48, 55, 60, 62, 67 }}},

    // D sus2 (x x 0 2 3 0)
    {"D_sus2", {{ -1, -1, 50, 57, 62, 64 }}},

    // E sus2 (0 2 4 1 0 0)
    {"E_sus2", {{ 40, 47, 54, 56, 59, 64 }}},

    // F sus2 (1 3 3 0 1 1 - Open G string variant)
    {"F_sus2", {{ 41, 48, 53, 55, 60, 65 }}},

    // G sus2 (3 0 0 0 3 3)
    {"G_sus2", {{ 43, 45, 50, 55, 62, 67 }}},
    
    // A sus2 (x 0 2 2 0 0)
    {"A_sus2", {{ -1, 45, 52, 57, 59, 64 }}},
    
    // B sus2 (Barre: x 2 4 4 2 2)
    {"B_sus2", {{ -1, 47, 54, 59, 61, 66 }}},

    // --- SUS4 CHORDS ---

    // C sus4 (Barre: x 3 5 5 6 3)
    {"C_sus4", {{ -1, 48, 55, 60, 65, 67 }}},

    // D sus4 (x x 0 2 3 3)
    {"D_sus4", {{ -1, -1, 50, 57, 62, 67 }}},

    // E sus4 (0 2 2 2 0 0)
    {"E_sus4", {{ 40, 47, 52, 57, 59, 64 }}},

    // F sus4 (Barre: 1 3 3 3 1 1)
    {"F_sus4", {{ 41, 48, 53, 58, 60, 65 }}},

    // G sus4 (3 3 0 0 1 3)
    {"G_sus4", {{ 43, 48, 50, 55, 60, 67 }}},
    
    // A sus4 (x 0 2 2 3 0)
    {"A_sus4", {{ -1, 45, 52, 57, 62, 64 }}},
    
    // B sus4 (Barre: x 2 4 4 5 2)
    {"B_sus4", {{ -1, 47, 54, 59, 64, 66 }}}

};

} // namespace airchestra