const vscode = require('vscode');

const keywords = ['var', 'const', 'func', 'if', 'else', 'while', 'for', 'return', 'break', 'continue', 'switch', 'case', 'default', 'try', 'catch', 'finally', 'throw', 'import', 'export', 'use', 'type', 'program', 'await', 'typeof'];
const types = ['int', 'float', 'string', 'bool', 'void', 'any', 'object'];
const constants = [{ name: 'true', doc: 'Boolean true value' }, { name: 'false', doc: 'Boolean false value' }];

const builtins = [
    { name: 'print', sig: 'print(value: any) -> void', doc: 'Print value to console' },
    { name: 'len', sig: 'len(arr: [any]) -> int', doc: 'Get array length' },
    { name: 'push', sig: 'push(arr: [any], value: any) -> void', doc: 'Add element to array' },
    { name: 'pop', sig: 'pop(arr: [any]) -> any', doc: 'Remove and return last element' },
    { name: 'slice', sig: 'slice(arr: [any], start: int, end: int) -> [any]', doc: 'Extract array slice' },
    { name: 'reverse', sig: 'reverse(arr: [any]) -> [any]', doc: 'Reverse array' },
    { name: 'join', sig: 'join(arr: [any], sep: string) -> string', doc: 'Join array elements' },
    { name: 'find', sig: 'find(arr: [any], value: any) -> int', doc: 'Find element index' },
    { name: 'includes', sig: 'includes(arr: [any], value: any) -> bool', doc: 'Check if array contains value' },
    { name: 'sort', sig: 'sort(arr: [any]) -> [any]', doc: 'Sort array' },
    { name: 'toUpper', sig: 'toUpper(str: string) -> string', doc: 'Convert to uppercase' },
    { name: 'toLower', sig: 'toLower(str: string) -> string', doc: 'Convert to lowercase' },
    { name: 'substr', sig: 'substr(str: string, start: int, len: int) -> string', doc: 'Extract substring' },
    { name: 'indexOf', sig: 'indexOf(str: string, search: string) -> int', doc: 'Find substring index' },
    { name: 'contains', sig: 'contains(str: string, search: string) -> bool', doc: 'Check if string contains substring' },
    { name: 'trim', sig: 'trim(str: string) -> string', doc: 'Remove whitespace' },
    { name: 'replace', sig: 'replace(str: string, old: string, new: string) -> string', doc: 'Replace substring' },
    { name: 'split', sig: 'split(str: string, sep: string) -> [string]', doc: 'Split string into array' },
    { name: 'startsWith', sig: 'startsWith(str: string, prefix: string) -> bool', doc: 'Check if starts with prefix' },
    { name: 'endsWith', sig: 'endsWith(str: string, suffix: string) -> bool', doc: 'Check if ends with suffix' },
    { name: 'repeat', sig: 'repeat(str: string, count: int) -> string', doc: 'Repeat string' },
    { name: 'charAt', sig: 'charAt(str: string, index: int) -> string', doc: 'Get character at index' },
    { name: 'charCodeAt', sig: 'charCodeAt(str: string, index: int) -> int', doc: 'Get character code' },
    { name: 'random', sig: 'random() -> float', doc: 'Random number [0,1)' },
    { name: 'pow', sig: 'pow(base: float, exp: float) -> float', doc: 'Power function' },
    { name: 'atan2', sig: 'atan2(y: float, x: float) -> float', doc: 'Arc tangent of y/x' },
    { name: 'toInt', sig: 'toInt(value: any) -> int', doc: 'Convert to integer' },
    { name: 'toFloat', sig: 'toFloat(value: any) -> float', doc: 'Convert to float' },
    { name: 'toString', sig: 'toString(value: any) -> string', doc: 'Convert to string' },
    { name: 'toBool', sig: 'toBool(value: any) -> bool', doc: 'Convert to boolean' },
    { name: 'keys', sig: 'keys(obj: object) -> [string]', doc: 'Get object keys' },
    { name: 'values', sig: 'values(obj: object) -> [any]', doc: 'Get object values' },
    { name: 'hasKey', sig: 'hasKey(obj: object, key: string) -> bool', doc: 'Check if object has key' },
    { name: 'clone', sig: 'clone(obj: object) -> object', doc: 'Clone object' },
    { name: 'merge', sig: 'merge(obj1: object, obj2: object) -> object', doc: 'Merge objects' },
    { name: 'millis', sig: 'millis() -> int', doc: 'Get milliseconds since epoch' },
    { name: 'sleep', sig: 'sleep(ms: int) -> void', doc: 'Sleep for milliseconds' },
    { name: 'assert', sig: 'assert(condition: bool, msg: string) -> void', doc: 'Assert condition' },
    { name: 'error', sig: 'error(msg: string) -> void', doc: 'Throw error' },
    { name: 'read', sig: 'read(path: string) -> string', doc: 'Read file contents' },
    { name: 'write', sig: 'write(path: string, content: string) -> void', doc: 'Write file' },
    { name: 'readDir', sig: 'readDir(path: string) -> [string]', doc: 'List directory' },
    { name: 'copy', sig: 'copy(src: string, dst: string) -> void', doc: 'Copy file' },
    { name: 'json_parse', sig: 'json_parse(json: string) -> object', doc: 'Parse JSON string' },
    { name: 'json_stringify', sig: 'json_stringify(obj: object) -> string', doc: 'Convert to JSON' },
    { name: 'fetch', sig: 'fetch(url: string) -> string', doc: 'HTTP GET request' },
    { name: 'createServer', sig: 'createServer(port: int, handler: func) -> void', doc: 'Create HTTP server' },
    { name: 'createCanvas', sig: 'createCanvas(width: int, height: int) -> object', doc: 'Create canvas' },
    { name: 'fillRect', sig: 'fillRect(x: int, y: int, w: int, h: int) -> void', doc: 'Draw filled rectangle' },
    { name: 'strokeRect', sig: 'strokeRect(x: int, y: int, w: int, h: int) -> void', doc: 'Draw rectangle outline' },
    { name: 'clearRect', sig: 'clearRect(x: int, y: int, w: int, h: int) -> void', doc: 'Clear rectangle' },
    { name: 'fillCircle', sig: 'fillCircle(x: int, y: int, r: int) -> void', doc: 'Draw filled circle' },
    { name: 'drawLine', sig: 'drawLine(x1: int, y1: int, x2: int, y2: int) -> void', doc: 'Draw line' },
    { name: 'drawImage', sig: 'drawImage(img: object, x: int, y: int) -> void', doc: 'Draw image' },
    { name: 'loadImage', sig: 'loadImage(path: string) -> object', doc: 'Load image file' },
    { name: 'isKeyDown', sig: 'isKeyDown(key: string) -> bool', doc: 'Check if key is pressed' },
    { name: 'getMouseX', sig: 'getMouseX() -> int', doc: 'Get mouse X position' },
    { name: 'getMouseY', sig: 'getMouseY() -> int', doc: 'Get mouse Y position' },
    { name: 'isMouseDown', sig: 'isMouseDown() -> bool', doc: 'Check if mouse button down' },
    { name: 'wasMouseClicked', sig: 'wasMouseClicked() -> bool', doc: 'Check if mouse was clicked' },
    { name: 'pollEvents', sig: 'pollEvents() -> void', doc: 'Poll input events' },
    { name: 'updateInputs', sig: 'updateInputs() -> void', doc: 'Update input state' },
    { name: 'getKeyState', sig: 'getKeyState(key: string) -> bool', doc: 'Get key state' },
    { name: 'CreateWindow', sig: 'CreateWindow(title: string, w: int, h: int) -> object', doc: 'Create window' },
    { name: 'createScene', sig: 'createScene() -> object', doc: 'Create 3D scene' },
    { name: 'render', sig: 'render(scene: object) -> void', doc: 'Render scene' },
    { name: 'renderWindow', sig: 'renderWindow(window: object) -> void', doc: 'Render window' },
    { name: 'PerspectiveCamera', sig: 'PerspectiveCamera(fov: float, aspect: float) -> object', doc: 'Create perspective camera' },
    { name: 'BoxGeometry', sig: 'BoxGeometry(w: float, h: float, d: float) -> object', doc: 'Create box geometry' },
    { name: 'SphereGeometry', sig: 'SphereGeometry(radius: float) -> object', doc: 'Create sphere geometry' },
    { name: 'PlaneGeometry', sig: 'PlaneGeometry(w: float, h: float) -> object', doc: 'Create plane geometry' },
    { name: 'TorusGeometry', sig: 'TorusGeometry(r: float, tube: float) -> object', doc: 'Create torus geometry' },
    { name: 'CylinderGeometry', sig: 'CylinderGeometry(r: float, h: float) -> object', doc: 'Create cylinder geometry' },
    { name: 'loadOBJ', sig: 'loadOBJ(path: string) -> object', doc: 'Load OBJ 3D model' },
    { name: 'PointLight', sig: 'PointLight(color: object) -> object', doc: 'Create point light' },
    { name: 'DirectionalLight', sig: 'DirectionalLight(color: object) -> object', doc: 'Create directional light' },
    { name: 'AmbientLight', sig: 'AmbientLight(color: object) -> object', doc: 'Create ambient light' },
    { name: 'setPosition', sig: 'setPosition(obj: object, x: float, y: float, z: float) -> void', doc: 'Set object position' },
    { name: 'setRotation', sig: 'setRotation(obj: object, x: float, y: float, z: float) -> void', doc: 'Set object rotation' },
    { name: 'setScale', sig: 'setScale(obj: object, x: float, y: float, z: float) -> void', doc: 'Set object scale' },
    { name: 'setColor', sig: 'setColor(obj: object, r: float, g: float, b: float) -> void', doc: 'Set object color' },
    { name: 'setGraphics', sig: 'setGraphics(obj: object, metallic: float) -> void', doc: 'Set graphics properties' },
    { name: 'setMetallic', sig: 'setMetallic(obj: object, value: float) -> void', doc: 'Set metallic property' },
    { name: 'enableDevMode', sig: 'enableDevMode() -> void', doc: 'Enable dev camera mode' },
    { name: 'updateDevCamera', sig: 'updateDevCamera(camera: object) -> void', doc: 'Update dev camera' },
    { name: 'handleDevInput', sig: 'handleDevInput(camera: object) -> void', doc: 'Handle dev input' },
    { name: 'add', sig: 'add(scene: object, obj: object) -> void', doc: 'Add object to scene' },
    { name: 'lookAt', sig: 'lookAt(obj: object, target: object) -> void', doc: 'Make object look at target' },
    { name: 'followTarget', sig: 'followTarget(obj: object, target: object) -> void', doc: 'Follow target' },
    { name: 'moveBy', sig: 'moveBy(obj: object, x: float, y: float, z: float) -> void', doc: 'Move object by offset' },
    { name: 'getPosition', sig: 'getPosition(obj: object) -> object', doc: 'Get object position' },
    { name: 'applyGravity', sig: 'applyGravity(obj: object, force: float) -> void', doc: 'Apply gravity' },
    { name: 'handleCollision', sig: 'handleCollision(obj1: object, obj2: object) -> void', doc: 'Handle collision' },
    { name: 'isColliding', sig: 'isColliding(obj1: object, obj2: object) -> bool', doc: 'Check collision' },
    { name: 'setBounciness', sig: 'setBounciness(obj: object, value: float) -> void', doc: 'Set bounciness' },
    { name: 'setFriction', sig: 'setFriction(obj: object, value: float) -> void', doc: 'Set friction' },
    { name: 'addVelocity', sig: 'addVelocity(obj: object, x: float, y: float, z: float) -> void', doc: 'Add velocity' },
    { name: 'applyNaturalCollision', sig: 'applyNaturalCollision(obj1: object, obj2: object) -> void', doc: 'Apply natural collision' },
    { name: 'getGroundNormal', sig: 'getGroundNormal(obj: object) -> object', doc: 'Get ground normal' },
    { name: 'applyAttractionToMesh', sig: 'applyAttractionToMesh(obj: object, mesh: object) -> void', doc: 'Apply attraction' },
    { name: 'deformVertices', sig: 'deformVertices(mesh: object, factor: float) -> void', doc: 'Deform mesh vertices' },
    { name: 'fillStyle', sig: 'fillStyle(color: string) -> void', doc: 'Set fill color' },
    { name: 'strokeStyle', sig: 'strokeStyle(color: string) -> void', doc: 'Set stroke color' },
    { name: 'loadCSS', sig: 'loadCSS(path: string) -> void', doc: 'Load CSS file' },
    { name: 'writeHead', sig: 'writeHead(code: int, headers: object) -> void', doc: 'Write HTTP headers' },
    { name: 'end', sig: 'end(content: string) -> void', doc: 'End HTTP response' },
    { name: 'close', sig: 'close() -> void', doc: 'Close connection' },
    { name: 'mysqlConnect', sig: 'mysqlConnect(host: string, user: string, password: string, database: string) -> object', doc: 'Connect to MySQL database' },
    { name: 'query', sig: 'query(sql: string) -> [object]', doc: 'Execute SQL query' },
    { name: 'wsConnect', sig: 'wsConnect(url: string) -> object', doc: 'Connect to WebSocket server' },
    { name: 'send', sig: 'send(message: string) -> void', doc: 'Send WebSocket message' },
    { name: 'receive', sig: 'receive() -> string', doc: 'Receive WebSocket message' }
];

