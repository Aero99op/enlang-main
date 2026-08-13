# enlang Ecosystem Language Specification

## PART 1: enlg (General Purpose Programming)
enlg is the core, general-purpose backend programming language of the ecosystem. It is equivalent in capability to Python.

### 1. Variables & Data Types
- **Variables**: Declaration, assignment, reassignment, mutation, multiple assignment, and unpacking.
- **Data Types**: `integer`, `float`, `complex`, `boolean`, `string`, `bytes`, `null`, `list`, `tuple`, `set`, `map` (dictionary), `range`, iterators, generators, classes, objects, and enums.

### 2. Operators & Expressions
- **Operators**: Full suite of arithmetic, comparison, equality, boolean logic, bitwise, membership, and identity operators.
- **Expressions**: Function calls, indexing, slicing, attribute access, object instantiation, comprehensions (list, set, dict, generator), and lambda expressions.

### 3. Control Flow & Functions
- **Control Flow**: `if`, `elif`, `else`, `while`, `for`, `break`, `continue`, `pass`, nested loops, match/case equivalents.
- **Functions**: Definitions, invocations, positional/keyword/default/variadic parameters, recursion, closures, higher-order functions, decorators, async (`async`/`await`), and generators (`yield`).

### 4. OOP & Advanced Features
- **OOP**: Classes, constructors, instance/class attributes, methods, inheritance (including multiple), overriding, properties, polymorphism, and dunder/special methods.
- **Exceptions**: `try`, `except` (multiple), `finally`, `else`, `raise`, and context-managers.
- **Modules & Packages**: Imports, aliasing, relative imports, and namespacing.
- **Type System**: Dynamic by default, with optional type annotations, generics, unions, and runtime inspection.

### 5. Python Interoperability (Core Requirement)
enlg must natively interoperate with the Python ecosystem without artificial whitelists.
- **Standard Library**: Native access to `os`, `sys`, `json`, `math`, `datetime`, `collections`, `asyncio`, `subprocess`, `sqlite3`, etc.
- **Third-Party Ecosystem**: Unrestricted access to `NumPy`, `Pandas`, `scikit-learn`, `PyTorch`, `FastAPI`, `Django`, AWS/GCP SDKs, etc., via the interop layer.

---

## PART 2: enlgf (English HTML Equivalent)
enlgf provides 1:1 expressive capability with modern HTML.
- **Structure**: `html`, `head`, `body`, metadata, titles.
- **Content**: Headings, paragraphs, links, lists, tables, embedded content (audio, video, iframe, canvas, SVG).
- **Semantics**: Sections, articles, navigation, headers, footers, details, summary.
- **Forms**: Inputs, labels, buttons, selects, textareas, validation.
- **Accessibility**: ARIA roles, semantic data attributes.

---

## PART 3: enlgd (English CSS Equivalent)
enlgd provides 1:1 expressive capability with modern CSS.
- **Selectors**: Element, class, ID, attributes, pseudo-classes/elements, combinators.
- **Styling**: Colors, typography, backgrounds, borders, shadows, gradients.
- **Layout**: Box model (margin/padding), display, positioning, flexbox, grid.
- **Advanced**: Media queries, container queries, transforms, transitions, animations, keyframes, custom properties (variables), calc expressions, and nesting.

---

## PART 4: enlgs (English JavaScript Equivalent)
enlgs provides 1:1 expressive capability with modern JavaScript for web interaction.
- **Core**: Variables, closures, arrays, objects, maps, sets, destructuring, spread/rest, conditionals, loops, classes.
- **Web API**: DOM manipulation, event handling, timers, storage APIs.
- **Async**: Promises, `async`/`await`, callbacks, fetch/network APIs.
- **Advanced**: Modules (imports/exports), JSON, RegExp, WebSockets.

---

## PART 5: enlgdb (English SQL Equivalent)
enlgdb provides full SQL-equivalent capabilities with robust backend adapters.
- **Database/Schema**: Create, alter, drop.
- **Tables & Columns**: Create, alter, drop, rename.
- **Data (DML)**: Insert, select, update, delete, merge/upsert.
- **Queries**: Where, order by, group by, having, distinct, limit, joins, subqueries, CTEs, unions.
- **Advanced**: Aggregation, constraints (PK, FK, unique), indexes, views, transactions (begin/commit/rollback), window functions.
- **Security**: Strictly parameterized execution preventing SQL injection. Target backends include SQLite, PostgreSQL, MySQL.
