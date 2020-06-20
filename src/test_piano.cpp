#include <cstdint>
#include <cstdlib>
#include <SDL2/SDL.h>
#include "main_test.h"
#include "extern/imgui/imgui.h"

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

#define HZ_TO_FNUM(hz, octave) (uint16_t)((hz) * pow(2, 20.0 - (octave)) / 49716)

static const uint16_t freq_table[N_TOTAL] = {
    [N_LC]    = HZ_TO_FNUM(261.63, 4),
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
    [N_C]     = HZ_TO_FNUM(523.25, 4)
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

#define HI(x) (N_TOTAL + (x))
static int keyboard_table[N_TOTAL * 2] = {
    [N_LC]     = SDL_SCANCODE_Z,
    [N_CS]     = SDL_SCANCODE_S,
    [N_D]      = SDL_SCANCODE_X,
    [N_DS]     = SDL_SCANCODE_D,
    [N_E]      = SDL_SCANCODE_C,
    [N_F]      = SDL_SCANCODE_V,
    [N_FS]     = SDL_SCANCODE_G,
    [N_G]      = SDL_SCANCODE_B,
    [N_GS]     = SDL_SCANCODE_H,
    [N_A]      = SDL_SCANCODE_N,
    [N_AS]     = SDL_SCANCODE_J,
    [N_B]      = SDL_SCANCODE_M,
    [N_C]      = SDL_SCANCODE_COMMA,

    [HI(N_LC)] = SDL_SCANCODE_Q,
    [HI(N_CS)] = SDL_SCANCODE_2,
    [HI(N_D)]  = SDL_SCANCODE_E,
    [HI(N_DS)] = SDL_SCANCODE_3,
    [HI(N_E)]  = SDL_SCANCODE_R,
    [HI(N_F)]  = SDL_SCANCODE_T,
    [HI(N_FS)] = SDL_SCANCODE_6,
    [HI(N_G)]  = SDL_SCANCODE_Y,
    [HI(N_GS)] = SDL_SCANCODE_7,
    [HI(N_A)]  = SDL_SCANCODE_U,
    [HI(N_AS)] = SDL_SCANCODE_8,
    [HI(N_B)]  = SDL_SCANCODE_I,
    [HI(N_C)]  = SDL_SCANCODE_O
};

enum Waveform {
    WAVE_SINE       = 0x00,
    WAVE_HALF_SINE  = 0x01,
    WAVE_ABS_SINE   = 0x02,
    WAVE_PULSE_SINE = 0x03
};

struct Operator {
    uint8_t attack;
    uint8_t decay;
    uint8_t sustain;
    uint8_t release;
    uint8_t volume;
    int     waveform;
};

struct Instrument {
    Operator mod;
    Operator car;

    Instrument() {
        mod.attack   = 0x0F;
        mod.decay    = 0x00;
        mod.sustain  = 0x07;
        mod.release  = 0x07;
        mod.volume   = 0x15;
        mod.waveform = WAVE_SINE;

        car.attack   = 0x0F;
        car.decay    = 0x00;
        car.sustain  = 0x07;
        car.release  = 0x07;
        car.volume   = 0x3F;
        car.waveform = WAVE_SINE;
    }
};

struct NoteInfo {
    int note = -1;
    int octave = 4;
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

static void ui_instrument_operator(Operator &op)
{
    const uint8_t single_min = 0x0;
    const uint8_t single_max = 0xF;
    const uint8_t vol_min = 0x00;
    const uint8_t vol_max = 0x3F;
    ImGui::SliderScalar("Attack",  ImGuiDataType_U8, &op.attack, &single_min, &single_max);
    ImGui::SliderScalar("Decay",   ImGuiDataType_U8, &op.decay, &single_min, &single_max);
    ImGui::SliderScalar("Sustain", ImGuiDataType_U8, &op.sustain, &single_min, &single_max);
    ImGui::SliderScalar("Release", ImGuiDataType_U8, &op.release, &single_min, &single_max);
    ImGui::SliderScalar("Volume",  ImGuiDataType_U8, &op.volume, &vol_min, &vol_max);
    ImGui::RadioButton("Sine",       &op.waveform, 0); ImGui::SameLine();
    ImGui::RadioButton("Half-Sine",  &op.waveform, 1);
    ImGui::RadioButton("Abs-Sine",   &op.waveform, 2); ImGui::SameLine();
    ImGui::RadioButton("Pulse-Sine", &op.waveform, 3);
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

    auto find = [&](int n, int o = -1) -> int {
        for(int i = 0; i < 9; ++i) {
            if(notes[i].note == n && (o == -1 || notes[i].octave == o)) {
                return i;
            }
        }

        return -1;
    };

    int octave = 4;

    for(int i = 0; i < N_TOTAL * 2; ++i) {
        if(keys[keyboard_table[i]]) {
            int key_note = i % N_TOTAL;
            int key_octave = i < N_TOTAL ? octave : octave + 1;
            int note_idx = find(key_note, key_octave);
            if(note_idx == -1) {
                int free_idx = find(-1);
                if(free_idx != -1) {
                    notes[free_idx].note = key_note;
                    notes[free_idx].octave = key_octave;
                }
            }
        }
    }

    for(int i = 0; i < 9; ++i) {
        int n = notes[i].octave > octave ? notes[i].note + N_TOTAL : notes[i].note;
        if(!keys[keyboard_table[n]]) {// || notes[i].octave > octave + 1 || notes[i].octave < octave) {
            notes[i].note = -1;
        }
    }

    for(int i = 0; i < 9; ++i) {
        if(notes[i].note != prev_notes[i].note) {
            if(notes[i].note != -1) {
                play(i, notes[i].octave, freq_table[notes[i].note]);
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

    ImGui::Separator();
    ImGui::Columns(2, "Instrument", true);
    ImGui::PushID("#modulator");
    ImGui::LabelText("", "Modulator");
    ui_instrument_operator(instrument.mod);
    ImGui::PopID();
    ImGui::NextColumn();
    ImGui::PushID("#carrier");
    ImGui::LabelText("", "Carrier");
    ui_instrument_operator(instrument.car);
    ImGui::PopID();
    ImGui::End();

    printf("\n");
    for(int i = 0; i < 9; ++i) {
        printf("%d %d\n", prev_notes[i].note, notes[i].note);
    }
}
