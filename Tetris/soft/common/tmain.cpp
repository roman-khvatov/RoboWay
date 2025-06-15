#include "interface.h"
#include "timer.h"

void tetris_game();

void entry()
{
    for(;;) tetris_game();

/*
    int x=0, y=0;
    Timer t(10);
    int br = 0;

    for (;;)
    {
        int keys = read_key();
        clr_keys(-1);

        if (keys) pixs.set_br(x, y, 0);

        if (keys & K_Right) ++x &= 7; else
        if (keys & K_Left) --x &= 7; else
        if (keys & K_Up) --y &= 15; else
        if (keys & K_Down) ++y &= 15;

        if (t.tick())
        {
            keys = -1;
            ++br &= 3;
        }
        if (keys) pixs.set_br(x, y, br);
    }
*/
}