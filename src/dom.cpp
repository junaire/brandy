#include "dom.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>
#include <vector>

#include "basic_block.h"
#include "cfg.h"
#include "function.h"

static void build(CFG &cfg, std::vector<BasicBlock *> &postorder,
                  std::set<BasicBlock *> &visited, BasicBlock *root) {
  if (visited.contains(root)) return;
  visited.insert(root);

  for (BasicBlock *succ : cfg.successors[root]) {
    build(cfg, postorder, visited, succ);
  }
  postorder.push_back(root);
};

static std::vector<BasicBlock *> buildPostOrder(CFG &cfg) {
  std::vector<BasicBlock *> postorder;
  std::set<BasicBlock *> visited;
  build(cfg, postorder, visited, cfg.function->basic_blocks[0]);
  return postorder;
}

static std::vector<BasicBlock *> buildCFGVisitNode(CFG &cfg) {
  std::vector<BasicBlock *> postorder = buildPostOrder(cfg);
  std::reverse(postorder.begin(), postorder.end());
  return postorder;
}

static void computeDominators(DomInfo &dom_info, CFG &cfg) {
  std::vector<BasicBlock *> postorder = buildCFGVisitNode(cfg);

  for (BasicBlock *bb : cfg.function->basic_blocks) {
    if (bb->isEntry()) continue;
    dom_info.dom[bb] = postorder;
  }

  auto intersect = [](std::vector<BasicBlock *> new_dom,
                      std::vector<BasicBlock *> dom) {
    std::vector<BasicBlock *> intersect;
    for (BasicBlock *node : dom) {
      if (std::find(new_dom.begin(), new_dom.end(), node) != new_dom.end()) {
        intersect.push_back(node);
      }
    }
    return intersect;
  };

  while (true) {
    bool changed = false;
    for (BasicBlock *node : postorder) {
      std::vector<BasicBlock *> new_dom;

      // intersect preds' dom.
      std::vector<BasicBlock *> preds = cfg.predecessors[node];
      if (!preds.empty()) {
        new_dom = dom_info.dom[preds[0]];
        for (int i = 1; i < preds.size(); ++i) {
          new_dom = intersect(new_dom, dom_info.dom[preds[i]]);
        }
      }
      // union node itself
      new_dom.push_back(node);

      // if new_dom != dom
      if (new_dom != dom_info.dom[node]) {
        dom_info.dom[node] = new_dom;
        changed = true;
      }
    }
    if (!changed) break;
  }
}

static std::vector<BasicBlock *> SymmetricDifference(
    const std::set<BasicBlock *> &all_dominators,
    const std::vector<BasicBlock *> &idom_cand) {
  std::vector<BasicBlock *> result =
      std::vector<BasicBlock *>(idom_cand.begin(), idom_cand.end());
  for (const BasicBlock *node : all_dominators) {
    if (auto it = std::find(result.begin(), result.end(), node);
        it != result.end()) {
      result.erase(it);
    }
  }
  return result;
}

static void computeIntermidiateDominators(DomInfo &dom_info, CFG &cfg) {
  for (auto [node, idom_cand] : dom_info.dom) {
    // strip itself, now it's sdom.
    idom_cand.erase(std::remove(idom_cand.begin(), idom_cand.end(), node),
                    idom_cand.end());

    if (idom_cand.size() == 1) {
      BasicBlock *idom = idom_cand.back();
      idom_cand.pop_back();
      dom_info.idom[node] = idom;
      continue;
    }

    std::set<BasicBlock *> all_dominators;
    for (BasicBlock *node : idom_cand) {
      if (all_dominators.contains(node)) continue;

      // all_dominators |= d.dominators - {d}
      std::set<BasicBlock *> node_dom(dom_info.dom[node].begin(),
                                      dom_info.dom[node].end());

      auto it = std::find(node_dom.begin(), node_dom.end(), node);
      assert(it != node_dom.end());
      node_dom.erase(it);
      all_dominators.merge(node_dom);
    }

    // idom_candidates = all_dominators.symmetric_difference(idom_candidates)
    idom_cand = SymmetricDifference(all_dominators, idom_cand);
    assert(idom_cand.size() <= 1);
    if (!idom_cand.empty()) {
      BasicBlock *idom = *idom_cand.begin();
      dom_info.idom[node] = idom;
      idom_cand.clear();
    }
  }
}

