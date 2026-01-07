#pragma once

#include <stdint.h>
#include <memory.h>

#ifdef _WIN32
#include <assert.h>
#else
#define assert(...)
#endif


static constexpr int tick_time = 10;  // In ms


struct Pixels {
    uint8_t br1[16];
    uint8_t br2[16];

    int get_br(int x, int y);
    void set_br(int x, int y, int br);
    void clear() {memset(this, 0, sizeof(*this));}
};

extern Pixels pixs;

enum Key {
    K_Up    = 1<<3,
    K_Down  = 1<<1,
    K_Left  = 1<<0,
    K_Right = 1<<2,
    K_Hit   = 1<<5,
    K_1     = 1<<7,
    K_2     = 1<<6,
    K_3     = 1<<4
};

////////////////////////////
// Functions implemeted by platform
uint8_t read_key();
void clr_keys(uint8_t keys);
uint32_t get_random();

///////////////////////////
// Main entry. Implemeted in common part
extern "C" void entry();
extern "C" void OurPlatformInit();
