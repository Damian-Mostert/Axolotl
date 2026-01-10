const vscode = require('vscode');
const fs = require('fs');
const path = require('path');

const keywords = ['var', 'const', 'func', 'if', 'else', 'while', 'for', 'return', 'break', 'continue', 'switch', 'case', 'default', 'try', 'catch', 'finally', 'throw', 'import', 'export', 'use', 'type', 'program', 'await', 'typeof'];
const types = ['int', 'float', 'string', 'bool', 'void', 'any', 'object'];
const constants = [{ name: 'true', doc: 'Boolean true value' }, { name: 'false', doc: 'Boolean false value' }];

// Built-in function return types and parent types
const builtinReturnTypes = {
    print: 'void', len: 'int', push: 'void', pop: 'any', slice: 'array', reverse: 'void', join: 'string',
    find: 'int', includes: 'bool', sort: 'void', toUpper: 'string', toLower: 'string',
    substr: 'string', indexOf: 'int', contains: 'bool', trim: 'string', replace: 'string',
    split: '[string]', startsWith: 'bool', endsWith: 'bool', repeat: 'string', charAt: 'string',
    charCodeAt: 'int', random: 'float', pow: 'float', atan2: 'float', abs: 'float',
    floor: 'int', ceil: 'int', round: 'int', min: 'float', max: 'float', clamp: 'float',
    lerp: 'float', typeof: 'string', toInt: 'int', toFloat: 'float', toString: 'string',
    toBool: 'bool', keys: '[string]', values: '[any]', hasKey: 'bool', clone: 'any',
    merge: 'object', millis: 'int', sleep: 'void', assert: 'void', error: 'void',
    read: 'string', write: 'void', readDir: '[string]', copy: 'void', json_parse: 'object',
    json_stringify: 'string', fetch: 'string', createServer: 'object', createCanvas: 'object',
    fillRect: 'void', strokeRect: 'void', clearRect: 'void', fillCircle: 'void',
    drawLine: 'void', drawImage: 'void', loadImage: 'object', fillStyle: 'void',
    strokeStyle: 'void', isKeyDown: 'bool', getMouseX: 'int', getMouseY: 'int',
    isMouseDown: 'bool', wasMouseClicked: 'bool', pollEvents: 'void', updateInputs: 'void',
    getKeyState: 'object', createScene: 'object', render: 'void', renderWindow: 'void',
    PerspectiveCamera: 'object', BoxGeometry: 'object', SphereGeometry: 'object', PlaneGeometry: 'object',
    TorusGeometry: 'object', CylinderGeometry: 'object', loadOBJ: 'object',
    PointLight: 'object', DirectionalLight: 'object', AmbientLight: 'object',
    setPosition: 'void', setRotation: 'void', setScale: 'void', setColor: 'void',
    lookAt: 'void', moveBy: 'void', getPosition: 'object', applyGravity: 'void',
    handleCollision: 'void', isColliding: 'bool', setBounciness: 'void', setFriction: 'void',
    addVelocity: 'void', applyNaturalCollision: 'void', getGroundNormal: 'object',
    applyAttractionToMesh: 'void', deformVertices: 'void', followTarget: 'void',
    setGraphics: 'void', setMetallic: 'void', enableDevMode: 'void', updateDevCamera: 'void',
    handleDevInput: 'int', createBox: 'object', createCamera: 'object', renderMesh: 'void',
    close: 'void', add: 'void', CreateWindow: 'object', applyMTL: 'void', setTexture: 'void'
};

const builtinParentTypes = {};

function isInCommentOrString(text, position) {
    let inString = false;
    let stringChar = null;
    let inComment = false;
    
    for (let i = 0; i < position && i < text.length; i++) {
        if (!inString && !inComment && text[i] === '/' && text[i + 1] === '/') {
            inComment = true;
            break;
        }
        if (!inString && (text[i] === '"' || text[i] === "'")) {
            inString = true;
            stringChar = text[i];
        } else if (inString && text[i] === stringChar && text[i - 1] !== '\\') {
            inString = false;
            stringChar = null;
        }
    }
    return inString || inComment;
}

const MAX_RECURSION_DEPTH = 10;
const MAX_PARSE_LINES = 10000;
const MAX_EXPRESSION_LENGTH = 1000;
const MAX_PROPERTY_DEPTH = 5;

