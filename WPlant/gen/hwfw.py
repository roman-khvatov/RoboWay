from typing import *
from dataclasses import dataclass

class Entity:
    def __init__(self, name: str, **kwargs):
        self.name = name
        self.__dict__.update(kwargs)

    def __str__(self) -> str:
        return self.name

@dataclass
class AltConnection:
    alt_from: 'Item'
    alt_to: 'Item'
    name: str

    def __str__(self) -> str:
        return f'{self.alt_from} => {self.alt_to}:{self.name}'

class Holder:
    root: Self = None

    def __init__(self):
        self.items : list['Item'] =  []
        self.nested : list[Self] = []
        self.alts : list[AltConnection] = []
        self.actions : list = []

    def __enter__(self):
        self.parent = self.root
        Holder.root = self
        self._past_enter()
        return self

    def __exit__(self, *_):
        self._before_exit()
        self.__class__.root = self.parent
        for item in self.items:
            item.active = False

    def _past_enter(self):
        pass
    def _before_exit(self):
        pass

    def register(self, item: 'Item'):
        self.items.append(item)

    def register_nested(self, item: Self):
        self.nested.append(item)

    def record_alt_connection(self, alt_from: 'Item', alt_to: 'Item', alt_name: str):
        self.alts.append(AltConnection(alt_from, alt_to, alt_name))

    def record_action(self, action):
        self.actions.append(action)

    def dump(self, fstream, ident: str=''):
        if hasattr(self, 'name'):
            print(f'{ident}*** {self.name} ***', file=fstream)
            ident += '   '
        for item in self.items:
            if str(item) and item.string:
                print(f'{ident}{item} = {item.string}', file=fstream)
        if self.alts:
            print(f'{ident} -- Alternative connections --', file=fstream)
            for item in self.alts:
                print(f'{ident}    {item}', file=fstream)
        if self.actions:
            print(f'{ident} ** Actions **', file=fstream)
            for item in self.actions:
                print(f'{ident}    {item}', file=fstream)
        if self.nested:
            print(f'{ident} ++ Nested Alternativrs ++', file=fstream)
            for item in self.nested:
                item.dump(fstream,ident + '    ')
                 

class Item:
    def __init__(self, *args, **kwargs):
        self.owner = Holder.root
        self.active = True
        ann = self.__class__.__annotations__.copy()
        for sc in self.__class__.__mro__:
            if hasattr(sc, '__annotations__'):
                ann.update(sc.__annotations__)
        args = list(args)
        if args and isinstance(args[0], str):   # 'name' arg
            assert 'name' in ann, ann
            self.name = args.pop(0)
        if args and isinstance(args[0], int):   # 'index' arg
            assert 'index' in ann
            self.index = args.pop(0)

        def set_attr(var_name: str, val):
            setattr(self, var_name, val)
            # Our list arguments now exists only in PinsGroup. But this class do not change Pins mode
            #if isinstance(val, (list, tuple)):
            #    for val1 in val:
            #        self.alt_connect(val1, var_name)
            if hasattr(val, 'set_alt_mode'):
                self.alt_connect(val, var_name)

        raw_args = {}
        assigned = set()
        to_assign = set()
        list_arg = None
        for var_name, var_type in ann.items():            
            l = getattr(self, var_name, None)
            if is_optional := get_origin(var_type) is Union:
                var_type = get_args(var_type) [0]
                setattr(self, var_name, None)
            if var_type is List:
                for item in l.list:
                    assert item not in raw_args
                    raw_args[item] = var_name
                if l.default is None:
                    if not is_optional:
                        to_assign.add(var_name)
                else:
                    setattr(self, var_name, l.default)
            elif get_origin(var_type) is list:
                list_arg = var_name

        for arg in args:
            if isinstance(arg, list):
                assert list_arg, f'List argument not expected'
                set_attr(list_arg, arg)
                for a in arg:
                    if isinstance(a, Item):
                        assert a.active, f"{a} is out of scope - can't connect"
                list_arg = None
            else:
                assert arg in raw_args, f'Unnamed arg "{arg}" not found in possible arguments: {"/".join(str(x) for x in raw_args.keys())}'
                if isinstance(arg, Item):
                    assert arg.active, f"{arg} is out of scope - can't connect"
                name = raw_args[arg]
                assert name not in assigned
                assigned.add(name)
                set_attr(name, arg)
                to_assign.discard(name)

        for name, val in kwargs.items():
            assert name in ann
            if isinstance(val, Item):
                assert val.active, f"{val} is out of scope - can't connect"
            if isinstance(val, Wire):
                val.append_ref_place(self, name)
                setattr(self, name, None)
            else:
                set_attr(name, val)
                to_assign.discard(name)

        assert not to_assign, f'Not assigned: {to_assign}'

        if Holder.root:
            Holder.root.register(self)

        self._post_init()

    def __str__(self) -> str:
        """ Returns short definition of Item """
        if hasattr(self, 'name') and self.name:
            result = self.name
        else:
            result = self.__class__.__name__
        if hasattr(self, 'index'):
            result += f':{self.index}'
        return result    

    @property
    def string(self) -> str:
        """ Full represetation of this class """
        args = []
        for name, val in self.__dict__.items():
            if name in ('name', 'index', 'owner', 'active') or val is None:
                continue
            if isinstance(val, (list, tuple)):
                val = str([str(x) for x in val])
            args.append(f'{name}={val}')
        return f'{self.__class__.__name__}({", ".join(args)})'


    def _post_init(self):
        pass

    def alt_connect(self, alt_from: Self, alt_name: str):
        if alt_from.owner is not self.owner:
            self.owner.record_alt_connection(alt_from, self, alt_name)
        elif hasattr(alt_from, 'set_alt_mode'):
            alt_from.set_alt_mode(self, alt_name)

class List:
    def __init__(self, *args, default: Optional[Entity] =None):
        self.default: Entity = default
        self.list: list[Entity] = args

class Wire(Item):
    def __lshift__(self, pin: Item):
        for tgt, name in self.places:
            setattr(tgt, name, pin)
            tgt.alt_connect(pin, name)

    def append_ref_place(self, target: Item, name: str):
        self.places.append((target, name))

    def __enter__(self):
        self.places = []
        return self

    def __exit__(self, *_):
        self.places = []

    def __str__(self) -> str:
        return ''

    @property
    def string(self) -> None:
        return None

class Alternative(Holder):
    def __init__(self, name: str):
        super().__init__()
        self.name = name

    def _past_enter(self):
        self.parent.register_nested(self)

