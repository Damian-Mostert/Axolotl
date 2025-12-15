#include "include/builtins.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

class PrintBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "print"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        bool first = true;
        for (auto &a : node->args) {
            Value v = interp->evaluate(a.get());
            if (!first) std::cout << " ";
            std::cout << interp->valueToString(v);
            first = false;
        }
        std::cout << std::endl;
        return "";
    }
};

class WriteBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "write"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) {
            throw std::runtime_error("write() expects 2 arguments: write(filepath, content)");
        }
        Value fpVal = interp->evaluate(node->args[0].get());
        Value contentVal = interp->evaluate(node->args[1].get());
        std::string filepath = interp->valueToString(fpVal);
        std::string content = interp->valueToString(contentVal);
        
        std::ofstream file(filepath, std::ios::out);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file for writing: " + filepath);
        }
        file << content;
        file.close();
        return "";
    }
};

class ReadBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "read"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) {
            throw std::runtime_error("read() expects 1 argument: read(filepath)");
        }
        Value fpVal = interp->evaluate(node->args[0].get());
        std::string filepath = interp->valueToString(fpVal);
        
        std::ifstream file(filepath, std::ios::in);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file for reading: " + filepath);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        return buffer.str();
    }
};

class ReadDirBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "readDir"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) {
            throw std::runtime_error("readDir() expects 1 argument: readDir(dirPath)");
        }
        Value dirVal = interp->evaluate(node->args[0].get());
        std::string dirPath = interp->valueToString(dirVal);
        
        auto result = std::make_shared<ArrayValue>();
        try {
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                result->elements.push_back(entry.path().filename().string());
            }
        } catch (const fs::filesystem_error& e) {
            throw std::runtime_error("Could not read directory: " + dirPath + " - " + e.what());
        }
        
        interp->lastValue = result;
        return "[array]";
    }
};

class CopyBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "copy"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) {
            throw std::runtime_error("copy() expects 2 arguments: copy(sourcePath, destPath)");
        }
        Value srcVal = interp->evaluate(node->args[0].get());
        Value dstVal = interp->evaluate(node->args[1].get());
        std::string sourcePath = interp->valueToString(srcVal);
        std::string destPath = interp->valueToString(dstVal);
        
        std::ifstream srcFile(sourcePath, std::ios::binary);
        if (!srcFile.is_open()) {
            throw std::runtime_error("Could not open source file: " + sourcePath);
        }
        std::ofstream dstFile(destPath, std::ios::binary);
        if (!dstFile.is_open()) {
            srcFile.close();
            throw std::runtime_error("Could not open destination file: " + destPath);
        }
        dstFile << srcFile.rdbuf();
        srcFile.close();
        dstFile.close();
        return "";
    }
};

REGISTER_BUILTIN(PrintBuiltin)
REGISTER_BUILTIN(WriteBuiltin)
REGISTER_BUILTIN(ReadBuiltin)
REGISTER_BUILTIN(ReadDirBuiltin)
REGISTER_BUILTIN(CopyBuiltin)
