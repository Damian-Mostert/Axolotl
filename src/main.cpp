#include "include/lexer.h"
#include "include/parser.h"
#include "include/interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <script.lang>" << std::endl;
    std::cout << "   or: " << programName << " (interactive mode)" << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        if (argc > 2) {
            printUsage(argv[0]);
            return 1;
        }
        
        std::string source;
        
        if (argc == 2) {
            // Read from file
            source = readFile(argv[1]);
        } else {
            // Interactive mode
            std::cout << "Compiler Engine v1.0" << std::endl;
            std::cout << "Type 'exit' to quit" << std::endl;
            std::cout << "> ";
            
            std::string line;
            while (std::getline(std::cin, line)) {
                if (line == "exit") break;
                if (line.empty()) {
                    std::cout << "> ";
                    continue;
                }
                source += line + "\n";
                
                if (line.find(';') != std::string::npos || line.find('}') != std::string::npos) {
                    try {
                        // Tokenize
                        Lexer lexer(source);
                        auto tokens = lexer.tokenize();
                        
                        // Parse
                        Parser parser(tokens);
                        auto ast = parser.parse();
                        
                        // Interpret
                        Interpreter interpreter;
                        interpreter.interpret(ast.get());
                        
                        std::cout << std::endl;
                        source = "";
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                        source = "";
                    }
                }
                std::cout << "> ";
            }
            return 0;
        }
        
        // Tokenize
        //std::cout << "[*] Tokenizing..." << std::endl;
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        //std::cout << "    Found " << tokens.size() << " tokens" << std::endl;
        
        // Parse
        //std::cout << "[*] Parsing..." << std::endl;
        Parser parser(tokens);
        auto ast = parser.parse();
        //std::cout << "    AST created successfully" << std::endl;
        
        // Interpret
        //std::cout << "[*] Executing..." << std::endl;
        Interpreter interpreter;
        interpreter.interpret(ast.get());
        //std::cout << "[*] Done!" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
