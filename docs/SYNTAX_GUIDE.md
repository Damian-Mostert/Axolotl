# Axolotl Language Syntax Guide

Complete reference for the Axolotl programming language syntax.

---

## Table of Contents
1. [Comments](#comments)
2. [Variables](#variables)
3. [Data Types](#data-types)
4. [Operators](#operators)
5. [Control Flow](#control-flow)
6. [Functions](#functions)
7. [Arrays](#arrays)
8. [Objects](#objects)
9. [Type System](#type-system)
10. [Modules](#modules)
11. [Error Handling](#error-handling)
12. [Programs](#programs)
13. [Special Features](#special-features)

---

## Comments

```axo
// Single-line comment
```

**Note:** Multi-line comments are not supported.

---

## Variables

### Declaration

```axo
var name: type = value;
const name: type = value;
```

### Rules
- `var` creates mutable variables
- `const` creates immutable variables
- Type annotation is **required**
- Initialization is **optional** (defaults: `0` for int, `""` for string, empty object for object)
- Semicolons are **optional**

### Examples

```axo
var counter: int = 0
const PI: float = 3.14159
var name: string = "Alice"
var active: bool = true
var data: object = {x: 10, y: 20}
```

---

## Data Types

### Primitive Types

| Type | Description | Example |
|------|-------------|---------|
| `int` | Integer numbers | `42`, `-10`, `0` |
| `float` | Floating-point numbers | `3.14`, `-0.5`, `2.0` |
| `string` | Text strings | `"hello"`, `"world"` |
| `bool` | Boolean values | `true`, `false` |
| `void` | No return value | Used in functions |
| `any` | Any type | Accepts all values |

### Complex Types

| Type | Description | Example |
|------|-------------|---------|
| `object` | Key-value pairs | `{name: "Alice", age: 30}` |
| `[type]` | Array of type | `[int]`, `[string]`, `[[int]]` |
| `func` | Function type | Function references |

### String Escape Sequences

```axo
\n  // Newline
\t  // Tab
\r  // Carriage return
\"  // Double quote
\\  // Backslash
```

---

## Operators

### Arithmetic Operators

```axo
+   // Addition
-   // Subtraction
*   // Multiplication
/   // Division
%   // Modulo
```

### Comparison Operators

```axo
==  // Equal to
!=  // Not equal to
<   // Less than
>   // Greater than
<=  // Less than or equal
>=  // Greater than or equal
```

### Logical Operators

```axo
&&  // Logical AND
||  // Logical OR
!   // Logical NOT
```

### Assignment Operator

```axo
=   // Assignment
```

### Unary Operators

```axo
-expr       // Negation
!expr       // Logical NOT
typeof expr // Type checking
```

### Operator Precedence (highest to lowest)

1. Postfix: `()` (call), `[]` (index), `.` (field)
2. Unary: `!`, `-`, `typeof`, `await`
3. Factor: `*`, `/`, `%`
4. Term: `+`, `-`
5. Comparison: `<`, `>`, `<=`, `>=`
6. Equality: `==`, `!=`
7. Logical AND: `&&`
8. Logical OR: `||`
9. Assignment: `=`

---

## Control Flow

### If Statement

```axo
if (condition) {
    // code
}

if (condition) {
    // code
} else {
    // code
}

if (condition1) {
    // code
} else if (condition2) {
    // code
} else {
    // code
}
```

**Note:** Braces are optional for single statements.

### While Loop

```axo
while (condition) {
    // code
}
```

### For Loop

```axo
for (var i: int = 0; i < 10; i = i + 1) {
    // code
}
```

**Rules:**
- Initializer **must** use `var` keyword or be an expression
- All three parts (init, condition, update) are required
- Semicolons separate the parts

### Break and Continue

```axo
break;      // Exit loop
continue;   // Skip to next iteration
```

### Switch Statement

```axo
switch (expression) {
    case value1:
        // code
        break;
    case value2:
        // code
        break;
    default:
        // code
        break;
}
```

**Features:**
- Supports fallthrough (omit `break`)
- Multiple cases can share code
- `default` case is optional

**Example:**

```axo
switch (grade) {
    case "A":
    case "B":
        print("Good grade!");
        break;
    case "C":
        print("Average");
        break;
    default:
        print("Other");
}
```

---

## Functions

### Function Declaration

```axo
func name(param1: type1, param2: type2) -> returnType {
    // code
    return value;
}
```

### Function with No Parameters

```axo
func greet() -> void {
    print("Hello!");
}
```

### Function Expression (Anonymous Function)

```axo
var multiply: func = func(x: int, y: int) -> int {
    return x * y;
};
```

### Calling Functions

```axo
result = functionName(arg1, arg2);
```

### Recursive Functions

```axo
func factorial(n: int) -> int {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
```

### Function Types in Parameters

```axo
func apply(f: (int) -> int, x: int) -> int {
    return f(x);
}
```

---

## Arrays

### Array Declaration

```axo
var numbers: [int] = [1, 2, 3, 4, 5];
var words: [string] = ["hello", "world"];
var empty: [int] = [];
```

### Nested Arrays

```axo
var matrix: [[int]] = [[1, 2], [3, 4], [5, 6]];
```

### Array Access

```axo
var first: int = numbers[0];
var last: int = numbers[4];
```

### Array Assignment

```axo
numbers[0] = 10;
```

### Array with Union Types

```axo
var mixed: [int|string] = [1, "two", 3, "four"];
```

---

## Objects

### Object Declaration

```axo
var person: object = {
    name: "Alice",
    age: 30,
    active: true
};
```

### Field Access

```axo
var name: string = person.name;
var age: int = person["age"];
```

### Field Assignment

```axo
person.name = "Bob";
person["age"] = 31;
```

### Nested Objects

```axo
var data: object = {
    user: {
        name: "Alice",
        settings: {
            theme: "dark"
        }
    }
};

var theme: string = data.user.settings.theme;
```

---

## Type System

### Custom Type Declarations

```axo
type Name = typeSpec;
```

### Union Types

```axo
type StringOrInt = string|int;
type Result = int|float|string;
```

### Array Types

```axo
type Numbers = [int];
type Matrix = [[float]];
type Mixed = [string|int];
```

### Object Types

```axo
type Person = {
    name: string,
    age: int
};

type User = {
    id: int|string,
    data: {
        email: string,
        verified: bool
    }
};
```

### Literal Types

```axo
type Status = "active"|"inactive"|"pending";
type ErrorCode = 404|500|503;
type Flag = true|false;
```

### Function Types

```axo
type Handler = (int, string) -> void;
type Transformer = (string) -> int;
```

### Complex Type Examples

```axo
// Union of object types
type Entity = {name: string}|{id: int};

// Array of union types
type Data = [string|int|bool];

// Nested structures
type Config = {
    server: {
        host: string,
        port: int
    },
    options: [string]
};
```

### Type Checking

```axo
if (typeof variable == "int") {
    // variable is int
}

if (typeof arr == "[int]") {
    // arr is array of int
}
```

**typeof returns:**
- Primitive types: `"int"`, `"float"`, `"string"`, `"bool"`
- Complex types: `"array"`, `"object"`, `"function"`
- Custom types: Returns the custom type name
- Array types: Returns full type like `"[int]"`

---

## Modules

### Import Statement

```axo
// Simple import (executes file)
import "path/to/file.axo";

// Named imports
import {functionName, variableName} from "path/to/file.axo";

// Default import
import defaultName from "path/to/file.axo";

// Mixed import
import defaultName, {namedExport} from "path/to/file.axo";

// JSON import
import "config.json";

// CSS import
import "styles.css";
```

### Export Statement

```axo
// Export function
export func myFunction() -> void {
    // code
}

// Export variable
export var myVar: int = 42;

// Named exports
export {functionName, variableName};

// Default export
export default func() -> int {
    return 42;
};
```

### Use Statement

```axo
// Execute file without importing exports
use "path/to/file.axo";
```

### Module Resolution

- Relative paths: `"./file.axo"`, `"../lib/utils.axo"`
- Extension optional: `"./file"` resolves to `"./file.axo"`
- Directory imports: `"./folder"` resolves to `"./folder/index.axo"`
- Supported extensions: `.axo`, `.json`, `.css`

---

## Error Handling

### Try-Catch-Finally

```axo
try {
    // code that might throw
} catch (error) {
    // handle error
} finally {
    // always executed
}
```

### Throw Statement

```axo
throw "Error message";
throw 404;
throw errorObject;
```

### Examples

```axo
func riskyOperation(mode: int) -> string {
    if (mode == 1) {
        throw "Operation failed";
    }
    return "Success";
}

try {
    var result: string = riskyOperation(1);
} catch (e) {
    print("Caught error:", e);
} finally {
    print("Cleanup");
}
```

### Nested Try-Catch

```axo
try {
    try {
        throw "Inner error";
    } catch (inner) {
        print("Inner catch:", inner);
        throw "Re-thrown";
    }
} catch (outer) {
    print("Outer catch:", outer);
}
```

---

## Programs

### Program Declaration

```axo
program name(param1: type1, param2: type2) {
    // code
}
```

**Note:** Programs are like functions but designed for async execution.

### Calling Programs

```axo
// Synchronous call
programName(arg1, arg2);

// Asynchronous call with await
await programName(arg1, arg2);
```

---

## Special Features

### typeof Operator

```axo
var x: int = 42;
var type: string = typeof x;  // "int"

if (typeof variable == "string") {
    // type-specific code
}
```

### await Expression

```axo
await programName(args);
await functionCall();
```

### Process Global

```axo
// Environment variables
var envVar: string = process.env.VAR_NAME;

// Command-line arguments
var args: [string] = process.args;

// Current working directory
var cwd: string = process.cwd;
```

### String Concatenation

```axo
var greeting: string = "Hello, " + name + "!";
var mixed: string = "Value: " + toString(42);
```

### Type Coercion

```axo
// Automatic in string concatenation
var result: string = "Number: " + 42;

// Explicit with toString()
var str: string = toString(123);
```

---

## Syntax Rules Summary

### Required Elements
- Type annotations on all variable declarations
- Parameter types in function declarations
- Return type in function declarations (use `void` for no return)
- Parentheses around conditions in `if`, `while`, `for`, `switch`

### Optional Elements
- Semicolons (recommended but not required)
- Braces for single-statement blocks
- Variable initialization (has defaults)

### Naming Conventions
- Variables/functions: camelCase or snake_case
- Types: PascalCase
- Constants: UPPER_CASE or camelCase

### Reserved Keywords

```
int, float, string, bool, void, any, object
if, else, while, for, return, break, continue
func, var, const
import, export, use
program, await
typeof, try, catch, finally, throw
switch, case, default
true, false
```

---

## Complete Example

```axo
// Import modules
import {helper} from "./utils.axo";

// Type definitions
type User = {
    id: int,
    name: string,
    email: string
};

type Result = User|string;

// Constants
const MAX_USERS: int = 100;

// Variables
var users: [User] = [];
var count: int = 0;

// Function with union return type
func findUser(id: int) -> Result {
    for (var i: int = 0; i < len(users); i = i + 1) {
        if (users[i].id == id) {
            return users[i];
        }
    }
    return "User not found";
}

// Function with error handling
func addUser(user: User) -> void {
    try {
        if (count >= MAX_USERS) {
            throw "Maximum users reached";
        }
        push(users, user);
        count = count + 1;
    } catch (e) {
        print("Error:", e);
    }
}

// Main logic
var newUser: User = {
    id: 1,
    name: "Alice",
    email: "alice@example.com"
};

addUser(newUser);

var result: Result = findUser(1);
if (typeof result == "User") {
    print("Found:", result.name);
} else {
    print("Error:", result);
}

// Export for other modules
export {findUser, addUser};
```

---

## Grammar Reference (EBNF-style)

```
Program         → Declaration* EOF

Declaration     → FuncDecl | VarDecl | ProgramDecl | ImportDecl 
                | ExportDecl | UseDecl | TypeDecl | Statement

FuncDecl        → "func" IDENTIFIER "(" Parameters? ")" "->" Type Block
VarDecl         → ("var" | "const") IDENTIFIER ":" Type ("=" Expression)? ";"?
ProgramDecl     → "program" IDENTIFIER "(" Parameters? ")" Block
ImportDecl      → "import" ImportSpec ";"?
ExportDecl      → "export" (Declaration | "{" IDENTIFIER ("," IDENTIFIER)* "}" | "default" Declaration)
UseDecl         → "use" STRING ";"?
TypeDecl        → "type" IDENTIFIER "=" TypeSpec ";"?

Statement       → ExprStmt | IfStmt | WhileStmt | ForStmt | ReturnStmt
                | TryStmt | ThrowStmt | BreakStmt | ContinueStmt 
                | SwitchStmt | Block

Block           → "{" Declaration* "}"
IfStmt          → "if" "(" Expression ")" Statement ("else" Statement)?
WhileStmt       → "while" "(" Expression ")" Statement
ForStmt         → "for" "(" (VarDecl | ExprStmt | ";") Expression ";" Expression ")" Statement
ReturnStmt      → "return" Expression? ";"?
TryStmt         → "try" Block ("catch" "(" IDENTIFIER ")" Block)? ("finally" Block)?
ThrowStmt       → "throw" Expression ";"?
BreakStmt       → "break" ";"?
ContinueStmt    → "continue" ";"?
SwitchStmt      → "switch" "(" Expression ")" "{" CaseClause* "}"
CaseClause      → ("case" Expression | "default") ":" Statement*

Expression      → Assignment
Assignment      → LogicalOr ("=" Assignment)?
LogicalOr       → LogicalAnd ("||" LogicalAnd)*
LogicalAnd      → Equality ("&&" Equality)*
Equality        → Comparison (("==" | "!=") Comparison)*
Comparison      → Term (("<" | ">" | "<=" | ">=") Term)*
Term            → Factor (("+" | "-") Factor)*
Factor          → Unary (("*" | "/" | "%") Unary)*
Unary           → ("!" | "-" | "typeof" | "await") Unary | Postfix
Postfix         → Primary (Call | Index | Field)*
Primary         → INTEGER | FLOAT | STRING | "true" | "false"
                | IDENTIFIER | ArrayLiteral | ObjectLiteral
                | FuncExpr | "(" Expression ")"

Call            → "(" Arguments? ")"
Index           → "[" Expression "]"
Field           → "." IDENTIFIER
ArrayLiteral    → "[" (Expression ("," Expression)*)? "]"
ObjectLiteral   → "{" (IDENTIFIER ":" Expression ("," IDENTIFIER ":" Expression)*)? "}"
FuncExpr        → "func" "(" Parameters? ")" "->" Type Block

Type            → SimpleType | ArrayType | FuncType | UnionType | ObjectType
SimpleType      → "int" | "float" | "string" | "bool" | "void" | "any" | "object" | IDENTIFIER
ArrayType       → "[" Type "]"
FuncType        → "(" (Type ("," Type)*)? ")" "->" Type
UnionType       → Type ("|" Type)+
ObjectType      → "{" (IDENTIFIER ":" Type ("," IDENTIFIER ":" Type)*)? "}"

Parameters      → IDENTIFIER ":" Type ("," IDENTIFIER ":" Type)*
Arguments       → Expression ("," Expression)*
```

---

**End of Syntax Guide**
