#pragma once

/*
    Queue of Buttons (and QEncoder)
    This module also directly control Int line - it set if any data in Button queue exists or QEncoder value is not 0
*/

// Buttons setup
enum ButtonState
{
    ButtonRelease,
    ButtonPressed,
    ButtonAutoRepeatOne,
    ButtonAutoRepeatTwo,
};

extern uint8_t ButtonsSetup[13]; // Setup of events of interest (bitset of ButtonState for each button)


// Put Button event in queue (if we interested in this event for this Button)
// On queue overflow patch last Button event to 'Overflow'
void SendButton(int buttonIndex, ButtonState state);

// +/- 1 to QEncoder value. Avoid overflow (+/- 64 is maximum QEncoder value)
void SendQuadEncoderValue(int delta);

// Return current QEncode value
int8_t GetQuadEncValue();

// Clear current QEncode value
void ClearQuadEncValue();

// Get total buttons in queue. Limit returned value by 'max_value'
uint8_t GetTotalButtons(uint8_t max_value);

// Fetch Button from queue. Returns 0 if no button in queue
uint8_t GetButtonFromQueue();
