#pragma once
#include <stdint.h>

enum Notes {
    N_C,
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

    N_TOTAL
};

enum KeyType {
    KEY_WHITE = 0,
    KEY_BLACK = 1
};

extern const KeyType note_key_type_table[N_TOTAL];
extern const char *note_name_table[N_TOTAL];

struct Note {
    int note;
    int octave;
};
