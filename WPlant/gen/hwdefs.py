import sys

from typing import *
from hwfw import *

K = 1000
M = K*K

Input = Entity('Input')
Output = Entity('Output')
HiZ = Entity('HiZ')
Pullup = Entity('Pullup')
Pulldown = Entity('Pulldown')
Repeater = Entity('Repeater')
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

    @property
    def c_code(self) -> str:
        return self.pin.get_c_code_for_pin_set(self.value)

class AnyPin(Item):
    name: str = ''
    index: int
    pin_mode: Optional[List] = List(Input, Output, AltFunc)  # AltFunc mode deduced automatically from connection
    alt_connection: Optional[Item] = None

    default: Optional[bool]

    def get_c_code_for_pin_set(self, value: bool) -> str:
        func = 'set' if value else 'clear'
        return f'sl_gpio_{func}_pin_output({self._port()[0]}, {self.index})'

    def alt_mode_mux_raw(self, who: Item, name: str):
        return find_mux_chain2(f'{self._port()[1]}_{self.index}', who.get_canonical_name(name))

    def alt_mode_mux(self, who: Item, name: str):
        return self.alt_mode_mux_raw(who, name).get_mux_string()

    @property
    def default_mux(self) -> str:
        return self.mux or self.get_setup_lines_base()[0]

    def set_alt_mode(self, who: Item, name: str):
        assert self.pin_mode != AltFunc, f'Pin {self} already connected'
        self.pin_mode = AltFunc
        self.alt_connection = who
        self.alt_connection_name = name
        self.mux = self.alt_mode_mux(who, name)
        if hasattr(who, '_analog') and hasattr(self, 'pullups'):
            self.pullups = HiZ
    
    def __lshift__(self, value: bool):
        self.owner.root.record_action(PinSet(self, value))

    def _port(self) -> tuple[str]:
        assert False

    @property
    def _direction(self) -> str:
        return 'GPIO_OUTPUT' if self.pin_mode is Output else 'GPIO_INPUT'

    @property
    def setup_lines(self) -> list[str]:
        return self.get_setup_lines()

    def get_setup_lines_base(self) -> list[str]:
        return [f'sl_gpio_set_configuration(sl_si91x_gpio_pin_config_t{{.port_pin={{.port={self._port()[0]}, .pin={self.index}}}, .direction={self._direction}}});']

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
    def _port(self) -> tuple[str]:
        return ('SL_GPIO_UULP_PORT', 'UULP_VBAT_GPIO')

    @property
    def setup_lines(self) -> list[str]:
        return self.get_setup_lines_base() + [
            '{',
            '    static const uulp_pad_config_t cfg = {',
           f'        .gpio_padnum = {self.index},',
            '        .mode = NPSS_GPIO_PIN_MUX_MODE0,',    # No other modes used in our HW
           f'        .receiver = {'GPIO_RECEIVER_EN' if self.pin_mode is Input else 'GPIO_RECEIVER_DS'},',
           f'        .direction = {self._direction},',
           f'        .output = {'GPIO_PIN_SET' if self.default else 'GPIO_PIN_CLEAR'},',
            '        .pad_select = GPIO_PAD_M4,',
            '        .polarity = GPIO_POLARITY_0};',
            '    sl_si91x_gpio_driver_set_uulp_pad_configuration(&cfg);',
            '}']

"""
    UULP Pins setup

    sl_status_t sl_si91x_gpio_driver_set_uulp_npss_pin_mux (uint8_t pin, sl_si91x_uulp_npss_mode_t mode)

    sl_status_t sl_si91x_gpio_driver_set_uulp_npss_wakeup_interrupt (uint8_t npssgpio_interrupt)
    sl_status_t sl_si91x_gpio_driver_clear_uulp_npss_wakeup_interrupt (uint8_t npssgpio_interrupt)

    sl_status_t sl_si91x_gpio_driver_set_uulp_pad_configuration (uulp_pad_config_t * pad_config) [[ or sl_si91x_gpio_set_uulp_pad_configuration ]]
      uulp_pad_config_t:
          uint8_t gpio_padnum;                 ///< UULP GPIO pin number
          sl_si91x_uulp_npss_mode_t mode;      ///< UULP GPIO mode               <mode - mux>
          sl_si91x_gpio_receiver_t receiver;   ///< UULP GPIO PAD receiver       <reciever enable>
          sl_si91x_gpio_direction_t direction; ///< UULP GPIO direction of PAD   <in/out>
          sl_si91x_gpio_pin_value_t output;    ///< UULP GPIO value driven on PAD <for mode=0>
          sl_si91x_gpio_uulp_pad_t pad_select; ///< UULP GPIO PAD selection       <mcu=0, nwp=0>
          sl_si91x_gpio_polarity_t polarity;   ///< UULP GPIO Polarity            <wakeup polarity - 0 if low, 1 if high>


"""

