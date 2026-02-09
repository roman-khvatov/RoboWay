#include "sprite.h"
#include "spr_defs.h"
#include "timer.h"
#include "arena.h"

struct ShIdx {
    uint16_t start;
    uint16_t length;
};

constexpr uint16_t operator ""_sh(const char* data, size_t length)
{
    uint16_t result = 0;
    uint16_t mask = 0x0101;
    for ( int idx = 0; idx < length; ++idx, mask <<= 1 )
    {
        switch ( *data++ )
        {
            case '.': result |= mask & 0xFF; break;
            case '+': result |= mask & 0xFF00; break;
            case '*': result |= mask; break;
        }
    }
    return result;
}

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
    {2, 5},
    {2, 4},
    {2, 3},
    {3, 3}
};

class Invation {
    Timer t, t2;
    Sprite spr = Sprite_platform;
    union {
        struct {
            uint8_t phase;
            uint8_t level;
            uint8_t sps_eaten;
            uint8_t last_row_count;
        };
        struct {
            int16_t anim_sps;
            int8_t anim_platform;
        };
    };

    uint8_t platform_pos = 4;

    void show();
    void move_bullets();
    bool move_sps();
    void chk_bullets();
    bool chk_platform();
    void fire();
    void move_platform(int delta);
    bool has_sps();

    enum GameStatus {
        Cont,
        Done,
        ImmBlast,
        Blast
    };
    GameStatus tick();
    GameStatus run_internal();

    void final_animate(GameStatus);

    void animate_platform();
    void animate_sps();

    void start_animate_platform();
    void start_animate_sps();

public:
    Invation() : t(CycleTime), t2(CycleTime) {
        phase=0;
        level = 0;
        sps_eaten = 0;
        last_row_count = 0;
    }
    void run();
};


void Invation::show()
{
    for ( int i = 0; i < 16; ++i )
    {
        pixs.br1[i] = arena.invation.bullets[i] | arena.invation.spsheeps[i];
        pixs.br2[i] = arena.invation.bullets[i];
    }
    spr.place(platform_pos, 15, phase);
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
    mask = 7 << (platform_pos-1);
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
    if (nxt_mask & arena.invation.spsheeps[14]) 
    {
        arena.invation.spsheeps[14] &= ~nxt_mask;
        pixs.br1[14] &= ~nxt_mask;
        ++sps_eaten;
    }
    if (delta == 1) nxt_mask <<= 1; else nxt_mask >>= 1;
    if (nxt_mask & arena.invation.spsheeps[15]) 
    {
        arena.invation.spsheeps[15] &= ~nxt_mask ; 
        pixs.br1[15] &= ~nxt_mask;
        ++sps_eaten;
    }
    platform_pos = new_pp;
}

bool Invation::has_sps()
{
    for ( int i = 0; i < 16; ++i )
    {
        if (arena.invation.spsheeps[i]) return true;
    }
    return false;
}

Invation::GameStatus Invation::tick()
{
    switch ( phase )
    {
        case 0: 
            if (arena.invation.spsheeps[15]) return Blast;
            if (move_sps()) return ImmBlast; break;
        case 1: case 3: move_bullets(); break;
    }
    ++phase;
    phase &= 3;
    return Cont;
}

Invation::GameStatus Invation::run_internal()
{
    GameStatus result = Cont;
    uint8_t perv_btn = 0;
    memset(&arena.invation, 0, sizeof(arena.invation));
    arena.invation.spsheeps[0] = bits[get_random() % bits_idx[0]] << 1;
    while(!result)
    {
        auto keys = read_key();
        clr_keys(K_1|K_2|K_3|K_Hit|K_Up);
        if (keys & K_1) return Done;

        uint8_t press = ~perv_btn & keys & (K_Left|K_Right);
        if (press) t2.reset(true);
        perv_btn = keys;

        if ( keys & (K_2 | K_3) )
        {
            move_platform(keys & K_3 ? 1 : -1);
        }
        else if ( (keys & (K_Left|K_Right)) && t2.tick() )
        {
            move_platform(keys & K_Right ? 1 : -1);
        }

        if ( t.tick() ) result = tick();
        if (keys & (K_Hit|K_Up)) fire();
        if ( level < TotalLevels-2 && sps_eaten >= LevelTreshold )
        {
            ++level;
            t.reinit(CycleTime+level);
            t2.reinit(CycleTime+level);
            sps_eaten = 0;
        }
        if ( last_row_count >= levels[level].sps_delta )
        {
            arena.invation.spsheeps[0] = bits[get_random() % bits_idx[levels[level].max_sps-1]] << 1;
            last_row_count = 0;
        }
        show();
    }
    return result;
}

void Invation::final_animate(GameStatus kind)
{
    anim_sps = -1;
    anim_platform = -1;
    switch ( kind )
    {
        case ImmBlast: start_animate_platform(); break;
        case Blast: start_animate_sps(); break;
        default: return;
    }
    while ( anim_sps != -1 || anim_platform >= 0 )
    {
        for ( ;;)
        {
            if (read_key() & K_1) return;
            if (t2.tick()) break;
        }
        if ( anim_sps != -1) animate_sps();
        if ( anim_platform >= 0 ) animate_platform();
    }
}

void Invation::start_animate_platform()
{
    uint8_t sh_mask = 7 << (platform_pos-1);
    pixs.br1[15] |= sh_mask;
    pixs.br2[15] |= sh_mask;
    anim_platform = 0;
}

void Invation::animate_platform()
{
    auto draw = [this](int color)
    {
        auto prev_row = 15-anim_platform;
        pixs.set_br(platform_pos, prev_row, color);
        if (platform_pos-1-anim_platform >= 0) pixs.set_br(platform_pos-1-anim_platform, prev_row, color);
        if (platform_pos+1+anim_platform < 8) pixs.set_br(platform_pos+1+anim_platform, prev_row, color);
    };
    draw(0);
    ++anim_platform;
    if (anim_platform >= 3 ) {anim_platform=-2; return;}
    draw(3-anim_platform);
}

void Invation::start_animate_sps()
{
    anim_sps = sh_idxs[arena.invation.spsheeps[15] >> 1];
    animate_sps();
}


void Invation::animate_sps()
{
    uint8_t sh_mask = 7 << (platform_pos-1);
    auto val = ships[anim_sps++];
    pixs.br1[15] = val & 0xFF;
    pixs.br2[15] = val >> 8;
    if (anim_platform == -1)
    {
        if ( sh_mask & val )
        {
            start_animate_platform();
        }
        else
        {
            pixs.br2[15] |= sh_mask;
        }
    }
    if ( val == 0 ) anim_sps = -1;
}


void Invation::run()
{
    auto result = run_internal();
    if (result != Done) final_animate(result);
}

void invation_game()
{
    Invation().run();
}
