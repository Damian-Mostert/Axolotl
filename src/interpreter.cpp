#include "include/interpreter.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/operators.h"
#include "include/jit.h"
#include "include/error_handler.h"
#include <variant>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <typeinfo>

namespace fs = std::filesystem;

// Global interpreter pointer for type checking
Interpreter* currentInterpreter = nullptr;

// Helper: trim whitespace
static inline std::string trim(const std::string &s) {
    size_t a = s.find_first_not_of(" \t\n\r");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\n\r");
    return s.substr(a, b - a + 1);
}

// Split a string by '|' into parts (no advanced parsing required here)
static std::vector<std::string> splitUnion(const std::string &s) {
    std::vector<std::string> out;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == '|') {
            out.push_back(trim(s.substr(start, i - start)));
            start = i + 1;
        }
    }
    return out;
}

// Forward declaration
bool valueMatchesType(const Value &v, const std::string &typeSpec);

// Forward declaration for interpreter access
class Interpreter;
extern Interpreter* currentInterpreter;

// Check whether a Value matches a declared type specification. Supports unions ("a|b"), array types ("[T]"), object types, and literal types.
bool valueMatchesType(const Value &v, const std::string &typeSpec) {
    std::string t = trim(typeSpec);
    if (t.empty()) return false;

    // Custom type lookup - resolve custom types first
    if (currentInterpreter && currentInterpreter->typeRegistry.find(t) != currentInterpreter->typeRegistry.end()) {
        std::string customTypeSpec = currentInterpreter->typeRegistry[t];
        return valueMatchesType(v, customTypeSpec);
    }

    // Array type: [inner] or specific array elements
    if (t.size() >= 2 && t.front() == '[' && t.back() == ']') {
        std::string inner = t.substr(1, t.size() - 2);
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(v)) return false;
        auto arr = std::get<std::shared_ptr<ArrayValue>>(v);
        
        // Check if it's a specific array with comma-separated elements
        if (inner.find(',') != std::string::npos) {
            // Split by comma for specific array elements: [type1, type2]
            std::vector<std::string> elementTypes;
            size_t start = 0;
            for (size_t i = 0; i <= inner.size(); ++i) {
                if (i == inner.size() || inner[i] == ',') {
                    elementTypes.push_back(trim(inner.substr(start, i - start)));
                    start = i + 1;
                }
            }
            if (arr->elements.size() != elementTypes.size()) return false;
            for (size_t i = 0; i < arr->elements.size(); ++i) {
                if (!valueMatchesType(arr->elements[i], elementTypes[i])) return false;
            }
            return true;
        } else {
            // Regular array type: all elements must match inner type
            for (auto &elem : arr->elements) {
                if (!valueMatchesType(elem, inner)) return false;
            }
            return true;
        }
    }

    // Object type: {field1:type1,field2:type2}
    if (t.size() >= 2 && t.front() == '{' && t.back() == '}') {
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(v)) return false;
        auto obj = std::get<std::shared_ptr<ObjectValue>>(v);
        
        std::string inner = t.substr(1, t.size() - 2);
        if (inner.empty()) return true; // Empty object type matches any object
        
        // Parse field specifications
        size_t pos = 0;
        while (pos < inner.size()) {
            // Find field name
            size_t colonPos = inner.find(':', pos);
            if (colonPos == std::string::npos) break;
            
            std::string fieldName = trim(inner.substr(pos, colonPos - pos));
            
            // Find field type (until comma or end)
            size_t commaPos = inner.find(',', colonPos + 1);
            if (commaPos == std::string::npos) commaPos = inner.size();
            
            std::string fieldType = trim(inner.substr(colonPos + 1, commaPos - colonPos - 1));
            
            // Check if object has this field and it matches the type
            if (obj->fields.find(fieldName) == obj->fields.end()) return false;
            if (!valueMatchesType(obj->fields[fieldName], fieldType)) return false;
            
            pos = commaPos + 1;
        }
        return true;
    }

    // Union: split and check any (only applies to non-array/object top-level types)
    if (t.find('|') != std::string::npos) {
        auto parts = splitUnion(t);
        for (auto &p : parts) {
            if (valueMatchesType(v, p)) return true;
        }
        return false;
    }

    // String literal type: "specific_string"
    if (t.size() >= 2 && t.front() == '"' && t.back() == '"') {
        if (!std::holds_alternative<std::string>(v)) return false;
        std::string expectedStr = t.substr(1, t.size() - 2);
        return std::get<std::string>(v) == expectedStr;
    }

    // Integer literal type: specific number
    if (t.find_first_not_of("0123456789-") == std::string::npos && !t.empty() && t != "-") {
        if (!std::holds_alternative<int>(v)) return false;
        try {
            int expectedInt = std::stoi(t);
            return std::get<int>(v) == expectedInt;
        } catch (...) {
            // Not a valid integer literal
        }
    }

    // Boolean literal types
    if (t == "true") {
        return std::holds_alternative<bool>(v) && std::get<bool>(v) == true;
    }
    if (t == "false") {
        return std::holds_alternative<bool>(v) && std::get<bool>(v) == false;
    }

    // "any" type matches anything
    if (t == "any") return true;

    // Base types
    if (t == "int") return std::holds_alternative<int>(v);
    if (t == "float") return std::holds_alternative<float>(v);
    if (t == "string") return std::holds_alternative<std::string>(v);
    if (t == "bool") return std::holds_alternative<bool>(v);
    if (t == "object") return std::holds_alternative<std::shared_ptr<ObjectValue>>(v);
    
    // Function types - accept function pointers
    if (t == "func" || t.rfind("(", 0) == 0) {
        return std::holds_alternative<FunctionDeclaration*>(v) || std::holds_alternative<FunctionExpression*>(v);
    }

    // Fallback: if declared type is unknown, be permissive
    return false;
}

// Environment
void Environment::define(const std::string &name, const Variable &var)
{
    if (scopes.empty())
    {
        scopes.push_back({});
    }
    scopes.back()[name] = var;
}

Variable Environment::get(const std::string &name) const
{
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
    {
        auto found = it->find(name);
        if (found != it->end())
        {
            return found->second;
        }
    }
    throw std::runtime_error("Undefined variable: " + name);
}

