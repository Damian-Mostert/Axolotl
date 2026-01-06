#include "include/builtins.h"
#include <SDL2/SDL.h>
#include <unordered_map>

static std::unordered_map<std::string, bool> keyStates;
static int mouseX = 0, mouseY = 0;
static bool mouseDown = false;
static bool mouseClicked = false;

class IsKeyDownBuiltin : public BuiltinFunction {
public:
// @desc Check if keyboard key is pressed
    std::string getName() const override { return "isKeyDown"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) throw std::runtime_error("isKeyDown() expects 1 argument: key name");
        std::string key = std::get<std::string>(interp->evaluate(node->args[0].get()));
        interp->lastValue = keyStates[key];
        return "[bool]";
    }
};

class GetMouseXBuiltin : public BuiltinFunction {
public:
// @desc Get mouse X coordinate
    std::string getName() const override { return "getMouseX"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 0) throw std::runtime_error("getMouseX() expects no arguments");
        int x, y;
        SDL_GetMouseState(&x, &y);
        interp->lastValue = x;
        return "[int]";
    }
};

class GetMouseYBuiltin : public BuiltinFunction {
public:
// @desc Get mouse Y coordinate
    std::string getName() const override { return "getMouseY"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 0) throw std::runtime_error("getMouseY() expects no arguments");
        int x, y;
        SDL_GetMouseState(&x, &y);
        interp->lastValue = y;
        return "[int]";
    }
};

class IsMouseDownBuiltin : public BuiltinFunction {
public:
// @desc Check if mouse button is pressed
    std::string getName() const override { return "isMouseDown"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 0) throw std::runtime_error("isMouseDown() expects no arguments");
        interp->lastValue = mouseDown;
        return "[bool]";
    }
};

class WasMouseClickedBuiltin : public BuiltinFunction {
public:
// @desc Check if mouse was clicked this frame
    std::string getName() const override { return "wasMouseClicked"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 0) throw std::runtime_error("wasMouseClicked() expects no arguments");
        bool result = mouseClicked;
        mouseClicked = false;
        interp->lastValue = result;
        return "[bool]";
    }
};

class UpdateInputsBuiltin : public BuiltinFunction {
public:
// @desc Update input state for current frame
    std::string getName() const override { return "updateInputs"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                for (auto& pair : canvases) {
                    if (pair.second->renderer) SDL_DestroyRenderer(pair.second->renderer);
                    if (pair.second->window) SDL_DestroyWindow(pair.second->window);
                }
                SDL_Quit();
                std::exit(0);
            } else if (event.type == SDL_KEYDOWN) {
                keyStates[SDL_GetKeyName(event.key.keysym.sym)] = true;
            } else if (event.type == SDL_KEYUP) {
                keyStates[SDL_GetKeyName(event.key.keysym.sym)] = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                mouseDown = true;
                mouseClicked = true;
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                mouseDown = false;
            }
        }
        return "";
    }
};

class GetKeyStateBuiltin : public BuiltinFunction {
public:
// @desc Get keyboard state object with arrow keys and space
    std::string getName() const override { return "getKeyState"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["left"] = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A] ? 1 : 0;
        obj->fields["right"] = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D] ? 1 : 0;
        obj->fields["up"] = keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W] ? 1 : 0;
        obj->fields["down"] = keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S] ? 1 : 0;
        obj->fields["space"] = keys[SDL_SCANCODE_SPACE] ? 1 : 0;
        interp->lastValue = obj;
        return "{object}";
    }
};

REGISTER_BUILTIN(IsKeyDownBuiltin)
REGISTER_BUILTIN(GetMouseXBuiltin)
REGISTER_BUILTIN(GetMouseYBuiltin)
REGISTER_BUILTIN(IsMouseDownBuiltin)
REGISTER_BUILTIN(WasMouseClickedBuiltin)
REGISTER_BUILTIN(UpdateInputsBuiltin)
REGISTER_BUILTIN(GetKeyStateBuiltin)
