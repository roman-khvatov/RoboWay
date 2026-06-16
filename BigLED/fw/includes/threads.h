#pragma once

#include <stdint.h>

#define THREAD() static void* _thread_support_cur_place; if (_thread_support_cur_place) goto *_thread_support_cur_place
#define THREAD_WITH_DELAY() static int32_t _thread_support_delay_value; THREAD()

#define YIELD() do {_thread_support_cur_place = &&cont_label; return; cont_label:;} while(0)
#define RESTART() do {_thread_support_cur_place = NULL; return;} while(0)

#define DELAY_SETUP_(ticks) (_thread_support_delay_value = (SysTick->VAL - (ticks)) & 0xFFFFFF)

#define IS_DELAY_DONE() ((int32_t((_thread_support_delay_value - SysTick->VAL) & 0xFFFFFF) << 8) >= 0)

#define DELAY_(ticks) do { DELAY_SETUP_(ticks); while(!IS_DELAY_DONE()) YIELD(); } while(0)


#define MS2TICKS_(ms) ((ms)*24000)
#define MKS2TICKS_(ms) ((ms)*24)

#define DELAY_SETUP_MS(time) DELAY_SETUP_(MS2TICKS_(time))
#define DELAY_SETUP_MKS(time) DELAY_SETUP_(MKS2TICKS_(time))

#define DELAY_MS(time) DELAY_(MS2TICKS_(time))
#define DELAY_MKS(time) DELAY_(MKS2TICKS_(time))

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
      DELAY_MKS(500); // Delay for 500mks

      // Separate delay setup/check
      DELAY_SETUP_MKS(500);

      while(!IS_DELAY_DONE())
      {
        ... another checks ...
        if (...) break;
        ....

        if (...)
        {
            // You can restart delay timeout in this way
            DELAY_SETUP_MKS(500);
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
