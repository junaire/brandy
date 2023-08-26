#include "ssa.h"

#include <map>
#include <set>
#include <vector>

#include "basic_block.h"
#include "cfg.h"
#include "context.h"
#include "dom.h"
#include "function.h"
#include "instruction.h"

static std::map<std::string, std::set<BasicBlock *>> GetDefBlockMap(
    Function &function) {
  std::map<std::string, std::set<BasicBlock *>> out;
  for (BasicBlock *bb : function.basic_blocks) {
    for (Instruction *instr : bb->instrs) {
      if (instr->hasDest()) {
        out[instr->GetDest()].insert(bb);
      }
    }
  }
  return out;
}

static PhiMap GetPhis(Function &function, DomInfo &dom_info) {
  PhiMap phis;

  for (auto [v, def_list] : GetDefBlockMap(function)) {
    for (auto d = def_list.begin(); d != def_list.end(); ++d) {
      for (BasicBlock *block : dom_info.df[*d]) {
        if (phis[block].contains(v)) continue;
        phis[block].insert(v);
        if (!def_list.contains(block)) def_list.insert(block);
      }
    }
  }
  return phis;
}

struct SSAConverter {
  CFG &cfg;
  Function &function;
  DomInfo &dom_info;
  Context *ctx;

  PhiMap phis;
  std::map<std::string, int> counters;
  using Stack = std::map<std::string, std::vector<std::string>>;
  Stack stack;

  std::map<
      BasicBlock *,
      std::map<std::string, std::vector<std::pair<BasicBlock *, std::string>>>>
      phi_args;

  std::map<BasicBlock *, std::map<std::string, std::string>> phi_dests;

  SSAConverter(Context *ctx, CFG &cfg, Function &function, DomInfo &dom_info)
      : ctx(ctx), cfg(cfg), function(function), dom_info(dom_info) {
    phis = GetPhis(function, dom_info);
  }

  std::string pushFresh(const std::string &var) {
    int index = counters[var]++;
    std::string fresh = var + "." + std::to_string(index);
    auto &var_stack = stack[var];
    var_stack.insert(stack[var].begin(), fresh);
    return fresh;
  }

  void Rename(BasicBlock *block) {
    // Copy save the old stack.
    Stack old_stack = stack;

    // Rename phi node dests.
    for (const std::string &p : phis[block]) {
      phi_dests[block][p] = pushFresh(p);
    }

    for (Instruction *instr : block->instrs) {
      // rename args of normal instructions
      if (instr->hasArgs()) {
        std::vector<std::string> new_args;
        for (const std::string &arg : instr->GetArgs()) {
          new_args.push_back(stack[arg][0]);
        }
        instr->instr["args"] = new_args;
      }
      // rename dest
      if (instr->hasDest()) {
        instr->instr["dest"] = pushFresh(instr->GetDest());
      }
    }

    // rename phis
    for (BasicBlock *s : cfg.successors[block]) {
      for (const std::string &p : phis[s]) {
        if (!stack[p].empty()) {
          phi_args[s][p].emplace_back(block, stack[p][0]);
        } else {
          // Looks like we can just throw it away?
          phi_args[s][p].emplace_back(block, "__undef");
        }
      }
    }
    // recursive calls.
    for (BasicBlock *b : dom_info.dom_tree[block]) {
      Rename(b);
    }
    stack = old_stack;
  }

  void InsertPhis() {
    for (BasicBlock *block : function.basic_blocks) {
      for (auto &[dest, pairs] : phi_args[block]) {
        nl::json phi;
        phi["op"] = "phi";
        phi["dest"] = phi_dests[block][dest];
        // FIXME: DO NOT HARDCODE THIS!
        phi["type"] = "int";
        for (const auto &pair : pairs) {
          phi["labels"].push_back(pair.first->name);
          phi["args"].push_back(pair.second);
        }
        auto *instr = ctx->CreateInstruction(std::move(phi), block);
        block->instrs.push_front(instr);
      }
    }
  }

  void ToSSA() {
    Rename(function.basic_blocks[0]);
    InsertPhis();
  }
};

void ToSSA(Context *ctx, Function &function, CFG &cfg, DomInfo &dom) {
  SSAConverter converter(ctx, cfg, function, dom);
  converter.ToSSA();
