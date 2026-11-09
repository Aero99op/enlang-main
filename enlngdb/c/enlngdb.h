#ifndef ENLNGDB_H
#define ENLNGDB_H

#ifndef __USE_MINGW_ANSI_STDIO
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../src/enlangg_version.h"

#define ENLNGDB_MAX_COLS 64
#define ENLNGDB_MAX_TABLES 256
#define ENLNGDB_MAX_NAME 64
#define ENLNGDB_MAGIC "ENLNG_C_EDB_V2"
#define ENLNGDB_MAGIC_V1 "ENLNG_C_EDB_V1"
#define ENLNGDB_MAX_INDEXES 8
#define ENLNGDB_MAX_RESULT_COLS 128     /* the columns of a query result: two whole tables joined */

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
    OP_NONE = 0,
    OP_EQ,
    OP_NEQ,
    OP_GT,
    OP_LT,
    OP_GTE,
    OP_LTE,
    OP_LIKE
} EnlngOp;

/* What a column holds. EK_ANY keeps whatever is written (older tables, and columns made without a type). */
typedef enum {
    EK_ANY = 0,
    EK_INTEGER,
    EK_REAL,
    EK_TEXT,
    EK_BOOLEAN,
    EK_DATE
} EnlngKind;

typedef struct {
    char name[ENLNGDB_MAX_NAME];
    EnlngValType type;          /* kept for older code; the column's kind is below */
    bool is_primary_key;
    bool is_unique;
    uint8_t kind;               /* EnlngKind */
    bool not_null;
    bool is_auto;               /* counts up by itself when no value is given */
    char default_text[64];      /* "" for none; read like a value written in a statement */
    char check_text[96];        /* "" for none: a condition every row must meet */
    char ref_table[ENLNGDB_MAX_NAME];   /* "" for none: values must exist in ref_table.ref_col */
    char ref_col[ENLNGDB_MAX_NAME];
    uint8_t on_update;          /* when the referred key is changed: 0 refuse, 1 follow it (cascade), 2 set null */
    uint8_t on_delete;          /* when the referred row is deleted: 0 refuse, 1 delete this row too (cascade), 2 set null */
} EnlngColumn;

typedef struct {
    char name[ENLNGDB_MAX_NAME];
    char cols[4][ENLNGDB_MAX_NAME];
    int col_count;
    bool is_unique;
} EnlngIndex;

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
    bool is_temp;               /* a temporary table is not saved to the file */
    char view_sql[1024];        /* not empty: this is a view, and this is its query */
    EnlngIndex indexes[ENLNGDB_MAX_INDEXES];
    int index_count;
    int64_t auto_next[ENLNGDB_MAX_COLS];    /* the next number of an auto column */
} EnlngTable;

/* a trigger: statements run after each row that an insert, update or delete changed in a table */
typedef struct {
    char name[ENLNGDB_MAX_NAME];
    char table[ENLNGDB_MAX_NAME];
    int event;                  /* 1 insert, 2 update, 3 delete */
    char* when;                 /* NULL for none: a condition that can use new.x and old.x */
    char* body;                 /* the statements, separated by ; (new.x and old.x stand for the row's values) */
} EnlngTrigger;

/* a procedure: statements saved under a name, run with call name(values); :parameter stands for a value */
typedef struct {
    char name[ENLNGDB_MAX_NAME];
    char* params;               /* "a, b" */
    char* body;
} EnlngProc;

/* a user who can sign in to the server: only a salted hash of the password is kept (PBKDF2 with SHA-256) */
typedef struct {
    char name[ENLNGDB_MAX_NAME];
    unsigned char salt[16];
    unsigned char hash[32];
    int rounds;
    bool admin;                 /* may do everything, also make users and tables */
} EnlngUser;

