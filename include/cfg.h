#pragma once

#include <map>
#include <string>
#include <vector>

class Function;
class BasicBlock;

struct CFG {
  Function *function = nullptr;
  std::map<BasicBlock *, std::vector<BasicBlock *>> predecessors;
  std::map<BasicBlock *, std::vector<BasicBlock *>> successors;

  void dump() const;
  void dumpDot(const std::string &filepath) const;
};

CFG BuildCFG(const Function &function);
