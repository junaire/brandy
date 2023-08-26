#pragma once

#include <deque>
#include <optional>
#include <string>

#include "common.h"
#include "instruction.h"

struct BasicBlock {
  // The label. If the original basic block doesn't have a label, we will
  // generate one for it.
  std::string name;
  std::deque<Instruction*> instrs;

  void dump() const;
  std::optional<Instruction*> getTerminator();

  bool isEntry() { return name == "Entry"; }
};
