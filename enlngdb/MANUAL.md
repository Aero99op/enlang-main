# 📚 EnlngDB: The Complete Reference Manual
### Sovereign Zero-SQL Database Engine — Definitive Specification & Developer Guide
**Version:** 2.0.0-sovereign-native | **Target Platforms:** Windows x64 / Linux x86_64 | **License:** MIT Sovereign Open Source

---

## Table of Contents

1. [Architectural Philosophy & Overview](#1-architectural-philosophy--overview)
   - 1.1 The Sovereign Zero-SQL Manifesto
   - 1.2 Dual Engine Implementations: Pure C vs Sovereign Python
   - 1.3 Storage Formats: `.edb` Binary vs Native Memory Snapshots
   - 1.4 File Extensions & Standards
2. [Lexical Grammar, Noise Filtering & Statement Anatomy](#2-lexical-grammar-noise-filtering--statement-anatomy)
   - 2.1 Domain Header Declaration (`type enlngdb` / `type enlgdb`)
   - 2.2 Conversational Noise Filtering (Silent Words)
   - 2.3 Punctuation, Semicolons & Single-Line Chaining
   - 2.4 Comments Syntax (`#` and `--`)
   - 2.5 Identifiers, Literals & String Quoting Rules
3. [Data Types, Schema Constraints & Directives](#3-data-types-schema-constraints--directives)
   - 3.1 Primitive & Structured Data Types
   - 3.2 Schema Constraints (`primary key`, `autoincrement`, `unique`, `not null`, `default`, `references`)
   - 3.3 Engine Directives & Performance Hints (`hint`)
4. [Database & Schema Lifecycle (DDL)](#4-database--schema-lifecycle-ddl)
   - 4.1 Database Navigation (`show databases`, `use database`, `show tables`)
   - 4.2 Table Creation: Mode 1 (Indented Block) vs Mode 2 (Inline)
   - 4.3 Schema Evolution: Adding & Dropping Columns (`alter table`, `delete column`)
   - 4.4 Destructive Operations & The Mandatory Safety Guard (`confirmed`)
5. [Data Ingestion & Insertion (DML)](#5-data-ingestion--insertion-dml)
   - 5.1 Insertion Verbs (`insert into`, `put into`, `save into`)
   - 5.2 Flexible Assignment Operators (`:`, `=`, `to`, `is`, space)
   - 5.3 Insertion Modes: Indented Block vs Fluent Inline
   - 5.4 Dynamic Schema Expansion (Auto-column Registration)
6. [Data Querying & Filtering Mastery (DQL)](#6-data-querying--filtering-mastery-dql)
   - 6.1 Query Initiation Keywords (`find`, `fetch`, `select`, `get`, `show all records from`)
   - 6.2 Field Projections, Aliasing & Deduplication (`distinct`)
   - 6.3 The Full Natural English Comparison Operator Matrix
   - 6.4 Logical Chaining (`and`, `or`, nested parenthesis)
   - 6.5 Sorting & Ordering (`order by`, `sorted by`, `highest`, `lowest`, `asc`, `desc`)
   - 6.6 Pagination & Limiting (`top`, `first`, `limit`, `offset`)
7. [Aggregations, Analytics & Relational Joins](#7-aggregations-analytics--relational-joins)
   - 7.1 Top-Level Direct English Counting
   - 7.2 Built-in Aggregate Functions (`count`, `sum`, `avg`, `min`, `max`)
   - 7.3 Multi-Table Relational Joins (`inner`, `left`, `right`, `full` and `cross` join, as words or as marks `- <- -> <-> *`, with or without `on`)
8. [Data Modification & Purging (DML Mutations)](#8-data-modification--purging-dml-mutations)
   - 8.1 Conversational Record Updates (`update`, `change`, `modify`, `in <table> update set`)
   - 8.2 Safe Row Deletions (Filtered vs Unconstrained Purge)
   - 8.3 Column Removal
9. [Persistence, Storage Internals & Concurrency](#9-persistence-storage-internals--concurrency)
   - 9.1 Disk Persistence Commands (`save database to`, `open database to`)
   - 9.2 Binary File Specification (`ENLNG_C_EDB_V2`)
   - 9.3 In-Memory Indexing & Automatic Re-indexing
   - 9.4 Concurrency Control: Multi-Reader Single-Writer Locking (`DatabaseLock`)
   - 9.5 MVCC & Row Versioning (`_version`)
10. [APIs, SDKs, CLI & Syntax Cheat Sheet](#10-apis-sdks-cli--syntax-cheat-sheet)
    - 10.1 Pure C Embedding API (`enlngdb.h`)
    - 10.2 Python Native SDK Integration
    - 10.3 CLI Tooling & Script Execution (`enlngdb.exe`)
    - 10.4 SQL-to-EnlngDB Cross-Reference Translation Table
    - 10.5 Compiler Diagnostic & Error Recovery Guide
11. [Expressions, Subqueries, Constraints, Views & Transactions (C engine)](#11-expressions-subqueries-constraints-views--transactions-c-engine)
    - 11.1 Conditions, 11.2 Subqueries, 11.3 Groups & `having`, 11.4 Expressions & functions, 11.5 Aliases & self joins
    - 11.6 `union`, `intersect`, `except`, `with`, 11.7 Window functions, 11.8 Inserting, 11.9 Updating & deleting
    - 11.10 Types & constraints, 11.11 Altering tables, 11.12 Views, indexes, `describe`, `explain`, 11.13 Transactions & databases, 11.14 What it does not have
12. [Beyond SQL: files, safety, speed (C engine)](#12-beyond-sql-files-safety-speed-c-engine)
    - 12.1 CSV files, 12.2 `preview` and `undo`, 12.3 Savepoints, 12.4 `natural join`, 12.5 `on delete` / `on update`
    - 12.6 Recursive `with`, 12.7 One program at a time, 12.8 Speed, 12.9 Limits, 12.10 Triggers and procedures, 12.11 Did you mean, 12.12 Users and the server

---

# 1. Architectural Philosophy & Overview

### 1.1 The Sovereign Zero-SQL Manifesto
Traditional relational database management systems enforce rigid, archaic SQL syntax rooted in 1970s IBM SEQUEL standards. These engines introduce heavy external dependencies, complex connection pool configurations, brittle driver binaries, and impedance mismatches with modern application logic.

**EnlngDB** rejects these dependencies entirely:
* **Zero SQL Syntax:** No cryptic `SELECT`, `FROM`, `WHERE` strictures or comma-placement anxiety. Queries are expressed in declarative, conversational English.
* **Zero External Library Dependencies:** The core engine relies on standard C99 runtime primitives (`stdio.h`, `stdlib.h`, `string.h`, `stdint.h`, `stdbool.h`) and standard Python runtime packages. There are zero links to `sqlite3`, `libpq`, `mysqlclient`, or third-party ORMs.
* **Sovereign Execution:** The engine parses, plans, optimizes, indexes, and executes queries natively in memory or via dedicated binary storage layouts.

---

### 1.2 Dual Engine Implementations: Pure C vs Sovereign Python
EnlngDB exists in two parallel, fully compatible execution environments:

| Feature | Pure C Native Engine (`enlngdb.exe` / `libenlngdb`) | Sovereign Python Engine (`enlngdb` / `enlgdb`) |
| :--- | :--- | :--- |
| **Primary Goal** | High-performance, embedded, microsecond latency | Toolchain integration, scripting, rapid pipeline evaluation |
| **Source Location** | `enlngdb/c/` (`enlngdb.c`, `enlngdb_parser.c`, `enlngdb.h`) | `enlngdb/` (`lexer.py`, `parser.py`, `storage.py`, `engine.py`) |
| **Memory Model** | Fixed contiguous cell arrays with dynamic realloc | Dynamic dictionary rows with automatic hash indexing |
| **Storage Format** | Native binary `.edb` (`ENLNG_C_EDB_V2`) | Native `.edb` / atomic JSON format (`ENLNGDB_SOVEREIGN_V1`) |
| **Concurrency** | Single-threaded atomic script executor | Readers-Writer lock (`DatabaseLock` via `threading.RLock`) |
| **Binary Footprint**| ~100 KB stand-alone executable | Pure Python modules (wheel package) |

---

### 1.3 Storage Formats: `.edb` Binary vs Native Memory Snapshots
* **`.edb` (Enlng Database Binary):** A high-performance, compact file format beginning with the 14-byte magic signature `ENLNG_C_EDB_V2` (files written by older versions, `ENLNG_C_EDB_V1`, are still read). It contains table definitions, column types and constraints, views, indexes, and packed row values.
* **Atomic Memory Snapshots:** Both engines maintain active state entirely in memory. When saving, data is written atomically to temporary files before being swapped in place to prevent file corruption during power failures or crashes.

---

### 1.4 File Extensions & Standards
* **`.enlngdb` / `.enlgdb`:** Declarative database script containing schema definitions, seeds, queries, and mutations.
* **`.edb`:** Compiled binary database storage container.

---

# 2. Lexical Grammar, Noise Filtering & Statement Anatomy

### 2.1 Domain Header Declaration
Every canonical EnlngDB script begins with an optional domain declaration header. This tells the parser and the toolchain that the incoming tokens belong strictly to the database domain.

```enlngdb
type enlngdb
```
*(Alternatively, `type enlgdb` is fully supported for compatibility).*

---

### 2.2 Conversational Noise Filtering (Silent Words)
EnlngDB features an intelligent conversational pre-parser and lexical noise filter. The following tokens are classified as **Silent Words** and are discarded automatically without altering query semantics:

```text
the, a, an, please, kindly, record, records, row, rows, entry, entries, data, item, items
```

This enables natural, conversational expressions. All of the following statements produce the **identical Abstract Syntax Tree (AST)**:

```enlngdb
find all records from users
find the records from users
find rows from the users
please find the data from users
find users
```

> **Delimitation Rule:** Silent words are only skipped when they appear outside string literals and do not serve as structural clause delimiters (such as `with`, `where`, `set`, `,`, `;`, `:`, `=`).

---

### 2.3 Punctuation, Semicolons & Single-Line Chaining
Statements in EnlngDB can be delimited either by **physical newlines** or by **semicolons (`;`)**.

#### Multi-line Script Format:
```enlngdb
type enlngdb

create table users with id, name, balance
insert record into users with id 1, name "Bibhu", balance 50000
find all records from users
```

#### Semicolon-Chained Single-Line Format:
```enlngdb
create table items with id, title; insert record into items with id 10, title "Keyboard"; find records from items;
```

---

### 2.4 Comments Syntax
EnlngDB supports both native Enlangg hash comments and standard database double-dash comments:

```enlngdb
# Native sovereign comment — ignored by parser
-- Standard SQL-style comment — also ignored by parser
create table customers with id, email # Inline comments are supported
```

---

### 2.5 Identifiers, Literals & String Quoting Rules
1. **Identifiers (Table & Column names):**
   * Bare names: `users`, `emp_id`, `created_at`.
   * Quoted identifiers: `"users"`, `'order details'`.
   * Dot-notation: `users.id`, `orders.customer_id`.
2. **String Literals:**
   * Double quotes: `"Quantum Computer"`
   * Single quotes: `'Hardware Engineer'`
3. **Numeric Literals:**
   * Integers: `42`, `-100`, `9999999`
   * Floating-point numbers: `3.14159`, `0.05`, `-12.50`
4. **Boolean Literals:**
   * `true`, `True`, `false`, `False`
5. **Null Literals:**
   * `null`, `Null`, `NULL`

---

# 3. Data Types, Schema Constraints & Directives

### 3.1 Primitive & Structured Data Types
> **C engine (`enlngdb.exe`, the playground, Studio):** a column is `integer`, `real`, `text`, `boolean` or `date`, or has no type (§11.10). `TIMESTAMP`, `JSON` and `BLOB` in the table below are not column types of the C engine, and a script that uses them gets an error.

EnlngDB supports the following foundational storage types:

| Data Type | Memory Representation (C) | Python Representation | Description |
| :--- | :--- | :--- | :--- |
| `INTEGER` | `int64_t` | `int` | 64-bit signed integer |
| `REAL` / `DOUBLE` | `double` | `float` | 64-bit IEEE 754 floating point |
| `TEXT` / `STRING` | `char*` (heap allocated) | `str` | UTF-8 encoded text string |
| `BOOLEAN` | `bool` | `bool` | Boolean (`true` / `false`) |
| `TIMESTAMP` | `char[32]` / `int64_t` | `str` / `datetime` | ISO-8601 or UNIX millisecond timestamp |
| `JSON` | `char*` (serialized string) | `dict` / `list` | Native nested JSON document |
| `BLOB` | `uint8_t*` (byte stream) | `bytes` | Binary object payload |
| `NULL` | Tag `ENLNG_VAL_NULL` | `None` | Null / unassigned value |

---

### 3.2 Schema Constraints
Constraints are declared directly within column definitions to guarantee data integrity:

1. **`primary key`**:
   Marks the column as the table's primary identifier. Automatically creates an indexed lookup table and rejects duplicate entries.
2. **`autoincrement`**:
   Automatically generates an incrementing integer starting from `1` if no value is explicitly supplied during insertion.
3. **`unique`**:
   Enforces distinct values across all rows. Rejects inserts or updates containing duplicate values.
4. **`not null`**:
   Disallows `null` or unassigned values for the specified column.
5. **`default <value>`**:
   Supplies a fallback literal value whenever the column is omitted during insertion.
6. **`references <table_name>(<column>)`**:
   Establishes a relational reference to another table's column (Foreign Key).

---

### 3.3 Engine Directives & Performance Hints (`hint`)
> **C engine (`enlngdb.exe`, the playground, Studio):** `hint` is not read; a statement with `hint` in it gets an error. What makes the C engine fast is described in §12.8.

Hints allow developers to advise the underlying storage engine on caching, index structures, and memory locality without polluting query semantics.

```enlngdb
# Table-level performance hints
create table accounts with:
    id as integer primary key
    balance as real
    hint storage: in-memory-fast, cache: true

# Query-level optimization hints
find all records from accounts where balance is at least 10000 hint index: btree, parallel: 4
```

---

# 4. Database & Schema Lifecycle (DDL)

### 4.1 Database Navigation
EnlngDB supports multi-database switching and inspection:

```enlngdb
# List all active databases
show databases

# Switch active database container (auto-loads <dbname>.edb if present)
use database enterprise_vault
use production_store

# List all tables in active database
show tables

# Inspect tables in a specific database
show tables from enterprise_vault
```

---

### 4.2 Table Creation: Two Distinct Dialects

#### Mode 1: Indented Block Syntax (Recommended for Production)
> **C engine (`enlngdb.exe`, the playground, Studio):** the indented `with:` form is not supported. Write the columns on one line after `with`, or in brackets over several lines (§11.10).

Provides clean, structured schema declarations with explicit data types and constraints:

```enlngdb
create table users with:
    id as integer primary key autoincrement
    email as text unique not null
    full_name as text not null
    balance as real default 0.0
    is_active as boolean default true
    created_at as text default "2026-01-01"
```

#### Mode 2: Inline Schema Definition (Rapid Prototyping)
Ideal for rapid prototyping or ad-hoc data structures:

```enlngdb
# Simple untyped column declaration (defaults to ANY/TEXT)
create table metrics with timestamp, cpu_load, memory_used, node_id

# Typed inline column declaration
create table if not exists products with id as integer primary key, title as text, price as real
```

---

### 4.3 Schema Evolution: Adding & Dropping Columns

#### Adding Columns:
```enlngdb
alter table users add column phone_number as text default "N/A"
alter table users add department as text
```

#### Dropping Columns:
EnlngDB permits dropping individual columns without rewriting the entire table:

```enlngdb
# Using alter table
alter table users drop column department confirmed

# Using conversational syntax
delete column department from users
drop column phone_number from users confirmed
```

---

### 4.4 Destructive Operations & The Mandatory Safety Guard (`confirmed`)
To protect against accidental data loss, **EnlngDB enforces a strict confirmation protocol**. Any unconstrained destructive command missing the `confirmed` keyword will fail immediately with a compilation error.

```enlngdb
# ❌ THIS WILL FAIL WITH A PARSER ERROR:
drop table users

# ✅ CORRECT: Explicit confirmation provided
drop table users confirmed

# ❌ THIS WILL FAIL:
truncate table logs

# ✅ CORRECT:
truncate table logs confirmed

# ❌ THIS WILL FAIL:
drop database test_db

# ✅ CORRECT:
drop database test_db confirmed
```

---

# 5. Data Ingestion & Insertion (DML)

### 5.1 Insertion Verbs
> **C engine (`enlngdb.exe`, the playground, Studio):** `insert into` and `insert record into` work; `put into` and `save into` do not.

All of the following ingestion keywords are recognized interchangeably:
* `insert into`
* `insert record into`
* `put into`
* `save into`

---

### 5.2 Flexible Assignment Operators
EnlngDB supports five natural assignment styles for key-value pairs:
1. Colon style: `name: "Bibhu"`
2. Equals style: `balance = 85000`
3. Word "is" style: `is_active is true`
4. Word "to" style: `role to "Admin"`
5. Space delimited: `id 101`

---

### 5.3 Insertion Modes

#### Mode 1: Indented Key-Value Block
> **C engine (`enlngdb.exe`, the playground, Studio):** the indented `with:` form is not supported; use Mode 2 or `values` (§11.8).

Highly readable for complex, multi-attribute records:

```enlngdb
insert into users with:
    id: 101
    email: "bibhu@enlangg.org"
    full_name: "Bibhudatta"
    balance: 150000.50
    is_active: true
```

#### Mode 2: Fluent Inline Insertion
Compact format for single-line operations:

```enlngdb
insert record into users with id 102, email: "alex@enlangg.org", full_name: "Alex", balance = 75000
put into products with id 1, title "MacBook Pro", price 2400.00, in_stock true
save into orders with order_id 9001, customer "Bibhu", total is 340.50
```

---

### 5.4 Dynamic Schema Expansion (Auto-Column Registration)
In EnlngDB, inserting a record with a new column does not crash the database. The engine **dynamically expands the table schema** at runtime, assigning `null` to that column for all existing rows:

```enlngdb
create table events with id, event_name
insert into events with id 1, event_name "Boot"

# The column 'severity' was never declared in CREATE TABLE!
insert into events with id 2, event_name "DiskWarning", severity "HIGH"

# Table 'events' now contains columns: id, event_name, severity
```

---

# 6. Data Querying & Filtering Mastery (DQL)

### 6.1 Query Initiation Keywords
Queries can begin with any of the following natural English terms:
* `find`
* `fetch`
* `select`
* `get`
* `show all records from`

---

### 6.2 Field Projections & Deduplication
```enlngdb
# Retrieve all fields
find all records from users
find * from users
get from users

# Retrieve specific fields
find id, full_name, balance from users
fetch email, balance from users

# Deduplicated unique values
select distinct department from employees
```

---

### 6.3 The Full Natural English Comparison Operator Matrix
EnlngDB translates standard English comparison phrases directly into high-speed byte comparison instructions:

| Relational Operator | Natural English Phrases Supported | Runnable Query Example |
| :---: | :--- | :--- |
| `=` | `is`, `is equal to`, `equal to`, `equals`, `==`, `=` | `find users where status is "Active"` |
| `!=` | `is not`, `is not equal to`, `not equal to`, `!=` | `find users where role is not "Guest"` |
| `>` | `is greater than`, `greater than`, `>`, `is above`, `more than` | `find users where balance is greater than 50000` |
| `>=` | `is at least`, `is greater than or equal to`, `greater than or equal to`, `>=` | `find users where age is at least 18` |
| `<` | `is less than`, `less than`, `<`, `is under`, `below` | `find products where price is under 20.0` |
| `<=` | `is at most`, `is less than or equal to`, `less than or equal to`, `<=` | `find tasks where retries is at most 3` |
| `LIKE` | `like` (supports `%` wildcards) | `find logs where message like "%timeout%"` |

---

### 6.4 Logical Chaining
Combine conditions seamlessly using `and`, `or`, and nested parentheses:

```enlngdb
find records from employees where (department is "Engineering" or department is "Research") and salary is at least 90000 and is_active is true
```

---

### 6.5 Sorting & Ordering
EnlngDB supports both standard SQL keywords and intuitive English direction modifiers:

```enlngdb
# Ascending orders
find records from products order by price ascending
find records from products sorted by price lowest first
find records from users order by name asc

# Descending orders
find records from employees order by salary descending
find records from employees sorted by salary highest first
find records from logs order by timestamp desc
```

---

### 6.6 Pagination & Limiting
Constrain query result size and implement pagination:

```enlngdb
# Limit using 'top'
find top 5 records from products order by price highest first

# Limit using 'first'
find first 10 records from users

# Limit and Offset pagination
find records from users order by id asc limit 20 offset 40
```

---

# 7. Aggregations, Analytics & Relational Joins

### 7.1 Top-Level Direct English Counting
Counting records does not require wrapping queries in cumbersome function calls:

```enlngdb
# Count total records in table
count records in users
count all from users

# Count with conditions
count records in orders where total is at least 500
count employees from staff where department is "Engineering"
```

---

### 7.2 Built-in Aggregate Functions
EnlngDB provides standard analytics aggregators:

```enlngdb
select count(*) from users
select sum(amount) from orders where status is "Completed"
select avg(salary) from employees where department is "Engineering"
select min(price), max(price) from products
```

---

### 7.3 Multi-Table Relational Joins
A join puts the rows of two tables side by side. You write the kind of join, with a word or with a mark;
the `on ...` part says which columns match, and can be left out when the names make it clear.

| Join | Words | Mark | What you get |
| :--- | :--- | :---: | :--- |
| inner | `join`, `inner join` | `-` | only the rows that have a match in both tables |
| left | `left join`, `left outer join` | `<-` | every row of the first table; `null` where the second has no match |
| right | `right join`, `right outer join` | `->` | every row of the second table; `null` where the first has no match |
| full | `full join`, `full outer join` | `<->` | every row of both tables |
| cross | `cross join` | `*` | every row paired with every row (no `on`) |

The words and the marks do the same thing, so use whichever reads better. A mark is a picture: the
arrow points at the table whose rows all stay (`student <- fees` keeps every student, `student -> fees`
keeps every fee, `<->` keeps both). A plain `-` has no arrow, so only the matches stay.

```enlngdb
# With 'on': say which columns match
find users.full_name, orders.total from users
inner join orders on users.id is orders.user_id
where orders.total is greater than 1000

select customers.name, invoices.amount from customers
left join invoices on customers.id = invoices.customer_id

# Without 'on': the columns are matched by their names
find name, amount from student inner join fees        # student.id with fees.student_id
find name, amount from student left join fees
find name, amount from student right join fees
find name, amount from student full join fees
find name, title from student cross join book

# The same joins, written with marks
find name, amount from student - fees                 # inner
find name, amount from student <- fees                # every student
find name, amount from student -> fees                # every fee
find name, amount from student <-> fees               # both
find name, title from student * book                  # cross
find name from student <- fees where amount is null   # students who have paid nothing

# Three tables, and then the usual where / group by / order by / limit
find name, sum(amount) from student left join fees group by name order by sum(amount) desc
find name, amount, title from student join fees on id is student_id join book on student.id is book.student_id
```

With three tables the tables before the new one can offer more than one way to match (`student.id` and
`fees.student_id` both fit `book.student_id`). EnlngDB then says so and shows the `on` to pick.

**How the columns are matched when `on` is left out.** One of these must give exactly one match:
* a table's `id` with the other table's `<table>_id` (`student.id` with `fees.student_id`; a table named `students` also matches `student_id`)
* a column with the same key-like name in both tables (a name ending in `_id`, `_no`, `_code` or `_key`, like `roll_no`)

A shared `name` or `title` is never used, because it is not a key. If there is no match, or more than
one, EnlngDB does not guess: it says so and shows the `on` to write.

If two joined tables have a column with the same name, write it as `table.column`.

---

# 8. Data Modification & Purging (DML Mutations)

### 8.1 Conversational Record Updates
EnlngDB offers complete flexibility when modifying existing records:

```enlngdb
# Standard update syntax
update records in users set balance = 92000.00 where id is 101

# Conversational 'in <table> update set' syntax
in table users update set status = "VIP", balance = balance + 5000 where id is 101

# Conversational 'change' / 'modify' verbs
change products set price to 49.99, in_stock to true where id is 15
modify employees set department: "AI Infrastructure" where emp_id is 8
```

---

### 8.2 Safe Row Deletions
```enlngdb
# Conditioned deletion (deletes only matching rows)
delete records from users where is_active is false
remove from orders where status is "Cancelled"

# Purging all rows (Protected: requires 'confirmed')
delete all from logs confirmed
remove all records from temp_cache confirmed
```

---

### 8.3 Column Removal
Drop a specific column from a table without losing remaining row data:

```enlngdb
delete column legacy_token from users
```

---

# 9. Persistence, Storage Internals & Concurrency

### 9.1 Disk Persistence Commands
Explicitly commit the database state to disk or load an existing container:

```enlngdb
# Save current database state to binary container
save database to "production_vault.edb"

# Load database container from disk
open database to "production_vault.edb"
```

---

### 9.2 Binary File Specification (`ENLNG_C_EDB_V2`)
The pure C native engine persists tables directly using a compact binary layout. Every field is written by itself, so the file does not depend on the compiler or the machine:

```text
+-------------------------------------------------------------------------+
| 16 bytes   : MAGIC "ENLNG_C_EDB_V2" (14 characters, padded)             |
| 64 bytes   : database name                                              |
| int32      : number of tables saved (temporary tables are not saved)    |
+-------------------------------------------------------------------------+
| FOR EACH TABLE:                                                         |
|   - table name (64 bytes)                                               |
|   - view query (uint32 length + text; length 0 for a table)             |
|   - column count int32 (C)                                              |
|   - FOR EACH COLUMN:                                                    |
|       * name (64 bytes)                                                 |
|       * 5 flag bytes: kind (0 any, 1 integer, 2 real, 3 text,           |
|         4 boolean, 5 date), primary key, unique, not null, auto         |
|       * default, check, references table, references column            |
|         (each: uint32 length + text)                                    |
|       * next auto number int64                                          |
|   - index count int32; for each: name, unique byte, column count,       |
|     column names (length + text)                                        |
|   - row count uint32 (R); past 4,294,967,294 rows: 0xFFFFFFFF, uint64   |
|   - FOR EACH ROW: cell count int32, then for each cell                  |
|       a type byte (0 null, 1 int, 2 double, 3 string, 4 bool) and       |
|       the value (int64 / double / uint32 length + text / 1 byte)        |
+-------------------------------------------------------------------------+
```

Version 1 files (before column types) have the same header but write the columns as raw structs and have no view, index or constraint data. They are read, and written back as version 2 the next time the database is saved.

The row count was written in 4 bytes by the 32-bit programs (those of the installer, before) and in 8 bytes by a program built for 64 bits. Both kinds of file are read (after a count of 4 bytes, the next 4 bytes are 0 only in a file of 8: there they are the high half, and in a file of 4 they would be the number of cells of a row, the name of a table or the part after the tables, none of which starts with 0), and the count is written in 4 bytes again.

---

### 9.3 Indexes
The C engine keeps every table in memory and reads it in full for each query (`explain` shows this). Keys and `unique` columns are checked by looking through the rows. A plain `create index` is recorded and shown by `show indexes`, but does not change how fast queries run; a `unique` index is enforced.

---

### 9.4 Concurrency Control: Multi-Reader Single-Writer Locking (Python engine only; the C engine runs one script at a time)
The engine utilizes a dedicated `DatabaseLock` architecture:
* **Shared Read Locks:** Unlimited concurrent reader threads may query tables simultaneously.
* **Exclusive Write Locks:** Data modifications (`insert`, `update`, `delete`, `alter`) acquire an exclusive lock, safely blocking conflicting readers and writers.

---

### 9.5 MVCC & Row Versioning (`_version`)
> **C engine (`enlngdb.exe`, the playground, Studio):** there is no `_version` column to read.

Every row in EnlngDB automatically tracks an internal `_version` counter (beginning at `1`). Every update operation increments this counter, enabling multi-version optimistic concurrency control and audit tracking.

---

# 10. APIs, SDKs, CLI & Syntax Cheat Sheet

### 10.1 Pure C Embedding API (`enlngdb.h`)
Embed EnlngDB directly into any native C or C++ application:

```c
#include "enlngdb.h"

int main() {
    // 1. Initialize database instance
    EnlngDatabase* db = enlngdb_create("finance_db");

    // 2. Execute declarative statements directly
    enlngdb_execute_statement(db, "create table users with id, name, balance;", true);
    enlngdb_execute_statement(db, "insert into users with id: 1, name: 'Bibhu', balance: 50000;", true);
    
    // 3. Query the database
    enlngdb_execute_statement(db, "find all records from users where balance is at least 40000;", true);

    // 4. Save to binary .edb container
    enlngdb_save(db, "finance_db.edb");

    // 5. Clean up allocated heap memory
    enlngdb_free(db);
    return 0;
}
```

---

### 10.2 Python Native SDK Integration
Embed EnlngDB in Python backends, automation scripts, or data processing pipelines:

```python
from enlngdb.engine import NativeExecutionEngine
from enlngdb.compiler import run_enlngdb_source

# Initialize engine
engine = NativeExecutionEngine(stream_output=True)

script = """
type enlngdb
create table products with id, title, price
insert into products with id 1, title "MacBook M4", price 1999.99
insert into products with id 2, title "Magic Mouse", price 79.00
find records from products where price is under 100.00
"""

# Execute source code directly
results = run_enlngdb_source(script, engine=engine)
print(results)
```

---

### 10.3 CLI Tooling & Script Execution
Run EnlngDB scripts directly from your shell:

```bash
# Run a script
enlngdb script.enlngdb

# Run one statement
enlngdb -e "show tables"

# The version, and a short help text
enlngdb --version
enlngdb
```
There is no interactive prompt in the C engine; the playground on the Enlangg site runs scripts in the browser.

---

### 10.4 SQL-to-EnlngDB Cross-Reference Translation Table

| Standard SQL | EnlngDB Equivalent |
| :--- | :--- |
| `SELECT * FROM users;` | `find all records from users` |
| `SELECT id, name FROM users WHERE id = 1;` | `find id, name from users where id is 1` |
| `SELECT * FROM items WHERE price >= 100 ORDER BY price DESC LIMIT 5;` | `find top 5 items from items where price is at least 100 order by price descending` |
| `INSERT INTO users (id, name) VALUES (1, 'Bibhu');` | `insert into users with id 1, name "Bibhu"` |
| `UPDATE users SET balance = 5000 WHERE id = 1;` | `update records in users set balance = 5000 where id is 1` |
| `DELETE FROM users WHERE is_active = false;` | `delete records from users where is_active is false` |
| `DELETE FROM users;` | `delete all from users confirmed` |
| `DROP TABLE users;` | `drop table users confirmed` |
| `SELECT COUNT(*) FROM users;` | `count records in users` |
| `ALTER TABLE users ADD COLUMN age INT;` | `alter table users add column age as integer` |

---

### 10.5 Compiler Diagnostic & Error Recovery Guide

| Error Message | Root Cause | Solution |
| :--- | :--- | :--- |
| `Expected 'with' after table name` | Missing `with` clause during table creation | Write: `create table <tbl> with col1, col2` or `with:` |
| `Destructive operation is permanently blocked` | Destructive statement missing confirmation | Append `confirmed` at the end (e.g. `drop table users confirmed`) |
| `Duplicate key error: id='1' already exists` | Primary key or Unique constraint violation | Ensure the inserted value is unique across table rows |
| `Table '...' does not exist` | Querying an uninitialized table | Execute `create table <tbl> with ...` first or check spelling |
| `Expected 'from' or 'in' after field list` | Missing table connector in query | Use `find <fields> from <table>` or `find <fields> in <table>` |

---
*EnlngDB Documentation Manual — Built for sovereign, zero-dependency, ultra-fast natural database computing.*

---

# 11. Expressions, Subqueries, Constraints, Views & Transactions (C engine)

Everything in this section is implemented by the native C engine (`enlngdb.exe`, the playground and Studio) and is checked by `tests/test_enlngdb_sql.py`. It is written the way the rest of EnlngDB is: words first, and the engine says so instead of guessing.

### 11.1 Conditions
```enlngdb
find name from student where marks between 70 and 85          # not between
find name from student where city in ("Pune", "Goa")          # not in
find name from student where not city is "Pune"
find name from student where name like "A_h%"                 # % any text, _ one letter
find name from student where city is null                     # is not null
find name from student where (marks > 85 or marks < 75) and city is not null
```
A comparison with a missing value (`null`) is never true, and neither is its opposite: `city is not "Pune"` does not return a student with no city. Use `is null` to ask for the missing ones.

### 11.2 Subqueries
```enlngdb
find name from student where id in (find student_id from fees)
find name from student where marks > (find avg(marks) from student)
find name from student where exists (find fee_id from fees where student_id is student.id)
find t.name from (find name, marks from student where marks > 85) as t
```
A subquery that does not look at the row outside it runs once; one that does (like `student.id` above) runs for each row. A subquery used as a value must give one column and at most one row.

### 11.3 Totals, groups, `having`, sorting
```enlngdb
find city, count(*), avg(marks) from student group by city having count(*) > 1
find city, marks, count(*) from student group by city, marks
find count(distinct city), sum(marks * 2), group_concat(name) from student
find city as c, count(*) as n from student group by c order by n desc, c
find name, marks from student order by 2 desc limit 2        # 2 = the second thing shown
```
`group by` and `order by` take several expressions; a number or a name from the select list means that item. A total (`sum`, `count` ...) is not allowed in `where` (use `having`). In a grouped query a column that is not grouped shows its value from the first row of the group.

### 11.4 Expressions and functions
```enlngdb
find name, marks + 5, marks * 2 - 1, marks / 4, marks % 7 from student
find upper(name), lower(name), length(name), substr(name, 1, 2), name + "-" + city from student
find concat(name, "-", city), coalesce(city, "none"), replace(name, "a", "A"), trim(name) from student
find abs(marks - 90), round(avg(marks), 1), floor(marks / 7), ceil(marks / 7), sqrt(marks), power(2, 3), mod(7, 3) from student
find case when marks >= 90 then "A" when marks >= 80 then "B" else "C" end from student
find cast(marks as text), year("2024-03-15"), datediff("2024-03-15", "2024-03-10"), now() from student
select 1 + 1                                                  # no table needed
```
`+` adds numbers and joins texts (two texts are always joined: `"80" + "1"` is `"801"`). `||` always joins, as in SQL: `1 || 2` is `"12"`, and `null || "x"` is null. Dividing by zero is an error, not a silent null. Functions: `upper lower length trim ltrim rtrim substr substring left right replace concat instr reverse coalesce ifnull nullif if least greatest abs sign round floor ceil ceiling sqrt power pow mod cast year month day date datediff now today current_date current_time`. Aggregates: `count sum total avg average min max group_concat`.

Write a minus with spaces (`marks - 5`); `marks-5` is read as one word. Inside the `on` of a join, `-` and `*` are the join marks, so put arithmetic there in brackets.

### 11.5 Aliases, self joins, `using`
```enlngdb
find name as n, marks as m from student
find s.name, f.amount from student as s left join fees as f on s.id is f.student_id
find a.name from student a join student b on a.marks < b.marks where b.name is "Karan"
find f.fee_id, g.fee_id from fees f join fees g using (student_id) where f.fee_id < g.fee_id   # the column has the same name in both
find student.*, fees.amount from student join fees                 # every column of one table
```
`natural join` is not supported; write the `on` or use the key-name matching of section 7.3.

### 11.6 Combining queries: `union`, `intersect`, `except`, `with`
```enlngdb
find name from student where marks > 75 union find name from student where marks < 85      # repeats dropped
find name from student where marks > 75 union all find name from student where marks < 85
find name from student where marks > 75 intersect find name from student where marks < 85
find name from student where marks > 75 except find name from student where marks < 85
find name from a union find name from b order by name limit 3     # order by / limit once, at the end
with big as (find name, marks from student where marks > 75) find name from big order by name
```
The queries of a union must show the same number of columns. `with` can name several queries; each sees the ones before it.

### 11.7 Window functions
```enlngdb
find name, rank() over (order by marks desc) from student
find name, row_number() over (partition by city order by marks) from student
find name, sum(marks) over (partition by city) from student        # the total of the group, on every row
find name, sum(marks) over (order by id) from student              # a running total
find name, lag(marks) over (order by id), lead(marks, 1, 0) over (order by id) from student
```
`rank`, `dense_rank`, `row_number`, `lag`, `lead`, and `count sum avg min max` with `over (...)`. They can be used in the select list and `order by`, not in `where`.

### 11.8 Inserting
```enlngdb
insert into student with id 5, name "Eve"                          # the original way: columns by name
insert into student values (5, "Eve", 50, "Goa"), (6, "Fay", 60, "Goa")
insert into student (id, name) values (7, "Gus")
insert into copy (id, name) find id, name from student where marks > 75
create table top as find name, marks from student where marks > 75
```
Several rows go in together or not at all. A row whose key is already there replaces that row, and the engine says so (`1 row replaced: ... already had a row with id 2`). The key is the column(s) marked `primary key`; a table with no declared key uses a column called `id`. A column called `customer_id` or `code` is just a column.

### 11.9 Updating and deleting
```enlngdb
update student set marks = marks + 5, city = upper(name) where id is 1 and city is "Pune"
update student set marks = 0 where marks > 85 limit 1
in student change city to New Delhi where id is 4
delete from student where id is 1 or marks between 80 and 90
delete all from student confirmed
truncate table student confirmed
```
Every `where` is the full condition language above. The new values of an update are worked out from the old rows, and all rows are checked before any is written. A column that does not exist is an error that lists the columns.

### 11.10 Column types and constraints
```enlngdb
create table person (
    id integer primary key auto_increment,
    email text unique not null,
    age integer default 18 check (age >= 0),
    role text default "user",
    born date,
    owner integer references student(id)
)
```
The same list can be written in brackets and over several lines when a line ends with `(` or `,`. Types: `integer` (`int`), `real` (`float`, `decimal`, `number`), `text` (`varchar`), `boolean`, `date` (`2024-03-15`, optionally with a time). A value is turned into the column's type when that is safe (`"30"` into `30`) and refused when it is not, with the reason. Columns without a type keep whatever is written, as before.

* `primary key` (also `primary key (a, b)`) : the row's identity; writing it again replaces the row. A key cannot be empty (an `auto_increment` key is filled in).
* `unique` (also `unique (a, b)`), `not null` (or `required`), `default <value>`, `check (<condition>)`, `auto_increment`.
* `references table(column)` : a value must exist there; a row that other rows refer to cannot be deleted, its key cannot change, and its table cannot be dropped. There is no `on delete cascade`; delete the rows that refer first.

### 11.11 Altering tables
```enlngdb
alter table student add column age integer default 18
alter table student rename column name to full_name
alter table student modify column marks as text       # all values are converted, or nothing changes
alter table student drop column marks confirmed
alter table student rename to pupils                  # also: rename table student to pupils
```

### 11.12 Views, indexes, describing
```enlngdb
create view top as find name, marks from student where marks > 75     # create or replace view ...
find name from top order by name
show views
drop view top
create unique index u_name on student (name)          # create index on student (name, city)
show indexes from student
drop index u_name
describe student                                      # or: show columns from student
explain find name from student s join fees f order by name limit 2
```
A view is a stored query: it shows the rows as they are now, and cannot be inserted into. A `unique` index is enforced. A plain index is recorded and shown, but EnlngDB keeps tables in memory and reads them in full for every query, so it does not change speed (`explain` says so).

### 11.13 Transactions and databases
```enlngdb
begin
delete from student where id is 1
rollback                                              # undoes everything since begin; 'commit' keeps it
create database shop                                  # makes it and switches to it
drop database shop confirmed
```
A transaction covers the whole database (tables, rows, views). One that is still open when a script ends is rolled back, and the engine says so. There are no savepoints and no second writer: one script runs at a time.

### 11.14 What EnlngDB does not have
Reading only part of a database file: a database is read whole when a script opens it and written whole when a script that changed it ends (§12.8). Certificates for the server (it is trusted the first time, as ssh does, §12.12). `show databases` shows only the open database. `natural join`, recursive `with`, `on delete` / `on update`, savepoints, CSV files, `preview`, `undo`, triggers, procedures, users, rights and the server are in section 12.

---

# 12. Beyond SQL: files, safety, speed (C engine)

Everything here is checked by `tests/test_enlngdb_more.py`.

### 12.1 CSV files
```enlngdb
create table pupil with name text, roll_no integer primary key, class integer, stream text
insert into pupil values ("Asha", 1, 11, "Science"), ("Ravi, Jr", 2, 12, "Commerce"), ("Sita", 3, null, "Arts")
export pupil to "pupils.csv"
export find name, class from pupil where stream is not "Arts" to "science.csv"
create table copy_of_pupil with name text, roll_no integer primary key, class integer, stream text
import "pupils.csv" into copy_of_pupil
import "pupils.csv" into new_pupil              # a table that does not exist is made from the first line
```
The first line of the file names the columns. A value in quotes is text; without quotes, a number, `true`, `false`, or (when empty) `null`. A typed column turns `"30"` into 30 as usual. All rows go in or none: a line with the wrong number of values, or a row that breaks a rule, is reported with its line number and nothing is imported. `export` writes to a temporary file and moves it into place, and quotes a text that would otherwise read back as something else (`"1"`, `"a, b"`, `""`).

### 12.2 `preview` and `undo`
```enlngdb
preview delete from pupil where class is null       # shows the rows that would go, and changes nothing
preview update pupil set class = 12 where roll_no is 1
delete from pupil where class is null
undo                                                # the deleted row is back
```
`preview` runs the statement on a copy and shows each table it would change: the rows that would be deleted, changed (before and after) or added, tables made or dropped. `undo` takes back the last `update`, `delete`, `truncate`, `alter`, `drop` or `import`, once, and only while nothing else has changed after it (an `insert` after a `delete` means the `delete` can no longer be undone). It lives as long as the script runs. `undo` keeps only what the statement changed: the old values of the rows an update changed, the rows a delete took away (moved, not copied), the rows a truncate or a drop took away, the number of rows an import added, a copy of a table whose columns an alter changed. So it works on a database of any size, and costs about what the change itself costs. Undoing a `drop table` brings its triggers back too.

### 12.3 Savepoints
```enlngdb
begin
update pupil set class = 11 where roll_no is 2
savepoint before_delete
delete from pupil where roll_no is 2
rollback to before_delete                            # the row is back; the update is kept
release before_delete
commit
```
`rollback to` goes back to the savepoint and keeps it; `release` forgets it (and the ones after it). A transaction can have 16 savepoints. `begin` copies nothing: a transaction keeps records of what each statement in it changed (as `undo` does), so it costs what its changes cost, on a database of any size; `undo` inside a transaction takes back its last statement. Users, rights, triggers and procedures are not part of a transaction.

### 12.4 `natural join`
```enlngdb
create table mark with roll_no integer, subject text, score integer
insert into mark values (1, "Maths", 91), (2, "Maths", 78)
find name, subject, score from pupil natural join mark
find name, score from pupil natural left join mark
```
Every column name the two sides share must match. When there is none, or when two tables before the join share the name, the engine says so instead of guessing.

### 12.5 `on delete` and `on update`
```enlngdb
create table owner with id integer primary key, name text
create table cat with id integer primary key, owner_id integer references owner(id) on delete cascade
create table vet with id integer primary key, owner_id integer references owner(id) on delete set null
insert into owner values (1, "Asha"), (2, "Ravi")
insert into cat values (1, 1), (2, 2)
insert into vet values (1, 1)
delete from owner where id is 1                      # cat 1 goes too; vet 1 keeps its row with owner_id null
create table cls with id integer primary key, title text
create table stu with sid integer primary key, cid integer references cls(id) on update cascade
insert into cls values (1, "Maths")
insert into stu values (1, 1)
update cls set id = 10 where id is 1                 # stu 1 now has cid 10
```
Without `on delete ...` (or with `on delete restrict`) a row that others refer to cannot be deleted, as before; the error now says how to cascade. A cascade can go through several tables. `set null` on a `not null` column refuses the delete. `on update cascade` makes the rows that refer to a key follow it when the key changes; `on update set null` empties them; without it (or with `on update restrict`) a key that other rows refer to cannot be changed, and the error says how to cascade. Both settings are saved in the file in a way that older versions of `enlngdb` still read.

### 12.6 Recursive `with`
```enlngdb
with recursive n as (select 1 as x union all find x + 1 from n where x < 5) find x from n
create table emp with id, name, boss
insert into emp values (1, "Asha", null), (2, "Ravi", 1), (3, "Meera", 2)
with recursive team as (find id, name from emp where id is 1 union find e.id, e.name from emp e join team t on e.boss is t.id) find name from team
```
The first query gives the starting rows; the second runs again on the rows found in the round before, until a round finds nothing new (`union` also drops rows already found). A recursive query that never ends is stopped after 10,000 rounds or 1,000,000 rows, with an error.

### 12.7 One program at a time
While a script has a database open, the operating system holds a lock on `<name>.edb.lock`. A second program that opens the same database waits up to 5 seconds and then stops with: `database 'shop' is in use by another program`. The lock goes away when the first program ends, also when it crashes. A database file is written to `<name>.edb.tmp` first, made sure to be on the disk, and then put in the place of the old one in one step, so a crash or a power cut while saving leaves the old file or the new one, whole. (The tests kill the program 30 times while it changes and saves a database; the program before this one lost the file in such a test, because it removed the old file before it moved the new one in.) A script that only reads (it has only `find`, `select`, `show`, `count`, `describe`, `explain`, `export` and `preview`) does not write the file again.

### 12.8 Speed
The engine looks rows up by value with a hash, built for the statement that needs it: the key, `unique` columns and `references` when rows are inserted, a join on `a.x = b.y`, `in (subquery)`, `union`, `distinct`, `intersect`, `except`, and recursive `with`. The answers are the same as when every row is compared (`1`, `1.0` and `"1"` are equal, text is compared without case, a number with a fraction is compared one by one). Measured with 20,000 rows on one machine:

| Work | Before | Now |
| :--- | ---: | ---: |
| join 20,000 x 20,000 on a key | 234 s | 0.6 s |
| insert 20,000 rows into a table with `unique` and `auto_increment` | 15.1 s | 0.25 s |
| insert 20,000 rows that `reference` another table | 5.9 s | 0.5 s |
| `in (subquery)` over 20,000 rows | 4.4 s | 0.4 s |
| insert 20,000 rows in one statement (key) | 3.4 s | 0.4 s |
| 5,000 inserts, one statement each | 1.9 s | 0.3 s |
| 2,000 queries `where id is X` on 20,000 rows | 19.3 s | 0.34 s |
| 2,000 updates `where id is X` on 20,000 rows | 24.0 s | 0.15 s |
| 3,000 updates and deletes by key in one script (20,000 rows) | 38.7 s | 1.6 s |
| 500 inserts, one statement each, into a table of 100,000 rows (with opening and saving it) | 0.80 s | 0.25 s |
| 100 deletes `where id is X` on 200,000 rows | 4.74 s | 0.46 s |
| 300 inserts into one table, each followed by `where id is X` on another of 200,000 rows | 12.4 s | 0.54 s |
| 200 queries `where id between a and b` on 200,000 rows | 26.8 s | 0.58 s |
| 200 queries `where id > a and id < b` on 200,000 rows | 41.5 s | 0.63 s |
| 200 updates on 200,000 rows, each taken back with `undo` | refused | 0.46 s |
| 300 queries `where name is n7` (a bare word) on 200,000 rows | 53.6 s | 0.58 s |
| 300 queries `where id is a or id is b` on 200,000 rows | 55.4 s | 0.52 s |
| 300 queries `where name like "n1999%"` on 200,000 rows | 82.4 s | 1.6 s |
| 300 queries `where name like "%377%"` on 200,000 rows | 91.4 s | 1.05 s |

The lines with 200,000 rows include opening them (about 0.4 s); "refused" means `undo` said the database was too big for it.

The engine makes two kinds of index by itself, the first time a query needs one, on a table of 48 rows or more. A condition `column is value` (alone, or joined to others with `and`) uses a hash of the column. A condition `column > value`, `<`, `>=`, `<=` or `column between a and b` uses a sorted list of the column: numbers and texts are kept in two lists, because they do not sort together (`"10"` and `"9"` compare as texts with each other and as numbers with a number), so a bound that is a number reads its range of numbers and every text, and the other way round. An index is kept while the table does not change, through updates of other columns, through inserts, through deletes (the other rows only move up) and through `undo`, and while other tables change; it is made again after an alter or a change of its column. The whole `where` is still tested on the rows found, so the answer is the same as when every row is read; when a range would take most of the rows, the rows are read in order instead. So there is no need to write `create index` for speed: a plain `create index` is only recorded, a `unique` one is checked. A value can be a bare word, as in `where name is Ravi`. `a or b` uses the indexes when both sides can, and joins the rows they give. `column like "abc%"` (a fixed start, then `%` or `_`) reads the texts that start that way, and every number. Any other `like` (`"%dra%"`, `"%sha"`, or `"dra"`, which looks anywhere) uses a third index, made of every three letters of every text: it reads the rows that have every three letters of the pattern's longest part between `%` and `_`, and every number; a pattern whose longest part is shorter than three letters reads every row. So do `not` (most rows pass it anyway) and a condition on a calculation, like `id + 0 = 5`.

A database is read whole when a script opens it, and written whole when a script that changed it ends. One machine, a table of `id`, a text and a number: 1,000,000 rows are a 38 MB file and take about 180 MB of memory; a script opens them and answers a query in about 0.4 s, or changes a row and saves them in 0.7 s; `import` of a CSV file of them takes 1.5 s (79 s before: each batch of 1,024 rows made the lookup of the whole table again).

**Larger than memory.** Up to a budget the engine takes memory as usual. Past it, new memory comes from a file that the system keeps in memory only where it is used, and writes to the disk where it is not, so a database can be larger than the memory a program may take: the disk is the limit. The budget is half of the computer's memory, or what `ENLNGDB_MEMORY` says (`500M`, `2G`; `off` keeps everything in memory). The file is in the folder for temporary files, or in `ENLNGDB_SPILL_DIR`, and goes away when the program ends; when it cannot be made, the engine goes on with memory alone. `ENLNGDB_MEMSTAT=1` prints at the end how much was in memory and how much in the file at most. The answers are the same either way (the tests run every kind of statement with a budget of 4 MB, and compare); past the budget, the work goes at the speed of the disk. Measured on one machine, with the memory the program may take limited to 300 MB: 3,000,000 rows of four columns (a 214 MB file, about 1 GB in memory) did not fit with everything in memory; with `ENLNGDB_MEMORY=100M` they were imported in 37 s, and opened, changed and saved in 25 s, using at most 150 MB. The 1,000,000 rows above, with `ENLNGDB_MEMORY=30M`, took 40 MB, in the same time as with all of them in memory.

`enlngdb.exe` and `enlangg.exe` from the installer are 64-bit programs. A 32-bit build (gcc of MinGW.org) has 2 GB of addresses in all, so there a database cannot grow much past 1 GB in memory and 1 GB in the file.

What it does not do: a database is read whole when it is opened, also for a query that needs one row, and an index is made in memory from every row the first time a query needs it; there is no file of pages that reads only what a query needs.

### 12.9 Limits
| Thing | The most |
| :--- | ---: |
| tables in a database | 256 |
| columns in a table | 64 |
| columns in a result (a join) | 128 |
| indexes on a table | 8 |
| characters in a name | 63 |
| length of a statement, of a text value | no limit but memory |
| characters of a `default` / a `check` / the query of a view | 63 / 95 / 1023 |
| triggers setting each other off | 6 levels |

A name, a `default` or a `check` that is too long is an error; nothing is cut short.

### 12.10 Triggers and procedures
```enlngdb
create table account with id integer primary key, owner text, balance integer
create table ledger with id integer primary key auto_increment, what text, account_id integer, amount integer
insert into account values (1, "Asha", 100), (2, "Ravi", 50)
create trigger log_change after update on account when new.balance is not old.balance do insert into ledger (what, account_id, amount) values ("balance changed", new.id, new.balance - old.balance)
create trigger no_overdraft after update on account when new.balance < 0 do raise "the balance cannot go below zero"
update account set balance = balance - 30 where id is 1
find owner, balance from account
find what, account_id, amount from ledger
create procedure transfer (src, dst, amount) as
  update account set balance = balance - :amount where id is :src;
  update account set balance = balance + :amount where id is :dst
call transfer(1, 2, 20)
find owner, balance from account
show triggers
show procedures
drop trigger no_overdraft
create trigger tidy before insert on account do set new.owner = upper(new.owner)
create trigger floor_zero before update on account when new.balance < 0 do raise "no overdraft"
insert into account values (3, "meera", 0)            # kept as MEERA
```
A **trigger** is statements that run by themselves after a statement added, changed or deleted rows in a table: `create trigger NAME after insert|update|delete on TABLE [when CONDITION] do STATEMENT [; STATEMENT ...]`. Inside it `new.column` is the value in the row after the change (insert, update) and `old.column` the value before (update, delete); `when` limits it to the rows where the condition holds. EnlngDB finds the rows by comparing the table before and after the statement, so a trigger also runs for rows that `import`, a cascade or a procedure changed, once for each row. A trigger that fails, or that runs `raise "message"`, takes the whole statement back, with everything the other triggers did:

```text
update account set balance = balance - 500 where id is 2
ERROR: the trigger 'no_overdraft' stopped this statement: the balance cannot go below zero.
       Nothing was changed.
```

What a trigger's statements do is a statement of its own, so it can set off the triggers of the table it changes (up to 6 levels deep; a loop stops with an error and the statement is taken back). The output of statements inside a trigger is not shown.

A **before trigger** (`create trigger NAME before insert|update|delete on TABLE [when CONDITION] do ...`) runs for each row before the change counts. `set new.column = value` changes the row that is being added or changed (each `set` sees the values the ones before it gave), and `raise "message"` stops the statement. The rows a before trigger changed must still keep every rule of the table (`unique`, `check`, references), or the statement is taken back; it cannot change a column that another table refers to, and a `before delete` trigger has no new row to set. Its other statements (changing other tables, say) run once the statement's own rows are settled, before the after triggers, with the values `new.x` and `old.x` had at that point of the trigger; they are statements of their own and set off their own triggers, and if one fails the whole statement is taken back. Before triggers are kept in the file after the procedures, in a part that an older `enlngdb` does not read: it opens the file without them (and loses them if it saves it). A table that is dropped takes its triggers with it; `show triggers` lists them; `drop trigger NAME` removes one. A table without a key (and no column called `id`) is compared row by row: an update is found when no row was added or removed; a key that was changed in an update is paired with the row it replaced.

A **procedure** is statements saved under a name: `create procedure NAME (a, b) as STATEMENT [; STATEMENT ...]` and `call NAME(1, "x")`. `:a` in the statements stands for the value as it was written in the call. The statements may go over several lines: a line that ends with `as`, `do`, `begin` or `;` goes on. If one statement fails, everything the call did is taken back. Triggers and procedures are saved in the database file after the tables; older versions of `enlngdb` open such a file and ignore them. `preview` does not apply to `create trigger`/`create procedure`.

With triggers in the database, each statement that changes rows keeps a copy of the tables to find what changed, so such statements take longer on big tables.

### 12.11 Did you mean
A table, a column or a command word that is not there but looks like one that is gets a suggestion:

```text
find nme from student
ERROR: there is no column 'nme'. Did you mean 'name'? The columns are: name roll_no stream.
fnd name from student
  * Did you mean 'find'? (the statement starts with 'fnd')
```
A word that differs by one letter (two for a word of five or more letters), or by two letters swapped, is suggested. Nothing is changed or guessed: the statement still fails.

### 12.12 Users and the server
```enlngdb
create user boss password "boss-pass-1" admin
create user asha password "asha-pass-1"
grant select, update on account to asha
grant execute on procedure transfer to asha
show users
show grants for asha
revoke update on account from asha
```
Databases can be served to other programs: `enlngdb --serve NAME [NAME...] [--port 7171] [--host 127.0.0.1] [--key FILE]` opens `NAME.edb` (and the others) and keeps them open, and many clients sign in and run statements at the same time. Each database needs at least one user, made in a script that uses it:

```text
enlngdb --serve shop bank --port 7171
enlngdb --connect 7171/shop --user asha shop_report.enlngdb   (runs a script; the password is asked for)
enlngdb --connect 10.0.0.5:7171/bank --user asha               (each line typed is run; 'exit' leaves)
```
`/NAME` says which database to sign in to (it can be left out when the server serves one); a connection stays in that database, and each database has its own users and rights. The password is asked for without showing it, or read from `ENLNGDB_PASSWORD`. Lines piped into the client go as one script. The client's exit code is 1 when a statement failed or was refused.

**Encryption.** The connection is encrypted. The server has a key of its own (made the first time in `enlngdb_server.key`, which must be kept private) and prints its fingerprint when it starts. The first time a client connects to a server, it shows that fingerprint and asks whether it is the one the server printed, or takes it from `--trust FINGERPRINT`; it then remembers the key (in `~/.enlngdb_known_servers`, or the file `ENLNGDB_KNOWN_SERVERS` names) and refuses, from then on, a server at that address whose key is different, because someone may be in between. Each connection agrees on fresh keys (X25519 on both sides, with the server's own key, then HKDF with SHA-256), and every message is sealed with ChaCha20-Poly1305 (RFC 8439): it cannot be read, and a message changed, sent again or put out of order ends the connection. The code is the engine's own (no libraries); `enlngdb --selftest` checks it against known answers, and the tests compare it with Python's `cryptography` on random keys and messages. `--plain` (on both sides) turns encryption off, and only for `127.0.0.1`.

**Users.** `create user NAME password "..." [admin]` (8 characters or more), `alter user NAME password "..."` (a user may change their own), `drop user NAME`, `show users`. Only a salted hash of a password is kept in the file (PBKDF2 with SHA-256, 60,000 rounds, a random salt per user); a wrong password and an unknown name take the same time, and three wrong passwords end the connection.

**Rights.** `grant select, insert, update, delete (or all) on TABLE (or *) to USER`, `grant execute on procedure NAME to USER`, and `revoke ... from USER`; `show grants [for USER]`. An admin may do everything. A user who is not one may read the tables they may `select` (every table named in a statement counts, also in a join, a subquery, `with`, `union`, `describe` and `explain`, and also a name in quotes), change rows of the table a statement changes when they have that right, `call` a procedure they may `execute`, use `begin`, `commit`, `rollback`, savepoints and `undo`, and change their own password. Making, changing or dropping tables, views, indexes, triggers, procedures, users and rights is for an admin, and so are `import` and `export` (they read and write files on the server's computer) and `show users`, `show triggers`, `show procedures`. `insert ... with` cannot make a table or a column for a user who is not an admin. A view, a trigger and a procedure run with the rights of the admin who made them: granting `select` on a view lets a user read what the view shows. Rights on a table go when it is dropped and follow it when it is renamed.

**The server.** One request runs at a time, so clients never see half a change. While one client has a transaction open, the requests of the others wait for it; after 10 seconds they are answered `busy` and nothing of them runs. A client that goes away with a transaction open has it rolled back. After a change, the database is saved as soon as no transaction is open, and again when the server stops (Ctrl+C, or an admin running `shutdown server`). `undo` takes back only a change the same client made. `use database` is refused for any database but the one served.

What it does not do: there are no certificates, so the first connection to a server is only as safe as the check of its fingerprint (as with ssh). By default the server listens only on this computer (`127.0.0.1`); `--host 0.0.0.0` lets other computers in. Rights are checked by the server only: a program that opens `NAME.edb` itself (with `enlngdb script.enlngdb`) is its owner, and nothing is checked, because whoever can read the file can read the data. Users, rights, triggers and procedures are not part of a transaction or of `undo`. An older `enlngdb` opens a file with users and ignores them (and loses them if it saves it). The browser playground has no server.
