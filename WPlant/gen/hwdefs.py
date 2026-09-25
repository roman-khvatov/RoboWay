from typing import *
from hwfw import *

K = 1000
M = K*K

Input = Entity('Input')
Output = Entity('Output')
Pullup = Entity('Pullup')
Pulldown = Entity('Pulldown')
FreeRun = Entity('FreeRun')
InCount = Entity('InCount')
AltFunc = Entity('AltFunc')

Gnd = Item()

@dataclass
class PinSet:
    pin: 'AnyPin'
    value: bool

    def __str__(self):
        return f'{self.pin} <= {self.value}'

class AnyPin(Item):
    name: str = ''
    index: int

    default: Optional[bool]
    
    def __lshift__(self, value: bool):
        self.owner.root.record_action(PinSet(self, value))

"""
Common for all:
    sl_status_t sl_gpio_driver_init(void)
        Includes:
            sl_si91x_gpio_driver_enable_clock


Common Pins setup:
    static __INLINE void sl_gpio_clear_pin_output(sl_gpio_port_t port, uint8_t pin)
    static __INLINE void sl_gpio_set_pin_output(sl_gpio_port_t port, uint8_t pin)


    sl_status_t sl_gpio_set_configuration(sl_si91x_gpio_pin_config_t pin_config)
        sl_si91x_gpio_pin_config_t:
            port_pin
                port  SL_GPIO_PORT_A/SL_GPIO_PORT_B/SL_GPIO_PORT_C/SL_GPIO_PORT_D/SL_GPIO_ULP_PORT/SL_GPIO_UULP_PORT
                pin   <N>
            direction GPIO_OUTPUT(0)/GPIO_INPUT(1)
        Calls:
           HP:
            sl_si91x_gpio_driver_enable_host_pad_selection/sl_si91x_gpio_driver_enable_pad_selection
            sl_si91x_gpio_driver_enable_pad_receiver
            sl_gpio_driver_set_pin_mode(mode-0)
            sl_si91x_gpio_set_pin_direction
           ULP:
            sl_si91x_gpio_driver_enable_pad_selection
            sl_si91x_gpio_driver_enable_ulp_pad_receiver
            sl_gpio_driver_set_pin_mode(mode-0)
            sl_si91x_gpio_set_pin_direction
           UULP:
            sl_si91x_gpio_driver_select_uulp_npss_receiver
            sl_si91x_gpio_driver_set_uulp_npss_pin_mux(mode-0)
            sl_si91x_gpio_set_uulp_npss_direction
            
"""

class UulpPin(AnyPin):
    pin_mode: Optional[List] = List(Input, Output, AltFunc)  # AltFunc mode deduced automatically from connection
    alt_connection: Optional[Item] = None

    def set_alt_mode(self, who: Item, name: str):
        self.pin_mode = AltFunc
        self.alt_connection = who
        self.alt_connection_name = name

"""
    UULP Pins setup

    sl_status_t sl_si91x_gpio_driver_set_uulp_npss_pin_mux (uint8_t pin, sl_si91x_uulp_npss_mode_t mode)

    sl_status_t sl_si91x_gpio_driver_set_uulp_npss_wakeup_interrupt (uint8_t npssgpio_interrupt)
    sl_status_t sl_si91x_gpio_driver_clear_uulp_npss_wakeup_interrupt (uint8_t npssgpio_interrupt)

    sl_status_t sl_si91x_gpio_driver_set_uulp_pad_configuration (uulp_pad_config_t * pad_config) [[ or sl_si91x_gpio_set_uulp_pad_configuration ]]
      uulp_pad_config_t:
          uint8_t gpio_padnum;                 ///< UULP GPIO pin number
          sl_si91x_uulp_npss_mode_t mode;      ///< UULP GPIO mode
          sl_si91x_gpio_receiver_t receiver;   ///< UULP GPIO PAD receiver
          sl_si91x_gpio_direction_t direction; ///< UULP GPIO direction of PAD
          sl_si91x_gpio_pin_value_t output;    ///< UULP GPIO value driven on PAD
          sl_si91x_gpio_uulp_pad_t pad_select; ///< UULP GPIO PAD selection
          sl_si91x_gpio_polarity_t polarity;   ///< UULP GPIO Polarity


"""

class Pin(UulpPin):
    pullups: Optional[List] = List(Pullup, Pulldown)

    strength: Optional[int]  # UlpPin ?

