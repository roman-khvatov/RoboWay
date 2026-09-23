from typing import *

class Entity:
    def __init__(self, name: str, **kwargs):
        self.name = name
        self.__dict__.update(kwargs)

    def __str__(self) -> str:
        return self.name

class Holder:
    root: Self = None

    def __init__(self):
        self.items : list['Item'] =  []
        self.nested : list[Self] = []

    def __enter__(self):
        self.parent = self.root
        self.__class__.root = self
        self._past_enter()
        return self

    def __exit__(self, *_):
        self._before_exit()
        self.__class__.root = self.parent

    def _past_enter(self):
        pass
    def _before_exit(self):
        pass

    def register(self, item: 'Item'):
        self.items.append(item)

    def register_nested(self, item: Self):
        self.nested.append(item)

class Item:
    def __init__(self, *args, **kwargs):
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
            if isinstance(val, (list, tuple)):
                for val1 in val:
                    if hasattr(val1, 'set_alt_mode'):
                        val1.set_alt_mode(self)
            elif hasattr(val, 'set_alt_mode'):
                val.set_alt_mode(self)

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
                list_arg = None
            else:
                assert arg in raw_args, f'Unnamed arg "{arg}" not found in possible arguments: {"/".join(str(x) for x in raw_args.keys())}'
                name = raw_args[arg]
                assert name not in assigned
                assigned.add(name)
                set_attr(name, arg)
                to_assign.discard(name)

        for name, val in kwargs.items():
            assert name in ann
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

    def _post_init(self):
        pass

class List:
    def __init__(self, *args, default: Optional[Entity] =None):
        self.default: Entity = default
        self.list: list[Entity] = args

class Wire(Item):
    def __lshift__(self, pin: Item):
        for tgt, name in self.places:
            setattr(tgt, name, pin)
            if hasattr(pin, 'set_alt_mode'):
                pi.set_alt_mode(tgt)

    def append_ref_place(self, target: Item, name: str):
        self.places.append((target, name))

    def __enter__(self):
        self.places = []
        return self

    def __exit__(self, *_):
        self.places = []

class Alternative(Holder):
    def __init__(self, name: str):
        super().__init__()
        self.name = name

    def _past_enter(self):
        self.parent.register_nested(self)

