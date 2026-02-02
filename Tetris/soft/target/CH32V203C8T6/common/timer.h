#pragma once

#include "interface.h"

class Timer {
    uint32_t cnt=0;
    uint32_t rate_setup;
    uint32_t max_setup;
public:
    Timer(int freq_hz, int freq_divider=1) 
    { 
        reinit(freq_hz, freq_divider);
    }

    void reinit(int freq_hz, int freq_divider = 1)
    {
        rate_setup = freq_hz * tick_time;
        max_setup = freq_divider * 1000;
        cnt = 0;
    }

    void reset(bool hit_now)
    {
        cnt = hit_now ? max_setup : 0;
    }

    bool tick()
    {
        cnt += rate_setup;
        if (cnt >= max_setup)
        {
            cnt %= max_setup;
            return true;
        }
        return false;
    }

    void wait() 
    {
        for(;;)
        {
            read_key();
            if (tick()) return;
        }
    }
};
