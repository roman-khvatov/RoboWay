/*I2C from PY32F002A*/

#include "thread.h"

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
        DELAY_WITH_RESTART(500_mks, QuadEncoderButtons() == 0);

        static uint8_t buttonState;
        
        WAIT((buttonState = QuadEncoderButtons()) != 0);
        
        if(buttonState == 3) continue;
        
        SendQuadEncoderValue(buttonState == 1 ? 1 : -1);
        
        DELAY(1_ms);            
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
            DELAY_SETUP(500_mks);
            restartDelay = false;
        }

        if(error)
        {
            DELAY(500_mks);
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
            DELAY_SETUP(500_mks);
            restartDelay = false;
        }

        if(error)
        {
            DELAY(500_mks);
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
            DELAY_SETUP(500_mks);
            restartDelay = false;
        }

        if(error)
        {
            DELAY(500_mks);
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
            DELAY_SETUP(500_mks);
            restartDelay = false;
        }

        if(error)
        {
            DELAY(500_mks);
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
            DELAY_SETUP(500_mks);
            restartDelay = false;
        }

        if(error)
        {
            DELAY(500_mks);
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