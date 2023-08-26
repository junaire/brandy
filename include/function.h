#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "basic_block.h"

class Context;

struct Function {
  std::string name;
  std::vector<std::string> args;
  std::deque<BasicBlock*> basic_blocks;
  std::map<std::string, BasicBlock*> block_map;

  void dump() const;

  static Function* Create(Context* ctx, const nl::json& function);

  BasicBlock* GetBasicBlock(const std::string& name) const;

  nl::json ToJson();
};