function parseDocument(document) {
    const variables = new Map();
    const functions = new Map();
    const customTypes = new Map();
    const properties = new Map();
    const objectProperties = new Map();
    
    const maxLines = Math.min(document.lineCount, MAX_PARSE_LINES);
    const fullText = document.getText();
    
    for (let i = 0; i < maxLines; i++) {
        try {
            const line = document.lineAt(i).text;
            if (line.trim().startsWith('//')) continue;
            if (line.length > MAX_EXPRESSION_LENGTH) continue;
            
            // Parse type declarations: type Name = ...
            const typeMatch = line.match(/^\s*type\s+(\w+)\s*=\s*(.+);?/);
            if (typeMatch) {
                customTypes.set(typeMatch[1], typeMatch[2].trim().replace(/;$/, ''));
                continue;
            }
            
            // Parse function declarations: func name(...) -> returnType
            const funcMatch = line.match(/^\s*func\s+(\w+)\s*\(([^)]*)\)\s*->\s*(\w+|\[[^\]]+\]|\{[^}]+\}|void|any)/);
            if (funcMatch) {
                const params = funcMatch[2].split(',').map(p => {
                    const pm = p.trim().match(/(\w+)\s*:\s*(.+)/);
                    if (pm) {
                        variables.set(pm[1], { type: pm[2].trim(), line: i, isConst: false, scope: 'param' });
                        return { name: pm[1], type: pm[2].trim() };
                    }
                    return null;
                }).filter(Boolean);
                functions.set(funcMatch[1], { returnType: funcMatch[3].trim(), params, line: i });
                continue;
            }
            
            // Parse variable declarations: var/const name: type = ... or var/const name = ...
            const varDeclMatch = line.match(/^\s*(var|const)\s+(\w+)\s*(?::\s*([^=]+))?\s*=/);
            if (varDeclMatch) {
                const varName = varDeclMatch[2];
                const declaredType = varDeclMatch[3] ? varDeclMatch[3].trim() : null;
                
                // Find the full initializer (may span multiple lines)
                const startPos = document.offsetAt(new vscode.Position(i, 0));
                const afterEquals = fullText.substring(startPos).match(/=\s*(.+)/);
                
                if (afterEquals) {
                    let initializer = '';
                    let braceCount = 0;
                    let inString = false;
                    let stringChar = null;
                    
                    for (let j = 0; j < afterEquals[1].length && j < 5000; j++) {
                        const char = afterEquals[1][j];
                        initializer += char;
                        
                        if (!inString) {
                            if (char === '"' || char === "'") {
                                inString = true;
                                stringChar = char;
                            } else if (char === '{') {
                                braceCount++;
                            } else if (char === '}') {
                                braceCount--;
                                if (braceCount === 0) break;
                            } else if (char === ';' && braceCount === 0) {
                                initializer = initializer.slice(0, -1);
                                break;
                            }
                        } else if (char === stringChar && afterEquals[1][j-1] !== '\\') {
                            inString = false;
                        }
                    }
                    
                    initializer = initializer.trim().replace(/;$/, '');
                    const inferredType = declaredType || inferExpressionType(initializer, variables, functions, customTypes, 0);
                    
                    variables.set(varName, {
                        type: inferredType,
                        line: i,
                        isConst: varDeclMatch[1] === 'const',
                        initializer,
                        inferred: !declaredType,
                        scope: 'global'
                    });
                    
                    // Extract object properties from initializer
                    if (initializer.includes('{')) {
                        const props = extractObjectProperties(initializer);
                        if (props.length > 0) {
                            objectProperties.set(varName, props);
                        }
                    }
                    
                    // Also extract from type definition if it's an object type
                    if (declaredType && declaredType.includes('{')) {
                        const typeProps = extractObjectProperties(declaredType);
                        if (typeProps.length > 0) {
                            const existing = objectProperties.get(varName) || [];
                            objectProperties.set(varName, [...new Set([...existing, ...typeProps])]);
                        }
                    }
                }
                continue;
            }
            
            // Track property assignments: obj.prop = value or obj.prop.subprop = value
            const propMatch = line.match(/^\s*([a-zA-Z_]\w*(?:\.[a-zA-Z_]\w*)*)\s*=\s*(.+);?/);
            if (propMatch && propMatch[1].includes('.') && !line.match(/^\s*(var|const|type|func)/)) {
                const propPath = propMatch[1];
                const parts = propPath.split('.');
                if (parts.length <= MAX_PROPERTY_DEPTH && variables.has(parts[0])) {
                    const value = propMatch[2].trim().replace(/;$/, '');
                    const inferredType = inferExpressionType(value, variables, functions, customTypes, 0);
                    properties.set(propPath, { type: inferredType, line: i, parent: parts[0] });
                }
            }
        } catch (err) {
            continue;
        }
    }
    
    return { variables, functions, customTypes, properties, objectProperties };
}

