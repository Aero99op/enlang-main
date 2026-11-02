[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# Enlangg: Programming in Plain English

<p align="center">
  <a href="https://enlangg.vercel.app"><img src="website/logo.svg" alt="Enlangg Logo" width="100" height="100"/></a>
</p>

Enlangg is a programming language whose programs read like sentences, and the set of small native tools around it. The language is called Enlng (files end in `.enlng`). The tools are written in C and need nothing else installed to run a program.

- Website, playground and manual: <https://enlangg.vercel.app>
- License: [MIT](LICENSE)

## Install

**Windows.** Download `enlangg-setup.exe` from the website. It installs for the current user, with no administrator rights, and adds the tools to `PATH`.

**Linux and macOS.**

```bash
curl -fsSL https://enlangg.vercel.app/install.sh | bash
```

**From source.** You need a C compiler (gcc or clang) and `make`.

```bash
make
make install PREFIX=$HOME/.local
```

## A first program

```enlng
type enlng

remember base_salary as 65000
remember bonus as 12000
freeze TAX_RATE as 0.18

remember gross_pay as base_salary plus bonus
remember net_pay as gross_pay minus gross_pay times TAX_RATE

when net_pay > 50000:
    show "Higher tier"
otherwise:
    show "Standard tier"

show "Net pay:" net_pay
```

```bash
enlng run hello.enlng             # built-in interpreter, no C compiler needed
enlng run --native hello.enlng    # build with gcc, then run (faster for heavy loops)
enlng build hello.enlng -o hello  # a standalone executable
```

## Tools

| Tool | Files | What it does |
|---|---|---|
| `enlng` | `.enlng` | The language: run, build, test, debug, packages, editor server |
| `enlngdb` | `.enlngdb` | A database written in C, with statements in English |
| `enlngf` | `.enlngf` | Window and page layouts |
| `enlngd` | `.enlngd` | Styles |
| `enlngs` | `.enlngs` | Scripts |
| `enlngm` | `.enlngm` | Mobile screens |
| `enlangg` | any of the above | A front door that picks the right tool for a file |

## Packages, the debugger and editor help

```bash
enlng add ../shared/geometry        # install a package from a folder, a file or a git address
enlng packages                      # list what is installed
enlng install                       # install everything named in enlng_packages.txt
enlng debug program.enlng           # run one statement at a time
enlng lsp                           # editor server (Language Server Protocol)
```

A package is used by name: `use "geometry"`.

## Try it in the browser

The playground at <https://enlangg.vercel.app/playground.html> runs the same interpreter, built to WebAssembly from the same C sources (`tools/build_wasm.ps1`). Nothing is sent to a server.

## Repository

```
src/            the language: lexer, parser, interpreter, C generator, editor server
enlng.c         the build of the enlng tool (includes src/)
enlngdb/        the database engine
enlngf/ enlngd/ enlngs/ enlngm/   the other tools
lib_*.enlng     the standard library, written in Enlng
book/           the source of the manual (The Enlng Book)
website/        the website, the playground and Enlangg Studio
desktop/        Enlangg Studio for Windows (Electron)
installer/      the Windows installer
vscode-enlang/  syntax highlighting for VS Code
tests/          the test suites
```

## Contributing

Fork the repository, make a branch, build with `make`, and open a pull request. The test programs are in `tests/`.
