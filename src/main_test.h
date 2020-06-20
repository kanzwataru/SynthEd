#pragma once

#ifdef __cplusplus
    #define EXPORT extern "C"
#else
    #define EXPORT
#endif

EXPORT void adlib_out(unsigned short addr, unsigned char val);
EXPORT int  test_play(void);
EXPORT void test_piano(void);
EXPORT void test_editor(void);
