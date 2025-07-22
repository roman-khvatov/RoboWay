#include "../../../common/interface.h"
#include "../Peripheral/inc/ch32v20x.h"
#include "../Core/core_riscv.h"

Pixels pixs;  // Working image of screen (to be updated by top level code)

static Pixels  working_pixels; // Copy of pixels to output to LED
static uint8_t col_index;     // Column index (scans by them) 0-7
static uint8_t phase;         // Scan phase: 0-2. In phase 0 pixels from 'br1' emited, in phase 1 pixels from 'br2' emited, in phase 2 no pixel update performed, so 'br2' still intact

static uint8_t cur_keys;      // Real current state of pressed keys
static uint8_t changed_keys;  // All keys which state changed from 'cur_keys' during current scan cycle
static uint8_t debounce;      // Debounce downcounter. If not zero all key scan suppressed
static uint8_t active_keys;   // Copy of 'cur_keys' but with posibility to top level code shut down separate bits from 1 to 0 (depress them)

static constexpr int debounce_time = 5; // How many full scans perform before unlocking keyboard

static uint16_t led_voltage[10];  // LED voltage DMA buffer (2 x 1+4 samples accumulated)
static uint8_t  leds_to_sample;   // Bitset of LED to sample. Bit 0 - samle high LED cluster, bit 1 - sample low LED cluster
static int16_t  adc_calibration = 0;

// Status of LED voltage sampling
enum class LEDVoltageReq : uint8_t {
    Idle,       // Do nothing
    Request,    // Sampling request got
    InProgress, // Sampling started (in progress)
    Ready       // Sampling done
};

static volatile LEDVoltageReq request_led_voltages; // Current LED sampling status

static volatile bool done; // Set to 'true' when LED scan cycle done
//////////////////////////////////////////////////////////////////////////////////////////

static void dma_init()
{
    DMA_InitTypeDef DMA_InitStructure = {0};
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&ADC1->RDATAR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)led_voltage;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = 10;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
}

void OurPlatformInit()
{
    // ClockInit
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_SPI1|RCC_APB2Periph_ADC1|RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2|RCC_APB1Periph_SPI2, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1|RCC_AHBPeriph_CRC, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div16);

    // IO init
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    // Buttons
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // ADC
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // LED OE
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIOB->BSHR = 1<<11;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    //PA15 - InInt indicator
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIOA->BSHR = 1<<31;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // SPI1 GPIO
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5|GPIO_Pin_3; // CLK + MOSI
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_PinRemapConfig(GPIO_Remap_SPI1, ENABLE);

    // SPI2 GPIO
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_15; // CLK + CS + MOSI
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // ADC init

    // ADC calibration
    // SPI init (2 ch)
    // Zero SPI chans (turn off LEDs)
    // TIM3 init (sys clock)
    // TIM2 init (PWM)
    // SysTick init (RND)
    // CRC init
    // EXTI0-EXTI7 init
    // EI: EXTI0-EXTI7 and TIM3
    // LED OE on
}

//// LED Voltage sampling external interface

// Request LED Voltage sampling
void led_sence_start() {dma_init(); request_led_voltages = LEDVoltageReq::Request;}

// Test is LED Voltage sampler done sampling
bool led_sence_ready() {return request_led_voltages == LEDVoltageReq::Ready;}

std::pair<uint16_t, uint16_t> led_sence_get()
{
    std::pair<uint16_t, uint16_t> result;

    auto sum = [](int idx)
    {
        uint16_t result = 0;
        for(int i=0; i<4; ++i)
        {
            uint16_t v = led_voltage[idx+i];
            int16_t val = v + adc_calibration;
            if (val < 0 || v == 0) val = 0; else
            if (val > 4095 || v == 4095) val = 4095;
            result += val
        }
        return result >> 2;
    };

    result.first = (leds_to_sample & 1) ? sum(1) : 0xFFFF;
    result.second = (leds_to_sample & 2) ? sum(6): 0xFFFF;
    request_led_voltages = LEDVoltageReq::Idle;
    return result;
}

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


//    ADC_RegularChannelConfig(ADC1, get_active_adc() ? 8 : 9, 1, ADC_SampleTime_1Cycles5);

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
        leds_to_sample = 0;
        if (working_pixels.br2[0]&1) leds_to_sample |= 1;
        if (working_pixels.br2[8]&1) leds_to_sample |= 2;
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    }
    if (phase == 0 && request_led_voltages == LEDVoltageReq::InProgress)
    {
        request_led_voltages = LEDVoltageReq::Ready;
        ADC_SoftwareStartConvCmd(ADC1, DISABLE);
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
