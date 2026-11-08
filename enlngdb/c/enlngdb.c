#ifndef __USE_MINGW_ANSI_STDIO
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include "enlngdb.h"
#include <ctype.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#ifndef __EMSCRIPTEN__
#include <sys/file.h>
#endif
#endif

static double get_time_ms(void) {
#ifdef _WIN32
    static LARGE_INTEGER freq;
    static int initialized = 0;
    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = 1;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)(now.QuadPart * 1000.0) / (double)freq.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
#endif
}

EnlngVal enlng_int(int64_t v) {
    EnlngVal val;
    val.type = ENLNG_VAL_INT;
    val.int_val = v;
    return val;
}

EnlngVal enlng_double(double v) {
    EnlngVal val;
    val.type = ENLNG_VAL_DOUBLE;
    val.double_val = v;
    return val;
}

EnlngVal enlng_str(const char* s) {
    EnlngVal val;
    val.type = ENLNG_VAL_STRING;
    val.str_val = s ? strdup(s) : strdup("");
    return val;
}

EnlngVal enlng_bool(bool b) {
    EnlngVal val;
    val.type = ENLNG_VAL_BOOL;
    val.bool_val = b;
    return val;
}

EnlngVal enlng_null(void) {
    EnlngVal val;
    val.type = ENLNG_VAL_NULL;
    val.int_val = 0;
    return val;
}

void enlng_free_val(EnlngVal* v) {
    if (v && v->type == ENLNG_VAL_STRING && v->str_val) {
        free(v->str_val);
        v->str_val = NULL;
    }
}

EnlngVal enlng_copy_val(const EnlngVal* src) {
    EnlngVal dest = *src;
    if (src->type == ENLNG_VAL_STRING && src->str_val) {
        dest.str_val = strdup(src->str_val);
    }
    return dest;
}

EnlngDatabase* enlngdb_create(const char* name) {
    EnlngDatabase* db = (EnlngDatabase*)calloc(1, sizeof(EnlngDatabase));
    if (name) {
        strncpy(db->name, name, sizeof(db->name) - 1);
    } else {
        strcpy(db->name, "default");
    }
    db->table_count = 0;
    return db;
}

void enlngdb_free_rows(EnlngTable* tbl) {
    for (size_t r = 0; r < tbl->row_count; r++) {
        for (int c = 0; c < tbl->rows[r].cell_count; c++) {
            enlng_free_val(&tbl->rows[r].cells[c]);
        }
        free(tbl->rows[r].cells);
    }
    free(tbl->rows);
    tbl->rows = NULL;
    tbl->row_count = 0;
    tbl->row_capacity = 0;
}

void enlngdb_clear(EnlngDatabase* db) {
    if (!db) return;
    for (int t = 0; t < db->table_count; t++) enlngdb_free_rows(&db->tables[t]);
    db->table_count = 0;
}

static void free_saves(EnlngDatabase* db, int from) {
    for (int i = from; i < db->nsaves; i++) {
        enlngdb_free(db->saves[i]);
        db->saves[i] = NULL;
    }
    if (from < db->nsaves) db->nsaves = from;
}

void enlngdb_free(EnlngDatabase* db) {
    if (!db) return;
    enlngdb_unlock(db);
    free_saves(db, 0);
    if (db->undo) { enlngdb_free_tables(db->undo, db->undo_count); db->undo = NULL; db->undo_count = 0; }
    if (db->txn) {
        enlngdb_free(db->txn);
        db->txn = NULL;
    }
    enlngdb_clear(db);
    free(db);
}

/* ---- One program at a time ----
 * <name>.edb.lock is locked by the operating system while a program has the database open. The lock goes
 * away by itself when the program ends, also when it crashes, so a lock is never left behind. */
static bool lock_try(const char* path, intptr_t* handle) {
#if defined(__EMSCRIPTEN__)
    (void)path;
    *handle = 0;
    return true;            /* the browser runs one script at a time in one page */
#elif defined(_WIN32)
    HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    OVERLAPPED ov;
    memset(&ov, 0, sizeof(ov));
    if (!LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &ov)) {
        CloseHandle(h);
        return false;
    }
    *handle = (intptr_t)h;
    return true;
#else
    int fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fd < 0) return false;
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        close(fd);
        return false;
    }
    *handle = (intptr_t)fd;
    return true;
#endif
}

static void lock_wait(void) {
#if defined(_WIN32)
    Sleep(100);
#elif !defined(__EMSCRIPTEN__)
    usleep(100000);
#endif
}

bool enlngdb_lock(EnlngDatabase* db, const char* name, char* err, size_t err_size) {
    if (!db || !name) return false;
    char path[300];
    snprintf(path, sizeof(path), "%s.edb.lock", name);
    if (db->lock_path[0] && strcmp(db->lock_path, path) == 0) return true;
    intptr_t handle = 0;
    bool got = false;
    for (int tries = 0; tries < 50 && !(got = lock_try(path, &handle)); tries++) lock_wait();
    if (!got) {
        if (err) snprintf(err, err_size, "database '%s' is in use by another program (it holds %s); waited 5 seconds. Run the scripts one after the other", name, path);
        return false;
    }
    enlngdb_unlock(db);
    snprintf(db->lock_path, sizeof(db->lock_path), "%s", path);
    db->lock_handle = handle;
    return true;
}