void Environment::set(const std::string &name, const Value &value)
{
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
    {
        auto found = it->find(name);
        if (found != it->end())
        {
            // Skip type checking for common types to avoid valueMatchesType overhead
            // Only check if type is complex (unions, arrays, or "any")
            const std::string &declared = found->second.type;
            if (!declared.empty() && 
                (declared.find('|') != std::string::npos || 
                 declared.find('[') != std::string::npos ||
                 declared == "any")) {
                if (!valueMatchesType(value, declared)) {
                    throw std::runtime_error("Type error: cannot assign value to variable '" + name + "' of type '" + declared + "'");
                }
            }
            found->second.value = value;
            return;
        }
    }
    throw std::runtime_error("Undefined variable: " + name);
}

bool Environment::has(const std::string &name) const
{
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
    {
        if (it->find(name) != it->end())
        {
            return true;
        }
    }
    return false;
}

void Environment::pushScope()
{
    scopes.push_back({});
}

void Environment::popScope()
{
    if (!scopes.empty())
    {
        scopes.pop_back();
    }
}

// Interpreter
Interpreter::Interpreter()
{
    environment.pushScope();
    jitCompiler = std::make_unique<LLVMJITCompiler>();
    currentInterpreter = this;
}

Interpreter::~Interpreter() {
    // Wait for all running programs to complete before destroying
    std::lock_guard<std::mutex> lock(programsMutex);
    for (auto& [name, future] : runningPrograms) {
        if (future.valid()) {
            future.wait();
        }
    }
    runningPrograms.clear();
}

void Interpreter::interpret(Program *program)
{
    program->accept(this);
}

std::string Interpreter::visit(IntegerLiteral *node)
{
    return std::to_string(node->value);
}

std::string Interpreter::visit(FloatLiteral *node)
{
    std::ostringstream oss;
    oss << node->value;
    return oss.str();
}

std::string Interpreter::visit(StringLiteral *node)
{
    return node->value;
}

std::string Interpreter::visit(BooleanLiteral *node)
{
    return node->value ? "true" : "false";
}

std::string Interpreter::visit(Identifier *node)
{
    Variable var = environment.get(node->name);
    // Store the variable for typeof operations (includes declared type info)
    lastVariable = var;
    lastVariableName = node->name;
    
    // Store array/object values in lastValue for later retrieval
    if (std::holds_alternative<std::shared_ptr<ArrayValue>>(var.value))
    {
        lastValue = var.value;
        return "[array]";
    }
    if (std::holds_alternative<std::shared_ptr<ObjectValue>>(var.value))
    {
        lastValue = var.value;
        return "{object}";
    }
    // Handle function values
    if (std::holds_alternative<FunctionDeclaration*>(var.value))
    {
        lastValue = var.value;
        return "[function]";
    }
    if (std::holds_alternative<FunctionExpression*>(var.value))
    {
        lastValue = var.value;
        return "[function]";
    }
    return valueToString(var.value);
}

std::string Interpreter::visit(BinaryOp *node)
{
    Value left = evaluate(node->left.get());
    Value right = evaluate(node->right.get());
    Value result = performBinaryOp(left, node->op, right);
    return valueToString(result);
}

std::string Interpreter::visit(UnaryOp *node)
{
    Value operand = evaluate(node->operand.get());
    Value result = performUnaryOp(node->op, operand);
    return valueToString(result);
}

