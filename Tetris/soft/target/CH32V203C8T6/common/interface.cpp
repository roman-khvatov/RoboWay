#include "interface.h"
#include "arena.h"

Pixels pixs;
Arena arena;

int Pixels::get_br(int x, int y)
{
    return ((br1[y] >> x) & 1) + ((br2[y] >> x) & 1)*2;
}

void Pixels::set_br(int x, int y, int br)
{
    uint8_t& b1 = br1[y];
    uint8_t& b2 = br2[y];
    uint8_t mask = 1 << x;
    switch (br&3)
    {
        case 0: b1 &= ~mask; b2 &= ~mask; break;
        case 1: b1 |=  mask; b2 &= ~mask; break;
        case 2: b1 &= ~mask; b2 |=  mask; break;
        case 3: b1 |=  mask; b2 |=  mask; break;
    }
}

