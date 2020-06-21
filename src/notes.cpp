#include "notes.h"

const KeyType note_key_type_table[N_TOTAL] = {
    [N_C]    = KEY_WHITE,
    [N_CS]    = KEY_BLACK,
    [N_D]     = KEY_WHITE,
    [N_DS]    = KEY_BLACK,
    [N_E]     = KEY_WHITE,
    [N_F]     = KEY_WHITE,
    [N_FS]    = KEY_BLACK,
    [N_G]     = KEY_WHITE,
    [N_GS]    = KEY_BLACK,
    [N_A]     = KEY_WHITE,
    [N_AS]    = KEY_BLACK,
    [N_B]     = KEY_WHITE,
};

const char *note_name_table[N_TOTAL] = {
    [N_C]    = "C ",
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
};
