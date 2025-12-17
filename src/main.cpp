#include "include/lexer.h"
#include "include/parser.h"
#include "include/interpreter.h"
#include "include/compiler.h"
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

const char* VERSION = "1.0.0";
const char* INSTALL_DIR = "/usr/local/share/axolotl";

void printHelp() {
    std::cout << BOLD_CYAN << "Axolotl Programming Language v" << VERSION << RESET << "\n\n";
    std::cout << BOLD << "USAGE:" << RESET << "\n";
    std::cout << "  axolotl [command] [options]\n\n";
    std::cout << BOLD << "COMMANDS:" << RESET << "\n";
    std::cout << "  <file.axo>           Run an Axolotl program\n";
    std::cout << "  (no args)            Start REPL or run index.axo\n";
    std::cout << "  init [name]          Create new project from template\n";
    std::cout << "  run <file>           Run a program (explicit)\n";
    std::cout << "  check <file>         Check syntax without running\n";
    std::cout << "  compile <file> [out] Compile to standalone executable\n";
    std::cout << "  examples             List available examples\n";
    std::cout << "  examples <name>      Copy example to current directory\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "  -v, --version        Show version information\n\n";
    std::cout << BOLD << "EXAMPLES:" << RESET << "\n";
    std::cout << "  axolotl hello.axo    # Run hello.axo\n";
    std::cout << "  axolotl init myapp   # Create new project\n";
    std::cout << "  axolotl examples     # List examples\n";
    std::cout << "\nDocumentation: " << INSTALL_DIR << "/README.md\n";
}

void printVersion() {
    std::cout << "Axolotl v" << VERSION << "\n";
}

void initProject(const std::string& name) {
    std::string dirName = name;
    if (dirName.empty()) {
        std::cout << "Enter project name: ";
        std::getline(std::cin, dirName);
        if (dirName.empty()) dirName = "my-axolotl-project";
    }
    
    std::string sampleDir = std::string(INSTALL_DIR) + "/sample";
    std::string cmd = "cp -r " + sampleDir + " " + dirName + " 2>/dev/null";
    int result = system(cmd.c_str());
    
    if (result != 0) {
        std::cerr << RED << "✗" << RESET << " Failed to copy sample directory\n";
        return;
    }
    
    std::cout << GREEN << "✓" << RESET << " Created project: " << BOLD << dirName << RESET << "\n";
    std::cout << "\nNext steps:\n";
    std::cout << "  cd " << dirName << "\n";
    std::cout << "  axolotl\n";
}

void listExamples() {
    std::string examplesDir = std::string(INSTALL_DIR) + "/examples";
    std::string cmd = "ls -1 " + examplesDir + " 2>/dev/null | grep .axo";
    std::cout << BOLD << "Available examples:" << RESET << "\n\n";
    system(cmd.c_str());
    std::cout << "\nCopy example: axolotl examples <name>\n";
    std::cout << "Example path: " << examplesDir << "\n";
}

void copyExample(const std::string& name) {
    std::string examplesDir = std::string(INSTALL_DIR) + "/examples";
    std::string srcFile = examplesDir + "/" + name;
    if (name.find(".axo") == std::string::npos) srcFile += ".axo";
    
    std::ifstream src(srcFile);
    if (!src.good()) {
        std::cerr << RED << "✗" << RESET << " Example not found: " << name << "\n";
        std::cerr << "Run 'axolotl examples' to see available examples\n";
        return;
    }
    
    std::string destName = name;
    if (destName.find(".axo") == std::string::npos) destName += ".axo";
    
    std::ofstream dest(destName);
    dest << src.rdbuf();
    src.close();
    dest.close();
    
    std::cout << GREEN << "✓" << RESET << " Copied " << BOLD << destName << RESET << " to current directory\n";
    std::cout << "Run: axolotl " << destName << "\n";
}

int main(int argc, char* argv[]) {
    std::string source;
    try {
        // Handle commands
        if (argc >= 2) {
            std::string arg1 = argv[1];
            
            if (arg1 == "-h" || arg1 == "--help") {
                printHelp();
                return 0;
            }
            if (arg1 == "-v" || arg1 == "--version") {
                printVersion();
                return 0;
            }
            if (arg1 == "init") {
                std::string name = argc >= 3 ? argv[2] : "";
                initProject(name);
                return 0;
            }
            if (arg1 == "examples") {
                if (argc >= 3) {
                    copyExample(argv[2]);
                } else {
                    listExamples();
                }
                return 0;
            }
            if (arg1 == "run" && argc >= 3) {
                argv[1] = argv[2]; // Shift argument
                argc = 2;
            }
            if (arg1 == "check" && argc >= 3) {
                std::string filename = argv[2];
                source = readFile(filename);
                Lexer lexer(source);
                auto tokens = lexer.tokenize();
                Parser parser(tokens);
                parser.parse();
                std::cout << GREEN << "✓" << RESET << " Syntax OK: " << filename << "\n";
                return 0;
            }
            if (arg1 == "compile" && argc >= 3) {
                std::string filename = argv[2];
                std::string outputFile = argc >= 4 ? argv[3] : "a.out";
                source = readFile(filename);
                Lexer lexer(source);
                auto tokens = lexer.tokenize();
                Parser parser(tokens);
                auto ast = parser.parse();
                Compiler compiler;
                compiler.compile(ast.get(), outputFile, source);
                return 0;
            }
        }
        
        if (argc > 2) {
            std::cerr << "Unknown command. Use 'axolotl --help' for usage\n";
            return 1;
        }

        std::string filename;
        if (argc == 2) {
            filename = argv[1];
            source = readFile(filename);
        } else {
            // Auto-run index.axo if it exists
            filename = "index.axo";
            std::ifstream testFile(filename);
            if (testFile.good()) {
                testFile.close();
                source = readFile(filename);
            } else {
                std::cerr << BOLD_RED << "Error: " << RESET << "No file specified and index.axo not found\n";
                std::cerr << "Usage: axolotl <file.axo>\n";
                std::cerr << "   or: create an index.axo file in the current directory\n";
                return 1;
            }
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
            std::string displayFilename = (argc == 2) ? std::string(argv[1]) : "index.axo";

            std::cerr << BOLD_RED << "✖ Fatal Parse Error: " << RESET
                      << BOLD << pe->what() << RESET << "\n";
            std::cerr << BOLD_CYAN << "  → File: " << RESET
                      << CYAN << displayFilename << RESET
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
