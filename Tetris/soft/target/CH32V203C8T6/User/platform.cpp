#include "../../../common/interface.h"
#include "../Peripheral/inc/ch32v20x.h"
#include "../Core/core_riscv.h"

Pixels pixs;  // Working image of screen (to be updated by top level code)

static Pixels working_pixels; // Copy of pixels to output to LED
static uint8_t col_index;     // Column index (scans by them) 0-7
static uint8_t phase;         // Scan phase: 0-2. In phase 0 pixels from 'br1' emited, in phase 1 pixels from 'br2' emited, in phase 2 no pixel update performed, so 'br2' still intact

static uint8_t cur_keys;      // Real current state of pressed keys
static uint8_t changed_keys;  // All keys which state changed from 'cur_keys' during current scan cycle
static uint8_t debounce;      // Debounce downcounter. If not zero all key scan suppressed
static uint8_t active_keys;   // Copy of 'cur_keys' but with posibility to top level code shut down separate bits from 1 to 0 (depress them)

static constexpr int debounce_time = 5; // How many full scans perform before unlocking keyboard

static uint16_t led_voltage;  // LED voltage accumulator (4 samples accumulated)
static uint8_t led_cnt;       // LED voltage sample number. Sample #0 discarded, sammples #1-4 accumulated into 'led_voltage'
static uint8_t leds_to_sample; // Bitset of LED to sample. Bit 0 - samle high LED cluster, bit 1 - sample low LED cluster

// Status of LED voltage sampling
enum class LEDVoltageReq : uint8_t {
    Idle,       // Do nothing
    Request,    // Sampling request got
    InProgress, // Sampling started (in progress)
    Ready       // Sampling done
};

uint16_t leds_voltage[2];   // Output voltages for LED sampling. 0 means that this LED cluster wasn't sampled
static volatile LEDVoltageReq request_led_voltages; // Current LED sampling status

static volatile bool done; // Set to 'true' when LED scan cycle done


//// LED Voltage sampling external interface

// Request LED Voltage sampling
void led_sence_start() {request_led_voltages = LEDVoltageReq::Request;}
// Reset status of LED Voltage samplier to Idle
void led_sence_off()   {request_led_voltages = LEDVoltageReq::Idle;}
// Test is LED Voltage sampler done sampling
bool led_sence_ready() {return request_led_voltages == LEDVoltageReq::Ready;}


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

PB0 - Rows 8-15    Ch1   ADC_IN8
PB1 - Rows 0-7     Ch0   ADC_IN9

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

int get_active_adc()
{
    if (leds_to_sample&1) return 0;
    if (leds_to_sample&2) return 1;
    return -1;
}

inline void start_adc()
{
    ADC_Cmd(ADC1, ENABLE);
}

void select_adc_channel()
{
    led_voltage=0;
    led_cnt=0;
    leds_to_sample=0;

    ADC_RegularChannelConfig(ADC1, get_active_adc() ? 8 : 9, 1, ADC_SampleTime_1Cycles5);
}

void seed_rnd_value(uint32_t val)
{
    CRC->DATAR = val;
    CRC->DATAR = SysTick->CNT;
}

void seed_rnd_value()
{
    CRC->DATAR = SysTick->CNT;
}

uint32_t get_random()
{
    seed_rnd_value();
    return CRC->DATAR;
}

extern "C" void TIM3_IRQHandler() __attribute__((interrupt("WCH-Interrupt-fast")));
extern "C" void ADC1_2_IRQHandler() __attribute__((interrupt("WCH-Interrupt-fast")));

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
    if (phase == 2 && request_led_voltages == LEDVoltageReq::Request && ((working_pixels.br2[0]&1) || (working_pixels.br2[8]&1)))
    {
        request_led_voltages = LEDVoltageReq::InProgress;
        if (working_pixels.br2[0]&1) leds_to_sample |= 1;
        if (working_pixels.br2[8]&1) leds_to_sample |= 2;
        leds_voltage[0]=leds_voltage[1]=0;
        switch_adc_channel();
        start_adc();        
    }
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
                active_keys = (active_keys & ~changed_keys) | (cur_keys & changed_keys);
                debounce = debounce_time;
            }
        }
    }
    GPIOA->BSHR = 1<<31;    // Reset PA15 to '0'
}

void ADC1_2_IRQHandler()
{
    uint16_t adc = ADC_GetConversionValue(ADC1);
    seed_rnd_value(adc);
    if (!led_cnt) {led_cnt=1; start_adc(); return;}
    led_voltage += adc;
    ++led_cnt;
    if (led_cnt == 5)
    {
        leds_voltage[get_active_adc()] = led_voltage >> 2;
        leds_to_sample &= leds_to_sample - 1;
        if (leds_to_sample)
        {
            select_adc_channel();
            start_adc();
        }
        else 
        {
            request_led_voltages = LEDVoltageReq::Ready;
        }
    }
    else
    {
        start_adc();
    }
}

uint8_t read_key()
{
    while(!done)
    {
        __WFI();
    }
    done = false;
    return active_keys;
}

void clr_keys(uint8_t keys)
{
    __disable_irq();
    active_keys &= ~keys;
    __enable_irq();
}

#define H(nm) \
extern "C" void nm() __attribute__((interrupt("WCH-Interrupt-fast"))); \
void nm() {seed_rnd_value();}

H(EXTI0_IRQHandler)
H(EXTI1_IRQHandler)
H(EXTI2_IRQHandler)
H(EXTI3_IRQHandler)
H(EXTI4_IRQHandler)
H(EXTI9_5_IRQHandler)
