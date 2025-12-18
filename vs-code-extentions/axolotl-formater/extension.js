const vscode = require('vscode');

function activate(context) {
    const formatter = vscode.languages.registerDocumentFormattingEditProvider('axolotl', {
        provideDocumentFormattingEdits(document) {
            const edits = [];
            let formatted = '';
            let indent = 0;
            
            for (let i = 0; i < document.lineCount; i++) {
                const line = document.lineAt(i);
                let text = line.text.trim();
                
                if (!text) {
                    formatted += '\n';
                    continue;
                }
                
                if (text.startsWith('}') || text.startsWith(']')) indent = Math.max(0, indent - 1);
                
                formatted += '    '.repeat(indent) + text + '\n';
                
                if (text.endsWith('{') || text.endsWith('[')) indent++;
                if (text.includes('}') && !text.startsWith('}')) indent = Math.max(0, indent - 1);
            }
            
            const fullRange = new vscode.Range(
                document.positionAt(0),
                document.positionAt(document.getText().length)
            );
            
            edits.push(vscode.TextEdit.replace(fullRange, formatted.trimEnd()));
            return edits;
        }
    });
    
    context.subscriptions.push(formatter);
}

function deactivate() {}

module.exports = { activate, deactivate };