class Pin(UulpPin):
    pullups: Optional[List] = List(Pullup, Pulldown, Repeater)
    strength: Optional[int]
    schmitt: Optional[bool]
    pad_pos: Optional[bool]

    def _port(self) -> tuple[str]:
        return ('SL_GPIO_PORT_A', 'GPIO')

    @property
    def setup_lines(self) -> list[str]:
        E = self._ext()
        result = self.get_setup_lines_base()
        if self.pullups is not None:
            if self.pullups is Pullup:
                mode = 'GPIO_PULLUP'
            elif self.pullups is Pulldown:
                mode = 'GPIO_PULLDOWN'
            elif self.pullups is Repeater:
                mode = 'GPIO_REPEATER'
            elif self.pullups is HiZ:
                mode = 'GPIO_HZ'
            else:
                assert False, self.pullups
            result.append(f'sl_si91x_gpio_driver_select{E}_pad_driver_disable_state({self.index}, {mode});')
        if self.schmitt is not None:
            result.append(f'sl_si91x_gpio_driver_select{E}_pad_schmitt_trigger({self.index}, {'GPIO_SCHMITT_TRIG_EN' if self.schmitt else 'GPIO_SCHMITT_TRIG_DIS'});')
        if self.strength is not None:
            mode = {
                2: 'GPIO_TWO_MILLI_AMPS',
                4: 'GPIO_FOUR_MILLI_AMPS',
                8: 'GPIO_EIGHT_MILLI_AMPS',
                12: 'GPIO_TWELVE_MILLI_AMPS'
            }.get(self.strength)
            assert mode, f'Unsupported driver strength: {self.strength} (Allowed 2/4/8/12)'
            result.append(f'sl_si91x_gpio_driver_select{E}_pad_driver_strength({self.index}, {mode});')
        if self.pad_pos is not None:
            result.append(f'sl_si91x_gpio_driver_enable{E}_pad_power_on_start({self.index}, {'GPIO_POS_EN' if self.pad_pos else 'GPIO_POS_DIS'});')
        if self.alt_connection:
            result += self._setup_mux(self.pin_mode, self.alt_connection, self.alt_connection_name)
        return result

    def _ext(self) -> str:
        return ''

    def _setup_mux(self, pin_mode: Entity, alt_connection: Item, alt_connection_name: str) -> list[str]:
        " pin_mode + alt_connection + alt_connection_name => sl_gpio_driver_set_pin_mode "
        result = []
        if hasattr(alt_connection, '_analog'):
            result.append(f'sl_si91x_gpio_driver_disable_pad_receiver({self.index});')
        return result

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
    slew_rate_high: Optional[bool]

    def _port(self) -> tuple[str]:
        return ('SL_GPIO_ULP_PORT', 'ULP_GPIO')

    def _ext(self) -> str:
        return '_ulp'

    @property
    def setup_lines(self) -> list[str]:
        result = super().setup_lines
        if self.slew_rate_high is not None:
            result.append(f'sl_si91x_gpio_driver_select_ulp_pad_slew_rate({self.index}, {'GPIO_SR_HIGH' if self.slew_rate_high else 'GPIO_SR_LOW'});')
        return result

    def _setup_mux(self, pin_mode: Entity, alt_connection: Item, alt_connection_name: str) -> list[str]:
        " pin_mode + alt_connection => sl_gpio_driver_set_pin_mode "
        result = []
        if hasattr(alt_connection, '_analog'):
            result.append(f'sl_si91x_gpio_driver_disable_ulp_pad_receiver({self.index});')
        return result


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
    index: int

    mode: List = List(FreeRun, InCount)    
    input: Optional[Item]
    output: Optional[Item]
    Freq: Optional[int]

    _canonical = {
        'input': 'SCT_IN_{index}',
        'output': 'SCT_OUT_{index}'
    }

    def get_setup(self) -> list[str]:
        result = [
            'RSI_CLK_CtClkConfig(M4CLK, SCT_CLOCK_SOURCE, SCT_CLOCK_DIV_FACT, ENABLE_STATIC_CLK);',
            f'RSI_CT_SetControl(CT, SL_COUNTER{self.index}_SOFT_RESET_ENABLE|SL_COUNTER{self.index}_PERIODIC_ENABLE|SL_COUNTER{self.index}_TRIGGER_ENABLE|SL_COUNTER{self.index}_UP_DIRECTION);',
        ]
        if self.mode is FreeRun:
            assert self.Freq, 'CT in FreeRun mode must have Freq setup'
            div = 16000000 // self.Freq
            result.append(f'sl_si91x_config_timer_set_match_count(SL_COUNTER_16BIT, SL_COUNTER_{self.index}, {div});')
            result.append(f'RSI_CT_OCUConfigSet(CT, SL_COUNTER{self.index}_OCU_OUTPUT_ENABLE|SL_OCU_OUTPUT{self.index}_TOGGLE_HIGH|SL_OCU_OUTPUT{self.index}_TOGGLE_LOW);')
            result.append(f'RSI_CT_OCUHighLowToggleSelect(CT, 0, {self.index}, 2);')
            result.append(f'RSI_CT_OCUHighLowToggleSelect(CT, 1, {self.index}, 3);')
            result.append(f'{{ OCU_PARAMS_T p{{.CompareVal1_{self.index} = {div // 2}, .CompareVal2_{self.index} = {div}}}; RSI_CT_WFGComapreValueSet(CT, {self.index}, &p);}}')
            result.append(f'sl_si91x_config_timer_select_action_event(INCREMENT, SL_NO_EVENT, SL_NO_EVENT);')
        else:
            result.append(f'sl_si91x_config_timer_set_match_count(SL_COUNTER_16BIT, SL_COUNTER_{self.index}, 0xFFFF);')
            ev0, ev1 = 'SL_EVENT0_RISING_EDGE', 'SL_NO_EVENT'
            if self.index:
                ev0, ev1 = ev1, ev0
            result.append(f'sl_si91x_config_timer_select_action_event(INCREMENT, {ev0}, {ev1});')
        return result

    @property
    def hw_reset(self) -> str:
        return 'sl_si91x_config_timer_deinit();'

