import os
from reportlab.lib import colors
from reportlab.platypus import Paragraph, Spacer, Table, TableStyle, PageBreak, HRFlowable
from book_styles import make_code_box

def generate_all_appendices(styles):
    story = []

    # =========================================================================
    # APPENDIX A: COMPLETE FORMAL EBNF SPECIFICATION
    # =========================================================================
    story.append(Paragraph("<b>APPENDIX A</b>", styles['DbPartRoman']))
    story.append(Paragraph("<b>Complete Formal EBNF Grammar &amp; Lexical Specification</b>", styles['DbPartTitle']))
    story.append(HRFlowable(width="100%", thickness=1.5, color=colors.HexColor("#0284c7"), spaceBefore=4, spaceAfter=14))
    story.append(Paragraph(
        "The following Extended Backus-Naur Form (EBNF) specification defines the complete, unambiguous "
        "syntactic grammar for EnlngDB version 2.0.0. All keywords are case-insensitive. Conversational filler words "
        "are eliminated during the pre-parsing normalization phase:",
        styles['DbBodyLead']
    ))
    ebnf_code = """
(* EnlngDB Formal Grammatical Specification (ISO/IEC 14977 EBNF) *)

program          ::= domain_header? statement_sequence
domain_header    ::= "type" [ "database" | "enlngdb" ] ( NEWLINE | SEMICOLON )
statement_sequence ::= ( statement ( NEWLINE | SEMICOLON )* )*

statement        ::= ddl_statement 
                   | dml_statement 
                   | dql_statement 
                   | admin_statement 
                   | transaction_statement

(* DDL: Data Definition Statements *)
ddl_statement    ::= create_table_stmt 
                   | drop_table_stmt 
                   | alter_table_stmt 
                   | vacuum_stmt

create_table_stmt ::= "create" "table" [ "if" "not" "exists" ] IDENTIFIER 
                      "with" ( "(" col_def_list ")" | col_def_list | ":" indent_col_list )
col_def_list     ::= col_def ( "," col_def )*
col_def          ::= IDENTIFIER [ col_type ] [ constraint_list ]
col_type         ::= "integer" | "int" | "number" | "text" | "string" | "real" | "float" | "bool" | "boolean"
constraint_list  ::= ( "primary" "key" | "unique" | "autoincrement" | "not" "null" )*

drop_table_stmt  ::= "drop" "table" [ "if" "exists" ] IDENTIFIER "confirmed"
alter_table_stmt ::= "alter" "table" IDENTIFIER 
                     ( "add" [ "column" ] col_def 
                     | "drop" [ "column" ] IDENTIFIER "confirmed" )
vacuum_stmt      ::= "vacuum" ( "database" | "table" IDENTIFIER )

(* DML: Data Manipulation Statements *)
dml_statement    ::= insert_stmt | update_stmt | delete_stmt

insert_stmt      ::= ( "insert" [ "record" | "into" ]* | "put" "into" | "save" "into" ) 
                     IDENTIFIER "with" assign_list
update_stmt      ::= "in" [ "table" ] IDENTIFIER ( "change" | "update" | "set" ) assign_list "where" condition
                   | "update" IDENTIFIER "set" assign_list "where" condition
delete_stmt      ::= ( "delete" [ "records" ] "from" | "remove" "from" ) IDENTIFIER "where" condition
                   | ( "delete" "all" "from" | "remove" "all" "from" ) IDENTIFIER "confirmed"

assign_list      ::= assign_pair ( "," assign_pair )*
assign_pair      ::= IDENTIFIER ( ":" | "=" | "to" | "is" | WHITESPACE ) literal

(* DQL: Data Query Statements *)
dql_statement    ::= query_init [ "all" ] [ "records" ] [ distinct_opt ] 
                     proj_list ( "from" | "in" ) IDENTIFIER [ "where" condition ] 
                     [ "order" "by" order_spec ] [ "limit" INTEGER [ "offset" INTEGER ] ]
query_init       ::= "find" | "fetch" | "select" | "get"
distinct_opt     ::= "distinct" | "unique"
proj_list        ::= "*" | col_name ( "," col_name )*

count_stmt       ::= "count" [ "records" | "all" ] ( "from" | "in" ) IDENTIFIER [ "where" condition ]
agg_stmt         ::= ( "sum" | "avg" | "min" | "max" ) "(" IDENTIFIER ")" 
                     ( "from" | "in" ) IDENTIFIER [ "where" condition ]

(* Relational Expressions & Predicates *)
condition        ::= expr_term ( ( "and" | "or" ) expr_term )*
expr_term        ::= [ "not" ] IDENTIFIER op_phrase literal | "(" condition ")"
op_phrase        ::= "is" | "is" "equal" "to" | "equals" | "==" | "=" 
                   | "is" "not" | "is" "not" "equal" "to" | "!="
                   | "is" "greater" "than" | "greater" "than" | ">" | "is" "above"
                   | "is" "at" "least" | "is" "greater" "than" "or" "equal" "to" | ">="
                   | "is" "less" "than" | "less" "than" | "<" | "is" "under"
                   | "is" "at" "most" | "is" "less" "than" "or" "equal" "to" | "<="
                   | "like" | "contains"

(* Transactions & System Directives *)
transaction_statement ::= "begin" [ "transaction" ] 
                        | "commit" [ "transaction" ] 
                        | "rollback" [ "transaction" ]
admin_statement  ::= "show" ( "tables" | "databases" )
                   | "use" [ "database" ] IDENTIFIER
                   | "save" [ "database" ] [ "to" STRING_LITERAL ]
                   | "open" [ "database" ] [ "from" STRING_LITERAL ]
                   | "explain" statement

(* Lexical Terminals *)
IDENTIFIER       ::= [A-Za-z_][A-Za-z0-9_]*
STRING_LITERAL   ::= '"' [^"\\r\\n]* '"' | "'" [^'\\r\\n]* "'"
INTEGER          ::= [0-9]+
REAL             ::= [0-9]+ "." [0-9]+
BOOLEAN          ::= "true" | "false" | "yes" | "no"
literal          ::= STRING_LITERAL | INTEGER | REAL | BOOLEAN
"""
    story.extend(make_code_box(ebnf_code))
    story.append(PageBreak())

    # =========================================================================
    # APPENDIX B: PURE C HEADER REFERENCE (enlngdb.h)
    # =========================================================================
    story.append(Paragraph("<b>APPENDIX B</b>", styles['DbPartRoman']))
    story.append(Paragraph("<b>Pure C Embedded Header Reference (enlngdb.h)</b>", styles['DbPartTitle']))
    story.append(HRFlowable(width="100%", thickness=1.5, color=colors.HexColor("#0284c7"), spaceBefore=4, spaceAfter=14))
    story.append(Paragraph(
        "The complete, sovereign C99 Application Binary Interface (ABI) exposed by `enlngdb.h`. Linkable against "
        "any C/C++ codebase with zero external dynamic dependencies and zero runtime memory bloat:",
        styles['DbBodyLead']
    ))
    c_header_code = """
#ifndef ENLNGDB_H
#define ENLNGDB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define ENLNGDB_VERSION "2.0.0-pure-c-native"
#define ENLNGDB_MAX_COLS 32
#define ENLNGDB_MAX_TABLES 64
#define ENLNGDB_MAX_NAME 64
#define ENLNGDB_MAGIC "ENLNG_C_EDB_V1"

typedef enum {
    ENLNG_VAL_NULL = 0,
    ENLNG_VAL_INT,
    ENLNG_VAL_DOUBLE,
    ENLNG_VAL_STRING,
    ENLNG_VAL_BOOL
} EnlngValType;

typedef struct {
    EnlngValType type;
    union {
        int64_t int_val;
        double double_val;
        char* str_val;
        bool bool_val;
    };
} EnlngVal;

typedef enum {
    OP_NONE = 0, OP_EQ, OP_NEQ, OP_GT, OP_LT, OP_GTE, OP_LTE, OP_LIKE
} EnlngOp;

typedef struct {
    char name[ENLNGDB_MAX_NAME];
    EnlngValType type;
    bool is_primary_key;
    bool is_unique;
} EnlngColumn;

typedef struct {
    EnlngVal* cells;
    int cell_count;
    int version;
} EnlngRow;

typedef struct {
    char name[ENLNGDB_MAX_NAME];
    EnlngColumn columns[ENLNGDB_MAX_COLS];
    int col_count;
    EnlngRow* rows;
    size_t row_count;
    size_t row_capacity;
    int version;
} EnlngTable;

typedef struct {
    char name[ENLNGDB_MAX_NAME];
    EnlngTable tables[ENLNGDB_MAX_TABLES];
    int table_count;
    char db_filepath[256];
} EnlngDatabase;

/* Core Database Management */
EnlngDatabase* enlngdb_create(const char* name);
void enlngdb_free(EnlngDatabase* db);
EnlngTable* enlngdb_create_table(EnlngDatabase* db, const char* name, const EnlngColumn* cols, int col_count);
EnlngTable* enlngdb_get_table(EnlngDatabase* db, const char* name);
bool enlngdb_drop_table(EnlngDatabase* db, const char* name, bool confirmed);

/* Row Operations */
bool enlngdb_insert_row(EnlngTable* table, const EnlngVal* cells, int cell_count);
size_t enlngdb_update_rows(EnlngTable* table, const char* col_name, const EnlngVal* new_val, int cond_col, EnlngOp op, const EnlngVal* cond_val);
size_t enlngdb_delete_rows(EnlngTable* table, int cond_col, EnlngOp op, const EnlngVal* cond_val);
void enlngdb_truncate_table(EnlngTable* table, bool confirmed);

/* Script & Statement Execution */
bool enlngdb_execute_statement(EnlngDatabase* db, const char* statement, bool print_output);
int enlngdb_execute_script(EnlngDatabase* db, const char* script_content, bool print_output);

/* Atomic Disk Persistence */
bool enlngdb_save(const EnlngDatabase* db, const char* filepath);
bool enlngdb_load(EnlngDatabase* db, const char* filepath);

/* Diagnostic & Performance Profiling */
void enlngdb_print_stats(const EnlngDatabase* db);
int64_t enlngdb_get_allocated_bytes(void);

#endif /* ENLNGDB_H */
"""
    story.extend(make_code_box(c_header_code))
    story.append(PageBreak())

    # =========================================================================
    # APPENDIX C: MASTER DIAGNOSTIC & ERROR RECOVERY REGISTRY
    # =========================================================================
    story.append(Paragraph("<b>APPENDIX C</b>", styles['DbPartRoman']))
    story.append(Paragraph("<b>Master Diagnostic &amp; Error Recovery Registry</b>", styles['DbPartTitle']))
    story.append(HRFlowable(width="100%", thickness=1.5, color=colors.HexColor("#0284c7"), spaceBefore=4, spaceAfter=14))
    story.append(Paragraph(
        "A comprehensive directory of all compiler, parser, and runtime error codes emitted by EnlngDB, "
        "including exact root cause diagnoses and remediation steps:",
        styles['DbBodyLead']
    ))
    err_data = [
        [Paragraph("<b>Diagnostic Code</b>", styles['DbH3']),
         Paragraph("<b>Root Cause Diagnosis</b>", styles['DbH3']),
         Paragraph("<b>Corrective Remediation</b>", styles['DbH3'])],
        [Paragraph("ERR_EXPECTED_WITH", styles['DbBody']),
         Paragraph("Table creation missing the mandatory 'with' connector keyword.", styles['DbBody']),
         Paragraph("Use: <code>create table &lt;tbl&gt; with col1, col2</code>", styles['DbBody'])],
        [Paragraph("ERR_DESTRUCTIVE_BLOCKED", styles['DbBody']),
         Paragraph("Destructive statement (drop/truncate) executed without 'confirmed'.", styles['DbBody']),
         Paragraph("Append 'confirmed': <code>drop table &lt;tbl&gt; confirmed</code>", styles['DbBody'])],
        [Paragraph("ERR_TABLE_NOT_FOUND", styles['DbBody']),
         Paragraph("Query or mutation references a table that does not exist in active database.", styles['DbBody']),
         Paragraph("Verify table name via <code>show tables</code> or run <code>create table</code>", styles['DbBody'])],
        [Paragraph("ERR_DUPLICATE_KEY", styles['DbBody']),
         Paragraph("Inserted record violates a PRIMARY KEY or UNIQUE constraint.", styles['DbBody']),
         Paragraph("Supply a distinct key value or verify existing table records", styles['DbBody'])],
        [Paragraph("ERR_EXPECTED_FROM_IN", styles['DbBody']),
         Paragraph("Query statement missing 'from' or 'in' before table identifier.", styles['DbBody']),
         Paragraph("Write: <code>find records from &lt;tbl&gt; where ...</code>", styles['DbBody'])],
        [Paragraph("ERR_MAX_COLS_EXCEEDED", styles['DbBody']),
         Paragraph("Table schema exceeds ENLNGDB_MAX_COLS (32 columns maximum).", styles['DbBody']),
         Paragraph("Normalize schema or increase ENLNGDB_MAX_COLS in <code>enlngdb.h</code>", styles['DbBody'])],
        [Paragraph("ERR_CORRUPT_EDB_FILE", styles['DbBody']),
         Paragraph("Loaded .edb file lacks valid 'ENLNG_C_EDB_V1' magic identifier.", styles['DbBody']),
         Paragraph("Verify file is not truncated or rebuild from source .enlngdb script", styles['DbBody'])],
        [Paragraph("ERR_TYPE_MISMATCH", styles['DbBody']),
         Paragraph("Comparison predicate operates across incompatible types (e.g. string vs int).", styles['DbBody']),
         Paragraph("Ensure literals match column types in WHERE condition", styles['DbBody'])],
        [Paragraph("ERR_SYNTAX_EMPTY_QUERY", styles['DbBody']),
         Paragraph("Execution engine received a null or empty command buffer.", styles['DbBody']),
         Paragraph("Provide valid EnlngDB statement string or script", styles['DbBody'])],
        [Paragraph("ERR_LOCK_TIMEOUT", styles['DbBody']),
         Paragraph("Transaction timed out waiting for exclusive write lock.", styles['DbBody']),
         Paragraph("Commit or rollback active transactions to release lock", styles['DbBody'])],
        [Paragraph("ERR_WAL_CHECKSUM_FAIL", styles['DbBody']),
         Paragraph("WAL recovery frame failed CRC32 checksum validation.", styles['DbBody']),
         Paragraph("Run database recovery tool with <code>--repair</code> flag", styles['DbBody'])],
        [Paragraph("ERR_BUFFER_OVERFLOW", styles['DbBody']),
         Paragraph("Token length exceeds maximum identifier buffer (64 bytes).", styles['DbBody']),
         Paragraph("Truncate identifier names to 63 characters or fewer", styles['DbBody'])],
        [Paragraph("ERR_TRANSACTION_ACTIVE", styles['DbBody']),
         Paragraph("Attempted to start new transaction while already in active block.", styles['DbBody']),
         Paragraph("Execute <code>commit</code> or <code>rollback</code> before <code>begin</code>", styles['DbBody'])],
        [Paragraph("ERR_TRANSACTION_NONE", styles['DbBody']),
         Paragraph("Executed <code>commit</code> or <code>rollback</code> with no transaction.", styles['DbBody']),
         Paragraph("Ensure <code>begin</code> was executed prior to transaction close", styles['DbBody'])],
    ]
    t_err = Table(err_data, colWidths=[130, 180, 190])
    t_err.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#f1f5f9")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#cbd5e1")),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
        ('LEFTPADDING', (0,0), (-1,-1), 6),
        ('RIGHTPADDING', (0,0), (-1,-1), 6),
    ]))
    story.append(t_err)
    story.append(PageBreak())

    # =========================================================================
    # APPENDIX D: 100 ANSI SQL TO SOVEREIGN ENLNGDB ROSETTA STONE
    # =========================================================================
    story.append(Paragraph("<b>APPENDIX D</b>", styles['DbPartRoman']))
    story.append(Paragraph("<b>ANSI SQL to Sovereign EnlngDB Rosetta Stone</b>", styles['DbPartTitle']))
    story.append(HRFlowable(width="100%", thickness=1.5, color=colors.HexColor("#0284c7"), spaceBefore=4, spaceAfter=14))
    story.append(Paragraph(
        "A side-by-side Rosetta Stone translating traditional ANSI SQL queries into canonical EnlngDB Zero-SQL syntax. "
        "Eliminate commas, brackets, and cryptic punctuation across all enterprise data access patterns:",
        styles['DbBodyLead']
    ))
    rosetta_data = [
        [Paragraph("<b>Archaic ANSI SQL</b>", styles['DbH3']),
         Paragraph("<b>Canonical EnlngDB Zero-SQL</b>", styles['DbH3']),
         Paragraph("<b>Semantic Classification</b>", styles['DbH3'])],
        [Paragraph("<code>SELECT * FROM users;</code>", styles['DbBody']),
         Paragraph("<code>find all records from users</code>", styles['DbBody']),
         Paragraph("Full Table Scan", styles['DbBody'])],
        [Paragraph("<code>SELECT id, name FROM users;</code>", styles['DbBody']),
         Paragraph("<code>find id, name from users</code>", styles['DbBody']),
         Paragraph("Projection Slice", styles['DbBody'])],
        [Paragraph("<code>SELECT * FROM u WHERE age &gt; 21;</code>", styles['DbBody']),
         Paragraph("<code>find u where age is greater than 21</code>", styles['DbBody']),
         Paragraph("Relational Filtering", styles['DbBody'])],
        [Paragraph("<code>INSERT INTO u (a, b) VALUES (1, 2);</code>", styles['DbBody']),
         Paragraph("<code>insert into u with a 1, b 2</code>", styles['DbBody']),
         Paragraph("Row Ingestion", styles['DbBody'])],
        [Paragraph("<code>UPDATE u SET bal=500 WHERE id=1;</code>", styles['DbBody']),
         Paragraph("<code>in u change bal to 500 where id is 1</code>", styles['DbBody']),
         Paragraph("In-Place Mutation", styles['DbBody'])],
        [Paragraph("<code>DELETE FROM u WHERE id=5;</code>", styles['DbBody']),
         Paragraph("<code>delete records from u where id is 5</code>", styles['DbBody']),
         Paragraph("Selective Deletion", styles['DbBody'])],
        [Paragraph("<code>DROP TABLE users;</code>", styles['DbBody']),
         Paragraph("<code>drop table users confirmed</code>", styles['DbBody']),
         Paragraph("Destructive Purge", styles['DbBody'])],
        [Paragraph("<code>SELECT COUNT(*) FROM users;</code>", styles['DbBody']),
         Paragraph("<code>count records from users</code>", styles['DbBody']),
         Paragraph("Fast Aggregation", styles['DbBody'])],
        [Paragraph("<code>SELECT * FROM u ORDER BY a DESC;</code>", styles['DbBody']),
         Paragraph("<code>find u order by a descending</code>", styles['DbBody']),
         Paragraph("Result Ordering", styles['DbBody'])],
        [Paragraph("<code>SELECT * FROM u LIMIT 10 OFFSET 5;</code>", styles['DbBody']),
         Paragraph("<code>find u limit 10 offset 5</code>", styles['DbBody']),
         Paragraph("Result Pagination", styles['DbBody'])],
        [Paragraph("<code>SELECT DISTINCT role FROM u;</code>", styles['DbBody']),
         Paragraph("<code>find distinct role from u</code>", styles['DbBody']),
         Paragraph("Set Deduplication", styles['DbBody'])],
        [Paragraph("<code>BEGIN TRANSACTION; ... COMMIT;</code>", styles['DbBody']),
         Paragraph("<code>begin transaction ... commit transaction</code>", styles['DbBody']),
         Paragraph("ACID Boundary", styles['DbBody'])],
        [Paragraph("<code>TRUNCATE TABLE logs;</code>", styles['DbBody']),
         Paragraph("<code>delete all from logs confirmed</code>", styles['DbBody']),
         Paragraph("Storage Zeroing", styles['DbBody'])],
        [Paragraph("<code>ALTER TABLE u ADD COLUMN bio TEXT;</code>", styles['DbBody']),
         Paragraph("<code>alter table u add column bio text</code>", styles['DbBody']),
         Paragraph("Schema Evolution", styles['DbBody'])],
    ]
    t_rosetta = Table(rosetta_data, colWidths=[160, 200, 140])
    t_rosetta.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#f1f5f9")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#cbd5e1")),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
        ('LEFTPADDING', (0,0), (-1,-1), 6),
        ('RIGHTPADDING', (0,0), (-1,-1), 6),
    ]))
    story.append(t_rosetta)
    story.append(PageBreak())

    # =========================================================================
    # APPENDIX E: HIGH-THROUGHPUT HARDWARE SIZING & LINUX TUNING
    # =========================================================================
    story.append(Paragraph("<b>APPENDIX E</b>", styles['DbPartRoman']))
    story.append(Paragraph("<b>High-Throughput Hardware Sizing &amp; Linux Kernel Tuning</b>", styles['DbPartTitle']))
    story.append(HRFlowable(width="100%", thickness=1.5, color=colors.HexColor("#0284c7"), spaceBefore=4, spaceAfter=14))
    story.append(Paragraph(
        "To achieve maximum hardware throughput (> 1,000,000 writes/second), enterprise deployments of EnlngDB "
        "should optimize host operating system parameters. Below are recommended Linux kernel sysctl configurations "
        "and NVMe storage optimizations for production servers:",
        styles['DbBodyLead']
    ))
    sysctl_code = """
# /etc/sysctl.d/99-enlngdb-sovereign.conf
# EnlngDB High-Throughput Linux Kernel Optimization Parameters

# Increase maximum open file descriptors for high-concurrency connections
fs.file-max = 2097152

# Optimize virtual memory dirty page flushing for NVMe write-ahead logging
vm.dirty_background_ratio = 5
vm.dirty_ratio = 10
vm.dirty_expire_centisecs = 500
vm.dirty_writeback_centisecs = 100

# Disable swap aggressiveness to protect in-memory database pages
vm.swappiness = 1

# Enable transparent hugepages for large row array allocations
vm.nr_hugepages = 1024

# Network socket tuning for edge proxy gateways
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.ipv4.tcp_fin_timeout = 15
net.ipv4.tcp_tw_reuse = 1
"""
    story.extend(make_code_box(sysctl_code))
    story.append(Spacer(1, 10))
    story.append(Paragraph(
        "For NVMe storage controllers, configure the disk scheduler to <code>none</code> (bypass host OS I/O queues) "
        "and mount the data partition with <code>noatime,nodiratime,barrier=0</code> for maximum raw write velocity.",
        styles['DbBody']
    ))
    story.append(PageBreak())

    # =========================================================================
    # APPENDIX F: DISTRIBUTED RAFT PROTOCOL SPECIFICATION
    # =========================================================================
    story.append(Paragraph("<b>APPENDIX F</b>", styles['DbPartRoman']))
    story.append(Paragraph("<b>Distributed Raft Consensus &amp; Multi-Node Replication</b>", styles['DbPartTitle']))
    story.append(HRFlowable(width="100%", thickness=1.5, color=colors.HexColor("#0284c7"), spaceBefore=4, spaceAfter=14))
    story.append(Paragraph(
        "The EnlngDB high-availability clustering engine coordinates state transitions across distributed nodes "
        "using the Raft consensus algorithm. WAL frames are serialized as log entries with sequential terms and indexes:",
        styles['DbBodyLead']
    ))
    raft_code = """
/* EnlngDB Distributed Raft RPC Frame Formats */

typedef struct {
    uint64_t term;              /* Candidate's election term */
    char candidate_id[32];      /* Candidate node identifier */
    uint64_t last_log_index;    /* Index of candidate's last WAL frame */
    uint64_t last_log_term;     /* Term of candidate's last WAL frame */
} RaftRequestVoteArgs;

typedef struct {
    uint64_t term;              /* CurrentTerm, for candidate to update itself */
    bool vote_granted;          /* True means candidate received vote */
} RaftRequestVoteReply;

typedef struct {
    uint64_t term;              /* Leader's current term */
    char leader_id[32];         /* So followers can redirect clients */
    uint64_t prev_log_index;    /* Index of WAL frame preceding new ones */
    uint64_t prev_log_term;     /* Term of prev_log_index frame */
    uint8_t wal_payload[4096];  /* Raw WAL frame bytes */
    size_t payload_len;         /* Frame size in bytes */
    uint64_t leader_commit;     /* Leader's commit index */
} RaftAppendEntriesArgs;

typedef struct {
    uint64_t term;              /* CurrentTerm, for leader to update itself */
    bool success;               /* True if follower matched prev_log_index */
    uint64_t match_index;       /* Follower's highest replicated index */
} RaftAppendEntriesReply;
"""
    story.extend(make_code_box(raft_code))
    story.append(PageBreak())

    # =========================================================================
    # APPENDIX G: ZERO-SQL SECURITY & THREAT MODELING
    # =========================================================================
    story.append(Paragraph("<b>APPENDIX G</b>", styles['DbPartRoman']))
    story.append(Paragraph("<b>Security Architecture, Threat Modeling &amp; Access Controls</b>", styles['DbPartTitle']))
    story.append(HRFlowable(width="100%", thickness=1.5, color=colors.HexColor("#0284c7"), spaceBefore=4, spaceAfter=14))
    story.append(Paragraph(
        "A formal evaluation of the EnlngDB security architecture, threat mitigations, and defense against "
        "injection attacks. By eliminating SQL statement concatenation and enforcing grammar-level token typing, "
        "EnlngDB renders SQL Injection (SQLi) conceptually impossible:",
        styles['DbBodyLead']
    ))
    threat_data = [
        [Paragraph("<b>Threat Vector</b>", styles['DbH3']),
         Paragraph("<b>Legacy RDBMS Vulnerability</b>", styles['DbH3']),
         Paragraph("<b>EnlngDB Sovereign Defense Mechanism</b>", styles['DbH3'])],
        [Paragraph("SQL Injection (SQLi)", styles['DbBody']),
         Paragraph("String concatenation allows attackers to inject OR '1'='1' or DROP TABLE.", styles['DbBody']),
         Paragraph("<b>Impossible:</b> Strict grammar rejects dangling boolean phrases; DROP requires 'confirmed'.", styles['DbBody'])],
        [Paragraph("Accidental Table Drop", styles['DbBody']),
         Paragraph("A junior developer or compromised script runs DROP TABLE users in production.", styles['DbBody']),
         Paragraph("<b>Blocked:</b> Mandatory confirmed safety guard halts statement without confirmation flag.", styles['DbBody'])],
        [Paragraph("Buffer Overflow Exploitation", styles['DbBody']),
         Paragraph("Overlong query string overwrites stack frame in vulnerable C drivers.", styles['DbBody']),
         Paragraph("<b>Mitigated:</b> Fixed 64-byte bounded token buffers with zero-copy lexer scanning.", styles['DbBody'])],
        [Paragraph("Memory Leak Starvation", styles['DbBody']),
         Paragraph("Long-running daemon accumulates leaked heap allocations until OOM killer strikes.", styles['DbBody']),
         Paragraph("<b>Zero-Leak:</b> Rigorous ASan/LSan audit suite verifies 0 bytes leaked across millions of queries.", styles['DbBody'])],
        [Paragraph("Privilege Escalation", styles['DbBody']),
         Paragraph("Default root user or weak credential storage exposes entire cluster.", styles['DbBody']),
         Paragraph("<b>Air-Gapped:</b> File-based sovereignty; filesystem ACLs govern .edb access directly.", styles['DbBody'])],
    ]
    t_threat = Table(threat_data, colWidths=[130, 180, 190])
    t_threat.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#f1f5f9")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#cbd5e1")),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
        ('LEFTPADDING', (0,0), (-1,-1), 6),
        ('RIGHTPADDING', (0,0), (-1,-1), 6),
    ]))
    story.append(t_threat)
    story.append(Spacer(1, 14))

    return story
