import sys
import json
from collections import OrderedDict

# Ref: https://capra.cs.cornell.edu/bril/lang/core.html#control
# `call` is not a terminator
TERMINATORS = ["jmp", "br", "ret"]

# label should be the first entry of the basic blocks.
# terminators should be the end of the basic blocks.
def form_basic_blocks(instrs):
    basic_blocks = []
    current_basic_block = []
    for instr in instrs:
        if "label" in instr:  # label has no effect, but we want to include it
            # end the current basic block if it's not empty
            if current_basic_block:
                basic_blocks.append(current_basic_block)
                current_basic_block = []
            current_basic_block.append(instr)

        else:  # A actual instruction
            current_basic_block.append(instr)
            if instr["op"] in TERMINATORS:
                basic_blocks.append(current_basic_block)
                current_basic_block = []

    # At the end of the function, we should have a implicit return
    basic_blocks.append(current_basic_block)

    return basic_blocks


# A mapping from names to the basic blocks
def name2blocks(basic_blocks):
    out = {}
    for basic_block in basic_blocks:
        if "label" in basic_block[0]:  # A label has a name
            name = basic_block[0]["label"]
            basic_block = basic_block[1:]  # The first one is a label, skip it
        else:
            name = f"b{len(out)}"
        out[name] = basic_block

    return out


# A mapping from name to names
def form_successors(name2block):
    cfg = (
        OrderedDict()
    )  # a dict with order, as we need to know the next block when implicit fallthrough
    for i, (name, block) in enumerate(name2block.items()):
        last = block[-1]
        if last["op"] in ("jmp", "br"):
            cfg[name] = last["labels"]
        elif (last["op"] == "ret") or (
            i == len(name2block) - 1
        ):  # no succusors for ret or the last one
            cfg[name] = []
        else:  # Implicit fallthrough
            cfg[name] = [list(name2block.keys())[i + 1]]  # the next basic block

    return cfg


# A CFG has two parts, one is a name2block mapping, another one is a name to names mapping that contains the successors
def get_cfg():
    program = json.load(sys.stdin)
    # get all functions in the program
    functions = program["functions"]
    for function in functions:
        # now we have all the instructions, we can form a CFG!
        instrs = function["instrs"]
        # form instrs to a list of basic blocks
        basic_blocks = form_basic_blocks(instrs)
        # form a name2block
        name2block = name2blocks(basic_blocks)
        # form basic blocks to a cfg
        cfg = form_successors(name2block)
    return name2block, cfg


if __name__ == "__main__":
    name2block, cfg = get_cfg()
    for name, block in name2block.items():
        print(name)
        print("   ", block)
    print("=" * 20)
    print(cfg)
