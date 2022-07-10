import sys
import json

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


# record every instructions that have been used ( in other instr's args)
# remove the instruction that has no side effects if it's not in the used set
def dead_code_elimination(basic_block):
    used = []
    for instr in basic_block:
        if "args" in instr:
            used += instr["args"]
    for instr in basic_block:
        if "dest" in instr and instr["dest"] not in used:
            basic_block.remove(instr)

def run_dead_code_elimination(basic_block):
    last = []
    while True:
        dead_code_elimination(basic_block)
        if last == basic_block:
            return
        last = basic_block

if __name__ == "__main__":
    program = json.load(sys.stdin)
    function = program["functions"][0] # we know there's only one function named main
    basic_block = function["instrs"]
    run_dead_code_elimination(basic_block)
    function["instrs"] = basic_block
    print(json.dumps(program))
