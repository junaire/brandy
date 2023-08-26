#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <string_view>
#include <utility>

#include "compiler.h"

// ============ Instruction ================

static std::optional<std::string> getOp(const Instruction &instr) {
  if (instr.contains("op")) {
    return instr["op"].template get<std::string>();
  }
  return std::nullopt;
}

static bool isInstruction(const Instruction &instr) {
  return getOp(instr).has_value();
}

static bool isTerminator(const Instruction &instr) {
  std::optional<std::string> op = getOp(instr);
  if (!op) return false;
  for (std::string_view terminator : kTerminators) {
    if (*op == terminator) return true;
  }
  return false;
}

// ============ BasicBlock ================

static std::optional<Instruction> getTerminator(const BasicBlock &bb) {
  assert(!bb.data.empty());
  Instruction terminator = bb.data.back();
  if (isTerminator(terminator)) return terminator;
  return std::nullopt;
}

void BasicBlock::dump() const {
  std::cout << name;
  if (name == "Entry" || name == "Exit") {
    std::cout << "\n";
  } else {
    std::cout << " [";
    for (const Instruction &instr : data) {
      std::cout << instr << ", ";
    }
    std::cout << "]\n";
  }
}

// ============ Function ================

static std::string createBBName() {
  static int i = 1;
  return "bb." + std::to_string(i++);
}

Function buildFunction(const nl::json &function) {
  Function program;
  program.name = function["name"];
  // std::cout << program.name << "\n";

  if (function.contains("args")) {
    for (const nl::json &arg : function["args"]) {
      program.args.insert(arg["name"]);
    }
  }

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

void Function::dump() const {
  std::cout << name << ":\n";
  for (const BasicBlock &bb : basic_blocks) {
    bb.dump();
  }
}

// ============ CFG ================

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

  // cfg.function.basic_blocks.insert(cfg.function.basic_blocks.begin(),
  //                                  BasicBlock{.name = "Entry"});
  // cfg.function.basic_blocks.push_back(BasicBlock{.name = "Exit"});

  // for (const std::string &name : entry_succs) {
  //   cfg.predecessors[name].push_back("Entry");
  // }
  // cfg.successors["Entry"] = std::move(entry_succs);

  // for (const std::string &name : exit_preds) {
  //   cfg.successors[name].push_back("Exit");
  // }
  // cfg.predecessors["Exit"] = std::move(exit_preds);

  return cfg;
}

