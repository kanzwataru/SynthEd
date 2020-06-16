#include <SDL2/SDL.h>
#include <cstdint>
#include <mutex>
#include "main_test.h"

#include "../extern/opl_dosbox/opl.h"

#define AMPLITUDE       28000
#define SAMPLE_RATE     44100
//#define SAMPLE_RATE     49716
#define RADIANS(x)      (2.0f * M_PI * (x))

static std::mutex audio_mutex;
static OPLChipClass opl(SAMPLE_RATE);

/*

struct Synth {
    size_t frame;
};

static void audio_callback(void *data, uint8_t *raw_buf, int bytes)
{
    Synth *synth = (Synth *)data;

    int16_t *buf = (int16_t *)raw_buf;
    int length = bytes / (sizeof *buf);

    for(int i = 0; i < length; ++i, synth->frame++) {
        float time = (float)synth->frame / (float)SAMPLE_RATE;

        float generator = sinf(2.0f * M_PI * 220.0f * time) * 0.35f;
        float modulator = sinf(4.0f * generator + 0.5f) * 0.35f;
        float mod3 = sinf(8.0f * modulator + 0.25f) * 0.75f;
        float mod4 = sinf(2.0f * mod3 + sinf(modulator * 0.5f) * 0.5f);
        float out = mod4;

        buf[i] = (int16_t)(AMPLITUDE * out);
    }
}
*/

void adlib_out(unsigned short addr, unsigned char val)
{
    std::lock_guard<std::mutex> lock(audio_mutex);

    opl.adlib_write(addr, val, 0);
}

static void audio_callback(void *data, uint8_t *raw_buf, int bytes)
{
    std::lock_guard<std::mutex> lock(audio_mutex);

    OPLChipClass *opl = (OPLChipClass *)data;
    opl->adlib_getsample((Bit16s *)raw_buf, bytes / 2);
}

static void song_play(OPLChipClass &opl)
{
    auto output = [&](uint8_t reg, uint8_t val) {
        opl.adlib_write(reg, val, 0);
    };

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


    output(0xA0, 0x98);
    output(0xB0, 0x31);
}

int main()
{
    if(0 != SDL_Init(SDL_INIT_AUDIO)) {
        SDL_Log(":%s", SDL_GetError());
        return 1;
    }

    int sample_num = 0;

    //struct Synth synth_data = {0};
    opl.adlib_init(SAMPLE_RATE, true);

    SDL_AudioSpec requested;
    requested.freq = SAMPLE_RATE;
    requested.format = AUDIO_S16SYS;
    requested.channels = 1;
    requested.samples = 2048;
    requested.callback = audio_callback;
    requested.userdata = &opl;

    SDL_AudioSpec got;
    if(0 != SDL_OpenAudio(&requested, &got))
        return 2;
    if(got.format != requested.format)
        return 3;

    SDL_PauseAudio(0);
    //song_play(opl);
    test_play();
    SDL_Delay(1000);
    SDL_PauseAudio(1);

    SDL_CloseAudio();

    return 0;
}
