#pragma once

#include <deque>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "json.hpp"

namespace nl = nlohmann;

using Instruction = nl::json;

inline constexpr std::string_view kTerminators[] = {"jmp", "br", "ret"};

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

  void dump() const;
};

struct Function {
  std::string name;
  std::set<std::string> args;
  std::vector<BasicBlock> basic_blocks;
  void dump() const;
};

struct CFG {
  Function function;
  std::map<std::string, std::vector<std::string>> predecessors;
  std::map<std::string, std::vector<std::string>> successors;
  void dump() const;
  void dumpDot(const std::string &filepath) const;
};

Function buildFunction(const nl::json &function);

CFG buildCFG(Function &function);

using DomRelation = std::map<std::string, std::vector<std::string>>;

struct DomInfo {
  DomRelation dom;
  std::map<std::string, std::string> idom;
  std::map<std::string, std::set<std::string>> df;
  std::map<std::string, std::set<std::string>> dom_tree;
  void dump();
};

DomInfo computeDomInfo(CFG &cfg);

using PhiMap = std::map<std::string, std::set<std::string>>;

Function convertToSSA(CFG &cfg, Function function, DomInfo &dom);

nl::json FunctionToJson(const Function& func);
