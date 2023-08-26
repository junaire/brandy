#pragma once
#include <memory>
#include <vector>

#include "common.h"

class Instruction;
class BasicBlock;
class Function;

class Context {
  std::vector<std::unique_ptr<Instruction>> instrs;
  std::vector<std::unique_ptr<BasicBlock>> basic_blocks;
  std::vector<std::unique_ptr<Function>> functions;

 public:
  Instruction* CreateInstruction(nl::json instr, BasicBlock* parent);

  BasicBlock* CreateBasicBlock();

  Function* CreateFunction();
};