void enlngdb_unlock(EnlngDatabase* db) {
    if (!db || !db->lock_path[0]) return;
#if defined(_WIN32)
    HANDLE h = (HANDLE)db->lock_handle;
    OVERLAPPED ov;
    memset(&ov, 0, sizeof(ov));
    UnlockFileEx(h, 0, 1, 0, &ov);
    CloseHandle(h);
    DeleteFileA(db->lock_path);     /* a program waiting on it opens a new one */
#elif !defined(__EMSCRIPTEN__)
    flock((int)db->lock_handle, LOCK_UN);
    close((int)db->lock_handle);    /* the file stays: removing it could let two programs lock two files */
#endif
    db->lock_path[0] = '\0';
    db->lock_handle = 0;
}

/* A whole copy of the database, rows included: what 'begin' keeps so 'rollback' can go back to it */
static EnlngDatabase* copy_database(const EnlngDatabase* db) {
    EnlngDatabase* c = (EnlngDatabase*)calloc(1, sizeof(EnlngDatabase));
    if (!c) return NULL;
    memcpy(c, db, sizeof(EnlngDatabase));
    c->txn = NULL;
    c->lock_path[0] = '\0';
    c->lock_handle = 0;
    c->nsaves = 0;
    memset(c->saves, 0, sizeof(c->saves));
    c->undo = NULL;
    c->undo_count = 0;
    c->undo_what[0] = '\0';
    for (int t = 0; t < db->table_count; t++) {
        const EnlngTable* src = &db->tables[t];
        EnlngTable* dst = &c->tables[t];
        dst->rows = NULL;
        dst->row_count = 0;
        dst->row_capacity = 0;
        for (size_t r = 0; r < src->row_count; r++) {
            enlngdb_append_row(dst, src->rows[r].cells, src->rows[r].cell_count);
        }
    }
    return c;
}

bool enlngdb_begin(EnlngDatabase* db) {
    if (!db || db->txn) return false;
    db->txn = copy_database(db);
    return db->txn != NULL;
}

bool enlngdb_commit(EnlngDatabase* db) {
    if (!db || !db->txn) return false;
    free_saves(db, 0);
    enlngdb_free(db->txn);
    db->txn = NULL;
    return true;
}

/* the tables of saved become the tables of db; saved is used up */
static void take_tables(EnlngDatabase* db, EnlngDatabase* saved) {
    enlngdb_clear(db);
    memcpy(db->tables, saved->tables, sizeof(saved->tables));
    db->table_count = saved->table_count;
    saved->table_count = 0;   /* its rows now belong to db */
    free(saved);
}

bool enlngdb_rollback(EnlngDatabase* db) {
    if (!db || !db->txn) return false;
    EnlngDatabase* saved = db->txn;
    db->txn = NULL;
    free_saves(db, 0);
    take_tables(db, saved);
    return true;
}

EnlngDatabase* enlngdb_copy(const EnlngDatabase* db) {
    return db ? copy_database(db) : NULL;
}

void enlngdb_restore(EnlngDatabase* db, EnlngDatabase* copy) {
    if (!db || !copy) return;
    take_tables(db, copy);
}

EnlngTable* enlngdb_copy_tables(const EnlngDatabase* db, int* count) {
    *count = 0;
    if (!db) return NULL;
    EnlngTable* t = (EnlngTable*)malloc(sizeof(EnlngTable) * (size_t)(db->table_count ? db->table_count : 1));
    if (!t) return NULL;
    for (int i = 0; i < db->table_count; i++) {
        memcpy(&t[i], &db->tables[i], sizeof(EnlngTable));
        t[i].rows = NULL;
        t[i].row_count = 0;
        t[i].row_capacity = 0;
        for (size_t r = 0; r < db->tables[i].row_count; r++) enlngdb_append_row(&t[i], db->tables[i].rows[r].cells, db->tables[i].rows[r].cell_count);
    }
    *count = db->table_count;
    return t;
}

void enlngdb_free_tables(EnlngTable* tables, int count) {
    if (!tables) return;
    for (int i = 0; i < count; i++) enlngdb_free_rows(&tables[i]);
    free(tables);
}

void enlngdb_restore_tables(EnlngDatabase* db, EnlngTable* tables, int count) {
    if (!db || !tables) return;
    enlngdb_clear(db);
    memcpy(db->tables, tables, sizeof(EnlngTable) * (size_t)count);   /* the rows now belong to db */
    db->table_count = count;
    free(tables);
}

static int find_save(const EnlngDatabase* db, const char* name) {
    for (int i = db->nsaves - 1; i >= 0; i--) if (strcmp(db->save_names[i], name) == 0) return i;
    return -1;
}

bool enlngdb_savepoint(EnlngDatabase* db, const char* name) {
    if (!db || !db->txn || !name || db->nsaves >= 16) return false;
    EnlngDatabase* c = copy_database(db);
    if (!c) return false;
    db->saves[db->nsaves] = c;
    snprintf(db->save_names[db->nsaves], sizeof(db->save_names[0]), "%s", name);
    db->nsaves++;
    return true;
}

/* back to the savepoint: what came after it is undone, and the savepoint stays */
bool enlngdb_rollback_to(EnlngDatabase* db, const char* name) {
    int i = db ? find_save(db, name) : -1;
    if (i < 0) return false;
    EnlngDatabase* c = copy_database(db->saves[i]);
    if (!c) return false;
    free_saves(db, i + 1);
    take_tables(db, c);
    return true;
}

bool enlngdb_release(EnlngDatabase* db, const char* name) {
    int i = db ? find_save(db, name) : -1;
    if (i < 0) return false;
    free_saves(db, i);
    return true;
}

