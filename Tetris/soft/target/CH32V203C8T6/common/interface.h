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
    K_Up =1,
    K_Down = 2,
    K_Left = 4,
    K_Right = 8,
    K_Hit = 16,
    K_1 = 32,
    K_2 = 64,
    K_3 = 128
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
