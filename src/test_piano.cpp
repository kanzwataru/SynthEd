#include <cstdint>
#include <cstdlib>
#include <SDL2/SDL.h>
#include "main_test.h"
#include "extern/imgui/imgui.h"
#include "instrument.h"
#include "notes.h"
#include "playback.h"

#define HI(x) (N_TOTAL + (x))
static int keyboard_table[N_TOTAL * 2] = {
    [N_C]      = SDL_SCANCODE_Z,
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

    [HI(N_C)]  = SDL_SCANCODE_Q,
    [HI(N_CS)] = SDL_SCANCODE_2,
    [HI(N_D)]  = SDL_SCANCODE_W,
    [HI(N_DS)] = SDL_SCANCODE_3,
    [HI(N_E)]  = SDL_SCANCODE_E,
    [HI(N_F)]  = SDL_SCANCODE_R,
    [HI(N_FS)] = SDL_SCANCODE_5,
    [HI(N_G)]  = SDL_SCANCODE_T,
    [HI(N_GS)] = SDL_SCANCODE_6,
    [HI(N_A)]  = SDL_SCANCODE_Y,
    [HI(N_AS)] = SDL_SCANCODE_7,
    [HI(N_B)]  = SDL_SCANCODE_U,
};

static bool initialized = false;
static Instrument instrument;

static Note prev_notes[N_TOTAL];

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
        playback_registers_clear();
        for(int i = 0; i < N_TOTAL; ++i) {
            prev_notes[i].note = -1;
            prev_notes[i].octave = 4;
        }
        //dummy_instrument_set();
        initialized = true;
    }

    for(int i = 0; i < 9; ++i) {
        playback_instrument_set(instrument, i);
    }


    SDL_PumpEvents();
    const uint8_t *keys = SDL_GetKeyboardState(nullptr);
    struct Note notes[9];
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
                playback_play_note(i, notes[i]);
            }
            else {
                playback_stop(i);
            }
        }

        prev_notes[i].note = notes[i].note;
        prev_notes[i].octave = notes[i].octave;
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
