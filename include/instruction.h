#pragma once

#include <cassert>
#include <string>

#include "common.h"

class BasicBlock;

struct Instruction {
  nl::json instr;
  BasicBlock* parent = nullptr;

  Instruction(nl::json instr, BasicBlock* parent)
      : instr(std::move(instr)), parent(parent) {}

  bool hasOp() { return instr.contains("op"); }

  std::string getOp() {
    assert(hasOp());
    return instr["op"].template get<std::string>();
  }

  bool isTerminator() {
    std::string op = getOp();
    for (std::string_view terminator : kTerminators) {
      if (op == terminator) return true;
    }
    return false;
  }

  // Because we construct the IR from JSON.
  bool isLabel() const { return instr.contains("label"); }

  std::string GetLabel() {
    assert(isLabel());
    return instr["label"].template get<std::string>();
  }

  bool hasDest() { return instr.contains("dest"); }

  std::string GetDest() {
    assert(hasDest());
    return instr["dest"].template get<std::string>();
  }

  bool hasArgs() { return instr.contains("args"); }

  std::vector<std::string> GetArgs() {
    return instr["args"].template get<std::vector<std::string>>();
  }

  std::vector<std::string> GetLabels() {
    std::vector<std::string> labels;
    assert(instr.contains("labels"));
    for (const auto& arg : instr["labels"]) {
      labels.push_back(arg.template get<std::string>());
    }
    return labels;
  }
};
