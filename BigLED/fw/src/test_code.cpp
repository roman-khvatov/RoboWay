/*I2C from PY32F002A*/

#include "thread.h"

/*max timeout delay = 300ms*/

enum ButtonState
{
    ButtonRelease,
    ButtonPressed,
    ButtonAutoRepeatOne,
    ButtonAutoRepeatTwo,
};

uint8_t QuadEncoderButtons();

void SendQuadEncoderValue(int);

void SendButton(int buttonIndex, ButtonState);

void QuadEncoderTask()
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

uint16_t buttonState;

void ButtonsTask()
{
    THREAD_WITH_DELAY();
    for(;;)
    {
        uint16_t newButtonState = WAIT_STABLE(2_ms, ReadButtons());
        for(auto index: BitScan(newButtonState &~ buttonState))
        {
            SendButton(index, ButtonPressed);
        }
        for(auto index: BitScan(~newButtonState & buttonState))
        {
            SendButton(index, ButtonRelease);
        }
        buttonState = newButtonState;    
    }    
}

int checkOneButton()
{
    if(total1(buttonState) != 1) return -1;
    return first1(buttonState);
}

#define WAIT_AUTOREPEAT(count) for(waitCounter = 0; waitCounter < count; ++waitCounter) if(WAIT_WITH_TIMEOUT(100_ms, currentButton != checkOneButton())) RESTART()

void AutoRepeatTask()
{
    THREAD_WITH_DELAY();
    for(;;)
    {
        static int currentButton;
        currentButton = checkOneButton();
        if(currentButton == -1) RESTART();
        WAIT_AUTOREPEAT(AutoRepeatOne);
        SendButton(currentButton, ButtonAutoRepeatOne);
        for(;;)
        {
            WAIT_AUTOREPEAT(AutoRepeatTwo);
            SendButton(currentButton, ButtonAutoRepeatTwo);
        }        
    }
}

enum I2CTaskSchedule
{
    Idle,
    Read,
    Write,
} currentSchedule;

void I2CSelectTask()
{
    THREAD();
    for(;;)
    {
        WAIT(I2CAcceptedAddress());
        currentSchedule = isI2CRead() ? Read : Write;
        YIELD();
    }
}

#define I2CWAIT() do {WAIT(I2CReady()); if(I2CAborted()) {currentSchedule = Idle; RESTART()};} while (0)

void I2CRead()
{
        
}

void I2CWrite()
{

}

void main()
{
    /*initialisations*/
    hardware_init();
    
    /*run tasks*/
    for(;;)
    {
        QuadEncoderTask();
        ButtonsTask();
        AutoRepeatTask();
        if(currentSchedule == Idle) I2CSelectTask();
        switch(currentSchedule)
        {
            case Read: I2CRead(); break;
            case Write: I2CWrite(); break;
        }            
    }
}