std::string Interpreter::visit(FunctionCall *node)
{
    // Determine function name (support callee identifiers)
    std::string callName = node->name;
    if (node->callee) {
        if (auto id = dynamic_cast<Identifier*>(node->callee.get())) {
            callName = id->name;
        }
    }

    // Built-in: write(...)
    if (callName == "write")
    {
        if (node->args.size() != 2)
        {
            throw std::runtime_error("write() expects exactly 2 arguments: write(filepath, content)");
        }

        // Evaluate arguments
        Value fpVal = evaluate(node->args[0].get());
        Value contentVal = evaluate(node->args[1].get());

        std::string filepath = valueToString(fpVal);
        std::string content = valueToString(contentVal);

        // Write to file
        std::ofstream file(filepath, std::ios::out);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open file for writing: " + filepath);
        }

        file << content;
        file.close();

        return ""; // write returns empty string
    }

    // Built-in: read(...)
    if (callName == "read")
    {
        if (node->args.size() != 1)
        {
            throw std::runtime_error("read() expects exactly 1 argument: read(filepath)");
        }

        // Evaluate filepath argument
        Value fpVal = evaluate(node->args[0].get());
        std::string filepath = valueToString(fpVal);

        // Open file
        std::ifstream file(filepath, std::ios::in);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open file for reading: " + filepath);
        }

        // Read file content into string
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        return buffer.str(); // return file content as std::string
    }

    // Built-in: copy(...)
    if (callName == "copy")
    {
        if (node->args.size() != 2)
        {
            throw std::runtime_error("copy() expects exactly 2 arguments: copy(sourcePath, destPath)");
        }

        // Evaluate source and destination arguments
        Value srcVal = evaluate(node->args[0].get());
        Value dstVal = evaluate(node->args[1].get());
        std::string sourcePath = valueToString(srcVal);
        std::string destPath = valueToString(dstVal);

        // Open source file
        std::ifstream srcFile(sourcePath, std::ios::binary);
        if (!srcFile.is_open())
        {
            throw std::runtime_error("Could not open source file for copying: " + sourcePath);
        }

        // Open destination file
        std::ofstream dstFile(destPath, std::ios::binary);
        if (!dstFile.is_open())
        {
            srcFile.close();
            throw std::runtime_error("Could not open destination file for copying: " + destPath);
        }

        // Copy content
        dstFile << srcFile.rdbuf();

        // Close files
        srcFile.close();
        dstFile.close();

        return nullptr; // or some Value representing success
    }

    // Built-in: print(...)
    if (callName == "print")
    {
        bool first = true;
        for (auto &a : node->args)
        {
            Value v = evaluate(a.get());
            if (!first)
                std::cout << " ";
            std::cout << valueToString(v);
            first = false;
        }
        std::cout << std::endl;
        return "";
    }

    // Built-in: len(array or string)
    if (callName == "len")
    {
        if (node->args.size() != 1)
        {
            throw std::runtime_error("len() expects 1 argument");
        }
        Value v = evaluate(node->args[0].get());
        if (std::holds_alternative<std::shared_ptr<ArrayValue>>(v))
        {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(v);
            return std::to_string(arr->elements.size());
        }
        if (std::holds_alternative<std::string>(v))
        {
            auto s = std::get<std::string>(v);
            return std::to_string(s.size());
        }
        throw std::runtime_error("len() requires array or string");
    }

    // Built-in: push(array, value)
    if (callName == "push")
    {
        if (node->args.size() != 2)
        {
            throw std::runtime_error("push() expects 2 arguments");
        }
        // Try to get the array by identifier
        Identifier *arrayId = dynamic_cast<Identifier *>(node->args[0].get());
        if (arrayId)
        {
            Variable arrayVar = environment.get(arrayId->name);
            Value arrVal = arrayVar.value;
            if (std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal))
            {
                auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
                Value val = evaluate(node->args[1].get());
                // Enforce array element typing based on variable's declared type
                if (!arrayVar.type.empty() && arrayVar.type.size() >= 2 && arrayVar.type.front() == '[' && arrayVar.type.back() == ']') {
                    std::string inner = arrayVar.type.substr(1, arrayVar.type.size() - 2);
                    if (!valueMatchesType(val, inner)) {
                        throw std::runtime_error("Type error: cannot push value to array '" + arrayId->name + "' of element type '" + inner + "'");
                    }
                }
                arr->elements.push_back(val);
                return "";
            }
        }
        throw std::runtime_error("push() requires array variable as first argument");
    }

    // Built-in: pop(array)
    if (callName == "pop")
    {
        if (node->args.size() != 1)
        {
            throw std::runtime_error("pop() expects 1 argument");
        }
        // Try to get the array by identifier
        Identifier *arrayId = dynamic_cast<Identifier *>(node->args[0].get());
        if (arrayId)
        {
            Variable arrayVar = environment.get(arrayId->name);
            Value arrVal = arrayVar.value;
            if (std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal))
            {
                auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
                if (!arr->elements.empty())
                {
                    Value last = arr->elements.back();
                    arr->elements.pop_back();
                    return valueToString(last);
                }
                return "";
            }
        }
        throw std::runtime_error("pop() requires array variable");
    }

    // Built-in: substr(string, start, length)
    if (callName == "substr")
    {
        if (node->args.size() != 3)
        {
            throw std::runtime_error("substr() expects 3 arguments");
        }
        Value s = evaluate(node->args[0].get());
        Value start = evaluate(node->args[1].get());
        Value len = evaluate(node->args[2].get());
        if (std::holds_alternative<std::string>(s) &&
            std::holds_alternative<int>(start) &&
            std::holds_alternative<int>(len))
        {
            std::string str = std::get<std::string>(s);
            int st = std::get<int>(start);
            int l = std::get<int>(len);
            if (st < 0 || st >= (int)str.size())
                return "";
            return str.substr(st, l);
        }
        throw std::runtime_error("substr() requires (string, int, int)");
    }

    // Built-in: toUpper(string)
    if (callName == "toUpper")
    {
        if (node->args.size() != 1)
        {
            throw std::runtime_error("toUpper() expects 1 argument");
        }
        Value s = evaluate(node->args[0].get());
        if (std::holds_alternative<std::string>(s))
        {
            std::string str = std::get<std::string>(s);
            for (char &c : str)
            {
                if (c >= 'a' && c <= 'z')
                {
                    c = c - 'a' + 'A';
                }
            }
            return str;
        }
        throw std::runtime_error("toUpper() requires string");
    }

    // Built-in: toLower(string)
    if (callName == "toLower")
    {
        if (node->args.size() != 1)
        {
            throw std::runtime_error("toLower() expects 1 argument");
        }
        Value s = evaluate(node->args[0].get());
        if (std::holds_alternative<std::string>(s))
        {
            std::string str = std::get<std::string>(s);
            for (char &c : str)
            {
                if (c >= 'A' && c <= 'Z')
                {
                    c = c - 'A' + 'a';
                }
            }
            return str;
        }
        throw std::runtime_error("toLower() requires string");
    }

    // Built-in: indexOf(string, substring)
    if (callName == "indexOf")
    {
        if (node->args.size() != 2)
        {
            throw std::runtime_error("indexOf() expects 2 arguments");
        }
        Value s = evaluate(node->args[0].get());
        Value sub = evaluate(node->args[1].get());
        if (std::holds_alternative<std::string>(s) && std::holds_alternative<std::string>(sub))
        {
            std::string str = std::get<std::string>(s);
            std::string substring = std::get<std::string>(sub);
            size_t pos = str.find(substring);
            if (pos != std::string::npos)
            {
                return std::to_string(pos);
            }
            return "-1";
        }
        throw std::runtime_error("indexOf() requires (string, string)");
    }

    // Built-in: contains(string, substring)
    if (callName == "contains")
    {
        if (node->args.size() != 2)
        {
            throw std::runtime_error("contains() expects 2 arguments");
        }
        Value s = evaluate(node->args[0].get());
        Value sub = evaluate(node->args[1].get());
        if (std::holds_alternative<std::string>(s) && std::holds_alternative<std::string>(sub))
        {
            std::string str = std::get<std::string>(s);
            std::string substring = std::get<std::string>(sub);
            bool result = str.find(substring) != std::string::npos;
            lastValue = result;
            return "[bool]";
        }
        throw std::runtime_error("contains() requires (string, string)");
    }

    // Built-in: millis() - returns current time in milliseconds
    if (callName == "millis")
    {
        if (node->args.size() != 0)
        {
            throw std::runtime_error("millis() expects no arguments");
        }
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        lastValue = static_cast<int>(millis);
        return "[int]";
    }

    // Built-in: sleep(milliseconds) - sleep for specified milliseconds
    if (callName == "sleep")
    {
        if (node->args.size() != 1)
        {
            throw std::runtime_error("sleep() expects 1 argument");
        }
        Value ms = evaluate(node->args[0].get());
        if (std::holds_alternative<int>(ms))
        {
            int milliseconds = std::get<int>(ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
            return "";
        }
        throw std::runtime_error("sleep() requires int argument");
    }

    // Built-in: toString(value) - convert value to string
    if (callName == "toString")
    {
        if (node->args.size() != 1)
        {
            throw std::runtime_error("toString() expects 1 argument");
        }
        Value v = evaluate(node->args[0].get());
        return valueToString(v);
    }

    // Check if this is a call to a function variable (via callee)
    if (node->callee)
    {
        // Special handling for Identifier callees - check functions map first
        Identifier* idCallee = dynamic_cast<Identifier*>(node->callee.get());
        if (idCallee)
        {
            // First check if it's a program
            auto progIt = programs.find(idCallee->name);
            if (progIt != programs.end())
            {
                // Handle user-defined programs - run synchronously
                ProgramDeclaration *prog = progIt->second;
                if (prog == nullptr)
                {
                    // Skip unknown built-in programs (like removed counter)
                    return "";
                }
                
                if (node->args.size() != prog->params.size())
                {
                    throw std::runtime_error("Program argument count mismatch");
                }

                // Run program synchronously
                environment.pushScope();

                for (size_t i = 0; i < node->args.size(); ++i)
                {
                    Value argVal = evaluate(node->args[i].get());
                    Variable param(argVal, prog->params[i].second, false);
                    environment.define(prog->params[i].first, param);
                }

                try
                {
                    executeBlock(prog->body.get());
                }
                catch (const ReturnException &e)
                {
                    environment.popScope();
                    if (std::holds_alternative<std::shared_ptr<ArrayValue>>(e.value) ||
                        std::holds_alternative<std::shared_ptr<ObjectValue>>(e.value) ||
                        std::holds_alternative<FunctionDeclaration*>(e.value) ||
                        std::holds_alternative<FunctionExpression*>(e.value))
                    {
                        lastValue = e.value;
                        if (std::holds_alternative<std::shared_ptr<ArrayValue>>(e.value))
                            return "[array]";
                        if (std::holds_alternative<std::shared_ptr<ObjectValue>>(e.value))
                            return "{object}";
                        return "[function]";
                    }
                    return valueToString(e.value);
                }

                environment.popScope();
                return "";
            }
            
            // Then check if it's a named function
            auto funcIt = functions.find(idCallee->name);
            if (funcIt != functions.end())
            {
                FunctionDeclaration *func = funcIt->second;
                if (node->args.size() != func->params.size())
                {
                    throw std::runtime_error("Function argument count mismatch");
                }

                environment.pushScope();

                for (size_t i = 0; i < node->args.size(); ++i)
                {
                    Value argVal = evaluate(node->args[i].get());
                    Variable param(argVal, func->params[i].second, false);
                    environment.define(func->params[i].first, param);
                }

                try
                {
                    executeBlock(func->body.get());
                }
                catch (const ReturnException &e)
                {
                    environment.popScope();
                    if (std::holds_alternative<std::shared_ptr<ArrayValue>>(e.value) ||
                        std::holds_alternative<std::shared_ptr<ObjectValue>>(e.value) ||
                        std::holds_alternative<FunctionDeclaration*>(e.value) ||
                        std::holds_alternative<FunctionExpression*>(e.value))
                    {
                        lastValue = e.value;
                        if (std::holds_alternative<std::shared_ptr<ArrayValue>>(e.value))
                            return "[array]";
                        if (std::holds_alternative<std::shared_ptr<ObjectValue>>(e.value))
                            return "{object}";
                        return "[function]";
                    }
                    return valueToString(e.value);
                }

                environment.popScope();
                return "";
            }
            
            // Otherwise try to get it from environment
            if (environment.has(idCallee->name))
            {
                Value calleeVal = evaluate(node->callee.get());
                
                // Check if it's a FunctionDeclaration*
                if (std::holds_alternative<FunctionDeclaration*>(calleeVal))
                {
                    FunctionDeclaration *func = std::get<FunctionDeclaration*>(calleeVal);
                    if (node->args.size() != func->params.size())
                    {
                        throw std::runtime_error("Function argument count mismatch");
                    }

                    environment.pushScope();

                    for (size_t i = 0; i < node->args.size(); ++i)
                    {
                        Value argVal = evaluate(node->args[i].get());
                        Variable param(argVal, func->params[i].second, false);
                        environment.define(func->params[i].first, param);
                    }

                    try
                    {
                        executeBlock(func->body.get());
                    }
                    catch (const ReturnException &e)
                    {
                        environment.popScope();
                        return valueToString(e.value);
                    }

                    environment.popScope();
                    return "";
                }
                
                // Check if it's a FunctionExpression*
                if (std::holds_alternative<FunctionExpression*>(calleeVal))
                {
                    FunctionExpression *func = std::get<FunctionExpression*>(calleeVal);
                    if (node->args.size() != func->params.size())
                    {
                        throw std::runtime_error("Function argument count mismatch");
                    }

                    environment.pushScope();

                    for (size_t i = 0; i < node->args.size(); ++i)
                    {
                        Value argVal = evaluate(node->args[i].get());
                        Variable param(argVal, func->params[i].second, false);
                        environment.define(func->params[i].first, param);
                    }

                    try
                    {
                        executeBlock(func->body.get());
                    }
                    catch (const ReturnException &e)
                    {
                        environment.popScope();
                        return valueToString(e.value);
                    }

                    environment.popScope();
                    return "";
                }
                
                throw std::runtime_error("Callee must be a function");
            }
            
            throw std::runtime_error("Undefined function: " + idCallee->name);
        }
        
        // For non-identifier callees, evaluate and check the result
        Value calleeVal = evaluate(node->callee.get());
        
        // Check if it's a FunctionDeclaration*
        if (std::holds_alternative<FunctionDeclaration*>(calleeVal))
        {
            FunctionDeclaration *func = std::get<FunctionDeclaration*>(calleeVal);
            if (node->args.size() != func->params.size())
            {
                throw std::runtime_error("Function argument count mismatch");
            }

            environment.pushScope();

            for (size_t i = 0; i < node->args.size(); ++i)
            {
                Value argVal = evaluate(node->args[i].get());
                Variable param(argVal, func->params[i].second, false);
                environment.define(func->params[i].first, param);
            }

            try
            {
                executeBlock(func->body.get());
            }
            catch (const ReturnException &e)
            {
                environment.popScope();
                return valueToString(e.value);
            }

            environment.popScope();
            return "";
        }
        
        // Check if it's a FunctionExpression*
        if (std::holds_alternative<FunctionExpression*>(calleeVal))
        {
            FunctionExpression *func = std::get<FunctionExpression*>(calleeVal);
            if (node->args.size() != func->params.size())
            {
                throw std::runtime_error("Function argument count mismatch");
            }

            environment.pushScope();

            for (size_t i = 0; i < node->args.size(); ++i)
            {
                Value argVal = evaluate(node->args[i].get());
                Variable param(argVal, func->params[i].second, false);
                environment.define(func->params[i].first, param);
            }

            try
            {
                executeBlock(func->body.get());
            }
            catch (const ReturnException &e)
            {
                environment.popScope();
                return valueToString(e.value);
            }

            environment.popScope();
            return "";
        }
        
        throw std::runtime_error("Callee must be a function");
    }

    // This should not be reached since all calls now go through callee path
    throw std::runtime_error("Invalid function call");
}

