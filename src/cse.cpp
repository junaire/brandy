#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "basic_block.h"
#include "cfg.h"
#include "dom.h"
#include "function.h"
#include "instruction.h"
#include "transform.h"

struct Identity {
  std::string op;
  std::vector<std::string> args;
  Identity(std::string op, std::vector<std::string> args)
      : op(std::move(op)), args(std::move(args)) {}

  friend bool operator==(const Identity &lhs, const Identity &rhs) {
    if (lhs.op != rhs.op) return false;
    if (lhs.args.size() != rhs.args.size()) return false;
    // +/* is commutative.
    if (lhs.op == "+" || lhs.op == "*") {
      return std::is_permutation(lhs.args.begin(), lhs.args.end(),
                                 rhs.args.begin());
    }
    return lhs.args == rhs.args;
  }
};

template <>
struct std::hash<Identity> {
  std::size_t operator()(const Identity &id) const {
    std::size_t seed = 0;
    std::hash<std::string> hasher;
    seed ^= hasher(id.op) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    for (const std::string &arg : id.args) {
      seed ^= hasher(arg) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

void cse(Function &func) {
  CFG cfg = BuildCFG(func);
  DomInfo dom = ComputeDomInfo(cfg);

  std::unordered_map<Identity, std::vector<Instruction *>> cand;

  for (BasicBlock *bb : func.basic_blocks) {
    for (Instruction *instr : bb->instrs) {
      if (!instr->hasOp() || !instr->hasArgs()) continue;

      std::string op = instr->getOp();
      std::vector<std::string> args;
      for (const std::string &arg : instr->GetArgs()) {
        args.push_back(arg);
      }
      Identity ident(std::move(op), std::move(args));
      cand[ident].push_back(instr);
    }
  }

  for (const auto &[ident, instrs] : cand) {
    if (instrs.size() < 2) continue;

    for (int i = 0; i < instrs.size(); ++i) {
      for (int j = i + 1; j < instrs.size(); ++j) {
        Instruction *a = instrs[i];
        Instruction *b = instrs[j];
        // if def(i) dominates def(j), rewrite j.
        if (dom.IsDominate(*a, *b)) {
          b->instr["op"] = "id";
          b->instr["args"] = {a->GetDest()};
        }
      }
    }
  }
}
