#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "basic_block.h"
#include "cfg.h"
#include "context.h"
#include "dom.h"
#include "function.h"
#include "instruction.h"
#include "ssa.h"
#include "transform.h"

void usage() {
  std::cout << "Usage:\n";
  std::cout << "$ cat test.bril | bril2json | brandy\n";
  std::cout << "$ brandy test.json\n";
  exit(-1);
}

int main(int argc, char** argv) {
  nl::json ir;
  if (argc > 2) usage();

  if (argc == 1) {
    ir = nl::json::parse(std::cin);
  } else {
    std::string file = argv[1];
    if (file.empty() || !std::filesystem::exists(file)) {
      std::cout << "Error: Invalid input\n";
      usage();
    }
    std::ifstream f(file);
    ir = nl::json::parse(f);
  }

  Context ctx;

  for (const nl::json& input : ir["functions"]) {
    Function* function = Function::Create(&ctx, input);
    CFG cfg = BuildCFG(*function);
    DomInfo dom = ComputeDomInfo(cfg);
    ToSSA(&ctx, *function, cfg, dom);
    Optimize(*function);

    nl::json prog;
    prog["functions"].push_back(function->ToJson());
    std::cout << prog << "\n";
  }
}
