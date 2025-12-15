# Interactive Game Features

## New Builtin Functions Added

### Input Functions (`src/builtins/input_functions.cpp`)

**Keyboard Input:**
- `isKeyDown(key: string) -> bool` - Check if a key is currently pressed
  - Examples: "W", "A", "S", "D", "Up", "Down", "Left", "Right", "Space"

**Mouse Input:**
- `getMouseX() -> int` - Get current mouse X position
- `getMouseY() -> int` - Get current mouse Y position
- `isMouseDown() -> bool` - Check if mouse button is pressed
- `wasMouseClicked() -> bool` - Check if mouse was clicked (consumes the click)

**Event Processing:**
- `updateInputs()` - Process all input events (replaces `pollEvents()`)

### Enhanced Canvas Functions (`src/builtins/canvas_functions.cpp`)

**New Drawing:**
- `canvas.fillCircle(x: int, y: int, radius: int)` - Draw filled circles

**Existing:**
- `createCanvas(width: int, height: int) -> object`
- `canvas.fillStyle(color: string)`
- `canvas.strokeStyle(color: string)`
- `canvas.fillRect(x, y, w, h)`
- `canvas.strokeRect(x, y, w, h)`
- `canvas.clearRect(x, y, w, h)`
- `canvas.render()`

## Game Example (`examples/game.axo`)

**Box Collector Game** - A simple interactive game demonstrating:
- ✅ Mouse dragging (click and drag the blue box)
- ✅ Keyboard controls (WASD or Arrow keys)
- ✅ Collision detection
- ✅ Score tracking
- ✅ Visual feedback (color changes when dragging)
- ✅ Win condition

**Controls:**
- Mouse: Click and drag the blue player box
- Keyboard: WASD or Arrow keys to move
- Goal: Collect all 5 orange boxes

## Building & Running

```bash
# Build
cmake --build build -j$(sysctl -n hw.ncpu)

# Run the game
./build/compiler examples/game.axo
```

## Implementation Notes

- Input state is tracked globally in `input_functions.cpp`
- `updateInputs()` polls SDL events and updates keyboard/mouse state
- Canvas uses SDL2 for rendering
- Circle drawing uses simple pixel-by-pixel algorithm
- All functions registered via `REGISTER_BUILTIN` macro

## Future Enhancements

Potential additions for more complex games:
- Text rendering on canvas
- Sprite/image loading
- Sound effects
- Timer functions
- More shape primitives (lines, polygons)
- Mouse button differentiation (left/right/middle)
- Key press events (not just key down)