EnlngTable* enlngdb_create_table(EnlngDatabase* db, const char* name, const EnlngColumn* cols, int col_count) {
    if (!db || !name) return NULL;
    for (int i = 0; i < db->table_count; i++) {
        if (strcmp(db->tables[i].name, name) == 0) {
            return &db->tables[i];
        }
    }
    if (db->table_count >= ENLNGDB_MAX_TABLES) return NULL;

    EnlngTable* tbl = &db->tables[db->table_count++];
    memset(tbl, 0, sizeof(EnlngTable));
    strncpy(tbl->name, name, sizeof(tbl->name) - 1);
    tbl->col_count = col_count < ENLNGDB_MAX_COLS ? col_count : ENLNGDB_MAX_COLS;
    if (cols) {
        for (int i = 0; i < tbl->col_count; i++) {
            tbl->columns[i] = cols[i];
        }
    }
    tbl->version = 1;
    return tbl;
}

EnlngTable* enlngdb_get_table(EnlngDatabase* db, const char* name) {
    if (!db || !name) return NULL;
    for (int i = 0; i < db->table_count; i++) {
        if (strcmp(db->tables[i].name, name) == 0) {
            return &db->tables[i];
        }
    }
    return NULL;
}

bool enlngdb_drop_table(EnlngDatabase* db, const char* name, bool confirmed) {
    if (!db || !name) return false;
    if (!confirmed) {
        fprintf(stderr, "[SECURITY ALERT] Dropping table '%s' requires 'confirmed' parameter.\n", name);
        return false;
    }
    for (int i = 0; i < db->table_count; i++) {
        if (strcmp(db->tables[i].name, name) == 0) {
            EnlngTable* tbl = &db->tables[i];
            for (size_t r = 0; r < tbl->row_count; r++) {
                for (int c = 0; c < tbl->rows[r].cell_count; c++) {
                    enlng_free_val(&tbl->rows[r].cells[c]);
                }
                free(tbl->rows[r].cells);
            }
            free(tbl->rows);

            for (int j = i; j < db->table_count - 1; j++) {
                db->tables[j] = db->tables[j + 1];
            }
            db->table_count--;
            return true;
        }
    }
    return false;
}

bool enlngdb_delete_column(EnlngTable* table, const char* col_name) {
    if (!table || !col_name) return false;
    int idx = -1;
    for (int c = 0; c < table->col_count; c++) {
        if (strcmp(table->columns[c].name, col_name) == 0) {
            idx = c;
            break;
        }
    }
    if (idx < 0) return false;

    for (size_t r = 0; r < table->row_count; r++) {
        EnlngRow* row = &table->rows[r];
        if (idx >= row->cell_count) continue;
        enlng_free_val(&row->cells[idx]);
        for (int c = idx; c < row->cell_count - 1; c++) {
            row->cells[c] = row->cells[c + 1];
        }
        row->cell_count--;
        row->version++;
    }
    for (int c = idx; c < table->col_count - 1; c++) {
        table->columns[c] = table->columns[c + 1];
        table->auto_next[c] = table->auto_next[c + 1];
    }
    table->col_count--;
    for (int i = 0; i < table->index_count; i++) {
        for (int k = 0; k < table->indexes[i].col_count; k++) {
            if (strcmp(table->indexes[i].cols[k], col_name) == 0) {
                for (int j = i; j < table->index_count - 1; j++) table->indexes[j] = table->indexes[j + 1];
                table->index_count--;
                i--;
                break;
            }
        }
    }
    table->version++;
    return true;
}

bool enlngdb_rename_table(EnlngDatabase* db, const char* from, const char* to) {
    EnlngTable* t = enlngdb_get_table(db, from);
    if (!t || enlngdb_get_table(db, to) || strlen(to) >= ENLNGDB_MAX_NAME) return false;
    strcpy(t->name, to);
    for (int i = 0; i < db->table_count; i++) {
        for (int c = 0; c < db->tables[i].col_count; c++) {
            if (strcmp(db->tables[i].columns[c].ref_table, from) == 0) strcpy(db->tables[i].columns[c].ref_table, to);
        }
    }
    t->version++;
    return true;
}

bool enlngdb_rename_column(EnlngTable* table, const char* from, const char* to) {
    int idx = -1;
    for (int c = 0; c < table->col_count; c++) {
        if (strcmp(table->columns[c].name, to) == 0) return false;
        if (strcmp(table->columns[c].name, from) == 0) idx = c;
    }
    if (idx < 0 || strlen(to) >= ENLNGDB_MAX_NAME) return false;
    strcpy(table->columns[idx].name, to);
    for (int i = 0; i < table->index_count; i++) {
        for (int k = 0; k < table->indexes[i].col_count; k++) {
            if (strcmp(table->indexes[i].cols[k], from) == 0) strcpy(table->indexes[i].cols[k], to);
        }
    }
    table->version++;
    return true;
}

bool enlngdb_add_column(EnlngTable* table, const char* col_name, const EnlngVal* default_val) {
    if (!table || !col_name || table->col_count >= ENLNGDB_MAX_COLS) return false;
    for (int c = 0; c < table->col_count; c++) {
        if (strcmp(table->columns[c].name, col_name) == 0) return false;
    }
    int idx = table->col_count++;
    memset(&table->columns[idx], 0, sizeof(EnlngColumn));
    strncpy(table->columns[idx].name, col_name, ENLNGDB_MAX_NAME - 1);
    table->columns[idx].type = ENLNG_VAL_STRING;

    for (size_t r = 0; r < table->row_count; r++) {
        EnlngRow* row = &table->rows[r];
        row->cells = (EnlngVal*)realloc(row->cells, table->col_count * sizeof(EnlngVal));
        for (int c = row->cell_count; c < table->col_count; c++) row->cells[c] = enlng_null();
        row->cells[idx] = default_val ? enlng_copy_val(default_val) : enlng_null();
        row->cell_count = table->col_count;
        row->version++;
    }
    table->version++;
    return true;
}

