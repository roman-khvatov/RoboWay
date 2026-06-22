/*I2C from PY32F002A*/

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

#define WAIT(condition) while (!(condition)) YIELD()
#define DELAY_WITH_RESTART(mks, restartCondition) do {DELAY_SETUP_MKS(mks); while(!IS_DELAY_DONE()) {if(restartCondition) DELAY_SETUP_MKS(mks); else YIELD();}} while(0)

/*max timeout delay = 300ms*/

void ScanButtons()
{
    /*code*/
    THREAD();
    for(;;)
    {
        
    } 
}

void GeneratePackets()
{
    /*code*/
    THREAD();
    for(;;)
    {
        
    }
}

void QuadEncoder()
{
    /*code*/
    THREAD_WITH_DELAY();
    for(;;)
    {
        DELAY_WITH_RESTART(500, QuadEncoderButtons() == 3);

        static uint8_t buttonState;
        
        WAIT((buttonState = QuadEncoderButtons()) != 3);
        
        if(buttonState == 0) continue;
        
        SendQuadEncoderValue(buttonState == 1 ? 1 : -1);
        
        DELAY_MS(1);            
    }
}

void SendPackets()
{
    /*code*/
    THREAD();
    for(;;)
    {
        
    }
}

void AcceptPackets()
{
    /*code*/
    THREAD();
    for(;;)
    {
        
    }
}

void AcceptReadRequest()
{
    /*code*/
    THREAD();
    for(;;)
    {
        
    }
}

void Task1()
{
    /*code*/

    bool error = false;
    bool restartDelay = false;

    THREAD_WITH_DELAY();
    for(;;)
    {
        ScanButtons();

        if(restartDelay)
        {
            DELAY_SETUP_MKS(500);
            restartDelay = false;
        }

        if(error)
        {
            DELAY_MKS(500);
            RESTART();
        }
    }
}

void Task2()
{
    /*code*/

    bool error = false;
    bool restartDelay = false;

    THREAD_WITH_DELAY();
    for(;;)
    {
        GeneratePackets();
    
        if(restartDelay)
        {
            DELAY_SETUP_MKS(500);
            restartDelay = false;
        }

        if(error)
        {
            DELAY_MKS(500);
            RESTART();
        }
    }
}

void Task3()
{
    /*code*/

    bool error = false;
    bool restartDelay = false;

    THREAD_WITH_DELAY();
    for(;;)
    {              
        QuadEncoder();
        SendPackets();
    
        if(restartDelay)
        {
            DELAY_SETUP_MKS(500);
            restartDelay = false;
        }

        if(error)
        {
            DELAY_MKS(500);
            RESTART();
        }
    }
}

void Task4()
{
    /*code*/

    bool error = false;
    bool restartDelay = false;

    THREAD_WITH_DELAY();
    for(;;)
    {
        AcceptPackets();
        
        if(restartDelay)
        {
            DELAY_SETUP_MKS(500);
            restartDelay = false;
        }

        if(error)
        {
            DELAY_MKS(500);
            RESTART();
        }
    }
}

void Task5()
{
    /*code*/

    bool error = false;
    bool restartDelay = false;

    THREAD_WITH_DELAY();
    for(;;)
    {
        AcceptReadRequest();
        SendPackets();
    
        if(restartDelay)
        {
            DELAY_SETUP_MKS(500);
            restartDelay = false;
        }

        if(error)
        {
            DELAY_MKS(500);
            RESTART();
        }
    }
}

void main()
{
    /*initialisations*/
    hardware_init();
    
    /*run tasks*/
    for(;;)
    {
        Task1();
        Task2();
        Task3();
        Task4();
        Task5();
    }
}