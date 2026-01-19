#include "sprite.h"
#include "spr_defs.h"
#include "timer.h"
#include "arena.h"

#include "spi.arr.inc"


constexpr int CycleTime = 2;
constexpr int TotalLevels = 6;
constexpr int LevelTreshold = 5;

struct LevelSetup {
    int max_sps;    // Maximum number of spaseship
    int sps_delta;  // Interval (in rows) between sps
};

const LevelSetup levels[TotalLevels] = {
    {1, 5},
    {2, 4},
    {3, 3},
    {4, 3},
    {6, 1}
};

class Invation {
    Timer t;
    Sprite spr = Sprite_platform;
    int phase=0;
    int level = 0;
    int sps_eaten = 0;
    int last_row_count = 0;
    int platform_pos = 4;

    void show();
    void move_bullets();
    bool move_sps();
    void chk_bullets();
    bool chk_platform();
    void fire();
    void move_platform(int delta);
};


void Invation::show()
{
    for ( int i = 0; i < 16; ++i )
    {
        pixs.br1[i] = arena.invation.bullets[i] | arena.invation.spsheeps[i];
        pixs.br2[i] = arena.invation.bullets[i];
    }
    spr.place(platform_pos, 14, phase);
}

void Invation::move_bullets()
{
    memmove(arena.invation.bullets, arena.invation.bullets+1, 15);
    arena.invation.bullets[13] = 0;
    chk_bullets();
}

bool Invation::move_sps()
{
    memmove(arena.invation.spsheeps+1, arena.invation.spsheeps, 15);
    arena.invation.spsheeps[0] = 0;
    chk_bullets();
    last_row_count++;
    return chk_platform();
}

void Invation::chk_bullets()
{
    for ( int i = 0; i < 16; ++i )
    {
        auto & a1 = arena.invation.bullets[i];
        auto & a2 = arena.invation.spsheeps[i];
        auto mask = a1 & a2;
        if ( mask )
        {
            a1 &= ~mask;
            a2 &= ~mask;
            ++sps_eaten;
        }
    }
}

bool Invation::chk_platform()
{
    int mask = 1<<platform_pos;
    arena.invation.spsheeps[14] &= ~mask;
    mask = 7 << (platform_pos+1);
    return (arena.invation.spsheeps[15] & mask) != 0;
}

void Invation::fire()
{
    int mask = 1 << platform_pos;
    if (arena.invation.spsheeps[13] & mask ) {arena.invation.spsheeps[13] &= ~mask; ++sps_eaten;}
    else arena.invation.bullets[13] |= mask;
}

void Invation::move_platform(int delta)
{
    int new_pp = platform_pos+delta;
    if (new_pp < 1 || new_pp > 6) return;
    int nxt_mask = 1 << new_pp;
    if (nxt_mask & arena.invation.spsheeps[13]) {arena.invation.spsheeps[13] &= ~nxt_mask ; ++sps_eaten;}
    if (delta == 1) nxt_mask >>= 1; else nxt_mask <<= 1;
    if (nxt_mask & arena.invation.spsheeps[14]) {arena.invation.spsheeps[14] &= ~nxt_mask ; ++sps_eaten;}
    platform_pos = new_pp;
}

void invation_game()
{}
