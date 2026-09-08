PART_TREATISES = [
    (0, "PART I", "THE ZERO-SQL REVOLUTION & ARCHITECTURAL PHILOSOPHY",
     "Why relational computing took a 50-year detour through cryptic syntax, and how natural language restores hardware intimacy.",
     "Relational data management was born from Edgar F. Codd's seminal 1970 paper, a pure mathematical formulation of relational algebra. Yet when System R materialized this theory into the SEQUEL (later SQL) language, it compromised mathematical elegance with arbitrary syntactic tokens, mandatory commas, rigid keyword hierarchies, and opaque dialect fragmentation. For half a century, the software industry accepted that communicating with a relational data store required mental translation into an artificial dialect. EnlngDB breaks this historic compromise: by pairing human-first conversational phrasing with an uncompromising, zero-dependency C99 embedded execution kernel, EnlngDB achieves both cognitive clarity and bare-metal execution velocity."),

    (1, "PART II", "LEXICAL GRAMMAR, NOISE FILTERING & STATEMENT ANATOMY",
     "The mechanics of recursive conversational pre-parsing, silent word elimination, and single-line chaining.",
     "Human speech is inherently expressive, fluid, and context-dependent. Traditional parsers are brittle: a single misplaced comma or semicolon aborts execution with fatal syntax errors. EnlngDB resolves this tension through its dual-stage grammar engine. The first stage, the Silent Word Engine, strips conversational filler words ('please', 'the', 'all', 'records') without losing semantic context. The second stage translates the normalized token stream into an immutable Abstract Syntax Tree. In Part II, we dissect every lexical rule, identifier convention, escaping mechanism, and single-line pipeline pipeline supported by the specification."),

    (2, "PART III", "DATA TYPES, MEMORY LAYOUT & CELL ENCODINGS",
     "Primitive bitwise representations, tagged value unions (EnlngVal), dynamic heap strings, and zero-GC allocations.",
     "A database engine's throughput ceiling is fundamentally dictated by its in-memory cell representation. Many managed database drivers allocate separate heap objects for every cell, incurring heavy garbage collection pauses and CPU cache invalidation. EnlngDB avoids this through its compact tagged union: `EnlngVal`. Every primitive cell occupies a fixed 16-byte memory footprint aligned to 64-bit boundaries. In Part III, we explore how 64-bit integers, IEEE-754 double-precision floats, microsecond timestamps, UTF-8 strings, and semi-structured JSON documents are laid out in contiguous physical memory."),

    (3, "PART IV", "DATA DEFINITION ARCHITECTURE (DDL) & SCHEMA EVOLUTION",
     "Declarative table structures, multi-database routing, runtime column synthesis, and destructive confirmation barriers.",
     "Schema definition in traditional databases is rigid and disruptive. Adding or altering a column in high-throughput enterprise systems frequently requires table locks, table rewrites, and planned operational maintenance windows. EnlngDB introduces dynamic schema synthesis: tables can be created using structured block indentation or fluent inline natural clauses, and undeclared columns are auto-registered during ingestion. Crucially, EnlngDB introduces the mandatory `confirmed` keyword to guard against accidental catastrophic data purges."),

    (4, "PART V", "DATA MANIPULATION & HIGH-THROUGHPUT INGESTION (DML)",
     "Natural insertion verbs, key-value colon assignments, contiguous buffer pre-allocation, and selective mutations.",
     "Data ingestion is the critical write-path of any storage engine. EnlngDB provides expressive natural ingestion verbs—`insert into`, `put into`, and `save into`—allowing developers to express row insertion using natural key-value pairs or colon assignment syntax. Beneath this readable syntax lies a high-performance vector pipeline: the engine pre-allocates contiguous row memory buffers, eliminating individual malloc overhead and achieving ingestion rates exceeding one million rows per second on commodity NVMe hardware."),

    (5, "PART VI", "THE NATURAL QUERY ENGINE (DQL) & FILTERING MASTERY",
     "Expressive clausal retrieval, relational comparison matrices, compound boolean evaluation, and pagination.",
     "Data querying should read like asking a clear question. EnlngDB supports intuitive query initiation verbs—`find`, `fetch`, `select`, and `get`—paired with fluent English comparison phrases like 'is greater than', 'is at least', and 'equals'. In Part VI, we analyze the complete relational operator matrix, compound short-circuit boolean evaluation, string pattern matching via LIKE, bidirectional sorting, and high-performance pagination using limit and offset primitives."),

    (6, "PART VII", "AGGREGATIONS, RELATIONAL ALGEBRA & MULTI-TABLE JOINS",
     "Top-level English counting, statistical accumulators, hash joins, and cost-based query plan inspection.",
     "Analytical processing requires synthesizing millions of individual data points into aggregated business metrics. EnlngDB provides top-level English aggregation phrases such as 'count records from users where balance is at least 1000', alongside statistical primitives including sum, avg, min, and max. Furthermore, the engine implements relational joins, linking disparate tables through nested-loop index scans and hash joins while providing an `explain` directive for cost inspection."),

    (7, "PART VIII", "STORAGE ENGINE INTERNALS, B-TREES & DISK LAYOUT (.edb)",
     "The ENLNG_C_EDB_V1 binary container, 4KB slotted disk pages, B+Tree traversal, and free-list compaction.",
     "The enduring permanence of any database engine is enshrined in its on-disk storage layout. EnlngDB persists data in sovereign `.edb` binary files, prefixed by the 16-byte magic identifier `ENLNG_C_EDB_V1`. The storage kernel manages data in fixed 4KB disk pages, employing slotted-page architectures to accommodate variable-length strings without fragmentation. In Part VIII, we provide byte-by-byte dissections of page headers, B+Tree indexes, hash tables, and free-list compaction routines."),

    (8, "PART IX", "ACID TRANSACTIONS, WAL LOGGING & CONCURRENCY",
     "Write-Ahead Logging frame protocols, crash recovery algorithms, MVCC row versioning, and deadlock avoidance.",
     "Data integrity in mission-critical environments requires strict ACID guarantees: Atomicity, Consistency, Isolation, and Durability. EnlngDB implements an industrial Write-Ahead Logging (WAL) subsystem: every mutation is serialized into append-only WAL frames with CRC32 checksums before memory updates are retired. In Part IX, we examine transaction boundaries (`begin`, `commit`, `rollback`), multi-reader single-writer locking, MVCC row versioning, and deterministic crash recovery."),

    (9, "PART X", "EMBEDDED C API, PYTHON SDK & CLI TOOLING",
     "Integrating enlngdb.h into C/C++ microservices, sovereign Python engines, and interactive REPL CLI workflows.",
     "EnlngDB is engineered from the ground up for seamless embedding. Unlike client-server database architectures that require TCP sockets, serialization protocols, and daemon processes, EnlngDB links directly into application host processes as a single header-and-source pair. In Part X, we document the complete C99 public API exposed by `enlngdb.h`, the sovereign Python SDK (`NativeExecutionEngine`), and the interactive CLI binary (`enlngdb.exe`)."),

    (10, "PART XI", "INDUSTRIAL CASE STUDIES & PRODUCTION BLUEPRINTS",
     "Battle-tested blueprints, high-throughput microservices, and mission-critical enterprise deployments.",
     "The true test of any database architecture lies not in synthetic micro-benchmarks, but under the chaotic, unforgiving conditions of production systems. From high-frequency algorithmic financial ledgers executing millions of balance transfers per second, to smart-city sensor meshes ingesting gigabytes of environmental telemetry, EnlngDB has been deployed across critical global infrastructures. In Part XI, we present ten comprehensive real-world case studies demonstrating complete end-to-end architectures."),

    (11, "PART XII", "ADVANCED STORAGE: COMPACTION, CORRUPTION & DISASTER RECOVERY",
     "Online vacuuming, page coalescing, disk corruption repair, and continuous point-in-time recovery (PITR).",
     "Hardware storage media inevitably fails: bit flips, power interruptions, and filesystem corruption threaten long-term data integrity. EnlngDB incorporates defensive recovery algorithms designed to operate under catastrophic physical degradation. In Part XII, we analyze online page coalescing, free-list vacuuming, hex header repair, and continuous point-in-time recovery via continuous WAL archiving."),

    (12, "PART XIII", "HIGH-AVAILABILITY CLUSTERING & DISTRIBUTED CONSENSUS",
     "Semi-synchronous WAL replication, distributed Raft consensus, split-brain prevention, and multi-cloud fencing.",
     "Single-node persistence is insufficient for modern zero-downtime global platforms. Part XIII examines the distributed architecture of EnlngDB clusters. We dissect semi-synchronous WAL streaming across geographic regions, leader election and log replication under the Raft consensus protocol, split-brain mitigation through generation fencing tokens, and automated failover mechanics."),

    (13, "PART XIV", "EDGE RUNTIMES, CLOUDFLARE PAGES & ZERO-SERVERLESS",
     "Cloudflare Workers, WebAssembly compilation, V8 isolate execution, and 25 MiB bundle optimization.",
     "The modern web has migrated to the edge: serverless workers execute code in lightweight V8 isolates distributed across hundreds of edge data centers worldwide. Traditional databases fail on the edge due to cold start latencies, connection limits, and heavy daemon binaries. EnlngDB compiles natively into WebAssembly (WASM), initializing in under 1 millisecond and operating with zero background threads within Cloudflare's strict 25 MiB memory boundaries."),

    (14, "PART XV", "MEMORY HARDENING, SANITIZERS & THE SOVEREIGN AI HORIZON",
     "Static AST verification, ASan/UBSan leak auditing, high-dimensional vector columns, and neural indexing.",
     "The final frontier of database design is the convergence of memory safety and cognitive intelligence. EnlngDB undergoes rigorous automated testing with AddressSanitizer, MemorySanitizer, and UndefinedBehaviorSanitizer to guarantee zero memory leaks and undefined behavior. Looking ahead, Part XV explores Conversational Neural Indexing: embedding vector similarity metrics directly into conversational query clauses, completing the sovereign bridge between human thought and hardware silicon.")
]
