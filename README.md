# Compiler Engine Language (CEL)

A simple, statically-typed programming language compiler built from scratch in C++ with **full VS Code syntax highlighting support**.

## ✨ Features

- **Lexical Analysis**: Complete tokenizer supporting all language constructs
- **Parser**: Recursive descent parser building Abstract Syntax Trees (AST)
- **Interpreter**: Tree-walking interpreter with scoped environments
- **Type System**: Support for int, float, string, and bool types
- **Control Flow**: if/else, while, and for loops
- **Functions**: First-class function declarations with parameters and return types
- **VS Code Integration**: Syntax highlighting, color themes, auto-formatting
- **Smart Editor**: Auto-indentation, bracket matching, code folding

## Language Syntax

### Variable Declaration

```lang
var name: type = value;
const name: type = value;
```

### Functions

```lang
func functionName(param1: type1, param2: type2) -> returnType {
    // function body
    return value;
}
```

### Control Flow

```lang
if (condition) {
    // true block
} else {
    // false block
}

while (condition) {
    // loop body
}

for (var i: int = 0; i < 10; i = i + 1) {
    // loop body
}
```

### Types

- `int`: 32-bit integer
- `float`: Single precision floating point
- `string`: Text string
- `bool`: Boolean value (true/false)
- `void`: No return value

### Operators

#### Arithmetic
- `+` Addition
- `-` Subtraction
- `*` Multiplication
- `/` Division
- `%` Modulo

#### Comparison
- `==` Equality
- `!=` Inequality
- `<` Less than
- `>` Greater than
- `<=` Less than or equal
- `>=` Greater than or equal

#### Logical
- `&&` Logical AND
- `||` Logical OR
- `!` Logical NOT

## VS Code Setup

Your project includes complete VS Code syntax highlighting! ✨

### Features
- 🎨 **Syntax Highlighting** - Keywords, types, strings, numbers, comments
- 🎯 **Color Themes** - Light and Dark themes optimized for readability
- ⚙️ **Smart Formatting** - Auto-indentation, bracket closing, code folding
- 🔧 **Language Configuration** - Comment syntax, bracket matching

### Quick Start in VS Code

1. Open this project in VS Code
2. Open any `.lang` file - highlighting works automatically
3. Optional: Change theme
   - Cmd+Shift+P → "Color Theme"
   - Select "Compiler Engine Light" or "Compiler Engine Dark"

### Configuration Files

All in `.vscode/`:
- `language-configuration.json` - Editor behavior (comments, brackets)
- `settings.json` - Editor preferences
- `syntaxes/lang.tmLanguage.json` - Tokenization/grammar rules
- `themes/` - Light and Dark color themes

### Customize Colors

Edit `.vscode/themes/compiler-engine-{light,dark}.json` to change colors. VS Code reloads changes automatically (Cmd+R).

For details, see [VSCODE_SETUP.md](./VSCODE_SETUP.md) and [SYNTAX_HIGHLIGHTING.md](./SYNTAX_HIGHLIGHTING.md).

## Building

### Prerequisites
- C++17 compatible compiler
- CMake 3.10 or higher

### Build Steps

```bash
cd /Users/damian/game-engine
mkdir -p build
cd build
cmake -S .. -B .
cmake --build .
```

### Running

Execute a script file:
```bash
./compiler examples/test.lang
```

Interactive mode:
```bash
./compiler
```

## Architecture

### Lexer (`lexer.h`, `lexer.cpp`)
- Tokenizes source code
- Handles keywords, operators, literals, and identifiers
- Skips comments and whitespace
- Tracks line and column information

### Parser (`parser.h`, `parser.cpp`)
- Builds Abstract Syntax Tree from tokens
- Implements recursive descent parsing
- Follows operator precedence and associativity
- Error handling with meaningful messages

### AST (`ast.h`, `ast.cpp`)
- Node classes for all language constructs
- Visitor pattern implementation
- Supports expressions, statements, and declarations

### Interpreter (`interpreter.h`, `interpreter.cpp`)
- Tree-walking interpreter
- Environment-based variable scoping
- Type coercion and value operations
- Function call support with parameter passing

## Example Programs

### Variables and Arithmetic
```lang
var x: int = 10;
var y: int = 20;
var result: int = x + y;
```

### Functions
```lang
func add(a: int, b: int) -> int {
    return a + b;
}

func main() -> void {
    var sum: int = add(5, 3);
}
```

### Loops and Conditionals
```lang
func countToTen() -> void {
    var i: int = 0;
    while (i < 10) {
        i = i + 1;
    }

    if (i == 10) {
        // Success
    }
}
```

## Error Handling

The compiler provides detailed error messages for:
- Syntax errors with line and column information
- Undefined variables or functions
- Type mismatches (basic)
- Parse errors with context

## Future Enhancements

- [ ] Arrays and pointers
- [ ] Structs and classes
- [ ] Standard library functions
- [ ] Type inference
- [ ] Error recovery
- [ ] Optimization passes
- [ ] Code generation to bytecode
- [ ] JIT compilation

## License

MIT License
