# Axolotl (AXO)

![Axolotl Icon](lang-ext/assets/icon.png)

Axolotl is a small, statically-typed language and compiler implemented in modern C++. This repository contains the compiler, a tree-walking interpreter, and a VS Code language extension (syntax, grammar, and file icons).

**Status:** active development — builds and runs sample programs in `examples/`.

**Highlights:** lexer, parser (recursive-descent), interpreter, VS Code syntax & icons, and pretty diagnostics.

## **Features**
- **Lexer**: tokenizes source with line/column tracking
- **Parser**: recursive descent parser producing an AST
- **Interpreter**: tree-walking execution with scoped environments
- **Types**: `int`, `float`, `string`, `bool`, `void`, `object`, array types (`[type]`)
- **Control flow**: `if/else`, `while`, `for` (use `var` in `for` initializers)
- **Functions**: named and inline `func` values with typed parameters/returns
- **VS Code integration**: syntax highlighting, grammar, and an icon theme
- **Pretty errors**: parse errors include file, line, column and a caret pointer to the offending token

## **Language Quick Reference**

Variable declarations

```
var name: type = value;
const name: type = value;
```

Functions

```
func functionName(param1: type1, param2: type2) -> returnType {
    // body
}
```

For loops (note: the parser expects `var` or an expression for the initializer):

```
for (var i: int = 0; i < len(arr); i = i + 1) {
    // ...
}
```

Array type

```
const a: [int] = [1, 2, 3];
```

Operators: arithmetic `+ - * / %`, comparison `== != < > <= >=`, logical `&& || !`.

## **Repository Layout**
- `src/` — C++ sources: lexer, parser, interpreter, main, etc.
- `include/` — public headers
- `examples/` — example `.axo` programs
- `lang-ext/` — VS Code extension (language, grammar, icon theme)
- `build/` — CMake build outputs

## **Build & Run**

Prerequisites
- C++17 toolchain (Apple Clang or GCC/Clang)
- CMake 3.10+

From the repository root (recommended):

```bash
# configure + build (out-of-source)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# run an example
./build/compiler examples/test.axo

# interactive REPL mode
./build/compiler
```

If you previously built in a different folder, remove `build/` and re-run `cmake -S . -B build` to avoid stale cache issues.

## **VS Code Extension (language + icons)**

The `lang-ext/` folder contains a small VS Code extension providing syntax highlighting and an icon theme.

Quick steps to test the extension locally (dev host):

```bash
# open VS Code to the extension folder and press F5
code --extensionDevelopmentPath=/path/to/Axolotl/lang-ext
```

Package and install the VSIX (to test in regular VS Code):

```bash
cd lang-ext
npm install -g vsce   # if you don't have it
npm run build          # uses `vsce package` (see package.json)
# installs lang-syntax-<version>.vsix
code --install-extension ./lang-syntax-1.0.0.vsix
```

After installing or running the dev host:
- In Command Palette → `Preferences: File Icon Theme` → choose `Axolotl Icons` to enable file icons for `.axo` files.
- If icons don't appear: `Developer: Reload Window`, ensure the extension is enabled, and that `.axo` is selected as the language for your file.

## **Error Reporting / Diagnostics**

The compiler now produces improved diagnostics for parse errors. Example:

```
Fatal error: Expected ']' after array type (line 1, col 14)
  File: examples/bad_array.axo:1:14
const a:[int = 5;
             ^
```

Notes:
- Parse errors carry token line/column and show the source line with a caret marker.
- This formatting is produced when running `./build/compiler <file>`; interactive mode prints similar errors when a complete expression/block is entered.

## **Examples**

Run the example suite to try small programs in `examples/`:

```bash
./build/compiler examples/test.axo
```

Open `examples/test.axo` and `examples/showcase.axo` to see language features.

## **Development Notes**

- `Token` objects track `line` and `column` in `include/token.h`.
- `ParseError` (in `include/parser.h`) now stores token location and value — the `main` runner formats these into the caret-style diagnostic.
- If you change parsing/lexing behavior, update tests and examples accordingly.

## **Contributing / Roadmap**

- Improve type checking and error recovery
- Add more standard library functions
- Add bytecode backend and JIT

## **License**

MIT

- Function call support with parameter passing