static int str_ci_cmp(const char* s1, const char* s2) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;
    while (*s1 && *s2) {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0) return diff;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

static bool sql_like_match(const char* pattern, const char* text) {
    if (!pattern || !text) return false;
    while (*pattern) {
        if (*pattern == '%') {
            pattern++;
            if (*pattern == '\0') return true;
            while (*text) {
                if (sql_like_match(pattern, text)) return true;
                text++;
            }
            return false;
        } else if (*pattern == '_') {
            if (*text == '\0') return false;
            pattern++;
            text++;
        } else {
            if (tolower((unsigned char)*pattern) != tolower((unsigned char)*text)) {
                return false;
            }
            pattern++;
            text++;
        }
    }
    return *text == '\0';
}

int enlng_compare_vals(const EnlngVal* a, const EnlngVal* b) {
    if (a->type == ENLNG_VAL_NULL && b->type == ENLNG_VAL_NULL) return 0;
    if (a->type == ENLNG_VAL_NULL) return -1;
    if (b->type == ENLNG_VAL_NULL) return 1;

    // Numeric comparison
    if ((a->type == ENLNG_VAL_INT || a->type == ENLNG_VAL_DOUBLE) &&
        (b->type == ENLNG_VAL_INT || b->type == ENLNG_VAL_DOUBLE)) {
        double da = (a->type == ENLNG_VAL_DOUBLE) ? a->double_val : (double)a->int_val;
        double db = (b->type == ENLNG_VAL_DOUBLE) ? b->double_val : (double)b->int_val;
        if (fabs(da - db) < 1e-9) return 0;
        return (da < db) ? -1 : 1;
    }

    if (a->type == ENLNG_VAL_STRING && b->type == ENLNG_VAL_STRING) {
        const char* sa = a->str_val ? a->str_val : "";
        const char* sb = b->str_val ? b->str_val : "";
        return str_ci_cmp(sa, sb);
    }

    if (a->type == ENLNG_VAL_BOOL && b->type == ENLNG_VAL_BOOL) {
        return (a->bool_val ? 1 : 0) - (b->bool_val ? 1 : 0);
    }

    // A number and a text: "90" is the number 90; other text comes after every number
    bool a_num = a->type == ENLNG_VAL_INT || a->type == ENLNG_VAL_DOUBLE;
    bool b_num = b->type == ENLNG_VAL_INT || b->type == ENLNG_VAL_DOUBLE;
    if ((a_num && b->type == ENLNG_VAL_STRING) || (b_num && a->type == ENLNG_VAL_STRING)) {
        const EnlngVal* num = a_num ? a : b;
        const char* s = a_num ? b->str_val : a->str_val;
        double dn = num->type == ENLNG_VAL_DOUBLE ? num->double_val : (double)num->int_val;
        char* end = NULL;
        double ds = s ? strtod(s, &end) : 0;
        int cmp;
        if (s && *s && end && *end == '\0') cmp = fabs(dn - ds) < 1e-9 ? 0 : (dn < ds ? -1 : 1);
        else cmp = -1;
        return a_num ? cmp : -cmp;
    }

    // Different kinds otherwise: never equal, ordered by kind
    return (int)a->type - (int)b->type;
}
#define compare_vals enlng_compare_vals

/* After an insert: "" when a new row was added, else which key it replaced ("id 1") */
char enlngdb_last_insert_replaced[96];

/* Adds a row at the end. Which rows count as the same, and what a column allows, is decided by the caller. */
bool enlngdb_append_row(EnlngTable* table, const EnlngVal* cells, int cell_count) {
    if (!table || !cells) return false;
    if (table->row_count >= table->row_capacity) {
        size_t new_cap = table->row_capacity == 0 ? 16 : table->row_capacity * 2;
        EnlngRow* new_rows = (EnlngRow*)realloc(table->rows, new_cap * sizeof(EnlngRow));
        if (!new_rows) return false;
        table->rows = new_rows;
        table->row_capacity = new_cap;
    }
    EnlngRow* target_row = &table->rows[table->row_count];
    target_row->cell_count = cell_count;
    target_row->version = 1;
    target_row->cells = (EnlngVal*)malloc((cell_count ? cell_count : 1) * sizeof(EnlngVal));
    for (int i = 0; i < cell_count; i++) {
        target_row->cells[i] = enlng_copy_val(&cells[i]);
    }
    table->row_count++;
    table->version++;
    return true;
}

bool enlngdb_insert_row(EnlngTable* table, const EnlngVal* cells, int cell_count) {
    enlngdb_last_insert_replaced[0] = '\0';
    return enlngdb_append_row(table, cells, cell_count);
}

#define compare_vals enlng_compare_vals

