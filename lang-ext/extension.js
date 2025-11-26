const vscode = require('vscode');
const path = require('path');
const fs = require('fs');

class AxolotlLanguageProvider {
    constructor() {
        this.symbols = new Map();
        this.functions = new Map();
        this.types = new Map();
        this.diagnostics = vscode.languages.createDiagnosticCollection('axolotl');
        this.initializeBuiltins();
    }

    initializeBuiltins() {
        // Built-in types
        const builtinTypes = [
            'int', 'float', 'string', 'bool', 'void', 'any', 'object'
        ];
        
        builtinTypes.forEach(type => {
            this.types.set(type, {
                name: type,
                definition: type,
            });
        });
        
        // Add func as a special type (function type)
        this.types.set('func', {
            name: 'func',
            definition: 'function type',
        });

        // Built-in functions with comprehensive documentation
        const builtinFunctions = [
            {
                name: 'print',
                params: [{name: 'args', type: 'any...'}],
                returnType: 'void',
                documentation: 'Print values to console with automatic spacing and newline'
            },
            {
                name: 'len',
                params: [{name: 'value', type: 'string|[any]'}],
                returnType: 'int',
                documentation: 'Get length of string or array'
            },
            {
                name: 'push',
                params: [{name: 'array', type: '[any]'}, {name: 'value', type: 'any'}],
                returnType: 'void',
                documentation: 'Add element to end of array (modifies original array)'
            },
            {
                name: 'pop',
                params: [{name: 'array', type: '[any]'}],
                returnType: 'any',
                documentation: 'Remove and return last element from array'
            },
            {
                name: 'substr',
                params: [{name: 'str', type: 'string'}, {name: 'start', type: 'int'}, {name: 'length', type: 'int'}],
                returnType: 'string',
                documentation: 'Extract substring starting at index with specified length'
            },
            {
                name: 'toUpper',
                params: [{name: 'str', type: 'string'}],
                returnType: 'string',
                documentation: 'Convert string to uppercase'
            },
            {
                name: 'toLower',
                params: [{name: 'str', type: 'string'}],
                returnType: 'string',
                documentation: 'Convert string to lowercase'
            },
            {
                name: 'indexOf',
                params: [{name: 'str', type: 'string'}, {name: 'substring', type: 'string'}],
                returnType: 'int',
                documentation: 'Find index of substring (-1 if not found)'
            },
            {
                name: 'contains',
                params: [{name: 'str', type: 'string'}, {name: 'substring', type: 'string'}],
                returnType: 'bool',
                documentation: 'Check if string contains substring'
            },
            {
                name: 'millis',
                params: [],
                returnType: 'int',
                documentation: 'Get current time in milliseconds since epoch'
            },
            {
                name: 'sleep',
                params: [{name: 'ms', type: 'int'}],
                returnType: 'void',
                documentation: 'Sleep for specified milliseconds (blocks execution)'
            },
            {
                name: 'toString',
                params: [{name: 'value', type: 'any'}],
                returnType: 'string',
                documentation: 'Convert any value to its string representation'
            },
            {
                name: 'typeof',
                params: [{name: 'value', type: 'any'}],
                returnType: 'string',
                documentation: 'Get runtime type of value (int, float, string, bool, array, object, function)'
            },
            {
                name: 'read',
                params: [{name: 'filepath', type: 'string'}],
                returnType: 'string',
                documentation: 'Read entire file contents as string'
            },
            {
                name: 'write',
                params: [{name: 'filepath', type: 'string'}, {name: 'content', type: 'string'}],
                returnType: 'void',
                documentation: 'Write content to file (overwrites existing file)'
            },
            {
                name: 'copy',
                params: [{name: 'source', type: 'string'}, {name: 'dest', type: 'string'}],
                returnType: 'void',
                documentation: 'Copy file from source to destination path'
            }
        ];

        builtinFunctions.forEach(func => {
            this.functions.set(func.name, func);
        });
    }

