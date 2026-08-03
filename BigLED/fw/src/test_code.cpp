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

#define I2CREAD()       ( {WAIT(I2CReadReady());  if(I2CAborted()) {currentSchedule = Idle; RESTART()}; I2CReadData();} )
#define I2CWRITE(data) do {WAIT(I2CWriteReady()); if(I2CAborted()) {currentSchedule = Idle; RESTART()}; I2CWriteData(data);} while (0)

void I2CRead()
{
    THREAD();
    for(;;)
    {
        uint8_t cmd = I2CREAD();

        switch(cmd)
        {
            case 0: EnableInterrupt(); break;
            case 1: TurnOLEDOn(); break;
            case 2: TurnOLEDOff(); break;
            case 3: {
                ButtonAutoRepeatOne = I2CREAD();
                ButtonAutoRepeatTwo = ButtonAutoRepeatOne / 2;
                ButtonAutoRepeatTwo = I2CREAD();
            } break;
            case 4: requestButtonState = true; break;
            default: {
                uint8_t ButtonIndex = (cmd >> 4) & 15;
                uint8_t ButtonSetup = cmd & 15;
                if(ButtonIndex == 0 || ButtonIndex == 14) break;
                if(ButtonIndex != 15)
                {
                    ButtonsSetup[ButtonIndex-1] = ButtonSetup;
                }
                else
                {
                    for(int i = 1; i < 14; i++)
                    {
                        ButtonSetup[i] = ButtonSetup;                                                
                    }
                }    
            } break;
        }
    }        
}

void I2CWrite()
{
    THREAD();
    for(;;)
    {
        static uint16_t tmp;
        static uint8_t counter;
        counter = 0;
        if(requestButtonState)
        {
            requestButtonState = false;
            tmp = buttonState;
            counter = GetTotalButtons(7);
            tmp |= counter << 12;
            I2CWRITE(tmp);
            I2CWRITE(tmp >> 8);
        }
        int8_t QuadValue = GetQuadEncValue();
        if(QuadValue != 0)
        {
            ClearQuadEncValue();
            I2CWRITE(QuadValue | 0x80);
        }
        GetPressedButtons();
        I2CWRITE();
    }
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