function activate(context) {
    console.log('Axolotl IntelliSense extension activated');
    
    const completionProvider = vscode.languages.registerCompletionItemProvider(
        { scheme: '*', language: 'axolotl' },
        {
            provideCompletionItems(document, position) {
                const items = [];

                keywords.forEach(kw => {
                    const item = new vscode.CompletionItem(kw, vscode.CompletionItemKind.Keyword);
                    item.sortText = '0' + kw;
                    items.push(item);
                });

                types.forEach(t => {
                    const item = new vscode.CompletionItem(t, vscode.CompletionItemKind.TypeParameter);
                    item.sortText = '0' + t;
                    items.push(item);
                });

                constants.forEach(c => {
                    const item = new vscode.CompletionItem(c.name, vscode.CompletionItemKind.Constant);
                    item.documentation = c.doc;
                    item.sortText = '0' + c.name;
                    items.push(item);
                });

                builtins.forEach(fn => {
                    const item = new vscode.CompletionItem(fn.name, vscode.CompletionItemKind.Function);
                    item.detail = fn.sig;
                    item.documentation = fn.doc;
                    item.sortText = '0' + fn.name;
                    items.push(item);
                });

                return items;
            }
        },
        '.', '(' // Trigger characters
    );

    const hoverProvider = vscode.languages.registerHoverProvider(
        { scheme: '*', language: 'axolotl' },
        {
            provideHover(document, position) {
                const range = document.getWordRangeAtPosition(position);
                if (!range) return;
                const word = document.getText(range);
                const fn = builtins.find(b => b.name === word);
                if (fn) {
                    return new vscode.Hover(`**${fn.sig}**\n\n${fn.doc}`);
                }
            }
        }
    );

    const signatureProvider = vscode.languages.registerSignatureHelpProvider(
        { scheme: '*', language: 'axolotl' },
        {
            provideSignatureHelp(document, position) {
                const line = document.lineAt(position.line).text;
                const beforeCursor = line.substring(0, position.character);
                const match = beforeCursor.match(/([a-zA-Z_][a-zA-Z0-9_]*)\s*\(/);
                if (match) {
                    const fnName = match[1];
                    const fn = builtins.find(b => b.name === fnName);
                    if (fn) {
                        const sigHelp = new vscode.SignatureHelp();
                        const sig = new vscode.SignatureInformation(fn.sig, fn.doc);
                        sigHelp.signatures = [sig];
                        sigHelp.activeSignature = 0;
                        sigHelp.activeParameter = 0;
                        return sigHelp;
                    }
                }
            }
        },
        '(', ','
    );

    context.subscriptions.push(completionProvider, hoverProvider, signatureProvider);
}

function deactivate() { }

module.exports = { activate, deactivate };
