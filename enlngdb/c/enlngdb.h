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

#define ENLNGDB_MAX_COLS 32
#define ENLNGDB_MAX_TABLES 64
#define ENLNGDB_MAX_NAME 64
#define ENLNGDB_MAGIC "ENLNG_C_EDB_V2"
#define ENLNGDB_MAGIC_V1 "ENLNG_C_EDB_V1"
#define ENLNGDB_MAX_INDEXES 8

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

typedef struct EnlngDatabase {
    char name[ENLNGDB_MAX_NAME];
    EnlngTable tables[ENLNGDB_MAX_TABLES];
    int table_count;
    char db_filepath[256];
    struct EnlngDatabase* txn;  /* the copy to go back to on 'rollback', while a transaction is open */
    bool dropped;               /* 'drop database' on the open one: nothing is saved at the end */
} EnlngDatabase;

typedef struct {
    EnlngColumn columns[ENLNGDB_MAX_COLS];
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
bool enlngdb_begin(EnlngDatabase* db);
bool enlngdb_commit(EnlngDatabase* db);
bool enlngdb_rollback(EnlngDatabase* db);
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
