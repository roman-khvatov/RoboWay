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

class AnyPin(Item):
    name: str = ''
    index: int

    default: Optional[bool]
    
    def __lshift__(self, value: bool):
        pass

class UulpPin(AnyPin):
    pin_mode: Optional[List] = List(Input, Output, AltFunc)  # AltFunc mode deduced automatically from connection
    alt_connection: Optional[Item] = None

    def set_alt_mode(self, who: Item):
        self.pin_mode = AltFunc
        self.alt_connection = who

class Pin(UulpPin):
    pullups: Optional[List] = List(Pullup, Pulldown)

    strength: Optional[int]  # UlpPin ?

class UlpPin(Pin):
    pass

class Sct(Item):
    name: str = ''

    mode: List = List(FreeRun, InCount)    
    input: Optional[Item]
    output: Optional[Item]

    _auto_connect = ['input', 'output']

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

    _auto_connect = ['mosi', 'miso', 'clk', 'cs0', 'cs1', 'cs2', 'cs3']

class PinsGroup(Item):
    name: str

    pins: list[Item]

class Opamp(Item):
    name: str = ''
    index: int

    inp: Item
    inm: Item

    _auto_connect = ['inp', 'inm']

class Comp(Item):
    name: str = ''
    index: int

    inp: Item
    inm: Item

    _auto_connect = ['inp', 'inm']

class Resistor(Item):
    name: str = ''
    index: int

    left: Item
    right: Item

    _auto_connect = ['left', 'right']

class Scaller(Item):
    name: str = ''

class Uart(Item):
    name: str = ''
    index: int
    
    rx: Optional[Item]
    tx: Optional[Item]

    mode: str

    _auto_connect = ['rx', 'tx']


class UlpUart(Uart):
    pass

class Dac(Item):
    name: str = ''

class Adc(Item):
    name: str = ''

    inp: list[Item]
    ref: Item

    def _post_init(self):
        self.connect_flds(self.ref, *self.inp)

class AuxLdo(Item):
    name: str = ''

class Pwm(Item):
    name: str = ''
    index: int

    Freq: int
    D: int
    output: Optional[Item]

    _auto_connect = ['output']
