#ifndef ENLNGS_H
#define ENLNGS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>

#define ENLNGS_VERSION "5.3.0"
#define MAX_VARS 512
#define MAX_FUNCS 128
#define MAX_PARAMS 16
#define MAX_BODY_LINES 256
#define MAX_LINE_LEN 512
#define MAX_STORES 32

typedef enum {
    VAL_NULL,
    VAL_BOOL,
    VAL_INT,
    VAL_DOUBLE,
    VAL_STRING
} EnlngsType;

typedef struct {
    EnlngsType type;
    union {
        bool b_val;
        long long i_val;
        double d_val;
        char s_val[256];
    };
} EnlngsVal;

typedef struct {
    char name[64];
    EnlngsVal val;
    bool is_const;
} EnlngsVar;

typedef struct {
    char name[64];
    int param_count;
    char params[MAX_PARAMS][64];
    int body_count;
    char body[MAX_BODY_LINES][MAX_LINE_LEN];
} EnlngsFunc;

typedef struct {
    char name[64];
    int state_count;
    char state_keys[64][64];
    EnlngsVal state_vals[64];
} EnlngsStore;

typedef struct {
    EnlngsVar vars[MAX_VARS];
    int var_count;
    EnlngsFunc funcs[MAX_FUNCS];
    int func_count;
    EnlngsStore stores[MAX_STORES];
    int store_count;
    bool in_function;
    EnlngsVal return_val;
    bool has_returned;
} EnlngsVM;

/* Core Public API */
EnlngsVM* enlngs_create_vm(void);
void enlngs_free_vm(EnlngsVM* vm);

EnlngsVal enlngs_eval_expr(EnlngsVM* vm, const char* expr);
bool enlngs_exec_line(EnlngsVM* vm, const char* line);
bool enlngs_exec_script(EnlngsVM* vm, const char* script);
int enlngs_run_file(const char* filepath);
int enlngs_compile_to_js(const char* filepath, const char* outpath);

/* Value Helpers */
EnlngsVal enlngs_val_null(void);
EnlngsVal enlngs_val_bool(bool b);
EnlngsVal enlngs_val_int(long long i);
EnlngsVal enlngs_val_double(double d);
EnlngsVal enlngs_val_string(const char* s);
void enlngs_print_val(EnlngsVal v);
char* enlngs_val_to_str(EnlngsVal v, char* buf, size_t bufsize);

#endif /* ENLNGS_H */
