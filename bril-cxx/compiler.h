#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "json.hpp"

namespace nl = nlohmann;

using Instruction = nl::json;

struct BasicBlock {
  std::string name;
  std::deque<Instruction> data;
  bool isValid() const {
    if (name == "Entry") return true;
    if (name == "Exit") return true;
    return !data.empty();
  }
  bool isEntry() const { return name == "Entry"; }

  bool isExit() const { return name == "Exit"; }
};

struct Function {
  std::string name;
  // std::vector<std::string> params;
  std::vector<BasicBlock> basic_blocks;
};

struct CFG {
  Function function;
  std::map<std::string, std::vector<std::string>> predecessors;
  std::map<std::string, std::vector<std::string>> successors;
};

Function buildFunction(const nl::json &function);

CFG buildCFG(Function &function);

struct DomNode {
  std::string name;
  std::vector<std::string> children;
};

DomNode buildDominatorTree(const CFG &cfg);
