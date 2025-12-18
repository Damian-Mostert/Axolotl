# 🦎 Axolotl Programming Language

A modern, statically-typed programming language with powerful type inference, union types, and seamless module system.

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Status](https://img.shields.io/badge/status-active-success)

---

## 🚀 Quick Start

```axo
// Hello World
print("Hello, Axolotl! 🦎");

// Variables with type inference
var name: string = "Alice";
var age: int = 25;
const PI: float = 3.14159;

// Functions
func greet(name: string) -> string {
    return "Hello, " + name + "!";
}

print(greet("World"));
```

---

## ✨ Key Features

### 🎯 **Static Typing with Union Types**
```axo
type Result = string|int|bool;
type User = {name: string, age: int};

var data: Result = "success";
var user: User = {name: "Bob", age: 30};
```

### 📦 **Modern Module System**
```axo
// Export
export func calculate(x: int) -> int {
    return x * 2;
}

// Import
import {calculate} from "./math.axo";
```

### 🔥 **Powerful Type System**
```axo
// Union types
type ID = string|int;

// Object types
type Person = {
    id: ID,
    name: string,
    email: string
};

// Array types
type Matrix = [[float]];

// Literal types
type Status = "active"|"pending"|"closed";
```

### ⚡ **First-Class Functions**
```axo
var multiply: func = func(x: int, y: int) -> int {
    return x * y;
};

var result: int = multiply(5, 3);
```

### 🛡️ **Error Handling**
```axo
try {
    throw "Something went wrong";
} catch (error) {
    print("Error:", error);
} finally {
    print("Cleanup");
}
```

### 🔄 **Control Flow**
```axo
// If-else
if (x > 10) {
    print("Large");
} else {
    print("Small");
}

// Loops
for (var i: int = 0; i < 10; i = i + 1) {
    print(i);
}

while (condition) {
    // code
}

// Switch with fallthrough
switch (grade) {
    case "A":
    case "B":
        print("Excellent!");
        break;
    default:
        print("Keep trying");
}
```

---

## 📚 Language Basics

### Variables
```axo
var mutable: int = 42;        // Mutable
const immutable: int = 100;   // Immutable
```

### Data Types
```axo
var integer: int = 42;
var decimal: float = 3.14;
var text: string = "hello";
var flag: bool = true;
var list: [int] = [1, 2, 3];
var dict: object = {key: "value"};
```

### Operators
```axo
// Arithmetic: + - * / %
// Comparison: == != < > <= >=
// Logical: && || !
// Special: typeof await
```

### Functions
```axo
func add(a: int, b: int) -> int {
    return a + b;
}

// Recursive
func factorial(n: int) -> int {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
```

### Arrays
```axo
var numbers: [int] = [1, 2, 3, 4, 5];
var first: int = numbers[0];
numbers[1] = 10;

push(numbers, 6);
var last: int = pop(numbers);
var size: int = len(numbers);
```

### Objects
```axo
var person: object = {
    name: "Alice",
    age: 30,
    email: "alice@example.com"
};

var name: string = person.name;
person.age = 31;
```

---

## 🎨 Advanced Features

### Custom Types
```axo
type Point = {x: float, y: float};
type Color = "red"|"green"|"blue";
type Optional = string|int;

var point: Point = {x: 10.5, y: 20.3};
var color: Color = "red";
```

### Type Checking
```axo
if (typeof variable == "int") {
    print("It's an integer!");
}

if (typeof arr == "[string]") {
    print("Array of strings!");
}
```

### Modules
```axo
// math.axo
export func square(x: int) -> int {
    return x * x;
}

export const PI: float = 3.14159;

// main.axo
import {square, PI} from "./math.axo";
print(square(5));
```

### Programs (Async)
```axo
program worker(data: [int]) {
    for (var i: int = 0; i < len(data); i = i + 1) {
        print("Processing:", data[i]);
    }
}

// Run synchronously
worker([1, 2, 3]);

// Run asynchronously
await worker([4, 5, 6]);
```

---

## 🛠️ Installation

### Prerequisites
- C++17 compiler (GCC/Clang/MSVC)
- CMake 3.10+

### Build from Source
```bash
# Clone repository
git clone https://github.com/yourusername/axolotl.git
cd axolotl

# Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run
./build/compiler examples/hello.axo
```

### Quick Install (Unix/Linux/macOS)
```bash
./install.sh
```

### Quick Install (Windows)
```cmd
install.bat
```

---

## 📖 Usage

### Run a Program
```bash
axolotl program.axo
```

### Compile to Executable
```bash
axolotl compile program.axo output_name
./output_name
```

### Interactive REPL
```bash
axolotl
```

---

## 📝 Examples

### Fibonacci
```axo
func fibonacci(n: int) -> int {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

print("Fib(10):", fibonacci(10));
```

### Array Processing
```axo
func sum(numbers: [int]) -> int {
    var total: int = 0;
    for (var i: int = 0; i < len(numbers); i = i + 1) {
        total = total + numbers[i];
    }
    return total;
}

var data: [int] = [1, 2, 3, 4, 5];
print("Sum:", sum(data));
```

### Error Handling
```axo
func divide(a: int, b: int) -> float {
    if (b == 0) {
        throw "Division by zero";
    }
    return a / b;
}

try {
    var result: float = divide(10, 0);
} catch (e) {
    print("Error:", e);
}
```

### Union Types
```axo
type Response = {success: bool, data: string}|{error: string};

func fetchData(id: int) -> Response {
    if (id > 0) {
        return {success: true, data: "User data"};
    }
    return {error: "Invalid ID"};
}

var response: Response = fetchData(1);
```

---

## 🎯 Built-in Functions

### I/O Functions
```axo
print(value1, value2, ...);      // Print values to console
write(filepath, content);        // Write content to file
read(filepath);                  // Read file content
readDir(dirPath);                // List directory contents
copy(sourcePath, destPath);      // Copy file
```

### Array Functions
```axo
len(array);                      // Get array/string length
push(array, value);              // Add element (returns new array)
pop(array);                      // Remove and return last element
slice(array, start, end);        // Extract array slice
reverse(array);                  // Reverse array
join(array, separator);          // Join array to string
find(array, value);              // Find index of value (-1 if not found)
includes(array, value);          // Check if array contains value
sort(array);                     // Sort array in-place
```

### String Functions
```axo
toUpper(str);                    // Convert to uppercase
toLower(str);                    // Convert to lowercase
substr(str, start, length);      // Extract substring
indexOf(str, search);            // Find first occurrence (-1 if not found)
contains(str, search);           // Check if string contains substring
trim(str);                       // Remove leading/trailing whitespace
replace(str, search, replace);   // Replace first occurrence
split(str, delimiter);           // Split string into array
startsWith(str, prefix);         // Check if starts with prefix
endsWith(str, suffix);           // Check if ends with suffix
repeat(str, count);              // Repeat string n times
charAt(str, index);              // Get character at index
charCodeAt(str, index);          // Get ASCII code at index
```

### Math Functions
```axo
// Trigonometric
sin(x);                          // Sine
cos(x);                          // Cosine
tan(x);                          // Tangent
asin(x);                         // Arc sine
acos(x);                         // Arc cosine
atan(x);                         // Arc tangent
atan2(y, x);                     // Arc tangent of y/x

// Exponential & Logarithmic
sqrt(x);                         // Square root
pow(base, exponent);             // Power
exp(x);                          // e^x
log(x);                          // Natural logarithm
log10(x);                        // Base-10 logarithm

// Random
random();                        // Random float [0, 1)
```

### Conversion Functions
```axo
toString(value);                 // Convert any value to string
toInt(value);                    // Convert to integer
toFloat(value);                  // Convert to float
toBool(value);                   // Convert to boolean
```

### Object Functions
```axo
keys(object);                    // Get array of object keys
values(object);                  // Get array of object values
hasKey(object, key);             // Check if object has key
clone(value);                    // Deep clone array/object
merge(obj1, obj2);               // Merge two objects
```

### Utility Functions
```axo
millis();                        // Current time in milliseconds
sleep(ms);                       // Sleep for milliseconds
typeof(value);                   // Get type of value
assert(condition, message);      // Assert condition or throw error
error(message);                  // Throw error with message
```

### Input Functions (SDL2)
```axo
isKeyDown(keyName);              // Check if key is pressed
getMouseX();                     // Get mouse X position
getMouseY();                     // Get mouse Y position
isMouseDown();                   // Check if mouse button is down
wasMouseClicked();               // Check if mouse was clicked (clears flag)
updateInputs();                  // Poll SDL events (call in game loop)
getKeyState();                   // Get object with arrow/WASD/space states
```

### JSON Functions
```axo
json_parse(jsonString);          // Parse JSON string to object
json_stringify(object);          // Convert object to JSON string
```

### HTTP Functions
```axo
// Fetch API
fetch(url);                      // GET request
fetch(url, options);             // Custom request
// Options: {method: "GET|POST|PUT|DELETE|PATCH", body: data, headers: {}}
// Returns: {status: int, body: string|object, headers: string, ok: bool, url: string}

// Server API
createServer(callback, port);    // Create HTTP server
// Callback receives (req, res) objects
// req: {method: string, url: string, path: string}
// res: {writeHead(status, headers), write(data), end(data)}
```

### Canvas 2D Functions
```axo
// Canvas Creation
createCanvas(width, height, title);  // Create 2D canvas window

// Drawing Methods (called on canvas object)
canvas.fillStyle("#RRGGBB");         // Set fill color
canvas.strokeStyle("#RRGGBB");       // Set stroke color
canvas.fillRect(x, y, w, h);         // Draw filled rectangle
canvas.strokeRect(x, y, w, h);       // Draw rectangle outline
canvas.clearRect(x, y, w, h);        // Clear rectangle area
canvas.fillCircle(x, y, radius);     // Draw filled circle
canvas.drawLine(x1, y1, x2, y2);     // Draw line

// Image Functions
loadImage(path);                     // Load image from file
canvas.drawImage(img, x, y);         // Draw image at position
canvas.drawImage(img, x, y, w, h);   // Draw scaled image

// Canvas Control
canvas.render();                     // Present canvas (2D mode)
canvas.close();                      // Close canvas window
pollEvents();                        // Handle window events
```

### Canvas 3D Functions
```axo
// Scene & Camera
createScene();                       // Create 3D scene
PerspectiveCamera(fov);              // Create perspective camera
camera.setPosition(x, y, z);         // Set camera position
camera.lookAt(x, y, z);              // Point camera at target
camera.followTarget(mesh, dist, h);  // Follow mesh with camera

// Geometry Creation
BoxGeometry(size);                   // Create box (cube)
BoxGeometry(w, h, d);                // Create box with dimensions
SphereGeometry(radius, wSeg, hSeg);  // Create sphere
PlaneGeometry(width, height);        // Create plane
TorusGeometry(r, tube, rSeg, tSeg);  // Create torus
CylinderGeometry(rTop, rBot, h, seg);// Create cylinder
loadOBJ(filepath);                   // Load .obj 3D model

// Mesh Manipulation
mesh.setPosition(x, y, z);           // Set mesh position
mesh.setRotation(x, y, z);           // Set mesh rotation (radians)
mesh.setScale(x, y, z);              // Set mesh scale
mesh.setColor("#RRGGBB");            // Set mesh color
mesh.setMetallic(value);             // Set metallic property (0-1)
mesh.setFriction(value);             // Set friction (0-1)
mesh.setBounciness(value);           // Set bounciness (0-1)
mesh.moveBy(dx, dy, dz);             // Move mesh relatively
mesh.getPosition();                  // Get position {x, y, z}
mesh.getGroundNormal();              // Get ground normal {x, y, z}
mesh.deformVertices(amp, freq, seed);// Deform mesh vertices

// Physics
mesh.addVelocity(x, y, z);           // Add velocity to mesh
applyGravity(mesh, g, obstacles...); // Apply gravity with collision
handleCollision(obj, obstacles...);  // Handle collision response
isColliding(mesh1, mesh2, ...);      // Check collision (returns bool)
applyNaturalCollision(meshes...);    // Apply physics collision
applyAttractionToMesh(m1, m2);       // Apply attraction force

// Lighting
PointLight("#RRGGBB", intensity);    // Create point light
AmbientLight("#RRGGBB", intensity);  // Create ambient light
DirectionalLight("#RRGGBB", int);    // Create directional light
light.setPosition(x, y, z);          // Set light position

// Scene Management
scene.add(mesh);                     // Add mesh to scene
scene.add(light);                    // Add light to scene
canvas.render(scene, camera);        // Render 3D scene

// Graphics Settings
setGraphics(scene, settings);        // Configure graphics
// Settings object: {
//   msaa: bool,              // Enable MSAA
//   msaaSamples: int,        // MSAA samples (4, 8, 16)
//   depthTest: bool,         // Enable depth testing
//   smoothing: bool,         // Enable line/polygon smoothing
//   shadows: bool,           // Enable shadows
//   shadowIntensity: float,  // Shadow darkness (0-1)
//   fog: bool,               // Enable fog
//   fogDensity: float,       // Fog density
//   fogColor: "#RRGGBB"      // Fog color
// }

// Dev Mode (Camera Controls)
enableDevMode(camera);               // Enable dev camera controls
updateDevCamera();                   // Update dev camera position
handleDevInput();                    // Handle dev input (WASD, mouse)
// Dev controls: WASD (move), QE (up/down), mouse drag (rotate), scroll (zoom)
```

### Example: 2D Canvas
```axo
var canvas: object = createCanvas(800, 600, "My Game");
var sprite: object = loadImage("player.png");

canvas.fillStyle("#87CEEB");
canvas.fillRect(0, 0, 800, 600);

canvas.fillStyle("#00FF00");
canvas.fillCircle(400, 300, 50);

canvas.drawImage(sprite, 100, 100);
canvas.render();
```

### Example: 3D Scene
```axo
var canvas: object = createCanvas(800, 600, "3D Demo");
var scene: object = createScene();
var camera: object = PerspectiveCamera(60);
camera.setPosition(0, 5, 10);
camera.lookAt(0, 0, 0);

var box: object = BoxGeometry(2);
box.setPosition(0, 0, 0);
box.setColor("#FF5733");
box.setRotation(0.5, 0.5, 0);

var light: object = PointLight("#FFFFFF", 1.0);
light.setPosition(5, 10, 5);

scene.add(box);
scene.add(light);

while (handleDevInput()) {
    updateDevCamera();
    canvas.render(scene, camera);
}
```

---

## 🌐 Environment & Process

### Environment Variables
```axo
// Load from .env file
var dbHost: string = process.env.DB_HOST;
var apiKey: string = process.env.API_KEY;
```

### Process Info
```axo
var args: [string] = process.args;    // Command-line args
var cwd: string = process.cwd;        // Working directory
```

---

## 🔧 VS Code Extension

Syntax highlighting and file icons available in `lang-ext/` folder.

### Install Extension
```bash
cd lang-ext
npm install -g vsce
npm run build
code --install-extension ./lang-syntax-1.0.0.vsix
```

### Enable File Icons
1. Open Command Palette (Ctrl+Shift+P)
2. Select "Preferences: File Icon Theme"
3. Choose "Axolotl Icons"

---

## 📊 Performance

- **JIT Compilation**: Automatic optimization for hot loops
- **Type Checking**: Compile-time type validation
- **Memory Efficient**: Smart pointer-based memory management
- **Fast Execution**: Tree-walking interpreter with optimizations

---

## 🤝 Contributing

Contributions welcome! Areas for improvement:
- [ ] Enhanced type inference
- [ ] More standard library functions
- [ ] Bytecode backend
- [ ] Language server protocol (LSP)
- [ ] Package manager

---

## 📄 Documentation

- **[Syntax Guide](SYNTAX_GUIDE.md)** - Complete language reference
- **[Examples](examples/)** - Sample programs
- **[Tests](tests/)** - Test suite

---

## 🐛 Error Messages

Axolotl provides clear, helpful error messages:

```
Fatal error: Expected ']' after array type (line 1, col 14)
  File: examples/bad_array.axo:1:14
const a:[int = 5;
             ^
```

---

## 🎓 Learning Resources

### Tutorials
1. Start with `examples/no_semicolons.axo` - Basic syntax
2. Explore `tests/arrays.axo` - Array operations
3. Study `tests/union_types_test.axo` - Advanced types
4. Review `tests/comprehensive_test.axo` - All features

### Language Philosophy
- **Explicit over implicit** - Type annotations required
- **Safety first** - Static typing with runtime checks
- **Modern features** - Union types, modules, error handling
- **Developer friendly** - Clear syntax, helpful errors

---

## 📜 License

MIT License - See [LICENSE](LICENSE) file

---

## 🙏 Acknowledgments

Built with modern C++17, featuring:
- Recursive descent parser
- Tree-walking interpreter
- LLVM JIT compilation (optional)
- Smart pointer memory management

---

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/yourusername/axolotl/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/axolotl/discussions)
- **Documentation**: [Syntax Guide](SYNTAX_GUIDE.md)

---

**Made with ❤️ by the Axolotl team**

🦎 *Regenerate your code with Axolotl!*
