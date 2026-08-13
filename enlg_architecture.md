# enlang Ecosystem Architecture

## 1. Unified Compilation & Execution Pipeline
The language ecosystem shares a unified architectural pipeline, customized per language (enlg, enlgf, enlgd, enlgs, enlgdb), but adhering to the following structural flow:

**SOURCE → LEXING → HINT / INTENT DISCOVERY → FLEXIBLE PARSING → AST → SEMANTIC ANALYSIS → TYPE / NAME ANALYSIS → IR → INTEROPERABILITY / CODE GENERATION → RUNTIME**

- **Lexing**: Tokenization of the source code.
- **Hint / Intent Discovery**: Keyword scanning to lock the primary operational intent.
- **Flexible Parsing**: Resolving operands and structures deterministically based on the locked intent.
- **AST & Semantic Analysis**: Generating a strict abstract syntax tree and validating types, names, and scoping rules.
- **IR & Code Generation**: Normalizing into an Intermediate Representation (IR) before lowering to Python (for enlg), HTML (for enlgf), CSS (for enlgd), JavaScript (for enlgs), or parameterized SQL/Adapter calls (for enlgdb).

## 2. Hint-Keyword Architecture
The cornerstone of enlang's parsing strategy.
- **Canonical Intent**: Every operation maps to a singular, internal canonical intent (e.g., `DECLARE_VARIABLE`).
- **Aliases**: Multiple valid English keywords map to the same intent (e.g., `declare`, `create`, `initialize`).
- **Deterministic Resolution**: The remaining tokens in the statement are parsed according to the strict requirements of that intent.
- **Fail-Closed Guarantee**: If a statement matches multiple conflicting interpretations, or is missing required tokens for the locked intent, the parser halts with a deterministic error. It never guesses.

## 3. Shared Toolchain
To avoid fragmenting the five languages, the ecosystem utilizes shared infrastructure:
- **Compiler Core**: Shared lexing, diagnostic, and intent-discovery engines.
- **CLI & Package Management**: A unified `enlang` tool for project building, dependency handling, and module resolution.
- **Language Server & Formatter**: Shared AST traversal mechanisms to provide a unified developer experience.

## 4. Language Interoperability & Communication
The five languages are designed to operate as a cohesive unit (e.g., for full-stack web applications):
- **enlg (Backend Logic)** communicates with **enlgdb (Data)** via explicit, parameterized data access interfaces.
- **enlg (Server)** serves or templates **enlgf (Document)** and **enlgd (Style)**.
- **enlgs (Behavior)** executes within the client bounds, communicating back to **enlg** APIs.

## 5. Unified Diagnostic Error Model
A centralized diagnostic architecture handling:
- **Lexical/Syntax Errors**: Invalid characters, unmatched scopes.
- **Intent Errors**: Unknown hints, ambiguous intents.
- **Semantic Errors**: Type mismatches, name resolution failures.
- **Interoperability Errors**: Missing modules, unsupported backend features.
- **Security Errors**: Boundary violations.

## 6. Security & Execution Boundaries
Security is architected fundamentally:
- **No Unsafe Evaluation**: `eval` and `exec` equivalents are strictly sandboxed or blocked.
- **Parameterized Database IR**: enlgdb translates all values to parameterized queries to prevent SQL injection.
- **Subprocess Isolation**: External command execution requires explicit language boundaries to prevent command injection.
- **Browser Context Isolation**: enlgs compilation respects web security paradigms (XSS mitigation).
