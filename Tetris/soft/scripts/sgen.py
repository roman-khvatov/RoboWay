import sys
from dataclasses import dataclass
from typing import *

@dataclass
class Sprite:
    pixels: list[str]
    palete: Optional[str] = None
    name: Optional[str] = None
    group_span: int = 0

    def rotate(self) -> Self:
        pixels = []
        for slc in zip(*self.pixels):
            pixels.append(''.join(slc)[::-1])
        return Sprite(pixels, self.palete, self.name)

    def get_pixels(self) -> tuple[int, int]:
        pixels1 = 0
        pixels2 = 0
        mask = 1
        for sym in ''.join(self.pixels):
            if sym != ' ':
                if self.palete:
                    idx = self.palete.find(sym)
                    assert idx != -1, f'Symbol {sym} of sprite {"\n".join(self.pixels)} not found in sprite palete {self.palete}'
                    if idx != 1: pixels1 |= mask
                    if idx != 0: pixels2 |= mask
                else:
                    pixels1 |= mask
            mask <<= 1
        return pixels1, pixels2

    def __str__(self):
        pixels, pixels2 = self.get_pixels()
        result = f'  {{0x{pixels:X}, {len(self.pixels[0])}, {len(self.pixels)}, {self.group_span}, {1 if self.palete else 0}}},'
        if self.palete:
            result += f'\n  {{0x{pixels2:X}, {len(self.pixels[0])}, {len(self.pixels)}, {self.group_span}, {1 if self.palete else 0}}},'
        return result

def load_sprite(stream, prefetch: Optional[str] = None) -> tuple[Optional[Sprite], Optional[str]]:
    pixels = []
    name = None
    palete = None
    while True:
        if prefetch:
            line = prefetch
            prefetch = None
        else:
            line = stream.readline()
        if not line:
            break
        line = line.rstrip()
        if line.startswith('%'):
            prefetch = line
            break
        if ':' in line:
            key, _, arg = line.partition(':')
            match key:
                case 'name':  name = arg
                case 'palete': paleter = arg
                case _: print(f'ERROR: Unknown keyword {key} in {line}')
            continue
        if not line:
            if pixels:
                break
            else:
                continue
        pixels.append(line)
    if not pixels:
        return (None, prefetch)
    width = max(len(x) for x in pixels)
    pixels = [ x.ljust(width) for x in pixels ]
    return (Sprite(pixels, palete, name), prefetch)

def gen_rotated(sprite: Sprite) -> list[Sprite]:
    result = []
    s = sprite
    while True:
        s = s.rotate()
        if s.pixels == sprite.pixels:
            break
        result.append(s)
    return result

class Parser:
    def __init__(self, fname):
        self.sprites : list[Sprite] = []
        self.names : dict[str, int] = {}
        self.tetris_indexes : list[int] = []

        with open(fname, "rt") as f:
            self.load_file(f)


    def print_sprites(self):
        for spr in self.sprites:
            if spr:
                print('// ' + '\n// '.join(spr.pixels))
                print(spr)

    def get_tetris_array(self) -> str:
        return ', '.join(str(x) for x in self.tetris_indexes)

    def load_file(self, stream):
        prefetch = None
        in_tetris = False
        while True:
            sprite, prefetch = load_sprite(stream, prefetch)
            if not sprite and not prefetch:
                break
            if prefetch == '%tetris':
                in_tetris = True
                assert not self.tetris_indexes
                prefetch = None
            if sprite:
                s_idx = self.add_sprite(sprite)
                if in_tetris:
                    self.tetris_indexes.append(s_idx)
                    t_list = gen_rotated(sprite)
                    sprite.group_span = len(t_list)
                    for item in t_list:
                        self.add_sprite(item)

    def add_sprite(self, sprite) -> int:
        result = len(self.sprites)
        if sprite.name:
            self.name[sprite.name] = result
        self.sprites.append(sprite)
        if sprite.palete:
            self.sprites.append(None)
        return result


spr = Parser(sys.argv[1])

print('''#include "sprite.h"
const SpriteDef sprites[] = {''')
spr.print_sprites()
print('};')
print(f'const int tetris_figures[] = {{{spr.get_tetris_array()}}};')
print(f'const int total_tetris_figures = {len(spr.tetris_indexes)};')
