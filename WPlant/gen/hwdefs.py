class Entity:
    def __init__(self, **kwargs):
        self.__dir__.update(kwargs)

Input = Entity()
Output = Entity()
Pullup = Entity()
Pulldown = Entity()
FreeRun = Entity()
InCount = Entity()

class List:
    def __init__(self, default: Entity, *args):
        self.default = default
        if default is not None:
            args = [default]+args
        self.list = args

class AnyPin:
    
    def set(self, value: bool):
        pass

    def connect(self, pin: Self):
        pass


Gnd = AnyPin()

class UulpPin(AnyPin):
    name: str
    index: int

    pin_mode: List = List(None, Input, Output)

class Pin(UulpPin):
    pullups: List = List(None, Pullup, Pulldown)

    Strength: Optional[int]  # UlpPin ?

class UlpPin(Pin):
    pass

class SCT:
    name: str

    mode: List = List(None, FreeRun, InCount)    
    input: Optional[AnyPin]
    output: Optional[AnyPin]

class SsiMst:
    name: str

    clock: int
    mosi: Optional[AnyPin]
    miso: Optional[AnyPin]
    clk: AnyPin
    cs0: Optional[AnyPin]
    cs1: Optional[AnyPin]
    cs2: Optional[AnyPin]
    cs3: Optional[AnyPin]

class PinsGroup:
    name: str

    pins: list[AnyPin]

class Opamp:
    name: str
    index: int

    inp: AnyPin
    inm: AnyPin

class Resistor:
    name: str
    index: int

    left: AnyPin
    right: AnyPin

class Scaller:
    name: str

class Uart:
    name: str
    index: int
    
    rx: Optional[AnyPin]
    tx: Optional[AnyPin]

    mode: str

class UlpUart(Uart):
    pass

class Dac:
    name: str
    index: int

class Adc:
    name: str

    inp: list[AnyPin]
    ref: AnyPin

class AuxLDO:
    name: str

class PWM:
    name: str
    index: int

    Freq: int
    D: int

####################################################

class Promise(AnyPin):
    def __init__(self):
        pass

    def set(self, pin: AnyPin):
        pass

class Alternative:
    def __init__(self, name: str):
        pass

class AlternativeGroup:
    def __init__(self, name: str):
        pass