function extractObjectProperties(objLiteral) {
    const props = [];
    try {
        // Extract from object literal: { key: value, ... }
        const literalMatch = objLiteral.match(/\{([^}]+)\}/);
        if (literalMatch) {
            const content = literalMatch[1];
            // Split by comma but respect nested braces and strings
            let current = '';
            let depth = 0;
            let inString = false;
            let stringChar = null;
            
            for (let i = 0; i < content.length; i++) {
                const char = content[i];
                
                if (!inString) {
                    if (char === '"' || char === "'") {
                        inString = true;
                        stringChar = char;
                    } else if (char === '{') {
                        depth++;
                    } else if (char === '}') {
                        depth--;
                    } else if (char === ',' && depth === 0) {
                        const prop = current.trim().match(/^([a-zA-Z_]\w*)\s*:/);
                        if (prop) props.push(prop[1]);
                        current = '';
                        continue;
                    }
                } else if (char === stringChar && content[i-1] !== '\\') {
                    inString = false;
                }
                
                current += char;
            }
            
            // Handle last property
            if (current.trim()) {
                const prop = current.trim().match(/^([a-zA-Z_]\w*)\s*:/);
                if (prop) props.push(prop[1]);
            }
        }
        
        // Also extract from type definition: {key:type, ...}
        const typeMatch = objLiteral.match(/^\{([^}]+)\}$/);
        if (typeMatch && !literalMatch) {
            const pairs = typeMatch[1].split(',');
            for (const pair of pairs) {
                const match = pair.trim().match(/^([a-zA-Z_]\w*)\s*:/);
                if (match) props.push(match[1]);
            }
        }
    } catch (err) {
        // Ignore parse errors
    }
    return props;
}

function resolveType(type, customTypes, depth = 0) {
    if (depth > MAX_RECURSION_DEPTH) return 'any';
    if (customTypes.has(type)) {
        return resolveType(customTypes.get(type), customTypes, depth + 1);
    }
    return type;
}