std::string Interpreter::visit(Block *node)
{
    executeBlock(node);
    return "";
}

std::string Interpreter::visit(VariableDeclaration *node)
{
    Value value;
    if (node->initializer)
    {
        value = evaluate(node->initializer.get());
    }
    else
    {
        // If declared type is "object" and no initializer, create empty object
        if (node->type == "object")
        {
            value = std::make_shared<ObjectValue>();
        }
        else
        {
            value = 0;
        }
    }
    // Enforce declared type if initializer exists
    if (node->initializer) {
        if (!valueMatchesType(value, node->type)) {
            // Diagnostic output: show declared type and initializer element/value types
            try {
                std::cerr << "[type-check] variable '" << node->name << "' declared as '" << node->type << "' but initializer = ";
                if (std::holds_alternative<std::shared_ptr<ArrayValue>>(value)) {
                    auto arr = std::get<std::shared_ptr<ArrayValue>>(value);
                    std::cerr << "[";
                    for (size_t i = 0; i < arr->elements.size(); ++i) {
                        if (i) std::cerr << ", ";
                        const Value &ev = arr->elements[i];
                        if (std::holds_alternative<int>(ev)) std::cerr << "int(" << std::get<int>(ev) << ")";
                        else if (std::holds_alternative<float>(ev)) std::cerr << "float(" << std::get<float>(ev) << ")";
                        else if (std::holds_alternative<std::string>(ev)) std::cerr << "string(\"" << std::get<std::string>(ev) << "\")";
                        else if (std::holds_alternative<bool>(ev)) std::cerr << "bool(" << (std::get<bool>(ev) ? "true" : "false") << ")";
                        else std::cerr << "<complex>";
                    }
                    std::cerr << "]\n";
                } else {
                    if (std::holds_alternative<int>(value)) std::cerr << "int(" << std::get<int>(value) << ")\n";
                    else if (std::holds_alternative<float>(value)) std::cerr << "float(" << std::get<float>(value) << ")\n";
                    else if (std::holds_alternative<std::string>(value)) std::cerr << "string(\"" << std::get<std::string>(value) << "\")\n";
                    else if (std::holds_alternative<bool>(value)) std::cerr << "bool(" << (std::get<bool>(value) ? "true" : "false") << ")\n";
                    else std::cerr << "<complex>\n";
                }
            } catch (...) {}

            throw std::runtime_error("Type error: initializer for '" + node->name + "' does not match declared type '" + node->type + "'");
        }
    }
    environment.define(node->name, Variable(value, node->type, false));
    return "";
}

