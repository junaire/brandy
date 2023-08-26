#include <set>
#include <string>

#include "basic_block.h"
#include "function.h"
#include "instruction.h"
#include "transform.h"

void die(Function &func) {
  std::set<std::string> uses;

  // Collect uses.
  for (BasicBlock *bb : func.basic_blocks) {
    for (Instruction *instr : bb->instrs) {
      if (!instr->hasArgs()) continue;
      for (const std::string &arg : instr->GetArgs()) {
        uses.insert(std::move(arg));
      }
    }
  }

  for (BasicBlock *bb : func.basic_blocks) {
    for (auto it = bb->instrs.begin(); it != bb->instrs.end(); it++) {
      if (!(*it)->hasDest()) continue;
      //  No use for this def.
      if (!uses.contains((*it)->GetDest()) /*or no side effect*/) {
        bb->instrs.erase(it);
      }
    }
  }
}