/* what a user may do with a table (object "*": every table) or a procedure */
#define ENLNGDB_RIGHT_SELECT  1u
#define ENLNGDB_RIGHT_INSERT  2u
#define ENLNGDB_RIGHT_UPDATE  4u
#define ENLNGDB_RIGHT_DELETE  8u
#define ENLNGDB_RIGHT_EXECUTE 16u
typedef struct {
    char user[ENLNGDB_MAX_NAME];
    char object[ENLNGDB_MAX_NAME];
    int is_proc;
    unsigned int rights;
} EnlngGrant;

typedef struct EnlngDatabase {
    char name[ENLNGDB_MAX_NAME];
    EnlngTable tables[ENLNGDB_MAX_TABLES];
    int table_count;
    char db_filepath[256];
    struct EnlngDatabase* txn;  /* the copy to go back to on 'rollback', while a transaction is open */
    bool dropped;               /* 'drop database' on the open one: nothing is saved at the end */
    struct EnlngDatabase* saves[16];    /* savepoints inside the open transaction, oldest first */
    char save_names[16][64];
    int nsaves;
    EnlngTable* undo;           /* the tables before the last update, delete, truncate, alter or drop (undo_count of them) */
    int undo_count;
    char undo_what[160];        /* that statement, as written */
    char lock_path[300];        /* "" for none: the lock file this program holds for the open database */
    intptr_t lock_handle;       /* the open lock file (a HANDLE on Windows, a file descriptor elsewhere) */
    EnlngTrigger* triggers;
    int trigger_count;
    EnlngProc* procs;
    int proc_count;
    void* journal;              /* what the last undoable statement changed, kept by the parser for 'undo' (NULL: nothing) */
    EnlngUser* users;
    int user_count;
    EnlngGrant* grants;
    int grant_count;
    void* txn_journal;          /* while a transaction is open: the records of what it changed, kept by the parser */
} EnlngDatabase;

typedef struct {
    EnlngColumn columns[ENLNGDB_MAX_RESULT_COLS];
    int col_count;
    EnlngRow* rows;
    size_t row_count;
    double execution_time_ms;
} EnlngQueryResult;

/* --- Core Engine Prototypes --- */
EnlngDatabase* enlngdb_create(const char* name);
void enlngdb_free(EnlngDatabase* db);

EnlngTable* enlngdb_create_table(EnlngDatabase* db, const char* name, const EnlngColumn* cols, int col_count);
EnlngTable* enlngdb_get_table(EnlngDatabase* db, const char* name);
bool enlngdb_drop_table(EnlngDatabase* db, const char* name, bool confirmed);
bool enlngdb_delete_column(EnlngTable* table, const char* col_name);
void enlngdb_free_rows(EnlngTable* table);
bool enlngdb_append_row(EnlngTable* table, const EnlngVal* cells, int cell_count);
bool enlngdb_rename_table(EnlngDatabase* db, const char* from, const char* to);
bool enlngdb_rename_column(EnlngTable* table, const char* from, const char* to);
void enlngdb_clear(EnlngDatabase* db);
/* One program at a time per database: the lock is held from opening a database until leaving it.
 * Returns false, with the reason in err, when another program still holds it after a few seconds. */