std::string Interpreter::visit(Assignment *node)
{
    Value value = evaluate(node->value.get());
    environment.set(node->name, value);
    return valueToString(value);
}

std::string Interpreter::visit(FieldAssignment *node)
{
    Value objVal = evaluate(node->object.get());
    Value val = evaluate(node->value.get());

    if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(objVal))
    {
        throw std::runtime_error("Field assignment requires object on left side");
    }

    auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
    obj->fields[node->field] = val;

    return "";
}

std::string Interpreter::visit(IndexAssignment *node)
{
    Value objVal = evaluate(node->object.get());
    Value idx = evaluate(node->index.get());
    Value val = evaluate(node->value.get());

    if (std::holds_alternative<std::shared_ptr<ArrayValue>>(objVal))
    {
        auto arr = std::get<std::shared_ptr<ArrayValue>>(objVal);
        if (!std::holds_alternative<int>(idx))
        {
            throw std::runtime_error("Array index must be integer");
        }
        int i = std::get<int>(idx);
        if (i < 0 || i >= (int)arr->elements.size())
        {
            throw std::runtime_error("Array index out of bounds");
        }
        // If we can, enforce element type from the variable declaration (if object expression is identifier)
        if (auto id = dynamic_cast<Identifier*>(node->object.get())) {
            if (environment.has(id->name)) {
                Variable arrayVar = environment.get(id->name);
                if (!arrayVar.type.empty() && arrayVar.type.size() >= 2 && arrayVar.type.front() == '[' && arrayVar.type.back() == ']') {
                    std::string inner = arrayVar.type.substr(1, arrayVar.type.size() - 2);
                    if (!valueMatchesType(val, inner)) {
                        throw std::runtime_error("Type error: cannot assign element to array '" + id->name + "' of element type '" + inner + "'");
                    }
                }
            }
        }
        arr->elements[i] = val;
        return "";
    }

    if (std::holds_alternative<std::shared_ptr<ObjectValue>>(objVal))
    {
        if (!std::holds_alternative<std::string>(idx))
        {
            throw std::runtime_error("Object index must be string");
        }
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        std::string key = std::get<std::string>(idx);
        obj->fields[key] = val;
        return "";
    }

    throw std::runtime_error("Index assignment requires array or object on left side");
}

std::string Interpreter::visit(IfStatement *node)
{
    Value cond = evaluate(node->condition.get());
    if (isTruthy(cond))
    {
        executeBlock(node->thenBlock.get());
    }
    else if (node->elseBlock)
    {
        executeBlock(node->elseBlock.get());
    }
    return "";
}

std::string Interpreter::visit(WhileStatement *node)
{
    // Try JIT compilation first
    if (jitCompiler && jitCompiler->isCompilable(node)) {
        if (jitCompiler->compileAndExecute(node, this, environment)) {
            return "";
        }
    }
    
    // Fallback to interpreted execution
    while (isTruthy(evaluate(node->condition.get())))
    {
        try {
            executeBlock(node->body.get());
        }
        catch (const BreakException&) {
            break;
        }
        catch (const ContinueException&) {
            continue;
        }
    }
    return "";
}

