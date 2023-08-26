#include "basic_block.h"

#include <iostream>

void BasicBlock::dump() const {
  std::cout << name << "\n";
  if (name != "Entry") {
    for (const Instruction *instr : instrs) {
      std::cout << instr->instr << "\n";
    }
    std::cout << "\n";
  }
}
