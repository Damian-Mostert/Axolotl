# Axolotl Language Extension v2.0

🦎 **Complete language support for Axolotl (.axo) files with advanced IntelliSense, type checking, and error detection.**

## ✨ Features

### 🧠 **IntelliSense & Auto-Completion**
- **Smart type suggestions** - Context-aware type completions including union types, arrays, and objects
- **Function auto-completion** - All built-in functions with parameter hints and documentation
- **Variable suggestions** - Auto-complete variables, functions, and types from current file
- **Keyword snippets** - Intelligent code snippets for all language constructs
- **Template snippets** - Ready-to-use templates for common patterns (classes, tests, pipelines)

### 🔍 **Hover Information**
- **Function signatures** - See parameter types and return types on hover
- **Type information** - View variable types and custom type definitions
- **Documentation** - Built-in function documentation with usage examples

### ⚠️ **Error Detection & Diagnostics**
- **Undefined variable detection** - Catch undefined variables in real-time
- **Type mismatch warnings** - Basic type checking for variable assignments
- **Missing semicolon detection** - Syntax error prevention
- **Real-time error highlighting** - See errors as you type

### 🎯 **Advanced Language Support**
- **Switch-case statements** with fallthrough support
- **Union types** (`string|int|bool`) with type checking
- **Custom type declarations** with IntelliSense support
- **Import/export system** with module resolution
- **Try-catch-finally** error handling
- **Array and object manipulation** with type safety
- **Function expressions** and higher-order functions

### 🛠️ **Development Tools**
- **Compile command** (`Ctrl+Shift+B`) - Compile current file
- **Run command** (`F5`) - Run current Axolotl file
- **Integrated terminal** - Execute Axolotl programs directly
- **Problem panel** - View all errors and warnings

## 🚀 Quick Start

### Basic Usage
```axolotl
// Type 'main' and press Tab for instant main function
func main() -> void {
    var message: string = "Hello, Axolotl!";
    print(message);
}

main();
```

### Advanced Features
```axolotl
// Union types with IntelliSense support
type ID = string|int;
type User = {name: string, id: ID, active: bool};

// Switch statements with auto-completion
func handleUser(user: User) -> string {
    switch (typeof(user.id)) {
        case "string":
            return "String ID: " + user.id;
        case "int":
            return "Numeric ID: " + toString(user.id);
        default:
            return "Unknown ID type";
    }
}

// Error handling with try-catch
func safeOperation() -> string {
    try {
        // Your code here
        return "Success";
    } catch (error) {
        print("Error:", error);
        return "Failed";
    }
}
```

## 🎯 Snippets & Templates

| Prefix | Description | Output |
|--------|-------------|--------|
| `func` | Function declaration | `func name(params) -> returnType { }` |
| `var` | Variable declaration | `var name: type = value;` |
| `const` | Constant declaration | `const name: type = value;` |
| `if` | If statement | `if (condition) { }` |
| `switch` | Switch statement | `switch (expr) { case: break; }` |
| `try` | Try-catch block | `try { } catch (e) { }` |
| `type` | Type declaration | `type Name = definition;` |
| `class` | Class-like structure | Complete class template |
| `test` | Test function | Test function template |
| `main` | Main function | Main function with call |

## 🔧 Configuration

Configure the extension in VS Code settings:

```json
{
    "axolotl.enableIntelliSense": true,
    "axolotl.enableTypeChecking": true,
    "axolotl.compilerPath": "./build/compiler",
    "axolotl.maxNumberOfProblems": 100
}
```

## 🎮 Keyboard Shortcuts

- `F5` - Run current Axolotl file
- `Ctrl+Shift+B` - Compile current file
- `Ctrl+Space` - Trigger IntelliSense
- `Ctrl+Shift+P` → "Axolotl" - Access all commands

## 📚 Language Reference

### Types
- **Primitives**: `int`, `float`, `string`, `bool`, `void`, `any`
- **Arrays**: `[int]`, `[string]`, `[[int]]` (nested)
- **Objects**: `{name: string, age: int}`
- **Union Types**: `string|int|bool`
- **Custom Types**: `type MyType = definition;`

### Built-in Functions
- **I/O**: `print()`, `read()`, `write()`, `copy()`
- **Arrays**: `len()`, `push()`, `pop()`
- **Strings**: `substr()`, `toUpper()`, `toLower()`, `indexOf()`, `contains()`
- **Utility**: `typeof()`, `toString()`, `millis()`, `sleep()`

### Control Flow
- **Conditionals**: `if/else`, `switch/case/default`
- **Loops**: `while`, `for`
- **Error Handling**: `try/catch/finally`, `throw`
- **Flow Control**: `break`, `continue`, `return`

### Modules
- **Import**: `import {exports} from "module";`
- **Export**: `export declaration;`
- **Use**: `use "module";`

## 🐛 Troubleshooting

### Common Issues
1. **IntelliSense not working**: Ensure file is saved as `.axo` extension
2. **Compiler not found**: Set correct `axolotl.compilerPath` in settings
3. **Icons not showing**: Enable "Axolotl Icons" file icon theme

### Debug Mode
Enable debug logging:
```json
{
    "axolotl.trace.server": "verbose"
}
```

## 🤝 Contributing

We welcome contributions! Here's how to help:

1. **Report Issues**: Found a bug? Report it on GitHub
2. **Suggest Features**: Have an idea? Open a feature request
3. **Contribute Code**: Fork, code, and submit a pull request
4. **Improve Docs**: Help make documentation better

## 📄 License

MIT License - see LICENSE file for details

---

**Made with ❤️ for the Axolotl programming language community**