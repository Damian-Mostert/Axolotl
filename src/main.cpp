#include "include/lexer.h"
#include "include/parser.h"
#include "include/interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <typeinfo>

// ANSI escape codes
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

#define BOLD_RED    "\033[1;31m"
#define BOLD_YELLOW "\033[1;33m"
#define BOLD_CYAN   "\033[1;36m"

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
    std::string source;
    try {
        if (argc > 2) {
            printUsage(argv[0]);
            return 1;
        }

        if (argc == 2) {
            source = readFile(argv[1]);
        } else {
            std::cout << "Compiler Engine v1.0\nType 'exit' to quit\n> ";
            std::string line;
            while (std::getline(std::cin, line)) {
                if (line == "exit") break;
                if (line.empty()) { std::cout << "> "; continue; }
                source += line + "\n";
                if (line.find(';') != std::string::npos || line.find('}') != std::string::npos) {
                    try {
                        Lexer lexer(source);
                        auto tokens = lexer.tokenize();
                        Parser parser(tokens);
                        auto ast = parser.parse();
                        Interpreter interpreter;
                        interpreter.interpret(ast.get());
                        source = "";
                        std::cout << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                        source = "";
                    }
                }
                std::cout << "> ";
            }
            return 0;
        }

        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto ast = parser.parse();
        Interpreter interpreter;
        interpreter.interpret(ast.get());

        return 0;

    } catch (const std::exception& e) {
        try {
            std::cerr << DIM << "[debug] caught exception type: " << typeid(e).name() << RESET << "\n";
        } catch (...) {}

        if (auto pe = dynamic_cast<const ParseError*>(&e)) {
            std::string filename = (argc == 2) ? std::string(argv[1]) : "<stdin>";

            std::cerr << BOLD_RED << "✖ Fatal Parse Error: " << RESET
                      << BOLD << pe->what() << RESET << "\n";
            std::cerr << BOLD_CYAN << "  → File: " << RESET
                      << CYAN << filename << RESET
                      << ":" << YELLOW << pe->getLine() << RESET
                      << ":" << YELLOW << pe->getColumn() << RESET << "\n\n";

            // ---- CONTEXT BLOCK ----
            std::istringstream ss(source);
            std::vector<std::string> lines;
            std::string temp;
            while (std::getline(ss, temp)) lines.push_back(temp);

            int errLine = pe->getLine();
            int start = std::max(1, errLine - 2); // 2 lines before
            int end = std::min((int)lines.size(), errLine + 2); // 2 lines after

            // Print context lines
            for (int i = start; i <= end; ++i) {
                bool isError = (i == errLine);
                if (isError) std::cerr << BOLD_RED;
                else std::cerr << DIM;

                std::cerr << (i - 1) << " - | " << RESET;
                if (i - 1 < lines.size()) std::cerr << lines[i - 1];
                std::cerr << "\n";

                // Print caret **immediately after the error line only**
                if (isError) {
                    int col = pe->getColumn();
                    if (col < 1) col = 1;
                    std::string pointer(col - 1, ' ');
                    int tokenLen = std::max(1, (int)pe->getTokenValue().size());

                    std::cerr << std::string(6, ' ') // prefix for "NNN - | "
                              << pointer
                              << BOLD_RED << std::string(tokenLen, '^') << RESET
                              << "\n";
                }
            }

            return 1;
        }

        std::cerr << BOLD_RED << "Fatal error: " << RESET << e.what() << "\n";
        return 1;
    }
}