bool enlngdb_lock(EnlngDatabase* db, const char* name, char* err, size_t err_size);
void enlngdb_unlock(EnlngDatabase* db);
/* The parser keeps a transaction as records of what changed (no copy of the tables); without it, 'begin' copies them. */
typedef struct {
    bool (*begin)(EnlngDatabase* db);
    bool (*commit)(EnlngDatabase* db);
    bool (*rollback)(EnlngDatabase* db);
    bool (*savepoint)(EnlngDatabase* db, const char* name);
    bool (*rollback_to)(EnlngDatabase* db, const char* name);
    bool (*release)(EnlngDatabase* db, const char* name);
    void (*drop)(void* txn_journal);
} EnlngTxnHooks;
extern const EnlngTxnHooks* enlngdb_txn_hooks;
bool enlngdb_begin(EnlngDatabase* db);
bool enlngdb_commit(EnlngDatabase* db);
bool enlngdb_rollback(EnlngDatabase* db);
/* inside a transaction: remember the database as it is now under a name, go back to it, or forget it */
bool enlngdb_savepoint(EnlngDatabase* db, const char* name);
bool enlngdb_rollback_to(EnlngDatabase* db, const char* name);
bool enlngdb_release(EnlngDatabase* db, const char* name);
/* a whole copy of the tables (rows included), and putting such a copy back (the copy is used up) */
EnlngDatabase* enlngdb_copy(const EnlngDatabase* db);
void enlngdb_restore(EnlngDatabase* db, EnlngDatabase* copy);
/* the same for just the tables that exist (much smaller than a whole database): for undo */
EnlngTable* enlngdb_copy_tables(const EnlngDatabase* db, int* count);
void enlngdb_restore_tables(EnlngDatabase* db, EnlngTable* tables, int count);
void enlngdb_free_tables(EnlngTable* tables, int count);
/* triggers and procedures: the lists are kept in the file after the tables */
void enlngdb_free_extras(EnlngDatabase* db);
void enlngdb_forget_triggers_of(EnlngDatabase* db, const char* table);
void enlngdb_rename_triggers_of(EnlngDatabase* db, const char* from, const char* to);
/* random bytes from the operating system (for the salt of a password); false if there are none */
bool enlngdb_random_bytes(unsigned char* out, size_t n);

/* --- users and the server ---
 * A database with users can be served to other programs (enlngdb --serve). There a user signs in, and each
 * statement is checked against what the user was granted. A program that opens the file itself is its owner
 * and is not checked: whoever can read the file can read the data. */
/* what the engine prints goes through this, so that a server can keep it for the client */
int enlngdb_printf(const char* fmt, ...);
/* SHA-256, HMAC-SHA-256 and PBKDF2-HMAC-SHA-256 (32 bytes), for passwords and for the server's encryption */
void enlngdb_sha256(const void* data, size_t n, unsigned char out[32]);
void enlngdb_hmac_sha256(const unsigned char* key, size_t kn, const unsigned char* msg, size_t mn, unsigned char out[32]);
void enlngdb_pbkdf2_sha256(const char* password, const unsigned char* salt, size_t sn, int rounds, unsigned char out[32]);
bool enlngdb_has_users(const EnlngDatabase* db);
/* true when the name and password are right; *admin tells whether the user may do everything */
bool enlngdb_login(const EnlngDatabase* db, const char* user, const char* password, bool* admin);
/* runs a script as a signed-in user (NULL: the owner), keeping what it prints; *failed counts the statements that
   failed. session tells the clients of a server apart (undo takes back only the same client's change). */
char* enlngdb_run_as(EnlngDatabase* db, const char* user, int session, const char* script, int* failed);

/* counts every change made to any table (rows, columns, tables, restores): a lookup index made earlier is
   good only while this number is the same as when it was made */
extern unsigned long long enlngdb_mutations;
/* Told just before a table of a database changes (or just after one is made), so that the parser can keep what
   'undo' needs to take the change back. NULL when nobody listens. The listener may take the rows of the table
   for itself on ENLNGDB_CH_FREE_ROWS and ENLNGDB_CH_DROP (it then leaves the table with no rows). */
enum {
    ENLNGDB_CH_APPEND = 1,      /* a row is about to be added at the end */
    ENLNGDB_CH_FREE_ROWS,       /* every row is about to go */
    ENLNGDB_CH_WHOLE,           /* the columns and the rows are about to change */
    ENLNGDB_CH_META,            /* the name, the columns or the indexes (not the rows) are about to change */
    ENLNGDB_CH_RENAME,          /* the table is about to get a new name (other tables that refer to it change too) */
    ENLNGDB_CH_DROP,            /* the table is about to go */
    ENLNGDB_CH_CREATED,         /* the table was just made */
    ENLNGDB_CH_RESET            /* every table of the database (t is its first table) is about to be replaced */
};
extern void (*enlngdb_change_hook)(EnlngTable* t, int what);
extern void (*enlngdb_journal_free)(void* journal);
bool enlngdb_add_column(EnlngTable* table, const char* col_name, const EnlngVal* default_val);
extern char enlngdb_last_insert_replaced[96];

