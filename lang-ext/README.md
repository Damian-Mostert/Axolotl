# Compiler Engine Language VS Code Extension

Syntax highlighting for `.lang` files (Compiler Engine Language).

## Installation

Since this is a local extension in the workspace, it's automatically available when you open the project.

## Features

- **Syntax Highlighting** for all language constructs
- **Comment Support** for `//` style comments
- **String Support** with escape sequences
- **Bracket Matching** and auto-closing
- **Proper Indentation** handling

## Colors

- **Blue**: Keywords (`if`, `while`, `for`, `func`, `var`, `const`, `return`)
- **Teal**: Types (`int`, `float`, `string`, `bool`, `void`)
- **Green**: Numbers (integers and floats)
- **Orange**: Strings
- **Gray**: Comments

## File Structure

```
lang-ext/
├── package.json                   # Extension metadata
├── language-configuration.json    # Language rules
├── syntaxes/
│   └── lang.tmLanguage.json      # Syntax definition
└── README.md                      # This file
```

## Development

To update the syntax highlighting:

1. Edit `syntaxes/lang.tmLanguage.json`
2. Reload VS Code (`Cmd+Shift+P` → "Developer: Reload Window")
3. Open a `.lang` file to see the changes

## License

MIT
