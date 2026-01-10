# VS Code Extension Enhancements

## New Features

### 1. Type Inference Support
The extension now supports type inference for variables declared without explicit type annotations:

```axolotl
// Old syntax (still supported)
const player: any = BoxGeometry(0.8, 1.6, 0.8);

// New syntax with type inference
const player = BoxGeometry(0.8, 1.6, 0.8);  // Inferred as 'any'
```

The extension will:
- Automatically infer types from initializers
- Show inferred types in hover tooltips
- Display "(inferred: type)" in autocomplete suggestions

### 2. Enhanced Error Detection

#### Const Reassignment
```axolotl
const x = 10;
x = 20;  // Error: Cannot reassign to const variable 'x'
```

#### Undefined Variables
```axolotl
undeclaredVar = 5;  // Error: Variable 'undeclaredVar' is not declared
```

#### Type Mismatches
```axolotl
var count: int = 0;
count = "string";  // Warning: Type mismatch: cannot assign 'string' to 'int'
```

#### Undefined Functions
```axolotl
unknownFunc();  // Warning: Function 'unknownFunc' is not defined
```

#### Unused Variables
```axolotl
const unused = 10;  // Hint: Variable 'unused' is declared but never used
```

### 3. Improved Variable Tracking

The extension now tracks:
- Variable scope (global, param)
- Whether variables are const or var
- Initializer expressions
- Type inference status
- Variable usage

### 4. Builtin Return Type Detection

The builtin extraction scripts now capture:
- Function signatures with parameters
- Return types
- Parameter types (when annotated)

Format in C++ source:
```cpp
//@desc Description of the function
// @params param1: type1, param2: type2
// @return returnType
std::string getName() const override { return "functionName"; }
```

### 5. Enhanced Hover Information

Hovering over variables now shows:
- Declaration syntax
- Inferred vs explicit types
- Resolved custom types
- Variable scope

### 6. Better Type Inference

The extension can now infer types from:
- Literals (int, float, string, bool)
- Array literals with element type detection
- Object literals
- Function calls (using builtin return types)
- Method calls
- Binary operations
- Comparison operations

### 7. Array Type Inference

```axolotl
const numbers = [1, 2, 3];      // Inferred as [int]
const names = ["a", "b", "c"];  // Inferred as [string]
const mixed = [];               // Inferred as [any]
```

## Usage

1. Install the extension
2. Open any `.axo` file
3. The extension will automatically:
   - Provide autocomplete with type information
   - Show diagnostics for errors and warnings
   - Display hover information with inferred types
   - Track variable usage and scope

## Configuration

No configuration needed - all features work out of the box!

## Building

To rebuild the extension with updated builtins:

```bash
# Unix/Linux/macOS
./build_extensions.sh

# Windows
build_extensions.bat
```

The builtin extraction will automatically include return types and parameter information from the C++ source files.
