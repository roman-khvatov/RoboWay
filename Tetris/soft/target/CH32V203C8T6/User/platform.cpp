#include <utility>

#include "../common/interface.h"
#include "ch32v20x.h"
#include "../Core/core_riscv.h"

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

extern "C" void SPI2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void SPI2_IRQHandler()
{
    GPIO_SetBits(GPIOB, GPIO_Pin_12);
	(void)SPI2->DATAR;
}

static constexpr int period1 = 1125*tick_time/256-1; // On PB1 (at HCLK)
static constexpr int period2 = 1125*3*tick_time/256-1;
static constexpr int period3 = 1125*28*tick_time/256-1;


void OurPlatformInit()
{
    // ClockInit
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_SPI1|RCC_APB2Periph_ADC1|RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2|RCC_APB1Periph_TIM3|RCC_APB1Periph_SPI2, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1|RCC_AHBPeriph_CRC, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

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
    GPIO_SetBits(GPIOB, GPIO_Pin_11);
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB, GPIO_Pin_11);

    //PA15 - InInt indicator
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOA, GPIO_Pin_15);

    // SPI1 GPIO
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5|GPIO_Pin_3; // CLK + MOSI
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_PinRemapConfig(GPIO_Remap_SPI1, ENABLE);

    // SPI2 GPIO
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13|GPIO_Pin_15; // CLK + <CS> + MOSI
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // SPI2 CS - manual
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB, GPIO_Pin_12);

    // PB10 - TIM2_CH3_2 (alt mapping)
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_PinRemapConfig(GPIO_FullRemap_TIM2, ENABLE);

    // ADC init
    ADC_InitTypeDef  ADC_InitStructure = {0};
    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 10;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);

    // ADC calibration
    ADC_BufferCmd(ADC1, DISABLE); //disable buffer
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    adc_calibration = Get_CalibrationValue(ADC1);
    ADC_BufferCmd(ADC1, ENABLE); //enable buffer

    for(int i=0; i<5; ++i)
    {
        ADC_RegularChannelConfig(ADC1, 1, i+1, ADC_SampleTime_239Cycles5);
        ADC_RegularChannelConfig(ADC1, 0, i+6, ADC_SampleTime_239Cycles5);
    }

    // SPI1 init
    SPI_InitTypeDef SPI_InitStructure={0};
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; // SPI1 on PB2 (at HCLK/2)
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init( SPI1, &SPI_InitStructure );

	SPI_Cmd( SPI1, ENABLE );

    // SPI2 init