bool enlng_eval_condition(const EnlngVal* val, EnlngOp op, const EnlngVal* target) {
    if (op == OP_NONE) return true;
    int cmp = enlng_compare_vals(val, target);
    switch (op) {
        case OP_EQ:  return cmp == 0;
        case OP_NEQ: return cmp != 0;
        case OP_GT:  return cmp > 0;
        case OP_LT:  return cmp < 0;
        case OP_GTE: return cmp >= 0;
        case OP_LTE: return cmp <= 0;
        case OP_LIKE: {
            if (val->type == ENLNG_VAL_STRING && target->type == ENLNG_VAL_STRING) {
                const char* pat = target->str_val ? target->str_val : "";
                const char* txt = val->str_val ? val->str_val : "";
                if (strchr(pat, '%') != NULL || strchr(pat, '_') != NULL) {
                    return sql_like_match(pat, txt);
                }
                /* no % or _: the text contains the pattern, ignoring case, at any length */
                size_t plen = strlen(pat);
                for (const char* t = txt; ; t++) {
                    size_t i = 0;
                    while (i < plen && t[i] && tolower((unsigned char)t[i]) == tolower((unsigned char)pat[i])) i++;
                    if (i == plen) return true;
                    if (!*t) return false;
                }
            }
            return false;
        }
        default: return true;
    }
}
#define eval_condition enlng_eval_condition

EnlngQueryResult* enlngdb_find(EnlngTable* table, const char* filter_col, EnlngOp op, const EnlngVal* target_val, int limit, int offset) {
    if (!table) return NULL;
    double t_start = get_time_ms();

    int col_idx = -1;
    if (filter_col && op != OP_NONE) {
        for (int c = 0; c < table->col_count; c++) {
            if (strcmp(table->columns[c].name, filter_col) == 0) {
                col_idx = c;
                break;
            }
        }
    }

    EnlngQueryResult* res = (EnlngQueryResult*)calloc(1, sizeof(EnlngQueryResult));
    res->col_count = table->col_count;
    for (int c = 0; c < table->col_count; c++) {
        res->columns[c] = table->columns[c];
    }

    size_t matched_cap = 16;
    res->rows = (EnlngRow*)malloc(matched_cap * sizeof(EnlngRow));
    res->row_count = 0;

    int skipped = 0;
    for (size_t r = 0; r < table->row_count; r++) {
        EnlngRow* row = &table->rows[r];
        bool match = true;

        if (col_idx >= 0 && col_idx < row->cell_count) {
            match = eval_condition(&row->cells[col_idx], op, target_val);
        }

        if (match) {
            if (offset > 0 && skipped < offset) {
                skipped++;
                continue;
            }

            if (res->row_count >= matched_cap) {
                matched_cap *= 2;
                res->rows = (EnlngRow*)realloc(res->rows, matched_cap * sizeof(EnlngRow));
            }

            EnlngRow* dest_row = &res->rows[res->row_count++];
            dest_row->cell_count = row->cell_count;
            dest_row->cells = (EnlngVal*)malloc(row->cell_count * sizeof(EnlngVal));
            for (int c = 0; c < row->cell_count; c++) {
                dest_row->cells[c] = enlng_copy_val(&row->cells[c]);
            }

            if (limit > 0 && (int)res->row_count >= limit) {
                break;
            }
        }
    }

    res->execution_time_ms = get_time_ms() - t_start;
    return res;
}

int enlngdb_count(EnlngTable* table, const char* filter_col, EnlngOp op, const EnlngVal* target_val) {
    if (!table) return 0;
    if (!filter_col || op == OP_NONE) return (int)table->row_count;

    int col_idx = -1;
    for (int c = 0; c < table->col_count; c++) {
        if (strcmp(table->columns[c].name, filter_col) == 0) {
            col_idx = c;
            break;
        }
    }
    if (col_idx < 0) return 0;

    int count = 0;
    for (size_t r = 0; r < table->row_count; r++) {
        if (col_idx < table->rows[r].cell_count) {
            if (eval_condition(&table->rows[r].cells[col_idx], op, target_val)) {
                count++;
            }
        }
    }
    return count;
}

int enlngdb_update(EnlngTable* table, const char* filter_col, EnlngOp op, const EnlngVal* target_val, const char* set_col, const EnlngVal* set_val) {
    if (!table || !set_col || !set_val) return 0;

    int set_col_idx = -1;
    for (int c = 0; c < table->col_count; c++) {
        if (strcmp(table->columns[c].name, set_col) == 0) {
            set_col_idx = c;
            break;
        }
    }
    if (set_col_idx < 0) return 0;

    int filter_col_idx = -1;
    if (filter_col && op != OP_NONE) {
        for (int c = 0; c < table->col_count; c++) {
            if (strcmp(table->columns[c].name, filter_col) == 0) {
                filter_col_idx = c;
                break;
            }
        }
        if (filter_col_idx < 0) return 0;
    }

    int updated_count = 0;
    for (size_t r = 0; r < table->row_count; r++) {
        EnlngRow* row = &table->rows[r];
        bool match = true;
        if (filter_col_idx >= 0 && target_val) {
            if (filter_col_idx < row->cell_count) {
                match = eval_condition(&row->cells[filter_col_idx], op, target_val);
            } else {
                match = false;
            }
        }

        if (match) {
            if (set_col_idx < row->cell_count) {
                enlng_free_val(&row->cells[set_col_idx]);
                row->cells[set_col_idx] = enlng_copy_val(set_val);
                updated_count++;
            }
        }
    }

    if (updated_count > 0) table->version++;
    return updated_count;
}

