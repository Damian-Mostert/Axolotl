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
#include <cmath>
#include <random>

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
        
        // Parse field specifications with proper brace matching
        size_t pos = 0;
        while (pos < inner.size()) {
            // Skip whitespace
            while (pos < inner.size() && (inner[pos] == ' ' || inner[pos] == '\t' || inner[pos] == '\n')) pos++;
            if (pos >= inner.size()) break;
            
            // Find field name
            size_t colonPos = inner.find(':', pos);
            if (colonPos == std::string::npos) break;
            
            std::string fieldName = trim(inner.substr(pos, colonPos - pos));
            
            // Find field type with proper brace matching
            size_t typeStart = colonPos + 1;
            size_t typeEnd = typeStart;
            int braceCount = 0;
            int bracketCount = 0;
            
            while (typeEnd < inner.size()) {
                char c = inner[typeEnd];
                if (c == '{') braceCount++;
                else if (c == '}') braceCount--;
                else if (c == '[') bracketCount++;
                else if (c == ']') bracketCount--;
                else if (c == ',' && braceCount == 0 && bracketCount == 0) {
                    break; // Found end of this field type
                }
                typeEnd++;
            }
            
            std::string fieldType = trim(inner.substr(typeStart, typeEnd - typeStart));
            
            // Check if object has this field and it matches the type
            if (obj->fields.find(fieldName) == obj->fields.end()) return false;
            if (!valueMatchesType(obj->fields[fieldName], fieldType)) return false;
            
            pos = typeEnd + 1; // Skip the comma
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
    std::string value = node->value;
    
    // Handle template string interpolation ${...}
    if (value.find("${") != std::string::npos) {
        std::string result;
        size_t pos = 0;
        
        while (pos < value.size()) {
            size_t start = value.find("${", pos);
            if (start == std::string::npos) {
                result += value.substr(pos);
                break;
            }
            
            result += value.substr(pos, start - pos);
            
            // Find matching closing brace
            size_t end = start + 2;
            int braceCount = 1;
            while (end < value.size() && braceCount > 0) {
                if (value[end] == '{') braceCount++;
                else if (value[end] == '}') braceCount--;
                end++;
            }
            
            if (braceCount != 0) {
                result += value.substr(start);
                break;
            }
            
            std::string expr = value.substr(start + 2, end - start - 3);
            // Parse and evaluate the expression
            try {
                Lexer exprLexer(expr);
                auto exprTokens = exprLexer.tokenize();
                Parser exprParser(exprTokens);
                auto exprAst = exprParser.parseExpression();
                Value exprValue = evaluate(exprAst.get());
                result += valueToString(exprValue);
            } catch (...) {
                result += "${" + expr + "}";
            }
            
            pos = end;
        }
        
        Value stringValue = result;
        lastValue = stringValue;
        return "[string]";
    }
    
    Value stringValue = value;
    lastValue = stringValue;
    return "[string]";
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
    // Handle string values to prevent evaluate() from parsing them
    if (std::holds_alternative<std::string>(var.value))
    {
        lastValue = var.value;
        return "[string]";
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

    // Built-in: readDir(...)
    if (callName == "readDir")
    {
        if (node->args.size() != 1)
        {
            throw std::runtime_error("readDir() expects exactly 1 argument: readDir(dirPath)");
        }

        Value dirVal = evaluate(node->args[0].get());
        std::string dirPath = valueToString(dirVal);

        auto result = std::make_shared<ArrayValue>();
        
        try {
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                result->elements.push_back(entry.path().filename().string());
            }
        } catch (const fs::filesystem_error& e) {
            throw std::runtime_error("Could not read directory: " + dirPath + " - " + e.what());
        }

        lastValue = result;
        return "[array]";
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

        return ""; // Return empty string for void functions
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
        std::string result = valueToString(v);
        lastValue = Value(result);
        return "[string]";
    }

    // Math functions
    if (callName == "sin") {
        if (node->args.size() != 1) throw std::runtime_error("sin() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        lastValue = std::sin(val);
        return "[float]";
    }
    if (callName == "cos") {
        if (node->args.size() != 1) throw std::runtime_error("cos() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        lastValue = std::cos(val);
        return "[float]";
    }
    if (callName == "tan") {
        if (node->args.size() != 1) throw std::runtime_error("tan() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        lastValue = std::tan(val);
        return "[float]";
    }
    if (callName == "sqrt") {
        if (node->args.size() != 1) throw std::runtime_error("sqrt() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        lastValue = std::sqrt(val);
        return "[float]";
    }
    if (callName == "pow") {
        if (node->args.size() != 2) throw std::runtime_error("pow() expects 2 arguments");
        Value base = evaluate(node->args[0].get());
        Value exp = evaluate(node->args[1].get());
        float b = std::holds_alternative<float>(base) ? std::get<float>(base) : static_cast<float>(std::get<int>(base));
        float e = std::holds_alternative<float>(exp) ? std::get<float>(exp) : static_cast<float>(std::get<int>(exp));
        lastValue = std::pow(b, e);
        return "[float]";
    }
    if (callName == "abs") {
        if (node->args.size() != 1) throw std::runtime_error("abs() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        if (std::holds_alternative<int>(v)) {
            lastValue = std::abs(std::get<int>(v));
            return "[int]";
        }
        lastValue = std::fabs(std::get<float>(v));
        return "[float]";
    }
    if (callName == "floor") {
        if (node->args.size() != 1) throw std::runtime_error("floor() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        lastValue = static_cast<int>(std::floor(val));
        return "[int]";
    }
    if (callName == "ceil") {
        if (node->args.size() != 1) throw std::runtime_error("ceil() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        lastValue = static_cast<int>(std::ceil(val));
        return "[int]";
    }
    if (callName == "round") {
        if (node->args.size() != 1) throw std::runtime_error("round() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        lastValue = static_cast<int>(std::round(val));
        return "[int]";
    }
    if (callName == "min") {
        if (node->args.size() != 2) throw std::runtime_error("min() expects 2 arguments");
        Value a = evaluate(node->args[0].get());
        Value b = evaluate(node->args[1].get());
        if (std::holds_alternative<int>(a) && std::holds_alternative<int>(b)) {
            lastValue = std::min(std::get<int>(a), std::get<int>(b));
            return "[int]";
        }
        float fa = std::holds_alternative<float>(a) ? std::get<float>(a) : static_cast<float>(std::get<int>(a));
        float fb = std::holds_alternative<float>(b) ? std::get<float>(b) : static_cast<float>(std::get<int>(b));
        lastValue = std::min(fa, fb);
        return "[float]";
    }
    if (callName == "max") {
        if (node->args.size() != 2) throw std::runtime_error("max() expects 2 arguments");
        Value a = evaluate(node->args[0].get());
        Value b = evaluate(node->args[1].get());
        if (std::holds_alternative<int>(a) && std::holds_alternative<int>(b)) {
            lastValue = std::max(std::get<int>(a), std::get<int>(b));
            return "[int]";
        }
        float fa = std::holds_alternative<float>(a) ? std::get<float>(a) : static_cast<float>(std::get<int>(a));
        float fb = std::holds_alternative<float>(b) ? std::get<float>(b) : static_cast<float>(std::get<int>(b));
        lastValue = std::max(fa, fb);
        return "[float]";
    }
    if (callName == "random") {
        if (node->args.size() != 0) throw std::runtime_error("random() expects no arguments");
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
        float randomValue = dis(gen);
        lastValue = randomValue;
        std::ostringstream oss;
        oss << randomValue;
        return oss.str();
    }

    // Advanced math functions
    if (callName == "log") {
        if (node->args.size() != 1) throw std::runtime_error("log() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        lastValue = std::log(val);
        return "[float]";
    }
    if (callName == "log10") {
        if (node->args.size() != 1) throw std::runtime_error("log10() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        lastValue = std::log10(val);
        return "[float]";
    }
    if (callName == "exp") {
        if (node->args.size() != 1) throw std::runtime_error("exp() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        lastValue = std::exp(val);
        return "[float]";
    }
    if (callName == "asin") {
        if (node->args.size() != 1) throw std::runtime_error("asin() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        lastValue = std::asin(val);
        return "[float]";
    }
    if (callName == "acos") {
        if (node->args.size() != 1) throw std::runtime_error("acos() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        lastValue = std::acos(val);
        return "[float]";
    }
    if (callName == "atan") {
        if (node->args.size() != 1) throw std::runtime_error("atan() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        lastValue = std::atan(val);
        return "[float]";
    }
    if (callName == "atan2") {
        if (node->args.size() != 2) throw std::runtime_error("atan2() expects 2 arguments");
        Value y = evaluate(node->args[0].get());
        Value x = evaluate(node->args[1].get());
        float fy = std::holds_alternative<float>(y) ? std::get<float>(y) : static_cast<float>(std::get<int>(y));
        float fx = std::holds_alternative<float>(x) ? std::get<float>(x) : static_cast<float>(std::get<int>(x));
        lastValue = std::atan2(fy, fx);
        return "[float]";
    }
    if (callName == "clamp") {
        if (node->args.size() != 3) throw std::runtime_error("clamp() expects 3 arguments");
        Value val = evaluate(node->args[0].get());
        Value minVal = evaluate(node->args[1].get());
        Value maxVal = evaluate(node->args[2].get());
        if (std::holds_alternative<int>(val) && std::holds_alternative<int>(minVal) && std::holds_alternative<int>(maxVal)) {
            int v = std::get<int>(val);
            int mn = std::get<int>(minVal);
            int mx = std::get<int>(maxVal);
            lastValue = std::max(mn, std::min(mx, v));
            return "[int]";
        }
        float fv = std::holds_alternative<float>(val) ? std::get<float>(val) : static_cast<float>(std::get<int>(val));
        float fmn = std::holds_alternative<float>(minVal) ? std::get<float>(minVal) : static_cast<float>(std::get<int>(minVal));
        float fmx = std::holds_alternative<float>(maxVal) ? std::get<float>(maxVal) : static_cast<float>(std::get<int>(maxVal));
        lastValue = std::max(fmn, std::min(fmx, fv));
        return "[float]";
    }
    if (callName == "lerp") {
        if (node->args.size() != 3) throw std::runtime_error("lerp() expects 3 arguments");
        Value a = evaluate(node->args[0].get());
        Value b = evaluate(node->args[1].get());
        Value t = evaluate(node->args[2].get());
        float fa = std::holds_alternative<float>(a) ? std::get<float>(a) : static_cast<float>(std::get<int>(a));
        float fb = std::holds_alternative<float>(b) ? std::get<float>(b) : static_cast<float>(std::get<int>(b));
        float ft = std::holds_alternative<float>(t) ? std::get<float>(t) : static_cast<float>(std::get<int>(t));
        lastValue = fa + (fb - fa) * ft;
        return "[float]";
    }

    // Array functions
    if (callName == "slice") {
        if (node->args.size() != 3) throw std::runtime_error("slice() expects 3 arguments");
        Value arrVal = evaluate(node->args[0].get());
        Value startVal = evaluate(node->args[1].get());
        Value endVal = evaluate(node->args[2].get());
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal)) throw std::runtime_error("slice() requires array");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        int start = std::get<int>(startVal);
        int end = std::get<int>(endVal);
        auto result = std::make_shared<ArrayValue>();
        for (int i = start; i < end && i < (int)arr->elements.size(); i++) {
            result->elements.push_back(arr->elements[i]);
        }
        lastValue = result;
        return "[array]";
    }
    if (callName == "reverse") {
        if (node->args.size() != 1) throw std::runtime_error("reverse() expects 1 argument");
        Value arrVal = evaluate(node->args[0].get());
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal)) throw std::runtime_error("reverse() requires array");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        auto result = std::make_shared<ArrayValue>();
        for (auto it = arr->elements.rbegin(); it != arr->elements.rend(); ++it) {
            result->elements.push_back(*it);
        }
        lastValue = result;
        return "[array]";
    }
    if (callName == "join") {
        if (node->args.size() != 2) throw std::runtime_error("join() expects 2 arguments");
        Value arrVal = evaluate(node->args[0].get());
        Value sepVal = evaluate(node->args[1].get());
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal)) throw std::runtime_error("join() requires array");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        std::string sep = std::get<std::string>(sepVal);
        std::string result;
        for (size_t i = 0; i < arr->elements.size(); i++) {
            if (i > 0) result += sep;
            result += valueToString(arr->elements[i]);
        }
        lastValue = result;
        return "[string]";
    }
    if (callName == "sort") {
        if (node->args.size() != 1) throw std::runtime_error("sort() expects 1 argument");
        Identifier *arrayId = dynamic_cast<Identifier *>(node->args[0].get());
        if (!arrayId) throw std::runtime_error("sort() requires array variable");
        Variable arrayVar = environment.get(arrayId->name);
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrayVar.value)) throw std::runtime_error("sort() requires array");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrayVar.value);
        std::sort(arr->elements.begin(), arr->elements.end(), [this](const Value& a, const Value& b) {
            return valueToString(a) < valueToString(b);
        });
        lastValue = arr;
        return "[array]";
    }
    if (callName == "find") {
        if (node->args.size() != 2) throw std::runtime_error("find() expects 2 arguments");
        Value arrVal = evaluate(node->args[0].get());
        Value searchVal = evaluate(node->args[1].get());
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal)) throw std::runtime_error("find() requires array");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        for (size_t i = 0; i < arr->elements.size(); i++) {
            if (valueToString(arr->elements[i]) == valueToString(searchVal)) {
                lastValue = static_cast<int>(i);
                return "[int]";
            }
        }
        lastValue = -1;
        return "[int]";
    }
    if (callName == "includes") {
        if (node->args.size() != 2) throw std::runtime_error("includes() expects 2 arguments");
        Value arrVal = evaluate(node->args[0].get());
        Value searchVal = evaluate(node->args[1].get());
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal)) throw std::runtime_error("includes() requires array");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        for (const auto& elem : arr->elements) {
            if (valueToString(elem) == valueToString(searchVal)) {
                lastValue = true;
                return "[bool]";
            }
        }
        lastValue = false;
        return "[bool]";
    }

    // String functions
    if (callName == "trim") {
        if (node->args.size() != 1) throw std::runtime_error("trim() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        if (!std::holds_alternative<std::string>(v)) throw std::runtime_error("trim() requires string");
        std::string str = std::get<std::string>(v);
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            lastValue = std::string("");
            return "[string]";
        }
        size_t end = str.find_last_not_of(" \t\n\r");
        lastValue = str.substr(start, end - start + 1);
        return "[string]";
    }
    if (callName == "replace") {
        if (node->args.size() != 3) throw std::runtime_error("replace() expects 3 arguments");
        Value strVal = evaluate(node->args[0].get());
        Value searchVal = evaluate(node->args[1].get());
        Value replaceVal = evaluate(node->args[2].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("replace() requires string");
        std::string str = std::get<std::string>(strVal);
        std::string search = std::get<std::string>(searchVal);
        std::string replacement = std::get<std::string>(replaceVal);
        size_t pos = str.find(search);
        if (pos != std::string::npos) {
            str.replace(pos, search.length(), replacement);
        }
        lastValue = str;
        return "[string]";
    }
    if (callName == "split") {
        if (node->args.size() != 2) throw std::runtime_error("split() expects 2 arguments");
        Value strVal = evaluate(node->args[0].get());
        Value delimVal = evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("split() requires string");
        std::string str = std::get<std::string>(strVal);
        std::string delim = std::get<std::string>(delimVal);
        auto result = std::make_shared<ArrayValue>();
        size_t start = 0;
        size_t end = str.find(delim);
        while (end != std::string::npos) {
            result->elements.push_back(str.substr(start, end - start));
            start = end + delim.length();
            end = str.find(delim, start);
        }
        result->elements.push_back(str.substr(start));
        lastValue = result;
        return "[array]";
    }
    if (callName == "startsWith") {
        if (node->args.size() != 2) throw std::runtime_error("startsWith() expects 2 arguments");
        Value strVal = evaluate(node->args[0].get());
        Value prefixVal = evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("startsWith() requires string");
        std::string str = std::get<std::string>(strVal);
        std::string prefix = std::get<std::string>(prefixVal);
        lastValue = str.rfind(prefix, 0) == 0;
        return "[bool]";
    }
    if (callName == "endsWith") {
        if (node->args.size() != 2) throw std::runtime_error("endsWith() expects 2 arguments");
        Value strVal = evaluate(node->args[0].get());
        Value suffixVal = evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("endsWith() requires string");
        std::string str = std::get<std::string>(strVal);
        std::string suffix = std::get<std::string>(suffixVal);
        if (suffix.length() > str.length()) {
            lastValue = false;
        } else {
            lastValue = str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
        }
        return "[bool]";
    }
    if (callName == "repeat") {
        if (node->args.size() != 2) throw std::runtime_error("repeat() expects 2 arguments");
        Value strVal = evaluate(node->args[0].get());
        Value countVal = evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("repeat() requires string");
        std::string str = std::get<std::string>(strVal);
        int count = std::get<int>(countVal);
        std::string result;
        for (int i = 0; i < count; i++) {
            result += str;
        }
        lastValue = result;
        return "[string]";
    }
    if (callName == "charAt") {
        if (node->args.size() != 2) throw std::runtime_error("charAt() expects 2 arguments");
        Value strVal = evaluate(node->args[0].get());
        Value idxVal = evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("charAt() requires string");
        std::string str = std::get<std::string>(strVal);
        int idx = std::get<int>(idxVal);
        if (idx < 0 || idx >= (int)str.length()) {
            lastValue = std::string("");
        } else {
            lastValue = std::string(1, str[idx]);
        }
        return "[string]";
    }
    if (callName == "charCodeAt") {
        if (node->args.size() != 2) throw std::runtime_error("charCodeAt() expects 2 arguments");
        Value strVal = evaluate(node->args[0].get());
        Value idxVal = evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("charCodeAt() requires string");
        std::string str = std::get<std::string>(strVal);
        int idx = std::get<int>(idxVal);
        if (idx < 0 || idx >= (int)str.length()) {
            lastValue = -1;
        } else {
            lastValue = static_cast<int>(str[idx]);
        }
        return "[int]";
    }

    // Type conversion functions
    if (callName == "toInt") {
        if (node->args.size() != 1) throw std::runtime_error("toInt() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        if (std::holds_alternative<int>(v)) {
            lastValue = std::get<int>(v);
        } else if (std::holds_alternative<float>(v)) {
            lastValue = static_cast<int>(std::get<float>(v));
        } else if (std::holds_alternative<bool>(v)) {
            lastValue = std::get<bool>(v) ? 1 : 0;
        } else if (std::holds_alternative<std::string>(v)) {
            try {
                lastValue = std::stoi(std::get<std::string>(v));
            } catch (...) {
                lastValue = 0;
            }
        } else {
            lastValue = 0;
        }
        return "[int]";
    }
    if (callName == "toFloat") {
        if (node->args.size() != 1) throw std::runtime_error("toFloat() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        if (std::holds_alternative<float>(v)) {
            lastValue = std::get<float>(v);
        } else if (std::holds_alternative<int>(v)) {
            lastValue = static_cast<float>(std::get<int>(v));
        } else if (std::holds_alternative<std::string>(v)) {
            try {
                lastValue = std::stof(std::get<std::string>(v));
            } catch (...) {
                lastValue = 0.0f;
            }
        } else {
            lastValue = 0.0f;
        }
        return "[float]";
    }
    if (callName == "toBool") {
        if (node->args.size() != 1) throw std::runtime_error("toBool() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        lastValue = isTruthy(v);
        return "[bool]";
    }

    // Utility functions
    if (callName == "assert") {
        if (node->args.size() != 2) throw std::runtime_error("assert() expects 2 arguments");
        Value condVal = evaluate(node->args[0].get());
        Value msgVal = evaluate(node->args[1].get());
        if (!isTruthy(condVal)) {
            throw std::runtime_error("Assertion failed: " + std::get<std::string>(msgVal));
        }
        return "";
    }
    if (callName == "error") {
        if (node->args.size() != 1) throw std::runtime_error("error() expects 1 argument");
        Value msgVal = evaluate(node->args[0].get());
        throw std::runtime_error(std::get<std::string>(msgVal));
    }
    if (callName == "keys") {
        if (node->args.size() != 1) throw std::runtime_error("keys() expects 1 argument");
        Value objVal = evaluate(node->args[0].get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(objVal)) throw std::runtime_error("keys() requires object");
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        auto result = std::make_shared<ArrayValue>();
        for (const auto& [key, val] : obj->fields) {
            result->elements.push_back(key);
        }
        lastValue = result;
        return "[array]";
    }
    if (callName == "values") {
        if (node->args.size() != 1) throw std::runtime_error("values() expects 1 argument");
        Value objVal = evaluate(node->args[0].get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(objVal)) throw std::runtime_error("values() requires object");
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        auto result = std::make_shared<ArrayValue>();
        for (const auto& [key, val] : obj->fields) {
            result->elements.push_back(val);
        }
        lastValue = result;
        return "[array]";
    }
    if (callName == "hasKey") {
        if (node->args.size() != 2) throw std::runtime_error("hasKey() expects 2 arguments");
        Value objVal = evaluate(node->args[0].get());
        Value keyVal = evaluate(node->args[1].get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(objVal)) throw std::runtime_error("hasKey() requires object");
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        std::string key = std::get<std::string>(keyVal);
        lastValue = obj->fields.find(key) != obj->fields.end();
        return "[bool]";
    }
    if (callName == "clone") {
        if (node->args.size() != 1) throw std::runtime_error("clone() expects 1 argument");
        Value v = evaluate(node->args[0].get());
        // Deep copy for arrays and objects
        if (std::holds_alternative<std::shared_ptr<ArrayValue>>(v)) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(v);
            auto newArr = std::make_shared<ArrayValue>();
            newArr->elements = arr->elements; // Shallow copy elements
            lastValue = newArr;
            return "[array]";
        }
        if (std::holds_alternative<std::shared_ptr<ObjectValue>>(v)) {
            auto obj = std::get<std::shared_ptr<ObjectValue>>(v);
            auto newObj = std::make_shared<ObjectValue>();
            newObj->fields = obj->fields; // Shallow copy fields
            lastValue = newObj;
            return "{object}";
        }
        lastValue = v;
        return valueToString(v);
    }
    if (callName == "merge") {
        if (node->args.size() != 2) throw std::runtime_error("merge() expects 2 arguments");
        Value obj1Val = evaluate(node->args[0].get());
        Value obj2Val = evaluate(node->args[1].get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(obj1Val) || 
            !std::holds_alternative<std::shared_ptr<ObjectValue>>(obj2Val)) {
            throw std::runtime_error("merge() requires two objects");
        }
        auto obj1 = std::get<std::shared_ptr<ObjectValue>>(obj1Val);
        auto obj2 = std::get<std::shared_ptr<ObjectValue>>(obj2Val);
        auto result = std::make_shared<ObjectValue>();
        result->fields = obj1->fields;
        for (const auto& [key, val] : obj2->fields) {
            result->fields[key] = val;
        }
        lastValue = result;
        return "{object}";
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
                    std::cerr << "[debug] Program '" << idCallee->name << "' expects " << prog->params.size() << " arguments but got " << node->args.size() << std::endl;
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
                        std::cerr << "[debug] Environment FunctionDeclaration '" << idCallee->name << "' expects " << func->params.size() << " arguments but got " << node->args.size() << std::endl;
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
                        std::cerr << "[debug] Environment FunctionExpression '" << idCallee->name << "' expects " << func->params.size() << " arguments but got " << node->args.size() << std::endl;
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
        else if (node->type == "string")
        {
            value = std::string("");
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

            std::cerr << "[debug] caught exception type: " << typeid(std::runtime_error).name() << std::endl;
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
    checkPendingWhens(node->name);
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
    checkPendingWhens();
    return "";
}

// Helper function to resolve import paths with Node.js-like behavior
std::string Interpreter::resolveImportPath(const std::string& requestedPath) {
    fs::path requested(requestedPath);
    
    // If path has an extension, use it as-is but validate it
    if (requested.has_extension()) {
        std::string ext = requested.extension().string();
        if (ext != ".axo" && ext != ".json") {
            throw std::runtime_error("Invalid file extension '" + ext + "'. Only .axo and .json files are allowed.");
        }
        
        // For relative paths, resolve relative to the importing file's directory
        fs::path absPath;
        if (requested.is_relative()) {
            // If we have a current module path, resolve relative to its directory
            if (!currentModulePath.empty()) {
                fs::path currentDir = fs::path(currentModulePath).parent_path();
                absPath = currentDir / requested;
            } else {
                absPath = fs::current_path() / requested;
            }
        } else {
            absPath = requested;
        }
        
        if (fs::exists(absPath)) {
            return absPath.string();
        }
        throw std::runtime_error("File not found: " + absPath.string());
    }
    
    // No extension provided - try .axo first (default for Axolotl files)
    fs::path axoPath = requested;
    axoPath += ".axo";
    fs::path absAxoPath = fs::absolute(axoPath);
    
    if (fs::exists(absAxoPath)) {
        return absAxoPath.string();
    }
    
    // If .axo doesn't exist, check if it's a directory with index.axo
    fs::path dirPath = fs::absolute(requested);
    if (fs::is_directory(dirPath)) {
        fs::path indexPath = dirPath / "index.axo";
        if (fs::exists(indexPath)) {
            return indexPath.string();
        }
    }
    
    // File not found
    throw std::runtime_error("Module not found: '" + requestedPath + "'. Tried: " + absAxoPath.string() + ", " + (dirPath / "index.axo").string());
}

std::string Interpreter::visit(ImportDeclaration *node)
{
    try
    {
        // Resolve the import path using Node.js-like resolution
        std::string resolvedPath = resolveImportPath(node->path);
        
        // Check file extension to determine how to process
        fs::path filePath(resolvedPath);
        std::string extension = filePath.extension().string();
        
        if (extension == ".json") {
            // For JSON files, just read content and make it available as a string
            std::ifstream file(resolvedPath);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open import file: " + resolvedPath);
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string jsonContent = buffer.str();
            
            // Store JSON content as a variable with the filename (without extension)
            std::string varName = filePath.stem().string();
            environment.define(varName, Variable(jsonContent, "string", true));
            
            return "";
        }
        else if (extension == ".axo") {
            // Process Axolotl files with export/import support
            bool alreadyImported = importedFiles.find(resolvedPath) != importedFiles.end();
            
            if (!alreadyImported) {
                // Mark as imported to prevent cycles
                importedFiles.insert(resolvedPath);
                
                std::ifstream file(resolvedPath);
                if (!file.is_open()) {
                    throw std::runtime_error("Could not open import file: " + resolvedPath);
                }
                
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string source = buffer.str();

                // Tokenize and parse
                Lexer lexer(source);
                auto tokens = lexer.tokenize();
                Parser parser(tokens);
                auto ast = parser.parse();

                // Set current module path and interpret to populate exports
                std::string savedModulePath = currentModulePath;
                currentModulePath = resolvedPath;
                interpret(ast.get());
                currentModulePath = savedModulePath;
                
                // Store the AST to keep function declarations alive
                importedASTs[resolvedPath] = std::move(ast);
            }
            
            // Handle import bindings
            if (!node->defaultImport.empty()) {
                // Default import: import x from "path";
                std::cerr << "[debug] Processing default import: " << node->defaultImport << std::endl;
                if (moduleDefaultExports.find(resolvedPath) != moduleDefaultExports.end()) {
                    Value defaultValue = moduleDefaultExports[resolvedPath];
                    environment.define(node->defaultImport, Variable(defaultValue, "any", true));
                    // Also register functions in the functions registry for proper function calls
                    if (std::holds_alternative<FunctionDeclaration*>(defaultValue)) {
                        functions[node->defaultImport] = std::get<FunctionDeclaration*>(defaultValue);
                    }
                }
            }
            
            if (!node->namedImports.empty()) {
                // Named imports: import {x, y} from "path";
                std::cerr << "[debug] Processing named imports: ";
                for (const auto& name : node->namedImports) {
                    std::cerr << name << " ";
                }
                std::cerr << std::endl;
                auto moduleExportsIt = moduleExports.find(resolvedPath);
                if (moduleExportsIt != moduleExports.end()) {
                    for (const std::string& importName : node->namedImports) {
                        auto exportIt = moduleExportsIt->second.find(importName);
                        if (exportIt != moduleExportsIt->second.end()) {
                            std::cerr << "[debug] Importing named export: " << importName << std::endl;
                            environment.define(importName, Variable(exportIt->second, "any", true));
                            // Also register functions in the functions registry for proper function calls
                            if (std::holds_alternative<FunctionDeclaration*>(exportIt->second)) {
                                functions[importName] = std::get<FunctionDeclaration*>(exportIt->second);
                            }
                        }
                    }
                }
                // IMPORTANT: Do not import anything else when named imports are specified
                return "";
            }
            
            // If no specific imports are specified, don't import anything
            // The module is loaded and executed, but no exports are made available
            return "";
        }
        else {
            throw std::runtime_error("Unsupported file type: " + extension + ". Only .axo and .json files are supported.");
        }
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("Import error (") + node->path + "): " + e.what());
    }
}

std::string Interpreter::visit(UseDeclaration *node)
{
    try
    {
        // Resolve the use path using Node.js-like resolution
        std::string resolvedPath = resolveImportPath(node->path);
        
        // Check file extension to determine how to process
        fs::path filePath(resolvedPath);
        std::string extension = filePath.extension().string();
        
        if (extension == ".json") {
            // For JSON files, just read content but don't make it available in scope
            // Use statements don't import anything into the current namespace
            std::ifstream file(resolvedPath);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open use file: " + resolvedPath);
            }
            // Just close the file - we're not importing anything
            file.close();
            return "";
        }
        else if (extension == ".axo") {
            // Process Axolotl files but don't import any exports
            bool alreadyImported = importedFiles.find(resolvedPath) != importedFiles.end();
            
            if (!alreadyImported) {
                // Mark as imported to prevent cycles
                importedFiles.insert(resolvedPath);
                
                std::ifstream file(resolvedPath);
                if (!file.is_open()) {
                    throw std::runtime_error("Could not open use file: " + resolvedPath);
                }
                
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string source = buffer.str();

                // Tokenize and parse
                Lexer lexer(source);
                auto tokens = lexer.tokenize();
                Parser parser(tokens);
                auto ast = parser.parse();

                // Set current module path and interpret in isolated environment
                std::string savedModulePath = currentModulePath;
                Environment savedEnvironment = environment;
                
                // Create a new isolated environment for the used file
                environment = Environment();
                environment.pushScope();
                
                currentModulePath = resolvedPath;
                interpret(ast.get());
                currentModulePath = savedModulePath;
                
                // Restore the original environment
                environment = savedEnvironment;
                
                // Store the AST to keep function declarations alive
                importedASTs[resolvedPath] = std::move(ast);
            }
            
            // IMPORTANT: Use statements do NOT import any exports into the current namespace
            // They only ensure the module is loaded and executed
            return "";
        }
        else {
            throw std::runtime_error("Unsupported file type: " + extension + ". Only .axo and .json files are supported.");
        }
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("Use error (") + node->path + "): " + e.what());
    }
}

