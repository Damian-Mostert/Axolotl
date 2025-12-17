#include "include/builtins.h"
#include "include/interpreter.h"
#include "include/ui_ast.h"
#include <gtk/gtk.h>
#include <map>

static bool gtkInitialized = false;
static std::map<int, GtkWidget*> windows;
static int nextWindowId = 1;

static void ensureGtkInit() {
    if (!gtkInitialized) {
        gtk_init(nullptr, nullptr);
        gtkInitialized = true;
    }
}

static GtkWidget* buildWidget(Expression* expr, Interpreter* interp) {
    if (auto elem = dynamic_cast<UIElement*>(expr)) {
        GtkWidget* widget = nullptr;
        
        if (elem->tagName == "div") {
            widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        } else if (elem->tagName == "button") {
            widget = gtk_button_new();
            for (auto& child : elem->children) {
                if (auto text = dynamic_cast<UIText*>(child.get())) {
                    gtk_button_set_label(GTK_BUTTON(widget), text->text.c_str());
                }
            }
        }
        
        if (!widget) return nullptr;
        
        for (auto& [key, val] : elem->attributes) {
            if (key == "className") {
                auto classVal = interp->evaluate(val.get());
                std::string className = std::get<std::string>(classVal);
                GtkStyleContext* ctx = gtk_widget_get_style_context(widget);
                gtk_style_context_add_class(ctx, className.c_str());
            }
        }
        
        if (elem->tagName == "div") {
            for (auto& child : elem->children) {
                GtkWidget* childWidget = buildWidget(child.get(), interp);
                if (childWidget) {
                    gtk_box_pack_start(GTK_BOX(widget), childWidget, FALSE, FALSE, 0);
                }
            }
        }
        
        return widget;
    } else if (auto frag = dynamic_cast<UIFragment*>(expr)) {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        for (auto& child : frag->children) {
            GtkWidget* childWidget = buildWidget(child.get(), interp);
            if (childWidget) {
                gtk_box_pack_start(GTK_BOX(box), childWidget, FALSE, FALSE, 0);
            }
        }
        return box;
    }
    
    return nullptr;
}

class CreateWindowBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "CreateWindow"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        ensureGtkInit();
        
        if (node->args.size() < 5) throw std::runtime_error("CreateWindow(title, x, y, width, height)");
        
        std::string title = std::get<std::string>(interp->evaluate(node->args[0].get()));
        int width = std::get<int>(interp->evaluate(node->args[3].get()));
        int height = std::get<int>(interp->evaluate(node->args[4].get()));
        
        GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), title.c_str());
        gtk_window_set_default_size(GTK_WINDOW(window), width, height);
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);
        
        int id = nextWindowId++;
        windows[id] = window;
        
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_windowId"] = id;
        
        interp->lastValue = obj;
        return "{object}";
    }
};

class WindowRenderBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "render"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee) throw std::runtime_error("render must be called on window");
        
        auto fa = dynamic_cast<FieldAccess*>(node->callee.get());
        if (!fa) throw std::runtime_error("render must be called on window");
        
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        int id = std::get<int>(obj->fields["_windowId"]);
        
        GtkWidget* window = windows[id];
        if (!window) throw std::runtime_error("Invalid window");
        
        if (node->args.size() < 1) throw std::runtime_error("render(ui)");
        
        GtkWidget* content = buildWidget(node->args[0].get(), interp);
        if (content) {
            gtk_container_add(GTK_CONTAINER(window), content);
            gtk_widget_show_all(window);
            gtk_main();
        }
        
        return "";
    }
};

class WindowCloseBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "close"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee) throw std::runtime_error("close must be called on window");
        
        auto fa = dynamic_cast<FieldAccess*>(node->callee.get());
        if (!fa) throw std::runtime_error("close must be called on window");
        
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        int id = std::get<int>(obj->fields["_windowId"]);
        
        GtkWidget* window = windows[id];
        if (window) {
            gtk_widget_destroy(window);
            windows.erase(id);
        }
        
        return "";
    }
};

static CreateWindowBuiltin createWindowInstance;
static WindowRenderBuiltin renderInstance;
static WindowCloseBuiltin closeInstance;
static bool uiBuiltinsRegistered = []() {
    BuiltinRegistry::instance().registerBuiltin(&createWindowInstance);
    BuiltinRegistry::instance().registerBuiltin(&renderInstance);
    BuiltinRegistry::instance().registerBuiltin(&closeInstance);
    return true;
}();
