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

#define ENLNGDB_MAX_COLS 256
#define ENLNGDB_MAX_TABLES 4096         /* the list of tables grows as tables are made */
#define ENLNGDB_MAX_NAME 64
#define ENLNGDB_MAGIC "ENLNG_C_EDB_V2"
#define ENLNGDB_MAGIC_V3 "ENLNG_C_EDB_V3"     /* rows with their places, and lookup indexes, so a file opens without reading its rows */
#define ENLNGDB_MAGIC_V4 "ENLNG_C_EDB_V4"     /* version 3 with a random id, which ties a log of changes (.edb-wal) to the file */
#define ENLNGDB_STORED_MAX 8                  /* lookup indexes kept in the file for a table */
#define ENLNGDB_MAGIC_V1 "ENLNG_C_EDB_V1"
#define ENLNGDB_MAX_INDEXES 32
#define ENLNGDB_MAX_RESULT_COLS 512     /* the columns of a query result: two whole tables joined */

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
    EK_DATE,
    EK_TIMESTAMP,               /* a moment, kept as "YYYY-MM-DD HH:MM:SSZ" (UTC) */
    EK_JSON,                    /* text that is JSON */
    EK_BLOB                     /* bytes, kept as 0x and two hex digits a byte */
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
    int origin;                 /* the row's place in the full file the database was opened from; -1 for a row added
                                   since. A save writes only what differs from that file (see the log, .edb-wal) */
} EnlngRow;

