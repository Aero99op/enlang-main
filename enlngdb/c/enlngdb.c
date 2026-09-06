#include "enlngdb.h"
#include <ctype.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
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

static EnlngVal enlng_copy_val(const EnlngVal* src) {
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

void enlngdb_free(EnlngDatabase* db) {
    if (!db) return;
    for (int t = 0; t < db->table_count; t++) {
        EnlngTable* tbl = &db->tables[t];
        for (size_t r = 0; r < tbl->row_count; r++) {
            for (int c = 0; c < tbl->rows[r].cell_count; c++) {
                enlng_free_val(&tbl->rows[r].cells[c]);
            }
            free(tbl->rows[r].cells);
        }
        free(tbl->rows);
    }
    free(db);
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

bool enlngdb_insert_row(EnlngTable* table, const EnlngVal* cells, int cell_count) {
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
    target_row->cells = (EnlngVal*)malloc(cell_count * sizeof(EnlngVal));

    for (int i = 0; i < cell_count; i++) {
        target_row->cells[i] = enlng_copy_val(&cells[i]);
    }

    table->row_count++;
    table->version++;
    return true;
}

static int compare_vals(const EnlngVal* a, const EnlngVal* b) {
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
#ifdef _WIN32
        return _stricmp(sa, sb);
#else
        return strcasecmp(sa, sb);
#endif
    }

    if (a->type == ENLNG_VAL_BOOL && b->type == ENLNG_VAL_BOOL) {
        return a->bool_val - b->bool_val;
    }

    return 0;
}

static bool eval_condition(const EnlngVal* val, EnlngOp op, const EnlngVal* target) {
    if (op == OP_NONE) return true;
    int cmp = compare_vals(val, target);
    switch (op) {
        case OP_EQ:  return cmp == 0;
        case OP_NEQ: return cmp != 0;
        case OP_GT:  return cmp > 0;
        case OP_LT:  return cmp < 0;
        case OP_GTE: return cmp >= 0;
        case OP_LTE: return cmp <= 0;
        case OP_LIKE: {
            if (val->type == ENLNG_VAL_STRING && target->type == ENLNG_VAL_STRING) {
                return strstr(val->str_val, target->str_val) != NULL;
            }
            return false;
        }
        default: return true;
    }
}

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

    int col_widths[ENLNGDB_MAX_COLS];
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

    printf("%zu row(s) in set (%.2f ms)\n\n", res->row_count, res->execution_time_ms);
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

/* Binary persistence: fast zero-copy disk serialization */
bool enlngdb_save(const EnlngDatabase* db, const char* filepath) {
    if (!db || !filepath) return false;
    FILE* f = fopen(filepath, "wb");
    if (!f) return false;

    // Header
    fwrite(ENLNGDB_MAGIC, 1, 16, f);
    fwrite(db->name, 1, ENLNGDB_MAX_NAME, f);
    fwrite(&db->table_count, sizeof(int), 1, f);

    for (int t = 0; t < db->table_count; t++) {
        const EnlngTable* tbl = &db->tables[t];
        fwrite(tbl->name, 1, ENLNGDB_MAX_NAME, f);
        fwrite(&tbl->col_count, sizeof(int), 1, f);
        fwrite(tbl->columns, sizeof(EnlngColumn), tbl->col_count, f);
        fwrite(&tbl->row_count, sizeof(size_t), 1, f);

        for (size_t r = 0; r < tbl->row_count; r++) {
            const EnlngRow* row = &tbl->rows[r];
            fwrite(&row->cell_count, sizeof(int), 1, f);
            for (int c = 0; c < row->cell_count; c++) {
                const EnlngVal* v = &row->cells[c];
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
        }
    }
    fclose(f);
    return true;
}

bool enlngdb_load(EnlngDatabase* db, const char* filepath) {
    if (!db || !filepath) return false;
    FILE* f = fopen(filepath, "rb");
    if (!f) return false;

    char magic[16];
    if (fread(magic, 1, 16, f) != 16 || memcmp(magic, ENLNGDB_MAGIC, 14) != 0) {
        fclose(f);
        return false;
    }

    fread(db->name, 1, ENLNGDB_MAX_NAME, f);
    int table_cnt = 0;
    fread(&table_cnt, sizeof(int), 1, f);

    for (int t = 0; t < table_cnt; t++) {
        char tbl_name[ENLNGDB_MAX_NAME];
        int col_cnt = 0;
        fread(tbl_name, 1, ENLNGDB_MAX_NAME, f);
        fread(&col_cnt, sizeof(int), 1, f);

        EnlngColumn cols[ENLNGDB_MAX_COLS];
        fread(cols, sizeof(EnlngColumn), col_cnt, f);

        EnlngTable* tbl = enlngdb_create_table(db, tbl_name, cols, col_cnt);
        size_t row_cnt = 0;
        fread(&row_cnt, sizeof(size_t), 1, f);

        for (size_t r = 0; r < row_cnt; r++) {
            int cell_cnt = 0;
            fread(&cell_cnt, sizeof(int), 1, f);
            EnlngVal cells[ENLNGDB_MAX_COLS];

            for (int c = 0; c < cell_cnt; c++) {
                uint8_t type_byte = 0;
                fread(&type_byte, 1, 1, f);
                cells[c].type = (EnlngValType)type_byte;

                if (cells[c].type == ENLNG_VAL_INT) {
                    fread(&cells[c].int_val, sizeof(int64_t), 1, f);
                } else if (cells[c].type == ENLNG_VAL_DOUBLE) {
                    fread(&cells[c].double_val, sizeof(double), 1, f);
                } else if (cells[c].type == ENLNG_VAL_BOOL) {
                    uint8_t b = 0;
                    fread(&b, 1, 1, f);
                    cells[c].bool_val = (b != 0);
                } else if (cells[c].type == ENLNG_VAL_STRING) {
                    uint32_t len = 0;
                    fread(&len, sizeof(uint32_t), 1, f);
                    char* s = (char*)malloc(len + 1);
                    if (len > 0) fread(s, 1, len, f);
                    s[len] = '\0';
                    cells[c].str_val = s;
                } else {
                    cells[c].int_val = 0;
                }
            }
            enlngdb_insert_row(tbl, cells, cell_cnt);
            for (int c = 0; c < cell_cnt; c++) {
                enlng_free_val(&cells[c]);
            }
        }
    }
    fclose(f);
    return true;
}
