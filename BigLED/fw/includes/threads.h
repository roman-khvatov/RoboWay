#pragma once

#include <stdint.h>

#define THREAD() static void* _thread_support_cur_place; if (_thread_support_cur_place) goto *_thread_support_cur_place
#define THREAD_WITH_DELAY() static int32_t _thread_support_delay_value; THREAD()

#define YIELD() do {_thread_support_cur_place = &&cont_label; return; cont_label:;} while(0)
#define RESTART() do {_thread_support_cur_place = NULL; return;} while(0)

#define DELAY_SETUP(ticks) (_thread_support_delay_value = (SysTick->VAL - (ticks)) & 0xFFFFFF)

#define IS_DELAY_DONE() ((int32_t((_thread_support_delay_value - SysTick->VAL) & 0xFFFFFF) << 8) >= 0)

#define DELAY(ticks) do { DELAY_SETUP_(ticks); while(!IS_DELAY_DONE()) YIELD(); } while(0)

#define SYSTICK_FREQ 24 // SysTick frequency (in MHZ)

inline constexpr unsigned operator ""_ms(long double val)  
{
    auto constexpr result = SYSTICK_FREQ*val*1000+0.5;
    static_assert( (unsigned long long)result < 0x7FFFFF, "Can't generate so long delay (23 bits of SysTick counter allowed for wait constant)");
    return unsigned(result);
}
inline constexpr unsigned operator ""_mks(long double val) 
{
    auto constexpr result = SYSTICK_FREQ*val+0.5;
    static_assert( (unsigned long long)result < 0x7FFFFF, "Can't generate so long delay (23 bits of SysTick counter allowed for wait constant)");
    return unsigned(result);
}
inline constexpr unsigned operator ""_ms(unsigned long long val)  
{
    auto constexpr result = SYSTICK_FREQ*val*1000;
    static_assert( result < 0x7FFFFF, "Can't generate so long delay (23 bits of SysTick counter allowed for wait constant)");
    return unsigned(result);
}

inline constexpr unsigned operator ""_mks(unsigned long long val) 
{
    auto constexpr result = SYSTICK_FREQ*val;
    static_assert( result < 0x7FFFFF, "Can't generate so long delay (23 bits of SysTick counter allowed for wait constant)");
    return unsigned(result);
}

#define WAIT(condition) while (!(condition)) YIELD()
#define DELAY_WITH_RESTART(ticks, restartCondition) do {DELAY_SETUP(ticks); while(!IS_DELAY_DONE()) {if(restartCondition) DELAY_SETUP(ticks); else YIELD();}} while(0)
#define WAIT_STABLE(ticks, expr) ({ \
    static unsigned value = 0; \
    unsigned dif_value; \
    DELAY_WITH_RESTART(ticks, ( (dif_value = value ^ (expr)),  (value ^= dif_value), dif_value)); \
    value;\
})
#define WAIT_WITH_TIMEOUT(ticks, condition) ({ \
    static bool waitAborted; \
    waitAborted = false; \
    DELAY_SETUP(ticks); \
    while(!IS_DELAY_DONE()) \
    if(condition) {waitAborted = true; break;} else YIELD();\
    waitAborted; \
})


/* How to use:

task definition:

void task1()
{
    // Anything placed here will be executed on EACH entry to task

    THREAD(); // or THREAD_WITH_DELAY() if you want to use any of DELAY_ functions
    for(;;)
    {
         ...

      ... YIELD(); // Synchronization point. Yield execution to other tasks


      // execution delay
      DELAY(500_mks); // Delay for 500mks

      // Separate delay setup/check
      DELAY_SETUP(0.5_ms);

      while(!IS_DELAY_DONE())
      {
        ... another checks ...
        if (...) break;
        ....

        if (...)
        {
            // You can restart delay timeout in this way
            DELAY_SETUP(500_mks);
        }

        YIELD();
      }

      if (...)
      {
          ...
          RESTART();  // Task will be immidiately restarted from begining
          // Not reached
      }

    }
}

NB: Automatic (on stack) variables will loose they value between YIELD() call (and any macro that includes it - DELAY for example)


Scheduler:

void main()
{
    hardware_init();
    some_other_init();
    ...

    // run tasks
    for(;;)
    {
        task1();
        task2();
        task3();
        ...
    }
}


*/
