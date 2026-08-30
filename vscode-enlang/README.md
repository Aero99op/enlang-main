# EnLang — VS Code Extension
### *Official Syntax Highlighting, Colorization & Developer Tooling for EnLang*

[![Version](https://img.shields.io/badge/extension-v1.0.0-indigo.svg)](https://marketplace.visualstudio.com/)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Official Visual Studio Code extension for **EnLang** — The Universal Natural English Programming Language. Provides rich syntax highlighting, colorful token distinction, auto-closing brackets, code snippets, and one-click execution commands across all 5 EnLang extensions:

- **`.enlg`** ➔ Core Backend Logic & Algorithms (Python 3)
- **`.enlgf`** ➔ Structural Frontend Markup (HTML5)
- **`.enlgd`** ➔ Styling & Design Systems (CSS3)
- **`.enlgs`** ➔ Client Scripting & DOM Logic (JavaScript ES6+)
- **`.enlgdb`** ➔ Database Schemas & Queries (ANSI SQL / SQLite)

---

## 🎨 Colorization & Syntax Scopes

| Code Token | Visual Color Category | EnLang Keywords |
| :--- | :--- | :--- |
| **Control Flow** | Magenta / Pink / Purple | `if`, `then`, `else`, `while`, `for`, `repeat`, `match`, `return` |
| **Declarations** | Royal Blue / Cyan | `define`, `set`, `function`, `class`, `interface`, `import`, `export` |
| **Operators** | Orange / Coral | `plus`, `minus`, `times`, `divided by`, `is`, `is not`, `equals` |
| **Strings** | Vibrant Green | `"Hello World"`, `'A+'` |
| **Numbers** | Teal / Cyan | `100`, `3.14159`, `99.99` |
| **Functions** | Amber / Gold | `display`, `print`, `read`, `call` |
| **Comments** | Soft Grey / Italic | `# Single line comments`, `/* Block comments */` |
| **Web Tokens** | Violet / Light Blue | `page`, `title`, `include`, `create`, `main`, `section`, `style`, `on`, `click` |
| **DB Tokens** | Indigo / Deep Blue | `table`, `columns`, `values`, `insert`, `select`, `all`, `where`, `order by` |

---

## ⚡ Keyboard Shortcuts & Commands

Open VS Code Command Palette (`Ctrl+Shift+P` / `Cmd+Shift+P`) and type **EnLang**:

- **`Ctrl+F5`**: `EnLang: Run Current File` (Executes `enlang run <active_file>`)
- **`EnLang: Build Current File`** (Transpiles source to native target `.html`, `.css`, `.js`, `.sql`, `.py`)
- **`EnLang: Check Syntax / Lint File`** (Performs static analysis and error checking)
- **`EnLang: Start Web Server`** (Launches zero-config EnLang HTTP Dev Server on port 8000)

---

## 💡 Code Snippets (Auto-Completion)

- Type `def` ➔ Auto-expands `define text name as "value"`
- Type `fn` ➔ Auto-expands `function name using param:`
- Type `if` ➔ Auto-expands `if condition then:`
- Type `page` ➔ Auto-expands `.enlgf` HTML page structure
- Type `style` ➔ Auto-expands `.enlgd` CSS design block
- Type `on` ➔ Auto-expands `.enlgs` Client DOM event handler
- Type `table` ➔ Auto-expands `.enlgdb` Database table definition & query

---

## 🚀 Installation

### Option 1: Install from `.vsix` Package
1. Download `enlang-1.0.0.vsix`
2. Open VS Code ➔ Go to Extensions (`Ctrl+Shift+X`)
3. Click the `...` menu at top right ➔ Select **Install from VSIX...**
4. Select `enlang-1.0.0.vsix` and restart VS Code!

### Option 2: Copy to Extensions Folder
Copy `vscode-enlang` directory into:
- **Windows:** `%USERPROFILE%\.vscode\extensions\`
- **Linux/macOS:** `~/.vscode/extensions/`

---

## 📜 License
Copyright © 2026 Spandan Prayas Patra. Distributed under the MIT License.
