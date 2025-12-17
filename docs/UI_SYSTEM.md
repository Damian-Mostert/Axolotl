# Axolotl UI System

## Overview

Axolotl now supports JSX-like syntax for building native UI applications using GTK3. The syntax is similar to React/JSX but renders to native GTK widgets.

## Features

- **Native UI rendering** via GTK3 (no HTML/browser)
- **CSS styling** with standard CSS modules
- **Event handlers** for user interactions
- **Window management** with CreateWindow builtin
- **Template syntax** using `<tag>` elements

## Basic Example

```axo
import "styles/menu.css";

func runMenu() -> any {
    const window:object = CreateWindow("Main Menu", 0, 0, 800, 600);
    window.render(<>
        <div className="menu-container">
            <button className="button-2d" onClick={func() -> void {
                window.close();
                import "src/scripts/levels/2d.axo";
            }}>Start 2D Level</button>
            <button className="button-3d" onClick={func() -> void {
                window.close();
                import "src/scripts/levels/3d.axo";
            }}>Start 3D Level</button>
        </div>
    </>);
}
```

## API Reference

### CreateWindow(title, x, y, width, height)

Creates a new GTK window.

**Parameters:**
- `title: string` - Window title
- `x: int` - X position
- `y: int` - Y position  
- `width: int` - Window width
- `height: int` - Window height

**Returns:** Window object with `render()` and `close()` methods

### window.render(ui)

Renders UI template to the window.

**Parameters:**
- `ui: UIElement` - UI template (e.g., `<div>...</div>`)

### window.close()

Closes and destroys the window.

## Supported Elements

- `<div>` - Container (GTK Box)
- `<button>` - Button widget

## Supported Attributes

- `className` - CSS class name
- `onClick` - Click event handler (buttons only)

## CSS Import

Import CSS files to style your UI:

```axo
import "styles/app.css";
```

CSS is loaded globally and applied to all windows using GTK's CSS provider.

## Implementation Details

- **Lexer**: Recognizes `<`, `>`, `/` for UI templates
- **Parser**: `parseUIElement()` and `parseUIFragment()` methods
- **AST**: `UIElement`, `UIFragment`, `UIText`, `UIExpression` nodes
- **Interpreter**: Stores UI nodes for rendering
- **Builtins**: `CreateWindow`, `render`, `close` functions
- **Backend**: GTK3 for native widgets and CSS styling

## Build Requirements

Add GTK3 to your build:

```bash
pkg-config --cflags --libs gtk+-3.0
```

Update CMakeLists.txt to link GTK3.
