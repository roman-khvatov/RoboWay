#include "../../../common/interface.h"

Pixels pixs;

static Pixels working_pixels;
static uint8_t col_index;
static uint8_t phase;

static uint8_t cur_keys;
static uint8_t changed_keys;
static uint8_t debounce;

static uint16_t led_voltage;
static uint8_t led_cnt;
static bool led_index;

static uint16_t leds_voltage[2];

static bool done;
\
// SPI1 - Cols (Remapping, PB3, PB5)
// SPI3 - Rows (???)

void timer_iterrupt()
{
    // Pixels
    uint16_t row=0;
    uint8_t col = ~(1<<col_index);
    if (phase!=2)
    {
        uint8_t* array = phase ? working_pixels.br2 : working_pixels.br1;
        row = array[col_index] | (array[col_index+8]<<8);
        // SPI1 <- col
        // SPI3 <- row
    }
    if (!debounce)
    {
        uint8_t buttons = ~PORTA();
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
            if (debounce) --debounce;
            if (changed_keys)
            {
                cur_keys ^= changed_keys;
                debounce = 5;
            }
        }
    }
}