"""

FreeRun:
    To enable periodic mode where counter re-runs after match value was reached, set PERIODIC_EN_COUNTER_x_FRM_REG in
        CT_GEN_CTRL_SET_REG register.

    a. Enable up direction.
        i. Write COUNTER_x_UP_DOWN in CT_GEN_CTRL_SET_REG register.
    b. Disable down direction
        i. Write COUNTER_x_UP_DOWN in CT_GEN_CTRL_RESET_REG register.
    c. Write peak value to COUNTER_x_MATCH in CT_MATCH_REG register.
    e. Select the start signals either from input events by setting START_COUNTER_x_EVENT_SEL in CT_START_COUNTER_
        EVENT_SEL Register or from software triggers.

OCU:
    a. To create simple signals:
        i. Set which OCU trigger should make output_x high by configuring MAKE_OUTPUT_x_HIGH_SEL in
            CT_OCU_CTRL_REG register.
        ii. Set which OCU trigger should make output_x low by configuring MAKE_OUTPUT_x_LOW_SEL in CT_OCU_CTRL_REG
            register.
    3. Set the compare values on which OCU triggers happen
        a. Configure OCU_COMPARE_0_REG and OCU_COMPARE_1_REG in CT_OCU_COMPARE_REG for counter 0 and in
            CT_OCU_COMPARE2_REG register for counter 1

Increment:
    CT_INCREMENT_COUNTER_EVENT_SEL register
       INCREMENT_COUNTER_0_EVENT_SEL :  1 -  EVE_0_RE Event 0 Rising Edge
                                        0 -  NONE     No event selected
"""

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

    _canonical = {
        'clk': 'SSI_MST_CLK',
        'mosi': 'SSI_MST_DATA0',
        'miso': 'SSI_MST_DATA1',
        'cs0': 'SSI_MST_CS0',
        'cs1': 'SSI_MST_CS1',
        'cs2': 'SSI_MST_CS2',
        'cs3': 'SSI_MST_CS3',
    }

    def get_setup(self) -> list[str]:
        return [
            'sl_si91x_ssi_init(SL_SSI_PRIMARY_ACTIVE, &ssi_handle);',
            '{',
            '    static sl_ssi_control_config_t cfg{.bit_width=8, .device_mode=SL_SSI_MASTER_ACTIVE, .clock_mode=SL_SSI_PERIPHERAL_CPOL0_CPHA0, .baud_rate=10000000, .transfer_mode=SL_SSI_PRIMARY_SINGLE_LINE_MODE};',
            '    sl_si91x_ssi_set_configuration(ssi_handle, &cfg, 0);',
            '}'
        ]


