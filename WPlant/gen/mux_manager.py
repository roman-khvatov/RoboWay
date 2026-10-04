import re

from dataclasses import dataclass
from typing import *
from mux_data import MUX_TABLE

@dataclass
class MuxSet:
    mux_name: str       # Root name of MUX source (GPIO, SOCPERH_ON_ULP_GPIO, ULP_GPIO, ULPPERH_ON_SOC_GPIO, UULP_VBAT_GPIO, ANALOG)
    mux_pin_idx: int    # Index of Pin port
    mux_mode: int       # Mode of appropriate MUX
    variant: Optional[int] = None
    parent: Optional[Self] = None

    def __str__(self) -> str:
        result = f'{self.mux_name}:{self.mux_pin_idx} => {self.mux_mode}'
        if self.variant is not None:
            result += f'[{self.variant}]'
        if self.parent:
            result = str(self.parent) + '; ' + result
        return result

    def get_mux_string(self) -> Optional[str]:
        match self.mux_name:
            case 'GPIO':                  return f'sl_gpio_set_pin_mode(SL_GPIO_PORT_A, {self.mux_pin_idx}, SL_GPIO_MODE_{self.mux_mode}, 0);'
            case 'ULP_GPIO':              return f'sl_gpio_set_pin_mode(SL_GPIO_ULP_PORT, {self.mux_pin_idx}, SL_GPIO_MODE_{self.mux_mode}, 0);'
            case 'UULP_VBAT_GPIO':        return f'sl_si91x_gpio_driver_set_uulp_npss_pin_mux({self.mux_pin_idx}, NPSS_GPIO_PIN_MUX_MODE{self.mux_mode});'
            case 'ULPPERH_ON_SOC_GPIO':   return f'sl_si91x_gpio_driver_set_ulp_peri_on_soc_pin_mode(GPIO_PTR({self.parent.mux_pin_idx}), SL_GPIO_MODE_{self.parent.mux_mode});'
            case 'SOCPERH_ON_ULP_GPIO':   return f'sl_si91x_gpio_driver_set_soc_peri_on_ulp_pin_mode(ULP_GPIO_PTR({self.parent.mux_pin_idx}), SL_GPIO_MODE_{self.parent.mux_mode});'
            case 'ANALOG':                return self.parent.get_mux_string()

def find_mux_chain(gpio_from: str, hw_to: str) -> Optional[MuxSet]:
    root_nm, _, root_pin = gpio_from.rpartition('_')
    root_pin = int(root_pin)
    assert root_nm in MUX_TABLE, f'Unknown PIN type {root_nm}'
    root_entry = MUX_TABLE[root_nm][gpio_from]
    if hw_to in root_entry:
        return MuxSet(root_nm, root_pin, root_entry[hw_to])
    nested = []
    for name, idx in root_entry.items():
        if mtch := re.match(r'^(.*)\[(\d+)\]$', name):
            return MuxSet(root_nm, root_pin, idx, int(mtch.group(1)))
        if mtch := re.match(r'^(SOCPERH_ON_ULP_GPIO|ULPPERH_ON_SOC_GPIO|AGPIO|TopGPIO)_(\d+)$', name):
            nested.append(mtch.groups() + (name, idx))
    assert nested, f'No MUX from {gpio_from} to {hw_to}'
    for nst_root, nst_idx, root_name, root_idx in nested:
        nst_idx = int(nst_idx)
        nn = nst_root
        if nn in ('AGPIO', 'TopGPIO'):
            nn = 'ANALOG'
        nst = MUX_TABLE[nn][f'{nst_root}_{nst_idx}']
        parent = MuxSet(root_name, root_pin, root_idx)
        match_result = _chk(nst, hw_to)
        if match_result is not False:
            return MuxSet(nn, nst_idx, -1, match_result, parent=parent)
    
def find_mux_chain2(gpio_from: str, hw_to: str|tuple[str]) -> MuxSet:
    if isinstance(hw_to, tuple):
        for item in hw_to:
            if result := find_mux_chain(gpio_from, item):
                return result
    else:
        if result := find_mux_chain(gpio_from, hw_to):
            return result            
    assert False, f'No MUX from {gpio_from} to {hw_to}'

def _chk(arr: list[str|tuple[str]], hw_to: str) -> Optional[int|bool]:
    for item in arr:
        if isinstance(item, tuple):
            if (result := _chk(item, hw_to)) is not False:
                return result
        elif item == hw_to:
            return None
        elif mtch := re.match(fr'^{hw_to}\[(\d+)\]$', item):
                return int(mtch.group(1))
        elif mtch := re.match(fr'^{hw_to}(\d)$', item):
            return int(mtch.group(1))
    return False
