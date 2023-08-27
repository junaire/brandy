#pragma once

class Function;

void die(Function &func);

void cse(Function &func);

void CopyProp(Function &func);

inline void Optimize(Function &func) {
  die(func);
  cse(func);
  CopyProp(func);
}
