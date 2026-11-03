#include "enlngdb.h"
#include <ctype.h>
#include <time.h>

static char* trim(char* s) {
    while (isspace((unsigned char)*s) || *s == ';') s++;
    if (*s == 0) return s;
    char* end = s + strlen(s) - 1;
    while (end > s && (isspace((unsigned char)*end) || *end == ';')) end--;
    end[1] = '\0';
    return s;
}

static bool str_starts_with_ci(const char* str, const char* prefix) {
    while (*prefix) {
        if (tolower((unsigned char)*str) != tolower((unsigned char)*prefix)) return false;
        str++;
        prefix++;
    }
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

static EnlngVal parse_val_token(const char* token) {
    char clean[256];
    strncpy(clean, token, sizeof(clean) - 1);
    clean[sizeof(clean) - 1] = '\0';
    char* t = trim(clean);

    // Strips enclosing quotes
    size_t len = strlen(t);
    if (len >= 2 && ((t[0] == '"' && t[len - 1] == '"') || (t[0] == '\'' && t[len - 1] == '\''))) {
        t[len - 1] = '\0';
        return enlng_str(t + 1);
    }

    if (strcmp(t, "true") == 0 || strcmp(t, "True") == 0) return enlng_bool(true);
    if (strcmp(t, "false") == 0 || strcmp(t, "False") == 0) return enlng_bool(false);
    if (strcmp(t, "null") == 0 || strcmp(t, "Null") == 0) return enlng_null();

    // Check if integer or double
    char* endptr = NULL;
    long long iv = strtoll(t, &endptr, 10);
    if (endptr && *endptr == '\0' && strlen(t) > 0) {
        return enlng_int(iv);
    }

    double dv = strtod(t, &endptr);
    if (endptr && *endptr == '\0' && strlen(t) > 0) {
        return enlng_double(dv);
    }

    return enlng_str(t);
}

static EnlngOp parse_op_phrase(const char* s, int* op_len) {
    struct {
        const char* phrase;
        EnlngOp op;
    } ops[] = {
        {"is greater than or equal to", OP_GTE},
        {"greater than or equal to",    OP_GTE},
        {"is at least",                 OP_GTE},
        {">=",                          OP_GTE},
        {"is less than or equal to",    OP_LTE},
        {"less than or equal to",       OP_LTE},
        {"is at most",                  OP_LTE},
        {"<=",                          OP_LTE},
        {"is greater than",             OP_GT},
        {"greater than",                OP_GT},
        {">",                           OP_GT},
        {"is less than",                OP_LT},
        {"less than",                   OP_LT},
        {"<",                           OP_LT},
        {"is not equal to",             OP_NEQ},
        {"not equal to",                OP_NEQ},
        {"is not",                      OP_NEQ},
        {"!=",                          OP_NEQ},
        {"is equal to",                 OP_EQ},
        {"equal to",                    OP_EQ},
        {"equals",                      OP_EQ},
        {"==",                          OP_EQ},
        {"=",                           OP_EQ},
        {"like",                        OP_LIKE},
        {"is",                          OP_EQ},
        {NULL,                          OP_NONE}
    };

    for (int i = 0; ops[i].phrase != NULL; i++) {
        size_t plen = strlen(ops[i].phrase);
        if (str_starts_with_ci(s, ops[i].phrase)) {
            // Check word boundary
            char next = s[plen];
            if (next == '\0' || isspace((unsigned char)next) || strchr("=<>!\"'", next)) {
                if (op_len) *op_len = (int)plen;
                return ops[i].op;
            }
        }
    }
    if (op_len) *op_len = 0;
    return OP_NONE;
}

/* "Table 'x' does not exist" with a way forward: what tables exist, a likely typo,
 * and how to create the table. */
static int edit_distance(const char* a, const char* b) {
    int la = (int)strlen(a), lb = (int)strlen(b);
    if (la > 63 || lb > 63) return 99;
    int prev[64], cur[64];
    for (int j = 0; j <= lb; j++) prev[j] = j;
    for (int i = 1; i <= la; i++) {
        cur[0] = i;
        for (int j = 1; j <= lb; j++) {
            int cost = tolower((unsigned char)a[i - 1]) == tolower((unsigned char)b[j - 1]) ? 0 : 1;
            int best = prev[j - 1] + cost;
            if (prev[j] + 1 < best) best = prev[j] + 1;
            if (cur[j - 1] + 1 < best) best = cur[j - 1] + 1;
            cur[j] = best;
        }
        for (int j = 0; j <= lb; j++) prev[j] = cur[j];
    }
    return prev[lb];
}

static void print_missing_table(const EnlngDatabase* db, const char* name) {
    printf("ERROR: Table '%s' does not exist in database '%s'.\n", name, db->name);
    if (db->table_count == 0) {
        printf("       This database has no tables yet. Create one with:\n");
        printf("         create table %s with id, name;\n", name);
        printf("       (or insert a record, which creates the table: insert into %s with id 1, name \"A\";)\n\n", name);
        return;
    }
    size_t nlen = strlen(name);
    for (int i = 0; i < db->table_count; i++) {
        const char* t = db->tables[i].name;
        size_t tlen = strlen(t);
        /* same name in another case, a missing/extra letter, or a swapped pair (studnet) */
        int d = edit_distance(t, name);
        bool close_match = str_ci_cmp(t, name) == 0 || (d <= 2 && tlen >= 4 && nlen >= 4);
        (void)tlen;
        if (close_match) {
            printf("       Did you mean '%s'?\n", t);
            break;
        }
    }
    printf("       Tables in '%s':", db->name);
    for (int i = 0; i < db->table_count; i++) printf("%s %s", i ? "," : "", db->tables[i].name);
    printf("\n\n");
}

/* ---- Queries -------------------------------------------------------------
 *   find|fetch|select|get [top N | first N] [distinct] <what> from <table>
 *       [inner join | left join | join <table> on <column> is <column>] ...
 *       [where <condition> [and|or <condition>] ...]     (and comes before or; ( ) groups)
 *       [group by <column>]
 *       [order by|sorted by <column> [ascending|descending|lowest first|highest first]]
 *       [limit N] [offset N]
 *   <what>: records, all records, *, or columns, and sum(c) avg(c) min(c) max(c) count(*)
 *   count records in <table> [where ...] [group by <column>]
 */

typedef enum { QT_WORD, QT_STR, QT_SYM, QT_END } QTokKind;
typedef struct {
    QTokKind kind;
    char text[256];
} QTok;

#define Q_MAX_TOKS 256
#define Q_MAX_COLS 64
#define Q_MAX_CONDS 64
#define Q_MAX_ITEMS ENLNGDB_MAX_COLS

typedef struct {
    QTok t[Q_MAX_TOKS + 1];
    int n, pos;
    char err[300];
} QLex;

static void q_lex(QLex* L, const char* s) {
    L->n = L->pos = 0;
    L->err[0] = '\0';
    while (*s && L->n < Q_MAX_TOKS) {
        if (isspace((unsigned char)*s) || *s == ';') { s++; continue; }
        QTok* t = &L->t[L->n++];
        size_t k = 0;
        if (*s == '"' || *s == '\'') {
            char q = *s;
            t->kind = QT_STR;
            t->text[k++] = *s++;
            while (*s && *s != q) { if (k < sizeof(t->text) - 2) t->text[k++] = *s; s++; }
            if (*s == q) s++;
            t->text[k++] = q;
        } else if (strchr("(),", *s)) {
            t->kind = QT_SYM;
            t->text[k++] = *s++;
        } else if (strchr("=<>!", *s)) {
            t->kind = QT_SYM;
            t->text[k++] = *s++;
            if (*s == '=') t->text[k++] = *s++;
        } else {
            t->kind = QT_WORD;
            while (*s && !isspace((unsigned char)*s) && !strchr("(),\"'=<>!;", *s)) {
                if (k < sizeof(t->text) - 1) t->text[k++] = *s;
                s++;
            }
        }
        t->text[k] = '\0';
    }
    L->t[L->n].kind = QT_END;
    L->t[L->n].text[0] = '\0';
}

static QTok* q_peek(QLex* L, int off) {
    int i = L->pos + off;
    return i < L->n ? &L->t[i] : &L->t[L->n];
}

/* the next token is the word (or symbol) w */
static bool q_is(QLex* L, int off, const char* w) {
    QTok* t = q_peek(L, off);
    return t->kind != QT_END && t->kind != QT_STR && str_ci_cmp(t->text, w) == 0;
}

static bool q_take(QLex* L, const char* w) {
    if (q_is(L, 0, w)) { L->pos++; return true; }
    return false;
}

/* a comparison phrase made of several tokens: "is at least", ">=", "is not" ... */
static EnlngOp q_take_op(QLex* L) {
    static const struct { const char* words; EnlngOp op; } ops[] = {
        {"is greater than or equal to", OP_GTE}, {"greater than or equal to", OP_GTE},
        {"is at least", OP_GTE}, {"at least", OP_GTE}, {">=", OP_GTE},
        {"is less than or equal to", OP_LTE}, {"less than or equal to", OP_LTE},
        {"is at most", OP_LTE}, {"at most", OP_LTE}, {"<=", OP_LTE},
        {"is greater than", OP_GT}, {"greater than", OP_GT}, {"is more than", OP_GT}, {"more than", OP_GT},
        {"is above", OP_GT}, {"above", OP_GT}, {">", OP_GT},
        {"is less than", OP_LT}, {"less than", OP_LT}, {"is under", OP_LT}, {"under", OP_LT},
        {"is below", OP_LT}, {"below", OP_LT}, {"<", OP_LT},
        {"is not equal to", OP_NEQ}, {"not equal to", OP_NEQ}, {"is not", OP_NEQ}, {"!=", OP_NEQ},
        {"is equal to", OP_EQ}, {"equal to", OP_EQ}, {"equals", OP_EQ}, {"==", OP_EQ}, {"=", OP_EQ},
        {"is like", OP_LIKE}, {"like", OP_LIKE}, {"is", OP_EQ},
        {NULL, OP_NONE}
    };
    for (int i = 0; ops[i].words; i++) {
        char buf[64];
        strncpy(buf, ops[i].words, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        int off = 0;
        bool ok = true;
        for (char* w = strtok(buf, " "); w; w = strtok(NULL, " ")) {
            if (!q_is(L, off, w)) { ok = false; break; }
            off++;
        }
        if (ok) { L->pos += off; return ops[i].op; }
    }
    return OP_NONE;
}

typedef struct {
    int kind;            /* 0 one comparison, 1 and, 2 or */
    int a, b;            /* for and / or */
    char col[128];
    EnlngOp op;
    EnlngVal val;
    char rword[128];     /* the right side as written, when it may be a column */
    int ci, rci;         /* column places, found before the rows are read */
} QCond;

typedef struct {
    QCond c[Q_MAX_CONDS];
    int n;
} QConds;

static int q_parse_or(QLex* L, QConds* P);

static int q_parse_atom(QLex* L, QConds* P) {
    if (q_take(L, "(")) {
        int r = q_parse_or(L, P);
        if (r < 0) return -1;
        if (!q_take(L, ")")) { snprintf(L->err, sizeof(L->err), "a '(' in the condition has no ')'"); return -1; }
        return r;
    }
    QTok* t = q_peek(L, 0);
    if (t->kind != QT_WORD) {
        snprintf(L->err, sizeof(L->err), "expected a column name in the condition, found '%s'", t->kind == QT_END ? "the end" : t->text);
        return -1;
    }
    if (P->n >= Q_MAX_CONDS) { snprintf(L->err, sizeof(L->err), "the condition is too long"); return -1; }
    QCond* c = &P->c[P->n];
    memset(c, 0, sizeof(*c));
    c->val = enlng_null();
    strncpy(c->col, t->text, sizeof(c->col) - 1);
    L->pos++;
    /* "marks of the student is 90": the words between the column and the comparison are skipped */
    if (q_take(L, "of")) {
        q_take(L, "the");
        if (q_peek(L, 0)->kind == QT_WORD) L->pos++;
    }
    c->op = q_take_op(L);
    if (c->op == OP_NONE) {
        snprintf(L->err, sizeof(L->err), "after '%s' expected a comparison such as 'is', 'is at least' or '>'", c->col);
        return -1;
    }
    QTok* v = q_peek(L, 0);
    if (v->kind == QT_END || v->kind == QT_SYM) {
        snprintf(L->err, sizeof(L->err), "the comparison on '%s' has no value", c->col);
        return -1;
    }
    c->val = parse_val_token(v->text);
    if (v->kind == QT_WORD) strncpy(c->rword, v->text, sizeof(c->rword) - 1);
    L->pos++;
    return P->n++;
}

static int q_join_conds(QLex* L, QConds* P, int kind, int a, int b) {
    if (P->n >= Q_MAX_CONDS) { snprintf(L->err, sizeof(L->err), "the condition is too long"); return -1; }
    QCond* c = &P->c[P->n];
    memset(c, 0, sizeof(*c));
    c->val = enlng_null();
    c->kind = kind;
    c->a = a;
    c->b = b;
    return P->n++;
}

static int q_parse_and(QLex* L, QConds* P) {
    int left = q_parse_atom(L, P);
    while (left >= 0 && q_take(L, "and")) {
        int right = q_parse_atom(L, P);
        if (right < 0) return -1;
        left = q_join_conds(L, P, 1, left, right);
    }
    return left;
}

static int q_parse_or(QLex* L, QConds* P) {
    int left = q_parse_and(L, P);
    while (left >= 0 && q_take(L, "or")) {
        int right = q_parse_and(L, P);
        if (right < 0) return -1;
        left = q_join_conds(L, P, 2, left, right);
    }
    return left;
}

/* rows being worked on: every column is named table.column */
typedef struct {
    int ncols;
    char names[Q_MAX_COLS][130];
    EnlngVal** rows;
    size_t nrows, cap;
} QSet;

static void q_set_add(QSet* S, EnlngVal* row) {
    if (S->nrows >= S->cap) {
        S->cap = S->cap ? S->cap * 2 : 16;
        S->rows = (EnlngVal**)realloc(S->rows, S->cap * sizeof(EnlngVal*));
    }
    S->rows[S->nrows++] = row;
}

static void q_set_free(QSet* S) {
    for (size_t r = 0; r < S->nrows; r++) {
        if (!S->rows[r]) continue;
        for (int c = 0; c < S->ncols; c++) enlng_free_val(&S->rows[r][c]);
        free(S->rows[r]);
    }
    free(S->rows);
    S->rows = NULL;
    S->nrows = S->cap = 0;
}

static const char* q_short(const char* name) {
    const char* dot = strchr(name, '.');
    return dot ? dot + 1 : name;
}

/* the place of a column: "marks" or "student.marks"; -1 if none, -2 if more than one table has it */
static int q_find_col(const QSet* S, const char* name) {
    for (int c = 0; c < S->ncols; c++) {
        if (str_ci_cmp(S->names[c], name) == 0) return c;
    }
    if (strchr(name, '.')) return -1;
    int found = -1;
    for (int c = 0; c < S->ncols; c++) {
        if (str_ci_cmp(q_short(S->names[c]), name) == 0) {
            if (found >= 0) return -2;
            found = c;
        }
    }
    return found;
}

static void q_col_error(const QSet* S, const char* name, int place, char* err, size_t n) {
    if (place == -2) {
        snprintf(err, n, "more than one table has a column '%s'; write it as table.%s", name, name);
        return;
    }
    /* one table: its columns by their short names */
    bool one_table = true;
    size_t plen = S->ncols ? (size_t)(q_short(S->names[0]) - S->names[0]) : 0;
    for (int c = 1; c < S->ncols; c++) one_table = one_table && strncmp(S->names[c], S->names[0], plen) == 0;
    size_t k = (size_t)snprintf(err, n, "there is no column '%s'. The columns are:", name);
    for (int c = 0; c < S->ncols && k < n; c++) k += (size_t)snprintf(err + k, n - k, " %s", one_table ? q_short(S->names[c]) : S->names[c]);
}

/* find each column a condition names; a word on the right that is a column is compared as one */
static bool q_bind(QConds* P, int i, const QSet* S, bool right_may_be_column, char* err, size_t n) {
    QCond* c = &P->c[i];
    if (c->kind != 0) return q_bind(P, c->a, S, right_may_be_column, err, n) && q_bind(P, c->b, S, right_may_be_column, err, n);
    c->ci = q_find_col(S, c->col);
    if (c->ci < 0) { q_col_error(S, c->col, c->ci, err, n); return false; }
    c->rci = -1;
    if (c->rword[0] && (right_may_be_column || strchr(c->rword, '.'))) {
        int r = q_find_col(S, c->rword);
        if (r >= 0) c->rci = r;
    }
    return true;
}

static bool q_eval(const QConds* P, int i, EnlngVal* row) {
    const QCond* c = &P->c[i];
    if (c->kind == 1) return q_eval(P, c->a, row) && q_eval(P, c->b, row);
    if (c->kind == 2) return q_eval(P, c->a, row) || q_eval(P, c->b, row);
    const EnlngVal* target = c->rci >= 0 ? &row[c->rci] : &c->val;
    return enlng_eval_condition(&row[c->ci], c->op, target);
}

static void q_free_conds(QConds* P) {
    for (int i = 0; i < P->n; i++) enlng_free_val(&P->c[i].val);
    P->n = 0;
}

static bool q_load_table(QSet* S, const EnlngTable* tbl) {
    if (S->ncols + tbl->col_count > Q_MAX_COLS) return false;
    for (int c = 0; c < tbl->col_count; c++) snprintf(S->names[S->ncols + c], sizeof(S->names[0]), "%s.%s", tbl->name, tbl->columns[c].name);
    S->ncols += tbl->col_count;
    for (size_t r = 0; r < tbl->row_count; r++) {
        EnlngVal* row = (EnlngVal*)malloc(S->ncols * sizeof(EnlngVal));
        for (int c = 0; c < tbl->col_count; c++) {
            row[c] = c < tbl->rows[r].cell_count ? enlng_copy_val(&tbl->rows[r].cells[c]) : enlng_null();
        }
        q_set_add(S, row);
    }
    return true;
}

typedef enum { QA_NONE, QA_COUNT, QA_SUM, QA_AVG, QA_MIN, QA_MAX } QAgg;

typedef struct {
    QAgg agg;
    char col[128];       /* "" for count(*) */
    char title[64];
    int ci;
} QItem;

static QAgg q_agg_word(const char* w) {
    if (str_ci_cmp(w, "count") == 0) return QA_COUNT;
    if (str_ci_cmp(w, "sum") == 0 || str_ci_cmp(w, "total") == 0) return QA_SUM;
    if (str_ci_cmp(w, "avg") == 0 || str_ci_cmp(w, "average") == 0) return QA_AVG;
    if (str_ci_cmp(w, "min") == 0 || str_ci_cmp(w, "lowest") == 0) return QA_MIN;
    if (str_ci_cmp(w, "max") == 0 || str_ci_cmp(w, "highest") == 0) return QA_MAX;
    return QA_NONE;
}

static EnlngVal q_aggregate(const QItem* it, EnlngVal** rows, size_t n) {
    if (it->agg == QA_COUNT) {
        int64_t k = 0;
        for (size_t r = 0; r < n; r++) {
            if (it->ci < 0 || rows[r][it->ci].type != ENLNG_VAL_NULL) k++;
        }
        return enlng_int(k);
    }
    if (it->agg == QA_MIN || it->agg == QA_MAX) {
        const EnlngVal* best = NULL;
        for (size_t r = 0; r < n; r++) {
            const EnlngVal* v = &rows[r][it->ci];
            if (v->type == ENLNG_VAL_NULL) continue;
            int cmp = best ? enlng_compare_vals(v, best) : 0;
            if (!best || (it->agg == QA_MIN ? cmp < 0 : cmp > 0)) best = v;
        }
        return best ? enlng_copy_val(best) : enlng_null();
    }
    /* sum / average of the numbers */
    int64_t isum = 0;
    double dsum = 0;
    bool any_double = false;
    size_t k = 0;
    for (size_t r = 0; r < n; r++) {
        const EnlngVal* v = &rows[r][it->ci];
        if (v->type == ENLNG_VAL_INT) { isum += v->int_val; dsum += (double)v->int_val; k++; }
        else if (v->type == ENLNG_VAL_DOUBLE) { dsum += v->double_val; any_double = true; k++; }
    }
    if (k == 0) return enlng_null();
    if (it->agg == QA_AVG) return enlng_double(dsum / (double)k);
    return any_double ? enlng_double(dsum) : enlng_int(isum);
}

/* keeps rows in the order they had when two are equal */
static void q_sort(EnlngVal** rows, size_t n, int ci, bool desc) {
    for (size_t i = 1; i < n; i++) {
        EnlngVal* cur = rows[i];
        size_t j = i;
        while (j > 0) {
            int cmp = enlng_compare_vals(&rows[j - 1][ci], &cur[ci]);
            if (desc ? cmp >= 0 : cmp <= 0) break;
            rows[j] = rows[j - 1];
            j--;
        }
        rows[j] = cur;
    }
}

static bool q_rows_same(const EnlngVal* a, const EnlngVal* b, int n) {
    for (int c = 0; c < n; c++) {
        if (a[c].type != b[c].type || enlng_compare_vals(&a[c], &b[c]) != 0) return false;
        if (a[c].type == ENLNG_VAL_STRING && strcmp(a[c].str_val ? a[c].str_val : "", b[c].str_val ? b[c].str_val : "") != 0) return false;
    }
    return true;
}

static bool q_fail(QLex* L, bool print_output) {
    if (print_output) printf("ERROR: %s.\n\n", L->err);
    return false;
}

static bool run_query(EnlngDatabase* db, const char* stmt, bool print_output) {
    clock_t t0 = clock();
    static QLex L;
    q_lex(&L, stmt);

    bool count_style = false;
    if (q_take(&L, "count")) {
        count_style = true;
    } else if (q_take(&L, "show")) {
        if (!q_take(&L, "all")) { snprintf(L.err, sizeof(L.err), "expected 'show all records from <table>'"); return q_fail(&L, print_output); }
    } else if (!(q_take(&L, "find") || q_take(&L, "fetch") || q_take(&L, "select") || q_take(&L, "get"))) {
        return false;
    }

    long limit = -1, offset = 0;
    bool distinct = false;
    for (;;) {
        if ((q_is(&L, 0, "top") || q_is(&L, 0, "first")) && q_peek(&L, 1)->kind == QT_WORD && isdigit((unsigned char)q_peek(&L, 1)->text[0])) {
            limit = atol(q_peek(&L, 1)->text);
            L.pos += 2;
        } else if (q_take(&L, "distinct") || q_take(&L, "different") || q_take(&L, "unique")) {
            distinct = true;
        } else {
            break;
        }
    }

    /* what to show */
    QItem items[Q_MAX_ITEMS];
    int nitems = 0;
    bool all_cols = false;
    char tbl_name[64] = {0};
    if (count_style) {
        /* count records in t / count all from t / count employees from t */
        while (!q_is(&L, 0, "from") && !q_is(&L, 0, "in") && q_peek(&L, 0)->kind != QT_END) L.pos++;
        memset(&items[0], 0, sizeof(items[0]));
        items[0].agg = QA_COUNT;
        strcpy(items[0].title, "count(*)");
        nitems = 1;
    } else if (q_is(&L, 0, "from") || q_is(&L, 0, "in")) {
        all_cols = true;
    } else {
        /* "all the records", "every record", "*": only such words before 'from' mean every column */
        static const char* every[] = {"all", "the", "*", "records", "record", "rows", "row", "every", "of", NULL};
        int k = 0;
        for (;; k++) {
            bool word = false;
            for (int w = 0; every[w]; w++) word = word || q_is(&L, k, every[w]);
            if (!word) break;
        }
        if (k > 0 && (q_is(&L, k, "from") || q_is(&L, k, "in"))) {
            L.pos += k;
            all_cols = true;
        }
    }
    if (!count_style && !all_cols && !q_is(&L, 0, "from") && !q_is(&L, 0, "in")) {
        for (;;) {
            QTok* t = q_peek(&L, 0);
            if (t->kind != QT_WORD) { snprintf(L.err, sizeof(L.err), "expected a column name, found '%s'", t->kind == QT_END ? "the end" : t->text); return q_fail(&L, print_output); }
            if (nitems >= Q_MAX_ITEMS) { snprintf(L.err, sizeof(L.err), "at most %d columns can be shown", Q_MAX_ITEMS); return q_fail(&L, print_output); }
            QItem* it = &items[nitems];
            memset(it, 0, sizeof(*it));
            QAgg agg = q_agg_word(t->text);
            if (agg != QA_NONE && q_is(&L, 1, "(")) {
                char fname[32];
                snprintf(fname, sizeof(fname), "%s", t->text);
                L.pos += 2;
                QTok* a = q_peek(&L, 0);
                if (a->kind != QT_WORD) { snprintf(L.err, sizeof(L.err), "%s( ) needs a column name", fname); return q_fail(&L, print_output); }
                if (strcmp(a->text, "*") != 0) strncpy(it->col, a->text, sizeof(it->col) - 1);
                else if (agg != QA_COUNT) { snprintf(L.err, sizeof(L.err), "%s(*) is not possible; name a column, like %s(marks)", fname, fname); return q_fail(&L, print_output); }
                L.pos++;
                if (!q_take(&L, ")")) { snprintf(L.err, sizeof(L.err), "%s( has no ')'", fname); return q_fail(&L, print_output); }
                it->agg = agg;
                snprintf(it->title, sizeof(it->title), "%s(%s)", fname, it->col[0] ? it->col : "*");
            } else {
                strncpy(it->col, t->text, sizeof(it->col) - 1);
                L.pos++;
            }
            nitems++;
            if (!(q_take(&L, ",") || q_take(&L, "and"))) break;
        }
        /* find users where ...: one word and no 'from' is the table */
        if (!q_is(&L, 0, "from") && !q_is(&L, 0, "in") && nitems == 1 && items[0].agg == QA_NONE) {
            strncpy(tbl_name, items[0].col, sizeof(tbl_name) - 1);
            nitems = 0;
            all_cols = true;
        }
    }
    if (!tbl_name[0]) {
        if (!(q_take(&L, "from") || q_take(&L, "in"))) { snprintf(L.err, sizeof(L.err), "expected 'from <table>'"); return q_fail(&L, print_output); }
        QTok* t = q_peek(&L, 0);
        if (t->kind != QT_WORD && t->kind != QT_STR) { snprintf(L.err, sizeof(L.err), "expected a table name after 'from'"); return q_fail(&L, print_output); }
        EnlngVal nv = parse_val_token(t->text);
        snprintf(tbl_name, sizeof(tbl_name), "%s", nv.type == ENLNG_VAL_STRING && nv.str_val ? nv.str_val : t->text);
        enlng_free_val(&nv);
        L.pos++;
    }

    EnlngTable* tbl = enlngdb_get_table(db, tbl_name);
    if (!tbl) {
        if (print_output) print_missing_table(db, tbl_name);
        return false;
    }

    static QSet S;
    memset(&S, 0, sizeof(S));
    static QConds P;
    P.n = 0;
    int where_root = -1;
    char group_col[128] = {0}, order_col[128] = {0};
    bool desc = false;
    bool ok = false;
    char err[300] = {0};
    EnlngQueryResult* res = NULL;

    int njoins = 0;
    q_load_table(&S, tbl);

    /* joins */
    while (q_is(&L, 0, "join") || q_is(&L, 0, "inner") || q_is(&L, 0, "left")) {
        bool left = q_take(&L, "left");
        q_take(&L, "inner");
        q_take(&L, "outer");
        if (!q_take(&L, "join")) { snprintf(err, sizeof(err), "expected 'join <table> on ...'"); goto done; }
        QTok* t = q_peek(&L, 0);
        EnlngTable* t2 = t->kind == QT_WORD ? enlngdb_get_table(db, t->text) : NULL;
        if (!t2) {
            if (print_output) print_missing_table(db, t->text);
            goto done_quiet;
        }
        L.pos++;
        if (!q_take(&L, "on")) { snprintf(err, sizeof(err), "a join needs 'on', like: join %s on %s.id is %s.%s_id", t2->name, tbl->name, t2->name, tbl->name); goto done; }
        int on_root = q_parse_or(&L, &P);
        if (on_root < 0) { snprintf(err, sizeof(err), "%s", L.err); goto done; }

        static QSet B, J;
        memset(&B, 0, sizeof(B));
        q_load_table(&B, t2);
        memset(&J, 0, sizeof(J));
        J.ncols = S.ncols + B.ncols;
        if (J.ncols > Q_MAX_COLS) { q_set_free(&B); snprintf(err, sizeof(err), "the joined tables have more than %d columns", Q_MAX_COLS); goto done; }
        for (int c = 0; c < S.ncols; c++) strcpy(J.names[c], S.names[c]);
        for (int c = 0; c < B.ncols; c++) strcpy(J.names[S.ncols + c], B.names[c]);
        if (!q_bind(&P, on_root, &J, true, err, sizeof(err))) { q_set_free(&B); goto done; }

        EnlngVal* both = (EnlngVal*)malloc(J.ncols * sizeof(EnlngVal));
        for (size_t a = 0; a < S.nrows; a++) {
            bool matched = false;
            for (size_t b = 0; b < B.nrows; b++) {
                memcpy(both, S.rows[a], S.ncols * sizeof(EnlngVal));
                memcpy(both + S.ncols, B.rows[b], B.ncols * sizeof(EnlngVal));
                if (!q_eval(&P, on_root, both)) continue;
                matched = true;
                EnlngVal* row = (EnlngVal*)malloc(J.ncols * sizeof(EnlngVal));
                for (int c = 0; c < J.ncols; c++) row[c] = enlng_copy_val(&both[c]);
                q_set_add(&J, row);
            }
            if (!matched && left) {
                EnlngVal* row = (EnlngVal*)malloc(J.ncols * sizeof(EnlngVal));
                for (int c = 0; c < S.ncols; c++) row[c] = enlng_copy_val(&S.rows[a][c]);
                for (int c = S.ncols; c < J.ncols; c++) row[c] = enlng_null();
                q_set_add(&J, row);
            }
        }
        free(both);
        q_set_free(&B);
        q_set_free(&S);
        S = J;
        njoins++;
    }

    /* where, group by, order by, limit, offset: in any order */
    while (q_peek(&L, 0)->kind != QT_END) {
        if (q_take(&L, "where")) {
            if (where_root >= 0) { snprintf(err, sizeof(err), "only one 'where' is possible; join conditions with 'and' / 'or'"); goto done; }
            where_root = q_parse_or(&L, &P);
            if (where_root < 0) { snprintf(err, sizeof(err), "%s", L.err); goto done; }
        } else if (q_is(&L, 0, "group") && q_is(&L, 1, "by")) {
            L.pos += 2;
            if (q_peek(&L, 0)->kind != QT_WORD) { snprintf(err, sizeof(err), "'group by' needs a column name"); goto done; }
            strncpy(group_col, q_peek(&L, 0)->text, sizeof(group_col) - 1);
            L.pos++;
        } else if ((q_is(&L, 0, "order") || q_is(&L, 0, "sorted") || q_is(&L, 0, "sort")) && q_is(&L, 1, "by")) {
            L.pos += 2;
            QTok* t = q_peek(&L, 0);
            if (t->kind != QT_WORD) { snprintf(err, sizeof(err), "'order by' needs a column name"); goto done; }
            strncpy(order_col, t->text, sizeof(order_col) - 1);
            L.pos++;
            /* order by sum(marks) */
            if (q_agg_word(order_col) != QA_NONE && q_is(&L, 0, "(") ) {
                char inner[128] = {0};
                L.pos++;
                if (q_peek(&L, 0)->kind == QT_WORD) { strncpy(inner, q_peek(&L, 0)->text, sizeof(inner) - 1); L.pos++; }
                q_take(&L, ")");
                char whole[128];
                snprintf(whole, sizeof(whole), "%s(%s)", order_col, inner);
                strcpy(order_col, whole);
            }
            if (q_take(&L, "desc") || q_take(&L, "descending")) desc = true;
            else if (q_take(&L, "asc") || q_take(&L, "ascending")) desc = false;
            else if (q_is(&L, 0, "highest") || q_is(&L, 0, "largest") || q_is(&L, 0, "biggest")) { L.pos++; q_take(&L, "first"); desc = true; }
            else if (q_is(&L, 0, "lowest") || q_is(&L, 0, "smallest")) { L.pos++; q_take(&L, "first"); desc = false; }
        } else if (q_take(&L, "limit")) {
            QTok* t = q_peek(&L, 0);
            if (t->kind != QT_WORD || !isdigit((unsigned char)t->text[0])) { snprintf(err, sizeof(err), "'limit' needs a number"); goto done; }
            limit = atol(t->text);
            L.pos++;
        } else if (q_take(&L, "offset") || q_take(&L, "skip")) {
            QTok* t = q_peek(&L, 0);
            if (t->kind != QT_WORD || !isdigit((unsigned char)t->text[0])) { snprintf(err, sizeof(err), "'offset' needs a number"); goto done; }
            offset = atol(t->text);
            L.pos++;
        } else {
            snprintf(err, sizeof(err), "I don't understand '%s' here. After the table can come: join, where, group by, order by, limit, offset", q_peek(&L, 0)->text);
            goto done;
        }
    }

    if (where_root >= 0) {
        if (!q_bind(&P, where_root, &S, false, err, sizeof(err))) goto done;
        size_t keep = 0;
        for (size_t r = 0; r < S.nrows; r++) {
            if (q_eval(&P, where_root, S.rows[r])) {
                S.rows[keep++] = S.rows[r];
            } else {
                for (int c = 0; c < S.ncols; c++) enlng_free_val(&S.rows[r][c]);
                free(S.rows[r]);
            }
        }
        S.nrows = keep;
    }

    bool any_agg = false;
    for (int i = 0; i < nitems; i++) {
        if (items[i].agg != QA_NONE) any_agg = true;
    }
    bool joined = njoins > 0;
    if (count_style && group_col[0]) {
        items[1] = items[0];
        memset(&items[0], 0, sizeof(items[0]));
        strncpy(items[0].col, group_col, sizeof(items[0].col) - 1);
        nitems = 2;
    }

    /* find the columns the items name */
    if (all_cols && group_col[0]) {
        memset(&items[0], 0, sizeof(items[0]));
        strncpy(items[0].col, group_col, sizeof(items[0].col) - 1);
        memset(&items[1], 0, sizeof(items[1]));
        items[1].agg = QA_COUNT;
        strcpy(items[1].title, "count(*)");
        nitems = 2;
        all_cols = false;
        any_agg = true;
    }
    if (all_cols) {
        nitems = 0;
        for (int c = 0; c < S.ncols && nitems < Q_MAX_ITEMS; c++, nitems++) {
            memset(&items[nitems], 0, sizeof(items[0]));
            items[nitems].ci = c;
            snprintf(items[nitems].title, sizeof(items[0].title), "%s", joined ? S.names[c] : q_short(S.names[c]));
        }
        if (S.ncols > Q_MAX_ITEMS) { snprintf(err, sizeof(err), "the result has more than %d columns; name the ones to show", Q_MAX_ITEMS); goto done; }
    } else {
        for (int i = 0; i < nitems; i++) {
            if (items[i].col[0] == '\0') { items[i].ci = -1; continue; }
            items[i].ci = q_find_col(&S, items[i].col);
            if (items[i].ci < 0) { q_col_error(&S, items[i].col, items[i].ci, err, sizeof(err)); goto done; }
            if (items[i].agg == QA_NONE) snprintf(items[i].title, sizeof(items[0].title), "%s", items[i].col);
        }
    }

    /* the rows to show */
    static QSet O;
    memset(&O, 0, sizeof(O));
    O.ncols = nitems;
    for (int i = 0; i < nitems; i++) snprintf(O.names[i], sizeof(O.names[0]), "%s", items[i].title);

    if (any_agg || group_col[0]) {
        int gi = -1;
        if (group_col[0]) {
            gi = q_find_col(&S, group_col);
            if (gi < 0) { q_col_error(&S, group_col, gi, err, sizeof(err)); goto done; }
        }
        size_t ngroups = 0;
        size_t* first = (size_t*)malloc((S.nrows + 1) * sizeof(size_t));
        int* group_of = (int*)malloc((S.nrows + 1) * sizeof(int));
        for (size_t r = 0; r < S.nrows; r++) {
            int g = -1;
            if (gi >= 0) {
                for (size_t k = 0; k < ngroups; k++) {
                    if (q_rows_same(&S.rows[first[k]][gi], &S.rows[r][gi], 1)) { g = (int)k; break; }
                }
            } else if (ngroups > 0) {
                g = 0;
            }
            if (g < 0) { first[ngroups] = r; g = (int)ngroups++; }
            group_of[r] = g;
        }
        if (gi < 0 && ngroups == 0) ngroups = 1; /* sum of no rows is still one answer */
        EnlngVal** members = (EnlngVal**)malloc((S.nrows + 1) * sizeof(EnlngVal*));
        for (size_t g = 0; g < ngroups; g++) {
            size_t m = 0;
            for (size_t r = 0; r < S.nrows; r++) {
                if (group_of[r] == (int)g) members[m++] = S.rows[r];
            }
            EnlngVal* row = (EnlngVal*)malloc((nitems ? nitems : 1) * sizeof(EnlngVal));
            for (int i = 0; i < nitems; i++) {
                if (items[i].agg != QA_NONE) row[i] = q_aggregate(&items[i], members, m);
                else row[i] = m > 0 ? enlng_copy_val(&members[0][items[i].ci]) : enlng_null();
            }
            q_set_add(&O, row);
        }
        free(members);
        free(first);
        free(group_of);
        if (order_col[0]) {
            int oi = q_find_col(&O, order_col);
            if (oi < 0) { q_set_free(&O); q_col_error(&O, order_col, oi, err, sizeof(err)); goto done; }
            q_sort(O.rows, O.nrows, oi, desc);
        }
    } else {
        if (order_col[0]) {
            int oi = q_find_col(&S, order_col);
            if (oi < 0) { q_col_error(&S, order_col, oi, err, sizeof(err)); goto done; }
            q_sort(S.rows, S.nrows, oi, desc);
        }
        for (size_t r = 0; r < S.nrows; r++) {
            EnlngVal* row = (EnlngVal*)malloc((nitems ? nitems : 1) * sizeof(EnlngVal));
            for (int i = 0; i < nitems; i++) row[i] = enlng_copy_val(&S.rows[r][items[i].ci]);
            q_set_add(&O, row);
        }
    }

    if (distinct) {
        size_t keep = 0;
        for (size_t r = 0; r < O.nrows; r++) {
            bool seen = false;
            for (size_t k = 0; k < keep && !seen; k++) seen = q_rows_same(O.rows[k], O.rows[r], O.ncols);
            if (seen) {
                for (int c = 0; c < O.ncols; c++) enlng_free_val(&O.rows[r][c]);
                free(O.rows[r]);
            } else {
                O.rows[keep++] = O.rows[r];
            }
        }
        O.nrows = keep;
    }

    /* the old count box, for a plain count */
    if (count_style && !group_col[0]) {
        long long k = O.nrows ? (long long)O.rows[0][0].int_val : 0;
        if (print_output) {
            printf("+----------+\n");
            printf("| count(*) |\n");
            printf("+----------+\n");
            printf("| %-8lld |\n", k);
            printf("+----------+\n");
            printf("1 row in set (0.01 ms)\n\n");
        }
        q_set_free(&O);
        ok = true;
        goto done;
    }

    res = (EnlngQueryResult*)calloc(1, sizeof(EnlngQueryResult));
    res->col_count = O.ncols;
    for (int c = 0; c < O.ncols; c++) {
        snprintf(res->columns[c].name, sizeof(res->columns[c].name), "%s", O.names[c]);
    }
    size_t from_row = (size_t)offset < O.nrows ? (size_t)offset : O.nrows;
    size_t to_row = O.nrows;
    if (limit >= 0 && from_row + (size_t)limit < to_row) to_row = from_row + (size_t)limit;
    res->rows = (EnlngRow*)calloc((to_row - from_row) + 1, sizeof(EnlngRow));
    for (size_t r = from_row; r < to_row; r++) {
        EnlngRow* dest = &res->rows[res->row_count++];
        dest->cell_count = O.ncols;
        dest->cells = O.rows[r];
        O.rows[r] = NULL;
    }
    q_set_free(&O);
    res->execution_time_ms = (double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC;
    if (print_output) enlngdb_print_result(res);
    enlngdb_free_result(res);
    ok = true;

done:
    if (!ok && err[0] && print_output) printf("ERROR: %s.\n\n", err);
done_quiet:
    q_set_free(&S);
    q_free_conds(&P);
    return ok;
}

bool enlngdb_execute_statement(EnlngDatabase* db, const char* statement, bool print_output) {
    if (!db || !statement) return false;
    char line[1024];
    strncpy(line, statement, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';

    char* stmt = trim(line);
    // Remove trailing semicolon
    size_t slen = strlen(stmt);
    while (slen > 0 && stmt[slen - 1] == ';') {
        stmt[--slen] = '\0';
    }
    stmt = trim(stmt);
    if (strlen(stmt) == 0) return true;

    // 0. type enlngdb / type enlgdb
    if (str_starts_with_ci(stmt, "type enlngdb") || str_starts_with_ci(stmt, "type enlgdb")) {
        return true;
    }

    // display / print
    if (str_starts_with_ci(stmt, "display ") || str_starts_with_ci(stmt, "print ")) {
        const char* p = stmt + (str_starts_with_ci(stmt, "display ") ? 8 : 6);
        while (isspace((unsigned char)*p)) p++;
        char clean_msg[1024];
        strncpy(clean_msg, p, sizeof(clean_msg) - 1);
        clean_msg[sizeof(clean_msg) - 1] = '\0';
        char* msg = trim(clean_msg);
        size_t mlen = strlen(msg);
        if (mlen >= 2 && ((msg[0] == '"' && msg[mlen - 1] == '"') || (msg[0] == '\'' && msg[mlen - 1] == '\''))) {
            msg[mlen - 1] = '\0';
            msg++;
        }
        if (print_output) {
            printf("%s\n", msg);
        }
        return true;
    }

    // hint
    if (str_starts_with_ci(stmt, "hint ") || str_starts_with_ci(stmt, "hint:")) {
        return true;
    }

    // save database to "<filename>"
    if (str_starts_with_ci(stmt, "save database to ") || str_starts_with_ci(stmt, "save database ")) {
        const char* p = stmt + (str_starts_with_ci(stmt, "save database to ") ? 17 : 14);
        while (isspace((unsigned char)*p)) p++;
        char path_buf[256];
        strncpy(path_buf, p, sizeof(path_buf) - 1);
        path_buf[sizeof(path_buf) - 1] = '\0';
        char* path = trim(path_buf);
        size_t plen = strlen(path);
        if (plen >= 2 && ((path[0] == '"' && path[plen - 1] == '"') || (path[0] == '\'' && path[plen - 1] == '\''))) {
            path[plen - 1] = '\0';
            path++;
        }
        bool ok = enlngdb_save(db, path);
        if (ok && print_output) {
            printf("Database saved to '%s'. (0.02 ms)\n\n", path);
        }
        return ok;
    }

    // open database to "<filename>" / load database from "<filename>"
    if (str_starts_with_ci(stmt, "open database to ") || str_starts_with_ci(stmt, "open database ") || str_starts_with_ci(stmt, "load database from ")) {
        const char* p = stmt;
        if (str_starts_with_ci(p, "open database to ")) p += 17;
        else if (str_starts_with_ci(p, "open database ")) p += 14;
        else if (str_starts_with_ci(p, "load database from ")) p += 19;
        while (isspace((unsigned char)*p)) p++;
        char path_buf[256];
        strncpy(path_buf, p, sizeof(path_buf) - 1);
        path_buf[sizeof(path_buf) - 1] = '\0';
        char* path = trim(path_buf);
        size_t plen = strlen(path);
        if (plen >= 2 && ((path[0] == '"' && path[plen - 1] == '"') || (path[0] == '\'' && path[plen - 1] == '\''))) {
            path[plen - 1] = '\0';
            path++;
        }
        bool ok = enlngdb_load(db, path);
        if (ok && print_output) {
            printf("Database loaded from '%s'. (0.02 ms)\n\n", path);
        }
        return ok;
    }

    // 1. show tables
    if (str_starts_with_ci(stmt, "show tables")) {
        if (print_output) enlngdb_print_tables(db);
        return true;
    }

    // 2. show databases
    if (str_starts_with_ci(stmt, "show databases")) {
        if (print_output) {
            printf("+---------------------------+\n");
            printf("| Database                  |\n");
            printf("+---------------------------+\n");
            printf("| %-25s |\n", db->name);
            printf("+---------------------------+\n");
            printf("1 row(s) in set (0.01 ms)\n\n");
        }
        return true;
    }

    // 3. use database <name> / use <name>
    if (str_starts_with_ci(stmt, "use database ")) {
        char dbname[64];
        if (sscanf(stmt + 13, "%63[^; \t\r\n]", dbname) == 1) {
            char* sc = strchr(dbname, ';');
            if (sc) *sc = '\0';
            trim(dbname);
            strncpy(db->name, dbname, sizeof(db->name) - 1);
            char edb_file[256];
            snprintf(edb_file, sizeof(edb_file), "%s.edb", dbname);
            FILE* testf = fopen(edb_file, "rb");
            if (!testf) {
                snprintf(edb_file, sizeof(edb_file), "tests/%s.edb", dbname);
                testf = fopen(edb_file, "rb");
            }
            if (testf) {
                fclose(testf);
                enlngdb_load(db, edb_file);
            }
            if (print_output) {
                printf("Database changed to '%s'. (0.01 ms)\n\n", db->name);
            }
            return true;
        }
    } else if (str_starts_with_ci(stmt, "use ")) {
        char dbname[64];
        if (sscanf(stmt + 4, "%63[^; \t\r\n]", dbname) == 1) {
            char* sc = strchr(dbname, ';');
            if (sc) *sc = '\0';
            trim(dbname);
            strncpy(db->name, dbname, sizeof(db->name) - 1);
            char edb_file[256];
            snprintf(edb_file, sizeof(edb_file), "%s.edb", dbname);
            FILE* testf = fopen(edb_file, "rb");
            if (!testf) {
                snprintf(edb_file, sizeof(edb_file), "tests/%s.edb", dbname);
                testf = fopen(edb_file, "rb");
            }
            if (testf) {
                fclose(testf);
                enlngdb_load(db, edb_file);
            }
            if (print_output) {
                printf("Database changed to '%s'. (0.01 ms)\n\n", db->name);
            }
            return true;
        }
    }

    // 4. create table [if not exists] <name> with <col1>, <col2>...
    if (str_starts_with_ci(stmt, "create table ")) {
        const char* p = stmt + 13;
        while (isspace((unsigned char)*p)) p++;
        bool if_not_exists = false;
        if (str_starts_with_ci(p, "if not exists ")) {
            p += 14;
            if_not_exists = true;
        } else if (str_starts_with_ci(p, "if not exist ")) {
            p += 13;
            if_not_exists = true;
        }
        while (isspace((unsigned char)*p)) p++;

        char tbl_name[64];
        const char* with_pos = strstr(p, " with ");
        if (!with_pos) with_pos = strstr(p, " WITH ");

        if (with_pos) {
            int nlen = (int)(with_pos - p);
            if (nlen >= (int)sizeof(tbl_name)) nlen = (int)sizeof(tbl_name) - 1;
            strncpy(tbl_name, p, nlen);
            tbl_name[nlen] = '\0';
            trim(tbl_name);

            if (if_not_exists && enlngdb_get_table(db, tbl_name) != NULL) {
                if (print_output) {
                    printf("Query OK, 0 rows affected. Table '%s' already exists. (0.01 ms)\n\n", tbl_name);
                }
                return true;
            }

            char* cols_str = (char*)(with_pos + 6);
            EnlngColumn cols[ENLNGDB_MAX_COLS];
            int col_count = 0;

            char cols_copy[512];
            strncpy(cols_copy, cols_str, sizeof(cols_copy) - 1);
            cols_copy[sizeof(cols_copy) - 1] = '\0';

            char* cp = cols_copy;
            while (*cp && col_count < ENLNGDB_MAX_COLS) {
                while (isspace((unsigned char)*cp)) cp++;
                if (!*cp) break;
                char* item_start = cp;
                while (*cp && *cp != ',') cp++;
                if (*cp == ',') {
                    *cp = '\0';
                    cp++;
                }
                char* col_item = trim(item_start);
                char cname[64];
                sscanf(col_item, "%63s", cname);

                memset(&cols[col_count], 0, sizeof(EnlngColumn));
                strncpy(cols[col_count].name, cname, ENLNGDB_MAX_NAME - 1);
                cols[col_count].type = ENLNG_VAL_STRING;
                if (strstr(col_item, "primary key") != NULL ||
                    str_ci_cmp(cname, "id") == 0 ||
                    (strlen(cname) > 3 && str_ci_cmp(cname + strlen(cname) - 3, "_id") == 0)) {
                    cols[col_count].is_primary_key = true;
                }
                col_count++;
            }

            EnlngTable* tbl = enlngdb_create_table(db, tbl_name, cols, col_count);
            if (tbl && print_output) {
                printf("Query OK, 0 rows affected. Table '%s' created with %d columns. (0.05 ms)\n\n", tbl_name, col_count);
            }
            return tbl != NULL;
        }
    }

    // 5. insert into <name> with / insert record into <name> with
    if (str_starts_with_ci(stmt, "insert into ") || str_starts_with_ci(stmt, "insert record into ")) {
        const char* start = str_starts_with_ci(stmt, "insert record into ") ? (stmt + 19) : (stmt + 12);
        char tbl_name[64];
        const char* with_pos = strstr(start, " with ");
        if (!with_pos) with_pos = strstr(start, " WITH ");

        if (with_pos) {
            int nlen = (int)(with_pos - start);
            if (nlen >= (int)sizeof(tbl_name)) nlen = (int)sizeof(tbl_name) - 1;
            strncpy(tbl_name, start, nlen);
            tbl_name[nlen] = '\0';
            trim(tbl_name);

            EnlngTable* tbl = enlngdb_get_table(db, tbl_name);
            if (!tbl) {
                // Auto create table
                tbl = enlngdb_create_table(db, tbl_name, NULL, 0);
            }

            const char* pairs_str = with_pos + 6;
            char pairs_copy[512];
            strncpy(pairs_copy, pairs_str, sizeof(pairs_copy) - 1);
            pairs_copy[sizeof(pairs_copy) - 1] = '\0';

            // each value goes in its own column's place; a column not given stays null
            EnlngVal cells[ENLNGDB_MAX_COLS];
            for (int i = 0; i < ENLNGDB_MAX_COLS; i++) cells[i] = enlng_null();
            int cell_cnt = 0;

            // Split pairs by comma outside quotes
            char* p = pairs_copy;
            while (*p) {
                while (isspace((unsigned char)*p)) p++;
                if (!*p) break;

                char* item_start = p;
                bool in_q = false;
                char q_char = 0;

                while (*p) {
                    if (!in_q && (*p == '"' || *p == '\'')) {
                        in_q = true;
                        q_char = *p;
                    } else if (in_q && *p == q_char) {
                        in_q = false;
                    } else if (!in_q && *p == ',') {
                        break;
                    }
                    p++;
                }

                char saved = *p;
                *p = '\0';

                char* pair = trim(item_start);
                char col_k[64];
                char* val_part = NULL;

                // Match key value: key val OR key: val OR key = val
                char* sep = strpbrk(pair, " :=");
                if (sep) {
                    int klen = (int)(sep - pair);
                    if (klen >= (int)sizeof(col_k)) klen = (int)sizeof(col_k) - 1;
                    strncpy(col_k, pair, klen);
                    col_k[klen] = '\0';
                    while (*sep == ' ' || *sep == ':' || *sep == '=') sep++;
                    val_part = sep;

                    // If column does not exist on table, add it
                    int col_idx = -1;
                    for (int c = 0; c < tbl->col_count; c++) {
                        if (strcmp(tbl->columns[c].name, col_k) == 0) {
                            col_idx = c;
                            break;
                        }
                    }
                    if (col_idx < 0 && tbl->col_count < ENLNGDB_MAX_COLS) {
                        col_idx = tbl->col_count++;
                        strncpy(tbl->columns[col_idx].name, col_k, ENLNGDB_MAX_NAME - 1);
                    }

                    if (col_idx >= 0) {
                        enlng_free_val(&cells[col_idx]);
                        cells[col_idx] = parse_val_token(val_part);
                    }
                }

                if (saved == ',') p++;
            }

            cell_cnt = tbl->col_count;
            bool ok = enlngdb_insert_row(tbl, cells, cell_cnt);
            for (int i = 0; i < ENLNGDB_MAX_COLS; i++) enlng_free_val(&cells[i]);

            if (ok && print_output) {
                if (enlngdb_last_insert_replaced[0]) {
                    printf("Query OK, 1 row replaced: '%s' already had a row with %s. (0.04 ms)\n\n", tbl->name, enlngdb_last_insert_replaced);
                } else {
                    printf("Query OK, 1 row affected (0.04 ms)\n\n");
                }
            }
            return ok;
        }
    }

    // 6. find / fetch / select / get / show all records from, and 7. count records in
    if (str_starts_with_ci(stmt, "find ") || str_starts_with_ci(stmt, "fetch ") || str_starts_with_ci(stmt, "select ") ||
        str_starts_with_ci(stmt, "get ") || str_starts_with_ci(stmt, "show all ") || str_starts_with_ci(stmt, "count ")) {
        return run_query(db, stmt, print_output);
    }

    // alter table <t> add [column] <c> [as <kind>] [default <v>]  /  add column <c> to <t> ...
    // alter table <t> drop|delete|remove column <c> [confirmed]
    if (str_starts_with_ci(stmt, "alter table ") || str_starts_with_ci(stmt, "add column ")) {
        static QLex A;
        q_lex(&A, stmt);
        char tname[64] = {0}, cname[64] = {0};
        bool adding = true;
        if (q_take(&A, "alter")) {
            q_take(&A, "table");
            if (q_peek(&A, 0)->kind != QT_WORD) goto alter_usage;
            snprintf(tname, sizeof(tname), "%s", q_peek(&A, 0)->text);
            A.pos++;
            if (q_take(&A, "add")) adding = true;
            else if (q_take(&A, "drop") || q_take(&A, "delete") || q_take(&A, "remove")) adding = false;
            else goto alter_usage;
            q_take(&A, "column");
            if (q_peek(&A, 0)->kind != QT_WORD) goto alter_usage;
            snprintf(cname, sizeof(cname), "%s", q_peek(&A, 0)->text);
            A.pos++;
        } else {
            q_take(&A, "add");
            q_take(&A, "column");
            if (q_peek(&A, 0)->kind != QT_WORD) goto alter_usage;
            snprintf(cname, sizeof(cname), "%s", q_peek(&A, 0)->text);
            A.pos++;
            if (!(q_take(&A, "to") || q_take(&A, "in"))) goto alter_usage;
            if (q_peek(&A, 0)->kind != QT_WORD) goto alter_usage;
            snprintf(tname, sizeof(tname), "%s", q_peek(&A, 0)->text);
            A.pos++;
        }
        if (!adding) {
            char del_stmt[256];
            snprintf(del_stmt, sizeof(del_stmt), "delete column %s from %s", cname, tname);
            return enlngdb_execute_statement(db, del_stmt, print_output);
        }
        EnlngVal def = enlng_null();
        if (q_take(&A, "as")) {
            /* the kind (text, integer, real ...) is accepted; values keep the kind they are written in */
            if (q_peek(&A, 0)->kind == QT_WORD) A.pos++;
        }
        if (q_take(&A, "default")) {
            if (q_peek(&A, 0)->kind == QT_END) goto alter_usage;
            def = parse_val_token(q_peek(&A, 0)->text);
            A.pos++;
        }
        if (q_peek(&A, 0)->kind != QT_END) goto alter_usage;
        EnlngTable* tbl = enlngdb_get_table(db, tname);
        if (!tbl) {
            enlng_free_val(&def);
            if (print_output) print_missing_table(db, tname);
            return false;
        }
        bool exists = false;
        for (int c = 0; c < tbl->col_count; c++) exists = exists || strcmp(tbl->columns[c].name, cname) == 0;
        bool added = !exists && enlngdb_add_column(tbl, cname, &def);
        enlng_free_val(&def);
        if (print_output) {
            if (added) printf("Query OK, column '%s' added to table '%s'. (0.02 ms)\n\n", cname, tname);
            else if (exists) printf("ERROR: Table '%s' already has a column called '%s'.\n\n", tname, cname);
            else printf("ERROR: Table '%s' already has %d columns, the most a table can have.\n\n", tname, ENLNGDB_MAX_COLS);
        }
        return added;
    alter_usage:
        if (print_output) {
            printf("ERROR: I don't understand this column change. Write it like:\n");
            printf("         alter table student add column age as integer default 18\n");
            printf("         add column age to student\n");
            printf("         alter table student drop column age confirmed\n\n");
        }
        return false;
    }

    // delete table <name> confirmed: another way to say drop table
    if (str_starts_with_ci(stmt, "delete table ") || str_starts_with_ci(stmt, "remove table ")) {
        char drop_stmt[1024];
        snprintf(drop_stmt, sizeof(drop_stmt), "drop table %s", stmt + 13);
        return enlngdb_execute_statement(db, drop_stmt, print_output);
    }

    // delete column <col> from <table>, or just: delete <col> from <table>
    if (str_starts_with_ci(stmt, "delete ") || str_starts_with_ci(stmt, "remove ") || str_starts_with_ci(stmt, "drop column ")) {
        const char* p = stmt + (str_starts_with_ci(stmt, "drop column ") ? 5 : 7);
        while (isspace((unsigned char)*p)) p++;
        bool said_column = str_starts_with_ci(p, "column ");
        if (said_column) p += 7;
        char col_name[64] = {0}, link[16] = {0}, tbl_name[64] = {0}, extra[16] = {0};
        int n = sscanf(p, "%63s %15s %63s %15s", col_name, link, tbl_name, extra);
        bool is_link = str_ci_cmp(link, "from") == 0 || str_ci_cmp(link, "in") == 0;
        bool is_row_word = str_ci_cmp(col_name, "records") == 0 || str_ci_cmp(col_name, "all") == 0 ||
                           str_ci_cmp(col_name, "from") == 0;
        bool ends_right = n == 3 || (n == 4 && str_ci_cmp(extra, "confirmed") == 0);
        if (ends_right && is_link && (said_column || !is_row_word)) {
            EnlngTable* tbl = enlngdb_get_table(db, tbl_name);
            if (!tbl) {
                if (print_output) print_missing_table(db, tbl_name);
                return false;
            }
            if (!enlngdb_delete_column(tbl, col_name)) {
                if (print_output) {
                    printf("ERROR: Table '%s' has no column called '%s'.\n", tbl_name, col_name);
                    printf("       Its columns are:");
                    for (int c = 0; c < tbl->col_count; c++) printf(" %s", tbl->columns[c].name);
                    printf("\n\n");
                }
                return false;
            }
            if (print_output) {
                printf("Query OK, column '%s' deleted from table '%s'. (0.02 ms)\n\n", col_name, tbl_name);
            }
            return true;
        }
    }

    // 8. delete / remove records from <table> [where ...]
    if (str_starts_with_ci(stmt, "delete records from ") || str_starts_with_ci(stmt, "delete from ") || str_starts_with_ci(stmt, "delete all from ") || str_starts_with_ci(stmt, "delete all records from ") ||
        str_starts_with_ci(stmt, "remove records from ") || str_starts_with_ci(stmt, "remove from ") || str_starts_with_ci(stmt, "remove all from ") || str_starts_with_ci(stmt, "remove all records from ")) {
        const char* p = stmt;
        if (str_starts_with_ci(p, "delete all records from ")) p += 24;
        else if (str_starts_with_ci(p, "delete all from ")) p += 16;
        else if (str_starts_with_ci(p, "delete records from ")) p += 20;
        else if (str_starts_with_ci(p, "delete from ")) p += 12;
        else if (str_starts_with_ci(p, "remove all records from ")) p += 24;
        else if (str_starts_with_ci(p, "remove all from ")) p += 16;
        else if (str_starts_with_ci(p, "remove records from ")) p += 20;
        else if (str_starts_with_ci(p, "remove from ")) p += 12;
        while (isspace((unsigned char)*p)) p++;

        char rest[512];
        strncpy(rest, p, sizeof(rest) - 1);
        rest[sizeof(rest) - 1] = '\0';

        bool confirmed = (strstr(rest, " confirmed") != NULL || strstr(rest, " CONFIRMED") != NULL);
        char* conf_pos = strstr(rest, " confirmed");
        if (!conf_pos) conf_pos = strstr(rest, " CONFIRMED");
        if (conf_pos) *conf_pos = '\0';

        char tbl_name[64] = {0};
        char filter_col[64] = {0};
        EnlngOp op = OP_NONE;
        EnlngVal target_val = enlng_null();

        char* where_pos = strstr(rest, " where ");
        if (!where_pos) where_pos = strstr(rest, " WHERE ");

        if (where_pos) {
            *where_pos = '\0';
            strncpy(tbl_name, trim(rest), sizeof(tbl_name) - 1);
            char* cond = trim(where_pos + 7);
            char cname[64];
            if (sscanf(cond, "%63s", cname) == 1) {
                char* dot = strchr(cname, '.');
                if (dot) strncpy(filter_col, dot + 1, sizeof(filter_col) - 1);
                else strncpy(filter_col, cname, sizeof(filter_col) - 1);

                char* op_start = cond + strlen(cname);
                while (isspace((unsigned char)*op_start)) op_start++;
                int op_len = 0;
                op = parse_op_phrase(op_start, &op_len);
                if (op != OP_NONE) {
                    char* val_str = trim(op_start + op_len);
                    target_val = parse_val_token(val_str);
                }
            }
        } else {
            strncpy(tbl_name, trim(rest), sizeof(tbl_name) - 1);
        }

        EnlngTable* tbl = enlngdb_get_table(db, tbl_name);
        if (!tbl) {
            if (print_output) print_missing_table(db, tbl_name);
            return false;
        }

        if (!filter_col[0] && !confirmed) {
            enlng_free_val(&target_val);
            if (print_output) {
                printf("REFUSED: 'delete from %s' with no 'where' would remove every row.\n", tbl_name);
                printf("         Nothing was deleted. To go ahead, add the word 'confirmed':\n");
                printf("         delete all from %s confirmed;\n\n", tbl_name);
            }
            return false;
        }

        int del_cnt = enlngdb_delete(tbl, filter_col[0] ? filter_col : NULL, op, &target_val, filter_col[0] ? true : confirmed);
        enlng_free_val(&target_val);
        if (print_output) {
            printf("Query OK, %d row(s) deleted. (0.02 ms)\n\n", del_cnt);
        }
        return true;
    }

    // 8. drop table <name> confirmed
    if (str_starts_with_ci(stmt, "drop table ")) {
        char tbl_name[64];
        bool confirmed = (strstr(stmt, " confirmed") != NULL || strstr(stmt, " CONFIRMED") != NULL);
        if (sscanf(stmt + 11, "%63s", tbl_name) == 1) {
            if (!confirmed && enlngdb_get_table(db, tbl_name)) {
                if (print_output) {
                    printf("REFUSED: dropping table '%s' deletes the table and all its rows.\n", tbl_name);
                    printf("         Nothing was changed. To go ahead, add the word 'confirmed':\n");
                    printf("         drop table %s confirmed;\n\n", tbl_name);
                }
                return false;
            }
            bool ok = enlngdb_drop_table(db, tbl_name, confirmed);
            if (ok && print_output) {
                printf("Query OK, table '%s' dropped successfully. (0.02 ms)\n\n", tbl_name);
            }
            return ok;
        }
    }

    // 9. update / change / in <table_name> update/change/set
    bool is_in_stmt = str_starts_with_ci(stmt, "in ");
    bool is_upd_stmt = str_starts_with_ci(stmt, "update ") || str_starts_with_ci(stmt, "change ") || str_starts_with_ci(stmt, "modify ");

    if (is_in_stmt || is_upd_stmt) {
        char tbl_name[64] = {0};
        const char* after_action = NULL;

        if (is_in_stmt) {
            const char* p = stmt + 3;
            while (isspace((unsigned char)*p)) p++;
            if (str_starts_with_ci(p, "table ")) p += 6;
            if (str_starts_with_ci(p, "the ")) p += 4;
            while (isspace((unsigned char)*p)) p++;

            char tname[64] = {0};
            if (sscanf(p, "%63s", tname) == 1) {
                strncpy(tbl_name, tname, sizeof(tbl_name) - 1);
                const char* post_tbl = p + strlen(tname);
                while (isspace((unsigned char)*post_tbl)) post_tbl++;

                if (str_starts_with_ci(post_tbl, "update ")) {
                    post_tbl += 7;
                } else if (str_starts_with_ci(post_tbl, "change ")) {
                    post_tbl += 7;
                } else if (str_starts_with_ci(post_tbl, "set ")) {
                    post_tbl += 4;
                } else if (str_starts_with_ci(post_tbl, "modify ")) {
                    post_tbl += 7;
                } else {
                    // Not an update statement starting with 'in'
                    return true;
                }

                while (isspace((unsigned char)*post_tbl)) post_tbl++;
                if (str_starts_with_ci(post_tbl, "set ")) {
                    post_tbl += 4;
                    while (isspace((unsigned char)*post_tbl)) post_tbl++;
                }
                after_action = post_tbl;
            }
        } else {
            const char* p = stmt;
            if (str_starts_with_ci(p, "update ")) p += 7;
            else if (str_starts_with_ci(p, "change ")) p += 7;
            else if (str_starts_with_ci(p, "modify ")) p += 7;
            while (isspace((unsigned char)*p)) p++;

            if (str_starts_with_ci(p, "records in ")) p += 11;
            else if (str_starts_with_ci(p, "rows in ")) p += 8;
            else if (str_starts_with_ci(p, "in ")) p += 3;
            if (str_starts_with_ci(p, "the ")) p += 4;
            if (str_starts_with_ci(p, "table ")) p += 6;
            while (isspace((unsigned char)*p)) p++;

            const char* set_pos = strstr(p, " set ");
            if (!set_pos) set_pos = strstr(p, " SET ");

            if (set_pos) {
                int tlen = (int)(set_pos - p);
                if (tlen >= (int)sizeof(tbl_name)) tlen = (int)sizeof(tbl_name) - 1;
                strncpy(tbl_name, p, tlen);
                tbl_name[tlen] = '\0';
                trim(tbl_name);

                after_action = set_pos + 5;
                while (isspace((unsigned char)*after_action)) after_action++;
            } else {
                char tname[64] = {0};
                if (sscanf(p, "%63s", tname) == 1) {
                    strncpy(tbl_name, tname, sizeof(tbl_name) - 1);
                    const char* post_tbl = p + strlen(tname);
                    while (isspace((unsigned char)*post_tbl)) post_tbl++;
                    if (str_starts_with_ci(post_tbl, "set ")) {
                        post_tbl += 4;
                        while (isspace((unsigned char)*post_tbl)) post_tbl++;
                    }
                    after_action = post_tbl;
                }
            }
        }

        if (tbl_name[0] && after_action && *after_action) {
            EnlngTable* tbl = enlngdb_get_table(db, tbl_name);
            if (!tbl) {
                if (print_output) print_missing_table(db, tbl_name);
                return false;
            }

            char rest[512];
            strncpy(rest, after_action, sizeof(rest) - 1);
            rest[sizeof(rest) - 1] = '\0';

            char filter_col[64] = {0};
            EnlngOp op = OP_NONE;
            EnlngVal target_val = enlng_null();

            char* where_pos = strstr(rest, " where ");
            if (!where_pos) where_pos = strstr(rest, " WHERE ");

            if (where_pos) {
                *where_pos = '\0';
                char* cond = trim(where_pos + 7);
                char cname[64];
                if (sscanf(cond, "%63s", cname) == 1) {
                    strncpy(filter_col, cname, sizeof(filter_col) - 1);
                    char* op_start = cond + strlen(cname);
                    while (isspace((unsigned char)*op_start)) op_start++;
                    int op_len = 0;
                    op = parse_op_phrase(op_start, &op_len);
                    if (op != OP_NONE) {
                        char* val_str = trim(op_start + op_len);
                        target_val = parse_val_token(val_str);
                    }
                }
            }

            int total_updated = 0;
            char* aptr = rest;
            while (*aptr) {
                while (isspace((unsigned char)*aptr) || *aptr == ',') aptr++;
                if (!*aptr) break;

                char* assign_end = strchr(aptr, ',');
                if (assign_end) *assign_end = '\0';

                char* sep = strstr(aptr, " = ");
                if (!sep) sep = strstr(aptr, " to ");
                if (!sep) sep = strstr(aptr, " is ");
                if (!sep) sep = strstr(aptr, " : ");
                if (!sep) sep = strchr(aptr, '=');

                if (sep) {
                    char col_name[64] = {0};
                    int clen = (int)(sep - aptr);
                    if (clen >= (int)sizeof(col_name)) clen = (int)sizeof(col_name) - 1;
                    strncpy(col_name, aptr, clen);
                    col_name[clen] = '\0';
                    trim(col_name);

                    if (*sep == '=') sep++;
                    else if (strncmp(sep, " = ", 3) == 0) sep += 3;
                    else if (strncmp(sep, " to ", 4) == 0) sep += 4;
                    else if (strncmp(sep, " is ", 4) == 0) sep += 4;
                    else if (strncmp(sep, " : ", 3) == 0) sep += 3;

                    char* val_str = trim(sep);
                    EnlngVal set_val = parse_val_token(val_str);

                    int count = enlngdb_update(tbl, filter_col[0] ? filter_col : NULL, op, &target_val, col_name, &set_val);
                    enlng_free_val(&set_val);
                    if (count > total_updated) total_updated = count;
                }

                if (assign_end) aptr = assign_end + 1;
                else break;
            }

            enlng_free_val(&target_val);

            if (print_output) {
                printf("Query OK, %d row(s) updated. (0.03 ms)\n\n", total_updated);
            }
            return true;
        }
    }

    if (print_output) {
        fprintf(stderr, "\n-- EnlangDB SyntaxError ------------------------------------------------\n");
        fprintf(stderr, "Location: <statement>:1:1\n\n");
        fprintf(stderr, "    1 | %s\n", stmt);
        fprintf(stderr, "      | ^\n\n");
        if (str_starts_with_ci(stmt, "create table")) {
            fprintf(stderr, "What: Incomplete 'create table' statement.\n");
            fprintf(stderr, "Why:  EnlngDB tables require column definitions after the 'with' keyword.\n\n");
            fprintf(stderr, "Suggestions:\n");
            fprintf(stderr, "  * Specify table columns, e.g.: create table users with id, name, email\n");
        } else if (str_starts_with_ci(stmt, "find") || str_starts_with_ci(stmt, "select")) {
            fprintf(stderr, "What: Malformed query statement.\n");
            fprintf(stderr, "Why:  EnlngDB queries require a target table specified via 'from <table>' or 'in <table>'.\n\n");
            fprintf(stderr, "Suggestions:\n");
            fprintf(stderr, "  * Add the source table, e.g.: find records from users\n");
            fprintf(stderr, "  * Or project columns: find user_id of users from users\n");
        } else if (str_starts_with_ci(stmt, "insert")) {
            fprintf(stderr, "What: Malformed 'insert' statement.\n");
            fprintf(stderr, "Why:  Records require target table and values specified via 'with' or 'values'.\n\n");
            fprintf(stderr, "Suggestions:\n");
            fprintf(stderr, "  * Insert key-value pairs: insert into users with id 1, name \"Aero\"\n");
        } else {
            fprintf(stderr, "What: Unrecognized or malformed database statement: '%s'\n", stmt);
            fprintf(stderr, "Why:  Statement does not match any valid conversational EnlngDB grammar.\n\n");
            fprintf(stderr, "Suggestions:\n");
            fprintf(stderr, "  * Supported commands: use database, show tables, create table, insert into, find records from, update, change, delete, delete column, drop table\n");
            fprintf(stderr, "  * Comments start with '#' or '--' at the beginning of a line.\n");
            fprintf(stderr, "  * Run 'enlngdb --help' to view command reference.\n");
        }
        fprintf(stderr, "------------------------------------------------------------------------\n\n");
    }
    return false;
}

int enlngdb_execute_script(EnlngDatabase* db, const char* script_content, bool print_output) {
    if (!db || !script_content) return 0;
    char* copy = strdup(script_content);
    int executed = 0;
    int failed = 0;

    char* ptr = copy;
    while (*ptr) {
        while (*ptr && (isspace((unsigned char)*ptr) || *ptr == ';' || *ptr == '\n' || *ptr == '\r')) ptr++;
        if (!*ptr) break;

        // Skip comments starting with '#' or '--'
        if (*ptr == '#' || (ptr[0] == '-' && ptr[1] == '-')) {
            while (*ptr && *ptr != '\n' && *ptr != '\r') ptr++;
            continue;
        }

        char* stmt_start = ptr;
        bool in_q = false;
        char q_char = 0;

        while (*ptr) {
            if (!in_q && (*ptr == '"' || *ptr == '\'')) {
                in_q = true;
                q_char = *ptr;
            } else if (in_q && *ptr == q_char) {
                in_q = false;
            } else if (!in_q && (*ptr == ';' || *ptr == '\n' || *ptr == '\r')) {
                break;
            }
            ptr++;
        }

        if (*ptr) {
            *ptr = '\0';
            ptr++;
        }

        char* trimmed = trim(stmt_start);
        if (*trimmed && *trimmed != '#' && !(trimmed[0] == '-' && trimmed[1] == '-')) {
            if (enlngdb_execute_statement(db, trimmed, print_output)) {
                executed++;
            } else {
                failed++;
            }
        }
    }
    free(copy);
    return failed > 0 ? -failed : executed;
}

int enlngdb_run_file(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "[ENLNGDB ERROR] Could not open file: %s\n", filepath);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer = (char*)malloc(sz + 1);
    if (!buffer) {
        fclose(f);
        return 1;
    }

    size_t actual = fread(buffer, 1, sz, f);
    buffer[actual] = '\0';
    fclose(f);

    // Derive database name from filename
    char dbname[64];
    const char* base = strrchr(filepath, '/');
    if (!base) base = strrchr(filepath, '\\');
    base = base ? base + 1 : filepath;
    strncpy(dbname, base, sizeof(dbname) - 1);
    char* dot = strrchr(dbname, '.');
    if (dot) *dot = '\0';

    EnlngDatabase* db = enlngdb_create(dbname);

    // Try auto-loading existing binary database file
    char edb_file[256];
    snprintf(edb_file, sizeof(edb_file), "%s.edb", dbname);
    enlngdb_load(db, edb_file);

    int count = enlngdb_execute_script(db, buffer, true);

    // Auto-save to the database that is active at the end: 'use database X' moves the
    // target from the script-named file to X's file (current folder first, then tests/,
    // the same order 'use database' loads it in). Nothing is saved if any statement failed.
    if (count >= 0) {
        char save_file[256];
        snprintf(save_file, sizeof(save_file), "%s.edb", db->name);
        if (strcmp(db->name, dbname) != 0) {
            FILE* here = fopen(save_file, "rb");
            if (here) {
                fclose(here);
            } else {
                char alt[256];
                snprintf(alt, sizeof(alt), "tests/%s.edb", db->name);
                FILE* there = fopen(alt, "rb");
                if (there) {
                    fclose(there);
                    snprintf(save_file, sizeof(save_file), "%s", alt);
                }
            }
        }
        enlngdb_save(db, save_file);
    } else {
        printf("NOTE: %d statement(s) failed, so nothing from this run was saved to disk.\n", -count);
        printf("      Fix the errors above and run the script again.\n");
    }

    free(buffer);
    enlngdb_free(db);
    return count >= 0 ? 0 : 1;
}
