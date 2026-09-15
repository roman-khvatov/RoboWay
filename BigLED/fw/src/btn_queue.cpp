#include "btn_queue.h"

int8_t current_quadenc_value;

// +/- 1 to QEncoder value. Avoid overflow (+/- 64 is maximum QEncoder value)
void SendQuadEncoderValue(int delta)
{
    current_quadenc_value += delta;
    if(current_quadenc_value > 64)
    {
        current_quadenc_value = 64;
    }
    else if(current_quadenc_value < -64)
    {
        current_quadenc_value = -64;
    }
}

// Return current QEncode value
int8_t GetQuadEncValue()
{
    return current_quadenc_value;    
}

// Clear current QEncode value
void ClearQuadEncValue()
{
    current_quadenc_value = 0;        
}
/////////////////////////////////////////////////////////////////////////////
uint8_t ButtonsSetup[13]; // Setup of events of interest (bitset of ButtonState for each button)

uint8_t ButtonsBuffer[64];
uint8_t Read_PTR;
uint8_t Size_PTR;

// Put Button event in queue (if we interested in this event for this Button)
// On queue overflow patch last Button event to 'Overflow'
void SendButton(int buttonIndex, ButtonState state)
{
    if((ButtonsSetup[buttonIndex] >> state) &1)
    {
        if(Size_PTR < 64)
        {
            ButtonsBuffer[(Read_PTR + Size_PTR) &63] = state + ((buttonIndex +1) << 2);
            Size_PTR++;
        }
        else
        {
            ButtonsBuffer[(Read_PTR + 63) &63] = 1;
        }    
    }    
}

// Get total buttons in queue. Limit returned value by 'max_value'
uint8_t GetTotalButtons(uint8_t max_value)
{
    if(Size_PTR > max_value)
    {
        return max_value;
    }
    else
    {
        return Size_PTR
    }
}

// Fetch Button from queue. Returns 0 if no button in queue
uint8_t GetButtonFromQueue()
{
    if(Size_PTR == 0) return 0;

    uint8_t result = ButtonsBuffer[Read_PTR];
    Read_PTR++;
    Read_PTR &= 63;
    Size_PTR--;
    return result;
}