std::string Interpreter::visit(ExportDeclaration *node)
{
    // Use the current module path being processed
    std::string currentModule = currentModulePath.empty() ? "<main>" : currentModulePath;
    
    if (node->declaration) {
        // Export declaration: export func name() { ... } or export var x = 5;
        node->declaration->accept(this);
        
        // Extract the name from the declaration and add to exports
        if (auto funcDecl = dynamic_cast<FunctionDeclaration*>(node->declaration.get())) {
            // The function should already be registered in functions registry by visit(FunctionDeclaration)
            // Get it from the registry to ensure we have the correct pointer
            auto funcIt = functions.find(funcDecl->name);
            if (funcIt != functions.end()) {
                Value funcValue = funcIt->second;  // Use the registered function pointer
                if (node->isDefault) {
                    moduleDefaultExports[currentModule] = funcValue;
                } else {
                    moduleExports[currentModule][funcDecl->name] = funcValue;
                }
            }
        } else if (auto varDecl = dynamic_cast<VariableDeclaration*>(node->declaration.get())) {
            // Get the variable value from environment
            if (environment.has(varDecl->name)) {
                Variable var = environment.get(varDecl->name);
                std::cerr << "[debug] Exporting variable '" << varDecl->name << "' with value: " << valueToString(var.value) << std::endl;
                if (node->isDefault) {
                    moduleDefaultExports[currentModule] = var.value;
                } else {
                    moduleExports[currentModule][varDecl->name] = var.value;
                }
            }
        }
    } else if (!node->namedExports.empty()) {
        // Named exports: export {x, y};
        for (const std::string& exportName : node->namedExports) {
            if (environment.has(exportName)) {
                Variable var = environment.get(exportName);
                moduleExports[currentModule][exportName] = var.value;
            }
        }
    }
    
    return "";
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
    if (!result.empty() && (result == "[array]" || result == "{object}" || result == "[function]" || result == "[bool]" || result == "[int]" || result == "[string]"))
    {
        return lastValue;
    }
    // Convert result string back to Value if needed
    if (result.empty())
        return std::string("");
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

std::string Interpreter::visit(CaseClause* node)
{
    // Case clauses are handled by SwitchStatement, not directly executed
    return "";
}

std::string Interpreter::visit(SwitchStatement* node)
{
    Value discriminantValue = evaluate(node->discriminant.get());
    bool matched = false;
    bool fallthrough = false;
    
    try {
        for (auto& caseClause : node->cases) {
            if (caseClause->isDefault) {
                // Default case - execute if no previous match or if falling through
                if (!matched || fallthrough) {
                    matched = true;
                    fallthrough = true;
                    for (auto& stmt : caseClause->statements) {
                        stmt->accept(this);
                        checkPendingWhens();
                    }
                }
            } else {
                // Regular case - check if value matches
                Value caseValue = evaluate(caseClause->value.get());
                bool caseMatches = (valueToString(discriminantValue) == valueToString(caseValue));
                
                if (caseMatches || fallthrough) {
                    matched = true;
                    fallthrough = true;
                    for (auto& stmt : caseClause->statements) {
                        stmt->accept(this);
                        checkPendingWhens();
                    }
                }
            }
        }
    }
    catch (const BreakException&) {
        // Break statement exits the switch
        return "";
    }
    
    return "";
}

std::string Interpreter::visit(WhenStatement* node)
{
    PendingWhen pw;
    pw.condition = node->condition.get();
    pw.body = node->body.get();
    pw.dependencies = node->dependencies;
    
    // Store initial values of dependencies
    for (const auto& dep : node->dependencies) {
        if (environment.has(dep)) {
            pw.lastValues[dep] = environment.get(dep).value;
        }
    }
    
    pendingWhens.push_back(std::move(pw));
    return "";
}

void Interpreter::checkPendingWhens(const std::string& changedVar)
{
    auto it = pendingWhens.begin();
    while (it != pendingWhens.end()) {
        bool shouldCheck = false;
        
        if (it->dependencies.empty()) {
            // No dependencies - check every time
            shouldCheck = true;
        } else if (!changedVar.empty()) {
            // Check if changed variable is in dependencies
            for (const auto& dep : it->dependencies) {
                if (dep == changedVar) {
                    shouldCheck = true;
                    break;
                }
            }
        }
        
        if (shouldCheck) {
            try {
                Value condValue = evaluate(it->condition);
                if (isTruthy(condValue)) {
                    executeBlock(it->body);
                    it = pendingWhens.erase(it);
                    continue;
                }
            } catch (...) {
            // If evaluation fails, keep the when statement
        }
        ++it;
    }
}
