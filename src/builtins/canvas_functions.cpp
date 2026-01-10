#include "include/builtins.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <OpenGL/gl.h>
#include <unordered_map>
#include <memory>
#include <iostream>
std::unordered_map<int, std::shared_ptr<CanvasContext>> canvases;
static std::unordered_map<int, SDL_Surface*> surfaces;
static int nextCanvasId = 1;
static int nextSurfaceId = 1;
class CreateCanvasBuiltin : public BuiltinFunction {
public:
    //@desc Create 2D canvas with width and height
    std::string getName() const override { return "createCanvas"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() < 2 || node->args.size() > 3) throw std::runtime_error("createCanvas() expects 2-3 arguments: createCanvas(width, height [, title])");
        int width = std::get<int>(interp->evaluate(node->args[0].get()));
        int height = std::get<int>(interp->evaluate(node->args[1].get()));
        std::string title = "Axolotl Canvas";
        if (node->args.size() == 3) {
            title = std::get<std::string>(interp->evaluate(node->args[2].get()));
        }
        
        static bool sdl_initialized = false;
        if (!sdl_initialized) {
            if (SDL_Init(SDL_INIT_VIDEO) < 0) throw std::runtime_error("SDL init failed");
            if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
                throw std::runtime_error("SDL_image init failed");
            }
            sdl_initialized = true;
        }
        
        SDL_Window* window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
        if (!window) throw std::runtime_error(std::string("Window creation failed: ") + SDL_GetError());
        
        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        SDL_RenderSetLogicalSize(renderer, width, height);
        if (!renderer) {
            SDL_DestroyWindow(window);
            throw std::runtime_error(std::string("Renderer creation failed: ") + SDL_GetError());
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        
        SDL_GLContext glContext = nullptr;
        
        auto ctx = std::make_shared<CanvasContext>();
        ctx->window = window;
        ctx->renderer = renderer;
        ctx->glContext = glContext;
        ctx->width = width;
        ctx->height = height;
        ctx->useOpenGL = false;
        ctx->is2D = true;
        
        int id = nextCanvasId++;
        canvases[id] = ctx;
        
        auto canvas = std::make_shared<ObjectValue>();
        canvas->fields["_id"] = id;
        canvas->fields["width"] = width;
        canvas->fields["height"] = height;
        interp->lastValue = canvas;
        return "{object}";
    }
};
class FillRectBuiltin : public BuiltinFunction {
public:
    //@desc Draw filled rectangle on canvas
    //@parent canvas
    std::string getName() const override { return "fillRect"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee || node->args.size() != 4) throw std::runtime_error("fillRect() expects 4 arguments: fillRect(x, y, width, height)");
        if (auto fa = dynamic_cast<FieldAccess*>(node->callee.get())) {
            Value canvasVal = interp->evaluate(fa->object.get());
            if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(canvasVal)) throw std::runtime_error("fillRect() must be called on canvas");
            auto canvas = std::get<std::shared_ptr<ObjectValue>>(canvasVal);
            int id = std::get<int>(canvas->fields["_id"]);
            auto ctx = canvases[id];
            if (!ctx->renderer) return "";
            int x = std::get<int>(interp->evaluate(node->args[0].get()));
            int y = std::get<int>(interp->evaluate(node->args[1].get()));
            int w = std::get<int>(interp->evaluate(node->args[2].get()));
            int h = std::get<int>(interp->evaluate(node->args[3].get()));
            SDL_SetRenderDrawColor(ctx->renderer, ctx->fillColor.r, ctx->fillColor.g, ctx->fillColor.b, ctx->fillColor.a);
            SDL_Rect rect{x, y, w, h};
            SDL_RenderFillRect(ctx->renderer, &rect);
        }
        return "";
    }
};
class StrokeRectBuiltin : public BuiltinFunction {
public:
    //@desc Draw rectangle outline on canvas
    //@parent canvas
    std::string getName() const override { return "strokeRect"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee || node->args.size() != 4) throw std::runtime_error("strokeRect() expects 4 arguments");
        if (auto fa = dynamic_cast<FieldAccess*>(node->callee.get())) {
            Value canvasVal = interp->evaluate(fa->object.get());
            auto canvas = std::get<std::shared_ptr<ObjectValue>>(canvasVal);
            int id = std::get<int>(canvas->fields["_id"]);
            auto ctx = canvases[id];
            int x = std::get<int>(interp->evaluate(node->args[0].get()));
            int y = std::get<int>(interp->evaluate(node->args[1].get()));
            int w = std::get<int>(interp->evaluate(node->args[2].get()));
            int h = std::get<int>(interp->evaluate(node->args[3].get()));
            SDL_SetRenderDrawColor(ctx->renderer, ctx->strokeColor.r, ctx->strokeColor.g, ctx->strokeColor.b, ctx->strokeColor.a);
            SDL_Rect rect{x, y, w, h};
            SDL_RenderDrawRect(ctx->renderer, &rect);
        }
        return "";
    }
};
class ClearRectBuiltin : public BuiltinFunction {
public:
    //@desc Clear rectangular area on canvas
    //@parent canvas
    std::string getName() const override { return "clearRect"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee || node->args.size() != 4) throw std::runtime_error("clearRect() expects 4 arguments");
        if (auto fa = dynamic_cast<FieldAccess*>(node->callee.get())) {
            Value canvasVal = interp->evaluate(fa->object.get());
            auto canvas = std::get<std::shared_ptr<ObjectValue>>(canvasVal);
            int id = std::get<int>(canvas->fields["_id"]);
            auto ctx = canvases[id];
            int x = std::get<int>(interp->evaluate(node->args[0].get()));
            int y = std::get<int>(interp->evaluate(node->args[1].get()));
            int w = std::get<int>(interp->evaluate(node->args[2].get()));
            int h = std::get<int>(interp->evaluate(node->args[3].get()));
            SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 255);
            SDL_Rect rect{x, y, w, h};
            SDL_RenderFillRect(ctx->renderer, &rect);
        }
        return "";
    }
};
class FillStyleBuiltin : public BuiltinFunction {
public:
    //@desc Set fill color for canvas drawing
    //@parent canvas
    std::string getName() const override { return "fillStyle"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee || node->args.size() != 1) throw std::runtime_error("fillStyle() expects 1 argument: color string");
        if (auto fa = dynamic_cast<FieldAccess*>(node->callee.get())) {
            Value canvasVal = interp->evaluate(fa->object.get());
            auto canvas = std::get<std::shared_ptr<ObjectValue>>(canvasVal);
            int id = std::get<int>(canvas->fields["_id"]);
            auto ctx = canvases[id];
            std::string color = std::get<std::string>(interp->evaluate(node->args[0].get()));
            if (color[0] == '#' && color.length() == 7) {
                ctx->fillColor.r = std::stoi(color.substr(1, 2), nullptr, 16);
                ctx->fillColor.g = std::stoi(color.substr(3, 2), nullptr, 16);
                ctx->fillColor.b = std::stoi(color.substr(5, 2), nullptr, 16);
            }
        }
        return "";
    }
};
class StrokeStyleBuiltin : public BuiltinFunction {
public:
    //@desc Set stroke color for canvas drawing
    //@parent canvas
    std::string getName() const override { return "strokeStyle"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee || node->args.size() != 1) throw std::runtime_error("strokeStyle() expects 1 argument");
        if (auto fa = dynamic_cast<FieldAccess*>(node->callee.get())) {
            Value canvasVal = interp->evaluate(fa->object.get());
            auto canvas = std::get<std::shared_ptr<ObjectValue>>(canvasVal);
            int id = std::get<int>(canvas->fields["_id"]);
            auto ctx = canvases[id];
            std::string color = std::get<std::string>(interp->evaluate(node->args[0].get()));
            if (color[0] == '#' && color.length() == 7) {
                ctx->strokeColor.r = std::stoi(color.substr(1, 2), nullptr, 16);
                ctx->strokeColor.g = std::stoi(color.substr(3, 2), nullptr, 16);
                ctx->strokeColor.b = std::stoi(color.substr(5, 2), nullptr, 16);
            }
        }
        return "";
    }
};
class RenderBuiltin : public BuiltinFunction {
public:
    //@desc Render scene with camera to canvas
    std::string getName() const override { return "render"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        std::cout << "[DEBUG] RenderBuiltin::execute called! args=" << node->args.size() << " callee=" << (node->callee ? "yes" : "no") << std::endl;
        if (!node->callee) throw std::runtime_error("render() must be called on canvas");
        if (node->args.size() >= 2) {
            std::cout << "[DEBUG] 2D render: passing to 3D (args=" << node->args.size() << ")" << std::endl;
            return "";
        }
        if (auto fa = dynamic_cast<FieldAccess*>(node->callee.get())) {
            Value canvasVal = interp->evaluate(fa->object.get());
            auto canvas = std::get<std::shared_ptr<ObjectValue>>(canvasVal);
            int id = std::get<int>(canvas->fields["_id"]);
            auto ctx = canvases[id];
            std::cout << "[DEBUG] 2D render: presenting (renderer=" << (ctx->renderer ? "valid" : "null") << ")" << std::endl;
            if (ctx->renderer) {
                SDL_RenderPresent(ctx->renderer);
                std::cout << "[DEBUG] 2D render: presented" << std::endl;
            }
        }
        return "";
    }
};
class FillCircleBuiltin : public BuiltinFunction {
public:
    //@desc Draw filled circle on canvas
    //@parent canvas
    std::string getName() const override { return "fillCircle"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee || node->args.size() != 3) throw std::runtime_error("fillCircle() expects 3 arguments: fillCircle(x, y, radius)");
        if (auto fa = dynamic_cast<FieldAccess*>(node->callee.get())) {
            Value canvasVal = interp->evaluate(fa->object.get());
            auto canvas = std::get<std::shared_ptr<ObjectValue>>(canvasVal);
            int id = std::get<int>(canvas->fields["_id"]);
            auto ctx = canvases[id];
            int cx = std::get<int>(interp->evaluate(node->args[0].get()));
            int cy = std::get<int>(interp->evaluate(node->args[1].get()));
            int r = std::get<int>(interp->evaluate(node->args[2].get()));
            SDL_SetRenderDrawColor(ctx->renderer, ctx->fillColor.r, ctx->fillColor.g, ctx->fillColor.b, ctx->fillColor.a);
            for (int w = 0; w < r * 2; w++) {
                for (int h = 0; h < r * 2; h++) {
                    int dx = r - w, dy = r - h;
                    if ((dx*dx + dy*dy) <= (r * r)) {
                        SDL_RenderDrawPoint(ctx->renderer, cx + dx, cy + dy);
                    }
                }
            }
        }
        return "";
    }
};
class PollEventsBuiltin : public BuiltinFunction {
public:
    //@desc Process window and input events
    std::string getName() const override { return "pollEvents"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                for (auto& pair : canvases) {
                    SDL_DestroyRenderer(pair.second->renderer);
                    SDL_DestroyWindow(pair.second->window);
                }
                SDL_Quit();
                std::exit(0);
            }
        }
        SDL_Delay(16);
        return "";
    }
};
class LoadImageBuiltin : public BuiltinFunction {
public:
    //@desc Load image from file path
    std::string getName() const override { return "loadImage"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) throw std::runtime_error("loadImage() expects 1 argument: loadImage(path)");
        std::string path = std::get<std::string>(interp->evaluate(node->args[0].get()));
        
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) throw std::runtime_error("Failed to load image: " + path);
        
        int surfaceId = nextSurfaceId++;
        surfaces[surfaceId] = surface;
        
        auto img = std::make_shared<ObjectValue>();
        img->fields["_surfaceId"] = surfaceId;
        img->fields["width"] = surface->w;
        img->fields["height"] = surface->h;
        img->fields["path"] = path;
        interp->lastValue = img;
        return "{object}";
    }
};
class DrawImageBuiltin : public BuiltinFunction {
public:
    //@desc Draw image on canvas at position
    std::string getName() const override { return "drawImage"; }
    //@parent canvas
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee) throw std::runtime_error("drawImage() must be called on canvas");
        if (node->args.size() < 3 || node->args.size() > 5) {
            throw std::runtime_error("drawImage() expects 3-5 arguments: drawImage(image, x, y [, width, height])");
        }
        
        if (auto fa = dynamic_cast<FieldAccess*>(node->callee.get())) {
            Value canvasVal = interp->evaluate(fa->object.get());
            auto canvas = std::get<std::shared_ptr<ObjectValue>>(canvasVal);
            int canvasId = std::get<int>(canvas->fields["_id"]);
            auto ctx = canvases[canvasId];
            
            Value imgVal = interp->evaluate(node->args[0].get());
            auto img = std::get<std::shared_ptr<ObjectValue>>(imgVal);
            int surfaceId = std::get<int>(img->fields["_surfaceId"]);
            SDL_Surface* surface = surfaces[surfaceId];
            
            int x = std::get<int>(interp->evaluate(node->args[1].get()));
            int y = std::get<int>(interp->evaluate(node->args[2].get()));
            
            SDL_Texture* texture = SDL_CreateTextureFromSurface(ctx->renderer, surface);
            if (!texture) throw std::runtime_error("Failed to create texture");
            
            SDL_Rect dest;
            dest.x = x;
            dest.y = y;
            
            if (node->args.size() == 5) {
                dest.w = std::get<int>(interp->evaluate(node->args[3].get()));
                dest.h = std::get<int>(interp->evaluate(node->args[4].get()));
            } else {
                dest.w = surface->w;
                dest.h = surface->h;
            }
            
            SDL_RenderCopy(ctx->renderer, texture, nullptr, &dest);
            SDL_DestroyTexture(texture);
        }
        return "";
    }
};
class DrawLineBuiltin : public BuiltinFunction {
public:
    //@desc Draw line between two points
    std::string getName() const override { return "drawLine"; }
    //@parent canvas
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee || node->args.size() != 4) throw std::runtime_error("drawLine() expects 4 arguments: drawLine(x1, y1, x2, y2)");
        if (auto fa = dynamic_cast<FieldAccess*>(node->callee.get())) {
            Value canvasVal = interp->evaluate(fa->object.get());
            auto canvas = std::get<std::shared_ptr<ObjectValue>>(canvasVal);
            int id = std::get<int>(canvas->fields["_id"]);
            auto ctx = canvases[id];
            int x1 = std::get<int>(interp->evaluate(node->args[0].get()));
            int y1 = std::get<int>(interp->evaluate(node->args[1].get()));
            int x2 = std::get<int>(interp->evaluate(node->args[2].get()));
            int y2 = std::get<int>(interp->evaluate(node->args[3].get()));
            SDL_SetRenderDrawColor(ctx->renderer, ctx->strokeColor.r, ctx->strokeColor.g, ctx->strokeColor.b, ctx->strokeColor.a);
            SDL_RenderDrawLine(ctx->renderer, x1, y1, x2, y2);
        }
        return "";
    }
};
class CloseCanvasBuiltin : public BuiltinFunction {
public:
    //@desc Close canvas window
    std::string getName() const override { return "close"; }
    //@parent canvas
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee) throw std::runtime_error("close() must be called on canvas");
        if (auto fa = dynamic_cast<FieldAccess*>(node->callee.get())) {
            Value canvasVal = interp->evaluate(fa->object.get());
            auto canvas = std::get<std::shared_ptr<ObjectValue>>(canvasVal);
            int id = std::get<int>(canvas->fields["_id"]);
            auto it = canvases.find(id);
            if (it != canvases.end()) {
                if (it->second->renderer) SDL_DestroyRenderer(it->second->renderer);
                if (it->second->window) SDL_DestroyWindow(it->second->window);
                canvases.erase(it);
            }
        }
        return "";
    }
};
REGISTER_BUILTIN(CreateCanvasBuiltin)
REGISTER_BUILTIN(FillRectBuiltin)
REGISTER_BUILTIN(StrokeRectBuiltin)
REGISTER_BUILTIN(ClearRectBuiltin)
REGISTER_BUILTIN(FillStyleBuiltin)
REGISTER_BUILTIN(StrokeStyleBuiltin)
REGISTER_BUILTIN(FillCircleBuiltin)
REGISTER_BUILTIN(DrawLineBuiltin)
REGISTER_BUILTIN(LoadImageBuiltin)
REGISTER_BUILTIN(DrawImageBuiltin)
REGISTER_BUILTIN(PollEventsBuiltin)
REGISTER_BUILTIN(RenderBuiltin)
REGISTER_BUILTIN(CloseCanvasBuiltin)
