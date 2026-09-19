def get_definitions(bool IntModule):
    uulp_pin('BTN1', 0, Input)
    uulp_pin('BTN2', 2, Input)                
    pin('BTN3', 27, Input, Pullup)

    pins_group('HumidityA', [ulp_pin(x, Output) for x in (0, 8, 9)])

    pin('HumidityOE', 57, Output)

    hum_in = ulp_pin(4, Input, Pulldown)

    with Promise() as res:
        opamp2 = opamp(2, inp=ulp_pin('#LeakDetector', 5), inm=res)
        res.set(resistor('OPAMP2_RES', 2, left=Gnd, right=opamp2))
    comp(1, inp=opamp2, inm=scaler('SCALER'))

    adc_inputs = [ulp_pin(6)]
    ulp_pin('EN9V', 7, Output)

    pin('LED1', 28, Output, Strength=12)
    pin('LED2', 55, Output, Strength=12)

    uart485_tx = pin(7, Output)
    uart('RS485', 1, rx=pin(6, Input, Pullup), tx=uart485_tx, mode='9600 8P1')
    ulp_pin('RS485_OE', 2, Output)

    ulp_uart('DBG_UART', rx=pin(8, Input), tx=pin(9, Input), mode='9600 8N1')

    spi_mosi = pin(12, Output)
    if IntModule:
        spi_miso = pin(11, Input)
        spi_cs2 = pin('#eInkCS', 15, Output)
    spi_clk = pin(52, Output)
    spi_cs1 = pin('#ValvesCS', 10, Output)

    if IntModule:
        ssi_mst('SPI', clock='10M', mosi=spi_mosi, miso=spi_miso, clk=spi_clk, cs1=spi_cs1, cs2=spi_cs2)
    else:
        ssi_mst('SPI', clock='10M', mosi=spi_mosi, clk=spi_clk, cs1=spi_cs1)

        with Promise() as res:
            opamp3 = opamp(3, inp=pin('#AccSence', 29), inm=res)
            res.set(resistor('OPAMP3_RES', 3, left=dac('DAC3V'), right=opamp3))
        adc_inputs.append(opamp3)

    adc('ADC', inp=adc_inputs, ref=aux_ldo('AUX_LDO'))

    if IntModule:
        buzzer = pin(29, Output)
        en = pin(30, Output)
        with AlternativeGroup('TimerMode'):
            with Alternative('BuzzerActive'):
                sct('SCT', output=buzzer, mode=FreeRun)
                en.set(True)
            with Alternate('HumMeasure'):
                sct('SCT', input=hum_in, mode=InCount)
                buzzer.set(False)
                en.set(False)
            with Alternative('Off'):
                buzzer.set(False)
                en.set(False)

        ulp_pin('eInkBUSY', 1, Input)
        pin('eInkDC', 54, Output)
        pin('eInkReset', 56, Output)
    else:
        ulp_pin('AccSenceEN', 1, Output)
        sct('SCT', input=hum_in, mode=InCount)

    with Alternative('FreqCalibrate'):
        uart485_tx.connect(pwm(0, Freq='10K', D=50))
