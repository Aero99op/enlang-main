#ifndef __USE_MINGW_ANSI_STDIO
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include "enlngdb.h"
#include <ctype.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#ifndef __EMSCRIPTEN__
#include <sys/file.h>
#include <sys/mman.h>
#endif
#endif

#define ENLNGDB_WRAP_ALLOC
#include "enlngdb.h"

/* printed text goes where the parser says (the screen, or a server's answer) */
#define printf enlngdb_printf

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

unsigned long long enlngdb_mutations = 1;

bool enlngdb_tables_room(EnlngDatabase* db, int n) {
    if (n <= db->table_cap) return true;
    if (n > ENLNGDB_MAX_TABLES) return false;
    int cap = db->table_cap ? db->table_cap : 8;
    while (cap < n) cap *= 2;
    if (cap > ENLNGDB_MAX_TABLES) cap = ENLNGDB_MAX_TABLES;
    EnlngTable* t = (EnlngTable*)realloc(db->tables, sizeof(EnlngTable) * (size_t)cap);
    if (!t) return false;
    memset(t + db->table_cap, 0, sizeof(EnlngTable) * (size_t)(cap - db->table_cap));
    db->tables = t;
    db->table_cap = cap;
    return true;
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

void (*enlngdb_change_hook)(EnlngTable* t, int what) = NULL;
void (*enlngdb_journal_free)(void* journal) = NULL;
#define CHANGE(t, what) do { if (enlngdb_change_hook) enlngdb_change_hook((t), (what)); } while (0)

void enlngdb_free_rows(EnlngTable* tbl) {
    enlngdb_mutations++;
    CHANGE(tbl, ENLNGDB_CH_FREE_ROWS);
    for (size_t r = 0; r < tbl->row_count; r++) {
        EnlngRow* row = &tbl->rowv[r];
        if (row->cell_count < 0) continue;          /* not read: it owns nothing */
        for (int c = 0; c < row->cell_count; c++) enlng_free_val(&row->cells[c]);
        free(row->cells);
    }
    free(tbl->rowv);
    tbl->rowv = NULL;
    tbl->row_count = 0;
    tbl->row_capacity = 0;
}

void enlngdb_clear(EnlngDatabase* db) {
    enlngdb_mutations++;
    if (!db) return;
    CHANGE(db->tables, ENLNGDB_CH_RESET);
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

static void enlngdb_free_sources(EnlngDatabase* db);

void enlngdb_free(EnlngDatabase* db) {
    enlngdb_mutations++;
    if (!db) return;
    enlngdb_unlock(db);
    free_saves(db, 0);
    if (db->undo) { enlngdb_free_tables(db->undo, db->undo_count); db->undo = NULL; db->undo_count = 0; }
    if (db->journal && enlngdb_journal_free) enlngdb_journal_free(db->journal);
    db->journal = NULL;
    if (db->txn_journal && enlngdb_txn_hooks) enlngdb_txn_hooks->drop(db->txn_journal);
    db->txn_journal = NULL;
    if (db->txn) {
        enlngdb_free(db->txn);
        db->txn = NULL;
    }
    enlngdb_clear(db);
    enlngdb_free_extras(db);
    free(db->tables);
    enlngdb_free_sources(db);
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
    c->journal = NULL;
    c->txn_journal = NULL;
    c->triggers = NULL;                 /* a copy is only of the tables; the lists stay with the open database */
    c->trigger_count = 0;
    c->procs = NULL;
    c->proc_count = 0;
    c->users = NULL;
    c->user_count = 0;
    c->grants = NULL;
    c->grant_count = 0;
    c->tables = NULL;
    c->table_cap = 0;
    c->sources = NULL;
    if (!enlngdb_tables_room(c, db->table_count > 0 ? db->table_count : 1)) { free(c); return NULL; }
    memcpy(c->tables, db->tables, sizeof(EnlngTable) * (size_t)db->table_count);
    for (int t = 0; t < db->table_count; t++) {
        const EnlngTable* src = &db->tables[t];
        EnlngTable* dst = &c->tables[t];
        dst->rowv = NULL;
        dst->row_count = 0;
        dst->row_capacity = 0;
        for (size_t r = 0; r < src->row_count; r++) {
            enlngdb_append_row(dst, (*enlngdb_row(src, r)).cells, (*enlngdb_row(src, r)).cell_count);
        }
    }
    return c;
}

const EnlngTxnHooks* enlngdb_txn_hooks = NULL;

bool enlngdb_begin(EnlngDatabase* db) {
    enlngdb_mutations++;
    if (!db || db->txn) return false;
    if (enlngdb_txn_hooks) return enlngdb_txn_hooks->begin(db);
    db->txn = copy_database(db);
    return db->txn != NULL;
}

bool enlngdb_commit(EnlngDatabase* db) {
    enlngdb_mutations++;
    if (!db || !db->txn) return false;
    if (db->txn_journal && enlngdb_txn_hooks) return enlngdb_txn_hooks->commit(db);
    free_saves(db, 0);
    enlngdb_free(db->txn);
    db->txn = NULL;
    return true;
}

/* the tables of saved become the tables of db; saved is used up */
static void take_tables(EnlngDatabase* db, EnlngDatabase* saved) {
    enlngdb_clear(db);
    free(db->tables);
    db->tables = saved->tables;   /* its rows now belong to db */
    db->table_cap = saved->table_cap;
    db->table_count = saved->table_count;
    free(saved);
}

bool enlngdb_rollback(EnlngDatabase* db) {
    enlngdb_mutations++;
    if (!db || !db->txn) return false;
    if (db->txn_journal && enlngdb_txn_hooks) return enlngdb_txn_hooks->rollback(db);
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
    enlngdb_mutations++;
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
        t[i].rowv = NULL;
        t[i].row_count = 0;
        t[i].row_capacity = 0;
        for (size_t r = 0; r < db->tables[i].row_count; r++) enlngdb_append_row(&t[i], (*enlngdb_row(&db->tables[i], r)).cells, (*enlngdb_row(&db->tables[i], r)).cell_count);
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
    enlngdb_mutations++;
    if (!db || !tables) return;
    enlngdb_clear(db);
    if (!enlngdb_tables_room(db, count)) { enlngdb_free_tables(tables, count); return; }
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
    if (db->txn_journal && enlngdb_txn_hooks) return enlngdb_txn_hooks->savepoint(db, name);
    EnlngDatabase* c = copy_database(db);
    if (!c) return false;
    db->saves[db->nsaves] = c;
    snprintf(db->save_names[db->nsaves], sizeof(db->save_names[0]), "%s", name);
    db->nsaves++;
    return true;
}

/* back to the savepoint: what came after it is undone, and the savepoint stays */
bool enlngdb_rollback_to(EnlngDatabase* db, const char* name) {
    enlngdb_mutations++;
    if (db && db->txn_journal && enlngdb_txn_hooks) return enlngdb_txn_hooks->rollback_to(db, name);
    int i = db ? find_save(db, name) : -1;
    if (i < 0) return false;
    EnlngDatabase* c = copy_database(db->saves[i]);
    if (!c) return false;
    free_saves(db, i + 1);
    take_tables(db, c);
    return true;
}

bool enlngdb_release(EnlngDatabase* db, const char* name) {
    if (db && db->txn_journal && enlngdb_txn_hooks) return enlngdb_txn_hooks->release(db, name);
    int i = db ? find_save(db, name) : -1;
    if (i < 0) return false;
    free_saves(db, i);
    return true;
}

EnlngTable* enlngdb_create_table(EnlngDatabase* db, const char* name, const EnlngColumn* cols, int col_count) {
    enlngdb_mutations++;
    if (!db || !name) return NULL;
    for (int i = 0; i < db->table_count; i++) {
        if (strcmp(db->tables[i].name, name) == 0) {
            return &db->tables[i];
        }
    }
    if (!enlngdb_tables_room(db, db->table_count + 1)) return NULL;

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
    CHANGE(tbl, ENLNGDB_CH_CREATED);
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
    enlngdb_mutations++;
    if (!db || !name) return false;
    if (!confirmed) {
        fprintf(stderr, "[SECURITY ALERT] Dropping table '%s' requires 'confirmed' parameter.\n", name);
        return false;
    }
    for (int i = 0; i < db->table_count; i++) {
        if (strcmp(db->tables[i].name, name) == 0) {
            EnlngTable* tbl = &db->tables[i];
            CHANGE(tbl, ENLNGDB_CH_DROP);
            for (size_t r = 0; r < tbl->row_count; r++) {
                EnlngRow* row = &tbl->rowv[r];
                if (row->cell_count < 0) continue;
                for (int c = 0; c < row->cell_count; c++) enlng_free_val(&row->cells[c]);
                free(row->cells);
            }
            free(tbl->rowv);

            for (int j = i; j < db->table_count - 1; j++) {
                db->tables[j] = db->tables[j + 1];
            }
            db->table_count--;
            enlngdb_forget_triggers_of(db, name);
            return true;
        }
    }
    return false;
}

bool enlngdb_delete_column(EnlngTable* table, const char* col_name) {
    enlngdb_mutations++;
    if (!table || !col_name) return false;
    int idx = -1;
    for (int c = 0; c < table->col_count; c++) {
        if (strcmp(table->columns[c].name, col_name) == 0) {
            idx = c;
            break;
        }
    }
    if (idx < 0) return false;
    CHANGE(table, ENLNGDB_CH_WHOLE);

    for (size_t r = 0; r < table->row_count; r++) {
        EnlngRow* row = &(*enlngdb_row(table, r));
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
    enlngdb_mutations++;
    EnlngTable* t = enlngdb_get_table(db, from);
    if (!t || enlngdb_get_table(db, to) || strlen(to) >= ENLNGDB_MAX_NAME) return false;
    CHANGE(t, ENLNGDB_CH_RENAME);
    strcpy(t->name, to);
    enlngdb_rename_triggers_of(db, from, to);
    for (int i = 0; i < db->table_count; i++) {
        for (int c = 0; c < db->tables[i].col_count; c++) {
            if (strcmp(db->tables[i].columns[c].ref_table, from) == 0) strcpy(db->tables[i].columns[c].ref_table, to);
        }
    }
    t->version++;
    return true;
}

bool enlngdb_rename_column(EnlngTable* table, const char* from, const char* to) {
    enlngdb_mutations++;
    int idx = -1;
    for (int c = 0; c < table->col_count; c++) {
        if (strcmp(table->columns[c].name, to) == 0) return false;
        if (strcmp(table->columns[c].name, from) == 0) idx = c;
    }
    if (idx < 0 || strlen(to) >= ENLNGDB_MAX_NAME) return false;
    CHANGE(table, ENLNGDB_CH_META);
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
    enlngdb_mutations++;
    if (!table || !col_name || table->col_count >= ENLNGDB_MAX_COLS) return false;
    for (int c = 0; c < table->col_count; c++) {
        if (strcmp(table->columns[c].name, col_name) == 0) return false;
    }
    CHANGE(table, ENLNGDB_CH_WHOLE);
    int idx = table->col_count++;
    memset(&table->columns[idx], 0, sizeof(EnlngColumn));
    strncpy(table->columns[idx].name, col_name, ENLNGDB_MAX_NAME - 1);
    table->columns[idx].type = ENLNG_VAL_STRING;

    for (size_t r = 0; r < table->row_count; r++) {
        EnlngRow* row = &(*enlngdb_row(table, r));
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
    enlngdb_mutations++;
    if (!table || !cells) return false;
    CHANGE(table, ENLNGDB_CH_APPEND);
    if (table->row_count >= table->row_capacity) {
        size_t new_cap = table->row_capacity == 0 ? 16 : table->row_capacity * 2;
        EnlngRow* new_rows = (EnlngRow*)realloc(table->rowv, new_cap * sizeof(EnlngRow));
        if (!new_rows) return false;
        table->rowv = new_rows;
        table->row_capacity = new_cap;
    }
    EnlngRow* target_row = &table->rowv[table->row_count];   /* a new place: nothing to read */
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
    enlngdb_mutations++;
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
        EnlngRow* row = &(*enlngdb_row(table, r));
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
        if (col_idx < (*enlngdb_row(table, r)).cell_count) {
            if (eval_condition(&(*enlngdb_row(table, r)).cells[col_idx], op, target_val)) {
                count++;
            }
        }
    }
    return count;
}

int enlngdb_update(EnlngTable* table, const char* filter_col, EnlngOp op, const EnlngVal* target_val, const char* set_col, const EnlngVal* set_val) {
    enlngdb_mutations++;
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
        EnlngRow* row = &(*enlngdb_row(table, r));
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
    enlngdb_mutations++;
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
        EnlngRow* row = &(*enlngdb_row(table, r));
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
                table->rowv[write_idx] = table->rowv[r];      /* moved as it is, read or not */
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

void enlngdb_free_extras(EnlngDatabase* db) {
    if (!db) return;
    for (int i = 0; i < db->trigger_count; i++) { free(db->triggers[i].when); free(db->triggers[i].body); }
    free(db->triggers);
    db->triggers = NULL;
    db->trigger_count = 0;
    for (int i = 0; i < db->proc_count; i++) { free(db->procs[i].params); free(db->procs[i].body); }
    free(db->procs);
    db->procs = NULL;
    db->proc_count = 0;
    free(db->users);
    db->users = NULL;
    db->user_count = 0;
    free(db->grants);
    db->grants = NULL;
    db->grant_count = 0;
}

void enlngdb_forget_triggers_of(EnlngDatabase* db, const char* table) {
    if (!db) return;
    int g = 0;
    for (int i = 0; i < db->grant_count; i++) {
        if (!db->grants[i].is_proc && strcmp(db->grants[i].object, table) == 0) continue;     /* rights on a table go with it */
        db->grants[g++] = db->grants[i];
    }
    db->grant_count = g;
    int w = 0;
    for (int i = 0; i < db->trigger_count; i++) {
        if (strcmp(db->triggers[i].table, table) == 0) { free(db->triggers[i].when); free(db->triggers[i].body); continue; }
        if (w != i) db->triggers[w] = db->triggers[i];
        w++;
    }
    db->trigger_count = w;
}

void enlngdb_rename_triggers_of(EnlngDatabase* db, const char* from, const char* to) {
    if (!db) return;
    for (int i = 0; i < db->grant_count; i++) {
        if (!db->grants[i].is_proc && strcmp(db->grants[i].object, from) == 0) snprintf(db->grants[i].object, sizeof(db->grants[i].object), "%s", to);
    }
    for (int i = 0; i < db->trigger_count; i++) {
        if (strcmp(db->triggers[i].table, from) == 0) snprintf(db->triggers[i].table, sizeof(db->triggers[i].table), "%s", to);
    }
}

static const char ENLNGDB_EXT_MAGIC[16] = "ENLNGDB-EXT1";
static const char ENLNGDB_EXT2_MAGIC[16] = "ENLNGDB-EXT2";
static const char ENLNGDB_EXT3_MAGIC[16] = "ENLNGDB-EXT3";
static const char ENLNGDB_EXT4_MAGIC[16] = "ENLNGDB-EXT4";     /* triggers that run once for a statement */

int enlngdb_list_files(const char* dir, const char* ext, char (*names)[ENLNGDB_MAX_NAME], long long* sizes, int max) {
    int n = 0;
    size_t el = strlen(ext);
#ifdef _WIN32
    char pattern[600];
    snprintf(pattern, sizeof(pattern), "%s\\*%s", dir && *dir ? dir : ".", ext);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        size_t l = strlen(fd.cFileName);
        if (l <= el || l - el >= ENLNGDB_MAX_NAME || _stricmp(fd.cFileName + l - el, ext) != 0 || n >= max) continue;
        memcpy(names[n], fd.cFileName, l - el);
        names[n][l - el] = '\0';
        sizes[n] = ((long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
        n++;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR* d = opendir(dir && *dir ? dir : ".");
    if (!d) return 0;
    struct dirent* e;
    while ((e = readdir(d)) != NULL && n < max) {
        size_t l = strlen(e->d_name);
        if (l <= el || l - el >= ENLNGDB_MAX_NAME || strcmp(e->d_name + l - el, ext) != 0) continue;
        char path[1200];
        snprintf(path, sizeof(path), "%s/%s", dir && *dir ? dir : ".", e->d_name);
        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        memcpy(names[n], e->d_name, l - el);
        names[n][l - el] = '\0';
        sizes[n] = (long long)st.st_size;
        n++;
    }
    closedir(d);
#endif
    /* sorted by name, as the folder may give them in any order */
    for (int i = 1; i < n; i++) {
        char nm[ENLNGDB_MAX_NAME];
        long long sz = sizes[i];
        memcpy(nm, names[i], sizeof(nm));
        int j = i - 1;
        while (j >= 0 && strcmp(names[j], nm) > 0) { memcpy(names[j + 1], names[j], sizeof(nm)); sizes[j + 1] = sizes[j]; j--; }
        memcpy(names[j + 1], nm, sizeof(nm));
        sizes[j + 1] = sz;
    }
    return n;
}

bool enlngdb_random_bytes(unsigned char* out, size_t n) {
#ifdef _WIN32
    typedef BOOLEAN (WINAPI *GenRandom)(PVOID, ULONG);
    static GenRandom gen = NULL;
    if (!gen) {
        HMODULE lib = LoadLibraryA("advapi32.dll");
        if (lib) gen = (GenRandom)(void*)GetProcAddress(lib, "SystemFunction036");     /* RtlGenRandom */
    }
    return gen && gen(out, (ULONG)n);
#else
    FILE* f = fopen("/dev/urandom", "rb");
    if (!f) return false;
    bool ok = fread(out, 1, n, f) == n;
    fclose(f);
    return ok;
#endif
}

/* What the file is read from or written to: the file itself, or bytes in memory (an encrypted file is opened, or
   gathered for sealing, in memory). */
typedef struct {
    FILE* f;                    /* NULL: the bytes below */
    unsigned char* buf;
    size_t len, cap, pos;
    bool bad;                   /* memory ran out while writing */
    bool borrowed;              /* the bytes belong to a source of the database (rows are read from them later) */
} EnlngIO;
typedef struct { fpos_t at; size_t pos; } EnlngIOMark;

static size_t eio_read(void* p, size_t size, size_t n, EnlngIO* io) {
    if (io->f) return fread(p, size, n, io->f);
    if (!size || io->pos >= io->len) return 0;
    size_t can = (io->len - io->pos) / size;
    if (can > n) can = n;
    memcpy(p, io->buf + io->pos, can * size);
    io->pos += can * size;
    return can;
}

static size_t eio_write(const void* p, size_t size, size_t n, EnlngIO* io) {
    if (io->f) return fwrite(p, size, n, io->f);
    size_t add = size * n;
    if (io->len + add > io->cap) {
        size_t cap = io->cap ? io->cap : (size_t)1 << 20;
        while (cap < io->len + add) cap *= 2;
        unsigned char* g = (unsigned char*)realloc(io->buf, cap);
        if (!g) { io->bad = true; return 0; }
        io->buf = g;
        io->cap = cap;
    }
    memcpy(io->buf + io->len, p, add);
    io->len += add;
    return n;
}

static bool eio_mark(EnlngIO* io, EnlngIOMark* m) {
    m->pos = io->pos;
    return io->f ? fgetpos(io->f, &m->at) == 0 : true;
}

static void eio_back(EnlngIO* io, const EnlngIOMark* m) {
    if (io->f) fsetpos(io->f, &m->at);
    else io->pos = m->pos;
}

static void eio_close(EnlngIO* io) {
    if (io->f) fclose(io->f);
    io->f = NULL;
    if (io->buf && !io->borrowed) { memset(io->buf, 0, io->len); free(io->buf); }
    io->buf = NULL;
}

static const char ENLNGDB_MAGIC_ENC[16] = "ENLNG_C_EDB_ENC1";
const char* enlngdb_open_password = NULL;
int enlngdb_load_error = 0;
static uint32_t le32(const uint8_t* p);
static void put32(uint8_t* p, uint32_t v);

bool enlngdb_set_password(EnlngDatabase* db, const char* password) {
    if (!db) return false;
    if (!password) {
        db->encrypted = false;
        memset(db->file_key, 0, sizeof(db->file_key));
        return true;
    }
    if (!enlngdb_random_bytes(db->file_salt, sizeof(db->file_salt))) return false;
    db->file_rounds = 200000;
    enlngdb_pbkdf2_sha256(password, db->file_salt, sizeof(db->file_salt), (int)db->file_rounds, db->file_key);
    db->encrypted = true;
    return true;
}

static void put_heap(EnlngIO* f, const char* s) {
    uint32_t len = s ? (uint32_t)strlen(s) : 0;
    eio_write(&len, sizeof(uint32_t), 1, f);
    if (len > 0) eio_write(s, 1, len, f);
}

static char* get_heap(EnlngIO* f) {
    uint32_t len = 0;
    if (eio_read(&len, sizeof(uint32_t), 1, f) != 1 || len > 16u * 1024u * 1024u) return NULL;
    char* s = (char*)malloc((size_t)len + 1);
    if (!s) return NULL;
    if (len > 0 && eio_read(s, 1, len, f) != len) { free(s); return NULL; }
    s[len] = '\0';
    return s;
}

static void put_text(EnlngIO* f, const char* s) {
    uint32_t len = (uint32_t)strlen(s);
    eio_write(&len, sizeof(uint32_t), 1, f);
    if (len > 0) eio_write(s, 1, len, f);
}

static bool get_text(EnlngIO* f, char* out, size_t cap) {
    uint32_t len = 0;
    if (eio_read(&len, sizeof(uint32_t), 1, f) != 1) return false;
    size_t keep = len < cap - 1 ? len : cap - 1;
    if (len > 0) {
        char* tmp = (char*)malloc(len);
        if (!tmp || eio_read(tmp, 1, len, f) != len) { free(tmp); return false; }
        memcpy(out, tmp, keep);
        free(tmp);
    }
    out[keep] = '\0';
    return true;
}

static void put_val(EnlngIO* f, const EnlngVal* v) {
    uint8_t type_byte = (uint8_t)v->type;
    eio_write(&type_byte, 1, 1, f);
    if (v->type == ENLNG_VAL_INT) {
        eio_write(&v->int_val, sizeof(int64_t), 1, f);
    } else if (v->type == ENLNG_VAL_DOUBLE) {
        eio_write(&v->double_val, sizeof(double), 1, f);
    } else if (v->type == ENLNG_VAL_BOOL) {
        uint8_t b = v->bool_val ? 1 : 0;
        eio_write(&b, 1, 1, f);
    } else if (v->type == ENLNG_VAL_STRING) {
        uint32_t len = v->str_val ? (uint32_t)strlen(v->str_val) : 0;
        eio_write(&len, sizeof(uint32_t), 1, f);
        if (len > 0) eio_write(v->str_val, 1, len, f);
    }
}

static bool get_val(EnlngIO* f, EnlngVal* v) {
    uint8_t type_byte = 0;
    if (eio_read(&type_byte, 1, 1, f) != 1) return false;
    v->type = (EnlngValType)type_byte;
    if (v->type == ENLNG_VAL_INT) {
        return eio_read(&v->int_val, sizeof(int64_t), 1, f) == 1;
    } else if (v->type == ENLNG_VAL_DOUBLE) {
        return eio_read(&v->double_val, sizeof(double), 1, f) == 1;
    } else if (v->type == ENLNG_VAL_BOOL) {
        uint8_t b = 0;
        if (eio_read(&b, 1, 1, f) != 1) return false;
        v->bool_val = (b != 0);
        return true;
    } else if (v->type == ENLNG_VAL_STRING) {
        uint32_t len = 0;
        if (eio_read(&len, sizeof(uint32_t), 1, f) != 1) return false;
        char* s = (char*)malloc((size_t)len + 1);
        if (len > 0 && eio_read(s, 1, len, f) != len) { free(s); v->type = ENLNG_VAL_NULL; return false; }
        s[len] = '\0';
        v->str_val = s;
        return true;
    }
    v->type = ENLNG_VAL_NULL;
    v->int_val = 0;
    return true;
}

#ifdef _WIN32
/* Windows 10 (1709) and later: the new file takes the name even while another program has the old one open or
   mapped (a reader of the server, a script reading it), and that program goes on reading the old one. */
static bool posix_rename(const char* tmp_path, const char* filepath) {
    typedef BOOL (WINAPI* SetInfo)(HANDLE, int, LPVOID, DWORD);
    static SetInfo set_info = NULL;
    static int looked = 0;
    if (!looked) {
        looked = 1;
        HMODULE k = GetModuleHandleA("kernel32.dll");
        if (k) set_info = (SetInfo)(void*)GetProcAddress(k, "SetFileInformationByHandle");
    }
    if (!set_info) return false;
    WCHAR full[1100];
    WCHAR wide[1100];
    if (!MultiByteToWideChar(CP_ACP, 0, filepath, -1, wide, 1100)) return false;
    DWORD fl = GetFullPathNameW(wide, 1100, full, NULL);
    if (fl == 0 || fl >= 1100) return false;
    struct { DWORD flags; HANDLE root; DWORD len; WCHAR name[1100]; } ri;
    memset(&ri, 0, sizeof(ri));
    ri.flags = 0x1 | 0x2;       /* FILE_RENAME_FLAG_REPLACE_IF_EXISTS | FILE_RENAME_FLAG_POSIX_SEMANTICS */
    memcpy(ri.name, full, (fl + 1) * sizeof(WCHAR));
    ri.len = fl * (DWORD)sizeof(WCHAR);
    HANDLE h = CreateFileA(tmp_path, DELETE | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    BOOL ok = set_info(h, 22 /* FileRenameInfoEx */, &ri, (DWORD)sizeof(ri));
    CloseHandle(h);
    return ok != 0;
}
#endif

bool enlngdb_replace_file(const char* tmp_path, const char* filepath) {
#ifdef _WIN32
    if (posix_rename(tmp_path, filepath)) return true;
    /* One step, never a moment without the file. Another program (a virus scanner, the search index) may hold
       the file for a moment just after it was written: then it is tried again, for up to about 2 seconds. */
    for (int attempt = 0; attempt < 40; attempt++) {
        if (MoveFileExA(tmp_path, filepath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
        DWORD e = GetLastError();
        if (e != ERROR_ACCESS_DENIED && e != ERROR_SHARING_VIOLATION && e != ERROR_LOCK_VIOLATION) break;
        Sleep(attempt < 10 ? 10 : 50);
    }
    return false;
#else
    if (rename(tmp_path, filepath) != 0) return false;
#ifndef __EMSCRIPTEN__
    {
        /* and the folder remembers the new name */
        char dir[1100];
        snprintf(dir, sizeof(dir), "%s", filepath);
        char* slash = strrchr(dir, '/');
        if (slash) *slash = '\0'; else snprintf(dir, sizeof(dir), ".");
        int dfd = open(dir, O_RDONLY);
        if (dfd >= 0) { fsync(dfd); close(dfd); }
    }
#endif
    return true;
#endif
}

/* ---- rows read on first use ----
 * A file of version 3 keeps, for each table, the rows as before, then the place of each row, then lookup indexes.
 * It is mapped into memory and nothing of its rows is read when it is opened: each row is read the first time a
 * statement needs it, and a lookup by the key uses the index kept in the file while nothing has changed. */
typedef struct EnlngSource {
    struct EnlngSource* next;
    unsigned char* base;
    size_t len;
    int mapped;                 /* 1: a view of a file; 0: a buffer of memory (an encrypted file, opened) */
} EnlngSource;

bool (*enlngdb_value_hash)(const EnlngVal* v, uint64_t* h) = NULL;
unsigned long long enlngdb_loaded_at = 0;

static void enlngdb_free_sources(EnlngDatabase* db) {
    EnlngSource* s = (EnlngSource*)db->sources;
    while (s) {
        EnlngSource* n = s->next;
#if defined(_WIN32)
        if (s->mapped) UnmapViewOfFile(s->base);
        else free(s->base);
#elif !defined(__EMSCRIPTEN__)
        if (s->mapped) munmap(s->base, s->len);
        else free(s->base);
#else
        free(s->base);
#endif
        free(s);
        s = n;
    }
    db->sources = NULL;
}

static EnlngSource* add_source(EnlngDatabase* db, unsigned char* base, size_t len, int mapped) {
    EnlngSource* s = (EnlngSource*)calloc(1, sizeof(EnlngSource));
    if (!s) return NULL;
    s->base = base;
    s->len = len;
    s->mapped = mapped;
    s->next = (EnlngSource*)db->sources;
    db->sources = s;
    return s;
}

/* the whole file in memory, mapped (read only) where the system can; NULL when it cannot */
static unsigned char* map_file(const char* path, size_t* len, int* mapped) {
    *mapped = 0;
#if defined(_WIN32)
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return NULL;
    LARGE_INTEGER sz;
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart <= 0 || (unsigned long long)sz.QuadPart > (unsigned long long)SIZE_MAX) { CloseHandle(h); return NULL; }
    HANDLE m = CreateFileMappingA(h, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(h);
    if (!m) return NULL;
    void* v = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(m);
    if (!v) return NULL;
    *len = (size_t)sz.QuadPart;
    *mapped = 1;
    return (unsigned char*)v;
#elif !defined(__EMSCRIPTEN__)
    int fd = open(path, O_RDONLY);
    if (fd < 0) return NULL;
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size <= 0) { close(fd); return NULL; }
    void* v = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (v == MAP_FAILED) return NULL;
    *len = (size_t)st.st_size;
    *mapped = 1;
    return (unsigned char*)v;
#else
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* b = n > 0 ? (unsigned char*)malloc((size_t)n) : NULL;
    if (!b || fread(b, 1, (size_t)n, f) != (size_t)n) { free(b); fclose(f); return NULL; }
    fclose(f);
    *len = (size_t)n;
    return b;
#endif
}

/* a value written in a file (a type byte, then the value); the place after it */
static const unsigned char* mem_val(const unsigned char* p, EnlngVal* v) {
    uint8_t type = *p++;
    v->type = (EnlngValType)type;
    if (type == ENLNG_VAL_INT) { memcpy(&v->int_val, p, 8); return p + 8; }
    if (type == ENLNG_VAL_DOUBLE) { memcpy(&v->double_val, p, 8); return p + 8; }
    if (type == ENLNG_VAL_BOOL) { v->bool_val = *p != 0; return p + 1; }
    if (type == ENLNG_VAL_STRING) {
        uint32_t n;
        memcpy(&n, p, 4);
        p += 4;
        char* t = (char*)malloc((size_t)n + 1);
        if (t) { memcpy(t, p, n); t[n] = '\0'; }
        v->str_val = t;
        if (!t) v->type = ENLNG_VAL_NULL;
        return p + n;
    }
    v->type = ENLNG_VAL_NULL;
    v->int_val = 0;
    return p;
}

/* the place after a value, without reading it */
static const unsigned char* mem_skip(const unsigned char* p) {
    uint8_t type = *p++;
    if (type == ENLNG_VAL_INT || type == ENLNG_VAL_DOUBLE) return p + 8;
    if (type == ENLNG_VAL_BOOL) return p + 1;
    if (type == ENLNG_VAL_STRING) { uint32_t n; memcpy(&n, p, 4); return p + 4 + n; }
    return p;
}

/* how many bytes a row takes in the file */
static size_t mem_row_len(const unsigned char* p) {
    const unsigned char* s = p;
    int n;
    memcpy(&n, p, 4);
    p += 4;
    for (int c = 0; c < n; c++) p = mem_skip(p);
    return (size_t)(p - s);
}

void enlngdb_load_row(const EnlngTable* tc, size_t i) {
    EnlngTable* t = (EnlngTable*)tc;
    EnlngRow* r = &t->rowv[i];
    const unsigned char* p = t->lazy + (uintptr_t)r->cells;
    int n;
    memcpy(&n, p, 4);
    p += 4;
    int cc = n > t->col_count ? n : t->col_count;
    EnlngVal* cells = (EnlngVal*)malloc((size_t)(cc ? cc : 1) * sizeof(EnlngVal));
    if (!cells) { fprintf(stderr, "ERROR: out of memory while reading a row\n"); exit(3); }
    for (int c = 0; c < n; c++) p = mem_val(p, &cells[c]);
    for (int c = n; c < cc; c++) cells[c] = enlng_null();
    r->cells = cells;
    r->cell_count = cc;
}

void enlngdb_materialize(EnlngDatabase* db) {
    for (int t = 0; t < db->table_count; t++) {
        EnlngTable* tb = &db->tables[t];
        for (size_t r = 0; r < tb->row_count; r++) (void)enlngdb_row(tb, r);
        tb->lazy = NULL;
        tb->nstored = 0;
    }
    enlngdb_free_sources(db);
}

typedef struct { uint64_t h; uint32_t row; } StoredEntry;
static int stored_cmp(const void* a, const void* b) {
    const StoredEntry* x = (const StoredEntry*)a;
    const StoredEntry* y = (const StoredEntry*)b;
    if (x->h != y->h) return x->h < y->h ? -1 : 1;
    return x->row < y->row ? -1 : x->row > y->row;
}

/* the columns of a table that get a lookup index in the file: its key (or id), unique columns, single-column indexes */
static int stored_columns(const EnlngTable* t, int* cols) {
    int n = 0;
    int keys = 0, key = -1;
    for (int c = 0; c < t->col_count; c++) if (t->columns[c].is_primary_key) { keys++; key = c; }
    if (keys == 0) for (int c = 0; c < t->col_count; c++) if (strcmp(t->columns[c].name, "id") == 0) key = c;
    if (keys <= 1 && key >= 0) cols[n++] = key;
    for (int c = 0; c < t->col_count && n < ENLNGDB_STORED_MAX; c++) {
        bool want = t->columns[c].is_unique;
        for (int i = 0; i < t->index_count && !want; i++) {
            if (t->indexes[i].col_count == 1 && strcmp(t->indexes[i].cols[0], t->columns[c].name) == 0) want = true;
        }
        bool have = false;
        for (int k = 0; k < n; k++) have = have || cols[k] == c;
        if (want && !have) cols[n++] = c;
    }
    return n;
}

/* the value of column c of row r, without reading the whole row when it is still in the file; *own when it must be freed */
static bool cell_of(const EnlngTable* t, size_t r, int c, EnlngVal* v) {
    const EnlngRow* row = &t->rowv[r];
    if (row->cell_count >= 0) {
        if (c < row->cell_count) *v = row->cells[c];
        else v->type = ENLNG_VAL_NULL;
        return false;
    }
    const unsigned char* p = t->lazy + (uintptr_t)row->cells;
    int n;
    memcpy(&n, p, 4);
    p += 4;
    if (c >= n) { v->type = ENLNG_VAL_NULL; return false; }
    for (int k = 0; k < c; k++) p = mem_skip(p);
    mem_val(p, v);
    return true;
}

/* the bytes a row takes when it is written */
static size_t row_bytes(const EnlngRow* row) {
    size_t n = 4;
    for (int c = 0; c < row->cell_count; c++) {
        const EnlngVal* v = &row->cells[c];
        n += 1;
        if (v->type == ENLNG_VAL_INT || v->type == ENLNG_VAL_DOUBLE) n += 8;
        else if (v->type == ENLNG_VAL_BOOL) n += 1;
        else if (v->type == ENLNG_VAL_STRING) n += 4 + (v->str_val ? strlen(v->str_val) : 0);
    }
    return n;
}

/* Version 3 rows of a table: the count (8 bytes), the bytes of the rows (8), the width of a place (1, and 7 zero
   bytes), the rows as in version 2, the place of each row from the first, then the lookup indexes: their count (4)
   and for each its column, entries and rows without a hash (4 each), the entries (8 bytes of hash, 4 of row, sorted)
   and those rows (4 each). A row still in the old file is copied as its bytes, without being read. */
static bool save_rows_v3(const EnlngTable* t, EnlngIO* f) {
    uint64_t rc = t->row_count;
    uint64_t* offs = (uint64_t*)malloc((size_t)(rc + 1) * sizeof(uint64_t));
    if (!offs) return false;
    uint64_t total = 0;
    for (size_t r = 0; r < t->row_count; r++) {
        const EnlngRow* row = &t->rowv[r];
        offs[r] = total;
        total += row->cell_count < 0 ? mem_row_len(t->lazy + (uintptr_t)row->cells) : row_bytes(row);
    }
    uint8_t width = total <= 0xFFFFFFFFu ? 4 : 8;
    uint8_t pad[7] = {0};
    eio_write(&rc, 8, 1, f);
    eio_write(&total, 8, 1, f);
    eio_write(&width, 1, 1, f);
    eio_write(pad, 1, 7, f);
    for (size_t r = 0; r < t->row_count; r++) {
        const EnlngRow* row = &t->rowv[r];
        if (row->cell_count < 0) {
            const unsigned char* p = t->lazy + (uintptr_t)row->cells;
            eio_write(p, 1, mem_row_len(p), f);
        } else {
            eio_write(&row->cell_count, sizeof(int), 1, f);
            for (int c = 0; c < row->cell_count; c++) put_val(f, &row->cells[c]);
        }
    }
    for (size_t r = 0; r < t->row_count; r++) {
        if (width == 4) { uint32_t o = (uint32_t)offs[r]; eio_write(&o, 4, 1, f); }
        else eio_write(&offs[r], 8, 1, f);
    }
    free(offs);
    int cols[ENLNGDB_STORED_MAX];
    int ncols = enlngdb_value_hash && rc <= 0xFFFFFFF0u && t->view_sql[0] == '\0' ? stored_columns(t, cols) : 0;
    uint32_t nidx = (uint32_t)ncols;
    eio_write(&nidx, 4, 1, f);
    for (int k = 0; k < ncols; k++) {
        StoredEntry* e = (StoredEntry*)malloc((size_t)(rc + 1) * sizeof(StoredEntry));
        uint32_t* spill = (uint32_t*)malloc((size_t)(rc + 1) * sizeof(uint32_t));
        uint32_t n = 0, ns = 0;
        if (!e || !spill) { free(e); free(spill); return false; }
        for (size_t r = 0; r < t->row_count; r++) {
            EnlngVal v;
            bool own = cell_of(t, r, cols[k], &v);
            uint64_t h;
            if (v.type != ENLNG_VAL_NULL && enlngdb_value_hash(&v, &h)) { e[n].h = h; e[n].row = (uint32_t)r; n++; }
            else spill[ns++] = (uint32_t)r;
            if (own) enlng_free_val(&v);
        }
        qsort(e, n, sizeof(StoredEntry), stored_cmp);
        uint32_t col = (uint32_t)cols[k];
        eio_write(&col, 4, 1, f);
        eio_write(&n, 4, 1, f);
        eio_write(&ns, 4, 1, f);
        for (uint32_t i = 0; i < n; i++) { eio_write(&e[i].h, 8, 1, f); eio_write(&e[i].row, 4, 1, f); }
        eio_write(spill, 4, ns, f);
        free(e);
        free(spill);
    }
    return true;
}

bool enlngdb_save(const EnlngDatabase* db, const char* filepath) {
    if (!db || !filepath) return false;
    /* write beside the file and move it into place, so a crash while saving leaves the old file whole */
    char tmp_path[1100];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", filepath);
    /* an encrypted database that was not opened is never written: that would put an empty one in its place */
    if (db->locked) return false;
    EnlngIO out;
    memset(&out, 0, sizeof(out));
    FILE* file = NULL;
    if (!db->encrypted) {
        file = fopen(tmp_path, "wb");
        if (!file) return false;
        setvbuf(file, NULL, _IOFBF, 1 << 20);
        out.f = file;
    }
    EnlngIO* f = &out;          /* encrypted: the bytes are gathered in memory, then sealed and written at once */

    int saved_tables = 0;
    for (int t = 0; t < db->table_count; t++) if (!db->tables[t].is_temp) saved_tables++;

    char magic[16] = {0};
    memcpy(magic, ENLNGDB_MAGIC_V3, 14);
    eio_write(magic, 1, 16, f);
    eio_write(db->name, 1, ENLNGDB_MAX_NAME, f);
    eio_write(&saved_tables, sizeof(int), 1, f);

    for (int t = 0; t < db->table_count; t++) {
        const EnlngTable* tbl = &db->tables[t];
        if (tbl->is_temp) continue;
        eio_write(tbl->name, 1, ENLNGDB_MAX_NAME, f);
        put_text(f, tbl->view_sql);
        eio_write(&tbl->col_count, sizeof(int), 1, f);
        for (int c = 0; c < tbl->col_count; c++) {
            const EnlngColumn* col = &tbl->columns[c];
            eio_write(col->name, 1, ENLNGDB_MAX_NAME, f);
            uint8_t flags[5] = { (uint8_t)((col->kind & 0x0F) | ((col->on_delete & 3) << 4) | ((col->on_update & 3) << 6)), col->is_primary_key, col->is_unique, col->not_null, col->is_auto };
            eio_write(flags, 1, 5, f);
            put_text(f, col->default_text);
            put_text(f, col->check_text);
            put_text(f, col->ref_table);
            put_text(f, col->ref_col);
            eio_write(&tbl->auto_next[c], sizeof(int64_t), 1, f);
        }
        eio_write(&tbl->index_count, sizeof(int), 1, f);
        for (int i = 0; i < tbl->index_count; i++) {
            const EnlngIndex* ix = &tbl->indexes[i];
            put_text(f, ix->name);
            uint8_t u = ix->is_unique;
            eio_write(&u, 1, 1, f);
            eio_write(&ix->col_count, sizeof(int), 1, f);
            for (int k = 0; k < ix->col_count; k++) put_text(f, ix->cols[k]);
        }
        if (!save_rows_v3(tbl, f)) out.bad = true;
    }
    if (db->trigger_count > 0 || db->proc_count > 0 || db->user_count > 0) {
        /* after triggers here; before triggers in a part of their own at the end, which a program that does not
           know them (and would read them as after triggers) never reaches */
        int n_after = 0, n_before = 0, n_stmt = 0;
        for (int i = 0; i < db->trigger_count; i++) {
            int ev = db->triggers[i].event;
            if (ev >= 20) n_stmt++;
            else if (ev >= 10) n_before++;
            else n_after++;
        }
        eio_write(ENLNGDB_EXT_MAGIC, 1, 16, f);
        eio_write(&n_after, sizeof(int), 1, f);
        for (int i = 0; i < db->trigger_count; i++) {
            const EnlngTrigger* tg = &db->triggers[i];
            if (tg->event >= 10) continue;
            eio_write(tg->name, 1, ENLNGDB_MAX_NAME, f);
            eio_write(tg->table, 1, ENLNGDB_MAX_NAME, f);
            eio_write(&tg->event, sizeof(int), 1, f);
            put_heap(f, tg->when);
            put_heap(f, tg->body);
        }
        eio_write(&db->proc_count, sizeof(int), 1, f);
        for (int i = 0; i < db->proc_count; i++) {
            const EnlngProc* pr = &db->procs[i];
            eio_write(pr->name, 1, ENLNGDB_MAX_NAME, f);
            put_heap(f, pr->params);
            put_heap(f, pr->body);
        }
        if (n_before > 0) {
            eio_write(ENLNGDB_EXT2_MAGIC, 1, 16, f);
            eio_write(&n_before, sizeof(int), 1, f);
            for (int i = 0; i < db->trigger_count; i++) {
                const EnlngTrigger* tg = &db->triggers[i];
                if (tg->event < 10 || tg->event >= 20) continue;
                eio_write(tg->name, 1, ENLNGDB_MAX_NAME, f);
                eio_write(tg->table, 1, ENLNGDB_MAX_NAME, f);
                eio_write(&tg->event, sizeof(int), 1, f);
                put_heap(f, tg->when);
                put_heap(f, tg->body);
            }
        }
        if (db->user_count > 0) {
            /* the users and what they may do; an older program stops before this part */
            eio_write(ENLNGDB_EXT3_MAGIC, 1, 16, f);
            eio_write(&db->user_count, sizeof(int), 1, f);
            for (int i = 0; i < db->user_count; i++) {
                const EnlngUser* u = &db->users[i];
                int admin = u->admin ? 1 : 0;
                eio_write(u->name, 1, ENLNGDB_MAX_NAME, f);
                eio_write(u->salt, 1, 16, f);
                eio_write(u->hash, 1, 32, f);
                eio_write(&u->rounds, sizeof(int), 1, f);
                eio_write(&admin, sizeof(int), 1, f);
            }
            eio_write(&db->grant_count, sizeof(int), 1, f);
            for (int i = 0; i < db->grant_count; i++) {
                const EnlngGrant* g = &db->grants[i];
                eio_write(g->user, 1, ENLNGDB_MAX_NAME, f);
                eio_write(g->object, 1, ENLNGDB_MAX_NAME, f);
                eio_write(&g->is_proc, sizeof(int), 1, f);
                eio_write(&g->rights, sizeof(unsigned int), 1, f);
            }
        }
        if (n_stmt > 0) {
            /* triggers for a whole statement; an older program stops before this part and does not run them */
            eio_write(ENLNGDB_EXT4_MAGIC, 1, 16, f);
            eio_write(&n_stmt, sizeof(int), 1, f);
            for (int i = 0; i < db->trigger_count; i++) {
                const EnlngTrigger* tg = &db->triggers[i];
                if (tg->event < 20) continue;
                eio_write(tg->name, 1, ENLNGDB_MAX_NAME, f);
                eio_write(tg->table, 1, ENLNGDB_MAX_NAME, f);
                eio_write(&tg->event, sizeof(int), 1, f);
                put_heap(f, tg->when);
                put_heap(f, tg->body);
            }
        }
    }
    if (db->encrypted) {
        /* "ENLNG_C_EDB_ENC1", the salt, the rounds, a new nonce, then the sealed file and its tag */
        unsigned char head[48];
        unsigned char nonce[12];
        bool good = !out.bad && enlngdb_random_bytes(nonce, sizeof(nonce)) && eio_write("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 1, 16, f) == 16;
        if (good) {
            out.len -= 16;      /* room for the tag */
            memset(head, 0, sizeof(head));
            memcpy(head, ENLNGDB_MAGIC_ENC, 16);
            memcpy(head + 16, db->file_salt, 16);
            put32(head + 32, db->file_rounds);
            memcpy(head + 36, nonce, 12);
            enlngdb_aead_seal(db->file_key, nonce, out.buf, out.len);
            file = fopen(tmp_path, "wb");
            good = file && fwrite(head, 1, sizeof(head), file) == sizeof(head) && fwrite(out.buf, 1, out.len + 16, file) == out.len + 16;
        }
        free(out.buf);
        if (!good) { if (file) fclose(file); remove(tmp_path); return false; }
    }
    bool wrote = fflush(file) == 0 && ferror(file) == 0;
    /* the new file is on the disk before it takes the old one's place: after a crash or a power cut the file is
       either the old one or the new one, whole */
#ifdef _WIN32
    if (wrote && _commit(_fileno(file)) != 0) wrote = false;
#elif !defined(__EMSCRIPTEN__)
    if (wrote && fsync(fileno(file)) != 0) wrote = false;
#endif
    if (fclose(file) != 0) wrote = false;
    if (!wrote) { remove(tmp_path); return false; }
    if (!enlngdb_replace_file(tmp_path, filepath)) { remove(tmp_path); return false; }
    return true;
}

/* adds a row at the end that owns these cells (they are not copied), as enlngdb_append_row does otherwise */
static bool take_row(EnlngTable* table, EnlngVal* cells, int cell_count) {
    enlngdb_mutations++;
    CHANGE(table, ENLNGDB_CH_APPEND);
    if (table->row_count >= table->row_capacity) {
        size_t new_cap = table->row_capacity == 0 ? 16 : table->row_capacity * 2;
        EnlngRow* new_rows = (EnlngRow*)realloc(table->rowv, new_cap * sizeof(EnlngRow));
        if (!new_rows) return false;
        table->rowv = new_rows;
        table->row_capacity = new_cap;
    }
    EnlngVal* own = (EnlngVal*)malloc((cell_count ? cell_count : 1) * sizeof(EnlngVal));
    if (!own) return false;
    memcpy(own, cells, (size_t)cell_count * sizeof(EnlngVal));
    EnlngRow* row = &table->rowv[table->row_count];
    row->cells = own;
    row->cell_count = cell_count;
    row->version = 1;
    table->row_count++;
    table->version++;
    return true;
}

/* skips n bytes of what is read */
static bool eio_skip(EnlngIO* io, uint64_t n) {
    if (!io->f) {
        if (n > io->len - io->pos) return false;
        io->pos += (size_t)n;
        return true;
    }
    unsigned char b[4096];
    while (n > 0) {
        size_t k = n > sizeof(b) ? sizeof(b) : (size_t)n;
        if (fread(b, 1, k, io->f) != k) return false;
        n -= k;
    }
    return true;
}

/* the rows of a version 3 table: in memory (a mapped file), only their places are taken; from a file read as a
   stream, the rows are read whole */
static bool load_rows_v3(EnlngTable* t, EnlngIO* f) {
    uint64_t rc = 0, total = 0;
    uint8_t width = 0, pad[7];
    if (eio_read(&rc, 8, 1, f) != 1 || eio_read(&total, 8, 1, f) != 1 || eio_read(&width, 1, 1, f) != 1 || eio_read(pad, 1, 7, f) != 7) return false;
    if ((width != 4 && width != 8) || rc > (uint64_t)SIZE_MAX / sizeof(EnlngRow)) return false;
    if (rc > t->row_capacity) {
        EnlngRow* nr = (EnlngRow*)realloc(t->rowv, (size_t)rc * sizeof(EnlngRow));
        if (!nr) return false;
        t->rowv = nr;
        t->row_capacity = (size_t)rc;
    }
    if (!f->f) {
        if (total > f->len - f->pos || rc * width > f->len - f->pos - total) return false;
        t->lazy = f->buf + f->pos;
        f->pos += (size_t)total;
        const unsigned char* o = f->buf + f->pos;
        for (uint64_t r = 0; r < rc; r++) {
            uint64_t off;
            if (width == 4) { uint32_t o32; memcpy(&o32, o + r * 4, 4); off = o32; }
            else memcpy(&off, o + r * 8, 8);
            if (off >= total) return false;
            EnlngRow* row = &t->rowv[r];
            row->cells = (EnlngVal*)(uintptr_t)off;
            row->cell_count = -1;
            row->version = 1;
        }
        t->row_count = (size_t)rc;
        f->pos += (size_t)(rc * width);
    } else {
        for (uint64_t r = 0; r < rc; r++) {
            int cell_cnt = 0;
            if (eio_read(&cell_cnt, sizeof(int), 1, f) != 1 || cell_cnt < 0 || cell_cnt > ENLNGDB_MAX_COLS) return false;
            EnlngVal cells[ENLNGDB_MAX_COLS];
            int got = 0;
            bool bad = false;
            for (; got < cell_cnt; got++) if (!get_val(f, &cells[got])) { bad = true; break; }
            if (!bad && take_row(t, cells, cell_cnt)) continue;
            for (int c = 0; c < got; c++) enlng_free_val(&cells[c]);
            return false;
        }
        if (!eio_skip(f, rc * width)) return false;
    }
    uint32_t nidx = 0;
    if (eio_read(&nidx, 4, 1, f) != 1 || nidx > 1000) return false;
    t->nstored = 0;
    for (uint32_t k = 0; k < nidx; k++) {
        uint32_t col = 0, n = 0, ns = 0;
        if (eio_read(&col, 4, 1, f) != 1 || eio_read(&n, 4, 1, f) != 1 || eio_read(&ns, 4, 1, f) != 1) return false;
        uint64_t bytes = (uint64_t)n * 12 + (uint64_t)ns * 4;
        if (!f->f && t->lazy && (int)col < t->col_count && t->nstored < ENLNGDB_STORED_MAX && n + (uint64_t)ns == rc && bytes <= f->len - f->pos) {
            t->stored[t->nstored].col = (int)col;
            t->stored[t->nstored].n = n;
            t->stored[t->nstored].nspill = ns;
            t->stored[t->nstored].at = f->buf + f->pos;
            t->stored[t->nstored].spill = f->buf + f->pos + (size_t)n * 12;
            t->nstored++;
        }
        if (!eio_skip(f, bytes)) return false;
    }
    return true;
}

bool enlngdb_load(EnlngDatabase* db, const char* filepath) {
    enlngdb_mutations++;
    enlngdb_load_error = 0;
    if (!db || !filepath) return false;
    FILE* file = fopen(filepath, "rb");
    if (!file) return false;
    setvbuf(file, NULL, _IOFBF, 1 << 20);
    EnlngIO in;
    memset(&in, 0, sizeof(in));
    in.f = file;
    EnlngIO* f = &in;

    char magic[16];
    if (eio_read(magic, 1, 16, f) != 16) { eio_close(f); return false; }
    if (memcmp(magic, ENLNGDB_MAGIC_ENC, 16) == 0) {
        /* encrypted: the whole file is opened in memory with the password, then read from there */
        unsigned char head[32];
        if (fread(head, 1, sizeof(head), file) != sizeof(head)) { eio_close(f); return false; }
        if (!enlngdb_open_password) { enlngdb_load_error = 1; eio_close(f); return false; }
        uint32_t rounds = le32(head + 16);
        if (rounds < 1000 || rounds > 10000000) { enlngdb_load_error = 2; eio_close(f); return false; }
        unsigned char* buf = NULL;
        size_t len = 0, cap = 0;
        for (;;) {
            if (len + (1 << 20) > cap) {
                cap = cap ? cap * 2 : (size_t)4 << 20;
                unsigned char* g = (unsigned char*)realloc(buf, cap);
                if (!g) { free(buf); eio_close(f); return false; }
                buf = g;
            }
            size_t got = fread(buf + len, 1, cap - len, file);
            len += got;
            if (got == 0) break;
        }
        fclose(file);
        in.f = NULL;
        unsigned char key[32];
        enlngdb_pbkdf2_sha256(enlngdb_open_password, head, 16, (int)rounds, key);
        if (len < 16 || !enlngdb_aead_open(key, head + 20, buf, len - 16)) {
            memset(key, 0, sizeof(key));
            free(buf);
            enlngdb_load_error = 2;
            return false;
        }
        db->encrypted = true;
        db->locked = false;
        memcpy(db->file_salt, head, 16);
        memcpy(db->file_key, key, 32);
        db->file_rounds = rounds;
        memset(key, 0, sizeof(key));
        in.buf = buf;
        in.len = len - 16;
        in.cap = len;
        in.pos = 0;
        if (eio_read(magic, 1, 16, f) != 16) { eio_close(f); return false; }
        if (memcmp(magic, ENLNGDB_MAGIC_V3, 14) == 0 && add_source(db, buf, len, 0)) in.borrowed = true;   /* rows are read from it later */
    } else if (memcmp(magic, ENLNGDB_MAGIC_V3, 14) == 0) {
        /* version 3: the file is mapped, and its rows are read when they are needed */
        size_t mlen = 0;
        int mapped = 0;
        unsigned char* base = map_file(filepath, &mlen, &mapped);
        if (base && mlen >= 16 && add_source(db, base, mlen, mapped)) {
            fclose(file);
            in.f = NULL;
            in.buf = base;
            in.len = mlen;
            in.cap = mlen;
            in.pos = 16;
            in.borrowed = true;
        } else if (base) {
#if defined(_WIN32)
            if (mapped) UnmapViewOfFile(base); else free(base);
#elif !defined(__EMSCRIPTEN__)
            if (mapped) munmap(base, mlen); else free(base);
#else
            free(base);
#endif
        }
    }
    bool v3 = memcmp(magic, ENLNGDB_MAGIC_V3, 14) == 0;
    bool v2 = v3 || memcmp(magic, ENLNGDB_MAGIC, 14) == 0;
    if (!v2 && memcmp(magic, ENLNGDB_MAGIC_V1, 14) != 0) {
        eio_close(f);
        return false;
    }

    if (eio_read(db->name, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME) { eio_close(f); return false; }
    int table_cnt = 0;
    if (eio_read(&table_cnt, sizeof(int), 1, f) != 1) { eio_close(f); return false; }

    for (int t = 0; t < table_cnt; t++) {
        char tbl_name[ENLNGDB_MAX_NAME];
        char view_sql[1024] = {0};
        int col_cnt = 0;
        if (eio_read(tbl_name, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME) break;
        if (v2 && !get_text(f, view_sql, sizeof(view_sql))) break;
        if (eio_read(&col_cnt, sizeof(int), 1, f) != 1 || col_cnt < 0 || col_cnt > ENLNGDB_MAX_COLS) break;

        EnlngColumn cols[ENLNGDB_MAX_COLS];
        int64_t autos[ENLNGDB_MAX_COLS] = {0};
        memset(cols, 0, sizeof(cols));
        bool bad = false;
        if (v2) {
            for (int c = 0; c < col_cnt && !bad; c++) {
                uint8_t flags[5];
                bad = eio_read(cols[c].name, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME || eio_read(flags, 1, 5, f) != 5;
                if (bad) break;
                cols[c].type = ENLNG_VAL_STRING;
                cols[c].kind = flags[0] & 0x0F;
                cols[c].on_delete = (flags[0] >> 4) & 3;
                cols[c].on_update = (flags[0] >> 6) & 3;
                cols[c].is_primary_key = flags[1];
                cols[c].is_unique = flags[2];
                cols[c].not_null = flags[3];
                cols[c].is_auto = flags[4];
                bad = !get_text(f, cols[c].default_text, sizeof(cols[c].default_text)) ||
                      !get_text(f, cols[c].check_text, sizeof(cols[c].check_text)) ||
                      !get_text(f, cols[c].ref_table, sizeof(cols[c].ref_table)) ||
                      !get_text(f, cols[c].ref_col, sizeof(cols[c].ref_col)) ||
                      eio_read(&autos[c], sizeof(int64_t), 1, f) != 1;
            }
        } else {
            ColumnV1 old[ENLNGDB_MAX_COLS];
            bad = eio_read(old, sizeof(ColumnV1), col_cnt, f) != (size_t)col_cnt;
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
            if (eio_read(&ix_cnt, sizeof(int), 1, f) != 1) break;
            for (int i = 0; i < ix_cnt; i++) {
                EnlngIndex ix;
                memset(&ix, 0, sizeof(ix));
                uint8_t u = 0;
                if (!get_text(f, ix.name, sizeof(ix.name)) || eio_read(&u, 1, 1, f) != 1 ||
                    eio_read(&ix.col_count, sizeof(int), 1, f) != 1 || ix.col_count < 0 || ix.col_count > 4) { bad = true; break; }
                ix.is_unique = u;
                for (int k = 0; k < ix.col_count; k++) {
                    if (!get_text(f, ix.cols[k], sizeof(ix.cols[k]))) { bad = true; break; }
                }
                if (bad) break;
                if (tbl->index_count < ENLNGDB_MAX_INDEXES) tbl->indexes[tbl->index_count++] = ix;
            }
            if (bad) break;
        }

        if (v3) {
            if (!load_rows_v3(tbl, f)) break;
            continue;
        }
        size_t row_cnt = 0;
        uint32_t rc32 = 0;
        if (eio_read(&rc32, sizeof(rc32), 1, f) != 1) break;
        row_cnt = rc32;
        {
            /* A 64-bit program before wrote the count in 8 bytes. Then the next 4 bytes are 0 (the high half), where
               in a file of 4 bytes they are the number of cells of the first row (never 0), the name of the next
               table or the part after the tables (neither starts with 0), or the end of the file. */
            uint32_t high = 1;
            EnlngIOMark at;                     /* also past 2 GB */
            bool marked = eio_mark(f, &at);
            if (rc32 != 0xFFFFFFFFu && eio_read(&high, sizeof(high), 1, f) == 1 && high == 0) {
                /* the 8-byte count, taken whole */
            } else if (marked) {
                eio_back(f, &at);
            }
        }
        if (rc32 == 0xFFFFFFFFu) {
            uint64_t rc64 = 0;
            if (eio_read(&rc64, sizeof(rc64), 1, f) != 1 || rc64 > (uint64_t)SIZE_MAX / sizeof(EnlngRow)) break;
            row_cnt = (size_t)rc64;
        }
        if (row_cnt > tbl->row_capacity) {
            /* the row list is made once at its size (it is not grown row by row) */
            EnlngRow* nr = (EnlngRow*)realloc(tbl->rowv, row_cnt * sizeof(EnlngRow));
            if (nr) { tbl->rowv = nr; tbl->row_capacity = row_cnt; }
        }
        for (size_t r = 0; r < row_cnt; r++) {
            int cell_cnt = 0;
            if (eio_read(&cell_cnt, sizeof(int), 1, f) != 1 || cell_cnt < 0 || cell_cnt > ENLNGDB_MAX_COLS) { bad = true; break; }
            EnlngVal cells[ENLNGDB_MAX_COLS];
            int got = 0;
            for (; got < cell_cnt; got++) {
                if (!get_val(f, &cells[got])) { bad = true; break; }
            }
            if (!bad && take_row(tbl, cells, cell_cnt)) continue;
            for (int c = 0; c < got; c++) enlng_free_val(&cells[c]);
            if (bad) break;
        }
        if (bad) break;
    }
    /* triggers and procedures, when the file has them (older programs stop before this and ignore it) */
    {
        char ext[16];
        int n = 0;
        if (eio_read(ext, 1, 16, f) == 16 && memcmp(ext, ENLNGDB_EXT_MAGIC, 16) == 0 && eio_read(&n, sizeof(int), 1, f) == 1 && n >= 0 && n < 100000) {
            enlngdb_free_extras(db);
            for (int i = 0; i < n; i++) {
                EnlngTrigger tg;
                memset(&tg, 0, sizeof(tg));
                if (eio_read(tg.name, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME || eio_read(tg.table, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME ||
                    eio_read(&tg.event, sizeof(int), 1, f) != 1) break;
                tg.when = get_heap(f);
                tg.body = get_heap(f);
                if (!tg.body) { free(tg.when); break; }
                if (tg.when && !tg.when[0]) { free(tg.when); tg.when = NULL; }
                db->triggers = (EnlngTrigger*)realloc(db->triggers, sizeof(EnlngTrigger) * (size_t)(db->trigger_count + 1));
                db->triggers[db->trigger_count++] = tg;
            }
            int np = 0;
            if (eio_read(&np, sizeof(int), 1, f) == 1 && np >= 0 && np < 100000) {
                for (int i = 0; i < np; i++) {
                    EnlngProc pr;
                    memset(&pr, 0, sizeof(pr));
                    if (eio_read(pr.name, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME) break;
                    pr.params = get_heap(f);
                    pr.body = get_heap(f);
                    if (!pr.params || !pr.body) { free(pr.params); free(pr.body); break; }
                    db->procs = (EnlngProc*)realloc(db->procs, sizeof(EnlngProc) * (size_t)(db->proc_count + 1));
                    db->procs[db->proc_count++] = pr;
                }
            }
            /* before triggers, then users: each part is there only when it has something */
            char ext2[16];
            int nb = 0;
            bool have_part = eio_read(ext2, 1, 16, f) == 16;
            if (have_part && memcmp(ext2, ENLNGDB_EXT2_MAGIC, 16) == 0 && eio_read(&nb, sizeof(int), 1, f) == 1 && nb >= 0 && nb < 100000) {
                for (int i = 0; i < nb; i++) {
                    EnlngTrigger tg;
                    memset(&tg, 0, sizeof(tg));
                    if (eio_read(tg.name, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME || eio_read(tg.table, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME ||
                        eio_read(&tg.event, sizeof(int), 1, f) != 1) break;
                    tg.when = get_heap(f);
                    tg.body = get_heap(f);
                    if (!tg.body || tg.event < 11 || tg.event > 13) { free(tg.when); free(tg.body); break; }
                    if (tg.when && !tg.when[0]) { free(tg.when); tg.when = NULL; }
                    db->triggers = (EnlngTrigger*)realloc(db->triggers, sizeof(EnlngTrigger) * (size_t)(db->trigger_count + 1));
                    db->triggers[db->trigger_count++] = tg;
                }
                have_part = eio_read(ext2, 1, 16, f) == 16;
            }
            int nu = 0;
            if (have_part && memcmp(ext2, ENLNGDB_EXT3_MAGIC, 16) == 0 && eio_read(&nu, sizeof(int), 1, f) == 1 && nu >= 0 && nu < 100000) {
                for (int i = 0; i < nu; i++) {
                    EnlngUser u;
                    int admin = 0;
                    memset(&u, 0, sizeof(u));
                    if (eio_read(u.name, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME || eio_read(u.salt, 1, 16, f) != 16 || eio_read(u.hash, 1, 32, f) != 32 ||
                        eio_read(&u.rounds, sizeof(int), 1, f) != 1 || eio_read(&admin, sizeof(int), 1, f) != 1) break;
                    u.name[ENLNGDB_MAX_NAME - 1] = '\0';
                    u.admin = admin != 0;
                    db->users = (EnlngUser*)realloc(db->users, sizeof(EnlngUser) * (size_t)(db->user_count + 1));
                    db->users[db->user_count++] = u;
                }
                int ng = 0;
                if (eio_read(&ng, sizeof(int), 1, f) == 1 && ng >= 0 && ng < 1000000) {
                    for (int i = 0; i < ng; i++) {
                        EnlngGrant g;
                        memset(&g, 0, sizeof(g));
                        if (eio_read(g.user, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME || eio_read(g.object, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME ||
                            eio_read(&g.is_proc, sizeof(int), 1, f) != 1 || eio_read(&g.rights, sizeof(unsigned int), 1, f) != 1) break;
                        g.user[ENLNGDB_MAX_NAME - 1] = g.object[ENLNGDB_MAX_NAME - 1] = '\0';
                        db->grants = (EnlngGrant*)realloc(db->grants, sizeof(EnlngGrant) * (size_t)(db->grant_count + 1));
                        db->grants[db->grant_count++] = g;
                    }
                }
                have_part = eio_read(ext2, 1, 16, f) == 16;
            }
            int ns = 0;
            if (have_part && memcmp(ext2, ENLNGDB_EXT4_MAGIC, 16) == 0 && eio_read(&ns, sizeof(int), 1, f) == 1 && ns >= 0 && ns < 100000) {
                for (int i = 0; i < ns; i++) {
                    EnlngTrigger tg;
                    memset(&tg, 0, sizeof(tg));
                    if (eio_read(tg.name, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME || eio_read(tg.table, 1, ENLNGDB_MAX_NAME, f) != ENLNGDB_MAX_NAME ||
                        eio_read(&tg.event, sizeof(int), 1, f) != 1) break;
                    tg.when = get_heap(f);
                    tg.body = get_heap(f);
                    if (!tg.body || tg.event < 21 || tg.event > 23) { free(tg.when); free(tg.body); break; }
                    if (tg.when && !tg.when[0]) { free(tg.when); tg.when = NULL; }
                    db->triggers = (EnlngTrigger*)realloc(db->triggers, sizeof(EnlngTrigger) * (size_t)(db->trigger_count + 1));
                    db->triggers[db->trigger_count++] = tg;
                }
            }
        }
    }
    eio_close(f);
    enlngdb_loaded_at = enlngdb_mutations;      /* the indexes kept in the file are good from here */
    return true;
}

/* ---- ChaCha20 and Poly1305 (RFC 8439): the server's messages and encrypted database files ---- */
static uint32_t le32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static void put32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
#define ROTL(v, n) (((v) << (n)) | ((v) >> (32 - (n))))
#define QR(a, b, c, d) a += b; d ^= a; d = ROTL(d, 16); c += d; b ^= c; b = ROTL(b, 12); a += b; d ^= a; d = ROTL(d, 8); c += d; b ^= c; b = ROTL(b, 7)

static void chacha_block(const uint8_t key[32], uint32_t counter, const uint8_t nonce[12], uint8_t out[64]) {
    uint32_t s[16], x[16];
    s[0] = 0x61707865; s[1] = 0x3320646e; s[2] = 0x79622d32; s[3] = 0x6b206574;
    for (int i = 0; i < 8; i++) s[4 + i] = le32(key + 4 * i);
    s[12] = counter;
    for (int i = 0; i < 3; i++) s[13 + i] = le32(nonce + 4 * i);
    memcpy(x, s, sizeof(s));
    for (int i = 0; i < 10; i++) {
        QR(x[0], x[4], x[8], x[12]); QR(x[1], x[5], x[9], x[13]); QR(x[2], x[6], x[10], x[14]); QR(x[3], x[7], x[11], x[15]);
        QR(x[0], x[5], x[10], x[15]); QR(x[1], x[6], x[11], x[12]); QR(x[2], x[7], x[8], x[13]); QR(x[3], x[4], x[9], x[14]);
    }
    for (int i = 0; i < 16; i++) put32(out + 4 * i, x[i] + s[i]);
}

static void chacha_xor(const uint8_t key[32], uint32_t counter, const uint8_t nonce[12], uint8_t* data, size_t n) {
    uint8_t ks[64];
    for (size_t off = 0; off < n; off += 64, counter++) {
        chacha_block(key, counter, nonce, ks);
        size_t k = n - off < 64 ? n - off : 64;
        for (size_t i = 0; i < k; i++) data[off + i] ^= ks[i];
    }
}

typedef struct { uint32_t r[5], h[5], pad[4]; size_t left; uint8_t buf[16]; int final; } Poly;

static void poly_init(Poly* st, const uint8_t key[32]) {
    st->r[0] = (le32(key + 0)) & 0x3ffffff;
    st->r[1] = (le32(key + 3) >> 2) & 0x3ffff03;
    st->r[2] = (le32(key + 6) >> 4) & 0x3ffc0ff;
    st->r[3] = (le32(key + 9) >> 6) & 0x3f03fff;
    st->r[4] = (le32(key + 12) >> 8) & 0x00fffff;
    for (int i = 0; i < 5; i++) st->h[i] = 0;
    for (int i = 0; i < 4; i++) st->pad[i] = le32(key + 16 + 4 * i);
    st->left = 0;
    st->final = 0;
}

static void poly_blocks(Poly* st, const uint8_t* m, size_t n) {
    const uint32_t hibit = st->final ? 0 : (1u << 24);
    uint32_t r0 = st->r[0], r1 = st->r[1], r2 = st->r[2], r3 = st->r[3], r4 = st->r[4];
    uint32_t s1 = r1 * 5, s2 = r2 * 5, s3 = r3 * 5, s4 = r4 * 5;
    uint32_t h0 = st->h[0], h1 = st->h[1], h2 = st->h[2], h3 = st->h[3], h4 = st->h[4];
    while (n >= 16) {
        h0 += (le32(m + 0)) & 0x3ffffff;
        h1 += (le32(m + 3) >> 2) & 0x3ffffff;
        h2 += (le32(m + 6) >> 4) & 0x3ffffff;
        h3 += (le32(m + 9) >> 6) & 0x3ffffff;
        h4 += (le32(m + 12) >> 8) | hibit;
        uint64_t d0 = (uint64_t)h0 * r0 + (uint64_t)h1 * s4 + (uint64_t)h2 * s3 + (uint64_t)h3 * s2 + (uint64_t)h4 * s1;
        uint64_t d1 = (uint64_t)h0 * r1 + (uint64_t)h1 * r0 + (uint64_t)h2 * s4 + (uint64_t)h3 * s3 + (uint64_t)h4 * s2;
        uint64_t d2 = (uint64_t)h0 * r2 + (uint64_t)h1 * r1 + (uint64_t)h2 * r0 + (uint64_t)h3 * s4 + (uint64_t)h4 * s3;
        uint64_t d3 = (uint64_t)h0 * r3 + (uint64_t)h1 * r2 + (uint64_t)h2 * r1 + (uint64_t)h3 * r0 + (uint64_t)h4 * s4;
        uint64_t d4 = (uint64_t)h0 * r4 + (uint64_t)h1 * r3 + (uint64_t)h2 * r2 + (uint64_t)h3 * r1 + (uint64_t)h4 * r0;
        uint32_t c = (uint32_t)(d0 >> 26); h0 = (uint32_t)d0 & 0x3ffffff;
        d1 += c; c = (uint32_t)(d1 >> 26); h1 = (uint32_t)d1 & 0x3ffffff;
        d2 += c; c = (uint32_t)(d2 >> 26); h2 = (uint32_t)d2 & 0x3ffffff;
        d3 += c; c = (uint32_t)(d3 >> 26); h3 = (uint32_t)d3 & 0x3ffffff;
        d4 += c; c = (uint32_t)(d4 >> 26); h4 = (uint32_t)d4 & 0x3ffffff;
        h0 += c * 5; c = h0 >> 26; h0 &= 0x3ffffff;
        h1 += c;
        m += 16;
        n -= 16;
    }
    st->h[0] = h0; st->h[1] = h1; st->h[2] = h2; st->h[3] = h3; st->h[4] = h4;
}

static void poly_update(Poly* st, const uint8_t* m, size_t n) {
    if (st->left) {
        size_t want = 16 - st->left;
        if (want > n) want = n;
        memcpy(st->buf + st->left, m, want);
        st->left += want; m += want; n -= want;
        if (st->left < 16) return;
        poly_blocks(st, st->buf, 16);
        st->left = 0;
    }
    if (n >= 16) { size_t full = n & ~(size_t)15; poly_blocks(st, m, full); m += full; n -= full; }
    if (n) { memcpy(st->buf, m, n); st->left = n; }
}

static void poly_finish(Poly* st, uint8_t mac[16]) {
    if (st->left) {
        size_t i = st->left;
        st->buf[i++] = 1;
        for (; i < 16; i++) st->buf[i] = 0;
        st->final = 1;
        poly_blocks(st, st->buf, 16);
    }
    uint32_t h0 = st->h[0], h1 = st->h[1], h2 = st->h[2], h3 = st->h[3], h4 = st->h[4];
    uint32_t c = h1 >> 26; h1 &= 0x3ffffff;
    h2 += c; c = h2 >> 26; h2 &= 0x3ffffff;
    h3 += c; c = h3 >> 26; h3 &= 0x3ffffff;
    h4 += c; c = h4 >> 26; h4 &= 0x3ffffff;
    h0 += c * 5; c = h0 >> 26; h0 &= 0x3ffffff;
    h1 += c;
    uint32_t g0 = h0 + 5; c = g0 >> 26; g0 &= 0x3ffffff;
    uint32_t g1 = h1 + c; c = g1 >> 26; g1 &= 0x3ffffff;
    uint32_t g2 = h2 + c; c = g2 >> 26; g2 &= 0x3ffffff;
    uint32_t g3 = h3 + c; c = g3 >> 26; g3 &= 0x3ffffff;
    uint32_t g4 = h4 + c - (1u << 26);
    uint32_t mask = (g4 >> 31) - 1;
    g0 &= mask; g1 &= mask; g2 &= mask; g3 &= mask; g4 &= mask;
    mask = ~mask;
    h0 = (h0 & mask) | g0; h1 = (h1 & mask) | g1; h2 = (h2 & mask) | g2; h3 = (h3 & mask) | g3; h4 = (h4 & mask) | g4;
    h0 = (h0 | (h1 << 26));
    h1 = ((h1 >> 6) | (h2 << 20));
    h2 = ((h2 >> 12) | (h3 << 14));
    h3 = ((h3 >> 18) | (h4 << 8));
    uint64_t f = (uint64_t)h0 + st->pad[0]; h0 = (uint32_t)f;
    f = (uint64_t)h1 + st->pad[1] + (f >> 32); h1 = (uint32_t)f;
    f = (uint64_t)h2 + st->pad[2] + (f >> 32); h2 = (uint32_t)f;
    f = (uint64_t)h3 + st->pad[3] + (f >> 32); h3 = (uint32_t)f;
    put32(mac, h0); put32(mac + 4, h1); put32(mac + 8, h2); put32(mac + 12, h3);
}

/* the tag of AEAD ChaCha20-Poly1305 over a ciphertext (no extra data) */
static void aead_tag(const uint8_t key[32], const uint8_t nonce[12], const uint8_t* ct, size_t n, uint8_t tag[16]) {
    uint8_t block[64];
    chacha_block(key, 0, nonce, block);
    Poly st;
    poly_init(&st, block);
    static const uint8_t zeros[16] = {0};
    poly_update(&st, ct, n);
    if (n % 16) poly_update(&st, zeros, 16 - n % 16);
    uint8_t lens[16] = {0};
    uint64_t ln = n;
    for (int i = 0; i < 8; i++) lens[8 + i] = (uint8_t)(ln >> (8 * i));
    poly_update(&st, lens, 16);
    poly_finish(&st, tag);
    memset(block, 0, sizeof(block));
}

/* seals data in place and writes the tag after it (data must have room for 16 more bytes) */
void enlngdb_aead_seal(const uint8_t key[32], const uint8_t nonce[12], uint8_t* data, size_t n) {
    chacha_xor(key, 1, nonce, data, n);
    aead_tag(key, nonce, data, n, data + n);
}

/* opens n bytes of ciphertext followed by the tag; false if anything was changed */
bool enlngdb_aead_open(const uint8_t key[32], const uint8_t nonce[12], uint8_t* data, size_t n) {
    uint8_t tag[16];
    aead_tag(key, nonce, data, n, tag);
    uint8_t diff = 0;
    for (int i = 0; i < 16; i++) diff |= (uint8_t)(tag[i] ^ data[n + i]);
    if (diff) return false;
    chacha_xor(key, 1, nonce, data, n);
    return true;
}


/* ---- memory -------------------------------------------------------------------------------------------------
 * Up to a budget the engine takes memory from the C library. Past it, new memory comes from a file mapped into
 * the program: the system keeps in memory only the pages of it that are used and writes the others to the file,
 * so the rows of a database can be larger than the memory of the computer (only the disk limits them). Memory
 * from the file does not count against the memory the system promises to programs, as it has a file behind it.
 *
 * The file is cut into chunks of 64 MB. A small block (up to 32 KB) comes from a list per size; a middle one (up
 * to 16 MB) is a power of two; a larger one gets chunks of its own, given back to the system when it is freed.
 * Every block of the file starts with 16 bytes that say its size. A block of the C library is told apart by its
 * address (the chunks are kept sorted), so memory of either kind can be freed or grown with the same calls. */
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef strdup

#if defined(__EMSCRIPTEN__)
void* enlngdb_malloc(size_t n) { return malloc(n); }
void* enlngdb_calloc(size_t n, size_t size) { return calloc(n, size); }
void* enlngdb_realloc(void* p, size_t n) { return realloc(p, n); }
void enlngdb_mfree(void* p) { free(p); }
char* enlngdb_strdup(const char* s) { return strdup(s); }
void enlngdb_memory_stats(size_t* heap, size_t* heap_peak, size_t* spill, size_t* spill_peak) {
    if (heap) *heap = 0;
    if (heap_peak) *heap_peak = 0;
    if (spill) *spill = 0;
    if (spill_peak) *spill_peak = 0;
}
#else

#ifdef _WIN32
#include <malloc.h>
#include <winioctl.h>
#define A_LIBC_SIZE(p) _msize(p)
#elif defined(__APPLE__)
#include <malloc/malloc.h>
#include <sys/mman.h>
#define A_LIBC_SIZE(p) malloc_size(p)
#else
#include <malloc.h>
#include <sys/mman.h>
#define A_LIBC_SIZE(p) malloc_usable_size(p)
#endif

#define A_CHUNK ((size_t)64 << 20)
#define A_SMALL_MAX ((size_t)32768)
#define A_MID_MAX ((size_t)16 << 20)
#define A_MAGIC 0xED8A110Cu
#define A_HUGE 0xFFFFu

typedef struct { uint32_t magic; uint32_t cls; uint64_t size; } AHdr;        /* size: the whole block */
typedef struct { char* base; size_t size; uint64_t off; int huge; } AChunk;

static int a_state;                 /* 0 not started, 1 the C library only, 2 the file may be used */
static size_t a_budget;
static size_t a_heap, a_heap_peak, a_spill, a_spill_peak;
static AChunk* a_chunks;
static int a_nchunks, a_capchunks;
static char* a_lo;
static char* a_hi;
static char* a_cur;
static char* a_end;
static uint64_t a_file_end;
static void* a_free_small[80];
static void* a_free_mid[16];
static uint32_t a_cls_size[80];
static int a_ncls;
static uint8_t a_cls_of[A_SMALL_MAX / 16 + 1];
static int a_file_ready;            /* 0 not yet, 1 open, -1 could not be made */
#ifdef _WIN32
static HANDLE a_file = INVALID_HANDLE_VALUE;
#else
static int a_fd = -1;
#endif

static size_t a_parse_size(const char* s) {
    char* end = NULL;
    double v = strtod(s, &end);
    if (end == s || v < 0) return 0;
    while (*end == ' ') end++;
    char u = (char)toupper((unsigned char)*end);
    if (u == 'K') v *= 1024.0;
    else if (u == 'M') v *= 1024.0 * 1024.0;
    else if (u == 'G') v *= 1024.0 * 1024.0 * 1024.0;
    else if (u == 'T') v *= 1024.0 * 1024.0 * 1024.0 * 1024.0;
    return (size_t)v;
}

static size_t a_physical_memory(void) {
#ifdef _WIN32
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) return (size_t)ms.ullTotalPhys;
    return (size_t)4 << 30;
#else
    long pages = sysconf(_SC_PHYS_PAGES), psz = sysconf(_SC_PAGESIZE);
    if (pages > 0 && psz > 0) return (size_t)pages * (size_t)psz;
    return (size_t)4 << 30;
#endif
}

static void a_report(void) {
    fprintf(stderr, "enlngdb memory: %.1f MB in memory at most, %.1f MB in the file at most\n",
            a_heap_peak / 1048576.0, a_spill_peak / 1048576.0);
}

static void a_start(void) {
    a_state = 1;
    const char* m = getenv("ENLNGDB_MEMORY");
    if (m && *m) {
        if (strcmp(m, "all") == 0 || strcmp(m, "off") == 0) return;
        a_budget = a_parse_size(m);
        if (a_budget < ((size_t)4 << 20)) a_budget = (size_t)4 << 20;
    } else {
        a_budget = a_physical_memory() / 2;
        /* a 32-bit program has 2 GB of addresses in all: past 1 GB the rest is left for the file */
        if (sizeof(void*) < 8 && a_budget > ((size_t)1 << 30)) a_budget = (size_t)1 << 30;
    }
    /* the sizes of small blocks: 16 to 256 by 16, then four sizes for each doubling */
    uint32_t s = 16;
    while (s <= 256) { a_cls_size[a_ncls++] = s; s += 16; }
    for (uint32_t p = 256; p < A_SMALL_MAX; p *= 2)
        for (int k = 1; k <= 4; k++) a_cls_size[a_ncls++] = p + p / 4 * (uint32_t)k;
    int c = 0;
    for (size_t i = 0; i <= A_SMALL_MAX / 16; i++) {
        while (a_cls_size[c] < i * 16) c++;
        a_cls_of[i] = (uint8_t)c;
    }
    const char* st = getenv("ENLNGDB_MEMSTAT");
    if (st && *st && *st != '0') atexit(a_report);
    a_state = 2;
}

static bool a_open_file(void) {
    if (a_file_ready) return a_file_ready > 0;
    a_file_ready = -1;
    char dir[600];
    const char* d = getenv("ENLNGDB_SPILL_DIR");
#ifdef _WIN32
    if (d && *d) snprintf(dir, sizeof(dir), "%s", d);
    else if (!GetTempPathA(sizeof(dir), dir)) snprintf(dir, sizeof(dir), ".");
    size_t dl = strlen(dir);
    while (dl > 0 && (dir[dl - 1] == '\\' || dir[dl - 1] == '/')) dir[--dl] = '\0';
    char path[700];
    unsigned char r[8] = {0};
    enlngdb_random_bytes(r, sizeof(r));
    snprintf(path, sizeof(path), "%s\\enlngdb-memory-%lu-%02x%02x%02x%02x.tmp", dir, (unsigned long)GetCurrentProcessId(),
             r[0], r[1], r[2], r[3]);
    a_file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW,
                         FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (a_file == INVALID_HANDLE_VALUE) return false;
    DWORD got = 0;
    DeviceIoControl(a_file, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &got, NULL);   /* parts never used take no disk */
#else
    if (d && *d) snprintf(dir, sizeof(dir), "%s", d);
    else {
        const char* t = getenv("TMPDIR");
        snprintf(dir, sizeof(dir), "%s", t && *t ? t : "/tmp");
    }
    char path[700];
    snprintf(path, sizeof(path), "%s/enlngdb-memory-XXXXXX", dir);
    a_fd = mkstemp(path);
    if (a_fd < 0) return false;
    unlink(path);           /* gone when the program ends */
#endif
    a_file_ready = 1;
    return true;
}

/* maps 'size' more bytes of the file (a multiple of 1 MB); NULL when the file cannot grow */
static char* a_map(size_t size, uint64_t* off_out) {
    if (!a_open_file()) return NULL;
    uint64_t off = a_file_end, end = a_file_end + size;
    char* base = NULL;
#ifdef _WIN32
    HANDLE map = CreateFileMappingA(a_file, NULL, PAGE_READWRITE, (DWORD)(end >> 32), (DWORD)(end & 0xFFFFFFFFu), NULL);
    if (!map) return NULL;
    base = (char*)MapViewOfFile(map, FILE_MAP_ALL_ACCESS, (DWORD)(off >> 32), (DWORD)(off & 0xFFFFFFFFu), size);
    CloseHandle(map);
    if (!base) return NULL;
#else
    if (ftruncate(a_fd, (off_t)end) != 0) return NULL;
    void* m = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, a_fd, (off_t)off);
    if (m == MAP_FAILED) return NULL;
    base = (char*)m;
#endif
    a_file_end = end;
    *off_out = off;
    return base;
}

static void a_unmap(char* base, size_t size, uint64_t off) {
#ifdef _WIN32
    UnmapViewOfFile(base);
    struct { LARGE_INTEGER from, to; } z;      /* FILE_ZERO_DATA_INFORMATION */
    z.from.QuadPart = (LONGLONG)off;
    z.to.QuadPart = (LONGLONG)(off + size);
    DWORD got = 0;
    DeviceIoControl(a_file, 0x000980C8u /* FSCTL_SET_ZERO_DATA */, &z, sizeof(z), NULL, 0, &got, NULL);   /* the disk is given back */
#else
    munmap(base, size);
#if defined(__linux__) && defined(FALLOC_FL_PUNCH_HOLE)
    fallocate(a_fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, (off_t)off, (off_t)size);
#else
    (void)off;
#endif
#endif
}

static bool a_add_chunk(char* base, size_t size, uint64_t off, int huge) {
    if (a_nchunks == a_capchunks) {
        int cap = a_capchunks ? a_capchunks * 2 : 64;
        AChunk* n = (AChunk*)realloc(a_chunks, sizeof(AChunk) * (size_t)cap);
        if (!n) return false;
        a_chunks = n;
        a_capchunks = cap;
    }
    int i = a_nchunks;
    while (i > 0 && a_chunks[i - 1].base > base) { a_chunks[i] = a_chunks[i - 1]; i--; }
    a_chunks[i].base = base;
    a_chunks[i].size = size;
    a_chunks[i].off = off;
    a_chunks[i].huge = huge;
    a_nchunks++;
    if (!a_lo || base < a_lo) a_lo = base;
    if (base + size > a_hi) a_hi = base + size;
    return true;
}

/* the chunk that holds p, or -1 (memory of the C library) */
static int a_chunk_of(const void* p) {
    const char* c = (const char*)p;
    if (!a_nchunks || c < a_lo || c >= a_hi) return -1;
    int lo = 0, hi = a_nchunks - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (c < a_chunks[mid].base) hi = mid - 1;
        else if (c >= a_chunks[mid].base + a_chunks[mid].size) lo = mid + 1;
        else return mid;
    }
    return -1;
}

static void a_note_spill(size_t n, int sign) {
    if (sign > 0) { a_spill += n; if (a_spill > a_spill_peak) a_spill_peak = a_spill; }
    else a_spill -= n;
}

/* a block of 'total' bytes (header included) cut from the current chunk */
static void* a_carve(size_t total) {
    if (!a_cur || (size_t)(a_end - a_cur) < total) {
        uint64_t off;
        char* base = a_map(A_CHUNK, &off);
        if (!base || !a_add_chunk(base, A_CHUNK, off, 0)) return NULL;
        a_cur = base;
        a_end = base + A_CHUNK;
    }
    void* b = a_cur;
    a_cur += total;
    return b;
}

static void* a_file_alloc(size_t n) {
    if (n > SIZE_MAX - 64) return NULL;
    size_t total = n + sizeof(AHdr);
    AHdr* h;
    if (total <= A_SMALL_MAX) {
        int c = a_cls_of[(total + 15) / 16];
        if (a_free_small[c]) { h = (AHdr*)a_free_small[c]; a_free_small[c] = *(void**)(h + 1); }
        else if (!(h = (AHdr*)a_carve(a_cls_size[c]))) return NULL;
        h->cls = (uint32_t)c;
        h->size = a_cls_size[c];
    } else if (total <= A_MID_MAX) {
        int k = 16;
        while (((size_t)1 << k) < total) k++;
        if (a_free_mid[k - 16]) { h = (AHdr*)a_free_mid[k - 16]; a_free_mid[k - 16] = *(void**)(h + 1); }
        else if (!(h = (AHdr*)a_carve((size_t)1 << k))) return NULL;
        h->cls = 0x100u + (uint32_t)k;
        h->size = (size_t)1 << k;
    } else {
        size_t size = (total + ((size_t)1 << 20) - 1) & ~(((size_t)1 << 20) - 1);
        uint64_t off;
        char* base = a_map(size, &off);
        if (!base) return NULL;
        if (!a_add_chunk(base, size, off, 1)) { a_unmap(base, size, off); return NULL; }
        h = (AHdr*)base;
        h->cls = A_HUGE;
        h->size = size;
    }
    h->magic = A_MAGIC;
    a_note_spill(h->size, 1);
    return h + 1;
}

static void a_file_free(void* p, int chunk) {
    AHdr* h = (AHdr*)p - 1;
    a_note_spill(h->size, -1);
    if (h->cls == A_HUGE) {
        AChunk ch = a_chunks[chunk];
        for (int i = chunk; i + 1 < a_nchunks; i++) a_chunks[i] = a_chunks[i + 1];
        a_nchunks--;
        a_unmap(ch.base, ch.size, ch.off);
        return;
    }
    if (h->cls >= 0x100u) {
        int k = (int)(h->cls - 0x100u);
        *(void**)(h + 1) = a_free_mid[k - 16];
        a_free_mid[k - 16] = h;
#if !defined(_WIN32) && defined(MADV_DONTNEED)
        /* a large free block need not be written to the file (its pages after the first are dropped) */
        size_t pg = 4096;
        char* from = (char*)h + pg;
        if (h->size > 2 * pg) madvise(from, h->size - pg, MADV_DONTNEED);
#endif
        return;
    }
    *(void**)(h + 1) = a_free_small[h->cls];
    a_free_small[h->cls] = h;
}

static size_t a_capacity(void* p) {
    AHdr* h = (AHdr*)p - 1;
    return (size_t)h->size - sizeof(AHdr);
}

/* no memory and no room in the file: say so and stop (the database file is left as it was) */
static void a_out_of_memory(size_t n) {
    fflush(stdout);
    fprintf(stderr, "\nERROR: out of memory: %.1f MB more were needed%s. The database file was not changed by this run.\n",
            n / 1048576.0, a_state == 2 ? ", and the file for memory could not grow (is the disk full? ENLNGDB_SPILL_DIR can name another folder)" : "");
    exit(3);
}

void* enlngdb_malloc(size_t n) {
    if (!a_state) a_start();
    if (n == 0) n = 1;
    if (a_state == 2 && a_heap + n > a_budget && a_file_ready >= 0) {
        void* p = a_file_alloc(n);
        if (p) return p;
    }
    void* p = malloc(n);
    if (!p && a_state == 2 && a_file_ready >= 0) p = a_file_alloc(n);     /* the system is out of memory first */
    if (!p) a_out_of_memory(n);
    else if (a_chunk_of(p) >= 0) return p;
    if (a_state == 2) {
        a_heap += A_LIBC_SIZE(p);
        if (a_heap > a_heap_peak) a_heap_peak = a_heap;
    }
    return p;
}

void* enlngdb_calloc(size_t n, size_t size) {
    if (size && n > SIZE_MAX / size) return NULL;
    size_t t = n * size;
    void* p = enlngdb_malloc(t);
    if (p) memset(p, 0, t ? t : 1);
    return p;
}

void enlngdb_mfree(void* p) {
    if (!p) return;
    int c = a_state == 2 ? a_chunk_of(p) : -1;
    if (c >= 0) { a_file_free(p, c); return; }
    if (a_state == 2) {
        size_t s = A_LIBC_SIZE(p);
        a_heap = a_heap >= s ? a_heap - s : 0;
    }
    free(p);
}

void* enlngdb_realloc(void* p, size_t n) {
    if (!p) return enlngdb_malloc(n);
    if (n == 0) n = 1;
    if (!a_state) a_start();
    int c = a_state == 2 ? a_chunk_of(p) : -1;
    if (c >= 0) {
        size_t cap = a_capacity(p);
        if (n <= cap) return p;
        void* q = enlngdb_malloc(n);
        if (!q) return NULL;
        memcpy(q, p, cap);
        a_file_free(p, a_chunk_of(p));
        return q;
    }
    if (a_state != 2) {
        void* q = realloc(p, n);
        if (!q) a_out_of_memory(n);
        return q;
    }
    size_t old = A_LIBC_SIZE(p);
    if (a_heap - old + n > a_budget && a_file_ready >= 0 && n > old) {
        void* q = a_file_alloc(n);
        if (q) {
            memcpy(q, p, old < n ? old : n);
            a_heap = a_heap >= old ? a_heap - old : 0;
            free(p);
            return q;
        }
    }
    void* q = realloc(p, n);
    if (!q) {
        q = a_file_ready >= 0 ? a_file_alloc(n) : NULL;
        if (!q) a_out_of_memory(n);
        memcpy(q, p, old < n ? old : n);
        a_heap = a_heap >= old ? a_heap - old : 0;
        free(p);
        return q;
    }
    a_heap = a_heap - old + A_LIBC_SIZE(q);
    if (a_heap > a_heap_peak) a_heap_peak = a_heap;
    return q;
}

char* enlngdb_strdup(const char* s) {
    size_t n = strlen(s) + 1;
    char* d = (char*)enlngdb_malloc(n);
    if (d) memcpy(d, s, n);
    return d;
}

void enlngdb_memory_stats(size_t* heap, size_t* heap_peak, size_t* spill, size_t* spill_peak) {
    if (heap) *heap = a_heap;
    if (heap_peak) *heap_peak = a_heap_peak;
    if (spill) *spill = a_spill;
    if (spill_peak) *spill_peak = a_spill_peak;
}
#endif
