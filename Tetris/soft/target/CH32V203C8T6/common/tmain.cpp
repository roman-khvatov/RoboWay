#include "interface.h"
#include "timer.h"
#include "spr_defs.h"
#include "sprite.h"
#include "timer.h"


static constexpr int scroll_mul = 2;

void tetris_game();
void snake_game();


// Mix <bits_first> of 'icon1' with 'icon2'
//  icon1  icon2
//  0    5 0    5 < bit number
//  ****** ******
//    +------+  << window
//  <->         << bits_first
static void hor_mix(int icon1, int icon2, uint8_t bits_first)
{
    const uint8_t* p1 = logos + icon1 * 28;
    const uint8_t* p2 = logos + icon2 * 28;

    uint8_t mask2 = (1 << bits_first) - 1;
    uint8_t shift2 = 6 - bits_first;

    auto action = [=](const uint8_t* src1, const uint8_t* src2, uint8_t* dst)
        {
            *dst++ = 0xFF;
            for(int i=0; i<14; ++i) *dst++ = (((*src1++ >> bits_first) | ((*src2++ & mask2) << shift2)) << 1) | 0x81;
            *dst++ = 0xFF;
        };
    action(p1, p2, pixs.br1);
    action(p1+14, p2+14, pixs.br2);
}

// Mix 'lines_first' of icon1 with icon2 (lines_first is a number of lines to skip)
static void ver_mix(int icon1, int icon2, uint8_t lines_first)
{
    const uint8_t* p1 = logos + icon1 * 28 + lines_first;
    const uint8_t* p2 = logos + icon2 * 28;

    auto action = [=](const uint8_t* src1, const uint8_t* src2, uint8_t* dst)
        {
            *dst++ = 0xFF;
            for (int i = lines_first; i < 14; ++i) *dst++ = (*src1++ << 1) | 0x81;
            for (int i = 0; i < lines_first; ++i) *dst++ = (*src2++ << 1) | 0x81;
            *dst++ = 0xFF;
        };
    action(p1, p2, pixs.br1);
    action(p1 + 14, p2 + 14, pixs.br2);
}

inline void draw_icon(int icon) {ver_mix(logos_entries[icon], logos_entries[icon], 0);}

static int scroll_hor(int icon, int delta)
{
    Timer t(6 * scroll_mul);
    int result = (icon + delta + LogosTotal) % LogosTotal;
    int icon2 = logos_entries[result];
    icon = logos_entries[icon];
    if (delta > 0)
    {
        for (int i = 0; i < 7; ++i)
        {
            hor_mix(icon, icon2, i);
            t.wait();
        }
    }
    else
    {
        for (int i = 0; i < 7; ++i)
        {
            hor_mix(icon2, icon, 6 - i);
            t.wait();
        }
    }
    return result;
}

static int scroll_ver(int icon, int delta)
{
    Timer t(14 * scroll_mul);
    int result = (icon + delta + LogosTotal) % LogosTotal;
    int icon2 = logos_entries[result];
    icon = logos_entries[icon];
    if (delta > 0)
    {
        for (int i = 0; i < 15; ++i)
        {
            ver_mix(icon, icon2, i);
            t.wait();
        }
    }
    else
    {
        for (int i = 0; i < 15; ++i)
        {
            ver_mix(icon2, icon, 14 - i);
            t.wait();
        }
    }
    return result;
}

static void freeze()
{
    for (int y = 0; y < 16; ++y)
    {
        pixs.br1[y] |= pixs.br2[y];
        pixs.br2[y] = 0;
    }
}

static uint8_t update_icon(int game)
{
    Timer t(4);
    uint8_t start_idx = logos_entries[game];
    uint8_t end_idx = logos_entries[game+1];
    uint8_t ico = start_idx;
    for (;;)
    {
        auto key = read_key();
        clr_keys(-1);
        if (key & (K_Up|K_Down|K_Left|K_Right|K_Hit)) return key;
        if (t.tick())
        {
            ver_mix(ico, ico, 0);
            ++ico;
            if (ico >= end_idx) ico = start_idx;
        }
    }
}

static void select_game(int& game)
{
    draw_icon(game);
    for (;;)
    {
        auto key = update_icon(game);
        // process LED calibration !!!
        switch (key)
        {
            case K_Up:    game = scroll_ver(game, 1); break;
            case K_Down:  game = scroll_ver(game, -1); break;
            case K_Right: game = scroll_hor(game, -1); break;
            case K_Left:  game = scroll_hor(game, 1); break;
            case K_Hit: return;
        }
        draw_icon(game);
    }
}

static bool rd_key()
{
    for(;;)
    {
        auto key = read_key();
        clr_keys(-1);
        if (key & K_1) return false;
        if (key & K_2) return true;
    }

}

void entry()
{
    int game=0;

    for (;;)
    {        
        select_game(game);
        for(;;)
        {
            pixs.clear();
            switch (game)
            {
                case Logo_tetris: tetris_game(); break;
                case Logo_snake: snake_game(); break;
                //case Logo_invation: invation_game(); break;
            }
            freeze();
            if (rd_key()) break;
        }
    }
}