void CFG::dump() const {
  std::cout << "\n";
  if (!successors.empty()) {
    std::cout << "Successors:\n";
    for (const auto &[name, succs] : successors) {
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

void CFG::dumpDot() const {
  std::string file = function.name + ".dot";
  std::ofstream f(file);
  f << "digraph " << function.name << " {\n";
  f << "node [shape=box, style=filled]\n";
  for (const auto &[name, bb] : function.basic_blocks) {
    f << "\"" << name << "\"\n";
  }
  for (const auto &[name, succs] : successors) {
    for (const std::string &succ : succs)
      f << "\"" << name << "\"-> \"" << succ << "\"[color=\"blue\"]\n";
  }
  // for (const auto &[name, preds] : cfg.predecessors) {
  //   for (const std::string &pred : preds)
  //     f << "\"" << name << "\"-> \"" << pred << "\"[color=\"red\"]\n";
  // }
  f << "}";
}

// ============ Dom ================

static void build(CFG &cfg, std::vector<std::string> &postorder,
                  std::set<std::string> &visited, const std::string &root) {
  if (visited.contains(root)) return;
  visited.insert(root);

  for (const std::string &succ : cfg.successors[root]) {
    build(cfg, postorder, visited, succ);
  }
  if (root != "Exit") postorder.push_back(root);
};

static std::vector<std::string> buildPostOrder(CFG &cfg) {
  std::vector<std::string> postorder;
  std::set<std::string> visited;
  build(cfg, postorder, visited, cfg.function.basic_blocks[0].name);
  return postorder;
}

static std::vector<std::string> buildCFGVisitNode(CFG &cfg) {
  std::vector<std::string> postorder = buildPostOrder(cfg);
  std::reverse(postorder.begin(), postorder.end());
  // postorder.erase(postorder.begin());
  return postorder;
}

static DomRelation computeDominators(CFG &cfg) {
  DomRelation dom;

  std::vector<std::string> postorder = buildCFGVisitNode(cfg);

  // dom["Entry"] = {"Entry"};

  for (const BasicBlock &bb : cfg.function.basic_blocks) {
    if (bb.isEntry() || bb.isExit()) continue;
    dom[bb.name] = postorder;
  }

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
      if (!preds.empty()) {
        new_dom = dom[preds[0]];
        for (int i = 1; i < preds.size(); ++i) {
          new_dom = intersect(new_dom, dom[preds[i]]);
        }
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

static std::vector<std::string> symmetric_difference(
    const std::set<std::string> &all_dominators,
    const std::vector<std::string> &idom_cand) {
  std::vector<std::string> result = idom_cand;
  for (const std::string &node : all_dominators) {
    if (auto it = std::find(result.begin(), result.end(), node);
        it != result.end()) {
      result.erase(it);
    }
  }
  return result;
}

static void computeIntermidiateDominators(DomInfo &dom_info, CFG &cfg) {
  for (auto [node, idom_cand] : dom_info.dom) {
    // strip itself, now its sdom.
    idom_cand.erase(std::remove(idom_cand.begin(), idom_cand.end(), node),
                    idom_cand.end());
    if (idom_cand.size() == 1) {
      std::string idom = idom_cand.back();
      idom_cand.pop_back();
      dom_info.idom[node] = idom;
      continue;
    }
    std::set<std::string> all_dominators;
    for (const std::string &node : idom_cand) {
      if (all_dominators.contains(node)) {
        continue;
      }
      // all_dominators |= d.dominators - {d}
      std::set<std::string> node_dom(dom_info.dom[node].begin(),
                                     dom_info.dom[node].end());
      auto it = std::find(node_dom.begin(), node_dom.end(), node);
      assert(it != node_dom.end());
      node_dom.erase(it);
      all_dominators.merge(node_dom);
    }
    // idom_candidates = all_dominators.symmetric_difference(idom_candidates)
    idom_cand = symmetric_difference(all_dominators, idom_cand);
    assert(idom_cand.size() <= 1);
    if (!idom_cand.empty()) {
      std::string idom = idom_cand.back();
      dom_info.idom[node] = idom;
      idom_cand.pop_back();
    }
  }
}

// static void computeDomFrontier(DomInfo &dom_info, CFG &cfg) {
//   std::vector<std::string> postorder = buildCFGVisitNode(cfg);
//   std::string runner;
//
//   for (const std::string &node : postorder) {
//     if (cfg.predecessors[node].size() >= 2) {
//       for (const std::string &father : cfg.predecessors[node]) {
//         runner = father;
//         if (dom_info.idom[node] == father) {
//           dom_info.df[runner].insert(node);
//         }
//         while (dom_info.idom[node] != father) {
//           dom_info.df[runner].insert(node);
//           if (!dom_info.idom[runner].empty()) {
//             std::cout << runner << "\n";
//             abort();
//           }
//           runner = dom_info.idom[runner];
//         }
//       }
//     }
//   }
// }

static auto invert(const DomRelation &dom) {
  DomRelation out;
  for (const auto &[name, succs] : dom) {
    for (const std::string &succ : succs) {
      out[succ].push_back(name);
    }
  }
  return out;
};

static void computeDomFrontier(DomInfo &dom_info, CFG &cfg) {
  DomRelation tree = invert(dom_info.dom);
  for (const auto &block : dom_info.dom) {
    // std::cout << block.first << "\n";
    std::set<std::string> dominated_succs;
    for (const auto &dominated : tree[block.first]) {
      for (const std::string &succ : cfg.successors[dominated]) {
        dominated_succs.insert(succ);
      }
    }
    for (const std::string &b : dominated_succs) {
      const auto &node_all_succs = tree[block.first];
      bool not_found = std::find(node_all_succs.begin(), node_all_succs.end(),
                                 b) == node_all_succs.end();
      bool equal = b == block.first;
      if (not_found || equal) {
        dom_info.df[block.first].insert(b);
      }
    }
  }
}

static void computeDomTree(CFG &cfg, DomInfo &dom_info) {
  DomRelation strict = invert(dom_info.dom);
  for (auto &[a, b] : strict) {
    b.erase(std::remove(b.begin(), b.end(), a), b.end());
  }
  DomRelation strict_2x;
  for (auto &[a, bs] : strict) {
    std::set<std::string> item;
    for (auto &b : bs) {
      for (const std::string &d : strict[b]) {
        item.insert(d);
      }
    }
    strict_2x[a] = std::vector<std::string>(item.begin(), item.end());
  }

  for (auto &[a, bs] : strict) {
    for (auto &b : bs) {
      auto &strict_2x_a = strict_2x[a];
      if (std::find(strict_2x_a.begin(), strict_2x_a.end(), b) ==
          strict_2x_a.end()) {
        dom_info.dom_tree[a].insert(b);
      }
    }
  }
}

DomInfo computeDomInfo(CFG &cfg) {
  DomInfo dom_info;
  dom_info.dom = computeDominators(cfg);
  computeIntermidiateDominators(dom_info, cfg);
  computeDomFrontier(dom_info, cfg);
  computeDomTree(cfg, dom_info);
  return dom_info;
}

static std::map<std::string, std::set<std::string>> getDefBlockMap(
    Function &function) {
  std::map<std::string, std::set<std::string>> out;
  for (BasicBlock &bb : function.basic_blocks) {
    for (Instruction &instr : bb.data) {
      if (instr.contains("dest")) {
        out[instr["dest"]].insert(bb.name);
      }
    }
  }
  return out;
}

void DomInfo::dump() {
  auto print = [](const auto &dom) {
    for (const auto &[name, doms] : dom) {
      std::cout << name << ": [";
      for (const std::string &d : doms) {
        std::cout << d << ", ";
      }
      std::cout << "]\n";
    }
  };

  std::cout << "dom:\n";
  print(dom);

  std::cout << "idom:\n";
  for (const auto &[name, idom] : idom) {
    std::cout << name << ": [";
    if (!idom.empty()) std::cout << idom;
    std::cout << "]\n";
  }

  std::cout << "dominator frontier:\n";
  print(df);
}

// ============ SSA ================

// Block name <=> variable name
static PhiMap getPhis(Function &function, DomInfo &dom_info) {
  auto defs = getDefBlockMap(function);
  auto &df = dom_info.df;
  std::map<std::string, std::set<std::string>> phis;
  for (auto [v, def_list] : defs) {
    // 遍历定义某个变量的基本块
    for (const std::string &d : def_list) {
      // 遍历基本块的支配边界
      for (const std::string &block : df[d]) {
        // 支配边界就是需要插入 phi node 的地方
        if (!phis[block].contains(v)) {
          phis[block].insert(v);
          // phi node 也是定义该变量的一个基本块，所以也将它加入 def_list 中
          if (!def_list.contains(block)) {
            def_list.insert(block);
          }
        }
      }
    }
  }
  for (const auto &[block_name, vars] : phis) {
    std::cout << block_name << ": [";
    for (const std::string &var : vars) {
      std::cout << var << ", ";
    }
    std::cout << "]\n";
  }
  return phis;
}

struct SSAReanme {
  CFG &cfg;
  Function &function;
  DomInfo &dom_info;
  PhiMap phis;
  std::map<std::string, int> counters;
  using Stack = std::map<std::string, std::vector<std::string>>;
  Stack stack;

  std::map<
      std::string,
      std::map<std::string, std::vector<std::pair<std::string, std::string>>>>
      phi_args;

  std::map<std::string, std::map<std::string, std::string>> phi_dests;

  SSAReanme(CFG &cfg, Function &function, DomInfo &dom_info)
      : cfg(cfg), function(function), dom_info(dom_info) {
    phis = getPhis(function, dom_info);
  }

  std::string pushFresh(const std::string &var) {
    int index = counters[var]++;
    std::string fresh = var + "." + std::to_string(index);
    auto &var_stack = stack[var];
    var_stack.insert(stack[var].begin(), fresh);
    return fresh;
  }

  void doRename(const std::string &block) {
    // Copy save the old stack.
    Stack old_stack = stack;

    // Rename phi node dests.
    for (const std::string &p : phis[block]) {
      phi_dests[block][p] = pushFresh(p);
    }

    auto it =
        std::find_if(function.basic_blocks.begin(), function.basic_blocks.end(),
                     [&](const BasicBlock &bb) { return bb.name == block; });
    assert(it != function.basic_blocks.end());

    for (Instruction &instr : it->data) {
      // rename args of normal instructions
      if (instr.contains("args")) {
        std::vector<std::string> new_args;
        for (const std::string &arg : instr["args"]) {
          new_args.push_back(arg);
        }
        instr["args"] = new_args;
      }
      // rename dest
      if (instr.contains("dest")) {
        instr["dest"] = pushFresh(instr["dest"]);
      }
    }

    // rename phis
    for (const std::string &s : cfg.successors[block]) {
      for (const std::string &p : phis[s]) {
        if (!stack[p].empty()) {
          phi_args[s][p].emplace_back(block, stack[p][0]);
        } else {
          phi_args[s][p].emplace_back(block, "__undef");
        }
      }
    }
    // recursive calls.
    for (const std::string &b : dom_info.dom_tree[block]) {
      doRename(b);
    }
    stack = old_stack;
  }

  void insertPhis() {
    for (auto &[block, instrs] : function.basic_blocks) {
      // insert
    }
  }

  Function convert() {
    // rename
    BasicBlock &entry = function.basic_blocks[0];
    std::cout << "Start renaming from " << entry.name << "\n";
    doRename(entry.name);
    // insert phis
    insertPhis();
    return function;
  }
};

Function convertToSSA(CFG &cfg, Function function, DomInfo &dom) {
  SSAReanme ssa(cfg, function, dom);
  return ssa.convert();
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
    Function func = buildFunction(function);
    // dumpFunction(func);
    CFG cfg = buildCFG(func);
    // dumpCFG(cfg);
    // dumpCFGToDot(cfg);
    DomInfo dom = computeDomInfo(cfg);
    // dumpDom(dom);
    Function ssa = convertToSSA(cfg, func, dom);
    // dumpFunction(ssa);
  }
}