int enlngdb_delete(EnlngTable* table, const char* filter_col, EnlngOp op, const EnlngVal* target_val, bool confirmed) {
    if (!table) return 0;
    if (!filter_col && !confirmed) {
        fprintf(stderr, "[SECURITY ALERT] Unconstrained delete requires 'confirmed'.\n");
        return 0;
    }

    int col_idx = -1;
    if (filter_col) {
        for (int c = 0; c < table->col_count; c++) {
            if (strcmp(table->columns[c].name, filter_col) == 0) {
                col_idx = c;
                break;
            }
        }
    }

    size_t write_idx = 0;
    int deleted = 0;
    for (size_t r = 0; r < table->row_count; r++) {
        EnlngRow* row = &table->rows[r];
        bool should_delete = false;
        if (col_idx >= 0 && col_idx < row->cell_count) {
            should_delete = eval_condition(&row->cells[col_idx], op, target_val);
        } else if (!filter_col && confirmed) {
            should_delete = true;
        }

        if (should_delete) {
            for (int c = 0; c < row->cell_count; c++) {
                enlng_free_val(&row->cells[c]);
            }
            free(row->cells);
            deleted++;
        } else {
            if (write_idx != r) {
                table->rows[write_idx] = table->rows[r];
            }
            write_idx++;
        }
    }
    table->row_count = write_idx;
    if (deleted > 0) table->version++;
    return deleted;
}

void enlngdb_free_result(EnlngQueryResult* res) {
    if (!res) return;
    for (size_t r = 0; r < res->row_count; r++) {
        for (int c = 0; c < res->rows[r].cell_count; c++) {
            enlng_free_val(&res->rows[r].cells[c]);
        }
        free(res->rows[r].cells);
    }
    free(res->rows);
    free(res);
}

void enlngdb_print_result(const EnlngQueryResult* res) {
    if (!res) return;
    if (res->col_count == 0) {
        printf("Empty set (0 rows) (%.2f ms)\n\n", res->execution_time_ms);
        return;
    }

    int col_widths[ENLNGDB_MAX_RESULT_COLS];
    for (int c = 0; c < res->col_count; c++) {
        col_widths[c] = (int)strlen(res->columns[c].name);
    }

    char buf[128];
    for (size_t r = 0; r < res->row_count; r++) {
        for (int c = 0; c < res->col_count; c++) {
            if (c < res->rows[r].cell_count) {
                EnlngVal* v = &res->rows[r].cells[c];
                int len = 4; // "null"
                if (v->type == ENLNG_VAL_INT) snprintf(buf, sizeof(buf), "%lld", (long long)v->int_val);
                else if (v->type == ENLNG_VAL_DOUBLE) snprintf(buf, sizeof(buf), "%.2f", v->double_val);
                else if (v->type == ENLNG_VAL_STRING) snprintf(buf, sizeof(buf), "%s", v->str_val ? v->str_val : "");
                else if (v->type == ENLNG_VAL_BOOL) snprintf(buf, sizeof(buf), "%s", v->bool_val ? "true" : "false");
                else strcpy(buf, "null");
                len = (int)strlen(buf);
                if (len > col_widths[c]) col_widths[c] = len;
            }
        }
    }

    // Border line
    printf("+");
    for (int c = 0; c < res->col_count; c++) {
        for (int i = 0; i < col_widths[c] + 2; i++) printf("-");
        printf("+");
    }
    printf("\n");

    // Header line
    printf("|");
    for (int c = 0; c < res->col_count; c++) {
        printf(" %-*s |", col_widths[c], res->columns[c].name);
    }
    printf("\n");

    // Border line
    printf("+");
    for (int c = 0; c < res->col_count; c++) {
        for (int i = 0; i < col_widths[c] + 2; i++) printf("-");
        printf("+");
    }
    printf("\n");

    // Data rows
    for (size_t r = 0; r < res->row_count; r++) {
        printf("|");
        for (int c = 0; c < res->col_count; c++) {
            buf[0] = '\0';
            if (c < res->rows[r].cell_count) {
                EnlngVal* v = &res->rows[r].cells[c];
                if (v->type == ENLNG_VAL_INT) snprintf(buf, sizeof(buf), "%lld", (long long)v->int_val);
                else if (v->type == ENLNG_VAL_DOUBLE) snprintf(buf, sizeof(buf), "%.2f", v->double_val);
                else if (v->type == ENLNG_VAL_STRING) snprintf(buf, sizeof(buf), "%s", v->str_val ? v->str_val : "");
                else if (v->type == ENLNG_VAL_BOOL) snprintf(buf, sizeof(buf), "%s", v->bool_val ? "true" : "false");
                else strcpy(buf, "null");
            }
            printf(" %-*s |", col_widths[c], buf);
        }
        printf("\n");
    }

    // Border line
    printf("+");
    for (int c = 0; c < res->col_count; c++) {
        for (int i = 0; i < col_widths[c] + 2; i++) printf("-");
        printf("+");
    }
    printf("\n");

    printf("%lu row(s) in set (%.2f ms)\n\n", (unsigned long)res->row_count, res->execution_time_ms);
}

void enlngdb_print_tables(const EnlngDatabase* db) {
    if (!db) return;
    printf("+---------------------------+\n");
    printf("| Tables_in_%-15s |\n", db->name);
    printf("+---------------------------+\n");
    for (int i = 0; i < db->table_count; i++) {
        printf("| %-25s |\n", db->tables[i].name);
    }
    printf("+---------------------------+\n");
    printf("%d table(s) in set (0.01 ms)\n\n", db->table_count);
}

/* Binary persistence. Version 2 writes every field by itself (no struct padding in the file);
 * version 1 files, from before column types, are still read. */
typedef struct {
    char name[ENLNGDB_MAX_NAME];
    EnlngValType type;
    bool is_primary_key;
    bool is_unique;
} ColumnV1;

