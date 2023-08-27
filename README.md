<div align="center">
  <h1>🐱 Brandy</code></h1>

  <p>
    <strong>C++ implementation of Bril compiler</strong>
  </p>
</div>

> “What I cannot create, I do not understand”

## About

I want to learn about compiler internals, specifically IR analysis and optimization. Although LLVM is a valuable resource, it can be quite complex. Therefore, I have decided to develop my own compiler because hands-on experience is the most effective way to learn.

To keep things as simple as possible, my compiler does not currently have a frontend, and it may never have one. Also, I prefer not to write the IR frontend and the serialization framework.
Brandy uses Bril, which is an educational IR used in Cornel CS 6120. Bril's canonical representation is JSON so all Instructions in Brandy is just a thin wrapper of a JSON object, which makes things a lot easier so I can focus on the analysis and optimization part.

Below is the official introduction about Bril language:
> Bril (the Big Red Intermediate Language) is a compiler IR made for teaching CS 6120, a grad compilers course.
> 
> It is an extremely simple instruction-based IR that is meant to be extended.
Its canonical representation is JSON, which makes it easy to build tools from scratch to manipulate it.

* [docs](https://capra.cs.cornell.edu/bril/)
* [langref](https://capra.cs.cornell.edu/bril/lang/index.html)

## Build
```bash
mkdir build && cd build
cmake ../ && make
```

## Install the parser
```bash
pip3 install --user flit
cd bril-txt
flit install --symlink --user
```
## Install the IR interpreter (optional)

First install [deno](https://deno.com/)
```bash
deno install brili.ts
```
Then add deno to your $PATH

## Run the compiler
```bash
cat test/loop-ssa.bril | bril2json | build/bin/brandy | bril2txt
```
