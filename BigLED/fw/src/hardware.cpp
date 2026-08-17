#include "hardware.h"

///////////////// Low level Hardware interface

#define PIN_INT LL_GPIO_PIN_5
#define PORT_INT GPIOA
#define PIN_ENABLE_OLED LL_GPIO_PIN_6
#define PORT_ENABLE_OLED GPIOA
#define PIN_QUAD_ENC_A LL_GPIO_PIN_1
#define PORT_QUAD_ENC GPIOA
#define PIN_QUAD_ENC_B LL_GPIO_PIN_0

#define PIN_BTN_1 LL_GPIO_PIN_3               
#define PORT_BTN_1 GPIOB
#define PIN_BTN_2 LL_GPIO_PIN_2               
#define PORT_BTN_2 GPIOB
#define PIN_BTN_3 LL_GPIO_PIN_1               
#define PORT_BTN_3 GPIOB
#define PIN_BTN_4 LL_GPIO_PIN_0               
#define PORT_BTN_4 GPIOB
#define PIN_BTN_5 LL_GPIO_PIN_7               
#define PORT_BTN_5 GPIOA
#define PIN_BTN_6 LL_GPIO_PIN_4
#define PORT_BTN_6 GPIOA

#define PORT_SCAN_1 GPIOA
#define PIN_SCAN_1 LL_GPIO_PIN_13
#define PORT_SCAN_2 GPIOA
#define PIN_SCAN_2 LL_GPIO_PIN_14

#define PORT_QUAD_PRESS GPIOB
#define PIN_QUAD_PRESS LL_GPIO_PIN_6

// Initialize all hardware
void hardware_init();

// Engage Int line - turn it to output and set appropriate Int level
void EnableInterrupt()
{
    LL_GPIO_SetPinMode(PORT_INT, PIN_INT, LL_GPIO_MODE_OUTPUT);    
}

// Signal interrupt (turn it on or off)
void SignalInterrupt(bool activate)
{
    if(activate)
    {
        LL_GPIO_SetOutputPin(PORT_INT, PIN_INT);
    }    
    else
    {
        LL_GPIO_ResetOutputPin(PORT_INT, PIN_INT);    
    }
}

// Connect OLED to I2C bus
void TurnOLEDOn()
{
    LL_GPIO_SetOutputPin(PORT_ENABLE_OLED, PIN_ENABLE_OLED);    
}

// Disconnect OLED from I2C bus
void TurnOLEDOff()
{
    LL_GPIO_ResetOutputPin(PORT_ENABLE_OLED, PIN_ENABLE_OLED);    
}

/////// Low level Buttons interface

// Return 2 bits of A/B signals of QEncoder
uint8_t QuadEncoderButtons()
{
    return LL_GPIO_ReadInputPort(PORT_QUAD_ENC) & (QUAD_ENC_PIN_B | QUAD_ENC_PIN_A);
}

uint16_t Combine()
{
    uint8_t a = LL_GPIO_ReadInputPort(GPIOA);
    uint8_t b = LL_GPIO_ReadInputPort(GPIOB);
    return (b & 15) | (a & 0x10) | ((a >> 2) & 0x20);    
}

// Read raw button state as bitset (13 bits)
uint16_t ReadButtons()
{
    LL_GPIO_SetPinMode(PORT_SCAN_1, PIN_SCAN_1, LL_GPIO_MODE_OUTPUT);
    uint16_t combined = Combine();
    LL_GPIO_SetPinMode(PORT_SCAN_1, PIN_SCAN_1, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(PORT_SCAN_2, PIN_SCAN_2, LL_GPIO_MODE_OUTPUT);
    combined |= Combine() << 6;
    LL_GPIO_SetPinMode(PORT_SCAN_2, PIN_SCAN_2, LL_GPIO_MODE_INPUT);
    
    if(LL_GPIO_IsInputPinSet(PORT_QUAD_PRESS, PIN_QUAD_PRESS)) combined |= 1 << 12;
    return combined;
}
