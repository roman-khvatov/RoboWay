
K = 1000
M = K*K

####################################################
class Entity:
    def __init__(self, **kwargs):
        self.__dir__.update(kwargs)

class Item:
    def __init__(self, *args, **kwargs):
        if args and isinstance(args[0], str):   # 'name' arg
            assert hasattr(self, 'name')
            self.name = args.pop(0)
        if args and isinstance(args[0], int):   # 'index' arg
            assert hasattr(self, 'index')
            self.index = args.pop(0)

        raw_args = {}
        assigned = set()
        to_assign = set()
        for var_name, var_type in self.__class__.__annotations__.items():
            if var_type is List:
                l = getattr(self, var_name)
                for item in l.list:
                    assert item not in raw_args
                    raw_args[item] = var_name
                if l.default is None:
                    to_assign.add(var_name)
                else:
                    setattr(self, var_name, l.default)

        for arg in args:
            assert arg in raw_args
            name = raw_args[arg]
            assert name not in assigned
            assigned.add(name)
            setattr(self, name, arg)
            to_assign.discard(name)

        assert not to_assign

        for name, val in kwargs.items():
            assert hasattr(self, name)
            if isinstance(val, Wire):
                val.append_ref_place(self, name)
            else:
                setattr(self, name, val)

            

class List:
    def __init__(self, *args, /, default: Optional[Entity] =None):
        self.default: Entity = default
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
    pin_mode: List = List(Input, Output)

class Pin(UulpPin):
    pullups: List = List(Pullup, Pulldown)

    Strength: Optional[int]  # UlpPin ?

class UlpPin(Pin):
    pass

class Sct(Item):
    name: str

    mode: List = List(FreeRun, InCount)    
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

class AuxLdo(Item):
    name: str

class Pwm(Item):
    name: str
    index: int

    Freq: int
    D: int

####################################################

class Wire(Item):
    def __init__(self):
        self.places = []

    def set(self, pin: Item):
        for tgt, name in self.places:
            setattr(tgt, name, pin)

    def append_ref_place(self, target: Item, name: str):
        self.places.append((target, name))

class Alternative:
    def __init__(self, name: str):
        pass

class AlternativeGroup:
    def __init__(self, name: str):
        pass