"""
    HP Pins:

    sl_status_t sl_si91x_gpio_driver_select_pad_driver_strength(uint8_t gpio_num, sl_si91x_gpio_driver_strength_select_t strength);
        * @param[in]    strength    -  Drive strength selector(E1,E2) of type
        *                    sl_si91x_gpio_driver_strength_select_t (GPIO driver strength select):
        *  -                  0, for two_milli_amps   (E1=0,E2=0)
        *  -                  1, for four_milli_amps  (E1=0,E2=1)
        *  -                  2, for eight_milli_amps (E1=1,E2=0)
        *  -                  3, for twelve_milli_amps(E1=1,E2=1)
    sl_status_t sl_si91x_gpio_driver_enable_pad_power_on_start(uint8_t gpio_num, sl_si91x_gpio_pos_t pos);
    sl_status_t sl_si91x_gpio_driver_select_pad_schmitt_trigger(uint8_t gpio_num, sl_si91x_gpio_schmitt_trig_t schmitt_trig);
    sl_status_t sl_si91x_gpio_driver_select_pad_driver_disable_state(uint8_t gpio_num, sl_si91x_gpio_driver_disable_state_t disable_state);
        * @param[in]    disable_state    -  Driver disable state of type
        *                  sl_si91x_gpio_driver_disable_state_t.
        *                 Possible values are
        * 
        *  -                  0, for HiZ       (P1=0,P2=0)
        *  -                  1, for Pull-up   (P1=0,P2=1)
        *  -                  2, for Pull-down (P1=1,P2=0)
        *  -                  3, for Repeater  (P1=1,P2=1)

    sl_status_t sl_gpio_driver_set_pin_mode(sl_gpio_t *gpio, sl_gpio_mode_t mode, uint32_t output_value);   [[ or sl_gpio_set_pin_mode(sl_gpio_port_t port, uint8_t pin, sl_gpio_mode_t mode, uint32_t output_value) ]]
"""

class UlpPin(Pin):
    pass

"""
    ULP pins:

    sl_status_t sl_si91x_gpio_driver_select_ulp_pad_slew_rate(uint8_t gpio_num, sl_si91x_gpio_slew_rate_t slew_rate);
    sl_status_t sl_si91x_gpio_driver_select_ulp_pad_driver_strength(uint8_t gpio_num, sl_si91x_gpio_driver_strength_select_t strength);
    sl_status_t sl_si91x_gpio_driver_enable_ulp_pad_power_on_start(uint8_t gpio_num, sl_si91x_gpio_pos_t pos);
    sl_status_t sl_si91x_gpio_driver_select_ulp_pad_schmitt_trigger(uint8_t gpio_num, sl_si91x_gpio_schmitt_trig_t schmitt_trig);
    sl_status_t sl_si91x_gpio_driver_select_ulp_pad_driver_disable_state(uint8_t gpio_num, sl_si91x_gpio_driver_disable_state_t disable_state);

    sl_status_t sl_gpio_driver_set_pin_mode(sl_gpio_t *gpio, sl_gpio_mode_t mode, uint32_t output_value);  [[ or sl_gpio_set_pin_mode(sl_gpio_port_t port, uint8_t pin, sl_gpio_mode_t mode, uint32_t output_value) ]]
"""


"""
ULP <-> HP modes:
    sl_status_t sl_si91x_gpio_driver_set_soc_peri_on_ulp_pin_mode(sl_gpio_t *gpio, sl_gpio_mode_t mode);
    sl_status_t sl_si91x_gpio_driver_set_ulp_peri_on_soc_pin_mode(sl_gpio_t *gpio, sl_gpio_mode_t mode);

"""

class Sct(Item):
    name: str = ''

    mode: List = List(FreeRun, InCount)    
    input: Optional[Item]
    output: Optional[Item]

class SsiMst(Item):
    name: str = ''

    clock: int
    mosi: Optional[Item]
    miso: Optional[Item]
    clk: Item
    cs0: Optional[Item]
    cs1: Optional[Item]
    cs2: Optional[Item]
    cs3: Optional[Item]

class PinsGroup(Item):
    name: str

    _no_pins_change = True

    pins: list[Item]

class Opamp(Item):
    name: str = ''
    index: int

    inp: Item
    inm: Item

class Comp(Item):
    name: str = ''
    index: int

    inp: Item
    inm: Item

class Resistor(Item):
    name: str = ''
    index: int

    left: Item
    right: Item

class Scaller(Item):
    name: str = ''

class Uart(Item):
    name: str = ''
    index: int
    
    rx: Optional[Item]
    tx: Optional[Item]

    mode: str


class UlpUart(Uart):
    pass

class Dac(Item):
    name: str = ''

class Adc(Item):
    name: str = ''

    inp: list[Item]
    ref: Item

class AuxLdo(Item):
    name: str = ''

class Pwm(Item):
    name: str = ''
    index: int

    Freq: int
    D: int
    output: Optional[Item]
