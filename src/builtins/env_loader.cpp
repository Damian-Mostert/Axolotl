#include "include/interpreter.h"
#include <fstream>
#include <sstream>

// Load .env file and return as object
std::shared_ptr<ObjectValue> loadEnvFile(const std::string& path = ".env") {
    auto envObj = std::make_shared<ObjectValue>();
    
    std::ifstream file(path);
    if (!file.is_open()) {
        return envObj; // Return empty object if .env doesn't exist
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        // Find = separator
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // Remove quotes if present
        if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') || 
                                   (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }
        
        envObj->fields[key] = value;
    }
    
    return envObj;
}