function inferExpressionType(expr, variables, functions, customTypes, depth = 0) {
    if (depth > MAX_RECURSION_DEPTH) return 'any';
    if (!expr || expr.length > MAX_EXPRESSION_LENGTH) return 'any';
    
    expr = expr.trim();
    
    // String literal
    if (expr.startsWith('"') || expr.startsWith("'")) return 'string';
    
    // Number literal
    if (/^-?\d+$/.test(expr)) return 'int';
    if (/^-?\d+\.\d+$/.test(expr)) return 'float';
    
    // Boolean literal
    if (expr === 'true' || expr === 'false') return 'bool';
    
    // Array literal
    if (expr.startsWith('[')) {
        try {
            const match = expr.match(/^\[\s*(.+?)\s*\]$/);
            if (match && match[1]) {
                const elements = match[1].split(',').slice(0, 10).map(e => e.trim()).filter(Boolean);
                if (elements.length > 0) {
                    const firstType = inferExpressionType(elements[0], variables, functions, customTypes, depth + 1);
                    return `[${firstType}]`;
                }
            }
        } catch (err) {
            return '[any]';
        }
        return '[any]';
    }
    
    // Object literal
    if (expr.startsWith('{')) return 'object';
    
    // Function call
    const callMatch = expr.match(/^(\w+)\s*\(/);
    if (callMatch) {
        const fnName = callMatch[1];
        // Check for pseudo-type first
        if (pseudoTypes.has(fnName)) return pseudoTypes.get(fnName);
        if (builtinReturnTypes[fnName]) return builtinReturnTypes[fnName];
        if (functions.has(fnName)) return functions.get(fnName).returnType;
        return 'any';
    }
    
    // Method call (e.g., obj.method())
    const methodMatch = expr.match(/^(\w+)\.(\w+)\s*\(/);
    if (methodMatch) {
        const methodName = methodMatch[2];
        if (builtinReturnTypes[methodName]) return builtinReturnTypes[methodName];
        return 'any';
    }
    
    // Field access (e.g., obj.field or obj.field.subfield)
    const fieldMatch = expr.match(/^([a-zA-Z_]\w*(?:\.[a-zA-Z_]\w*)*)$/);
    if (fieldMatch && fieldMatch[1].includes('.')) {
        const parts = fieldMatch[1].split('.');
        if (parts.length <= MAX_PROPERTY_DEPTH && variables.has(parts[0])) {
            return 'any';
        }
    }
    
    // Binary operations
    if (expr.includes('+') || expr.includes('-') || expr.includes('*') || expr.includes('/')) {
        if (expr.includes('.')) return 'float';
        return 'int';
    }
    
    // Comparison operations
    if (expr.includes('==') || expr.includes('!=') || expr.includes('<') || expr.includes('>') || 
        expr.includes('<=') || expr.includes('>=') || expr.includes('&&') || expr.includes('||')) {
        return 'bool';
    }
    
    // Variable reference
    if (variables.has(expr)) {
        return resolveType(variables.get(expr).type, customTypes, depth + 1);
    }
    
    return 'any';
}

// Load builtins from generated JSON
let builtins = [];
const pseudoTypes = new Map(); // Map function names to their pseudo-types

try {
    const builtinsPath = path.join(__dirname, 'builtins.json');
    if (fs.existsSync(builtinsPath)) {
        const data = JSON.parse(fs.readFileSync(builtinsPath, 'utf8'));
        for (const [category, funcs] of Object.entries(data)) {
            funcs.forEach(fn => {
                builtins.push({
                    name: fn.name,
                    sig: fn.signature,
                    doc: fn.description || `[${category}] ${fn.signature}`,
                    category: category,
                    returnType: fn.returnType || 'void',
                    parent: fn.parent || ''
                });
                // Update builtinReturnTypes from JSON
                if (fn.returnType) {
                    builtinReturnTypes[fn.name] = fn.returnType;
                }
                // Update builtinParentTypes from JSON parent field
                if (fn.parent) {
                    builtinParentTypes[fn.name] = fn.parent;
                }
                // Track pseudo-types: functions that return specific object types
                if (fn.returnType === 'object' || !fn.returnType) {
                    if (fn.name === 'createScene') pseudoTypes.set(fn.name, 'scene');
                    else if (fn.name === 'createCanvas') pseudoTypes.set(fn.name, 'canvas');
                    else if (fn.name === 'PerspectiveCamera') pseudoTypes.set(fn.name, 'camera');
                    else if (fn.name === 'BoxGeometry' || fn.name === 'SphereGeometry' || 
                             fn.name === 'PlaneGeometry' || fn.name === 'TorusGeometry' || 
                             fn.name === 'CylinderGeometry' || fn.name === 'loadOBJ') {
                        pseudoTypes.set(fn.name, 'mesh');
                    }
                    else if (fn.name === 'PointLight' || fn.name === 'DirectionalLight' || fn.name === 'AmbientLight') {
                        pseudoTypes.set(fn.name, 'light');
                    }
                }
            });
        }
    }
    
    // Load parent types
    const parentsPath = path.join(__dirname, 'builtins_parents.json');
    if (fs.existsSync(parentsPath)) {
        const parentData = JSON.parse(fs.readFileSync(parentsPath, 'utf8'));
        for (const [name, parent] of Object.entries(parentData)) {
            builtinParentTypes[name] = parent;
        }
    }
} catch (err) {
    console.error('Failed to load builtins.json:', err);
}

function activate(context) {
    const completionProvider = vscode.languages.registerCompletionItemProvider('axolotl', {
        provideCompletionItems(document, position) {
            const items = [];
            const { variables, functions, customTypes } = parseDocument(document);
            
            keywords.forEach(kw => items.push(new vscode.CompletionItem(kw, vscode.CompletionItemKind.Keyword)));
            types.forEach(t => items.push(new vscode.CompletionItem(t, vscode.CompletionItemKind.TypeParameter)));
            
            // Add custom types
            customTypes.forEach((def, name) => {
                const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.TypeParameter);
                item.detail = def;
                items.push(item);
            });
            
            constants.forEach(c => {
                const item = new vscode.CompletionItem(c.name, vscode.CompletionItemKind.Constant);
                item.documentation = c.doc;
                items.push(item);
            });
            
            // Add local variables
            variables.forEach((info, name) => {
                const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Variable);
                item.detail = info.type;
                if (info.inferred) {
                    item.documentation = `${info.isConst ? 'const' : 'var'} ${name} = ${info.initializer || '...'} (inferred: ${info.type})`;
                } else {
                    item.documentation = `${info.isConst ? 'const' : 'var'} ${name}: ${info.type}`;
                }
                items.push(item);
            });
            
            // Add local functions
            functions.forEach((info, name) => {
                const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Function);
                const params = info.params.map(p => `${p.name}: ${p.type}`).join(', ');
                item.detail = `func ${name}(${params}) -> ${info.returnType}`;
                items.push(item);
            });
            
            builtins.forEach(fn => {
                // Only show global functions (no parent)
                if (!fn.parent) {
                    const item = new vscode.CompletionItem(fn.name, vscode.CompletionItemKind.Function);
                    item.detail = fn.sig;
                    item.documentation = fn.doc;
                    items.push(item);
                }
            });
            return items;
        }
    });

    const hoverProvider = vscode.languages.registerHoverProvider('axolotl', {
        provideHover(document, position) {
            const range = document.getWordRangeAtPosition(position);
            if (!range) return;
            const word = document.getText(range);
            const { variables, functions, customTypes } = parseDocument(document);
            
            // Check property access: obj.prop or obj.prop.subprop
            const fullText = document.lineAt(position.line).text;
            const beforeCursor = fullText.substring(0, position.character);
            const propMatch = beforeCursor.match(/([a-zA-Z_]\w*(?:\.[a-zA-Z_]\w*)*)$/);            
            if (propMatch) {
                const propPath = propMatch[1];
                const parts = propPath.split('.');
                
                if (parts.length > 1 && parts.length <= MAX_PROPERTY_DEPTH) {
                    const { variables, properties } = parseDocument(document);
                    const rootVar = parts[0];
                    
                    if (variables.has(rootVar)) {
                        const md = new vscode.MarkdownString();
                        md.appendCodeblock(propPath, 'axolotl');
                        
                        if (properties.has(propPath)) {
                            const propInfo = properties.get(propPath);
                            md.appendMarkdown(`\n\n**Type:** \`${propInfo.type}\``);
                        } else {
                            md.appendMarkdown(`\n\n**Type:** \`any\` (property of \`${rootVar}\`)`);
                        }
                        
                        return new vscode.Hover(md);
                    }
                }
            }
            
            // Check local variables
            if (variables.has(word)) {
                const info = variables.get(word);
                const resolvedType = resolveType(info.type, customTypes);
                const md = new vscode.MarkdownString();
                
                if (info.inferred) {
                    md.appendCodeblock(`${info.isConst ? 'const' : 'var'} ${word} = ${info.initializer || '...'}`, 'axolotl');
                    md.appendMarkdown(`\n\n**Inferred type:** \`${info.type}\``);
                } else {
                    md.appendCodeblock(`${info.isConst ? 'const' : 'var'} ${word}: ${info.type}`, 'axolotl');
                }
                
                if (resolvedType !== info.type) {
                    md.appendMarkdown(`\n\n**Resolved type:** \`${resolvedType}\``);
                }
                
                if (info.scope) {
                    md.appendMarkdown(`\n\n*Scope: ${info.scope}*`);
                }
                
                return new vscode.Hover(md);
            }
            
            // Check local functions
            if (functions.has(word)) {
                const info = functions.get(word);
                const params = info.params.map(p => `${p.name}: ${p.type}`).join(', ');
                const md = new vscode.MarkdownString();
                md.appendCodeblock(`func ${word}(${params}) -> ${info.returnType}`, 'axolotl');
                return new vscode.Hover(md);
            }
            
            // Check custom types
            if (customTypes.has(word)) {
                const def = customTypes.get(word);
                const md = new vscode.MarkdownString();
                md.appendCodeblock(`type ${word} = ${def}`, 'axolotl');
                return new vscode.Hover(md);
            }
            
            // Check builtins
            const fn = builtins.find(b => b.name === word);
            if (fn) {
                const md = new vscode.MarkdownString();
                md.appendCodeblock(fn.sig, 'axolotl');
                const returnType = builtinReturnTypes[word];
                if (returnType) {
                    md.appendMarkdown(`\n\n**Returns:** \`${returnType}\`\n\n`);
                }
                md.appendMarkdown(fn.doc);
                return new vscode.Hover(md);
            }
        }
    });

    const formatter = vscode.languages.registerDocumentFormattingEditProvider('axolotl', {
        provideDocumentFormattingEdits(document) {
            const edits = [];
            let formatted = '';
            let indent = 0;
            for (let i = 0; i < document.lineCount; i++) {
                const line = document.lineAt(i);
                let text = line.text.trim();
                if (!text) { formatted += '\\n'; continue; }
                if (text.startsWith('}') || text.startsWith(']')) indent = Math.max(0, indent - 1);
                formatted += '    '.repeat(indent) + text + '\\n';
                if (text.endsWith('{') || text.endsWith('[')) indent++;
                if (text.includes('}') && !text.startsWith('}')) indent = Math.max(0, indent - 1);
            }
            const fullRange = new vscode.Range(document.positionAt(0), document.positionAt(document.getText().length));
            edits.push(vscode.TextEdit.replace(fullRange, formatted.trimEnd()));
            return edits;
        }
    });

    const builtinBrowser = vscode.commands.registerCommand('axolotl.browseBuiltins', async () => {
        const functions = builtins.map(fn => ({ label: fn.name, description: fn.sig, detail: fn.doc, fn: fn }));
        const selectedFn = await vscode.window.showQuickPick(functions, { placeHolder: 'Select a builtin function', matchOnDescription: true });
        if (selectedFn) {
            const editor = vscode.window.activeTextEditor;
            if (editor) {
                editor.edit(editBuilder => editBuilder.insert(editor.selection.active, selectedFn.fn.name + '()'));
                const pos = editor.selection.active;
                editor.selection = new vscode.Selection(pos.line, pos.character - 1, pos.line, pos.character - 1);
            }
        }
    });

    const diagnosticCollection = vscode.languages.createDiagnosticCollection('axolotl');
    let diagnosticTimeout = null;
    const DEBOUNCE_DELAY = 500;
    
    function updateDiagnostics(document) {
        if (document.languageId !== 'axolotl') return;
        
        const diagnostics = [];
        let parseResult;
        
        try {
            parseResult = parseDocument(document);
        } catch (err) {
            console.error('Parse error:', err);
            return;
        }
        
        const { variables, functions, customTypes, properties } = parseResult;
        const usedVariables = new Set();
        const maxLines = Math.min(document.lineCount, MAX_PARSE_LINES);
        
        for (let i = 0; i < maxLines; i++) {
            try {
                const line = document.lineAt(i);
                const text = line.text;
                if (text.length > MAX_EXPRESSION_LENGTH) continue;
                if (text.trim().startsWith('//')) continue;
                
                // Check for invalid object literal syntax (e.g., x:quality: instead of quality:)
                const objLiteralMatch = text.match(/\{[^}]*\}/);
                if (objLiteralMatch) {
                    const objContent = objLiteralMatch[0];
                    // Check for pattern like "word:word:" which is invalid
                    if (/\w+:\w+:/.test(objContent)) {
                        const diagnostic = new vscode.Diagnostic(
                            line.range,
                            'Invalid object literal syntax - unexpected colon',
                            vscode.DiagnosticSeverity.Error
                        );
                        diagnostics.push(diagnostic);
                    }
                }
                
                // Check for accessing undefined nested properties (e.g., x.y.ss.s when x doesn't have y)
                const nestedAccessMatch = text.match(/^\s*([a-zA-Z_]\w*)(\.\w+){2,}/);
                if (nestedAccessMatch && !text.includes('=')) {
                    const rootVar = nestedAccessMatch[1];
                    if (variables.has(rootVar)) {
                        const diagnostic = new vscode.Diagnostic(
                            line.range,
                            `Accessing undefined nested properties on '${rootVar}'`,
                            vscode.DiagnosticSeverity.Warning
                        );
                        diagnostics.push(diagnostic);
                    }
                }
                
                // Check for undefined variables in expressions
                const identifiers = text.match(/\b[a-zA-Z_]\w*\b/g);
                if (identifiers) {
                    identifiers.slice(0, 50).forEach(id => {
                        if (variables.has(id)) {
                            usedVariables.add(id);
                        }
                    });
                }
                
                // Track property access usage: obj.prop or obj.prop.subprop
                const propAccessMatch = text.match(/\b([a-zA-Z_]\w*(?:\.[a-zA-Z_]\w*)+)\b/g);
                if (propAccessMatch) {
                    propAccessMatch.slice(0, 20).forEach(propPath => {
                        const parts = propPath.split('.');
                        if (parts.length <= MAX_PROPERTY_DEPTH && variables.has(parts[0])) {
                            usedVariables.add(parts[0]);
                        }
                    });
                }
                
                // Check variable/property assignments: name = expr or obj.prop = expr
                const assignMatch = text.match(/^\s*([a-zA-Z_]\w*(?:\.[a-zA-Z_]\w*)*)\s*=\s*(.+);?/);
                if (assignMatch && !text.match(/^\s*(var|const|type|func)/)) {
                    const target = assignMatch[1];
                    const expr = assignMatch[2].trim().replace(/;$/, '');
                    
                    if (expr.length > MAX_EXPRESSION_LENGTH) continue;
                    
                    // Property assignment: obj.prop or obj.prop.subprop
                    if (target.includes('.')) {
                        const parts = target.split('.');
                        if (parts.length > MAX_PROPERTY_DEPTH) continue;
                        
                        const rootVar = parts[0];
                        if (!variables.has(rootVar)) {
                            const diagnostic = new vscode.Diagnostic(
                                line.range,
                                `Variable '${rootVar}' is not declared`,
                                vscode.DiagnosticSeverity.Error
                            );
                            diagnostics.push(diagnostic);
                        } else {
                            const rootInfo = variables.get(rootVar);
                            if (rootInfo.isConst) {
                                const diagnostic = new vscode.Diagnostic(
                                    line.range,
                                    `Cannot modify property of const variable '${rootVar}'`,
                                    vscode.DiagnosticSeverity.Warning
                                );
                                diagnostics.push(diagnostic);
                            }
                        }
                    } else {
                        // Simple variable assignment
                        if (!variables.has(target)) {
                            const diagnostic = new vscode.Diagnostic(
                                line.range,
                                `Variable '${target}' is not declared`,
                                vscode.DiagnosticSeverity.Error
                            );
                            diagnostics.push(diagnostic);
                        } else {
                            const varInfo = variables.get(target);
                            
                            if (varInfo.isConst) {
                                const diagnostic = new vscode.Diagnostic(
                                    line.range,
                                    `Cannot reassign to const variable '${target}'`,
                                    vscode.DiagnosticSeverity.Error
                                );
                                diagnostics.push(diagnostic);
                            }
                            
                            const exprType = inferExpressionType(expr, variables, functions, customTypes, 0);
                            const varType = resolveType(varInfo.type, customTypes, 0);
                            
                            if (exprType !== 'any' && varType !== 'any' && exprType !== varType) {
                                if (!(varType === 'float' && exprType === 'int')) {
                                    const isArrayCompat = varType.startsWith('[') && exprType.startsWith('[');
                                    if (!isArrayCompat) {
                                        const diagnostic = new vscode.Diagnostic(
                                            line.range,
                                            `Type mismatch: cannot assign '${exprType}' to '${varType}'`,
                                            vscode.DiagnosticSeverity.Warning
                                        );
                                        diagnostics.push(diagnostic);
                                    }
                                }
                            }
                        }
                    }
                }
                
                // Check for missing semicolons (optional warning)
                if (text.trim() && !text.trim().startsWith('//') && !text.trim().startsWith('/*') &&
                    !text.trim().endsWith(';') && !text.trim().endsWith('{') && !text.trim().endsWith('}') &&
                    text.match(/^\s*(var|const|return|throw|break|continue)\s+/)) {
                    const diagnostic = new vscode.Diagnostic(
                        new vscode.Range(i, text.length, i, text.length),
                        'Consider adding a semicolon',
                        vscode.DiagnosticSeverity.Information
                    );
                    diagnostics.push(diagnostic);
                }
                
                // Check for undefined function calls (but not arguments inside function calls)
                const funcCallMatch = text.match(/\b(\w+)\s*\(/g);
                if (funcCallMatch) {
                    funcCallMatch.slice(0, 20).forEach(match => {
                        const fnName = match.replace(/\s*\($/, '');
                        const matchIndex = text.indexOf(match);
                        const beforeMatch = text.substring(0, matchIndex);
                        
                        // Check if this is a method call (preceded by dot)
                        if (beforeMatch.trimEnd().endsWith('.')) {
                            // Validate method exists for the object type
                            const objMatch = beforeMatch.match(/([a-zA-Z_]\w*)\s*\.\s*$/);
                            if (objMatch && variables.has(objMatch[1])) {
                                const varInfo = variables.get(objMatch[1]);
                                let resolvedType = varInfo.type;
                                if (varInfo.initializer) {
                                    const initMatch = varInfo.initializer.match(/^(\w+)\s*\(/);
                                    if (initMatch && pseudoTypes.has(initMatch[1])) {
                                        resolvedType = pseudoTypes.get(initMatch[1]);
                                    }
                                }
                                // Check if method exists for this type
                                const methodExists = builtins.some(fn => {
                                    if (fn.name !== fnName || !fn.parent) return false;
                                    const parents = fn.parent.split(',').map(p => p.trim());
                                    return parents.some(p => {
                                        if (p === resolvedType || p === varInfo.type) return true;
                                        if (pseudoTypes.has(p) && pseudoTypes.get(p) === resolvedType) return true;
                                        return false;
                                    });
                                });
                                if (!methodExists) {
                                    const diagnostic = new vscode.Diagnostic(
                                        line.range,
                                        `Method '${fnName}' does not exist on type '${resolvedType}'`,
                                        vscode.DiagnosticSeverity.Warning
                                    );
                                    diagnostics.push(diagnostic);
                                }
                            }
                            return;
                        }
                        
                        // Skip if this identifier is inside parentheses (it's an argument)
                        const openParens = (beforeMatch.match(/\(/g) || []).length;
                        const closeParens = (beforeMatch.match(/\)/g) || []).length;
                        const isInsideCall = openParens > closeParens;
                        
                        if (!isInsideCall && !functions.has(fnName) && !builtinReturnTypes[fnName] && 
                            !keywords.includes(fnName) && fnName !== 'if' && fnName !== 'while' && fnName !== 'for') {
                            const diagnostic = new vscode.Diagnostic(
                                line.range,
                                `Function '${fnName}' is not defined`,
                                vscode.DiagnosticSeverity.Warning
                            );
                            diagnostics.push(diagnostic);
                        }
                    });
                }
                
                if (diagnostics.length > 100) break;
            } catch (err) {
                continue;
            }
        }
        
        // Check for unused variables (info level)
        let unusedCount = 0;
        variables.forEach((info, name) => {
            if (unusedCount > 50) return;
            if (!usedVariables.has(name) && info.scope === 'global') {
                const diagnostic = new vscode.Diagnostic(
                    new vscode.Range(info.line, 0, info.line, 100),
                    `Variable '${name}' is declared but never used`,
                    vscode.DiagnosticSeverity.Hint
                );
                diagnostics.push(diagnostic);
                unusedCount++;
            }
        });
        
        diagnosticCollection.set(document.uri, diagnostics);
    }
    
    // Update diagnostics on document change with debouncing
    context.subscriptions.push(
        vscode.workspace.onDidChangeTextDocument(e => {
            if (diagnosticTimeout) clearTimeout(diagnosticTimeout);
            diagnosticTimeout = setTimeout(() => updateDiagnostics(e.document), DEBOUNCE_DELAY);
        }),
        vscode.workspace.onDidOpenTextDocument(doc => updateDiagnostics(doc)),
        diagnosticCollection
    );
    
    // Initial diagnostics for open documents
    vscode.workspace.textDocuments.forEach(doc => updateDiagnostics(doc));
    
    // Dot completion provider for methods and properties
    const dotCompletionProvider = vscode.languages.registerCompletionItemProvider('axolotl', {
        provideCompletionItems(document, position) {
            const items = [];
            const line = document.lineAt(position.line).text;
            const beforeCursor = line.substring(0, position.character);
            
            // Check if we're after a dot: obj.
            const dotMatch = beforeCursor.match(/([a-zA-Z_]\w*)\.$/);
            if (!dotMatch) return items;
            
            const objName = dotMatch[1];
            const { variables, objectProperties } = parseDocument(document);
            
            // Check if it's a known variable
            if (variables.has(objName)) {
                // Add object literal properties
                if (objectProperties.has(objName)) {
                    const props = objectProperties.get(objName);
                    props.forEach(prop => {
                        const item = new vscode.CompletionItem(prop, vscode.CompletionItemKind.Property);
                        item.detail = `property of ${objName}`;
                        items.push(item);
                    });
                }
                const varInfo = variables.get(objName);
                const varType = varInfo.type;
                
                // Resolve pseudo-type if variable was initialized with a builtin constructor
                let resolvedType = varType;
                if (varInfo.initializer) {
                    const initMatch = varInfo.initializer.match(/^(\w+)\s*\(/);
                    if (initMatch && pseudoTypes.has(initMatch[1])) {
                        resolvedType = pseudoTypes.get(initMatch[1]);
                    }
                }
                
                // Add methods that match this object type or pseudo-type
                const addedMethods = new Set();
                builtins.forEach(fn => {
                    if (fn.parent && !addedMethods.has(fn.name)) {
                        const parents = fn.parent.split(',').map(p => p.trim());
                        // Check if parent matches resolved type, varType, or if parent is a pseudo-type that resolves to same type
                        const matchesType = parents.some(p => {
                            if (p === resolvedType || p === varType) return true;
                            // Check if parent is a pseudo-type constructor that resolves to our type
                            if (pseudoTypes.has(p) && pseudoTypes.get(p) === resolvedType) return true;
                            return false;
                        });
                        if (matchesType) {
                            const item = new vscode.CompletionItem(fn.name, vscode.CompletionItemKind.Method);
                            item.detail = fn.sig;
                            item.documentation = fn.doc;
                            items.push(item);
                            addedMethods.add(fn.name);
                        }
                    }
                });
                
                // Add common object methods only if no specific type matched
                if ((resolvedType === 'object' || varType === 'object' || varType === 'any') && addedMethods.size === 0) {
                    ['keys', 'values', 'hasKey', 'clone', 'merge'].forEach(method => {
                        if (builtinReturnTypes[method]) {
                            const fn = builtins.find(b => b.name === method);
                            if (fn) {
                                const item = new vscode.CompletionItem(method, vscode.CompletionItemKind.Method);
                                item.detail = fn.sig;
                                item.documentation = fn.doc;
                                items.push(item);
                            }
                        }
                    });
                }
                
                // Add array methods
                if (varType.startsWith('[')) {
                    ['push', 'pop', 'slice', 'reverse', 'join', 'find', 'includes', 'sort', 'len'].forEach(method => {
                        const fn = builtins.find(b => b.name === method);
                        if (fn) {
                            const item = new vscode.CompletionItem(method, vscode.CompletionItemKind.Method);
                            item.detail = fn.sig;
                            item.documentation = fn.doc;
                            items.push(item);
                        }
                    });
                }
                
                // Add string methods
                if (varType === 'string') {
                    ['toUpper', 'toLower', 'substr', 'indexOf', 'contains', 'trim', 'replace', 'split', 
                     'startsWith', 'endsWith', 'repeat', 'charAt', 'charCodeAt', 'len'].forEach(method => {
                        const fn = builtins.find(b => b.name === method);
                        if (fn) {
                            const item = new vscode.CompletionItem(method, vscode.CompletionItemKind.Method);
                            item.detail = fn.sig;
                            item.documentation = fn.doc;
                            items.push(item);
                        }
                    });
                }
            }
            
            return items;
        }
    }, '.');
    
    context.subscriptions.push(completionProvider, dotCompletionProvider, hoverProvider, formatter, builtinBrowser);
}

function deactivate() {}

module.exports = { activate, deactivate };