class PinsGroup(Item):
    name: str

    _no_pins_change = True

    pins: list[Item]

    def get_setup(self) -> list[str]:
        return []
    
class Opamp(Item):
    name: str = ''
    index: int

    inp: Item
    inm: Item

    _canonical = {
        'inp': 'OPAMP{index}_IN',
        'inm': 'OPAMP{index}_IN',
    }
    _analog = True

    def get_setup(self) -> list[str]:
        res_tap_item = None

        def cvt_to_index(input: Item, input_name: str, mux: list[dict[str, int]]) -> tuple[int, Optional[Item]]:
            result2 = False
            if hasattr(input, 'alt_mode_mux_raw'):
                result1 = input.alt_mode_mux_raw(self, input.alt_connection_name).variant
            else:
                m = mux[self.index-1]
                name = self._con_to_str(input)
                assert name in m, f'{self}: Connection of {input_name} ({name}) not valid. Valid are: {list(m.keys())}'
                result1 = m[name]
                if name == f'OPAMP{self.index}_RESTAP':
                    result2 = input
            return  result1, result2

        sel_inp_mux, en1 = cvt_to_index(self.inp, 'inp', self._INP_C)
        sel_inm_mux, en2 = cvt_to_index(self.inm, 'inm', self._INM_C)
        res_tap_item = en1 or en2

        result = [
            'sl_si91x_opamp_init();',
            '{',
           f'    static OPAMP_CONFIG_T cfg{{.opamp{self.index} = {{',
           f'            opamp{self.index}_dyn_en = 0,',
           f'            opamp{self.index}_sel_p_mux = {sel_inp_mux},',
           f'            opamp{self.index}_sel_n_mux = {sel_inm_mux},',
           f'            opamp{self.index}_lp_mode = 0,']
        if res_tap_item:
            r1 = res_tap_item.R1
            r2 = res_tap_item.R2
            res_mux_sel = cvt_to_index(res_tap_item.left, 'R1', self._R1_C)[0]
            res_to_out_vdd = cvt_to_index(res_tap_item.right, 'R2', self._R2_C)[0]
            result += [

           f'            opamp{self.index}_r1_sel = {r1},',
           f'            opamp{self.index}_r2_sel = {r2},',
           f'            opamp{self.index}_en_res_bank = 1,',
           f'            opamp{self.index}_res_mux_sel = {res_mux_sel},',
           f'            opamp{self.index}_res_to_out_vdd = {res_to_out_vdd}']
        return result + [
           f'            opamp{self.index}_out_mux_en = 0',
           f'            opamp{self.index}_out_mux_sel = 0',
           f'            opamp{self.index}_enable = 1'
            '        }',
            '    };',
           f'    RSI_OPAMP1_Config(OPAMP, {self.index}, &cfg);',
            '}'
        ]

    @staticmethod
    def _con_to_str(item: Item) -> str:
        if isinstance(item, Dac):
            return 'DAC'
        if item is Gnd:
            return 'GND'
        if isinstance(item, Resistor):
            return f'OPAMP{item.index}_RESTAP'
        if isinstance(item, Opamp):
            return f'OPAMP{item.index}_OUT'
        assert False, f'Invalid input to OpAmp: {item}'

    _INP_C = [
        {'DAC': 6, 'OPAMP1_RESTAP': 7, 'GND': 8},
        {'DAC': 3, 'OPAMP2_RESTAP': 4, 'GND': 5, 'OPAMP1_OUT': 6},
        {'DAC': 2, 'OPAMP3_RESTAP': 3, 'GND': 4, 'OPAMP2_OUT': 5, 'OPAMP2_RESTAP': 6}
    ]
    _INM_C = [
        {'DAC': 2, 'OPAMP1_RESTAP': 3, 'OPAMP1_OUT': 4},
        {'DAC': 1, 'OPAMP2_RESTAP': 2, 'OPAMP2_OUT': 3},
        {'DAC': 1, 'OPAMP3_RESTAP': 2, 'OPAMP3_OUT': 3}
    ]
    _R1_C = [
        {'DAC': 6, 'GND': 7},
        {'DAC': 3, 'GND': 4, 'OPAMP1_OUT': 5},
        {'DAC': 2, 'GND': 3, 'OPAMP2_OUT': 4}
    ]
    _R2_C = [
        {'OPAMP1_OUT': 0, 'VDD': 1},
        {'OPAMP2_OUT': 0, 'VDD': 1, 'DAC': 2, 'GND': 3},
        {'OPAMP3_OUT': 0, 'VDD': 1}
    ]

