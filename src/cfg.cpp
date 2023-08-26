#include "cfg.h"

#include <fstream>
#include <iostream>

#include "basic_block.h"
#include "function.h"

CFG BuildCFG(const Function &function) {
  CFG cfg = {.function = const_cast<Function *>(&function)};

  for (auto it = cfg.function->basic_blocks.begin(),
            end = cfg.function->basic_blocks.end();
       it != end; ++it) {
    BasicBlock *bb = *it;
    Instruction *instr = (*it)->instrs.back();
    std::string op = instr->getOp();

    if (op == "br" || op == "jmp") {
      for (const std::string &dst : instr->GetLabels()) {
        BasicBlock *succ = function.GetBasicBlock(dst);
        cfg.successors[bb].push_back(succ);
        cfg.predecessors[succ].push_back(bb);
      }
    } else if (op == "ret" || std::next(it) == end) {
      // No successors.
    } else {
      // Fall through.
      BasicBlock *next_bb = *std::next(it);
      cfg.successors[bb].push_back(next_bb);
      cfg.predecessors[next_bb].push_back(bb);
    }
  }

  return cfg;
}

void CFG::dump() const {
  if (!successors.empty()) {
    std::cout << "Successors:\n";
    for (const auto &[node, succs] : successors) {
      std::cout << node->name << ": ";
      std::cout << "[";
      for (const BasicBlock *succ : succs) {
        std::cout << succ->name << ", ";
      }
      std::cout << "]\n";
    }
  }
  if (!predecessors.empty()) {
    std::cout << "Predecessors:\n";
    for (const auto &[node, preds] : predecessors) {
      std::cout << node->name << ": ";
      std::cout << "[";
      for (const BasicBlock *pred : preds) {
        std::cout << pred->name << ", ";
      }
      std::cout << "]\n";
    }
  }
}

void CFG::dumpDot(const std::string &filepath) const {
  std::string file = filepath + "/" + function->name + ".dot";
  std::ofstream f(file);

  f << "digraph " << function->name << " {\n";
  f << "node [shape=box, style=filled]\n";

  for (const BasicBlock *bb : function->basic_blocks) {
    f << "\"" << bb->name << "\"\n";
  }
  for (const auto &[node, succs] : successors) {
    for (const BasicBlock *succ : succs)
      f << "\"" << node->name << "\"-> \"" << succ->name
        << "\"[color=\"blue\"]\n";
  }
  for (const auto &[node, preds] : predecessors) {
    for (const BasicBlock *pred : preds)
      f << "\"" << node->name << "\"-> \"" << pred->name
        << "\"[color=\"red\"]\n";
  }

  f << "}";
  f.close();
}
