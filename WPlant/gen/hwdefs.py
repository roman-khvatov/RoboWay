
K = 1000
M = K*K

####################################################
class Entity:
    def __init__(self, **kwargs):
        self.__dir__.update(kwargs)

class Item:
    pass

class List:
    def __init__(self, default: Optional[Entity], *args):
        self.default: Entity = default
        if default is not None:
            args = [default]+args
        self.list: list[Entity] = args

Input = Entity()
Output = Entity()
Pullup = Entity()
Pulldown = Entity()
FreeRun = Entity()
InCount = Entity()

Gnd = Item()

class AnyPin(Item):
    name: str
    index: int

    default: Optional[bool]
    
    def set(self, value: bool):
        pass

    def connect(self, pin: Item):
        pass

class UulpPin(AnyPin):
    pin_mode: List = List(None, Input, Output)

class Pin(UulpPin):
    pullups: List = List(None, Pullup, Pulldown)

    Strength: Optional[int]  # UlpPin ?

class UlpPin(Pin):
    pass

class SCT(Item):
    name: str

    mode: List = List(None, FreeRun, InCount)    
    input: Optional[Item]
    output: Optional[Item]

class SsiMst:
    name: str

    clock: int
    mosi: Optional[Item]
    miso: Optional[Item]
    clk: Item
    cs0: Optional[Item]
    cs1: Optional[Item]
    cs2: Optional[Item]
    cs3: Optional[Item]

class PinsGroup:
    name: str

    pins: list[Item]

class Opamp(Item):
    name: str
    index: int

    inp: Item
    inm: Item

class Resistor(Item):
    name: str
    index: int

    left: Item
    right: Item

class Scaller(Item):
    name: str

class Uart:
    name: str
    index: int
    
    rx: Optional[Item]
    tx: Optional[Item]

    mode: str

class UlpUart(Uart):
    pass

class Dac(Item):
    name: str
    index: int

class Adc:
    name: str

    inp: list[Item]
    ref: Item

class AuxLDO(Item):
    name: str

class PWM(Item):
    name: str
    index: int

    Freq: int
    D: int

####################################################

class Promise(Item):
    def __init__(self):
        pass

    def set(self, pin: Item):
        pass

class Alternative:
    def __init__(self, name: str):
        pass

class AlternativeGroup:
    def __init__(self, name: str):
        pass
