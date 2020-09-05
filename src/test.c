//#include <dos.h>
//#include <conio.h>
#include <stdio.h>
#include <time.h>

#if 1
#include "main_test.h"
#endif

enum Notes {
    N_CS    = 0x016B,
    N_D     = 0x0181,
    N_DS    = 0x0198,
    N_E     = 0x01B0,
    N_F     = 0x01CA,
    N_FS    = 0x01E5,
    N_G     = 0x0202,
    N_GS    = 0x0220,
    N_A     = 0x0241,
    N_AS    = 0x0263,
    N_B     = 0x0287,
    N_C     = 0x02AE
};

enum Lengths {
    L_LEAST = 1,

    L_SIXT = L_LEAST,
    L_EIGT = L_SIXT * 2,
    L_QUART = L_EIGT * 2,
    L_HALF = L_QUART * 2,
    L_WHOLE = L_HALF * 2
};

struct OctaveNote {
    unsigned short octave;
    unsigned short note;
    unsigned long  duration;
};

struct OctaveNote song[] = {
    {0, 0,    L_QUART + L_EIGT},
    {3, N_B,  L_EIGT + L_EIGT},
    {4, N_CS, L_EIGT},
    {4, N_D,  L_EIGT},
    {4, N_A,  L_EIGT},
    {4, N_FS, L_QUART},
    {4, N_FS, L_EIGT},
    {4, N_E,  L_SIXT},
    {4, N_FS, L_SIXT + L_HALF + L_QUART},

    {4, N_E,  L_EIGT},
    {4, N_FS, L_EIGT},
    {4, N_A,  L_EIGT}, //
    {4, N_FS, L_EIGT},
    {4, N_CS, L_EIGT},
    {4, N_D,  L_EIGT},
    {4, N_CS, L_QUART},
    {4, N_CS, L_EIGT},
    {3, N_B,  L_SIXT},
    {3, N_FS, L_SIXT + L_HALF + L_QUART + L_EIGT},

    {3, N_B,  L_EIGT + L_EIGT},
    {4, N_CS, L_EIGT},
    {4, N_D,  L_EIGT},
    {4, N_A,  L_EIGT},
    {4, N_FS, L_QUART},
    {4, N_FS, L_EIGT},
    {4, N_E,  L_SIXT},
    {4, N_FS, L_SIXT + L_HALF + L_QUART},

    {4, N_E,  L_EIGT},
    {4, N_FS, L_EIGT},
    {4, N_A,  L_EIGT}, //
    {4, N_FS, L_EIGT},
    {4, N_A,  L_EIGT},
    {5, N_D,  L_EIGT},
    {5, N_CS, L_EIGT + L_SIXT},
    {4, N_B,  L_EIGT + L_SIXT},
    {4, N_FS, L_EIGT + L_QUART + L_EIGT},
    {4, 0,    L_EIGT}
};

struct OctaveNote song_bass[] = {
    {1, N_B,  L_WHOLE},
    {2, N_G,  L_QUART * 3},
    {0, 0,    L_EIGT},
    {2, N_D,  L_EIGT},
    {2, N_A,  L_EIGT},
    {2, N_FS, L_EIGT},
    {1, N_A,  L_WHOLE},

    {1, N_B,  L_QUART * 3},
    {0, 0,    L_EIGT},
    {1, N_FS, L_EIGT},
    {1, N_B,  L_EIGT},
    {2, N_E,  L_EIGT},
    {2, N_D,  L_EIGT}, //
    {2, N_CS, L_EIGT},
    {1, N_B,  L_EIGT + L_EIGT + L_HALF + L_QUART},

    {1, N_G,  L_QUART * 3},
    {0, 0,    L_EIGT},
    {2, N_D,  L_EIGT},
    {2, N_A,  L_EIGT},
    {2, N_B,  L_EIGT},
    {3, N_CS, L_HALF + L_EIGT},
    {3, N_CS, L_EIGT},
    {2, N_A,  L_EIGT},
    {2, N_E,  L_EIGT},
    {1, N_B,  L_WHOLE}
};

