#pragma once
#include <string>
#include <vector>

class ErrorHandler {
public:
    static void showRuntimeError(const std::string& error, const std::string& context = "");
    static void showFatalError(const std::string& error, const std::string& file = "", int line = 0, int col = 0);
    static void showSuggestion(const std::string& error, const std::vector<std::string>& suggestions);
    
private:
    static std::string colorize(const std::string& text, const std::string& color);
    static std::string formatErrorBox(const std::string& title, const std::string& message);
    static std::vector<std::string> getSuggestions(const std::string& error);
};