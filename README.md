# EnLang — The Universal Natural English Programming Language & Full-Stack Engine
### *Build Full-Stack Web Apps, Mobile UIs, Logic & 3D Visuals in Pure Natural English*

[![Version](https://img.shields.io/badge/version-1.0.0--Stable-indigo.svg)](https://pypi.org/project/enlang/)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![PyPI](https://img.shields.io/pypi/v/enlang.svg)](https://pypi.org/project/enlang/)

Created and Authored by **Spandan Prayas Patra**.

---

## 🌟 Overview

**EnLang** is a deterministic, human-readable programming language and transpilation engine that converts natural English into clean, production-grade native code targets:

- **`.enlg`** ➔ Core Backend Logic & Algorithms (Python 3 / Bytecode VM)
- **`.enlgf`** ➔ Structural Frontend Web Markup (HTML5)
- **`.enlgd`** ➔ Responsive Design & Aesthetics (CSS3)
- **`.enlgs`** ➔ Client-Side Reactive Scripting & DOM Logic (Vanilla JavaScript ES6+)
- **`.enlgm`** ➔ Mobile Applications & Screens (Flutter / Dart)

---

## 🚀 Official Installation

Install EnLang globally on your system using `pip`:

```bash
pip install enlang
```

---

## 🛠️ CLI Command Reference

Once installed, use the `enlang` (or `enlg`) command anywhere in your terminal:

### 1️⃣ Execution & Live Dev Server
```bash
# Run any source file with auto-detection (.enlg, .enlgf, .enlgs, .enlgm, .html, .js, .py):
enlang run tournament_app/tournament.enlgf

# Run on a custom port:
enlang run index.enlgf -p 3000
```

### 2️⃣ Compilation & Production Build
```bash
enlang build page.enlgf     # Bundles HTML5, CSS3, and JS into page.html
enlang build style.enlgd    # Compiles to modern CSS3 (style.css)
enlang build script.enlgs   # Compiles to pure Vanilla JavaScript (script.js)
enlang build app.enlg       # Transpiles to clean Python (app.py)
enlang build app.enlgm      # Compiles to Flutter/Dart (app.dart)
```

### 3️⃣ Version & Package Management
```bash
# Check installed version and check PyPI for updates:
enlang check -v

# Update enlang to latest release:
enlang update -v latest

# Install a specific version co-existing alongside current version:
enlang install -v 1.0.0

# Replace current active installation with a specific version:
enlang replace -v 1.0.0

# List all co-existing installed versions:
enlang list -v

# Switch active default version:
enlang switch -v 1.0.0
```

### 4️⃣ Interactive REPL Shell
```bash
enlang repl
```

---

## 🎨 Sample Syntax

### 🔹 1. Markup (`index.enlgf`)
```enlgf
page:
    head:
        title "Vortex Esports"
    body:
        header class "main-navbar":
            h1 "VORTEX ESPORTS"
            button "LOG IN" id "btn-login"
```

### 🔹 2. Design (`style.enlgd`)
```enlgd
.main-navbar:
    background "rgba(15, 23, 42, 0.8)"
    backdrop-filter "blur(12px)"
    padding "16px 32px"
    display "flex"
    justify-content "space-between"
```

### 🔹 3. Client Scripting (`app.enlgs`)
```enlgs
in script:
    create score as 0

    when "btn-login" is clicked:
        put { user: "Kiryu", score: 50 } into userRecord
        add "Kiryu" to activePlayersList
        set text of "user-badge" to "Welcome, Kiryu!"
        add class "active" to "user-badge"
        after 3 seconds:
            hide element "welcome-banner"
```

---

## 📄 License
MIT License. Copyright (c) 2026 Spandan Prayas Patra.