#if 0
#define WAIT_TIME 105
#else
#define WAIT_TIME 105000
#endif

struct Track {
    struct OctaveNote *notes;
    int playhead;
    int track_end;
    int time_left;
    unsigned short prev_note;
};

struct Track melody = {
    .notes = song,
    .track_end = sizeof(song) / sizeof(song[0])
};

struct Track bassline = {
    .notes = song_bass,
    .track_end = sizeof(song_bass) / sizeof(song_bass[0])
};

void play(int voice, unsigned short octave, unsigned short note);
void stop(int voice);
void wait(unsigned int time);

void play_track(struct Track *track, int channel)
{
    if(--track->time_left <= 0) {
        play(channel, track->notes[track->playhead].octave, track->notes[track->playhead].note);
        track->time_left = track->notes[track->playhead].duration;
        ++track->playhead;
    }
}

void play_song(void)
{
    //melody.time_left = melody.notes[0].duration;
    //bassline.time_left = bassline.notes[0].duration;

    //while(!kbhit() &&
    while(
          (melody.playhead < melody.track_end || bassline.playhead < bassline.track_end)
    ) {
        wait(1);
        play_track(&melody, 0);
        play_track(&bassline, 2);
    }
}

void output(unsigned char reg, unsigned char data)
{
#if 0
    outp(0x0388, reg);

    for(int i = 0; i < 6; ++i) {
        volatile unsigned char stat = inp(0x0388);
    }

    outp(0x389, data);
#else
    adlib_out(reg, data);
#endif
}

void clear(void)
{
    for(int i = 0x01; i <= 0xF5; ++i) {
        output(i, 0);
    }
}

void play(int voice, unsigned short octave, unsigned short note)
{
    unsigned char lsb = (unsigned char)(0x00FF & note);
    unsigned char msb = (1 << 5) | ((7 & octave) << 2) | ((0xFF00 & note) >> 8);
    output(0xA0 + voice, lsb);
    output(0xB0 + voice, msb);
    //printf("[%d] octave: %u note: %u lsb: 0x%2X msb: 0x%2X\n", voice, octave, note, lsb, msb);
}

void stop(int voice)
{
    output(0xB0 + voice, 0x00);
}

void wait(unsigned int time)
{
    clock_t start_time = clock();
    while(clock() < start_time + time * WAIT_TIME) {}
}

#if 0
int main(void)
#else
int test_play(void)
#endif
{
    clear();

    output(0x20, 0x01);
    output(0x40, 0x2A);
    output(0x60, 0xF0);
    output(0x80, 0x77);
    output(0x23, 0x01);
    output(0x43, 0x00);
    output(0x63, 0xF0);

    output(0x20 + 1, 0x01);
    output(0x40 + 1, 0x10);
    output(0x60 + 1, 0xF0);
    output(0x80 + 1, 0x77);
    output(0x23 + 1, 0x01);
    output(0x43 + 1, 0x05);
    output(0x63 + 1, 0xF0);

    output(0x20 + 2, 0x01);
    output(0x40 + 2, 0x10);
    output(0x60 + 2, 0xF0);
    output(0x80 + 2, 0x77);
    output(0x23 + 2, 0x01);
    output(0x43 + 2, 0x05);
    output(0x63 + 2, 0xF0);

    //output(0xA0, 0x98);
    //output(0xB0, 0x31);

    /*
    play(0, 4, N_DS);

    while(!kbhit()) {
    }
    */

    /*
    while(playhead < song_end && !kbhit()) {
        if(song[playhead].note == prev_note.note) {
            stop(0);
            stop(1);
        }
        if(song[playhead].note != 0) {
            play(0, song[playhead].octave, song[playhead].note);
            play(1, song[playhead].octave + 1, song[playhead].note);
        }

        wait(song[playhead].duration);

        playhead++;
        prev_note = song[playhead];
    }*/

    play_song();

    stop(0);
    stop(1);
    stop(2);
    //clear();

    return 0;
}