std::string Interpreter::visit(ForStatement *node)
{
    // Try JIT compilation first
    if (jitCompiler && jitCompiler->isCompilable(node)) {
        environment.pushScope();
        if (node->init) {
            node->init->accept(this);
        }
        if (jitCompiler->compileAndExecute(node, this, environment)) {
            environment.popScope();
            return "";
        }
        environment.popScope();
    }
    
    // Fallback to interpreted execution
    environment.pushScope();

    if (node->init)
    {
        node->init->accept(this);
    }

    while (isTruthy(evaluate(node->condition.get())))
    {
        try {
            executeBlock(node->body.get());
        }
        catch (const BreakException&) {
            break;
        }
        catch (const ContinueException&) {
            // Continue to update expression
        }
        evaluate(node->update.get());
    }

    environment.popScope();
    return "";
}

std::string Interpreter::visit(ReturnStatement *node)
{
    Value value = 0;
    if (node->value)
    {
        value = evaluate(node->value.get());
    }
    throw ReturnException(value);
}

std::string Interpreter::visit(FunctionDeclaration *node)
{
    functions[node->name] = node;
    // Also store in environment so typeof can access it
    environment.define(node->name, Variable(node, "function", false));
    return "";
}

std::string Interpreter::visit(Program *node)
{
    for (auto &decl : node->declarations)
    {
        decl->accept(this);
    }
    return "";
}

std::string Interpreter::visit(ExpressionStatement *node)
{
    // Evaluate the expression and discard the result
    evaluate(node->expression.get());
    return "";
}

std::string Interpreter::visit(ImportDeclaration *node)
{
    try
    {
        fs::path requested(node->path);
        fs::path absPath = fs::absolute(requested);
        std::string absStr = absPath.string();

        if (importedFiles.find(absStr) != importedFiles.end())
        {
            // Already imported, skip
            return "";
        }

        // Mark as imported to prevent cycles
        importedFiles.insert(absStr);

        std::ifstream file(absStr);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open import file: " + absStr);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        // Tokenize
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        // Parse
        Parser parser(tokens);
        auto ast = parser.parse();

        // Interpret using the same interpreter (share environment/functions)
        interpret(ast.get());

        return "";
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("Import error (") + node->path + "): " + e.what());
    }
}

std::string Interpreter::visit(TypeDeclaration *node)
{
    // Store the type definition in the type registry
    typeRegistry[node->name] = node->typeSpec;
    return "";
}

std::string Interpreter::visit(ThrowStatement *node)
{
    Value value = evaluate(node->expression.get());
    throw ThrowException(value);
}

std::string Interpreter::visit(TryStatement *node)
{
    bool finallyExecuted = false;
    
    try {
        executeBlock(node->tryBlock.get());
    }
    catch (const ThrowException &e) {
        if (node->catchBlock) {
            environment.pushScope();
            if (!node->catchVariable.empty()) {
                environment.define(node->catchVariable, Variable(e.value, "any", false));
            }
            try {
                executeBlock(node->catchBlock.get());
            }
            catch (...) {
                environment.popScope();
                if (node->finallyBlock) {
                    executeBlock(node->finallyBlock.get());
                    finallyExecuted = true;
                }
                throw;
            }
            environment.popScope();
        } else {
            if (node->finallyBlock) {
                executeBlock(node->finallyBlock.get());
                finallyExecuted = true;
            }
            throw;
        }
    }
    catch (const std::runtime_error &e) {
        // Normal errors (like undefined variables) should kill the program
        ErrorHandler::showFatalError(e.what());
        std::exit(1);
    }
    catch (...) {
        if (node->finallyBlock && !finallyExecuted) {
            executeBlock(node->finallyBlock.get());
        }
        throw;
    }
    
    if (node->finallyBlock && !finallyExecuted) {
        executeBlock(node->finallyBlock.get());
    }
    
    return "";
}

std::string Interpreter::visit(BreakStatement *node)
{
    throw BreakException();
}

std::string Interpreter::visit(ContinueStatement *node)
{
    throw ContinueException();
}

std::string Interpreter::visit(ProgramDeclaration *node)
{
    programs[node->name] = node;
    return "";
}

std::string Interpreter::visit(AwaitExpression *node)
{
    // Handle await with function call - run the program asynchronously and wait for it
    if (auto funcCall = dynamic_cast<FunctionCall*>(node->expression.get())) {
        std::string programName;
        if (funcCall->callee) {
            if (auto id = dynamic_cast<Identifier*>(funcCall->callee.get())) {
                programName = id->name;
            }
        } else {
            programName = funcCall->name;
        }
        
        // Check if it's a program
        auto progIt = programs.find(programName);
        if (progIt != programs.end()) {
            ProgramDeclaration *prog = progIt->second;
            if (prog == nullptr) {
                return "";
            }
            
            if (funcCall->args.size() != prog->params.size()) {
                throw std::runtime_error("Program argument count mismatch");
            }

            // Evaluate arguments in the main thread first
            std::vector<Value> argValues;
            for (size_t i = 0; i < funcCall->args.size(); ++i) {
                argValues.push_back(evaluate(funcCall->args[i].get()));
            }
            
            // Create a lambda to run the program in background
            auto programRunner = [this, prog, argValues]() {
                Environment localEnv = this->environment; // Copy current environment
                localEnv.pushScope();

                for (size_t i = 0; i < argValues.size(); ++i) {
                    Variable param(argValues[i], prog->params[i].second, false);
                    localEnv.define(prog->params[i].first, param);
                }

                // Execute program body with local environment
                Environment savedEnv = this->environment;
                this->environment = localEnv;
                try {
                    executeBlock(prog->body.get());
                } catch (...) {
                    // Restore environment on exception
                    this->environment = savedEnv;
                    throw;
                }
                this->environment = savedEnv;
            };

            // Start program in background thread and wait for it
            auto future = std::async(std::launch::async, programRunner);
            future.wait();
            return "";
        }
    }
    
    // For non-program expressions, just evaluate normally
    evaluate(node->expression.get());
    return "";
}

