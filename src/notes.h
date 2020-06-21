#pragma once
#include <stdint.h>

enum Notes {
    N_LC,
    N_CS,
    N_D,
    N_DS,
    N_E,
    N_F,
    N_FS,
    N_G,
    N_GS,
    N_A,
    N_AS,
    N_B,
    N_C,

    N_TOTAL
};

static const char *note_name_table[N_TOTAL] = {
    [N_LC]    = "C ",
    [N_CS]    = "C#",
    [N_D]     = "D ",
    [N_DS]    = "D#",
    [N_E]     = "E ",
    [N_F]     = "F ",
    [N_FS]    = "F#",
    [N_G]     = "G ",
    [N_GS]    = "G#",
    [N_A]     = "A ",
    [N_AS]    = "A#",
    [N_B]     = "B ",
    [N_C]     = "C "
};

struct Note {
    int note;
    int octave;
};