static void put_text(FILE* f, const char* s) {
    uint32_t len = (uint32_t)strlen(s);
    fwrite(&len, sizeof(uint32_t), 1, f);
    if (len > 0) fwrite(s, 1, len, f);
}

static bool get_text(FILE* f, char* out, size_t cap) {
    uint32_t len = 0;
    if (fread(&len, sizeof(uint32_t), 1, f) != 1) return false;
    size_t keep = len < cap - 1 ? len : cap - 1;
    if (len > 0) {
        char* tmp = (char*)malloc(len);
        if (!tmp || fread(tmp, 1, len, f) != len) { free(tmp); return false; }
        memcpy(out, tmp, keep);
        free(tmp);
    }
    out[keep] = '\0';
    return true;
}

static void put_val(FILE* f, const EnlngVal* v) {
    uint8_t type_byte = (uint8_t)v->type;
    fwrite(&type_byte, 1, 1, f);
    if (v->type == ENLNG_VAL_INT) {
        fwrite(&v->int_val, sizeof(int64_t), 1, f);
    } else if (v->type == ENLNG_VAL_DOUBLE) {
        fwrite(&v->double_val, sizeof(double), 1, f);
    } else if (v->type == ENLNG_VAL_BOOL) {
        uint8_t b = v->bool_val ? 1 : 0;
        fwrite(&b, 1, 1, f);
    } else if (v->type == ENLNG_VAL_STRING) {
        uint32_t len = v->str_val ? (uint32_t)strlen(v->str_val) : 0;
        fwrite(&len, sizeof(uint32_t), 1, f);
        if (len > 0) fwrite(v->str_val, 1, len, f);
    }
}

static bool get_val(FILE* f, EnlngVal* v) {
    uint8_t type_byte = 0;
    if (fread(&type_byte, 1, 1, f) != 1) return false;
    v->type = (EnlngValType)type_byte;
    if (v->type == ENLNG_VAL_INT) {
        return fread(&v->int_val, sizeof(int64_t), 1, f) == 1;
    } else if (v->type == ENLNG_VAL_DOUBLE) {
        return fread(&v->double_val, sizeof(double), 1, f) == 1;
    } else if (v->type == ENLNG_VAL_BOOL) {
        uint8_t b = 0;
        if (fread(&b, 1, 1, f) != 1) return false;
        v->bool_val = (b != 0);
        return true;
    } else if (v->type == ENLNG_VAL_STRING) {
        uint32_t len = 0;
        if (fread(&len, sizeof(uint32_t), 1, f) != 1) return false;
        char* s = (char*)malloc((size_t)len + 1);
        if (len > 0 && fread(s, 1, len, f) != len) { free(s); v->type = ENLNG_VAL_NULL; return false; }
        s[len] = '\0';
        v->str_val = s;
        return true;
    }
    v->type = ENLNG_VAL_NULL;
    v->int_val = 0;
    return true;
}

bool enlngdb_save(const EnlngDatabase* db, const char* filepath) {
    if (!db || !filepath) return false;
    /* write beside the file and move it into place, so a crash while saving leaves the old file whole */
    char tmp_path[1100];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", filepath);
    FILE* f = fopen(tmp_path, "wb");
    if (!f) return false;

    int saved_tables = 0;
    for (int t = 0; t < db->table_count; t++) if (!db->tables[t].is_temp) saved_tables++;

    char magic[16] = {0};
    memcpy(magic, ENLNGDB_MAGIC, 14);
    fwrite(magic, 1, 16, f);
    fwrite(db->name, 1, ENLNGDB_MAX_NAME, f);
    fwrite(&saved_tables, sizeof(int), 1, f);

    for (int t = 0; t < db->table_count; t++) {
        const EnlngTable* tbl = &db->tables[t];
        if (tbl->is_temp) continue;
        fwrite(tbl->name, 1, ENLNGDB_MAX_NAME, f);
        put_text(f, tbl->view_sql);
        fwrite(&tbl->col_count, sizeof(int), 1, f);
        for (int c = 0; c < tbl->col_count; c++) {
            const EnlngColumn* col = &tbl->columns[c];
            fwrite(col->name, 1, ENLNGDB_MAX_NAME, f);
            uint8_t flags[5] = { (uint8_t)((col->kind & 0x0F) | ((col->on_delete & 3) << 4)), col->is_primary_key, col->is_unique, col->not_null, col->is_auto };
            fwrite(flags, 1, 5, f);
            put_text(f, col->default_text);
            put_text(f, col->check_text);
            put_text(f, col->ref_table);
            put_text(f, col->ref_col);
            fwrite(&tbl->auto_next[c], sizeof(int64_t), 1, f);
        }
        fwrite(&tbl->index_count, sizeof(int), 1, f);
        for (int i = 0; i < tbl->index_count; i++) {
            const EnlngIndex* ix = &tbl->indexes[i];
            put_text(f, ix->name);
            uint8_t u = ix->is_unique;
            fwrite(&u, 1, 1, f);
            fwrite(&ix->col_count, sizeof(int), 1, f);
            for (int k = 0; k < ix->col_count; k++) put_text(f, ix->cols[k]);
        }
        fwrite(&tbl->row_count, sizeof(size_t), 1, f);
        for (size_t r = 0; r < tbl->row_count; r++) {
            const EnlngRow* row = &tbl->rows[r];
            fwrite(&row->cell_count, sizeof(int), 1, f);
            for (int c = 0; c < row->cell_count; c++) put_val(f, &row->cells[c]);
        }
    }
    bool wrote = fflush(f) == 0 && ferror(f) == 0;
    if (fclose(f) != 0) wrote = false;
    if (!wrote) { remove(tmp_path); return false; }
    remove(filepath);
    if (rename(tmp_path, filepath) != 0) { remove(tmp_path); return false; }
    return true;
}