//  SPI_SSOutputCmd(SPI2, ENABLE );

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
//	SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // SPI2 on PB1 (at HCLK)
	SPI_Init( SPI2, &SPI_InitStructure );

    SPI_Cmd( SPI2, ENABLE );

    // TIM3 init (sys clock) & Int
    NVIC_InitTypeDef NVIC_InitStructure={0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure={0};

    TIM_TimeBaseInitStructure.TIM_Period = period3; // On PB1 (at HCLK)
    TIM_TimeBaseInitStructure.TIM_Prescaler = 127;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit( TIM3, &TIM_TimeBaseInitStructure);

    TIM_ClearITPendingBit( TIM3, TIM_IT_Update );

    NVIC_InitStructure.NVIC_IRQChannel =TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority =1;
    NVIC_InitStructure.NVIC_IRQChannelCmd =ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = SPI2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;
    NVIC_Init(&NVIC_InitStructure);

    SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);

    // Zero SPI chans (turn off LEDs)
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);
    SPI1->DATAR = 0xFF;
    SPI2->DATAR = 0;

    // TIM2 init (PWM)
	TIM_TimeBaseInitStructure.TIM_Period = 0xFFFF; // On PB1 (at HCLK)
	TIM_TimeBaseInitStructure.TIM_Prescaler = 0;
	TIM_TimeBaseInit( TIM2, &TIM_TimeBaseInitStructure);

    TIM_OCInitTypeDef TIM_OCInitStructure={0};
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 34734; // 1.75V on output
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init( TIM2, &TIM_OCInitStructure );

	TIM_CtrlPWMOutputs(TIM2, ENABLE );
	TIM_OC3PreloadConfig( TIM2, TIM_OCPreload_Disable );
	TIM_ARRPreloadConfig( TIM2, ENABLE );
	TIM_Cmd( TIM2, ENABLE );

    // SysTick init (RND)
    SysTick->CTLR =5;

    // CRC init
    CRC_ResetDR();

    // EXTI0-EXTI7 init & EI
    for(uint8_t pin_source = 0; pin_source < 8; ++pin_source) GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, pin_source);

    EXTI_InitTypeDef EXTI_InitStructure = {0};

    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Line = EXTI_Line0|EXTI_Line1|EXTI_Line2|EXTI_Line3|EXTI_Line4|EXTI_Line5|EXTI_Line6|EXTI_Line7;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    for(auto intn: {EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn, EXTI4_IRQn, EXTI9_5_IRQn})
    {
        NVIC_InitStructure.NVIC_IRQChannel = intn;
        NVIC_Init(&NVIC_InitStructure);
    }

    // EI: TIM3
    TIM_Cmd( TIM3, ENABLE );

    // LED OE on
    GPIO_ResetBits(GPIOB, GPIO_Pin_11);
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
            CRC->DATAR = v;
            int16_t val = v + adc_calibration;
            if (val < 0 || v == 0) val = 0; else
            if (val > 4095 || v == 4095) val = 4095;
            result += val;
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
    GPIO_SetBits(GPIOA, GPIO_Pin_15); // Set 'InInt' indicator

    // Pixels
    uint16_t row=0;
    uint8_t col = ~(1<<col_index);
    uint16_t nxt_period = period3;

    uint16_t row2 = working_pixels.br2[col_index] | (working_pixels.br2[col_index+8]<<8);
    uint16_t row1 = working_pixels.br1[col_index] | (working_pixels.br1[col_index+8]<<8);

    switch(phase)
    {
        case 0: // Br - 3
            nxt_period = period3;
            row = row1&row2;
            break;
        case 1: // Br - 1
            nxt_period = period1;
            row = row1|row2;
            break;
        case 2: // Br - 2
            nxt_period = period2;
            row = row2;
            break;
        default: ;
    }
    TIM_SetAutoreload(TIM3, nxt_period);
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);
    SPI1->DATAR = col;
    SPI2->DATAR = row;
/*
    if (phase!=2)
    {
        uint8_t* array = phase ? working_pixels.br2 : working_pixels.br1;
        row = array[col_index] | (array[col_index+8]<<8);
        GPIO_ResetBits(GPIOB, GPIO_Pin_12);
        SPI1->DATAR = col;
        SPI2->DATAR = row;
    }
*/
    if (!debounce)
    {
        uint8_t buttons = ~GPIOA->INDR;
        changed_keys |= cur_keys ^ buttons;
    }
    // ADC
    if (phase == 0 && request_led_voltages == LEDVoltageReq::Request && ((working_pixels.br2[0]&1) || (working_pixels.br2[8]&1)))
    {
        request_led_voltages = LEDVoltageReq::InProgress;
        leds_to_sample = 0;
        if (working_pixels.br2[0]&1) leds_to_sample |= 1;
        if (working_pixels.br2[8]&1) leds_to_sample |= 2;
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    }
    if (phase == 1 && request_led_voltages == LEDVoltageReq::InProgress)
    {
        request_led_voltages = LEDVoltageReq::Ready;
        ADC_SoftwareStartConvCmd(ADC1, DISABLE);
    }
    ++phase;
    if (phase == 3)
    {
        phase = 0;
        ++col_index;
        if (col_index == 8)
        {
            col_index = 0;
            done = true;
            working_pixels = pixs;
            if (debounce) --debounce; else
            if (changed_keys)
            {
                cur_keys ^= changed_keys;
                active_keys = (active_keys & ~changed_keys) | (cur_keys & changed_keys);
                debounce = debounce_time;
                changed_keys = 0;
            }
        }
    }
    TIM_ClearITPendingBit( TIM3, TIM_IT_Update );
    GPIO_ResetBits(GPIOA, GPIO_Pin_15); // Reset 'InInt' indicator
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

#define H(nm, ln) \
extern "C" void nm() __attribute__((interrupt("WCH-Interrupt-fast"))); \
void nm() {seed_rnd_value(); EXTI_ClearITPendingBit(ln);}

H(EXTI0_IRQHandler, EXTI_Line0)
H(EXTI1_IRQHandler, EXTI_Line1)
H(EXTI2_IRQHandler, EXTI_Line2)
H(EXTI3_IRQHandler, EXTI_Line3)
H(EXTI4_IRQHandler, EXTI_Line4)
H(EXTI9_5_IRQHandler, EXTI_Line5|EXTI_Line6|EXTI_Line7)
