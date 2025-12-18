#include "include/compiler.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/interpreter.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <set>
#include <map>
#include <regex>
#include <filesystem>
#include <sys/stat.h>
#ifndef _WIN32
#include <libgen.h>
#endif
#include <algorithm>

std::string escapeForC(const std::string& str) {
    std::stringstream ss;
    for (char c : str) {
        if (c == '\\') ss << "\\\\";
        else if (c == '"') ss << "\\\"";
        else if (c == '\n') ss << "\\n";
        else if (c == '\r') ss << "\\r";
        else if (c == '\t') ss << "\\t";
        else ss << c;
    }
    return ss.str();
}

std::string readFileContent(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string toHexArray(const std::string& data) {
    std::stringstream ss;
    for (size_t i = 0; i < data.size(); ++i) {
        if (i > 0) ss << ",";
        if (i % 16 == 0) ss << "\n    ";
        ss << "0x" << std::hex << std::setfill('0') << std::setw(2) 
           << (int)(unsigned char)data[i];
    }
    return ss.str();
}

std::string getAxolotlRoot() {
    struct stat buffer;
    if (stat("/usr/local/share/axolotl/src", &buffer) == 0) return "/usr/local/share/axolotl";
    if (stat("/usr/local/lib/axolotl/libaxolotl.a", &buffer) == 0) return "/usr/local";
    if (stat("CMakeLists.txt", &buffer) == 0) return ".";
    if (stat("/Users/damian/Axolotl/CMakeLists.txt", &buffer) == 0) return "/Users/damian/Axolotl";
    return "/Users/damian/Axolotl";
}

std::string normalizePath(const std::string& base, const std::string& rel) {
    if (rel[0] != '.') return rel;
    std::string result = base;
    size_t pos = 0;
    while (pos < rel.size() && rel.substr(pos, 2) == "./") pos += 2;
    if (pos < rel.size() && rel.substr(pos, 3) == "../") {
        size_t lastSlash = result.find_last_of('/');
        if (lastSlash != std::string::npos) result = result.substr(0, lastSlash);
        pos += 3;
    }
    return result + "/" + rel.substr(pos);
}

void collectImports(const std::string& filePath, const std::string& baseDir, std::set<std::string>& files, std::map<std::string, std::string>& fileContents) {
    if (files.count(filePath)) return;
    files.insert(filePath);
    
    std::string fullPath = baseDir + "/" + filePath;
    std::string content = readFileContent(fullPath);
    if (content.empty()) return;
    
    fileContents[filePath] = content;
    
    std::string fileDir = filePath.substr(0, filePath.find_last_of('/'));
    
    std::regex importRegex("import\\s+\\\"([^\\\"]+)\\\"");
    std::smatch match;
    std::string::const_iterator searchStart(content.cbegin());
    while (std::regex_search(searchStart, content.cend(), match, importRegex)) {
        std::string importPath = match[1].str();
        std::string normalizedPath = normalizePath(fileDir, importPath);
        collectImports(normalizedPath, baseDir, files, fileContents);
        searchStart = match.suffix().first;
    }
}

void Compiler::compile(Program*, const std::string& outputFile, const std::string& sourceCode) {
    std::cout << "🔨 Building standalone executable...\n";
    
    std::string root = getAxolotlRoot();
    std::filesystem::path outPath(outputFile);
    std::string baseDir = outPath.parent_path().string();
    if (baseDir.empty()) baseDir = ".";
    
    std::set<std::string> importedFiles;
    std::map<std::string, std::string> fileContents;
    
    std::regex importRegex("import\\s+\\\"([^\\\"]+)\\\"");
    std::smatch match;
    std::string::const_iterator searchStart(sourceCode.cbegin());
    while (std::regex_search(searchStart, sourceCode.cend(), match, importRegex)) {
        std::string importPath = match[1].str();
        collectImports(importPath, baseDir.empty() ? "." : baseDir, importedFiles, fileContents);
        searchStart = match.suffix().first;
    }
    
    std::string cppFile = outputFile + "_standalone.cpp";
    std::ofstream cpp(cppFile);
    
    cpp << "// Auto-generated standalone Axolotl executable\n";
    cpp << "#include <iostream>\n#include <string>\n#include <memory>\n#include <map>\n\n";
    
    cpp << "std::map<std::string, std::string> EMBEDDED_FILES = {\n";
    for (const auto& [path, content] : fileContents) {
        cpp << "    {\"" << path << "\", \"" << escapeForC(content) << "\"},\n";
    }
    cpp << "};\n\n";
    
    cpp << "const char* EMBEDDED_SOURCE = \"" << escapeForC(sourceCode) << "\";\n\n";
    
    cpp << "#include \"" << root << "/include/lexer.h\"\n";
    cpp << "#include \"" << root << "/include/parser.h\"\n";
    cpp << "#include \"" << root << "/include/interpreter.h\"\n";
    cpp << "#include <fstream>\n#include <sys/stat.h>\n";
    cpp << "#include <SDL2/SDL.h>\n\n";
    
    cpp << "void writeEmbeddedFiles() {}\n\n";
    cpp << "int main(int argc, char* argv[]) {\n";
    cpp << "    writeEmbeddedFiles();\n";
    cpp << "    try {\n";
    cpp << "        std::string source = EMBEDDED_SOURCE;\n";
    cpp << "        Lexer lexer(source);\n";
    cpp << "        auto tokens = lexer.tokenize();\n";
    cpp << "        Parser parser(tokens);\n";
    cpp << "        auto ast = parser.parse();\n";
    cpp << "        Interpreter interpreter;\n";
    cpp << "        interpreter.interpret(ast.get());\n";
    cpp << "        std::cout.flush();\n";
    cpp << "        if (SDL_WasInit(SDL_INIT_VIDEO)) {\n";
    cpp << "            SDL_Event e;\n";
    cpp << "            bool quit = false;\n";
    cpp << "            while (!quit) {\n";
    cpp << "                while (SDL_PollEvent(&e)) {\n";
    cpp << "                    if (e.type == SDL_QUIT) quit = true;\n";
    cpp << "                }\n";
    cpp << "                SDL_Delay(16);\n";
    cpp << "            }\n";
    cpp << "        }\n";
    cpp << "        return 0;\n";
    cpp << "    } catch (const std::exception& e) {\n";
    cpp << "        std::cerr << \"Error: \" << e.what() << \"\\n\";\n";
    cpp << "        std::cerr.flush();\n";
    cpp << "        return 1;\n";
    cpp << "    }\n";
    cpp << "}\n";
    cpp.close();
    
    if (!importedFiles.empty()) {
        std::cout << "📦 Embedding " << importedFiles.size() << " imported file(s)...\n";
    }
    std::cout << "📦 Embedding interpreter...\n";
    
    std::stringstream compileCmd;
#ifdef _WIN32
    compileCmd << "g++ -std=c++17 -O3 -fexceptions ";
#else
    compileCmd << "c++ -std=c++17 -O3 -fexceptions ";
#endif
    
    // Add include paths
    struct stat buffer;
    if (stat("/usr/local/share/axolotl/include", &buffer) == 0) {
        compileCmd << "-I/usr/local/share/axolotl ";
    } else {
        compileCmd << "-I" << root << " ";
    }
    compileCmd << "-I/opt/homebrew/include ";
    
    compileCmd << cppFile << " ";
    
    // Link prebuilt library or object files with force-load to include all builtins
#ifdef _WIN32
    if (stat((root + "/build/libaxolotl_static.a").c_str(), &buffer) == 0) {
        compileCmd << "-Wl,--whole-archive " << root << "/build/libaxolotl_static.a -Wl,--no-whole-archive ";
    }
#elif __APPLE__
    if (stat("/usr/local/lib/axolotl/libaxolotl.a", &buffer) == 0) {
        compileCmd << "-Wl,-force_load,/usr/local/lib/axolotl/libaxolotl.a ";
    } else if (stat((root + "/build/libaxolotl_static.a").c_str(), &buffer) == 0) {
        compileCmd << "-Wl,-force_load," << root << "/build/libaxolotl_static.a ";
    }
#else
    if (stat("/usr/local/lib/axolotl/libaxolotl.a", &buffer) == 0) {
        compileCmd << "-Wl,--whole-archive /usr/local/lib/axolotl/libaxolotl.a -Wl,--no-whole-archive ";
    } else if (stat((root + "/build/libaxolotl_static.a").c_str(), &buffer) == 0) {
        compileCmd << "-Wl,--whole-archive " << root << "/build/libaxolotl_static.a -Wl,--no-whole-archive ";
    }
#endif
    
    // Add LLVM libraries from config or llvm-config
    std::string llvm_flags;
    std::ifstream llvm_config("/usr/local/lib/axolotl/llvm_flags.txt");
    if (llvm_config) {
        std::string line;
        while (std::getline(llvm_config, line)) {
            llvm_flags += line + " ";
        }
        llvm_config.close();
    } else {
        FILE* llvm_pipe = popen("llvm-config --cxxflags --ldflags --libs core native ExecutionEngine MCJIT RuntimeDyld 2>/dev/null", "r");
        if (llvm_pipe) {
            char buffer[4096];
            while (fgets(buffer, sizeof(buffer), llvm_pipe)) {
                llvm_flags += buffer;
            }
            pclose(llvm_pipe);
        }
    }
    if (!llvm_flags.empty()) {
        llvm_flags.erase(std::remove(llvm_flags.begin(), llvm_flags.end(), '\n'), llvm_flags.end());
        compileCmd << llvm_flags << " -fexceptions ";
    }
    
    // Add platform-specific libraries
#ifdef __APPLE__
    compileCmd << "-framework CoreFoundation -framework OpenGL ";
    compileCmd << "-L/opt/homebrew/lib -Wl,-rpath,/opt/homebrew/lib -lSDL2 -lSDL2_image -lcurl ";
    FILE* gtk_pipe = popen("pkg-config --cflags --libs gtk+-3.0 2>/dev/null", "r");
    if (gtk_pipe) {
        char gtk_buf[2048];
        std::string gtk_flags;
        while (fgets(gtk_buf, sizeof(gtk_buf), gtk_pipe)) gtk_flags += gtk_buf;
        pclose(gtk_pipe);
        if (!gtk_flags.empty()) {
            gtk_flags.erase(std::remove(gtk_flags.begin(), gtk_flags.end(), '\n'), gtk_flags.end());
            compileCmd << gtk_flags << " ";
        }
    }
#elif _WIN32
    compileCmd << "-lSDL2 -lSDL2_image -lcurl -lopengl32 ";
#else
    compileCmd << "-lSDL2 -lSDL2_image -lcurl -lGL ";
    FILE* gtk_pipe = popen("pkg-config --cflags --libs gtk+-3.0 2>/dev/null", "r");
    if (gtk_pipe) {
        char gtk_buf[2048];
        std::string gtk_flags;
        while (fgets(gtk_buf, sizeof(gtk_buf), gtk_pipe)) gtk_flags += gtk_buf;
        pclose(gtk_pipe);
        if (!gtk_flags.empty()) {
            gtk_flags.erase(std::remove(gtk_flags.begin(), gtk_flags.end(), '\n'), gtk_flags.end());
            compileCmd << gtk_flags << " ";
        }
    }
#endif
    
    compileCmd << "-o " << outputFile << " 2>&1";
    
    std::cout << "🔧 Compiling...\n";
    int result = std::system(compileCmd.str().c_str());
    
    if (result == 0) {
        std::cout << "✅ Successfully compiled to: " << outputFile << "\n";
        
        // No runtime files needed - everything is embedded
        
        // Add icon if available in current directory
        std::string iconPath = "icon.png";
        if (stat(iconPath.c_str(), &buffer) == 0) {
            std::cout << "🎨 Adding application icon...\n";
            
#ifdef __APPLE__
            // Create macOS app bundle
            std::filesystem::path outPath(outputFile);
            std::string exeName = outPath.filename().string();
            std::string appName = outputFile + ".app";
            std::string bundleExePath = appName + "/Contents/MacOS/" + exeName;
            
            std::filesystem::create_directories(appName + "/Contents/MacOS");
            std::filesystem::create_directories(appName + "/Contents/Resources");
            std::filesystem::rename(outputFile, bundleExePath);
            std::filesystem::copy_file(iconPath, appName + "/Contents/Resources/icon.png", std::filesystem::copy_options::overwrite_existing);
            
            // Make executable
            std::system(("chmod +x \"" + bundleExePath + "\"").c_str());
            
            // Create Info.plist
            std::ofstream plist(appName + "/Contents/Info.plist");
            plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            plist << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
            plist << "<plist version=\"1.0\">\n<dict>\n";
            plist << "  <key>CFBundleExecutable</key>\n  <string>" << exeName << "</string>\n";
            plist << "  <key>CFBundleIconFile</key>\n  <string>icon.png</string>\n";
            plist << "  <key>CFBundleName</key>\n  <string>" << exeName << "</string>\n";
            plist << "  <key>CFBundleIdentifier</key>\n  <string>com.axolotl." << exeName << "</string>\n";
            plist << "  <key>CFBundlePackageType</key>\n  <string>APPL</string>\n";
            plist << "  <key>NSHighResolutionCapable</key>\n  <true/>\n";
            plist << "</dict>\n</plist>\n";
            plist.close();
            
            std::system(("codesign --force --deep --sign - \"" + appName + "\" 2>/dev/null").c_str());
            std::system(("xattr -cr \"" + appName + "\" 2>/dev/null").c_str());
            
            std::cout << "📱 Created macOS app bundle: " << appName << "\n";
#elif _WIN32
            // Windows: create .ico and embed with resource compiler
            std::cout << "💡 To add icon on Windows, use: rcedit " << outputFile << ".exe --set-icon " << iconPath << "\n";
#else
            // Linux: copy icon alongside executable
            std::filesystem::copy_file(iconPath, outputFile + ".png", std::filesystem::copy_options::overwrite_existing);
            std::cout << "🐧 Icon copied as: " << outputFile << ".png\n";
#endif
        }
        
        // Clean up temporary files
        std::remove(cppFile.c_str());
        
#ifdef __APPLE__
        if (stat(iconPath.c_str(), &buffer) == 0) {
            std::cout << "\n🎉 Done! Open with: open " << outputFile << ".app\n";
        } else {
            std::cout << "\n🎉 Done! Run with: ./" << outputFile << "\n";
        }
#else
        std::cout << "\n🎉 Done! Run with: ./" << outputFile << "\n";
#endif
    } else {
        std::remove(cppFile.c_str());
        std::cerr << "❌ Compilation failed\n";
        std::cerr << "💡 Make sure all dependencies are installed\n";
    }
}
