# Enlangg: The English-Syntax Programming Language & Ecosystem

<p align="center">
  <a href="https://enlangg.vercel.app"><img src="website/logo.svg" alt="Enlangg Logo" width="100" height="100"/></a>
</p>

<p align="center">
  <strong>Write Pure English. Compile to Native Speed.</strong><br>
  A deterministic, human-readable programming language, self-hosted core engine, and full-stack development ecosystem.
</p>

<p align="center">
  <a href="https://enlangg.vercel.app"><img src="https://img.shields.io/badge/Website-enlangg.vercel.app-10b981.svg" alt="Website"/></a>
  <a href="https://pypi.org/project/enlang/"><img src="https://img.shields.io/pypi/v/enlang.svg?color=blue" alt="PyPI Version"/></a>
  <a href="https://pypi.org/project/enlang/"><img src="https://img.shields.io/pypi/dm/enlang.svg" alt="PyPI Downloads"/></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-amber.svg" alt="License"/></a>
  <a href="https://enlangg.vercel.app/docs.html"><img src="https://img.shields.io/badge/Specification-Canonical_Manual-purple.svg" alt="Manual"/></a>
</p>

---

## 🏛️ Critical Architectural Distinction: Enlangg vs enlang

To avoid confusion between the native compiler engine and the Python distribution package, the ecosystem is organized into two distinct layers:

```
                          ┌─────────────────────────────────────────────────────────┐
                          │                 THE ENLANGG ECOSYSTEM                   │
                          └────────────────────────────┬────────────────────────────┘
                                                       │
                   ┌───────────────────────────────────┴───────────────────────────────────┐
                   ▼                                                                       ▼
┌───────────────────────────────────────┐                               ┌───────────────────────────────────────┐
│     Enlangg (The Language & Core)     │                               │      enlang (The PyPI Package)        │
├───────────────────────────────────────┤                               ├───────────────────────────────────────┤
│ • The actual Programming Language     │                               │ • The Official Python CLI & Toolchain │
│ • Natural English grammar & semantics │                               │ • Distributed on PyPI: pip install enlang
│ • Inferred memory slot architecture   │                               │ • Multi-domain transpilation engine   │
│ • Bare-metal C compiler (enlng.c)     │                               │ • Live dev server & hot reloading     │
│ • Self-hosted VM & parser (enlang-core│                               │ • No C compiler required to install   │
│ • Sub-2ms warm native execution       │                               │ • CLI commands: `enlang` and `enlg`   │
└───────────────────────────────────────┘                               └───────────────────────────────────────┘
```

