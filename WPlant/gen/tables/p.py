from pprint import pp
import re

def parse(text: str) -> list[list[str|tuple[str]]]:
    result = []
    line_acc = []
    for item in re.findall('[^,"\n]+[,\n]|"[^"]+"[,\n]|,|\n', text):
        if new_line := item.endswith('\n'):
            item = item[:-1]
        item = item.removesuffix(',')
        if item:
            if item[0] == '"':
                item = item[1:-1]
                if '\n' in item:
                    item = tuple(item.splitlines())
            line_acc.append(item)
        if new_line:
            result.append(line_acc)
            line_acc = []
    return result

def parse_gpio(text: str) -> dict[str, dict[str, int]]:
    result = {}
    for l in parse(text):
        # l[0] is an index
        idx, *rest = l
        if isinstance(idx, tuple):
            idx = idx[0]
        if '/' in idx:
            idx = idx.partition('/')[0].strip()
        dst = result.setdefault(idx, {})
        for grp in rest:
            for chunk in grp:
                mtch = re.match(r'(\d+):\s*(.*)$', chunk)
                assert mtch, chunk
                idx, tok = mtch.groups()
                if tok:
                    dst[tok] = int(idx)
    return result

def parse_analog(text: str) -> dict[str, list[str|tuple]]:
    result = {}
    for l in parse(text):
        # l[0] is an index
        idx, *rest = l
        idx = idx.partition('-')[0].strip()
        result[idx] = [item for item in rest if item]
    return result

                

result = {}

for fname in ("gpio", "SOCPERH_ON_ULP_GPIO", "ulp_gpio", "ULPPERH_ON_SOC_GPIO", "UULP_VBAT_GPIO"):
    with open(fname+'.csv', 'rt') as f:
        text = f.read()
    result[fname.upper()] = parse_gpio(text)
    
with open('analog.csv', 'rt') as f:
    text = f.read()
result['ANALOG'] = parse_analog(text)

pp(result, indent=1, width=128, sort_dicts=False)



