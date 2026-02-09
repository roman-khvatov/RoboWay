bits = { idx: [] for idx in range(1, 4) }

def cnt(val):
    result = 0
    for idx in range(6):
        if (1 << idx) & val:
            result += 1
    return result

for val in range(1, 64):
    bc = cnt(val)
    if bc <= 3:
       bits[bc].append(val)

print('const uint8_t bits[] = {')
for length, arr in bits.items():
    for val in arr:
        print(f'{val}, ', end='')
    print(f' // {length}')
print('};')

print('const uint8_t bits_idx[] = {', end='')
length = 0
for arr in bits.values():
    length += len(arr)
    print(f'{length},', end='')
print('};')


class Ship:
    def __init__(self, pos_x):
        self.x_l = pos_x
        self.x_r = pos_x
        self.length = 1

    def step(self):
        self.x_l -= 1
        self.x_r += 1
        if self.length < 3:
            self.length += 1

    def put(self, arena):

        def fit(pos, val):
            if 0 <= pos < 8:
                arena[pos] = max(arena[pos], val)

        for idx in range(3):
            l = self.x_l+idx
            r = self.x_r-idx
            if l > r:
                continue
            fit(l, 3-idx)
            fit(r, 3-idx)

def build_sp(value, dst):
    ships = []
    for idx in range(6):
        if (value >> idx) & 1:
            ships.append(Ship(idx+1))
            if len(ships) > 3:
                return
    while True:
        arena = 8 * [0]
        for sp in ships:
            sp.put(arena)
        line = ''
        for item in arena:
            line += " .+*"[item]
        if line == '        ':
            dst.append("0,")
            return
        dst.append(f'"{line}"_sh,')
        for sp in ships:
            sp.step()

print('const uint16_t ships[]={')
ships = []
indexes = {}
for val in range(1, 64):
    start = len(ships)
    build_sp(val, ships)
    if len(ships) != start:
        indexes[val] = (start, len(ships) - start)
        print(' '.join(ships[start:]))
print('};')

print('const uint16_t sh_idxs[] = {')
for idx in range(64):
    if idx not in indexes:
        print('  {},')
    else:
        start, length = indexes[idx]
        print(f' {start},')
print('};')

