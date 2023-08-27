#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "common.h"

class Context;
class BasicBlock;
class Instruction;

struct Function {
  std::string name;
  std::vector<std::string> args;
  std::deque<BasicBlock*> basic_blocks;
  std::map<std::string, BasicBlock*> block_map;
  std::map<std::string, Instruction*> all_instrs;

  void dump() const;

  static Function* Create(Context* ctx, const nl::json& function);

  BasicBlock* GetBasicBlock(const std::string& name) const;

  Instruction* GetInstrByName(const std::string& name);

  nl::json ToJson();
};