Value Interpreter::evaluate(Expression *expr)
{
    std::string result = expr->accept(this);
    // Check if a complex value was stored
    if (!result.empty() && (result == "[array]" || result == "{object}" || result == "[function]" || result == "[bool]" || result == "[int]"))
    {
        return lastValue;
    }
    // Convert result string back to Value if needed
    if (result.empty())
        return 0;
    if (result == "true")
        return true;
    if (result == "false")
        return false;
    try
    {
        if (result.find('.') != std::string::npos)
        {
            return std::stof(result);
        }
        return std::stoi(result);
    }
    catch (...)
    {
        return result;
    }
}

void Interpreter::execute(Statement *stmt)
{
    stmt->accept(this);
}

void Interpreter::executeBlock(Block *block)
{
    environment.pushScope();
    for (auto &stmt : block->statements)
    {
        stmt->accept(this);
    }
    environment.popScope();
}

Value Interpreter::performBinaryOp(const Value &left, BinaryOperator op, const Value &right)
{
    // Fast path: int operations (most common in loops)
    if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
        int l = std::get<int>(left);
        int r = std::get<int>(right);
        switch (op) {
            case BinaryOperator::ADD: return l + r;
            case BinaryOperator::SUBTRACT: return l - r;
            case BinaryOperator::MULTIPLY: return l * r;
            case BinaryOperator::DIVIDE: return l / r;
            case BinaryOperator::MODULO: return l % r;
            case BinaryOperator::LESS: return l < r;
            case BinaryOperator::GREATER: return l > r;
            case BinaryOperator::LESS_EQUAL: return l <= r;
            case BinaryOperator::GREATER_EQUAL: return l >= r;
            case BinaryOperator::EQUAL: return l == r;
            case BinaryOperator::NOT_EQUAL: return l != r;
            default: break;
        }
    }
    
    // Float operations
    if (std::holds_alternative<float>(left) && std::holds_alternative<float>(right)) {
        float l = std::get<float>(left);
        float r = std::get<float>(right);
        switch (op) {
            case BinaryOperator::ADD: return l + r;
            case BinaryOperator::SUBTRACT: return l - r;
            case BinaryOperator::MULTIPLY: return l * r;
            case BinaryOperator::DIVIDE: return l / r;
            case BinaryOperator::LESS: return l < r;
            case BinaryOperator::GREATER: return l > r;
            case BinaryOperator::LESS_EQUAL: return l <= r;
            case BinaryOperator::GREATER_EQUAL: return l >= r;
            case BinaryOperator::EQUAL: return l == r;
            case BinaryOperator::NOT_EQUAL: return l != r;
            default: break;
        }
    }
    
    // String concatenation
    if (op == BinaryOperator::ADD) {
        return valueToString(left) + valueToString(right);
    }
    
    // Logical operations
    switch (op) {
        case BinaryOperator::LOGICAL_AND:
            return isTruthy(left) && isTruthy(right);
        case BinaryOperator::LOGICAL_OR:
            return isTruthy(left) || isTruthy(right);
        case BinaryOperator::ASSIGN:
            return right;
        case BinaryOperator::EQUAL:
            return valueToString(left) == valueToString(right);
        case BinaryOperator::NOT_EQUAL:
            return valueToString(left) != valueToString(right);
        default:
            break;
    }

    throw std::runtime_error("Unknown operator: " + binaryOpToString(op));
}

Value Interpreter::performUnaryOp(UnaryOperator op, const Value &operand)
{
    switch (op) {
        case UnaryOperator::NEGATE:
            if (std::holds_alternative<int>(operand)) {
                return -std::get<int>(operand);
            }
            return -std::get<float>(operand);
        case UnaryOperator::LOGICAL_NOT:
            return !isTruthy(operand);
        case UnaryOperator::TYPEOF: {
            std::string typeStr = getTypeOfValue(operand);
            // Clear variable info after typeof operation
            lastVariableName.clear();
            lastVariable = Variable();
            return typeStr;
        }
    }
    throw std::runtime_error("Unknown unary operator: " + unaryOpToString(op));
}

bool Interpreter::isTruthy(const Value &v)
{
    if (std::holds_alternative<bool>(v))
    {
        return std::get<bool>(v);
    }
    if (std::holds_alternative<int>(v))
    {
        return std::get<int>(v) != 0;
    }
    if (std::holds_alternative<float>(v))
    {
        return std::get<float>(v) != 0.0f;
    }
    if (std::holds_alternative<std::string>(v))
    {
        return !std::get<std::string>(v).empty();
    }
    if (std::holds_alternative<std::shared_ptr<ArrayValue>>(v))
    {
        auto arr = std::get<std::shared_ptr<ArrayValue>>(v);
        return !arr->elements.empty();
    }
    if (std::holds_alternative<std::shared_ptr<ObjectValue>>(v))
    {
        auto obj = std::get<std::shared_ptr<ObjectValue>>(v);
        return !obj->fields.empty();
    }
    return false;
}

std::string Interpreter::valueToString(const Value &v)
{
    if (std::holds_alternative<int>(v))
    {
        return std::to_string(std::get<int>(v));
    }
    if (std::holds_alternative<float>(v))
    {
        std::ostringstream oss;
        oss << std::get<float>(v);
        return oss.str();
    }
    if (std::holds_alternative<std::string>(v))
    {
        return std::get<std::string>(v);
    }
    if (std::holds_alternative<bool>(v))
    {
        return std::get<bool>(v) ? "true" : "false";
    }
    if (std::holds_alternative<FunctionDeclaration*>(v))
    {
        return "[function]";
    }
    if (std::holds_alternative<FunctionExpression*>(v))
    {
        return "[function]";
    }
    if (std::holds_alternative<std::shared_ptr<ArrayValue>>(v))
    {
        auto arr = std::get<std::shared_ptr<ArrayValue>>(v);
        std::string result = "[";
        for (size_t i = 0; i < arr->elements.size(); ++i)
        {
            if (i > 0)
                result += ", ";
            result += valueToString(arr->elements[i]);
        }
        result += "]";
        return result;
    }
    if (std::holds_alternative<std::shared_ptr<ObjectValue>>(v))
    {
        auto obj = std::get<std::shared_ptr<ObjectValue>>(v);
        std::string result = "{";
        bool first = true;
        for (auto &[key, val] : obj->fields)
        {
            if (!first)
                result += ", ";
            result += key + ": " + valueToString(val);
            first = false;
        }
        result += "}";
        return result;
    }
    return "";
}

