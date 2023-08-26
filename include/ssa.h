#pragma once

#include <map>
#include <set>
#include <string>

class Function;
class CFG;
class DomInfo;
class BasicBlock;
class Context;

using PhiMap = std::map<BasicBlock *, std::set<std::string>>;

void ToSSA(Context *ctx, Function &function, CFG &cfg, DomInfo &dom);
