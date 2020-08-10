#pragma once
#include "notes.h"
#include "instrument.h"

void playback_registers_clear(void);
void playback_instrument_set(const Instrument &instr, int voice);
void playback_play_note(int voice, Note note);
void playback_stop(int voice);
