bits = { idx: [] for idx in range(1, 7) }

def cnt(val):
    result = 0
    for idx in range(6):
        if (1 << idx) & val:
            result += 1
    return result

for val in range(1, 64):
    bits[cnt(val)].append(val)

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
