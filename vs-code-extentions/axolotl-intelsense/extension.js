const vscode = require('vscode');

const keywords = ['import','var', 'const', 'func', 'if', 'else', 'while', 'for', 'return', 'break', 'continue', 'switch', 'case', 'default', 'try', 'catch', 'finally', 'throw', 'import', 'export', 'use', 'type', 'program', 'await', 'typeof'];
const types = ['int', 'float', 'string', 'bool', 'void', 'any', 'object'];
const builtins = ['CreateWindow', 'print', 'len', 'push', 'pop', 'slice', 'reverse', 'join', 'find', 'includes', 'sort', 'toUpper', 'toLower', 'substr', 'indexOf', 'contains', 'trim', 'replace', 'split', 'startsWith', 'endsWith', 'repeat', 'charAt', 'charCodeAt', 'sin', 'cos', 'tan', 'sqrt', 'pow', 'exp', 'log', 'log10', 'random', 'toString','atan','atan2', 'toInt', 'toFloat', 'toBool', 'keys', 'values', 'hasKey', 'clone', 'merge', 'millis', 'sleep', 'assert', 'error', 'read', 'write', 'readDir', 'copy', 'createCanvas', 'loadImage', 'createScene', 'PerspectiveCamera', 'BoxGeometry', 'SphereGeometry', 'PlaneGeometry', 'TorusGeometry', 'CylinderGeometry', 'loadOBJ', 'PointLight', 'AmbientLight', 'DirectionalLight', 'setGraphics', 'enableDevMode', 'updateDevCamera', 'handleDevInput', 'isKeyDown', 'getMouseX', 'getMouseY', 'isMouseDown', 'wasMouseClicked', 'updateInputs', 'getKeyState', 'pollEvents', 'json_parse', 'json_stringify', 'fetch', 'createServer'];

function activate(context) {
    const provider = vscode.languages.registerCompletionItemProvider('axolotl', {
        provideCompletionItems(document, position) {
            const items = [];

            keywords.forEach(kw => {
                const item = new vscode.CompletionItem(kw, vscode.CompletionItemKind.Keyword);
                items.push(item);
            });

            types.forEach(t => {
                const item = new vscode.CompletionItem(t, vscode.CompletionItemKind.TypeParameter);
                items.push(item);
            });

            builtins.forEach(fn => {
                const item = new vscode.CompletionItem(fn, vscode.CompletionItemKind.Function);
                item.detail = 'Built-in function';
                items.push(item);
            });

            return items;
        }
    });

    context.subscriptions.push(provider);
}

function deactivate() { }

module.exports = { activate, deactivate };
