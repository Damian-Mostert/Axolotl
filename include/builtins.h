#ifndef BUILTINS_H
#define BUILTINS_H

#include "interpreter.h"
#include "ast.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <SDL2/SDL.h>

struct CanvasContext {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_GLContext glContext;
    int width, height;
    bool useOpenGL = false;
    bool is2D = false;
    SDL_Color fillColor{0, 0, 0, 255};
    SDL_Color strokeColor{0, 0, 0, 255};
    float lineWidth = 1.0f;
};

extern std::unordered_map<int, std::shared_ptr<CanvasContext>> canvases;

// Base class for built-in functions
class BuiltinFunction {
public:
    virtual ~BuiltinFunction() = default;
    virtual std::string getName() const = 0;
    virtual std::string execute(Interpreter* interp, FunctionCall* node) = 0;
};

// Registry for built-in functions
class BuiltinRegistry {
public:
    static BuiltinRegistry& instance();
    void registerBuiltin(BuiltinFunction* func);
    BuiltinFunction* getBuiltin(const std::string& name);
    std::vector<std::string> getAllNames() const;
    
private:
    BuiltinRegistry() = default;
    std::unordered_map<std::string, BuiltinFunction*> builtins;
};

// Macro to register a builtin
#define REGISTER_BUILTIN(ClassName) \
    static ClassName _builtin_##ClassName##_instance; \
    static struct ClassName##Registrar { \
        ClassName##Registrar() { \
            BuiltinRegistry::instance().registerBuiltin(&_builtin_##ClassName##_instance); \
        } \
    } _builtin_##ClassName##_registrar;

#endif // BUILTINS_H
