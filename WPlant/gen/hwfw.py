from typing import *
from dataclasses import dataclass

from mux_manager import *

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

    @property
    def mux_setup(self) -> str:
        return self.alt_from.alt_mode_mux(self.alt_to, self.name)

    @property
    def mux_reset(self) -> str:
        return self.alt_from.default_mux
        
class Holder:
    root: Self = None

    def __init__(self):
        self.items : list['Item'] =  []
        self.nested : list[Self] = []
        self.nested_switches : dict[str, dict[str, Self]] = {}
        self.alts : list[AltConnection] = []
        self.actions : list = []

    def __enter__(self):
        self.parent = self.root
        Holder.root = self
        self._past_enter()
        return self

    def __exit__(self, *_):
        self._before_exit()
        Holder.root = self.parent
        for item in self.items:
            item.active = False

    def _past_enter(self):
        pass
    def _before_exit(self):
        pass

    @property
    def alt_muxes_setup(self) -> list[str]:
        return [x.mux_setup for x in self.alts]

    @property
    def alt_muxes_reset(self) -> list[str]:
        return [x.mux_reset for x in self.alts]

    def alt_muxes_group_setup(self, group_of_mutexes: list[Self]) -> list[str]:
        " Group of Holder mux setup/reset (this group setup with automatic reset of all listed in other groups but not in this one) "
        all_items : dict[str, 'Item'] = {}
        for group in group_of_mutexes:
            for item in group.items:
                all_items[str(item)] = item
            for item in group.alts:
                all_items[str(item.alt_from)] = item.alt_from
        result = [item.mux_setup for item in self.alts]
        result += self.get_setup()
        for item in self.alts:
            all_items.pop(str(item.alt_from))
        for item in self.items:
            all_items.pop(str(item), None)
        for item in all_items.values():
            for attrn in ('hw_reset', 'default_mux', 'mux_reset'):
                if dmux := getattr(item, attrn, None):
                    if attrn == 'hw_reset':
                        result.append(f'// {item}')
                    result.append(dmux)
        result += [item.c_code for item in self.actions]
        return result

    @staticmethod
    def switch_function_name(name: str) -> str:
        return f'hw_switch_{name}'

    def get_setup(self) -> list[str]:
        result = []
        for item in self.items:
            if its := item.get_setup():
                result.append(f'// {item}')
                result += its
        for name, val in self.nested_switches.items():            
            result.append(f'// Default for {self.switch_function_name(name)}')
            result.append(f'{self.switch_function_name(name)}({name}::{list(val)[0]});')
        return result

    def generate_c_code(self, fstream):
        print('void hw_setup()\n{', file=fstream)
        for item in self.get_setup():
            print('    ' + item, file=fstream)
        print('}', file=fstream)

        for mux_name, mux_body in self.nested_switches.items():
            print(f'void {self.switch_function_name(mux_name)}({mux_name} selector)\n{{\n    switch(selector)\n    {{', file=fstream)
            all_items = list(mux_body.values())
            for sel_name, sel_body in mux_body.items():
                print(f'        case {mux_name}::{sel_name}:\n            {{', file=fstream)
                for item in sel_body.alt_muxes_group_setup(all_items):
                    print('                ' + item, file=fstream)
                print('                break;\n            }', file=fstream)
            print('    }\n}', file=fstream)

        for item in self.nested:
            nm = item.name
            if nm.startswith('!'):
                # on-off mode
                print(f'void enable_{nm[1:]}(bool on)\n{{\n    if (on)\n    {{', file=fstream)
                for line in item.alt_muxes_setup:
                    print('        ' + line, file=fstream)
                print('    }\n    else\n    {', file=fstream)
                for line in item.alt_muxes_reset:
                    print('        ' + line, file=fstream)
                print('    }\n}', file=fstream)
            else:
                print(f'void set_{nm}()\n{{', file=fstream)
                for line in item.alt_muxes_setup:
                    print('    ' + line, file=fstream)
                print('    }\n}', file=fstream)

    def register(self, item: 'Item'):
        self.items.append(item)

    def register_nested(self, item: Self):
        if ':' in item.name:
            sw_name, _, sw_case = item.name.partition(':')
            self.nested_switches.setdefault(sw_name, {})[sw_case] = item
        else:
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
            print(f'{ident} ++ Nested Alternatives ++', file=fstream)
            for item in self.nested:
                item.dump(fstream,ident + '    ')
        for sw_name, sw_body in self.nested_switches.items():
            print(f'{ident} ++ Switch {sw_name} ++', file=fstream)
            for case_name, case_body in sw_body.items():
                print(f'{ident}  ++ Case {case_name} ++', file=fstream)
                case_body.dump(fstream,ident + '      ')
                 
class Item:
    def __init__(self, *args, **kwargs):
        self.owner = Holder.root
        self.active = True
        self.mux : Optional[str] = None
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
            if isinstance(val, (list, tuple)) and not hasattr(self, '_no_pins_change'):
                for idx, val1 in enumerate(val):
                    self.alt_connect(val1, f'{var_name}:{idx}')
            elif hasattr(val, 'set_alt_mode'):
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
            if name in ('name', 'index', 'owner', 'active', 'mux') or val is None:
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

    def get_canonical_name(self, pin_name: str) -> str|tuple[str]:
        nm = pin_name
        if ':' in nm:
            nm = nm.partition(':')[0]
        result = self._canonical[nm]
        if isinstance(result, str):
            return result.format(**self.__dict__)
        return tuple(x.format(**self.__dict__) for x in result)

    def get_setup(self) -> list[str]:
        result = self.setup_lines
        if self.mux:
            result.append(self.mux)
        return result

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

    def get_setup(self) -> list[str]:
        return []

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