bool enlngdb_load(EnlngDatabase* db, const char* filepath) {
    if (!db || !filepath) return false;
    FILE* f = fopen(filepath, "rb");
    if (!f) return false;

    char magic[16];
    if (fread(magic, 1, 16, f) != 16) { fclose(f); return false; }
    bool v2 = memcmp(magic, ENLNGDB_MAGIC, 14) == 0;
    if (!v2 && memcmp(magic, ENLNGDB_MAGIC_V1, 14) != 0) {
        fclose(f);
        return false;
    }

    if (fread(db->name, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME) { fclose(f); return false; }
    int table_cnt = 0;
    if (fread(&table_cnt, sizeof(int), 1, f) != 1) { fclose(f); return false; }

    for (int t = 0; t < table_cnt; t++) {
        char tbl_name[ENLNGDB_MAX_NAME];
        char view_sql[1024] = {0};
        int col_cnt = 0;
        if (fread(tbl_name, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME) break;
        if (v2 && !get_text(f, view_sql, sizeof(view_sql))) break;
        if (fread(&col_cnt, sizeof(int), 1, f) != 1 || col_cnt < 0 || col_cnt > ENLNGDB_MAX_COLS) break;

        EnlngColumn cols[ENLNGDB_MAX_COLS];
        int64_t autos[ENLNGDB_MAX_COLS] = {0};
        memset(cols, 0, sizeof(cols));
        bool bad = false;
        if (v2) {
            for (int c = 0; c < col_cnt && !bad; c++) {
                uint8_t flags[5];
                bad = fread(cols[c].name, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME || fread(flags, 1, 5, f) != 5;
                if (bad) break;
                cols[c].type = ENLNG_VAL_STRING;
                cols[c].kind = flags[0] & 0x0F;
                cols[c].on_delete = (flags[0] >> 4) & 3;
                cols[c].is_primary_key = flags[1];
                cols[c].is_unique = flags[2];
                cols[c].not_null = flags[3];
                cols[c].is_auto = flags[4];
                bad = !get_text(f, cols[c].default_text, sizeof(cols[c].default_text)) ||
                      !get_text(f, cols[c].check_text, sizeof(cols[c].check_text)) ||
                      !get_text(f, cols[c].ref_table, sizeof(cols[c].ref_table)) ||
                      !get_text(f, cols[c].ref_col, sizeof(cols[c].ref_col)) ||
                      fread(&autos[c], sizeof(int64_t), 1, f) != 1;
            }
        } else {
            ColumnV1 old[ENLNGDB_MAX_COLS];
            bad = fread(old, sizeof(ColumnV1), col_cnt, f) != (size_t)col_cnt;
            bool key_seen = false;
            for (int c = 0; c < col_cnt && !bad; c++) {
                memcpy(cols[c].name, old[c].name, ENLNGDB_MAX_NAME);
                cols[c].type = ENLNG_VAL_STRING;
                /* version 1 used only the first key-like column to tell rows apart */
                cols[c].is_primary_key = old[c].is_primary_key && !key_seen;
                key_seen = key_seen || old[c].is_primary_key;
                cols[c].is_unique = old[c].is_unique;
            }
        }
        if (bad) break;

        EnlngTable* tbl = enlngdb_create_table(db, tbl_name, cols, col_cnt);
        if (!tbl) break;
        snprintf(tbl->view_sql, sizeof(tbl->view_sql), "%s", view_sql);
        for (int c = 0; c < col_cnt; c++) tbl->auto_next[c] = autos[c];

        if (v2) {
            int ix_cnt = 0;
            if (fread(&ix_cnt, sizeof(int), 1, f) != 1) break;
            for (int i = 0; i < ix_cnt; i++) {
                EnlngIndex ix;
                memset(&ix, 0, sizeof(ix));
                uint8_t u = 0;
                if (!get_text(f, ix.name, sizeof(ix.name)) || fread(&u, 1, 1, f) != 1 ||
                    fread(&ix.col_count, sizeof(int), 1, f) != 1 || ix.col_count < 0 || ix.col_count > 4) { bad = true; break; }
                ix.is_unique = u;
                for (int k = 0; k < ix.col_count; k++) {
                    if (!get_text(f, ix.cols[k], sizeof(ix.cols[k]))) { bad = true; break; }
                }
                if (bad) break;
                if (tbl->index_count < ENLNGDB_MAX_INDEXES) tbl->indexes[tbl->index_count++] = ix;
            }
            if (bad) break;
        }

        size_t row_cnt = 0;
        if (fread(&row_cnt, sizeof(size_t), 1, f) != 1) break;
        for (size_t r = 0; r < row_cnt; r++) {
            int cell_cnt = 0;
            if (fread(&cell_cnt, sizeof(int), 1, f) != 1 || cell_cnt < 0 || cell_cnt > ENLNGDB_MAX_COLS) { bad = true; break; }
            EnlngVal cells[ENLNGDB_MAX_COLS];
            int got = 0;
            for (; got < cell_cnt; got++) {
                if (!get_val(f, &cells[got])) { bad = true; break; }
            }
            if (!bad) enlngdb_append_row(tbl, cells, cell_cnt);
            for (int c = 0; c < got; c++) enlng_free_val(&cells[c]);
            if (bad) break;
        }
        if (bad) break;
    }
    fclose(f);
    return true;
}
