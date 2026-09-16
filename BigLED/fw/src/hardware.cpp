#include <stdint.h>

#include <py32f0xx_ll_gpio.h>
#include <py32f0xx_ll_i2c.h>
#include <py32f0xx_ll_bus.h>
#include <py32f0xx_ll_utils.h>
#include <py32f0xx_ll_rcc.h>

#include "hardware.h"

///////////////// Low level Hardware interface

#define PIN_INT LL_GPIO_PIN_5          //input
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

#define I2CAddress1 0x55
#define I2CAddress2 0x56

static bool IntActiveHigh = true;
static bool IntActivated = false;

static void APP_SystemClockConfig(void);
static void APP_GPIOConfig(void);

static void APP_SystemClockConfig(void)
{
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1);

  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSISYS);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSISYS);

  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);

//  LL_Init1msTick(8000000);
  SysTick->LOAD  = 0xFFFFFFul;
  SysTick->VAL   = 0UL;
  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

  LL_SetSystemCoreClock(8000000);
}


static void APP_GPIOConfig(void)
{
  // PA0
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA | LL_IOP_GRP1_PERIPH_GPIOB | LL_IOP_GRP1_PERIPH_GPIOF);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

  LL_GPIO_SetPinMode(PORT_INT, PIN_INT, LL_GPIO_MODE_INPUT);
  LL_GPIO_ResetOutputPin(PORT_ENABLE_OLED, PIN_ENABLE_OLED);
  LL_GPIO_SetPinMode(PORT_ENABLE_OLED, PIN_ENABLE_OLED, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinMode(PORT_QUAD_ENC, PIN_QUAD_ENC_A, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinMode(PORT_QUAD_ENC, PIN_QUAD_ENC_B, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinMode(PORT_BTN_1, PIN_BTN_1, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinMode(PORT_BTN_2, PIN_BTN_2, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinMode(PORT_BTN_3, PIN_BTN_3, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinMode(PORT_BTN_4, PIN_BTN_4, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinMode(PORT_BTN_5, PIN_BTN_5, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinMode(PORT_BTN_6, PIN_BTN_6, LL_GPIO_MODE_INPUT);

  LL_GPIO_SetOutputPin(PORT_SCAN_1, PIN_SCAN_1);
  LL_GPIO_SetOutputPin(PORT_SCAN_2, PIN_SCAN_2);
  LL_GPIO_SetPinMode(PORT_SCAN_1, PIN_SCAN_1, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinMode(PORT_SCAN_2, PIN_SCAN_2, LL_GPIO_MODE_INPUT);

  LL_GPIO_SetPinMode(PORT_QUAD_PRESS, PIN_QUAD_PRESS, LL_GPIO_MODE_INPUT);

  LL_GPIO_SetPinPull(PORT_QUAD_ENC, PIN_QUAD_ENC_A, LL_GPIO_PULL_DOWN);
  LL_GPIO_SetPinPull(PORT_QUAD_ENC, PIN_QUAD_ENC_B, LL_GPIO_PULL_DOWN);
  LL_GPIO_SetPinPull(PORT_BTN_1, PIN_BTN_1, LL_GPIO_PULL_DOWN);
  LL_GPIO_SetPinPull(PORT_BTN_2, PIN_BTN_2, LL_GPIO_PULL_DOWN);
  LL_GPIO_SetPinPull(PORT_BTN_3, PIN_BTN_3, LL_GPIO_PULL_DOWN);
  LL_GPIO_SetPinPull(PORT_BTN_4, PIN_BTN_4, LL_GPIO_PULL_DOWN);
  LL_GPIO_SetPinPull(PORT_BTN_5, PIN_BTN_5, LL_GPIO_PULL_DOWN);
  LL_GPIO_SetPinPull(PORT_BTN_6, PIN_BTN_6, LL_GPIO_PULL_DOWN);
  LL_GPIO_SetPinPull(PORT_QUAD_PRESS, PIN_QUAD_PRESS, LL_GPIO_PULL_DOWN);

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  // PF1 SCL
  GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_12;
  LL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  // PF0 SDA
  GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_12;
  LL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  IntActiveHigh = LL_GPIO_IsInputPinSet(PORT_INT, PIN_INT);
  if(IntActiveHigh) LL_GPIO_SetPinMode(PORT_INT, PIN_INT, LL_GPIO_MODE_OUTPUT);
  else LL_GPIO_ResetOutputPin(PORT_INT, PIN_INT); 
  LL_GPIO_SetPinMode(PORT_INT, PIN_INT, LL_GPIO_MODE_OUTPUT);
}

void I2CInit()
{
  LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_I2C1);
  LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_I2C1);

  LL_I2C_InitTypeDef I2C_InitStruct;
  /*
   * Clock speed:
   * - standard = 100khz
   * - fast     = 400khz
  */
  I2C_InitStruct.ClockSpeed      = LL_I2C_MAX_SPEED_FAST;
  I2C_InitStruct.DutyCycle       = LL_I2C_DUTYCYCLE_16_9;
  I2C_InitStruct.OwnAddress1     = IntActiveHigh ? I2CAddress1 : I2CAddress2;
  I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
  LL_I2C_Init(I2C1, &I2C_InitStruct);
}

// Initialize all hardware
void hardware_init()
{
    //init buttons
    //seperate init i2c
    APP_SystemClockConfig();
    APP_GPIOConfig();
    I2CInit();    
}

// Engage Int line - turn it to output and set appropriate Int level
void EnableInterrupt()
{
    if (IntActivated) return;
    LL_GPIO_SetPinMode(PORT_INT, PIN_INT, LL_GPIO_MODE_OUTPUT);    
    IntActivated = true;
    SignalInterrupt(false);
}

// Signal interrupt (turn it on or off)
void SignalInterrupt(bool activate)
{
    if (!IntActivated) return;
    if(activate == IntActiveHigh)
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
    return LL_GPIO_ReadInputPort(PORT_QUAD_ENC) & (PIN_QUAD_ENC_B | PIN_QUAD_ENC_A);
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
