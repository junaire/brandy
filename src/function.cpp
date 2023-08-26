#include "function.h"

#include <iostream>

#include "basic_block.h"
#include "context.h"

static std::optional<std::string> getOp(const nl::json &instr) {
  if (instr.contains("op")) {
    return instr["op"].template get<std::string>();
  }
  return std::nullopt;
}

static bool isInstruction(const nl::json &instr) {
  return getOp(instr).has_value();
}

static bool isTerminator(const nl::json &instr) {
  std::optional<std::string> op = getOp(instr);
  if (!op) return false;
  for (std::string_view terminator : kTerminators) {
    if (*op == terminator) return true;
  }
  return false;
}

static std::string createBBName() {
  static int i = 1;
  return "bb." + std::to_string(i++);
}

Function *Function::Create(Context *ctx, const nl::json &function) {
  Function *program = ctx->CreateFunction();
  program->name = function["name"];
  if (function.contains("args")) {
    for (const nl::json &arg : function["args"]) {
      program->args.push_back(std::move(arg["name"]));
    }
  }

  BasicBlock *bb = ctx->CreateBasicBlock();
  for (const nl::json &instr : function["instrs"]) {
    // Real instruction, not a label.
    if (isInstruction(instr)) {
      bb->instrs.push_back(ctx->CreateInstruction(instr, bb));
      // If it's a terminator, push the basic block into the function and reset
      // it.
      if (isTerminator(instr)) {
        program->basic_blocks.push_back(bb);
        bb = ctx->CreateBasicBlock();
      }
    } else {
      // Label should be the first thing in the basic block so let's stop here.
      if (!bb->instrs.empty()) {
        program->basic_blocks.push_back(bb);
        bb = ctx->CreateBasicBlock();
      }
      bb->instrs.push_back(ctx->CreateInstruction(instr, bb));
    }
  }
  // The last basic block.
  if (!bb->instrs.empty()) program->basic_blocks.push_back(bb);

  // Get every basic block a name.
  for (BasicBlock *bb : program->basic_blocks) {
    if (bb->instrs[0]->isLabel()) {
      std::string name = bb->instrs[0]->GetLabel();
      bb->instrs.pop_front();
      bb->name = std::move(name);
    } else {
      std::string name = createBBName();
      bb->name = std::move(name);
    }
    program->block_map[bb->name] = bb;
  }

  return program;
}

BasicBlock *Function::GetBasicBlock(const std::string &name) const {
  if (auto it = block_map.find(name); it != block_map.end()) {
    return it->second;
  }
  return nullptr;
}

void Function::dump() const {
  std::cout << name << " ";
  if (!args.empty()) {
    std::cout << "(";
    for (int i = 0; i < args.size(); ++i) {
      std::cout << args[i];
      if (i != args.size() - 1) {
        std::cout << " ";
      }
    }
    std::cout << ")";
  }
  std::cout << "\n";
  for (const BasicBlock *bb : basic_blocks) {
    bb->dump();
  }
}

nl::json Function::ToJson() {
  nl::json out;
  out["name"] = name;
  for (BasicBlock *bb : basic_blocks) {
    nl::json label;
    label["label"] = bb->name;
    out["instrs"].push_back(std::move(label));

    for (Instruction *instr : bb->instrs) {
      out["instrs"].push_back(instr->instr);
    }
  }
  return out;
}
