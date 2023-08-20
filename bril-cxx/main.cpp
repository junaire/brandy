#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
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
  assert(!bb.data.empty());
  Instruction terminator = bb.data.back();
  if (isTerminator(terminator)) return terminator;
  return std::nullopt;
}

static void dumpBasicBlock(const BasicBlock &bb) {
  std::cout << bb.name;
  if (bb.name == "Entry" || bb.name == "Exit") {
    std::cout << "\n";
  } else {
    std::cout << " [";
    for (const Instruction &instr : bb.data) {
      std::cout << instr << ", ";
    }
    std::cout << "]\n";
  }
}

static void dumpFunction(const Function &function) {
  std::cout << function.name << ":\n";
  for (const BasicBlock &bb : function.basic_blocks) {
    dumpBasicBlock(bb);
  }
}

static void dumpCFG(const CFG &cfg) {
  dumpFunction(cfg.function);
  std::cout << "\n";
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
  std::string file = cfg.function.name + ".dot";
  std::ofstream f(file);
  f << "digraph " << cfg.function.name << " {\n";
  f << "node [shape=box, style=filled]\n";
  for (const auto &[name, bb] : cfg.function.basic_blocks) {
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
      bb.data.push_back(instr);
      if (isTerminator(instr)) {
        program.basic_blocks.push_back(bb);
        bb.data.clear();
      }
    } else {
      if (!bb.data.empty()) {
        program.basic_blocks.push_back(bb);
        bb.data.clear();
      }
      bb.data.push_back(instr);
    }
  }
  if (!bb.data.empty()) program.basic_blocks.push_back(bb);

  for (BasicBlock &bb : program.basic_blocks) {
    assert(bb.isValid() && "BasicBlock is invalid");
    // dumpBasicBlock(bb);
    if (bb.data[0].contains("label")) {
      std::string name = bb.data[0]["label"];
      // std::cout << "have a label: " << name << "\n";
      bb.data.pop_front();
      bb.name = name;
    } else {
      std::string name = createBBName();
      // std::cout << "no label: " << name << "\n";
      bb.name = name;
    }
  }

  return program;
}

CFG buildCFG(Function &function) {
  CFG cfg = {.function = function};

  for (auto it = cfg.function.basic_blocks.begin(),
            end = cfg.function.basic_blocks.end();
       it != end; ++it) {
    std::string bb_name = it->name;
    Instruction &instr = it->data.back();

    std::string op = instr["op"].template get<std::string>();
    // std::cout << "name: " << bb_name << " op: " << op << "\n";

    if (op == "br" || op == "jmp") {
      for (const nl::json &dst : instr["labels"]) {
        std::string dst_name = dst.template get<std::string>();
        cfg.successors[bb_name].push_back(dst_name);
        cfg.predecessors[dst_name].push_back(bb_name);
      }
    } else if (op == "ret" || std::next(it) == end) {
      // No successors.
      // std::cout << "no successors\n";
    } else {
      // Fall through.
      std::string next_bb = std::next(it)->name;
      // std::cout << "Fall through " << next_bb << "\n";
      cfg.successors[bb_name].push_back(next_bb);
      cfg.predecessors[next_bb].push_back(bb_name);
    }
  }

  std::vector<std::string> entry_succs;
  std::vector<std::string> exit_preds;

  for (const BasicBlock &bb : cfg.function.basic_blocks) {
    if (cfg.predecessors[bb.name].empty()) {
      entry_succs.push_back(bb.name);
    }
    if (cfg.successors[bb.name].empty()) {
      exit_preds.push_back(bb.name);
    }
  }

  cfg.function.basic_blocks.insert(cfg.function.basic_blocks.begin(),
                                   BasicBlock{.name = "Entry"});
  cfg.function.basic_blocks.push_back(BasicBlock{.name = "Exit"});

  for (const std::string &name : entry_succs) {
    cfg.predecessors[name].push_back("Entry");
  }
  cfg.successors["Entry"] = std::move(entry_succs);

  for (const std::string &name : exit_preds) {
    cfg.successors[name].push_back("Exit");
  }
  cfg.predecessors["Exit"] = std::move(exit_preds);

  return cfg;
}

static void build(CFG &cfg, std::vector<std::string> &postorder,
                  std::set<std::string> &visited, const std::string &root) {
  if (visited.contains(root)) return;
  visited.insert(root);

  for (const std::string &succ : cfg.successors[root]) {
    build(cfg, postorder, visited, succ);
  }
  if (root != "Exit") postorder.push_back(root);
};

std::vector<std::string> buildPostOrder(CFG &cfg) {
  std::vector<std::string> postorder;
  std::set<std::string> visited;
  build(cfg, postorder, visited, "Entry");
  return postorder;
}

DomRelation computeDominators(CFG &cfg) {
  DomRelation dom;
  std::vector<std::string> postorder = buildPostOrder(cfg);
  std::reverse(postorder.begin(), postorder.end());

  assert(postorder[0] == "Entry");
  dom["Entry"] = {"Entry"};

  for (const BasicBlock &bb : cfg.function.basic_blocks) {
    if (bb.isEntry() || bb.isExit()) continue;
    dom[bb.name] = postorder;
  }

  postorder.erase(postorder.begin());

  // for (const std::string &node : postorder) {
  //   std::cout << node << "\n";
  // }

  auto intersect = [](std::vector<std::string> new_dom,
                      std::vector<std::string> dom) {
    std::vector<std::string> intersect;
    for (const std::string &node : dom) {
      if (std::find(new_dom.begin(), new_dom.end(), node) != new_dom.end()) {
        intersect.push_back(node);
      }
    }
    return intersect;
  };

  while (true) {
    bool changed = false;
    for (const std::string &node : postorder) {
      std::vector<std::string> new_dom;

      // intersect preds' dom.
      std::vector<std::string> preds = cfg.predecessors[node];
      assert(!preds.empty() && "preds empty");
      new_dom = dom[preds[0]];
      for (int i = 1; i < preds.size(); ++i) {
        new_dom = intersect(new_dom, dom[preds[i]]);
      }
      // union node itself
      new_dom.push_back(node);

      // if new_dom != dom
      if (new_dom != dom[node]) {
        dom[node] = new_dom;
        changed = true;
      }
    }
    if (!changed) break;
  }
  return dom;
}

void dumpDom(const DomRelation &dom) {
  for (const auto &[name, doms] : dom) {
    std::cout << name << ": \n[";
    for (const std::string &d : doms) {
      std::cout << d << ", ";
    }
    std::cout << "]\n";
  }
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
    DomRelation dom = computeDominators(cfg);
    dumpDom(dom);
  }
}
