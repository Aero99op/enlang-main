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
   - 7.3 Multi-Table Relational Joins (`inner join`, `left join`, `right join`)
8. [Data Modification & Purging (DML Mutations)](#8-data-modification--purging-dml-mutations)
   - 8.1 Conversational Record Updates (`update`, `change`, `modify`, `in <table> update set`)
   - 8.2 Safe Row Deletions (Filtered vs Unconstrained Purge)
   - 8.3 Column Removal
9. [Persistence, Storage Internals & Concurrency](#9-persistence-storage-internals--concurrency)
   - 9.1 Disk Persistence Commands (`save database to`, `open database to`)
   - 9.2 Binary File Specification (`ENLNG_C_EDB_V1`)
   - 9.3 In-Memory Indexing & Automatic Re-indexing
   - 9.4 Concurrency Control: Multi-Reader Single-Writer Locking (`DatabaseLock`)
   - 9.5 MVCC & Row Versioning (`_version`)
10. [APIs, SDKs, CLI & Syntax Cheat Sheet](#10-apis-sdks-cli--syntax-cheat-sheet)
    - 10.1 Pure C Embedding API (`enlngdb.h`)
    - 10.2 Python Native SDK Integration
    - 10.3 CLI Tooling & Script Execution (`enlngdb.exe`)
    - 10.4 SQL-to-EnlngDB Cross-Reference Translation Table
    - 10.5 Compiler Diagnostic & Error Recovery Guide

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
| **Storage Format** | Native binary `.edb` (`ENLNG_C_EDB_V1`) | Native `.edb` / atomic JSON format (`ENLNGDB_SOVEREIGN_V1`) |
| **Concurrency** | Single-threaded atomic script executor | Readers-Writer lock (`DatabaseLock` via `threading.RLock`) |
| **Binary Footprint**| ~100 KB stand-alone executable | Pure Python modules (wheel package) |

---

### 1.3 Storage Formats: `.edb` Binary vs Native Memory Snapshots
* **`.edb` (Enlng Database Binary):** A high-performance, compact file format beginning with the 14-byte magic signature `ENLNG_C_EDB_V1`. It contains table definitions, column types, primary key flags, and packed row values.
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
EnlngDB supports relational joins using intuitive dot-notation syntax:

```enlngdb
# Inner Join
find users.full_name, orders.total from users 
inner join orders on users.id is orders.user_id 
where orders.total is greater than 1000

# Left Outer Join
select customers.name, invoices.amount from customers 
left join invoices on customers.id = invoices.customer_id
```

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

### 9.2 Binary File Specification (`ENLNG_C_EDB_V1`)
The pure C native engine persists tables directly using a compact binary layout:

```text
+-------------------------------------------------------------------------+
| BYTE 0 - 13   : MAGIC IDENTIFIER "ENLNG_C_EDB_V1"                       |
| BYTE 14 - 77  : Database Name (64 bytes null-terminated ASCII)          |
| BYTE 78 - 81  : Table Count uint32_t (N)                                |
+-------------------------------------------------------------------------+
| FOR EACH TABLE (0 to N-1):                                              |
|   - Table Name (64 bytes)                                               |
|   - Column Count uint32_t (C)                                           |
|   - FOR EACH COLUMN:                                                    |
|       * Name (64 bytes)                                                 |
|       * Type uint32_t (0=NULL, 1=INT, 2=DOUBLE, 3=STRING, 4=BOOL)       |
|       * is_primary_key uint8_t, is_unique uint8_t                       |
|   - Row Count uint64_t (R)                                              |
|   - FOR EACH ROW:                                                       |
|       * Row Version uint32_t                                            |
|       * FOR EACH CELL:                                                  |
|           - Type uint8_t                                                |
|           - Value Payload (int64 / double / length-prefixed string)    |
+-------------------------------------------------------------------------+
```

---

### 9.3 In-Memory Indexing & Automatic Re-indexing
* **Primary Key Index:** A direct hash map mapping primary key values to internal row offsets ($O(1)$ lookup time).
* **Unique Index:** Prevents duplicate keys by maintaining an inverted index.
* **Auto-Rebuild:** Whenever an `update`, `delete`, or `alter table` mutation occurs, table indexes are rebuilt automatically in memory.

---

### 9.4 Concurrency Control: Multi-Reader Single-Writer Locking
The engine utilizes a dedicated `DatabaseLock` architecture:
* **Shared Read Locks:** Unlimited concurrent reader threads may query tables simultaneously.
* **Exclusive Write Locks:** Data modifications (`insert`, `update`, `delete`, `alter`) acquire an exclusive lock, safely blocking conflicting readers and writers.

---

### 9.5 MVCC & Row Versioning (`_version`)
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
# Execute script using native Pure C engine binary
./enlngdb.exe run script.enlngdb

# Launch interactive conversational REPL
./enlngdb.exe
```

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
