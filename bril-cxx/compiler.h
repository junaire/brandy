#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "json.hpp"

namespace nl = nlohmann;

using Instruction = nl::json;

using BasicBlock = std::deque<Instruction>;

struct Function {
  std::string name;
  // std::vector<std::string> params;
  std::vector<BasicBlock> basic_blocks;
};

struct CFG {
  std::string name;
  std::map<std::string, BasicBlock> basic_blocks;
  std::map<std::string, std::vector<std::string>> predecessors;
  std::map<std::string, std::vector<std::string>> successors;
};

Function buildFunction(const nl::json &function);

CFG buildCFG(Function &function);
