#pragma once

#include <map>
#include <string>
#include <vector>

class BasicBlock;
class Instruction;
class CFG;

using DomRelation = std::map<BasicBlock*, std::vector<BasicBlock*>>;

struct DomInfo {
  DomRelation dom;
  std::map<BasicBlock*, BasicBlock*> idom;
  DomRelation df;
  DomRelation dom_tree;

  void dump() const;

  bool IsDominate(const Instruction& a, const Instruction& b);
};

DomInfo ComputeDomInfo(CFG& cfg);