class Resistor(Item):
    name: str = ''
    index: int

    left: Item
    right: Item
    r1: Optional[int]
    r2: Optional[int]

    _analog = True

    def get_setup(self) -> list[str]:
        return []

    _R1_ENC = [0, 20, 60, 140]
    _R2_ENC = [20, 30, 40, 60, 120, 250, 500, 1000]

    @property
    def R1(self) -> int:
        try:
            return self._R1_ENC.index(self.r1)
        except ValueError:
            print(f"ERROR ({self}): Can't use {self.r1}K as R1 resistor value. Available values: {self._R1_ENC}", file=sys.stderr)
            return 0

    @property
    def R2(self) -> int:
        try:
            return self._R2_ENC.index(self.r2)
        except ValueError:
            print(f"ERROR ({self}): Can't use {self.r2}K as R2 resistor value. Available values: {self._R2_ENC}", file=sys.stderr)
            return 0

    def shot(self) -> Self:
        self.k1 = 0
        self.k2 = 1000
        return self

    def set_div(self, K: Optional[float] = None, Vin: Optional[float] = None, Vout: Optional[float] = None) -> Self:
        # K is R1/(R1+R2)
        if K is None:
            K = Vout/Vin
        # 1/K = 1 + R2/R1 => R1 = R2/(1/K-1)
        nK = 1/K - 1
        best_k = None
        for r2 in self._R2_ENC:
            r1 = r2/nK
            if r1 > 160:
                continue
            if r1 < 10:
                continue
            if r1 <= 40:
                r1 = 20
            elif r1 <= 100:
                r1 = 60
            else:
                r1 = 140
            real_k = r1/(r1+r2)
            if best_k is None or abs(real_k-K) <= abs(best_k-K):
                best_k = K
                best_r1 = r1
                best_r2 = r2
        assert best_k is not None, f"Can't find R1/R2 for K={K}"
        self.r1 = best_r1
        self.r2 = best_r2
        return self



class Comp(Item):
    name: str = ''
    index: int

    inp: Item
    inm: Item

    _canonical = {
        'inp': 'COMP{index}_P',
        'inm': 'COMP{index}_N',
   }
    _analog = True

    # Temporary!
    def get_setup(self) -> list[str]:
        return []

class Scaller(Item):
    name: str = ''

    _analog = True

    # Temporary!
    def get_setup(self) -> list[str]:
        return []

class UlpUart(Item):
    name: str = ''
    
    rx: Optional[Item]
    tx: Optional[Item]

    mode: str

    _canonical = {
        'rx': 'ULP_UART_RX',
        'tx': 'ULP_UART_TX',
    }

    # Temporary!
    def get_setup(self) -> list[str]:
        return []

class Uart(UlpUart):
    index: int

    _canonical = {
        'rx': 'UART{index}_RX',
        'tx': 'UART{index}_TX',
    }

    # Temporary!
    def get_setup(self) -> list[str]:
        return []

class Dac(Item):
    name: str = ''

    # Temporary!
    def get_setup(self) -> list[str]:
        return []

class Adc(Item):
    name: str = ''

    inp: list[Item]
    ref: Item         # ???

    _canonical = {
        'inp': 'ADCP'
    }

    # Temporary!
    def get_setup(self) -> list[str]:
        return []

class AuxLdo(Item):
    name: str = ''

    # Temporary!
    def get_setup(self) -> list[str]:
        return []

class Pwm(Item):
    name: str = ''
    index: int

    Freq: int
    D: int
    output: Optional[Item]

    _canonical = {
        'output': ('PWM_{index}L', 'PWM_{index}H')
    }

    # Temporary!
    def get_setup(self) -> list[str]:
        return []