### 1. **Enlangg** *(The Programming Language & Core Compiler Engine)*
* **Repository Location**: [`enlang-core/`](enlang-core/) and [`enlng.c`](enlng.c) / [`enlangg.c`](enlangg.c)
* **What It Is**: The standalone, compiled programming language. It transforms spoken English syntax into high-performance, bare-metal native binaries via a deterministic stack-allocated slot memory model (no garbage collector pauses, zero runtime dependencies).
* **Execution**: Compiled with any standard C compiler (`gcc -O3 enlng.c -o enlng`) or run via the self-hosted VM in [`enlang-core/`](enlang-core/).
* **Official Manual & Live Sandbox**: [enlangg.vercel.app](https://enlangg.vercel.app)

### 2. **enlang** *(The Official PyPI Toolchain & CLI Package)*
* **Repository Location**: [`enlang/`](enlang/) and [`enlg/`](enlg/) (with domain packages `enlgd`, `enlgdb`, `enlgf`, `enlgm`, `enlgs`)
* **What It Is**: The official developer toolchain distributed globally via Python's package index (`pip install enlang`). It provides a zero-setup command line interface (`enlang run`, `enlang build`, `enlang check`) that lets developers transpile natural English code across 6 domains (Backend, Frontend, CSS Styling, Client JS, Database, and Mobile Dart).
* **Package URL**: [pypi.org/project/enlang](https://pypi.org/project/enlang/)

---

## ⚡ Quickstart

### Option A: Install via PyPI (Fastest for Python Developers)

```bash
# 1. Install the CLI toolchain globally
pip install enlang

# 2. Run any source file with auto-detection
enlang run examples/atm_management.enlg

# 3. Build full-stack web or mobile artifacts
enlang build examples/portfolio.enlgf
```

### Option B: Native Standalone Compiler (Bare-Metal C Machine Code)

```bash
# On Linux / macOS:
curl -fsSL https://enlangg.vercel.app/install.sh | bash

# On Windows (PowerShell):
irm https://enlangg.vercel.app/install.ps1 | iex

# Or compile the standalone C source in 1 second:
gcc -O3 enlng.c -o enlng
./enlng examples/sample_loop.enlg
```

---

## 📖 The Spoken English Syntax

Enlangg replaces cryptic symbols and boilerplate with readable spoken English clauses:

```enlng
type enlng

// Stack-allocated memory slots
create base_salary of 65000
create bonus of 12000
create tax_rate of 0.18

// Spoken arithmetic
create gross_pay of base_salary plus bonus
create deductions of gross_pay multiplied by tax_rate
create net_pay of gross_pay minus deductions

// Natural conditional logic
if net_pay is greater than 50000:
    display "Executive compensation tier approved"
otherwise:
    display "Standard compensation tier"

display "Net Take-Home Pay: ", net_pay
```

---

## 🌐 The 6 Specialized Domain Tiers

Enlangg enforces strict domain isolation so you can build full-stack systems in cohesive English:

| Domain | File Extension | Target Output | Description |
|---|---|---|---|
| **Core Logic** | `.enlg` / `.enlng` | Native C / Python Bytecode | Systems logic, algorithms, math, and data processing |
| **Frontend UI** | `.enlgf` | HTML5 Semantic DOM | Declarative web page structure and layouts |
| **Styling** | `.enlgd` | Modern CSS3 | Visual aesthetics, flexbox/grid, animations |
| **Scripting** | `.enlgs` | Vanilla JavaScript (ES6+) | Client-side reactive events, async fetch, DOM state |
| **Database** | `.enlgdb` | SQLite / SQL Engine | Declarative table schemas, migrations, CRUD operations |
| **Mobile** | `.enlgm` | Flutter / Dart | Native cross-platform Android and iOS screens |

---

## 📁 Repository Directory Structure

```
enlang/
├── enlang/               # PyPI package entry point (python -m enlang)
├── enlang-core/          # Core Language Engine (Lexer, Parser, Compiler, VM written in Enlangg)
│   ├── core/             # VM, Compiler, Parser source files (.enlng)
│   ├── stdlib/           # Standard library modules
│   └── tests/            # Core compiler test suite
├── enlg/                 # Python CLI implementation, AST, and runtime engine
├── enlgd/                # Design & CSS domain transpiler
├── enlgdb/               # Database domain schema engine
├── enlgf/                # Frontend HTML domain transpiler
├── enlgm/                # Mobile Flutter/Dart domain transpiler
├── enlgs/                # Client-side JavaScript domain transpiler
├── website/              # Official documentation & video learning portal (enlangg.vercel.app)
├── vscode-enlang/        # Official VS Code extension for syntax highlighting
├── examples/             # 40+ curated demo applications (ATM, YouTube UI, Portfolio, ML)
├── tests/                # Automated cross-platform test suites
├── enlng.c               # Canonical zero-dependency native C compiler
├── enlangg.c             # Multi-target universal C compiler implementation
├── install.ps1           # Windows automated installer
├── install.sh            # Linux & macOS automated installer
├── install.cmd           # Windows CMD one-liner installer
├── setup.py              # PyPI package build configuration
└── pyproject.toml        # PEP 518/621 build system specification
```

---

## 🎥 Video Tutorials & Interactive Documentation

* **Official Website**: [https://enlangg.vercel.app](https://enlangg.vercel.app)
* **Video Masterclass (NotebookLM)**: [https://enlangg.vercel.app/learn.html](https://enlangg.vercel.app/learn.html)
* **In-Browser Sandbox IDE**: [https://enlangg.vercel.app/playground.html](https://enlangg.vercel.app/playground.html)
* **Standard Library Reference**: [https://enlangg.vercel.app/library.html](https://enlangg.vercel.app/library.html)
* **Canonical Specification Book (193 pp.)**: [Download PDF](https://enlangg.vercel.app/enlangg_the_enlng.pdf)

---

## 🤝 Contributing

We welcome contributions from developers worldwide! Whether you want to improve compiler performance, add standard library functions, or expand language domains:

1. Fork this repository.
2. Create your feature branch: `git checkout -b feature/natural-operator`
3. Commit your changes: `git commit -m "Add spoken modulo operator"`
4. Push to the branch: `git push origin feature/natural-operator`
5. Open a Pull Request.

---

## 📜 License

Enlangg and the `enlang` distribution package are released under the open-source **[MIT License](LICENSE)**. Free to use, modify, and distribute for both personal and commercial projects.
