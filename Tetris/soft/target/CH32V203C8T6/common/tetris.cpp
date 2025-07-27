#include "sprite.h"
#include "spr_defs.h"
#include "timer.h"

/* Color map for Tetris:

Full - Active figure
2/3  - Settled down figures
1/3  - Final frosen image (after 'game over')
2/3 -> {Full -> 2/3 -> 1/3 -> 2/3} x 2 -> 0  - Highlight sequence for collapsed lines
{ Full -> 2/3 } x * -> 2/3  - Settle down highlight 
*/


class TetrisGame {
    static constexpr int max_level = 10;
    static constexpr int fall_down_mult = 10;
    static constexpr int settle_down_mult = 10;
    static constexpr int settle_down_timeout = 2 * settle_down_mult;
    static constexpr int squeeze_mult = 8;
    static constexpr int squeeze_count = 2;
    static constexpr int lines_per_level = 10;

    int level = 1; // Freqency in Hz

    int collapsed_lines = 0;
    Sprite figure = 0;
    Timer timer = 1;

    bool place_figure(); // Retrun true if successfully placed
    void process_key(int key);
    void settle_down();
    void squeeze();

public:

    void run()
    {
        while(place_figure())
        {
            timer.reinit(level);
            for (;;)
            {
                auto key = read_key();
                clr_keys(-1);
                process_key(key);
                if (key & K_3)
                {
                    return;
                }
                if (timer.tick() && !figure.move(0, 1, 0))
                {
                    settle_down();
                    squeeze();
                    break;
                }
            }
        }
    }
};

// Retrun true if successfully placed
bool TetrisGame::place_figure()
{
    int sprite_idx = tetris_figures[get_random() % total_tetris_figures];
    figure = Sprite(sprite_idx);
    return figure.place(4, figure.spr().height/2, 0);
}

void TetrisGame::process_key(int key)
{
    if (key & K_Left)  figure.move(-1, 0, 0); else
    if (key & K_Right) figure.move(1, 0, 0); else
    if (key & K_Up)    figure.move(0, 0, 1); else
    if (key & K_Down)  timer.reinit(level*fall_down_mult);
}

void TetrisGame::settle_down()
{
    bool color_ff = true;
    int countdown = settle_down_timeout;
    
    timer.reinit(level*settle_down_mult);
    while (countdown > 0)
    {
        figure.move(0, 0, 0, color_ff ? SC_2 : SC_Full);        
        auto key = read_key();
        clr_keys(-1);
        key &= ~K_Down;
        if (key)
        {
            process_key(key);
            while (figure.move(0, 1, 0)) {;} // Try to fall down
            countdown = settle_down_timeout;
        }
        if (timer.tick())
        {
            color_ff = !color_ff;
            --countdown;
        }
    }
    figure.move(0, 0, 0, SC_2);
    timer.reinit(level);
}

void TetrisGame::squeeze()
{
    uint16_t squeeze_mask = 0;
    for (int y = 0; y < 16; ++y)
    {
        if (pixs.br2[y] == 0xFF) 
        {
            squeeze_mask |= 1 << y;
            ++collapsed_lines;
        }
    }
    if (!squeeze_mask) return;
    timer.reinit(level* squeeze_mult);

    auto fill = [squeeze_mask](uint8_t c1, uint8_t c2) {
        uint16_t m = squeeze_mask;
        for (int y = 0; y < 16; ++y, m>>=1)
        {
            if (m & 1)
            {
                pixs.br1[y] = c1;
                pixs.br2[y] = c2;
            }
        }
    };
    auto wait = [this]() {do { read_key(); } while(!timer.tick());};

    for (int rep = 0; rep < squeeze_count; ++rep)
    {
        // {Full -> 2/3 -> 1/3 -> 2/3}
        fill(0xFF, 0xFF); wait();
        fill(0, 0xFF); wait();
        fill(0xFF, 0); wait();
        fill(0, 0xFF); wait();
    }
    fill(0, 0); wait();
    timer.reinit(level);
    int dst = 16;
    for (int y = 16; y--;)
    {
        if (pixs.br1[y] | pixs.br2[y])
        {
            --dst;
            if (dst != y) {pixs.br1[dst] = pixs.br1[y]; pixs.br2[dst] = pixs.br2[y];}
        }
    }
    while (dst--)
    {
        pixs.br1[dst] = pixs.br2[dst] = 0;
    }
    if (collapsed_lines >= lines_per_level)
    {
        if (level < max_level) ++level;
        collapsed_lines -= lines_per_level;
    }
}

void tetris_game()
{
    TetrisGame().run();
}