static auto invert(const DomRelation &dom) {
  DomRelation out;
  for (const auto &[node, succs] : dom) {
    for (BasicBlock *succ : succs) {
      out[succ].push_back(node);
    }
  }
  return out;
};

static void computeDomFrontier(DomInfo &dom_info, CFG &cfg) {
  DomRelation tree = invert(dom_info.dom);
  for (const auto &[block, doms] : dom_info.dom) {
    std::set<BasicBlock *> dominated_succs;
    for (const auto &dominated : tree[block]) {
      for (BasicBlock *succ : cfg.successors[dominated]) {
        dominated_succs.insert(succ);
      }
    }
    for (BasicBlock *b : dominated_succs) {
      const auto &node_all_succs = tree[block];
      bool not_found = std::find(node_all_succs.begin(), node_all_succs.end(),
                                 b) == node_all_succs.end();
      if (not_found || (b == block)) {
        dom_info.df[block].push_back(b);
      }
    }
  }
}

static void computeDomTree(DomInfo &dom_info, CFG &cfg) {
  DomRelation strict = invert(dom_info.dom);
  for (auto &[a, b] : strict) {
    b.erase(std::remove(b.begin(), b.end(), a), b.end());
  }
  DomRelation strict_2x;
  for (auto &[a, bs] : strict) {
    std::set<BasicBlock *> item;
    for (auto &b : bs) {
      for (BasicBlock *d : strict[b]) {
        item.insert(d);
      }
    }
    strict_2x[a] = std::vector<BasicBlock *>(item.begin(), item.end());
  }

  for (auto &[a, bs] : strict) {
    for (auto &b : bs) {
      auto &strict_2x_a = strict_2x[a];
      if (std::find(strict_2x_a.begin(), strict_2x_a.end(), b) ==
          strict_2x_a.end()) {
        dom_info.dom_tree[a].push_back(b);
      }
    }
  }
}

DomInfo ComputeDomInfo(CFG &cfg) {
  DomInfo dom_info;
  computeDominators(dom_info, cfg);
  computeIntermidiateDominators(dom_info, cfg);
  computeDomFrontier(dom_info, cfg);
  computeDomTree(dom_info, cfg);
  return dom_info;
}

bool DomInfo::IsDominate(const Instruction &a, const Instruction &b) {
  BasicBlock *x = a.parent;
  BasicBlock *y = b.parent;
  assert(x);
  assert(y);

  // if x == y then check if x executes before y.
  if (x == y) {
    auto x_it = std::find(x->instrs.begin(), x->instrs.end(), &a);
    auto y_it = std::find(y->instrs.begin(), y->instrs.end(), &b);
    assert(x_it != x->instrs.end());
    assert(y_it != y->instrs.end());
    return x_it > y_it;
  }

  // else check if y is in x's dom_tree.
  if (auto it = dom_tree.find(x); it != dom_tree.end()) {
    return std::find(it->second.begin(), it->second.end(), y) !=
           it->second.end();
  }
  return false;
}

void DomInfo::dump() const {
  auto print = [](const auto &dom) {
    for (const auto &[block, doms] : dom) {
      std::cout << block->name << ": [";
      for (const BasicBlock *d : doms) {
        std::cout << d->name << ", ";
      }
      std::cout << "]\n";
    }
  };

  std::cout << "dom:\n";
  print(dom);

  std::cout << "idom:\n";
  for (const auto &[block, idom] : idom) {
    std::cout << block->name << ": [";
    if (idom) std::cout << idom->name;
    std::cout << "]\n";
  }

  std::cout << "dominator frontier:\n";
  print(df);

  std::cout << "dom tree:\n";
  print(dom_tree);
}
