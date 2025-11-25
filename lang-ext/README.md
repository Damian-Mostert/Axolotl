# Axolotl Language Extension for VS Code

Complete language support for the Axolotl programming language (.axo files) in Visual Studio Code.

## Features

### 🎨 Syntax Highlighting
- **Keywords**: `func`, `var`, `const`, `type`, `program`, `if`, `else`, `while`, `for`, `return`, `import`, `use`, `await`, `typeof`
- **Types**: Primitive types (`int`, `float`, `string`, `bool`, `void`, `any`, `object`) and custom types
- **Operators**: Arithmetic, comparison, logical, and assignment operators
- **Literals**: Numbers, strings, booleans, arrays, and objects
- **Comments**: Line comments (`//`) and block comments (`/* */`)
- **Advanced Types**: Union types (`|`), array types (`[type]`), object types (`{field: type}`)

### 📝 Code Snippets
Comprehensive snippets for common Axolotl patterns:
- `func` - Function declaration
- `var` / `const` - Variable declarations
- `type` - Custom type declaration
- `union` - Union type with string literals
- `objtype` - Object type definition
- `tuple` - Tuple type definition
- `if` / `ifelse` - Conditional statements
- `for` / `while` - Loop constructs
- `array` / `mixarray` - Array declarations
- `object` / `objarr` - Object declarations
- `program` - Program declaration
- `main` - Main function template
- `recursive` - Recursive function template
- `strops` - String operations
- `arrops` - Array operations
- `objaccess` - Complex object access patterns

### 🔧 Language Features
- **Auto-closing pairs**: Brackets, parentheses, quotes
- **Bracket matching**: Highlight matching brackets
- **Code folding**: Fold functions, blocks, and regions
- **Smart indentation**: Automatic indentation for blocks
- **Word pattern recognition**: Proper word boundaries for variables and functions

### 🚨 Error Detection
- **Problem matcher**: Detects compilation errors with file, line, and column information
- **Error patterns**: Recognizes Axolotl compiler error format
- **Syntax validation**: Highlights syntax errors in real-time

### 🛠️ Build Integration
- **Tasks**: Pre-configured build and run tasks
- **Problem matchers**: Integration with Axolotl compiler output
- **Debugging**: Launch configurations for debugging Axolotl programs

## Language Overview

Axolotl is a statically-typed language with modern features:

### Basic Types
```axolotl
var number: int = 42;
var pi: float = 3.14;
var message: string = "Hello, World!";
var flag: bool = true;
```

### Custom Types
```axolotl
// Union types
type Status = "active" | "inactive" | "pending";

// Object types
type User = {
    name: string,
    age: int,
    status: Status
};

// Array types
type Coordinates = [int, int];
type Numbers = [int];
```

### Functions
```axolotl
func factorial(n: int) -> int {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
```

### Arrays and Objects
```axolotl
var numbers: [int] = [1, 2, 3, 4, 5];
var mixed: [int|string] = [1, "two", 3, "four"];

var person: object = {
    name: "Alice",
    age: 30,
    hobbies: ["reading", "coding"]
};
```

### Advanced Features
```axolotl
// Programs (async execution)
program backgroundTask(delay: int) {
    sleep(delay);
    print("Task completed!");
}

// Function expressions
var callback: (int) -> string = func(n: int) -> string {
    return "Number: " + n;
};

// Type checking
if (typeof x == "int") {
    print("x is an integer");
}
```

## Installation

1. **From VSIX**: Install the `.vsix` file directly in VS Code
2. **Development**: Clone the repository and press F5 to run in development mode

### Building the Extension
```bash
cd lang-ext
npm install -g vsce
npm run build
```

### Installing the VSIX
```bash
code --install-extension ./axo-syntax-1.1.0.vsix
```

## Usage

1. **File Association**: Files with `.axo` or `.axolotl` extensions are automatically recognized
2. **Icon Theme**: Enable "Axolotl Icons" in VS Code settings for custom file icons
3. **Syntax Highlighting**: Automatic syntax highlighting for Axolotl code
4. **Snippets**: Type snippet prefixes and press Tab to expand
5. **Build Tasks**: Use Ctrl+Shift+P → "Tasks: Run Task" → "Build Axolotl"

## Configuration

### Recommended Settings
Add to your VS Code `settings.json`:
```json
{
    "files.associations": {
        "*.axo": "axo",
        "*.axolotl": "axo"
    },
    "editor.tabSize": 4,
    "editor.insertSpaces": true,
    "editor.detectIndentation": false
}
```

### Workspace Setup
For Axolotl projects, add these files to your `.vscode` folder:

**tasks.json** (Build and run tasks)
**launch.json** (Debug configurations)

## Examples

The extension recognizes all Axolotl language constructs:

### Type Definitions
```axolotl
type Direction = "north" | "south" | "east" | "west";
type Priority = 1 | 2 | 3 | 4 | 5;

type Product = {
    id: int,
    name: string,
    price: float,
    available: bool
};
```

### Complex Data Structures
```axolotl
var users: [object] = [
    {name: "Alice", scores: [10, 20, 30]},
    {name: "Bob", scores: [15, 25, 35]}
];

print(users[0].name);           // Alice
print(users[0].scores[1]);      // 20
```

### Error Handling Patterns
```axolotl
if (typeof data == "array" && len(data) > 0) {
    print("Processing", len(data), "items");
} else {
    print("No data to process");
}
```

## Contributing

1. Fork the repository
2. Make your changes
3. Test with various Axolotl code samples
4. Submit a pull request

## License

MIT License - see LICENSE.md for details

## Support

- **Issues**: Report bugs and feature requests on GitHub
- **Documentation**: See the main Axolotl repository for language documentation
- **Examples**: Check the `examples/` folder for sample Axolotl programs