    async provideCompletionItems(document, position, token, context) {
        const items = [];
        const line = document.lineAt(position).text;
        const linePrefix = line.substring(0, position.character);

        // Parse document for symbols
        await this.parseDocument(document);

        // Type completions after ':'
        if (linePrefix.includes(':') && !linePrefix.includes('->')) {
            const typeItems = this.getTypeCompletions();
            items.push(...typeItems);
        }

        // Function completions
        if (this.isInFunctionCallContext(linePrefix)) {
            const functionItems = this.getFunctionCompletions();
            items.push(...functionItems);
        }

        // Variable completions
        const variableItems = this.getVariableCompletions(document.uri.toString());
        items.push(...variableItems);

        // Keyword completions
        const keywordItems = this.getKeywordCompletions(linePrefix);
        items.push(...keywordItems);

        // Snippet completions
        const snippetItems = this.getSnippetCompletions();
        items.push(...snippetItems);

        return items;
    }

    getTypeCompletions() {
        const items = [];

        // Basic types
        this.types.forEach((type, name) => {
            const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.TypeParameter);
            item.detail = `Type: ${type.definition}`;
            item.documentation = new vscode.MarkdownString(`Built-in type \`${name}\``);
            items.push(item);
        });

        // Array type suggestions
        const arrayTypes = ['[int]', '[float]', '[string]', '[bool]', '[object]', '[any]', '[[int]]', '[[string]]'];
        arrayTypes.forEach(arrayType => {
            const item = new vscode.CompletionItem(arrayType, vscode.CompletionItemKind.TypeParameter);
            item.detail = `Array type`;
            item.documentation = new vscode.MarkdownString(`Array type: ${arrayType}`);
            items.push(item);
        });

        // Union type suggestions
        const unionTypes = [
            'string|int', 'int|float', 'bool|string', 'any|void', 
            'string|int|bool', 'int|float|string', '[int]|[string]',
            'object|string', 'func|string'
        ];
        unionTypes.forEach(unionType => {
            const item = new vscode.CompletionItem(unionType, vscode.CompletionItemKind.TypeParameter);
            item.detail = `Union type`;
            item.documentation = new vscode.MarkdownString(`Union type allowing: ${unionType.split('|').join(' or ')}`);
            items.push(item);
        });

        // Object type templates
        const objectTypes = [
            '{name: string, age: int}',
            '{id: int, active: bool}',
            '{data: [any], count: int}',
            '{x: int, y: int}',
            '{key: string, value: any}'
        ];
        objectTypes.forEach(objType => {
            const item = new vscode.CompletionItem(objType, vscode.CompletionItemKind.TypeParameter);
            item.detail = `Object type template`;
            item.documentation = new vscode.MarkdownString(`Object type: \`${objType}\``);
            items.push(item);
        });

