#include "../../../common/interface.h"

Pixels pixs;

static Pixels working_pixels;
static uint8_t col_index;
static uint8_t phase;

static uint8_t cur_keys;
static uint8_t changed_keys;
static uint8_t debounce;

static constexpr int debounce_time = 5;

static uint16_t led_voltage;
static uint8_t led_cnt;
static uint8_t leds_to_sample;


enum class LEDVoltageReq : uint8_t {
    Idle,
    Request,
    InProgress,
    Ready
};

volatile uint16_t leds_voltage[2];
volatile LEDVoltageReq request_led_voltages;

static volatile bool done;


/*

SPI1 (8 bit): (Remapping mapping, see AFIO)

PB5 - DI-COL         MOSI
PB3 - CLK-COL        CLK

SPI2 (16 bit):

PB12 -  CS           CS
PB13 -  CLK-ROW      CLK
PB15 -  DI-ROW       MOSI

PA0-PA7 - buttons

PA0 - B1   J2     LEFT
PA1 - B5   J6     DOWN
PA2 - B4   J5     RIGHT
PA3 - B3   J4     UP
PA4 - B7   BUT3
PA5 - B2   J3     HIT
PA6 - B6   BUT2
PA7 - B8   BUT1

PB0/PB1 - CS (analog)

PB0 - Rows 8-15
PB1 - Rows 0-7

PB10 - PWM       (TIM2_CH3_2/3, alt mapping)
PB11 - LED OE

PA15 - InInt indicator

Timers:

SysTick - CRC feed register (free running on max speed)

TIM3 - System clock (for timer interrupt)
TIM2 - PWM for LEDs LDO


EXTI0-EXTI7 - Connected to PA0-7. Feed RND generator ob any button state change


EXTI* int handlers:

EXTI0_IRQHandler
EXTI1_IRQHandler
EXTI2_IRQHandler
EXTI3_IRQHandler
EXTI4_IRQHandler
EXTI9_5_IRQHandler

*/

// SPI1 - Cols
// SPI2 - Rows

extern "C" void TIM3_IRQHandler() __attribute__((interrupt("WCH-Interrupt-fast")));


void TIM3_IRQHandler()
{
    GPIOA->BSHR = 1<<15;    // Set PA15 to '1'

    // Pixels
    uint16_t row=0;
    uint8_t col = ~(1<<col_index);
    if (phase!=2)
    {
        uint8_t* array = phase ? working_pixels.br2 : working_pixels.br1;
        row = array[col_index] | (array[col_index+8]<<8);
        SPI1->DATAR = col;
        SPI2->DATAR = row;
    }
    if (!debounce)
    {
        uint8_t buttons = ~GPIOA->INDR;
        changed_keys |= cur_keys ^ buttons;
    }
    // ADC
    ++phase;
    if (phase == 3)
    {
        phase = 0;
        ++col_index;
        if (col_index == 7)
        {
            col_index = 0;
            done = true;
            working_pixels = pixs;
            if (debounce) --debounce;
            if (changed_keys)
            {
                cur_keys ^= changed_keys;
                debounce = debounce_time;
            }
        }
    }
    GPIOA->BSHR = 1<<31;    // Reset PA15 to '0'
}
