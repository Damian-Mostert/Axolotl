#include "include/builtins.h"
#include "include/interpreter.h"
#include "include/ui_ast.h"
#include <gtk/gtk.h>
#include <map>
#include <functional>

static bool gtkInitialized = false;
static std::map<int, GtkWidget*> windows;
static std::map<GtkWidget*, std::function<void()>> clickHandlers;
static int nextWindowId = 1;
static Interpreter* globalInterp = nullptr;

static void ensureGtkInit() {
    if (!gtkInitialized) {
        gtk_init(nullptr, nullptr);
        gtkInitialized = true;
    }
}

static void onButtonClick(GtkWidget* widget, gpointer data) {
    auto it = clickHandlers.find(widget);
    if (it != clickHandlers.end()) {
        it->second();
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
            } else if (key == "onClick") {
                clickHandlers[widget] = [interp, funcExpr = val.get()]() {
                    if (auto fe = dynamic_cast<FunctionExpression*>(funcExpr)) {
                        interp->environment.pushScope();
                        try {
                            interp->executeBlock(fe->body.get());
                        } catch (...) {
                            interp->environment.popScope();
                            throw;
                        }
                        interp->environment.popScope();
                    }
                };
                if (elem->tagName == "button") {
                    g_signal_connect(widget, "clicked", G_CALLBACK(onButtonClick), nullptr);
                } else {
                    GtkWidget* eventBox = gtk_event_box_new();
                    gtk_container_add(GTK_CONTAINER(eventBox), widget);
                    g_signal_connect(eventBox, "button-press-event", G_CALLBACK(+[](GtkWidget* w, GdkEventButton* e, gpointer d) -> gboolean {
                        onButtonClick(w, d);
                        return TRUE;
                    }), nullptr);
                    widget = eventBox;
                }
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
        gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
        g_signal_connect(window, "destroy", G_CALLBACK(+[](GtkWidget* w, gpointer d) {
            gtk_main_quit();
        }), nullptr);
        
        int id = nextWindowId++;
        windows[id] = window;
        
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_windowId"] = id;
        
        interp->lastValue = obj;
        return "{object}";
    }
};

class LoadCSSBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "loadCSS"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        ensureGtkInit();
        if (node->args.size() != 1) throw std::runtime_error("loadCSS(path)");
        std::string path = std::get<std::string>(interp->evaluate(node->args[0].get()));
        
        GtkCssProvider* provider = gtk_css_provider_new();
        GError* error = nullptr;
        gtk_css_provider_load_from_path(provider, path.c_str(), &error);
        if (error) {
            g_error_free(error);
        }
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
        g_object_unref(provider);
        return "";
    }
};

class WindowRenderBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "renderWindow"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        globalInterp = interp;
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
            gtk_main_quit();
            gtk_widget_destroy(window);
            windows.erase(id);
        }
        
        return "";
    }
};

static CreateWindowBuiltin createWindowInstance;
static WindowRenderBuiltin renderInstance;
static WindowCloseBuiltin closeInstance;
static LoadCSSBuiltin loadCSSInstance;
static bool uiBuiltinsRegistered = []() {
    BuiltinRegistry::instance().registerBuiltin(&createWindowInstance);
    BuiltinRegistry::instance().registerBuiltin(&renderInstance);
    BuiltinRegistry::instance().registerBuiltin(&closeInstance);
    BuiltinRegistry::instance().registerBuiltin(&loadCSSInstance);
    return true;
}();
