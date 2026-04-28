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
    
    // E Major (0 2 2 1 0 0)
    {"E_Major", {{ 40, 47, 52, 56, 59, 64 }}},
    
    // A Major (x 0 2 2 2 0)
    {"A_Major", {{ -1, 45, 52, 57, 61, 64 }}},
    
    // D Major (x x 0 2 3 2)
    {"D_Major", {{ -1, -1, 50, 54, 57, 62 }}},
    
    // G Major (3 2 0 0 0 3)
    {"G_Major", {{ 43, 47, 50, 55, 59, 67 }}},
    
    // C Major (x 3 2 0 1 0)
    {"C_Major", {{ -1, 48, 52, 55, 60, 64 }}},
    
    // F Major (Barre: 1 3 3 2 1 1)
    {"F_Major", {{ 41, 48, 53, 57, 60, 65 }}},


    // --- MINOR CHORDS ---

    // E Minor (0 2 2 0 0 0)
    {"E_Minor", {{ 40, 47, 52, 55, 59, 64 }}},
    
    // A Minor (x 0 2 2 1 0)
    {"A_Minor", {{ -1, 45, 52, 57, 60, 64 }}},
    
    // D Minor (x x 0 2 3 1)
    {"D_Minor", {{ -1, -1, 50, 57, 60, 65 }}},
    
    // B Minor (Barre: x 2 4 4 3 2)
    {"B_Minor", {{ -1, 47, 54, 59, 62, 66 }}},


    // --- 7TH CHORDS (Great for Blues/Jazz) ---

    // G7 (3 2 0 0 0 1)
    {"G7", {{ 43, 47, 50, 55, 59, 65 }}},
    
    // D7 (x x 0 2 1 2)
    {"D7", {{ -1, -1, 50, 57, 60, 62 }}},
    
    // A7 (x 0 2 0 2 0)
    {"A7", {{ -1, 45, 52, 55, 61, 64 }}}
};

} // namespace airchestra