#include "notes.h"
#include "instrument.h"
#include "main_test.h"

#include <cmath>
#include <cassert>

const KeyType note_key_type_table[N_TOTAL] = {
    [N_C]     = KEY_WHITE,
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
    [N_C]     = "C ",
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

#define HZ_TO_FNUM(hz, octave) (uint16_t)((hz) * pow(2, 20.0 - (octave)) / 49716)

static const uint16_t freq_table[N_TOTAL] = {
    [N_C]     = HZ_TO_FNUM(261.63, 4),
    [N_CS]    = HZ_TO_FNUM(277.18, 4),
    [N_D]     = HZ_TO_FNUM(293.66, 4),
    [N_DS]    = HZ_TO_FNUM(311.13, 4),
    [N_E]     = HZ_TO_FNUM(329.63, 4),
    [N_F]     = HZ_TO_FNUM(349.23, 4),
    [N_FS]    = HZ_TO_FNUM(369.99, 4),
    [N_G]     = HZ_TO_FNUM(391.99, 4),
    [N_GS]    = HZ_TO_FNUM(415.31, 4),
    [N_A]     = HZ_TO_FNUM(440.00, 4),
    [N_AS]    = HZ_TO_FNUM(466.16, 4),
    [N_B]     = HZ_TO_FNUM(493.88, 4),
};

static uint8_t reg_b0s[VOICE_COUNT];

static uint8_t register_of(uint8_t addr, uint8_t voice, uint8_t op)
{
    static const uint8_t offsets[2][9] = {
        {0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12}, // OP 1
        {0x03, 0x04, 0x05, 0x0B, 0x0C, 0x0D, 0x13, 0x14, 0x15}  // OP 2
    };

    return addr + offsets[op][voice];
}

void playback_registers_clear(void)
{
    for(int i = 0x01; i <= 0xF5; ++i) {
        adlib_out(i, 0);
    }
}

void playback_instrument_set(const Instrument &instr, int voice)
{
    adlib_out(0x01, 1 << 5);
    adlib_out(register_of(0x20, voice, 0), 0x01);
    adlib_out(register_of(0x40, voice, 0), (255 - instr.mod.volume) & ~((1 << 7) | (1 << 6)));
    adlib_out(register_of(0x60, voice, 0), (instr.mod.attack << 4) | instr.mod.decay);
    adlib_out(register_of(0x80, voice, 0), (instr.mod.sustain << 4) | instr.mod.release);
    adlib_out(register_of(0xE0, voice, 0), instr.mod.waveform);

    adlib_out(register_of(0x20, voice, 1), 0x01);
    adlib_out(register_of(0x40, voice, 1), (255 - instr.car.volume) & ~((1 << 7) | (1 << 6)));
    adlib_out(register_of(0x60, voice, 1), (instr.car.attack << 4) | instr.car.decay);
    adlib_out(register_of(0x80, voice, 1), (instr.car.sustain << 4) | instr.car.release);
    adlib_out(register_of(0xE0, voice, 1), instr.car.waveform);
}

void playback_stop(int voice)
{
    assert(voice < VOICE_COUNT);

    adlib_out(0xB0 + voice, reg_b0s[voice] & 0x1F);
}

void playback_play_note(int voice, Note note)
{
    assert(voice < VOICE_COUNT);
    assert(note.note < N_TOTAL);

    unsigned short freq = freq_table[note.note];
    unsigned short octave = note.octave;

    unsigned char lsb = (unsigned char)(0x00FF & freq);
    unsigned char msb = (1 << 5) | ((7 & octave) << 2) | ((0xFF00 & freq) >> 8);
    adlib_out(0xA0 + voice, lsb);
    adlib_out(0xB0 + voice, msb);
    reg_b0s[voice] = msb;
}
