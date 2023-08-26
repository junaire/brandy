#include "context.h"

#include "basic_block.h"
#include "function.h"
#include "instruction.h"

Instruction* Context::CreateInstruction(nl::json instr, BasicBlock* parent) {
  return instrs
      .emplace_back(std::make_unique<Instruction>(std::move(instr), parent))
      .get();
}

BasicBlock* Context::CreateBasicBlock() {
  return basic_blocks.emplace_back(std::make_unique<BasicBlock>()).get();
}

Function* Context::CreateFunction() {
  return functions.emplace_back(std::make_unique<Function>()).get();
}
