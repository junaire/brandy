import sys
import json
from collections import OrderedDict

# value tuple <=> canonical variable name
table = OrderedDict()

# instr.dest <=> value index
var2num = OrderedDict()

def determine_index(arg):
    index = var2num[arg]
    orig = list(table.items())[index][0][-1]
    if type(orig) is int:
        return "#" + str(index)
    else:
        return orig


def build_value(instr):
    value = (instr["op"],)
    if "value" in instr:
        value += (instr["value"],)
    elif "args" in instr:
        for arg in instr["args"]:
            print(arg)
            index = determine_index(arg)
            value += (index,)
    return value


def lvn(basic_block):
    for instr in basic_block:
        if "dest" not in instr:
            continue
        value = build_value(instr)
        if value in table:
            # redundancy!
            var2num[instr["dest"]] = list(table.keys()).index(value)
        else:
            table[value] = instr["dest"]
            var2num[instr["dest"]] = list(table.keys()).index(value)
    print("Table: ", table)
    print("Var2num: ", var2num)


if __name__ == "__main__":
    program = json.load(sys.stdin)
    function = program["functions"][0]  # we know there's only one function named main
    basic_block = function["instrs"]
    lvn(basic_block)
