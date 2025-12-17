"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.deactivate = exports.activate = void 0;
const vscode = require("vscode");
const BUILTINS = {
    'print': 'print(...args: any) -> void',
    'len': 'len(value: string|[any]) -> int',
    'push': 'push(array: [any], value: any) -> void',
    'pop': 'pop(array: [any]) -> any',
    'slice': 'slice(array: [any], start: int, end: int) -> [any]',
    'reverse': 'reverse(array: [any]) -> [any]',
    'join': 'join(array: [any], separator: string) -> string',
    'sort': 'sort(array: [any]) -> void',
    'find': 'find(array: [any], value: any) -> int',
    'includes': 'includes(array: [any], value: any) -> bool',
    'substr': 'substr(str: string, start: int, length: int) -> string',
    'toUpper': 'toUpper(str: string) -> string',
    'toLower': 'toLower(str: string) -> string',
    'indexOf': 'indexOf(str: string, substring: string) -> int',
    'contains': 'contains(str: string, substring: string) -> bool',
    'trim': 'trim(str: string) -> string',
    'replace': 'replace(str: string, search: string, replacement: string) -> string',
    'split': 'split(str: string, delimiter: string) -> [string]',
    'startsWith': 'startsWith(str: string, prefix: string) -> bool',
    'endsWith': 'endsWith(str: string, suffix: string) -> bool',
    'repeat': 'repeat(str: string, count: int) -> string',
    'charAt': 'charAt(str: string, index: int) -> string',
    'charCodeAt': 'charCodeAt(str: string, index: int) -> int',
    'millis': 'millis() -> int',
    'sleep': 'sleep(ms: int) -> void',
    'typeof': 'typeof(value: any) -> string',
    'toString': 'toString(value: any) -> string',
    'toInt': 'toInt(value: string|float|bool) -> int',
    'toFloat': 'toFloat(value: string|int) -> float',
    'toBool': 'toBool(value: any) -> bool',
    'sin': 'sin(angle: float) -> float',
    'cos': 'cos(angle: float) -> float',
    'tan': 'tan(angle: float) -> float',
    'sqrt': 'sqrt(value: float) -> float',
    'pow': 'pow(base: float, exponent: float) -> float',
    'abs': 'abs(value: int|float) -> int|float',
    'floor': 'floor(value: float) -> int',
    'ceil': 'ceil(value: float) -> int',
    'round': 'round(value: float) -> int',
    'min': 'min(a: int|float, b: int|float) -> int|float',
    'max': 'max(a: int|float, b: int|float) -> int|float',
    'random': 'random() -> float',
    'log': 'log(value: float) -> float',
    'log10': 'log10(value: float) -> float',
    'exp': 'exp(value: float) -> float',
    'asin': 'asin(value: float) -> float',
    'acos': 'acos(value: float) -> float',
    'atan': 'atan(value: float) -> float',
    'atan2': 'atan2(y: float, x: float) -> float',
    'clamp': 'clamp(value: int|float, min: int|float, max: int|float) -> int|float',
    'lerp': 'lerp(start: float, end: float, t: float) -> float',
    'keys': 'keys(object: object) -> [string]',
    'values': 'values(object: object) -> [any]',
    'hasKey': 'hasKey(object: object, key: string) -> bool',
    'clone': 'clone(value: any) -> any',
    'merge': 'merge(obj1: object, obj2: object) -> object',
    'assert': 'assert(condition: bool, message: string) -> void',
    'error': 'error(message: string) -> void',
    'read': 'read(filepath: string) -> string',
    'write': 'write(filepath: string, content: string) -> void',
    'copy': 'copy(source: string, dest: string) -> void',
    'readDir': 'readDir(dirPath: string) -> [string]',
    'fetch': 'fetch(url: string, options?: object) -> object',
    'createCanvas': 'createCanvas(width: int, height: int) -> object',
    'createServer': 'createServer(callback: func, port: int) -> void',
    'fillStyle': 'fillStyle(color: string) -> void',
    'strokeStyle': 'strokeStyle(color: string) -> void',
    'fillRect': 'fillRect(x: int, y: int, width: int, height: int) -> void',
    'strokeRect': 'strokeRect(x: int, y: int, width: int, height: int) -> void',
    'clearRect': 'clearRect(x: int, y: int, width: int, height: int) -> void',
    'fillCircle': 'fillCircle(x: int, y: int, radius: int) -> void',
    'render': 'render() -> void',
    'pollEvents': 'pollEvents() -> void',
    'updateInputs': 'updateInputs() -> void',
    'isKeyDown': 'isKeyDown(key: string) -> bool',
    'isMouseDown': 'isMouseDown() -> bool',
    'wasMouseClicked': 'wasMouseClicked() -> bool',
    'getMouseX': 'getMouseX() -> int',
    'getMouseY': 'getMouseY() -> int',
    'json_parse': 'json_parse(jsonString: string) -> object',
    'json_stringify': 'json_stringify(object: object) -> string',
    'drawImage': 'drawImage(image: object, x: int, y: int) -> void',
    'loadImage': 'loadImage(path: string) -> object',
    'createBox': 'createBox(width: float, height: float, depth: float) -> object',
    'createSphere': 'createSphere(radius: float, segments: int) -> object',
    'createCamera': 'createCamera(fov: float) -> object',
    'setPosition': 'setPosition(x: float, y: float, z: float) -> void',
    'setRotation': 'setRotation(x: float, y: float, z: float) -> void',
    'renderMesh': 'renderMesh(mesh: object, camera: object) -> void',
    'setLoop': 'setLoop(callback: func) -> void',
    'clear': 'clear() -> void'
};
function parseFile(text) {
    const symbols = [];
    const lines = text.split('\n');
    lines.forEach((line, i) => {
        const varMatch = line.match(/\b(var|const)\s+([a-zA-Z_]\w*)\s*:\s*([^=]+)/);
        if (varMatch) {
            symbols.push({ name: varMatch[2], type: varMatch[3].trim(), kind: varMatch[1], line: i });
        }
        const funcMatch = line.match(/\bfunc\s+([a-zA-Z_]\w*)\s*\([^)]*\)\s*->\s*(\S+)/);
        if (funcMatch) {
            symbols.push({ name: funcMatch[1], type: funcMatch[2], kind: 'func', line: i });
        }
        const typeMatch = line.match(/\btype\s+([A-Z]\w*)\s*=/);
        if (typeMatch) {
            symbols.push({ name: typeMatch[1], type: 'type', kind: 'type', line: i });
        }
    });
    return symbols;
}
function activate(context) {
    context.subscriptions.push(vscode.languages.registerCompletionItemProvider('axolotl', {
        provideCompletionItems(doc, pos) {
            const items = [];
            const symbols = parseFile(doc.getText());
            Object.keys(BUILTINS).forEach(fn => {
                const item = new vscode.CompletionItem(fn, vscode.CompletionItemKind.Function);
                item.detail = BUILTINS[fn];
                items.push(item);
            });
            symbols.forEach(s => {
                const kind = s.kind === 'func' ? vscode.CompletionItemKind.Function :
                    s.kind === 'type' ? vscode.CompletionItemKind.Class :
                        vscode.CompletionItemKind.Variable;
                const item = new vscode.CompletionItem(s.name, kind);
                item.detail = s.type;
                items.push(item);
            });
            return items;
        }
    }), vscode.languages.registerHoverProvider('axolotl', {
        provideHover(doc, pos) {
            const range = doc.getWordRangeAtPosition(pos);
            const word = doc.getText(range);
            if (BUILTINS[word]) {
                return new vscode.Hover(new vscode.MarkdownString(`\`\`\`axolotl\n${BUILTINS[word]}\n\`\`\``));
            }
            const symbols = parseFile(doc.getText());
            const sym = symbols.find(s => s.name === word);
            if (sym) {
                return new vscode.Hover(new vscode.MarkdownString(`\`\`\`axolotl\n${sym.name}: ${sym.type}\n\`\`\``));
            }
        }
    }));
}
exports.activate = activate;
function deactivate() { }
exports.deactivate = deactivate;
//# sourceMappingURL=extension.js.map