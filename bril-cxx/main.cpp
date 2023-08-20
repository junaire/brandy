#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>

#include "compiler.h"

static std::optional<std::string> getOp(const Instruction &instr) {
  if (instr.contains("op")) {
    return instr["op"].template get<std::string>();
  }
  return std::nullopt;
}

static bool isInstruction(const Instruction &instr) {
  return getOp(instr).has_value();
}

static constexpr std::string_view kTerminators[] = {"jmp", "br", "ret"};

static bool isTerminator(const Instruction &instr) {
  std::optional<std::string> op = getOp(instr);
  if (!op) return false;
  for (std::string_view terminator : kTerminators) {
    if (*op == terminator) return true;
  }
  return false;
}

static std::optional<Instruction> getTerminator(const BasicBlock &bb) {
  assert(!bb.empty());
  Instruction terminator = bb.back();
  if (isTerminator(terminator)) return terminator;
  return std::nullopt;
}

static void dumpBasicBlock(const BasicBlock &bb) {
  std::cout << "[";
  for (const Instruction &instr : bb) {
    std::cout << instr << ", ";
  }
  std::cout << "]\n";
}

static void dumpFunction(const Function &function) {
  std::cout << function.name << ":\n";
  for (const BasicBlock &bb : function.basic_blocks) {
    for (const Instruction &instr : bb) {
      std::cout << instr << "\n";
    }
    std::cout << "\n";
  }
}

static void dumpCFG(const CFG &cfg) {
  std::cout << cfg.name << ":\n";
  std::cout << "BasicBlocks:\n";
  for (const auto &[name, bb] : cfg.basic_blocks) {
    std::cout << name << ":\n";
    for (const Instruction &instr : bb) std::cout << instr << "\n";
    std::cout << "\n";
  }
  if (!cfg.successors.empty()) {
    std::cout << "Successors:\n";
    for (const auto &[name, succs] : cfg.successors) {
      std::cout << name << ": ";
      std::cout << "[";
      for (const std::string &succ : succs) {
        std::cout << succ << ", ";
      }
      std::cout << "]\n";
    }
  }
  // if (!cfg.predecessors.empty()) {
  //   std::cout << "Predecessors:\n";
  //   for (const auto &[name, preds] : cfg.predecessors) {
  //     std::cout << name << ": ";
  //     std::cout << "[";
  //     for (const std::string &pred : preds) {
  //       std::cout << pred << ", ";
  //     }
  //     std::cout << "]\n";
  //   }
  // }
}

static void dumpCFGToDot(const CFG &cfg) {
  std::string file = cfg.name + ".dot";
  std::ofstream f(file);
  f << "digraph " << cfg.name << " {\n";
  f << "node [shape=box, style=filled]\n";
  for (const auto &[name, bb] : cfg.basic_blocks) {
    f << "\"" << name << "\"\n";
  }
  for (const auto &[name, succs] : cfg.successors) {
    for (const std::string &succ : succs)
      f << "\"" << name << "\"-> \"" << succ << "\"[color=\"blue\"]\n";
  }
  // for (const auto &[name, preds] : cfg.predecessors) {
  //   for (const std::string &pred : preds)
  //     f << "\"" << name << "\"-> \"" << pred << "\"[color=\"red\"]\n";
  // }
  f << "}";
}

static std::string createBBName() {
  static int i = 0;
  return "bb." + std::to_string(i++);
}

Function buildFunction(const nl::json &function) {
  Function program;
  program.name = function["name"];
  // std::cout << program.name << "\n";

  BasicBlock bb;
  for (const Instruction &instr : function["instrs"]) {
    // std::cout << instr << "\n";
    if (isInstruction(instr)) {
      bb.push_back(instr);
      if (isTerminator(instr)) {
        program.basic_blocks.push_back(bb);
        bb.clear();
      }
    } else {
      if (!bb.empty()) {
        program.basic_blocks.push_back(bb);
        bb.clear();
      }
      bb.push_back(instr);
    }
  }
  if (!bb.empty()) program.basic_blocks.push_back(bb);

  return program;
}

CFG buildCFG(Function &function) {
  CFG cfg = {.name = function.name};
  // std::cout << cfg.name << "\n";

  for (BasicBlock &bb : function.basic_blocks) {
    assert(!bb.empty() && "BasicBlock is empty");
    // dumpBasicBlock(bb);
    if (bb[0].contains("label")) {
      std::string name = bb[0]["label"];
      // std::cout << "have a label: " << name << "\n";
      bb.pop_front();
      cfg.basic_blocks[name] = std::move(bb);
    } else {
      std::string name = createBBName();
      // std::cout << "no label: " << name << "\n";
      cfg.basic_blocks[name] = std::move(bb);
    }
  }
  for (const auto &[name, bb] : cfg.basic_blocks) {
    if (std::optional<Instruction> terminator = getTerminator(bb)) {
      for (const nl::json &dst : terminator.value()["labels"]) {
        std::string dst_name = dst.template get<std::string>();
        cfg.successors[name].push_back(dst_name);
        cfg.predecessors[dst_name].push_back(name);
      }
    }
  }
  return cfg;
}

int main(int argc, char **argv) {
  nl::json ir;
  if (argc == 1)
    ir = nl::json::parse(std::cin);
  else {
    std::ifstream f(argv[1]);
    ir = nl::json::parse(f);
  }

  for (const nl::json &function : ir["functions"]) {
    Function program = buildFunction(function);
    // dumpFunction(program);
    CFG cfg = buildCFG(program);
    // dumpCFG(cfg);
    dumpCFGToDot(cfg);
  }
}
