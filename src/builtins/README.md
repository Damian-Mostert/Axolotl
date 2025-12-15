# Built-in Functions

This directory contains modular built-in function implementations.

## Adding a New Built-in Function

1. Create a new `.cpp` file in this directory (e.g., `my_functions.cpp`)
2. Include the builtins header: `#include "include/builtins.h"`
3. Define your function class inheriting from `BuiltinFunction`
4. Implement `getName()` and `execute()` methods
5. Register it with `REGISTER_BUILTIN(YourClassName)`
6. Add the file to `CMakeLists.txt`

## Example

```cpp
#include "include/builtins.h"

class MyFunctionBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "myFunction"; }
    
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        // Validate arguments
        if (node->args.size() != 1) {
            throw std::runtime_error("myFunction() expects 1 argument");
        }
        
        // Evaluate arguments
        Value arg = interp->evaluate(node->args[0].get());
        
        // Do your logic
        // ...
        
        // Return result as string
        return "result";
    }
};

REGISTER_BUILTIN(MyFunctionBuiltin)
```

## Available Helper Methods

- `interp->evaluate(expr)` - Evaluate an expression
- `interp->valueToString(value)` - Convert Value to string
- `interp->isTruthy(value)` - Check if value is truthy
- Access `interp->lastValue` for complex return types (arrays, objects)