        return items;
    }

    getFunctionCompletions() {
        const items = [];

        this.functions.forEach((func, name) => {
            const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Function);
            
            const paramStr = func.params.map(p => `${p.name}: ${p.type}`).join(', ');
            item.detail = `${name}(${paramStr}) -> ${func.returnType}`;
            
            const snippet = `${name}(${func.params.map((p, i) => `\${${i + 1}:${p.name}}`).join(', ')})`;
            item.insertText = new vscode.SnippetString(snippet);
            
            if (func.documentation) {
                const markdown = new vscode.MarkdownString();
                markdown.appendCodeblock(`${name}(${paramStr}) -> ${func.returnType}`, 'axolotl');
                markdown.appendMarkdown('\n\n' + func.documentation);
                item.documentation = markdown;
            }
            
            items.push(item);
        });

        return items;
    }

    getVariableCompletions(uri) {
        const items = [];
        const symbols = this.symbols.get(uri) || [];

        symbols.forEach(symbol => {
            const item = new vscode.CompletionItem(symbol.name, symbol.kind);
            item.detail = symbol.detail || symbol.type;
            if (symbol.documentation) {
                item.documentation = new vscode.MarkdownString(symbol.documentation);
            }
            items.push(item);
        });

        return items;
    }

    getKeywordCompletions(linePrefix) {
        const items = [];
        
        const keywords = [
            { name: 'func', detail: 'Function declaration', snippet: 'func ${1:name}(${2:params}) -> ${3:returnType} {\n\t${4}\n}' },
            { name: 'var', detail: 'Variable declaration', snippet: 'var ${1:name}: ${2:type} = ${3:value};' },
            { name: 'const', detail: 'Constant declaration', snippet: 'const ${1:name}: ${2:type} = ${3:value};' },
            { name: 'if', detail: 'If statement', snippet: 'if (${1:condition}) {\n\t${2}\n}' },
            { name: 'else', detail: 'Else clause', snippet: 'else {\n\t${1}\n}' },
            { name: 'while', detail: 'While loop', snippet: 'while (${1:condition}) {\n\t${2}\n}' },
            { name: 'for', detail: 'For loop', snippet: 'for (var ${1:i}: int = 0; ${1:i} < ${2:limit}; ${1:i} = ${1:i} + 1) {\n\t${3}\n}' },
            { name: 'switch', detail: 'Switch statement', snippet: 'switch (${1:expression}) {\n\tcase ${2:value}:\n\t\t${3}\n\t\tbreak;\n\tdefault:\n\t\t${4}\n}' },
            { name: 'case', detail: 'Case clause', snippet: 'case ${1:value}:\n\t${2}\n\tbreak;' },
            { name: 'default', detail: 'Default case', snippet: 'default:\n\t${1}' },
            { name: 'try', detail: 'Try-catch block', snippet: 'try {\n\t${1}\n} catch (${2:e}) {\n\t${3}\n}' },
            { name: 'catch', detail: 'Catch clause', snippet: 'catch (${1:e}) {\n\t${2}\n}' },
            { name: 'finally', detail: 'Finally clause', snippet: 'finally {\n\t${1}\n}' },
            { name: 'throw', detail: 'Throw statement', snippet: 'throw ${1:"error message"};' },
            { name: 'return', detail: 'Return statement', snippet: 'return ${1:value};' },
            { name: 'break', detail: 'Break statement', snippet: 'break;' },
            { name: 'continue', detail: 'Continue statement', snippet: 'continue;' },
            { name: 'import', detail: 'Import statement', snippet: 'import {${1:exports}} from "${2:module}";' },
            { name: 'export', detail: 'Export statement', snippet: 'export ${1:declaration};' },
            { name: 'use', detail: 'Use statement', snippet: 'use "${1:module}";' },
            { name: 'type', detail: 'Type declaration', snippet: 'type ${1:Name} = ${2:definition};' },
            { name: 'program', detail: 'Program declaration', snippet: 'program ${1:name}(${2:params}) {\n\t${3}\n}' },
            { name: 'await', detail: 'Await expression', snippet: 'await ${1:expression}' },
            { name: 'typeof', detail: 'Typeof operator', snippet: 'typeof(${1:value})' }
        ];

        keywords.forEach(keyword => {
            const item = new vscode.CompletionItem(keyword.name, vscode.CompletionItemKind.Keyword);
            item.detail = keyword.detail;
            if (keyword.snippet) {
                item.insertText = new vscode.SnippetString(keyword.snippet);
            }
            items.push(item);
        });

        return items;
    }

    getSnippetCompletions() {
        const items = [];

        const snippets = [
            {
                name: 'main',
                detail: 'Main function template',
                snippet: 'func main() -> void {\n\t${1}\n}\n\nmain();'
            },
            {
                name: 'class',
                detail: 'Class-like object type',
                snippet: 'type ${1:ClassName} = {\n\t${2:field}: ${3:type}\n};\n\nfunc create${1:ClassName}(${4:params}) -> ${1:ClassName} {\n\treturn {\n\t\t${2:field}: ${5:value}\n\t};\n}'
            },
            {
                name: 'test',
                detail: 'Test function template',
                snippet: 'func test${1:Name}() -> void {\n\tprint("Testing ${1:Name}");\n\t${2}\n\tprint("✓ ${1:Name} test passed");\n}'
            },
            {
                name: 'pipeline',
                detail: 'Data processing pipeline',
                snippet: 'func process${1:Name}(input: [${2:InputType}]) -> [${3:OutputType}] {\n\tvar result: [${3:OutputType}] = [];\n\t\n\tfor (var i: int = 0; i < len(input); i = i + 1) {\n\t\tvar item: ${2:InputType} = input[i];\n\t\t${4:// process item}\n\t\tpush(result, ${5:processedItem});\n\t}\n\t\n\treturn result;\n}'
            }
        ];

        snippets.forEach(snippet => {
            const item = new vscode.CompletionItem(snippet.name, vscode.CompletionItemKind.Snippet);
            item.detail = snippet.detail;
            item.insertText = new vscode.SnippetString(snippet.snippet);
            items.push(item);
        });

        return items;
    }

    isInFunctionCallContext(linePrefix) {
        const functionCallPattern = /\w+\s*\($/;
        return functionCallPattern.test(linePrefix);
    }

    async provideHover(document, position, token) {
        const line = document.lineAt(position.line).text;
        const char = position.character;
        
        // Check if hovering over a string literal
        const stringMatch = line.match(/"([^"]*)"/g);
        if (stringMatch) {
            for (const str of stringMatch) {
                const startIdx = line.indexOf(str);
                const endIdx = startIdx + str.length;
                if (char >= startIdx && char <= endIdx) {
                    const markdown = new vscode.MarkdownString();
                    markdown.appendCodeblock(`string: ${str}`, 'axolotl');
                    return new vscode.Hover(markdown, new vscode.Range(position.line, startIdx, position.line, endIdx));
                }
            }
        }
        
        // Check if hovering over a number literal
        const numberMatch = line.match(/\b(\d+\.\d+|\d+)\b/g);
        if (numberMatch) {
            for (const num of numberMatch) {
                const startIdx = line.indexOf(num);
                const endIdx = startIdx + num.length;
                if (char >= startIdx && char <= endIdx) {
                    const type = num.includes('.') ? 'float' : 'int';
                    const markdown = new vscode.MarkdownString();
                    markdown.appendCodeblock(`${type}: ${num}`, 'axolotl');
                    return new vscode.Hover(markdown, new vscode.Range(position.line, startIdx, position.line, endIdx));
                }
            }
        }
        
        const range = document.getWordRangeAtPosition(position);
        if (!range) return undefined;

        const word = document.getText(range);

        // Check built-in functions
        const func = this.functions.get(word);
        if (func) {
            const paramStr = func.params.map(p => `${p.name}: ${p.type}`).join(', ');
            const signature = `${func.name}(${paramStr}) -> ${func.returnType}`;
            
            const markdown = new vscode.MarkdownString();
            markdown.appendCodeblock(signature, 'axolotl');
            if (func.documentation) {
                markdown.appendMarkdown('\n\n' + func.documentation);
            }
            
            return new vscode.Hover(markdown, range);
        }

        // Check types
        const type = this.types.get(word);
        if (type) {
            const markdown = new vscode.MarkdownString();
            markdown.appendCodeblock(`type ${type.name} = ${type.definition}`, 'axolotl');
            return new vscode.Hover(markdown, range);
        }

        // Check variables in current document
        await this.parseDocument(document);
        const symbols = this.symbols.get(document.uri.toString()) || [];
        const symbol = symbols.find(s => s.name === word);
        if (symbol) {
            const markdown = new vscode.MarkdownString();
            markdown.appendCodeblock(`${symbol.name}: ${symbol.type}`, 'axolotl');
            if (symbol.documentation) {
                markdown.appendMarkdown('\n\n' + symbol.documentation);
            }
            return new vscode.Hover(markdown, range);
        }

        return undefined;
    }

    async parseDocument(document) {
        const text = document.getText();
        const uri = document.uri.toString();
        const symbols = [];
        const diagnostics = [];

        const lines = text.split('\n');
        
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];
            
            // Parse variable declarations (including for loop variables)
            const varMatches = line.matchAll(/\b(var|const)\s+(\w+)\s*:\s*([^=;,)]+?)(?:\s*[=;,)]|$)/g);
            for (const varMatch of varMatches) {
                const [, keyword, name, type] = varMatch;
                symbols.push({
                    name,
                    type: type.trim(),
                    kind: keyword === 'const' ? vscode.CompletionItemKind.Constant : vscode.CompletionItemKind.Variable,
                    detail: `${keyword} ${name}: ${type.trim()}`,
                    range: new vscode.Range(i, 0, i, line.length)
                });
            }

            // Parse function declarations and their parameters (named and inline)
            const funcMatches = line.matchAll(/func(?:\s+(\w+))?\s*\(([^)]*)\)\s*->\s*([\w\[\]|]+)/g);
            for (const funcMatch of funcMatches) {
                const [, name, params, returnType] = funcMatch;
                
                // Add named function to symbols
                if (name) {
                    symbols.push({
                        name,
                        type: 'function',
                        kind: vscode.CompletionItemKind.Function,
                        detail: `function ${name}() -> ${returnType}`,
                        range: new vscode.Range(i, 0, i, line.length)
                    });
                }
                
                // Parse function parameters as variables (for both named and inline functions)
                if (params.trim()) {
                    const paramList = params.split(',');
                    paramList.forEach(param => {
                        const paramMatch = param.trim().match(/(\w+)\s*:\s*([\w\[\]|]+)/);
                        if (paramMatch) {
                            const [, paramName, paramType] = paramMatch;
                            symbols.push({
                                name: paramName,
                                type: paramType.trim(),
                                kind: vscode.CompletionItemKind.Variable,
                                detail: `parameter ${paramName}: ${paramType.trim()}`,
                                range: new vscode.Range(i, 0, i, line.length)
                            });
                        }
                    });
                }
            }

            // Parse program declarations
            const progMatch = line.match(/^\s*program\s+(\w+)\s*\(([^)]*)\)/);
            if (progMatch) {
                const [, name, params] = progMatch;
                symbols.push({
                    name,
                    type: 'program',
                    kind: vscode.CompletionItemKind.Function,
                    detail: `program ${name}() (async, can be awaited)`,
                    range: new vscode.Range(i, 0, i, line.length)
                });
                
                // Parse program parameters as variables
                if (params.trim()) {
                    const paramList = params.split(',');
                    paramList.forEach(param => {
                        const paramMatch = param.trim().match(/(\w+)\s*:\s*([\w\[\]|]+)/);
                        if (paramMatch) {
                            const [, paramName, paramType] = paramMatch;
                            symbols.push({
                                name: paramName,
                                type: paramType.trim(),
                                kind: vscode.CompletionItemKind.Variable,
                                detail: `parameter ${paramName}: ${paramType.trim()}`,
                                range: new vscode.Range(i, 0, i, line.length)
                            });
                        }
                    });
                }
            }

            // Parse type declarations (multi-line support)
            const typeMatch = line.match(/^\s*type\s+(\w+)\s*=/);
            if (typeMatch) {
                const [, name] = typeMatch;
                let definition = line.substring(line.indexOf('=') + 1).trim();
                
                // Handle multi-line type definitions
                let j = i;
                while (j < lines.length && !definition.includes(';')) {
                    j++;
                    if (j < lines.length) {
                        definition += ' ' + lines[j].trim();
                    }
                }
                definition = definition.replace(/;$/, '').trim();
                
                this.types.set(name, { name, definition });
                symbols.push({
                    name,
                    type: 'type',
                    kind: vscode.CompletionItemKind.Class,
                    detail: `type ${name}`,
                    range: new vscode.Range(i, 0, i, line.length)
                });
            }

            // Parse import statements
            const importMatch = line.match(/^\s*import\s+(?:\{([^}]+)\}|([\w]+))\s+from\s+["']([^"']+)["']/);
            if (importMatch) {
                const [, namedImports, defaultImport] = importMatch;
                if (namedImports) {
                    // Named imports: import {x, y} from "module"
                    namedImports.split(',').forEach(name => {
                        const trimmedName = name.trim();
                        symbols.push({
                            name: trimmedName,
                            type: 'imported',
                            kind: vscode.CompletionItemKind.Variable,
                            detail: `imported ${trimmedName}`,
                            range: new vscode.Range(i, 0, i, line.length)
                        });
                    });
                }
                if (defaultImport) {
                    // Default import: import x from "module"
                    symbols.push({
                        name: defaultImport,
                        type: 'imported',
                        kind: vscode.CompletionItemKind.Variable,
                        detail: `imported ${defaultImport}`,
                        range: new vscode.Range(i, 0, i, line.length)
                    });
                }
            }

            // Enhanced error detection
            this.detectErrors(line, i, diagnostics, symbols);
        }

        this.symbols.set(uri, symbols);
        this.diagnostics.set(document.uri, diagnostics);
    }

    detectErrors(line, lineNumber, diagnostics, symbols) {
        // Skip lines that are comments
        if (line.trim().startsWith('//')) {
            return;
        }
        
        // Remove comments and string literals
        let cleanLine = line;
        const commentIndex = cleanLine.indexOf('//');
        if (commentIndex !== -1) {
            cleanLine = cleanLine.substring(0, commentIndex);
        }
        const lineWithoutStrings = cleanLine.replace(/"[^"]*"/g, '""');
        
        // Check for undefined variables (only outside strings)
        const identifierMatches = lineWithoutStrings.matchAll(/\b([a-zA-Z_]\w*)\b/g);
        for (const match of identifierMatches) {
            const identifier = match[1];
            const column = match.index || 0;
            
            // Skip keywords and built-ins
            if (this.isKeyword(identifier) || this.functions.has(identifier) || this.types.has(identifier)) {
                continue;
            }
            
            // Skip if it's an object field name (identifier followed by colon)
            if (lineWithoutStrings.substring(column).match(/^\w+\s*:/)) {
                continue;
            }
            
            // Skip if it's a property access (preceded by dot or closing bracket)
            if (column > 0) {
                const prevChar = lineWithoutStrings.substring(column - 1, column);
                if (prevChar === '.' || prevChar === ']') {
                    continue;
                }
            }
            
            // Check if identifier is defined
            const isDefined = symbols.some(s => s.name === identifier) || this.isBuiltinConstant(identifier);
            if (!isDefined) {
                const diagnostic = new vscode.Diagnostic(
                    new vscode.Range(lineNumber, column, lineNumber, column + identifier.length),
                    `Undefined variable: ${identifier}`,
                    vscode.DiagnosticSeverity.Warning
                );
                diagnostic.code = 'undefined-variable';
                diagnostic.source = 'axolotl';
                diagnostics.push(diagnostic);
            }
        }

    }

    isKeyword(word) {
        const keywords = [
            'func', 'var', 'const', 'if', 'else', 'while', 'for', 'return', 'break', 'continue',
            'switch', 'case', 'default', 'try', 'catch', 'finally', 'throw', 'import', 'export',
            'use', 'type', 'program', 'await', 'typeof', 'true', 'false', 'int', 'float',
            'string', 'bool', 'void', 'any', 'object'
        ];
        return keywords.includes(word);
    }

    isBuiltinConstant(word) {
        const constants = ['true', 'false', 'null'];
        return constants.includes(word);
    }

    dispose() {
        this.diagnostics.dispose();
    }
}