typedef struct {
    char name[ENLNGDB_MAX_NAME];
    EnlngColumn columns[ENLNGDB_MAX_COLS];
    int col_count;
    EnlngRow* rowv;             /* the rows: read them with enlngdb_row(t, i), since a row of a file opened lazily
                                   is read from the file the first time it is needed */
    size_t row_count;
    size_t row_capacity;
    int version;
    bool is_temp;               /* a temporary table is not saved to the file */
    char view_sql[1024];        /* not empty: this is a view, and this is its query */
    EnlngIndex indexes[ENLNGDB_MAX_INDEXES];
    int index_count;
    int64_t auto_next[ENLNGDB_MAX_COLS];    /* the next number of an auto column */
    /* A table of a file opened lazily: its rows are in the file (mapped into memory) from 'lazy' on. A row not read
       yet has cell_count -1, and its 'cells' holds its place from 'lazy'; enlngdb_row reads it the first time. */
    const unsigned char* lazy;
    size_t lazy_bytes;          /* how many bytes of rows are there: a row read from the file must end inside them */
    size_t base_bytes;          /* ... and of the rows at base_data */
    int nstored;                /* lookup indexes kept in the file: for a column, (hash, row) sorted by hash */
    struct {
        int col; uint32_t n, nspill; const unsigned char* at; const unsigned char* spill;
        /* the same column in order, for ranges and 'like "abc%"': its numbers (value 8 bytes, row 4) ascending, the rows
           with a text in the order of the texts, and the rows with another kind of value; nulls are in none */
        bool has_range; uint32_t nn, ns, no; const unsigned char* nums; const unsigned char* texts; const unsigned char* others;
    } stored[ENLNGDB_STORED_MAX];
    /* The table as it is in the full file last opened or saved ('base'): a save writes only what differs from it, to
       the log (.edb-wal), while the columns and the other tables are the same. base_rows is -1 for a table not there. */
    long long base_rows;
    int base_pos;                       /* its place among the saved tables of that file */
    const unsigned char* base_data;     /* its rows in that file, and the place of each */
    const unsigned char* base_offs;
    uint8_t base_width;
    uint8_t base_sig[32];               /* SHA-256 of its columns and indexes as they were */
    /* what the log changed when the file was opened: the indexes kept in the file still find the other rows */
    bool wal_applied;
    const unsigned char* wal_del;       /* rows of the file deleted (sorted places, 4 bytes each) */
    uint32_t nwal_del;
    const uint32_t* wal_set;            /* rows of the file changed (sorted) */
    uint32_t nwal_set;
    size_t wal_app_from;                /* the rows from here on were added */
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
    EnlngTable* tables;         /* table_cap of them made, table_count used; the list moves when it grows */
    int table_count;
    int table_cap;
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
    bool encrypted;             /* the file is written sealed with file_key (ChaCha20-Poly1305) */
    bool locked;                /* the file is encrypted and was not opened (no password, or a wrong one): nothing
                                   can be run, and the file is never written */
    unsigned char file_salt[16];
    unsigned char file_key[32]; /* PBKDF2-SHA-256 of the password with file_salt, file_rounds times */
    uint32_t file_rounds;
    void* sources;              /* the files (mapped) and buffers that rows not read yet come from; freed with the database */
    bool has_base;              /* opened or saved as a full file of version 4: a save can write only the changes */
    uint8_t file_id[16];
    uint8_t extras_sig[32];     /* SHA-256 of the triggers, procedures, users and rights in that file */
    int base_ntables;
    size_t base_len;
    char base_path[300];
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
/* makes the list of tables hold at least n (it may move); false when there is no memory or n is past the most */
bool enlngdb_tables_room(EnlngDatabase* db, int n);
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
/* the files of a folder that end in ext (".edb"): their names without it, and their sizes; returns how many
   (at most max), sorted by name */
int enlngdb_list_files(const char* dir, const char* ext, char (*names)[ENLNGDB_MAX_NAME], long long* sizes, int max);

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
/* AEAD ChaCha20-Poly1305 (RFC 8439, no extra data): seal writes the 16-byte tag after the n bytes (room needed);
   open checks the tag after the n bytes and opens them in place, false (nothing opened) if anything was changed */
void enlngdb_aead_seal(const uint8_t key[32], const uint8_t nonce[12], uint8_t* data, size_t n);
bool enlngdb_aead_open(const uint8_t key[32], const uint8_t nonce[12], uint8_t* data, size_t n);
/* true when the name and password are right; *admin tells whether the user may do everything */
bool enlngdb_login(const EnlngDatabase* db, const char* user, const char* password, bool* admin);
/* runs a script as a signed-in user (NULL: the owner), keeping what it prints; *failed counts the statements that
   failed. session tells the clients of a server apart (undo takes back only the same client's change). */
char* enlngdb_run_as(EnlngDatabase* db, const char* user, int session, const char* script, int* failed);
/* true when every statement of a script only reads (a server may run it in a reader process); when in doubt, false */
bool enlngdb_script_reads_only(const char* script);
/* true while 'set time zone' has not changed the zone (UTC) */
bool enlngdb_zone_default(void);

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

/* A name in backticks may have spaces: `order details`. Inside the engine each space of such a name is this byte,
   so the name stays one word; what the engine prints shows it as a space again. */
#define ENLNGDB_NAME_SPACE '\x1f'
/* puts a file written beside path (tmp) in its place in one step; false if it could not */
bool enlngdb_replace_file(const char* tmp, const char* path);

/* --- Rows read on first use --- */
void enlngdb_load_row(const EnlngTable* t, size_t i);
static inline EnlngRow* enlngdb_row(const EnlngTable* t, size_t i) {
    EnlngRow* r = &((EnlngTable*)t)->rowv[i];
    if (r->cell_count < 0) enlngdb_load_row(t, i);
    return r;
}
/* reads every row not read yet (a server keeps its databases whole), and lets go of the files they came from */
void enlngdb_materialize(EnlngDatabase* db);
/* the hash the lookup indexes use (set by the parser): false for a value that is not looked up by hash */
extern bool (*enlngdb_value_hash)(const EnlngVal* v, uint64_t* h);
/* enlngdb_mutations just after the last file was opened: the indexes kept in a file are good while it is the same */
extern unsigned long long enlngdb_loaded_at;
/* the text in column col of row b of the file the table was opened from (not ended by a zero byte); NULL if it is not a text */
const char* enlngdb_base_text(const EnlngTable* t, size_t b, int col, size_t* len);
/* A script as an editor saved it, made plain UTF-8: the mark at the start of a UTF-8 file (Notepad writes one) is
   taken off, and a UTF-16 file (Notepad's "Unicode") is turned into UTF-8. buf (n bytes, a zero after them) is
   from malloc; what is returned is buf or a new buffer (then buf was freed). */
char* enlngdb_script_utf8(char* buf, size_t n);
/* runs statements given on the command line (-e); what they change is saved to the database they 'use' */
int enlngdb_run_inline(const char* text);
/* A whole copy of the database as it is now (unsaved changes too) written to path, as a full file of its own: it
   opens as a database, and an encrypted database gives an encrypted copy with the same password. Written to path.tmp
   first and put in place in one step. The database and its own file are not touched. */
bool enlngdb_backup_file(EnlngDatabase* db, const char* path);
/* The database becomes what the file at path holds: its tables, triggers, procedures, users and rights. The file
   is read whole and checked first; when it cannot be read nothing changes. password: for an encrypted file (NULL:
   none). Returns 0, or enlngdb_load_error's reason (1 no password, 2 wrong password, 4 not a database), or -1
   when there is no such file. The next save writes the full file. */
int enlngdb_restore_file(EnlngDatabase* db, const char* path, const char* password);
#define ENLNGDB_END_MARK "ENLNG_EDB_END_V4"   /* the last 16 bytes of a version 4 file: a file without them was cut */
#define ENLNGDB_STORED_RANGE 0x80000000u    /* in the count of kept indexes: each is followed by its column in order */

/* --- Binary Disk Persistence --- */
/* The password enlngdb_load uses when the file is encrypted (NULL: none); set it before, and clear it after. When
   the load fails, enlngdb_load_error says why: 1 the file is encrypted and no password was given, 2 the password is
   wrong (or the file was changed), 0 anything else. */
extern const char* enlngdb_open_password;
extern int enlngdb_load_error;          /* also 3: the log of changes (.edb-wal) was damaged; 4: the file is not a database this program can read */
/* turn the encryption of the file on with this password (a new salt each time), or off (password NULL) */
bool enlngdb_set_password(EnlngDatabase* db, const char* password);
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
