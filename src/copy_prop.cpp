#include <algorithm>
#include <iostream>
#include <vector>

#include "basic_block.h"
#include "function.h"
#include "instruction.h"
#include "transform.h"

// TODO: Make it work across basic blocks.
void CopyProp(Function& func) {
  for (BasicBlock* bb : func.basic_blocks) {
    std::vector<std::vector<std::string>> copies;
    for (Instruction* instr : bb->instrs) {
      if (!instr->hasOp() || !instr->hasDest()) continue;
      if (instr->getOp() != "id") continue;

      // Only handle id operation (x: int = id y;)
      std::string arg = instr->GetArgs()[0];
      std::string dest = instr->GetDest();
      bool exist = false;
      for (std::vector<std::string>& copy : copies) {
        if (std::find(copy.begin(), copy.end(), arg) != copy.end()) {
          copy.push_back(dest);
          exist = true;
          break;
        }
      }
      if (!exist) copies.emplace_back(std::vector<std::string>{arg, dest});
    }

    for (std::vector<std::string>& copy : copies) {
      if (copy.size() == 1) continue;
      auto beg = copy.begin();
      for (auto it = std::next(beg); it != copy.end(); ++it) {
        Instruction* instr = func.GetInstrByName(*it);
        instr->instr["args"] = {*beg};
      }
    }
  }
}
