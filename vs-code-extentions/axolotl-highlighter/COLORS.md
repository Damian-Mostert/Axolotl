# Axolotl Color Configuration

To enable pink colors for types and custom colors for built-in functions, add this to your VS Code `settings.json`:

```json
"editor.tokenColorCustomizations": {
  "textMateRules": [
    {
      "scope": ["storage.type.primitive.axolotl", "entity.name.type.axolotl"],
      "settings": {
        "foreground": "#ff69b4",
        "fontStyle": "bold"
      }
    },
    {
      "scope": "support.function.builtin.axolotl",
      "settings": {
        "foreground": "#4ec9b0",
        "fontStyle": "italic"
      }
    }
  ]
}
```

Or use the workspace settings by creating `.vscode/settings.json` in your project.
