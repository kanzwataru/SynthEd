#pragma once
#include <stdint.h>

#define VOICE_COUNT 9

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
