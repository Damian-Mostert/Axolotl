#include "include/error_handler.h"
#include <iostream>
#include <sstream>
#include <algorithm>

std::string ErrorHandler::colorize(const std::string& text, const std::string& color) {
    if (color == "red") return "\033[1;31m" + text + "\033[0m";
    if (color == "yellow") return "\033[1;33m" + text + "\033[0m";
    if (color == "blue") return "\033[1;34m" + text + "\033[0m";
    if (color == "green") return "\033[1;32m" + text + "\033[0m";
    if (color == "cyan") return "\033[1;36m" + text + "\033[0m";
    return text;
}

std::string ErrorHandler::formatErrorBox(const std::string& title, const std::string& message) {
    std::string box = "╭─ " + colorize(title, "red") + " ─╮\n";
    box += "│ " + message + " │\n";
    box += "╰" + std::string(title.length() + message.length() + 2, '─') + "╯";
    return box;
}

std::vector<std::string> ErrorHandler::getSuggestions(const std::string& error) {
    std::vector<std::string> suggestions;
    
    if (error.find("Undefined variable") != std::string::npos) {
        suggestions.push_back("Check if the variable is declared before use");
        suggestions.push_back("Verify the variable name spelling");
        suggestions.push_back("Make sure the variable is in scope");
    }
    else if (error.find("Array index out of bounds") != std::string::npos) {
        suggestions.push_back("Check array length with len(array)");
        suggestions.push_back("Ensure index is >= 0 and < array length");
        suggestions.push_back("Consider using bounds checking");
    }
    else if (error.find("Type error") != std::string::npos) {
        suggestions.push_back("Check variable type declarations");
        suggestions.push_back("Use explicit type conversion if needed");
        suggestions.push_back("Verify function parameter types");
    }
    else if (error.find("Undefined function") != std::string::npos) {
        suggestions.push_back("Check function name spelling");
        suggestions.push_back("Ensure function is declared before use");
        suggestions.push_back("Verify function is in current scope");
    }
    
    return suggestions;
}

void ErrorHandler::showRuntimeError(const std::string& error, const std::string& context) {
    std::cerr << "\n" << colorize("💥 Runtime Error", "red") << "\n";
    std::cerr << "┌─ " << colorize("Error Details", "yellow") << " ─┐\n";
    std::cerr << "│ " << error << "\n";
    if (!context.empty()) {
        std::cerr << "│ Context: " << colorize(context, "cyan") << "\n";
    }
    std::cerr << "└" << std::string(50, '─') << "┘\n";
    
    auto suggestions = getSuggestions(error);
    if (!suggestions.empty()) {
        std::cerr << "\n" << colorize("💡 Suggestions:", "green") << "\n";
        for (size_t i = 0; i < suggestions.size(); ++i) {
            std::cerr << "  " << (i + 1) << ". " << suggestions[i] << "\n";
        }
    }
    std::cerr << "\n";
}

void ErrorHandler::showFatalError(const std::string& error, const std::string& file, int line, int col) {
    std::cerr << "\n" << colorize("💀 Fatal Error", "red") << "\n";
    std::cerr << "╭─ " << colorize("Critical Failure", "red") << " ─╮\n";
    std::cerr << "│ " << error << "\n";
    
    if (!file.empty() && line > 0) {
        std::cerr << "│ Location: " << colorize(file + ":" + std::to_string(line) + ":" + std::to_string(col), "yellow") << "\n";
    }
    
    std::cerr << "╰" << std::string(50, '─') << "╯\n";
    
    auto suggestions = getSuggestions(error);
    if (!suggestions.empty()) {
        std::cerr << "\n" << colorize("🔧 Quick Fixes:", "blue") << "\n";
        for (const auto& suggestion : suggestions) {
            std::cerr << "  → " << suggestion << "\n";
        }
    }
    std::cerr << "\n" << colorize("Program terminated.", "red") << "\n\n";
}

void ErrorHandler::showSuggestion(const std::string& error, const std::vector<std::string>& suggestions) {
    std::cerr << "\n" << colorize("⚠️  Warning", "yellow") << "\n";
    std::cerr << error << "\n\n";
    
    if (!suggestions.empty()) {
        std::cerr << colorize("Did you mean:", "cyan") << "\n";
        for (const auto& suggestion : suggestions) {
            std::cerr << "  → " << colorize(suggestion, "green") << "\n";
        }
    }
    std::cerr << "\n";
}