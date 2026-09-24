from hwdefs import *

def get_definitions(IntModule: bool):
    UulpPin('BTN1', 0, Input)
    UulpPin('BTN2', 2, Input)                
    Pin('BTN3', 27, Input, Pullup)

    PinsGroup('HumidityA', [UlpPin(x, Output) for x in (0, 8, 9)])

    Pin('HumidityOE', 57, Output)

    hum_in = UlpPin(4, Input, Pulldown)

    with Wire() as res:
        opamp2 = Opamp(2, inp=UlpPin('#LeakDetector', 5), inm=res)
        res << Resistor('OPAMP2_RES', 2, left=Gnd, right=opamp2)
    Comp(1, inp=opamp2, inm=Scaller('SCALER'))

    adc_inputs = [UlpPin(6)]
    UlpPin('EN9V', 7, Output)

    Pin('LED1', 28, Output, strength=12)
    Pin('LED2', 55, Output, strength=12)

    uart485_tx = Pin(7, Output)
    Uart('RS485', 1, rx=Pin(6, Input, Pullup), tx=uart485_tx, mode='9600 8P1')
    UlpPin('RS485_OE', 2, Output)

    UlpUart('DBG_UART', rx=Pin(8, Input), tx=Pin(9, Input), mode='9600 8N1')

    spi_mosi = Pin(12, Output)
    if IntModule:
        spi_miso = Pin(11, Input)
        spi_cs2 = Pin('#eInkCS', 15, Output)
    spi_clk = Pin(52, Output)
    spi_cs1 = Pin('#ValvesCS', 10, Output)

    if IntModule:
        SsiMst('SPI', clock=10*M, mosi=spi_mosi, miso=spi_miso, clk=spi_clk, cs1=spi_cs1, cs2=spi_cs2)
    else:
        SsiMst('SPI', clock=10*M, mosi=spi_mosi,                clk=spi_clk, cs1=spi_cs1)

        with Wire() as res:
            opamp3 = Opamp(3, inp=Pin('#AccSence', 29), inm=res)
            res << Resistor('OPAMP3_RES', 3, left=Dac('DAC3V'), right=opamp3)
        adc_inputs.append(opamp3)

    Adc('ADC', inp=adc_inputs, ref=AuxLdo('AUX_LDO'))

    if IntModule:
        buzzer = Pin(29, Output)
        en = Pin(30, Output)
        # Switch Alternative: TimerMode can be Off/BuzzerActive/HumMeasure
        with Alternative('TimerMode:Off'): # First alternative is default
            buzzer << False
            en << False
        with Alternative('TimerMode:BuzzerActive'):
            Sct('SCT', output=buzzer, mode=FreeRun)
            en << True
        with Alternative('TimerMode:HumMeasure'):
            Sct('SCT', input=hum_in, mode=InCount)
            buzzer << False
            en << False

        UlpPin('eInkBUSY', 1, Input)
        Pin('eInkDC', 54, Output)
        Pin('eInkReset', 56, Output)
        UlpPin('TFSelect', 11, Output)
    else:
        UlpPin('AccSenceEN', 1, Output)
        Sct('SCT', input=hum_in, mode=InCount)

    with Alternative('!FreqCalibrate'): # BiStable alternative - can be turned on and off
        Pwm(0, Freq=10*K, D=50, output=uart485_tx)

import sys

with Holder() as h:
    get_definitions(True)
    print('===== IntModule ========')
    h.dump(sys.stdout)
with Holder() as h:
    get_definitions(False)
    print('===== ExtModule ========')
    h.dump(sys.stdout)
