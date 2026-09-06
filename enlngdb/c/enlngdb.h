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
    OP_NONE = 0,
    OP_EQ,
    OP_NEQ,
    OP_GT,
    OP_LT,
    OP_GTE,
    OP_LTE,
    OP_LIKE
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

#endif /* ENLNGDB_H */
