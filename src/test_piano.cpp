#include <cstdint>
#include <SDL2/SDL.h>
#include "main_test.h"
#include "extern/imgui/imgui.h"

enum Notes {
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

static const uint16_t freq_table[N_TOTAL] = {
    [N_CS]    = 0x016B,
    [N_D]     = 0x0181,
    [N_DS]    = 0x0198,
    [N_E]     = 0x01B0,
    [N_F]     = 0x01CA,
    [N_FS]    = 0x01E5,
    [N_G]     = 0x0202,
    [N_GS]    = 0x0220,
    [N_A]     = 0x0241,
    [N_AS]    = 0x0263,
    [N_B]     = 0x0287,
    [N_C]     = 0x02AE
};

static const char *note_name_table[N_TOTAL] = {
    [N_CS]    = "C#",
    [N_D]     = "D",
    [N_DS]    = "D#",
    [N_E]     = "E",
    [N_F]     = "F",
    [N_FS]    = "F#",
    [N_G]     = "G",
    [N_GS]    = "G#",
    [N_A]     = "A",
    [N_AS]    = "A#",
    [N_B]     = "B",
    [N_C]     = "C"
};

static int keyboard_table[N_TOTAL] = {
    [N_CS]    = SDL_SCANCODE_S,
    [N_D]     = SDL_SCANCODE_X,
    [N_DS]    = SDL_SCANCODE_D,
    [N_E]     = SDL_SCANCODE_C,
    [N_F]     = SDL_SCANCODE_V,
    [N_FS]    = SDL_SCANCODE_G,
    [N_G]     = SDL_SCANCODE_B,
    [N_GS]    = SDL_SCANCODE_H,
    [N_A]     = SDL_SCANCODE_N,
    [N_AS]    = SDL_SCANCODE_J,
    [N_B]     = SDL_SCANCODE_M,
    [N_C]     = SDL_SCANCODE_COMMA
};

static bool initialized = false;
static int  prev_note_held = -1;

static void registers_clear(void)
{
    for(int i = 0x01; i <= 0xF5; ++i) {
        adlib_out(i, 0);
    }
}

static void dummy_instrument_set()
{
    adlib_out(0x20, 0x01);
    adlib_out(0x40, 0x2A);
    adlib_out(0x60, 0xF0);
    adlib_out(0x80, 0x77);
    adlib_out(0x23, 0x01);
    adlib_out(0x43, 0x00);
    adlib_out(0x63, 0xF0);

    adlib_out(0x20 + 1, 0x01);
    adlib_out(0x40 + 1, 0x10);
    adlib_out(0x60 + 1, 0xF0);
    adlib_out(0x80 + 1, 0x77);
    adlib_out(0x23 + 1, 0x01);
    adlib_out(0x43 + 1, 0x05);
    adlib_out(0x63 + 1, 0xF0);

    adlib_out(0x20 + 2, 0x01);
    adlib_out(0x40 + 2, 0x10);
    adlib_out(0x60 + 2, 0xF0);
    adlib_out(0x80 + 2, 0x77);
    adlib_out(0x23 + 2, 0x01);
    adlib_out(0x43 + 2, 0x05);
    adlib_out(0x63 + 2, 0xF0);
}

static void stop(int voice)
{
    adlib_out(0xB0 + voice, 0x00);
}

static void play(int voice, unsigned short octave, unsigned short note)
{
    unsigned char lsb = (unsigned char)(0x00FF & note);
    unsigned char msb = (1 << 5) | ((7 & octave) << 2) | ((0xFF00 & note) >> 8);
    adlib_out(0xA0 + voice, lsb);
    adlib_out(0xB0 + voice, msb);
    //printf("[%d] octave: %u note: %u lsb: 0x%2X msb: 0x%2X\n", voice, octave, note, lsb, msb);
}

void test_piano()
{
    if(!initialized) {
        registers_clear();
        dummy_instrument_set();
        initialized = true;
    }

    SDL_PumpEvents();
    const uint8_t *keys = SDL_GetKeyboardState(nullptr);
    int note_held = -1;

    for(int i = 0; i < N_TOTAL; ++i) {
        if(keys[keyboard_table[i]]) {
            note_held = i;
        }
    }
    const char *key_label = note_held == -1 ? "None" : note_name_table[note_held];

    ImGui::Begin("Piano");
    ImGui::LabelText("Keys Held", ": %s", key_label);
    ImGui::End();

    if(note_held != prev_note_held) {
        if(note_held != -1) {
            play(0, 4, freq_table[note_held]);
        }
        else {
            stop(0);
        }
    }

    prev_note_held = note_held;
}
