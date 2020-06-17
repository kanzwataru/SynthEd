#include <cstdint>
#include <cstdlib>
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

struct Operator {
    uint8_t attack;
    uint8_t decay;
    uint8_t sustain;
    uint8_t release;
};

struct Instrument {
    Operator mod;
    Operator car;

    Instrument() {
        //TODO: I want C99 designated initializers instead of constructors!
        mod.attack  = 0xF;
        mod.decay   = 0x0;
        mod.sustain = 0x7;
        mod.release = 0x7;

        car.attack  = 0xF;
        car.decay   = 0x0;
        car.sustain = 0x7;
        car.release = 0x7;
    }
};

struct NoteInfo {
    int note = -1;
    uint8_t reg_b0 = 0;
};

static bool initialized = false;
static Instrument instrument;
static NoteInfo prev_notes[9];

static void registers_clear(void)
{
    for(int i = 0x01; i <= 0xF5; ++i) {
        adlib_out(i, 0);
    }
}

static uint8_t register_of(uint8_t addr, uint8_t voice, uint8_t op)
{
    static const uint8_t offsets[2][9] = {
        {0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12}, // OP 1
        {0x03, 0x04, 0x05, 0x0B, 0x0C, 0x0D, 0x13, 0x14, 0x15}  // OP 2
    };

    return addr + offsets[op][voice];
}

static void instrument_set(const Instrument &instr, int voice)
{
    adlib_out(register_of(0x20, voice, 0), 0x01);
    adlib_out(register_of(0x40, voice, 0), 0x2A);
    adlib_out(register_of(0x60, voice, 0), (instr.mod.attack << 4) | instr.mod.decay);
    adlib_out(register_of(0x80, voice, 0), (instr.mod.sustain << 4) | instr.mod.release);

    adlib_out(register_of(0x20, voice, 1), 0x01);
    adlib_out(register_of(0x40, voice, 1), 0x00);
    adlib_out(register_of(0x60, voice, 1), (instr.car.attack << 4) | instr.car.decay);
    adlib_out(register_of(0x80, voice, 1), (instr.car.sustain << 4) | instr.car.release);
}

#if 0
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
#endif

static void stop(int voice)
{
    //adlib_out(0xB0 + voice, 0x00);
    adlib_out(0xB0 + voice, prev_notes[voice].reg_b0 & 0x1F);
}

static void play(int voice, unsigned short octave, unsigned short note)
{
    unsigned char lsb = (unsigned char)(0x00FF & note);
    unsigned char msb = (1 << 5) | ((7 & octave) << 2) | ((0xFF00 & note) >> 8);
    adlib_out(0xA0 + voice, lsb);
    adlib_out(0xB0 + voice, msb);
    prev_notes[voice].reg_b0 = msb;
    //printf("[%d] octave: %u note: %u lsb: 0x%2X msb: 0x%2X\n", voice, octave, note, lsb, msb);
}

void test_piano()
{
    if(!initialized) {
        registers_clear();
        //dummy_instrument_set();
        initialized = true;
    }

    for(int i = 0; i < 9; ++i) {
        instrument_set(instrument, i);
    }


    SDL_PumpEvents();
    const uint8_t *keys = SDL_GetKeyboardState(nullptr);
    struct NoteInfo notes[9];
    memcpy(notes, prev_notes, sizeof(notes));

    auto find = [&](int n) -> int {
        for(int i = 0; i < 9; ++i) {
            if(notes[i].note == n) {
                return i;
            }
        }

        return -1;
    };

    for(int i = 0; i < N_TOTAL; ++i) {
        if(keys[keyboard_table[i]]) {
            int note_idx = find(i);
            if(note_idx == -1) {
                int free_idx = find(-1);
                if(free_idx != -1) {
                    notes[free_idx].note = i;
                }
            }
        }
    }

    for(int i = 0; i < 9; ++i) {
        if(!keys[keyboard_table[notes[i].note]]) {
            notes[i].note = -1;
        }
    }

    for(int i = 0; i < 9; ++i) {
        if(notes[i].note != prev_notes[i].note) {
            if(notes[i].note != -1) {
                play(i, 4, freq_table[notes[i].note]);
            }
            else {
                stop(i);
            }
        }

        prev_notes[i].note = notes[i].note;
    }

    auto label_of = [&](int i) { return notes[i].note == -1 ? "__" : note_name_table[notes[i].note]; };

    ImGui::Begin("Piano");
    ImGui::LabelText("Keys Held", ": %s %s %s %s %s %s %s %s %s",
                     label_of(0), label_of(1), label_of(2), label_of(3), label_of(4), label_of(5), label_of(6), label_of(7), label_of(8));

    const uint8_t single_min = 0x0;
    const uint8_t single_max = 0xF;
    ImGui::Separator();
    ImGui::Columns(2, "Instrument", true);
    ImGui::PushID("#modulator");
    ImGui::LabelText("", "Modulator");
    ImGui::SliderScalar("Attack",  ImGuiDataType_U8, &instrument.mod.attack, &single_min, &single_max);
    ImGui::SliderScalar("Decay",   ImGuiDataType_U8, &instrument.mod.decay, &single_min, &single_max);
    ImGui::SliderScalar("Sustain", ImGuiDataType_U8, &instrument.mod.sustain, &single_min, &single_max);
    ImGui::SliderScalar("Release", ImGuiDataType_U8, &instrument.mod.release, &single_min, &single_max);
    ImGui::PopID();
    ImGui::NextColumn();
    ImGui::PushID("#carrier");
    ImGui::LabelText("", "Carrier");
    ImGui::SliderScalar("Attack",  ImGuiDataType_U8, &instrument.car.attack, &single_min, &single_max);
    ImGui::SliderScalar("Decay",   ImGuiDataType_U8, &instrument.car.decay, &single_min, &single_max);
    ImGui::SliderScalar("Sustain", ImGuiDataType_U8, &instrument.car.sustain, &single_min, &single_max);
    ImGui::SliderScalar("Release", ImGuiDataType_U8, &instrument.car.release, &single_min, &single_max);
    ImGui::PopID();
    ImGui::End();

    printf("\n");
    for(int i = 0; i < 9; ++i) {
        printf("%d %d\n", prev_notes[i].note, notes[i].note);
    }
}