bool enlngdb_insert_row(EnlngTable* table, const EnlngVal* cells, int cell_count);
EnlngQueryResult* enlngdb_find(EnlngTable* table, const char* filter_col, EnlngOp op, const EnlngVal* target_val, int limit, int offset);
int enlngdb_count(EnlngTable* table, const char* filter_col, EnlngOp op, const EnlngVal* target_val);
int enlngdb_update(EnlngTable* table, const char* filter_col, EnlngOp op, const EnlngVal* target_val, const char* set_col, const EnlngVal* set_val);
int enlngdb_delete(EnlngTable* table, const char* filter_col, EnlngOp op, const EnlngVal* target_val, bool confirmed);

void enlngdb_free_result(EnlngQueryResult* res);
void enlngdb_print_result(const EnlngQueryResult* res);
void enlngdb_print_tables(const EnlngDatabase* db);

/* --- Binary Disk Persistence --- */
bool enlngdb_save(const EnlngDatabase* db, const char* filepath);
bool enlngdb_load(EnlngDatabase* db, const char* filepath);

/* --- Natural Conversational Parser & Query Executor --- */
bool enlngdb_execute_statement(EnlngDatabase* db, const char* statement, bool print_output);
int enlngdb_execute_script(EnlngDatabase* db, const char* script_content, bool print_output);
int enlngdb_run_file(const char* filepath);

/* --- Helper constructors --- */
EnlngVal enlng_int(int64_t v);
EnlngVal enlng_double(double v);
EnlngVal enlng_str(const char* s);
EnlngVal enlng_bool(bool b);
EnlngVal enlng_null(void);
void enlng_free_val(EnlngVal* v);
EnlngVal enlng_copy_val(const EnlngVal* src);
int enlng_compare_vals(const EnlngVal* a, const EnlngVal* b);
bool enlng_eval_condition(const EnlngVal* val, EnlngOp op, const EnlngVal* target);

#endif /* ENLNGDB_H */

/* Memory. Up to a budget (ENLNGDB_MEMORY, half of the computer's memory when it is not set) the engine takes
 * memory as usual; past it, new memory comes from a file (in ENLNGDB_SPILL_DIR, or the folder for temporary
 * files) that the system keeps in memory only where it is used, so a database can be larger than the memory.
 * A file of the engine includes this header a second time, after every system header, with ENLNGDB_WRAP_ALLOC. */
#ifndef ENLNGDB_ALLOC_DECLARED
#define ENLNGDB_ALLOC_DECLARED
void* enlngdb_malloc(size_t n);
void* enlngdb_calloc(size_t n, size_t size);
void* enlngdb_realloc(void* p, size_t n);
void enlngdb_mfree(void* p);
char* enlngdb_strdup(const char* s);
/* bytes taken as usual memory now and at most, and bytes of the file in use now and at most */
void enlngdb_memory_stats(size_t* heap, size_t* heap_peak, size_t* spill, size_t* spill_peak);
#endif
#if defined(ENLNGDB_WRAP_ALLOC) && !defined(ENLNGDB_ALLOC_WRAPPED)
#define ENLNGDB_ALLOC_WRAPPED
#define malloc(n) enlngdb_malloc(n)
#define calloc(n, size) enlngdb_calloc(n, size)
#define realloc(p, n) enlngdb_realloc(p, n)
#define free(p) enlngdb_mfree(p)
#define strdup(s) enlngdb_strdup(s)
#endif