function activate(context) {
    const provider = new AxolotlLanguageProvider();

    // Register completion provider
    const completionProvider = vscode.languages.registerCompletionItemProvider(
        { scheme: 'file', language: 'axo' },
        provider,
        '.', ':', '(', '|', ' '
    );

    // Register hover provider
    const hoverProvider = vscode.languages.registerHoverProvider(
        { scheme: 'file', language: 'axo' },
        provider
    );

    // Register document change listener for diagnostics
    const documentChangeListener = vscode.workspace.onDidChangeTextDocument(async (event) => {
        if (event.document.languageId === 'axo') {
            await provider.parseDocument(event.document);
        }
    });

    // Register document open listener
    const documentOpenListener = vscode.workspace.onDidOpenTextDocument(async (document) => {
        if (document.languageId === 'axo') {
            await provider.parseDocument(document);
        }
    });

    // Register commands
    const compileCommand = vscode.commands.registerCommand('axolotl.compile', () => {
        const editor = vscode.window.activeTextEditor;
        if (editor && editor.document.languageId === 'axo') {
            const terminal = vscode.window.createTerminal('Axolotl Compiler');
            terminal.sendText(`./build/compiler "${editor.document.fileName}"`);
            terminal.show();
        }
    });

    const runCommand = vscode.commands.registerCommand('axolotl.run', () => {
        const editor = vscode.window.activeTextEditor;
        if (editor && editor.document.languageId === 'axo') {
            const terminal = vscode.window.createTerminal('Axolotl Runner');
            terminal.sendText(`./build/compiler "${editor.document.fileName}"`);
            terminal.show();
        }
    });

    context.subscriptions.push(
        completionProvider,
        hoverProvider,
        documentChangeListener,
        documentOpenListener,
        compileCommand,
        runCommand,
        provider
    );

    // Parse all open Axolotl documents
    vscode.workspace.textDocuments.forEach(async (document) => {
        if (document.languageId === 'axo') {
            await provider.parseDocument(document);
        }
    });
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
};