std::string Interpreter::getTypeOfValue(const Value &v)
{
    // If we have variable info from the last identifier access, use declared type when appropriate
    if (!lastVariableName.empty() && !lastVariable.type.empty()) {
        std::string declaredType = lastVariable.type;
        
        // For custom types, check if they exist in type registry
        if (typeRegistry.find(declaredType) != typeRegistry.end()) {
            return declaredType;  // Return the custom type name
        }
        
        // For built-in types, return the declared type if it matches the runtime type
        if (declaredType == "int" && std::holds_alternative<int>(v)) return "int";
        if (declaredType == "float" && std::holds_alternative<float>(v)) return "float";
        if (declaredType == "string" && std::holds_alternative<std::string>(v)) return "string";
        if (declaredType == "bool" && std::holds_alternative<bool>(v)) return "bool";
        if (declaredType == "function" && (std::holds_alternative<FunctionDeclaration*>(v) || std::holds_alternative<FunctionExpression*>(v))) return "function";
        if (declaredType.size() >= 2 && declaredType.front() == '[' && declaredType.back() == ']' && std::holds_alternative<std::shared_ptr<ArrayValue>>(v)) {
            return declaredType;  // Return array type like [int] or custom array type
        }
        if (declaredType == "object" && std::holds_alternative<std::shared_ptr<ObjectValue>>(v)) return "object";
    }
    
    // Fallback to runtime type detection
    if (std::holds_alternative<int>(v))
    {
        return "int";
    }
    if (std::holds_alternative<float>(v))
    {
        return "float";
    }
    if (std::holds_alternative<std::string>(v))
    {
        return "string";
    }
    if (std::holds_alternative<bool>(v))
    {
        return "bool";
    }
    if (std::holds_alternative<FunctionDeclaration*>(v) || std::holds_alternative<FunctionExpression*>(v))
    {
        return "function";
    }
    if (std::holds_alternative<std::shared_ptr<ArrayValue>>(v))
    {
        return "array";
    }
    if (std::holds_alternative<std::shared_ptr<ObjectValue>>(v))
    {
        return "object";
    }
    return "unknown";
}

std::string Interpreter::visit(ArrayLiteral *node)
{
    auto arr = std::make_shared<ArrayValue>();
    for (auto &elem : node->elements)
    {
        arr->elements.push_back(evaluate(elem.get()));
    }
    lastValue = arr;
    return "[array]";
}

std::string Interpreter::visit(ObjectLiteral *node)
{
    auto obj = std::make_shared<ObjectValue>();
    for (auto &field : node->fields)
    {
        obj->fields[field.first] = evaluate(field.second.get());
    }
    lastValue = obj;
    return "{object}";
}

std::string Interpreter::visit(IndexAccess *node)
{
    Value obj = evaluate(node->object.get());
    Value idx = evaluate(node->index.get());

    // Array indexing
    if (std::holds_alternative<std::shared_ptr<ArrayValue>>(obj))
    {
        auto arr = std::get<std::shared_ptr<ArrayValue>>(obj);
        if (std::holds_alternative<int>(idx))
        {
            int i = std::get<int>(idx);
            if (i >= 0 && i < (int)arr->elements.size())
            {
                Value elem = arr->elements[i];
                // Store complex values for chaining
                if (std::holds_alternative<std::shared_ptr<ArrayValue>>(elem) ||
                    std::holds_alternative<std::shared_ptr<ObjectValue>>(elem))
                {
                    lastValue = elem;
                    if (std::holds_alternative<std::shared_ptr<ArrayValue>>(elem))
                    {
                        return "[array]";
                    }
                    else
                    {
                        return "{object}";
                    }
                }
                return valueToString(elem);
            }
        }
        throw std::runtime_error("Array index out of bounds");
    }

    // Object field access via [] with string key
    if (std::holds_alternative<std::shared_ptr<ObjectValue>>(obj))
    {
        auto obj_val = std::get<std::shared_ptr<ObjectValue>>(obj);
        std::string key = valueToString(idx);
        if (obj_val->fields.find(key) != obj_val->fields.end())
        {
            Value field = obj_val->fields[key];
            // Store complex values for chaining
            if (std::holds_alternative<std::shared_ptr<ArrayValue>>(field) ||
                std::holds_alternative<std::shared_ptr<ObjectValue>>(field))
            {
                lastValue = field;
                if (std::holds_alternative<std::shared_ptr<ArrayValue>>(field))
                {
                    return "[array]";
                }
                else
                {
                    return "{object}";
                }
            }
            return valueToString(field);
        }
        return "";
    }

    // String indexing
    if (std::holds_alternative<std::string>(obj))
    {
        auto str = std::get<std::string>(obj);
        if (std::holds_alternative<int>(idx))
        {
            int i = std::get<int>(idx);
            if (i >= 0 && i < (int)str.size())
            {
                return std::string(1, str[i]);
            }
        }
        throw std::runtime_error("String index out of bounds");
    }

    throw std::runtime_error("Index access requires array, object, or string");
}

std::string Interpreter::visit(FieldAccess *node)
{
    Value obj = evaluate(node->object.get());

    if (std::holds_alternative<std::shared_ptr<ObjectValue>>(obj))
    {
        auto obj_val = std::get<std::shared_ptr<ObjectValue>>(obj);
        if (obj_val->fields.find(node->field) != obj_val->fields.end())
        {
            Value field = obj_val->fields[node->field];
            // Store complex values for chaining
            if (std::holds_alternative<std::shared_ptr<ArrayValue>>(field) ||
                std::holds_alternative<std::shared_ptr<ObjectValue>>(field))
            {
                lastValue = field;
                if (std::holds_alternative<std::shared_ptr<ArrayValue>>(field))
                {
                    return "[array]";
                }
                else
                {
                    return "{object}";
                }
            }
            return valueToString(field);
        }
        return "";
    }

    throw std::runtime_error("Field access requires object");
}

std::string Interpreter::visit(FunctionExpression* node)
{
    // Store the function expression pointer in lastValue so it can be retrieved
    lastValue = node;
    return "[function]";
}
