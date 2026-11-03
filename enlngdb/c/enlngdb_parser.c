#include "enlngdb.h"
#include <ctype.h>
#include <time.h>
#include <math.h>

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

/* ---- Queries ---------------------------------------------------------------
 *   [with <name> as (<query>), ...]
 *   find|fetch|select|get [top N | first N] [distinct] <what> [from <source>
 *       [inner|left|right|full|cross join <source> [on <condition> | using (<column>)]] ...
 *       [where <condition>] [group by <expr>, ...] [having <condition>]
 *       [order by <expr> [asc|desc], ...] [limit N] [offset N]]
 *   <query> union [all] | intersect | except <query>
 *   <what>: records, *, or expressions: columns, a + b, upper(name), case when ... end,
 *           sum(x) avg(x) min(x) max(x) count(*) count(distinct x), rank() over (...), (subquery)
 *   <condition>: comparisons (is, is at least, =, >), between, in (...), in (<query>), like, is null,
 *           exists (<query>), not, and, or, ( )
 *   count records in <table> [where ...] [group by <column>]
 */

typedef enum { QT_WORD, QT_STR, QT_SYM, QT_END } QTokKind;
typedef struct {
    QTokKind kind;
    char text[256];
} QTok;

#define Q_MAX_TOKS 1024
#define Q_MAX_COLS 64
#define Q_MAX_ITEMS 64
#define Q_MAX_SRC 8
#define Q_MAX_GROUP 8
#define Q_MAX_ORD 8
#define Q_MAX_WITH 8
#define Q_MAX_ARGS 16

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
        } else if (strchr("(),+*/%", *s)) {
            t->kind = QT_SYM;
            t->text[k++] = *s++;
        } else if (s[0] == '<' && s[1] == '-' && s[2] == '>') {
            /* join marks: <-> both tables whole, <- the first whole, -> the second whole, - matches only */
            t->kind = QT_SYM;
            memcpy(t->text, "<->", 3);
            k = 3;
            s += 3;
        } else if (s[0] == '<' && s[1] == '-' && !isdigit((unsigned char)s[2]) && s[2] != '.') {
            t->kind = QT_SYM;
            memcpy(t->text, "<-", 2);
            k = 2;
            s += 2;
        } else if (s[0] == '-' && s[1] == '>') {
            t->kind = QT_SYM;
            memcpy(t->text, "->", 2);
            k = 2;
            s += 2;
        } else if (s[0] == '-' && !isdigit((unsigned char)s[1]) && s[1] != '.') {
            /* a lone '-' is a join mark or a minus; -5 stays a number */
            t->kind = QT_SYM;
            t->text[k++] = *s++;
        } else if (s[0] == '<' && s[1] == '>') {
            t->kind = QT_SYM;
            memcpy(t->text, "<>", 2);
            k = 2;
            s += 2;
        } else if (strchr("=<>!", *s)) {
            t->kind = QT_SYM;
            t->text[k++] = *s++;
            if (*s == '=') t->text[k++] = *s++;
        } else {
            t->kind = QT_WORD;
            while (*s && !isspace((unsigned char)*s) && !strchr("(),\"'=<>!;+*/%", *s)) {
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

/* the tokens from..to written out again, as the title of a column: sum(amount), marks + 5 */
static void q_text(const QLex* L, int from, int to, char* out, size_t n) {
    size_t k = 0;
    out[0] = '\0';
    for (int i = from; i < to && k + 2 < n; i++) {
        const QTok* t = &L->t[i];
        if (i > from) {
            const QTok* p = &L->t[i - 1];
            bool prev_open = p->kind == QT_SYM && p->text[0] == '(';
            bool cur_close = t->kind == QT_SYM && (t->text[0] == ')' || t->text[0] == ',');
            bool call_open = t->kind == QT_SYM && t->text[0] == '(' && p->kind == QT_WORD && str_ci_cmp(p->text, "over") != 0;
            if (!(prev_open || cur_close || call_open)) out[k++] = ' ';
        }
        for (const char* x = t->text; *x && k + 1 < n; x++) out[k++] = *x;
    }
    out[k] = '\0';
}

/* ---- Expressions ---- */
typedef struct QExpr QExpr;
typedef struct QQuery QQuery;

typedef enum {
    EK_LIT, EK_NAME, EK_NEG, EK_NOT, EK_BIN, EK_CMP, EK_AND, EK_OR,
    EK_BETWEEN, EK_INLIST, EK_INSUB, EK_EXISTS, EK_ISNULL, EK_LIKE,
    EK_FUNC, EK_AGG, EK_CASE, EK_SCALAR, EK_WIN
} QKind;

/* How a bare word is read: NM_COL must be a column; the soft ones fall back to the word itself as a value
 * ("where city is Pune"). NM_SOFT_COL prefers a column of that name ("on id is student_id"). */
enum { NM_COL, NM_SOFT_LIT, NM_SOFT_COL };

typedef enum { QA_NONE, QA_COUNT, QA_SUM, QA_AVG, QA_MIN, QA_MAX, QA_CONCAT } QAgg;
typedef enum { QW_NONE, QW_ROW_NUMBER, QW_RANK, QW_DENSE_RANK, QW_LAG, QW_LEAD, QW_AGG } QWin;

struct QExpr {
    QKind kind;
    EnlngVal val;               /* a literal; for a soft name, the word as a value */
    char text[130];             /* a column name, a function name, a cast type */
    int mode;
    int op;                     /* EnlngOp for a comparison; + - * / % for arithmetic */
    bool neg;                   /* not between / not in / not like / is not null */
    bool distinct, star;
    QAgg agg;
    QWin win;
    QExpr *a, *b, *c;
    QExpr* args[Q_MAX_ARGS];
    int nargs;
    QQuery* sub;
    struct QSetS* cache;        /* a subquery that does not look at the outer row runs once */
    bool cacheable_known, cacheable;
    /* over (...) */
    QExpr* part[4];
    int npart;
    QExpr* ord[4];
    bool ord_desc[4];
    int nord;
    EnlngVal* wval;             /* the value for each row or group, computed before the rows are shown */
    size_t wn;
};

typedef struct {
    QExpr* e;
    char title[130];
    bool star;
    char star_table[64];
} QSel;

typedef struct {
    char table[64];
    char alias[64];
    QQuery* sub;
    int jk;                     /* JoinKind; the first source has none */
    QExpr* on;
    char using_col[64];
} QSrc;

typedef struct {
    QExpr* e;
    bool desc;
} QOrdKey;

typedef enum { SO_NONE, SO_UNION, SO_UNION_ALL, SO_INTERSECT, SO_EXCEPT } QSetOp;

struct QQuery {
    bool count_style, distinct, all_cols, has_from;
    long limit, offset;
    QSel sel[Q_MAX_ITEMS];
    int nsel;
    QSrc src[Q_MAX_SRC];
    int nsrc;
    QExpr* where;
    QExpr* group[Q_MAX_GROUP];
    int ngroup;
    QExpr* having;
    QOrdKey ord[Q_MAX_ORD];
    int nord;
    int setop;
    QQuery* next;
    QOrdKey cord[Q_MAX_ORD];    /* order by / limit / offset after the last part of a union */
    int ncord;
    long climit, coffset;
    struct { char name[64]; QQuery* q; } with[Q_MAX_WITH];
    int nwith;
};

static void q_free_query(QQuery* q);

static QExpr* qe_new(QKind kind) {
    QExpr* e = (QExpr*)calloc(1, sizeof(QExpr));
    e->kind = kind;
    e->val = enlng_null();
    return e;
}

static void q_free_set_cache(struct QSetS* s);

static void q_free_expr(QExpr* e) {
    if (!e) return;
    enlng_free_val(&e->val);
    q_free_expr(e->a);
    q_free_expr(e->b);
    q_free_expr(e->c);
    for (int i = 0; i < e->nargs; i++) q_free_expr(e->args[i]);
    for (int i = 0; i < e->npart; i++) q_free_expr(e->part[i]);
    for (int i = 0; i < e->nord; i++) q_free_expr(e->ord[i]);
    if (e->sub) q_free_query(e->sub);
    if (e->cache) q_free_set_cache(e->cache);
    if (e->wval) {
        for (size_t i = 0; i < e->wn; i++) enlng_free_val(&e->wval[i]);
        free(e->wval);
    }
    free(e);
}

static void q_free_query(QQuery* q) {
    while (q) {
        QQuery* next = q->next;
        for (int i = 0; i < q->nsel; i++) q_free_expr(q->sel[i].e);
        for (int i = 0; i < q->nsrc; i++) {
            q_free_query(q->src[i].sub);
            q_free_expr(q->src[i].on);
        }
        q_free_expr(q->where);
        for (int i = 0; i < q->ngroup; i++) q_free_expr(q->group[i]);
        q_free_expr(q->having);
        for (int i = 0; i < q->nord; i++) q_free_expr(q->ord[i].e);
        for (int i = 0; i < q->ncord; i++) q_free_expr(q->cord[i].e);
        for (int i = 0; i < q->nwith; i++) q_free_query(q->with[i].q);
        free(q);
        q = next;
    }
}

/* parse state */
typedef struct {
    QLex* L;
    int rhs_mode;               /* how bare words right of a comparison are read */
    bool in_rhs;
    bool cond;                  /* a bare value in a condition is an error: "expected a comparison" */
    bool no_bool;               /* a select item: 'and' separates items */
    bool no_minus_star;         /* in "on ...", '-' and '*' are join marks */
} QP;

static QExpr* qp_or(QP* P);
static QExpr* qp_add(QP* P);
static QQuery* qp_query(QP* P);

static QExpr* qp_fail(QP* P, const char* fmt, const char* a) {
    snprintf(P->L->err, sizeof(P->L->err), fmt, a);
    return NULL;
}

static bool q_query_start(QLex* L, int off) {
    return q_is(L, off, "find") || q_is(L, off, "select") || q_is(L, off, "fetch") || q_is(L, off, "get") ||
           q_is(L, off, "count") || q_is(L, off, "show") || q_is(L, off, "with");
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
        {"is not equal to", OP_NEQ}, {"not equal to", OP_NEQ}, {"is not", OP_NEQ}, {"!=", OP_NEQ}, {"<>", OP_NEQ},
        {"is equal to", OP_EQ}, {"equal to", OP_EQ}, {"equals", OP_EQ}, {"==", OP_EQ}, {"=", OP_EQ},
        {"is", OP_EQ},
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

static QAgg q_agg_word(const char* w) {
    if (str_ci_cmp(w, "count") == 0) return QA_COUNT;
    if (str_ci_cmp(w, "sum") == 0 || str_ci_cmp(w, "total") == 0) return QA_SUM;
    if (str_ci_cmp(w, "avg") == 0 || str_ci_cmp(w, "average") == 0) return QA_AVG;
    if (str_ci_cmp(w, "min") == 0 || str_ci_cmp(w, "lowest") == 0) return QA_MIN;
    if (str_ci_cmp(w, "max") == 0 || str_ci_cmp(w, "highest") == 0) return QA_MAX;
    if (str_ci_cmp(w, "group_concat") == 0 || str_ci_cmp(w, "string_agg") == 0 || str_ci_cmp(w, "join_text") == 0) return QA_CONCAT;
    return QA_NONE;
}

static bool q_is_number_word(const char* s) {
    if (*s == '-' || *s == '+') s++;
    return isdigit((unsigned char)*s) || (*s == '.' && isdigit((unsigned char)s[1]));
}

/* ( query ) is a subquery when a query word follows the bracket */
static QExpr* qp_subquery_expr(QP* P, QKind kind) {
    QLex* L = P->L;
    QExpr* e = qe_new(kind);
    QP inner = *P;
    inner.in_rhs = false;
    inner.cond = false;
    inner.no_bool = false;
    inner.no_minus_star = false;
    e->sub = qp_query(&inner);
    if (!e->sub) { q_free_expr(e); return NULL; }
    if (!q_take(L, ")")) {
        snprintf(L->err, sizeof(L->err), "a '(' before a subquery has no ')'");
        q_free_expr(e);
        return NULL;
    }
    return e;
}

static QExpr* qp_case(QP* P) {
    QLex* L = P->L;
    QExpr* e = qe_new(EK_CASE);
    bool save_bool = P->no_bool, save_cond = P->cond;
    P->no_bool = false;
    if (!q_is(L, 0, "when")) {
        e->a = qp_add(P);                       /* case x when 1 then ... */
        if (!e->a) { q_free_expr(e); return NULL; }
    }
    while (q_take(L, "when")) {
        P->cond = e->a == NULL;
        QExpr* w = e->a ? qp_add(P) : qp_or(P);
        if (!w) { q_free_expr(e); return NULL; }
        if (e->nargs + 2 > Q_MAX_ARGS) { q_free_expr(w); q_free_expr(e); return qp_fail(P, "a case has too many 'when' parts%s", ""); }
        e->args[e->nargs++] = w;
        if (!q_take(L, "then")) { q_free_expr(e); return qp_fail(P, "a 'when' needs 'then' after it%s", ""); }
        P->cond = false;
        QExpr* th = qp_or(P);
        if (!th) { q_free_expr(e); return NULL; }
        e->args[e->nargs++] = th;
    }
    if (e->nargs == 0) { q_free_expr(e); return qp_fail(P, "a case needs at least one 'when ... then ...'%s", ""); }
    if (q_take(L, "else")) {
        e->b = qp_or(P);
        if (!e->b) { q_free_expr(e); return NULL; }
    }
    if (!q_take(L, "end")) { q_free_expr(e); return qp_fail(P, "a case ends with 'end'%s", ""); }
    P->no_bool = save_bool;
    P->cond = save_cond;
    return e;
}

static bool qp_order_list(QP* P, QExpr** exprs, bool* desc, int* n, int max);

/* over ( partition by ..., order by ... ) */
static bool qp_over(QP* P, QExpr* e) {
    QLex* L = P->L;
    if (!q_take(L, "(")) { snprintf(L->err, sizeof(L->err), "'over' needs a bracket: over (order by ...)"); return false; }
    if (q_is(L, 0, "partition") && q_is(L, 1, "by")) {
        L->pos += 2;
        for (;;) {
            if (e->npart >= 4) { snprintf(L->err, sizeof(L->err), "'partition by' can name at most 4 expressions"); return false; }
            QExpr* x = qp_add(P);
            if (!x) return false;
            e->part[e->npart++] = x;
            if (!q_take(L, ",")) break;
        }
    }
    if ((q_is(L, 0, "order") || q_is(L, 0, "sorted")) && q_is(L, 1, "by")) {
        L->pos += 2;
        if (!qp_order_list(P, e->ord, e->ord_desc, &e->nord, 4)) return false;
    }
    if (!q_take(L, ")")) { snprintf(L->err, sizeof(L->err), "'over (' has no ')'"); return false; }
    return true;
}

static QExpr* qp_primary(QP* P) {
    QLex* L = P->L;
    QTok* t = q_peek(L, 0);
    if (t->kind == QT_STR) {
        QExpr* e = qe_new(EK_LIT);
        e->val = parse_val_token(t->text);
        L->pos++;
        return e;
    }
    if (t->kind == QT_SYM && t->text[0] == '(' && t->text[1] == '\0') {
        L->pos++;
        if (q_query_start(L, 0)) return qp_subquery_expr(P, EK_SCALAR);
        bool save_bool = P->no_bool, save_cond = P->cond, save_ms = P->no_minus_star;
        P->no_bool = false;
        P->cond = false;
        P->no_minus_star = false;
        QExpr* e = qp_or(P);
        P->no_bool = save_bool;
        P->cond = save_cond;
        P->no_minus_star = save_ms;
        if (!e) return NULL;
        if (!q_take(L, ")")) { q_free_expr(e); return qp_fail(P, "a '(' has no ')'%s", ""); }
        return e;
    }
    if (t->kind != QT_WORD) {
        snprintf(L->err, sizeof(L->err), "expected a value or a column name, found '%s'", t->kind == QT_END ? "the end" : t->text);
        return NULL;
    }
    if (q_is(L, 0, "case")) {
        L->pos++;
        return qp_case(P);
    }
    if (q_is(L, 0, "exists") && q_is(L, 1, "(")) {
        L->pos += 2;
        return qp_subquery_expr(P, EK_EXISTS);
    }
    if (q_is(L, 0, "cast") && q_is(L, 1, "(")) {
        L->pos += 2;
        QExpr* e = qe_new(EK_FUNC);
        strcpy(e->text, "cast");
        bool save_bool = P->no_bool;
        P->no_bool = false;
        e->args[0] = qp_or(P);
        P->no_bool = save_bool;
        e->nargs = e->args[0] ? 1 : 0;
        if (!e->args[0]) { q_free_expr(e); return NULL; }
        if (!q_take(L, "as") || q_peek(L, 0)->kind != QT_WORD) { q_free_expr(e); return qp_fail(P, "cast needs a type: cast(x as text)%s", ""); }
        snprintf(e->text + 5, sizeof(e->text) - 5, "%s", q_peek(L, 0)->text);   /* "cast" NUL <type> */
        L->pos++;
        if (!q_take(L, ")")) { q_free_expr(e); return qp_fail(P, "cast( has no ')'%s", ""); }
        return e;
    }
    if (q_is(L, 1, "(") && !q_is(L, 0, "in") && !q_is(L, 0, "from")) {
        /* a function, an aggregate, or a window function */
        char fname[64];
        snprintf(fname, sizeof(fname), "%s", t->text);
        for (char* c = fname; *c; c++) *c = (char)tolower((unsigned char)*c);
        L->pos += 2;
        QExpr* e = qe_new(EK_FUNC);
        snprintf(e->text, sizeof(e->text), "%s", fname);
        e->agg = q_agg_word(fname);
        bool save_bool = P->no_bool, save_cond = P->cond, save_ms = P->no_minus_star;
        P->no_bool = false;
        P->cond = false;
        P->no_minus_star = false;
        if (q_take(L, "distinct")) e->distinct = true;
        if (q_is(L, 0, "*") && q_is(L, 1, ")")) {
            e->star = true;
            L->pos++;
        } else if (!q_is(L, 0, ")")) {
            for (;;) {
                if (e->nargs >= Q_MAX_ARGS) { q_free_expr(e); return qp_fail(P, "too many arguments to %s", fname); }
                QExpr* x = qp_or(P);
                if (!x) { q_free_expr(e); return NULL; }
                e->args[e->nargs++] = x;
                if (!q_take(L, ",")) break;
            }
        }
        P->no_bool = save_bool;
        P->cond = save_cond;
        P->no_minus_star = save_ms;
        if (!q_take(L, ")")) { snprintf(L->err, sizeof(L->err), "%s( has no ')'", fname); q_free_expr(e); return NULL; }
        if (e->star && e->agg != QA_COUNT) {
            snprintf(L->err, sizeof(L->err), "%s(*) is not possible; name a column, like %s(marks)", fname, fname);
            q_free_expr(e);
            return NULL;
        }
        /* a window function, or an aggregate over ( ... ) */
        QWin w = QW_NONE;
        if (strcmp(fname, "row_number") == 0) w = QW_ROW_NUMBER;
        else if (strcmp(fname, "rank") == 0) w = QW_RANK;
        else if (strcmp(fname, "dense_rank") == 0) w = QW_DENSE_RANK;
        else if (strcmp(fname, "lag") == 0) w = QW_LAG;
        else if (strcmp(fname, "lead") == 0) w = QW_LEAD;
        if (q_is(L, 0, "over")) {
            L->pos++;
            if (w == QW_NONE && e->agg != QA_NONE) w = QW_AGG;
            if (w == QW_NONE) { q_free_expr(e); return qp_fail(P, "'over' can follow count, sum, avg, min, max, row_number, rank, dense_rank, lag or lead, not %s", fname); }
            e->kind = EK_WIN;
            e->win = w;
            if (!qp_over(P, e)) { q_free_expr(e); return NULL; }
            if ((w == QW_LAG || w == QW_LEAD || w == QW_AGG) && e->nargs == 0 && !e->star) {
                snprintf(L->err, sizeof(L->err), "%s( ) needs a column name, like %s(marks)", fname, fname);
                q_free_expr(e);
                return NULL;
            }
            if ((w == QW_ROW_NUMBER || w == QW_RANK || w == QW_DENSE_RANK) && e->nord == 0) {
                q_free_expr(e);
                return qp_fail(P, "%s needs an order: over (order by ...)", fname);
            }
            return e;
        }
        if (w != QW_NONE && w != QW_AGG) { q_free_expr(e); return qp_fail(P, "%s needs 'over (...)' after it", fname); }
        if (e->agg != QA_NONE) e->kind = EK_AGG;
        return e;
    }
    /* a number, a true/false/null, or a bare word */
    if (q_is_number_word(t->text) || str_ci_cmp(t->text, "true") == 0 || str_ci_cmp(t->text, "false") == 0 || str_ci_cmp(t->text, "null") == 0) {
        EnlngVal v = parse_val_token(t->text);
        if (v.type != ENLNG_VAL_STRING) {
            QExpr* e = qe_new(EK_LIT);
            e->val = v;
            L->pos++;
            return e;
        }
        enlng_free_val(&v);
    }
    QExpr* e = qe_new(EK_NAME);
    snprintf(e->text, sizeof(e->text), "%s", t->text);
    e->mode = P->in_rhs ? P->rhs_mode : NM_COL;
    e->val = parse_val_token(t->text);
    L->pos++;
    return e;
}

static QExpr* qp_unary(QP* P) {
    QLex* L = P->L;
    if (q_is(L, 0, "-")) {
        L->pos++;
        QExpr* x = qp_unary(P);
        if (!x) return NULL;
        QExpr* e = qe_new(EK_NEG);
        e->a = x;
        return e;
    }
    if (q_is(L, 0, "+")) {
        L->pos++;
        return qp_unary(P);
    }
    return qp_primary(P);
}

static QExpr* qp_bin(QKind kind, int op, QExpr* a, QExpr* b) {
    QExpr* e = qe_new(kind);
    e->op = op;
    e->a = a;
    e->b = b;
    return e;
}

static QExpr* qp_mul(QP* P) {
    QLex* L = P->L;
    QExpr* left = qp_unary(P);
    while (left) {
        int op = 0;
        if (q_is(L, 0, "*") && !P->no_minus_star) op = '*';
        else if (q_is(L, 0, "/")) op = '/';
        else if (q_is(L, 0, "%")) op = '%';
        else break;
        L->pos++;
        QExpr* right = qp_unary(P);
        if (!right) { q_free_expr(left); return NULL; }
        left = qp_bin(EK_BIN, op, left, right);
    }
    return left;
}

static QExpr* qp_add(QP* P) {
    QLex* L = P->L;
    QExpr* left = qp_mul(P);
    while (left) {
        QTok* t = q_peek(L, 0);
        if (q_is(L, 0, "+")) {
            L->pos++;
            QExpr* right = qp_mul(P);
            if (!right) { q_free_expr(left); return NULL; }
            left = qp_bin(EK_BIN, '+', left, right);
        } else if (q_is(L, 0, "-") && !P->no_minus_star) {
            L->pos++;
            QExpr* right = qp_mul(P);
            if (!right) { q_free_expr(left); return NULL; }
            left = qp_bin(EK_BIN, '-', left, right);
        } else if (t->kind == QT_WORD && t->text[0] == '-' && q_is_number_word(t->text) && !P->no_minus_star) {
            /* "marks -5": the lexer read -5 as one number */
            QExpr* lit = qe_new(EK_LIT);
            lit->val = parse_val_token(t->text + 1);
            L->pos++;
            left = qp_bin(EK_BIN, '-', left, lit);
        } else {
            break;
        }
    }
    return left;
}

static bool q_word_is_select_end(QLex* L) {
    QTok* t = q_peek(L, 0);
    if (t->kind == QT_END) return true;
    if (t->kind == QT_SYM) return t->text[0] == ')' || t->text[0] == ',';
    return false;
}

/* one comparison: a is b, a between x and y, a in (...), a like p, a is null */
static QExpr* qp_pred(QP* P) {
    QLex* L = P->L;
    QExpr* left = qp_add(P);
    if (!left) return NULL;

    /* "marks of the student is 90": the words between the column and the comparison are skipped */
    if (left->kind == EK_NAME && q_is(L, 0, "of")) {
        int save = L->pos;
        L->pos++;
        q_take(L, "the");
        if (q_peek(L, 0)->kind == QT_WORD) L->pos++;
        if (!(q_is(L, 0, "is") || q_peek(L, 0)->kind == QT_SYM || q_is(L, 0, "like") || q_is(L, 0, "between") || q_is(L, 0, "in") || q_is(L, 0, "not") ||
              q_is(L, 0, "greater") || q_is(L, 0, "less") || q_is(L, 0, "at") || q_is(L, 0, "more") || q_is(L, 0, "above") || q_is(L, 0, "below") ||
              q_is(L, 0, "under") || q_is(L, 0, "equals") || q_is(L, 0, "equal"))) L->pos = save;
    }

    bool negate = false;
    if (q_is(L, 0, "is")) {
        int k = 1;
        bool n = false;
        if (q_is(L, k, "not")) { n = true; k++; }
        if (q_is(L, k, "null")) {
            L->pos += k + 1;
            QExpr* e = qe_new(EK_ISNULL);
            e->a = left;
            e->neg = n;
            return e;
        }
        if (q_is(L, k, "between") || q_is(L, k, "like") || (q_is(L, k, "in") && q_is(L, k + 1, "("))) {
            L->pos += n ? 2 : 1;
            negate = n;
        }
    } else if (q_is(L, 0, "not") && (q_is(L, 1, "between") || q_is(L, 1, "like") || (q_is(L, 1, "in") && q_is(L, 2, "(")))) {
        L->pos++;
        negate = true;
    }

    bool save_rhs = P->in_rhs;
    if (q_take(L, "between")) {
        P->in_rhs = true;
        QExpr* lo = qp_add(P);
        QExpr* hi = NULL;
        if (lo && q_take(L, "and")) hi = qp_add(P);
        else if (lo) qp_fail(P, "'between' needs 'and', like: between 70 and 90%s", "");
        P->in_rhs = save_rhs;
        if (!lo || !hi) { q_free_expr(left); q_free_expr(lo); q_free_expr(hi); return NULL; }
        QExpr* e = qe_new(EK_BETWEEN);
        e->a = left;
        e->b = lo;
        e->c = hi;
        e->neg = negate;
        return e;
    }
    if (q_is(L, 0, "in") && q_is(L, 1, "(")) {
        L->pos += 2;
        if (q_query_start(L, 0)) {
            QExpr* e = qp_subquery_expr(P, EK_INSUB);
            if (!e) { q_free_expr(left); return NULL; }
            e->a = left;
            e->neg = negate;
            return e;
        }
        QExpr* e = qe_new(EK_INLIST);
        e->a = left;
        e->neg = negate;
        P->in_rhs = true;
        bool save_bool = P->no_bool;
        P->no_bool = false;
        for (;;) {
            if (e->nargs >= Q_MAX_ARGS) { qp_fail(P, "an 'in' list can hold at most %s values", "16"); break; }
            QExpr* x = qp_add(P);
            if (!x) { q_free_expr(e); P->in_rhs = save_rhs; P->no_bool = save_bool; return NULL; }
            e->args[e->nargs++] = x;
            if (!q_take(L, ",")) break;
        }
        P->in_rhs = save_rhs;
        P->no_bool = save_bool;
        if (L->err[0] || !q_take(L, ")")) {
            if (!L->err[0]) qp_fail(P, "an 'in (' list has no ')'%s", "");
            q_free_expr(e);
            return NULL;
        }
        return e;
    }
    if (q_take(L, "like")) {
        P->in_rhs = true;
        QExpr* pat = qp_add(P);
        P->in_rhs = save_rhs;
        if (!pat) { q_free_expr(left); return NULL; }
        QExpr* e = qe_new(EK_LIKE);
        e->a = left;
        e->b = pat;
        e->neg = negate;
        return e;
    }

    int save = L->pos;
    EnlngOp op = q_take_op(L);
    if (op == OP_NONE) {
        if (P->cond && (left->kind == EK_NAME || left->kind == EK_LIT || left->kind == EK_BIN)) {
            snprintf(L->err, sizeof(L->err), "after '%s' expected a comparison such as 'is', 'is at least' or '>'",
                     left->kind == EK_NAME ? left->text : "the value");
            q_free_expr(left);
            return NULL;
        }
        return left;
    }
    (void)save;
    QTok* v = q_peek(L, 0);
    if (v->kind == QT_END || (v->kind == QT_SYM && v->text[0] != '(' && v->text[0] != '-')) {
        snprintf(L->err, sizeof(L->err), "the comparison on '%s' has no value", left->kind == EK_NAME ? left->text : "the left side");
        q_free_expr(left);
        return NULL;
    }
    P->in_rhs = true;
    QExpr* right = qp_add(P);
    P->in_rhs = save_rhs;
    if (!right) { q_free_expr(left); return NULL; }
    QExpr* e = qp_bin(EK_CMP, op, left, right);
    return e;
}

static QExpr* qp_not(QP* P) {
    QLex* L = P->L;
    if (q_is(L, 0, "not") && !q_is(L, 1, "between") && !q_is(L, 1, "like")) {
        L->pos++;
        QExpr* x = qp_not(P);
        if (!x) return NULL;
        QExpr* e = qe_new(EK_NOT);
        e->a = x;
        return e;
    }
    return qp_pred(P);
}

static QExpr* qp_and(QP* P) {
    QLex* L = P->L;
    QExpr* left = qp_not(P);
    while (left && !P->no_bool && q_is(L, 0, "and")) {
        L->pos++;
        QExpr* right = qp_not(P);
        if (!right) { q_free_expr(left); return NULL; }
        left = qp_bin(EK_AND, 0, left, right);
    }
    return left;
}

static QExpr* qp_or(QP* P) {
    QLex* L = P->L;
    QExpr* left = qp_and(P);
    while (left && !P->no_bool && q_is(L, 0, "or")) {
        L->pos++;
        QExpr* right = qp_and(P);
        if (!right) { q_free_expr(left); return NULL; }
        left = qp_bin(EK_OR, 0, left, right);
    }
    return left;
}

/* a condition: where, having, on, case when, check */
static QExpr* qp_condition(QP* P, int rhs_mode, bool no_minus_star) {
    bool s_rhs = P->in_rhs, s_cond = P->cond, s_mode = P->rhs_mode, s_ms = P->no_minus_star, s_bool = P->no_bool;
    P->in_rhs = false;
    P->cond = true;
    P->rhs_mode = rhs_mode;
    P->no_minus_star = no_minus_star;
    P->no_bool = false;
    QExpr* e = qp_or(P);
    P->in_rhs = s_rhs;
    P->cond = s_cond;
    P->rhs_mode = s_mode;
    P->no_minus_star = s_ms;
    P->no_bool = s_bool;
    return e;
}

/* a value: set column to <value>, values (...) */
static QExpr* qp_value(QP* P, int rhs_mode) {
    bool s_rhs = P->in_rhs, s_cond = P->cond, s_mode = P->rhs_mode, s_ms = P->no_minus_star, s_bool = P->no_bool;
    P->in_rhs = true;
    P->cond = false;
    P->rhs_mode = rhs_mode;
    P->no_minus_star = false;
    P->no_bool = true;
    QExpr* e = qp_or(P);
    P->in_rhs = s_rhs;
    P->cond = s_cond;
    P->rhs_mode = s_mode;
    P->no_minus_star = s_ms;
    P->no_bool = s_bool;
    return e;
}

/* expr [asc|desc|highest first|lowest first], ... */
static bool qp_order_list(QP* P, QExpr** exprs, bool* desc, int* n, int max) {
    QLex* L = P->L;
    for (;;) {
        if (*n >= max) { snprintf(L->err, sizeof(L->err), "'order by' can name at most %d expressions", max); return false; }
        bool s_bool = P->no_bool;
        P->no_bool = false;
        QExpr* x = qp_add(P);
        P->no_bool = s_bool;
        if (!x) return false;
        bool d = false;
        if (q_take(L, "desc") || q_take(L, "descending")) d = true;
        else if (q_take(L, "asc") || q_take(L, "ascending")) d = false;
        else if (q_is(L, 0, "highest") || q_is(L, 0, "largest") || q_is(L, 0, "biggest")) { L->pos++; q_take(L, "first"); d = true; }
        else if (q_is(L, 0, "lowest") || q_is(L, 0, "smallest")) { L->pos++; q_take(L, "first"); d = false; }
        exprs[*n] = x;
        desc[*n] = d;
        (*n)++;
        if (!q_take(L, ",")) break;
    }
    return true;
}

/* ---- Joins ---------------------------------------------------------------
 *   join, inner join      only the rows that match                         -
 *   left join             every row of the first table                     <-
 *   right join            every row of the table after the join word       ->
 *   full join             every row of both                                <->
 *   cross join            every row with every row (no 'on')               *
 * "left outer join" and so on are the same. The marks on the right are the other way to write them:
 * the arrow points at the table whose rows all stay.
 */
typedef enum { JK_INNER, JK_LEFT, JK_RIGHT, JK_FULL, JK_CROSS } JoinKind;

static bool q_take_join(QLex* L, JoinKind* kind) {
    static const struct { const char* word; JoinKind kind; bool outer_ok; } words[] = {
        {"inner", JK_INNER, false}, {"left", JK_LEFT, true}, {"right", JK_RIGHT, true},
        {"full", JK_FULL, true}, {"cross", JK_CROSS, false}, {NULL, JK_INNER, false}
    };
    static const struct { const char* mark; JoinKind kind; } marks[] = {
        {"<->", JK_FULL}, {"<-", JK_LEFT}, {"->", JK_RIGHT}, {"-", JK_INNER}, {"*", JK_CROSS}, {NULL, JK_INNER}
    };
    if (q_take(L, "join")) { *kind = JK_INNER; return true; }
    for (int i = 0; words[i].word; i++) {
        if (!q_is(L, 0, words[i].word)) continue;
        if (q_is(L, 1, "join")) { L->pos += 2; *kind = words[i].kind; return true; }
        if (words[i].outer_ok && q_is(L, 1, "outer") && q_is(L, 2, "join")) { L->pos += 3; *kind = words[i].kind; return true; }
    }
    for (int i = 0; marks[i].mark; i++) {
        if (q_is(L, 0, marks[i].mark)) { L->pos++; *kind = marks[i].kind; return true; }
    }
    return false;
}

static bool q_reserved_word(const char* w);

/* "the" and "a" before a table name are skipped (but not in "from a where ...": a is the table) */
static void q_skip_silent(QLex* L) {
    while ((q_is(L, 0, "the") || q_is(L, 0, "a") || q_is(L, 0, "an")) && q_peek(L, 1)->kind == QT_WORD && !q_reserved_word(q_peek(L, 1)->text)) L->pos++;
}

static bool q_reserved_word(const char* w) {
    static const char* words[] = {
        "join", "inner", "left", "right", "full", "cross", "outer", "natural", "on", "using", "where", "group", "order",
        "sorted", "sort", "limit", "offset", "skip", "having", "union", "intersect", "except", "minus", "and", "or", "not",
        "as", "set", "values", "select", "find", "fetch", "get", "from", "in", "with", "confirmed", "top", "first",
        "distinct", NULL
    };
    for (int i = 0; words[i]; i++) if (str_ci_cmp(w, words[i]) == 0) return true;
    return false;
}

/* a table (or view, or common table) name, or ( query ), then an optional alias */
static bool qp_source(QP* P, QSrc* s) {
    QLex* L = P->L;
    q_skip_silent(L);
    if (q_is(L, 0, "(")) {
        L->pos++;
        QP inner = *P;
        inner.in_rhs = inner.cond = inner.no_bool = inner.no_minus_star = false;
        s->sub = qp_query(&inner);
        if (!s->sub) return false;
        if (!q_take(L, ")")) { snprintf(L->err, sizeof(L->err), "a '(' before a subquery has no ')'"); return false; }
    } else {
        QTok* t = q_peek(L, 0);
        if (t->kind != QT_WORD && t->kind != QT_STR) {
            snprintf(L->err, sizeof(L->err), "expected a table name after 'from'");
            return false;
        }
        EnlngVal nv = parse_val_token(t->text);
        snprintf(s->table, sizeof(s->table), "%s", nv.type == ENLNG_VAL_STRING && nv.str_val ? nv.str_val : t->text);
        enlng_free_val(&nv);
        L->pos++;
    }
    QTok* a = q_peek(L, 0);
    if (q_take(L, "as")) {
        a = q_peek(L, 0);
        if (a->kind != QT_WORD || isdigit((unsigned char)a->text[0])) { snprintf(L->err, sizeof(L->err), "'as' needs a name after it"); return false; }
        snprintf(s->alias, sizeof(s->alias), "%s", a->text);
        L->pos++;
    } else if (a->kind == QT_WORD && !q_reserved_word(a->text) && !q_is(L, 1, "(")) {
        snprintf(s->alias, sizeof(s->alias), "%s", a->text);
        L->pos++;
    }
    if (!s->alias[0]) snprintf(s->alias, sizeof(s->alias), "%s", s->table[0] ? s->table : "subquery");
    return true;
}

static bool q_compound_word(QLex* L, int* op) {
    if (q_is(L, 0, "union")) { L->pos++; *op = q_take(L, "all") ? SO_UNION_ALL : SO_UNION; return true; }
    if (q_is(L, 0, "intersect")) { L->pos++; *op = SO_INTERSECT; return true; }
    if (q_is(L, 0, "except") || q_is(L, 0, "minus")) { L->pos++; *op = SO_EXCEPT; return true; }
    return false;
}

static bool q_at_compound(QLex* L) {
    return q_is(L, 0, "union") || q_is(L, 0, "intersect") || q_is(L, 0, "except") || q_is(L, 0, "minus");
}

/* one find ... from ... where ... (no union): the select list, the sources and the clauses */
static QQuery* qp_select(QP* P) {
    QLex* L = P->L;
    QQuery* q = (QQuery*)calloc(1, sizeof(QQuery));
    q->limit = -1;
    q->climit = -1;

    if (q_take(L, "count")) {
        q->count_style = true;
    } else if (q_take(L, "show")) {
        if (!q_take(L, "all")) { snprintf(L->err, sizeof(L->err), "expected 'show all records from <table>'"); goto fail; }
    } else if (!(q_take(L, "find") || q_take(L, "fetch") || q_take(L, "select") || q_take(L, "get"))) {
        snprintf(L->err, sizeof(L->err), "expected find, select, fetch or get");
        goto fail;
    }

    for (;;) {
        if ((q_is(L, 0, "top") || q_is(L, 0, "first")) && q_peek(L, 1)->kind == QT_WORD && isdigit((unsigned char)q_peek(L, 1)->text[0])) {
            q->limit = atol(q_peek(L, 1)->text);
            L->pos += 2;
        } else if (q_take(L, "distinct") || q_take(L, "different") || q_take(L, "unique")) {
            q->distinct = true;
        } else {
            break;
        }
    }

    /* what to show */
    bool from_next = q_is(L, 0, "from") || (q_is(L, 0, "in") && !q_is(L, 1, "("));
    if (q->count_style) {
        /* count records in t / count all from t / count employees from t */
        while (!(q_is(L, 0, "from") || q_is(L, 0, "in")) && q_peek(L, 0)->kind != QT_END) L->pos++;
    } else if (from_next) {
        q->all_cols = true;
    } else {
        /* "all the records", "every record", "*": only such words before 'from' mean every column */
        static const char* every[] = {"all", "the", "*", "records", "record", "rows", "row", "every", "of", NULL};
        int k = 0;
        for (;; k++) {
            bool word = false;
            for (int w = 0; every[w]; w++) word = word || q_is(L, k, every[w]);
            if (!word) break;
        }
        if (k > 0 && (q_is(L, k, "from") || q_is(L, k, "in"))) {
            L->pos += k;
            q->all_cols = true;
        }
    }
    if (!q->count_style && !q->all_cols) {
        for (;;) {
            if (q->nsel >= Q_MAX_ITEMS) { snprintf(L->err, sizeof(L->err), "at most %d columns can be shown", Q_MAX_ITEMS); goto fail; }
            QSel* it = &q->sel[q->nsel];
            int start = L->pos;
            if (q_is(L, 0, "*")) {
                L->pos++;
                it->star = true;
            } else if (q_peek(L, 0)->kind == QT_WORD && q_is(L, 1, "*") && q_peek(L, 0)->text[0] && q_peek(L, 0)->text[strlen(q_peek(L, 0)->text) - 1] == '.') {
                /* student.* */
                snprintf(it->star_table, sizeof(it->star_table), "%.*s", (int)strlen(q_peek(L, 0)->text) - 1, q_peek(L, 0)->text);
                it->star = true;
                L->pos += 2;
            } else {
                bool s_bool = P->no_bool, s_rhs = P->in_rhs, s_cond = P->cond;
                P->no_bool = true;
                P->in_rhs = false;
                P->cond = false;
                it->e = qp_or(P);
                P->no_bool = s_bool;
                P->in_rhs = s_rhs;
                P->cond = s_cond;
                if (!it->e) goto fail;
                /* t.* : a word ending in .* */
                q_text(L, start, L->pos, it->title, sizeof(it->title));
                if (q_take(L, "as")) {
                    QTok* a = q_peek(L, 0);
                    if (a->kind != QT_WORD && a->kind != QT_STR) { snprintf(L->err, sizeof(L->err), "'as' needs a name after it"); goto fail; }
                    EnlngVal av = parse_val_token(a->text);
                    snprintf(it->title, sizeof(it->title), "%s", av.type == ENLNG_VAL_STRING && av.str_val ? av.str_val : a->text);
                    enlng_free_val(&av);
                    L->pos++;
                }
            }
            q->nsel++;
            if (!(q_take(L, ",") || q_take(L, "and"))) break;
        }
        /* "t.*" arrives as the word t.* */
        for (int i = 0; i < q->nsel; i++) {
            QSel* it = &q->sel[i];
            if (it->e && it->e->kind == EK_NAME) {
                size_t n = strlen(it->e->text);
                if (n >= 2 && strcmp(it->e->text + n - 2, ".*") == 0) {
                    snprintf(it->star_table, sizeof(it->star_table), "%.*s", (int)(n - 2), it->e->text);
                    it->star = true;
                    q_free_expr(it->e);
                    it->e = NULL;
                }
            }
        }
        from_next = q_is(L, 0, "from") || (q_is(L, 0, "in") && !q_is(L, 1, "("));
        /* find users where ...: one word and no 'from' is the table */
        if (!from_next && q->nsel == 1 && q->sel[0].e && q->sel[0].e->kind == EK_NAME && q->sel[0].e->mode == NM_COL &&
            !strchr(q->sel[0].e->text, '.') && !q_is(L, 0, ")") && !q_at_compound(L)) {
            QSrc* s = &q->src[0];
            snprintf(s->table, sizeof(s->table), "%s", q->sel[0].e->text);
            snprintf(s->alias, sizeof(s->alias), "%s", s->table);
            q->nsrc = 1;
            q->has_from = true;
            q_free_expr(q->sel[0].e);
            q->sel[0].e = NULL;
            q->nsel = 0;
            q->all_cols = true;
        }
    }

    if (!q->has_from) {
        if (q_is(L, 0, "from") || (q_is(L, 0, "in") && !q_is(L, 1, "("))) {
            L->pos++;
            q->has_from = true;
            QSrc* s = &q->src[q->nsrc];
            if (!qp_source(P, s)) goto fail;
            q->nsrc++;
        } else if (q->all_cols || q->count_style) {
            snprintf(L->err, sizeof(L->err), "expected 'from <table>'");
            goto fail;
        }
    }

    /* joins */
    JoinKind jk;
    while (q->has_from && q_take_join(L, &jk)) {
        if (q->nsrc >= Q_MAX_SRC) { snprintf(L->err, sizeof(L->err), "at most %d tables can be joined in one query", Q_MAX_SRC); goto fail; }
        QSrc* s = &q->src[q->nsrc];
        s->jk = jk;
        if (!qp_source(P, s)) goto fail;
        q->nsrc++;
        if (jk == JK_CROSS) {
            if (q_is(L, 0, "on")) { snprintf(L->err, sizeof(L->err), "a cross join pairs every row with every row, so it has no 'on'"); goto fail; }
        } else if (q_take(L, "on")) {
            s->on = qp_condition(P, NM_SOFT_COL, true);
            if (!s->on) goto fail;
        } else if (q_take(L, "using")) {
            if (!q_take(L, "(") || q_peek(L, 0)->kind != QT_WORD) { snprintf(L->err, sizeof(L->err), "'using' needs a column in brackets: using (id)"); goto fail; }
            snprintf(s->using_col, sizeof(s->using_col), "%s", q_peek(L, 0)->text);
            L->pos++;
            if (!q_take(L, ")")) { snprintf(L->err, sizeof(L->err), "'using (' has no ')'"); goto fail; }
        }
    }

    /* where, group by, having, order by, limit, offset: in any order */
    while (q_peek(L, 0)->kind != QT_END) {
        if (q_is(L, 0, ")") || q_at_compound(L)) break;
        if (q_take(L, "where")) {
            if (q->where) { snprintf(L->err, sizeof(L->err), "only one 'where' is possible; join conditions with 'and' / 'or'"); goto fail; }
            q->where = qp_condition(P, NM_SOFT_LIT, false);
            if (!q->where) goto fail;
        } else if (q_is(L, 0, "group") && q_is(L, 1, "by")) {
            L->pos += 2;
            for (;;) {
                if (q->ngroup >= Q_MAX_GROUP) { snprintf(L->err, sizeof(L->err), "'group by' can name at most %d expressions", Q_MAX_GROUP); goto fail; }
                bool s_bool = P->no_bool;
                P->no_bool = false;
                QExpr* g = qp_add(P);
                P->no_bool = s_bool;
                if (!g) goto fail;
                q->group[q->ngroup++] = g;
                if (!q_take(L, ",")) break;
            }
        } else if (q_take(L, "having")) {
            if (q->having) { snprintf(L->err, sizeof(L->err), "only one 'having' is possible; join conditions with 'and' / 'or'"); goto fail; }
            q->having = qp_condition(P, NM_SOFT_LIT, false);
            if (!q->having) goto fail;
        } else if ((q_is(L, 0, "order") || q_is(L, 0, "sorted") || q_is(L, 0, "sort")) && q_is(L, 1, "by")) {
            L->pos += 2;
            QExpr* es[Q_MAX_ORD];
            bool ds[Q_MAX_ORD];
            int n = 0;
            if (!qp_order_list(P, es, ds, &n, Q_MAX_ORD - q->nord)) goto fail;
            for (int i = 0; i < n; i++) { q->ord[q->nord].e = es[i]; q->ord[q->nord].desc = ds[i]; q->nord++; }
        } else if (q_take(L, "limit")) {
            QTok* t = q_peek(L, 0);
            if (t->kind != QT_WORD || !isdigit((unsigned char)t->text[0])) { snprintf(L->err, sizeof(L->err), "'limit' needs a number"); goto fail; }
            q->limit = atol(t->text);
            L->pos++;
        } else if (q_take(L, "offset") || q_take(L, "skip")) {
            QTok* t = q_peek(L, 0);
            if (t->kind != QT_WORD || !isdigit((unsigned char)t->text[0])) { snprintf(L->err, sizeof(L->err), "'offset' needs a number"); goto fail; }
            q->offset = atol(t->text);
            L->pos++;
        } else {
            snprintf(L->err, sizeof(L->err), "I don't understand '%s' here. After the table can come: a join (inner, left, right, full or cross join; or the marks - <- -> <-> *), where, group by, having, order by, limit, offset, union, intersect, except", q_peek(L, 0)->text);
            goto fail;
        }
    }
    (void)q_word_is_select_end;
    return q;

fail:
    q_free_query(q);
    return NULL;
}

/* [with name as (query), ...] query [union|intersect|except query ...] [order by ...] [limit ...] */
static QQuery* qp_query(QP* P) {
    QLex* L = P->L;
    QQuery* head = NULL;
    QQuery* ctes[Q_MAX_WITH] = {0};
    char names[Q_MAX_WITH][64];
    int nctes = 0;

    if (q_take(L, "with")) {
        for (;;) {
            QTok* t = q_peek(L, 0);
            if (t->kind != QT_WORD || nctes >= Q_MAX_WITH) {
                snprintf(L->err, sizeof(L->err), nctes >= Q_MAX_WITH ? "a 'with' can name at most %d queries" : "'with' needs a name: with big as (find ...)", Q_MAX_WITH);
                goto fail;
            }
            snprintf(names[nctes], sizeof(names[0]), "%s", t->text);
            L->pos++;
            if (!q_take(L, "as") || !q_take(L, "(")) { snprintf(L->err, sizeof(L->err), "'with %s' needs: as (find ...)", names[nctes]); goto fail; }
            QP inner = *P;
            inner.in_rhs = inner.cond = inner.no_bool = inner.no_minus_star = false;
            ctes[nctes] = qp_query(&inner);
            if (!ctes[nctes]) goto fail;
            nctes++;
            if (!q_take(L, ")")) { snprintf(L->err, sizeof(L->err), "a '(' before a subquery has no ')'"); goto fail; }
            if (!q_take(L, ",")) break;
        }
    }

    head = qp_select(P);
    if (!head) goto fail;
    QQuery* cur = head;
    int op;
    while (q_compound_word(L, &op)) {
        cur->setop = op;
        QQuery* nxt = qp_select(P);
        if (!nxt) goto fail;
        cur->next = nxt;
        cur = nxt;
    }
    if (head->next) {
        /* order by / limit after the last part belong to the whole result */
        for (QQuery* p = head; p->next; p = p->next) {
            if (p->nord || p->offset || p->limit >= 0) {
                snprintf(L->err, sizeof(L->err), "put 'order by' and 'limit' once, after the last query of a union");
                goto fail;
            }
        }
        for (int i = 0; i < cur->nord; i++) head->cord[head->ncord++] = cur->ord[i];
        cur->nord = 0;
        head->climit = cur->limit;
        head->coffset = cur->offset;
        cur->limit = -1;
        cur->offset = 0;
    }
    for (int i = 0; i < nctes; i++) {
        snprintf(head->with[i].name, sizeof(head->with[i].name), "%s", names[i]);
        head->with[i].q = ctes[i];
    }
    head->nwith = nctes;
    return head;

fail:
    q_free_query(head);
    for (int i = 0; i < nctes; i++) q_free_query(ctes[i]);
    return NULL;
}

/* ---- Rows being worked on, and evaluating an expression on them ---- */

/* rows being worked on: every column is named table.column */
typedef struct QSetS {
    int ncols;
    char names[Q_MAX_COLS][130];
    EnlngVal** rows;
    size_t nrows, cap;
} QSet;

static QSet* qs_new(void) {
    return (QSet*)calloc(1, sizeof(QSet));
}

static void q_set_add(QSet* S, EnlngVal* row) {
    if (S->nrows >= S->cap) {
        S->cap = S->cap ? S->cap * 2 : 16;
        S->rows = (EnlngVal**)realloc(S->rows, S->cap * sizeof(EnlngVal*));
    }
    S->rows[S->nrows++] = row;
}

static void q_set_clear(QSet* S) {
    for (size_t r = 0; r < S->nrows; r++) {
        if (!S->rows[r]) continue;
        for (int c = 0; c < S->ncols; c++) enlng_free_val(&S->rows[r][c]);
        free(S->rows[r]);
    }
    free(S->rows);
    S->rows = NULL;
    S->nrows = S->cap = 0;
}

static void q_set_free(QSet* S) {
    if (!S) return;
    q_set_clear(S);
    free(S);
}

static void q_free_set_cache(struct QSetS* s) {
    q_set_free(s);
}

static EnlngVal* q_row_new(int n) {
    return (EnlngVal*)malloc((n ? n : 1) * sizeof(EnlngVal));
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

typedef struct {
    EnlngDatabase* db;
    bool print;
    char err[300];
    bool err_printed;
    struct { const char* name; QQuery* q; } ctes[32];
    int nctes;
    int outer_hits;             /* how many names a subquery took from the row outside it */
    int depth;
} QRun;

typedef struct QEnv {
    const QSet* set;
    EnlngVal* row;
    EnlngVal** grp;             /* the rows of the current group, for sum(), count() ... */
    size_t ngrp;
    bool has_grp;
    size_t unit;                /* which row or group, for window functions */
    const struct QEnv* outer;
    QRun* R;
} QEnv;

static void q_run_error(QRun* R, const char* fmt, const char* a, const char* b) {
    if (R->err[0]) return;
    snprintf(R->err, sizeof(R->err), fmt, a, b);
}

static bool q_exec(QRun* R, QQuery* q, const QEnv* outer, QSet* out);
static EnlngVal q_eval(const QEnv* E, QExpr* e);

/* ---- values ---- */
static bool q_num(const EnlngVal* v, double* d, bool* is_int, int64_t* iv) {
    if (v->type == ENLNG_VAL_INT) { *d = (double)v->int_val; *iv = v->int_val; *is_int = true; return true; }
    if (v->type == ENLNG_VAL_DOUBLE) { *d = v->double_val; *iv = (int64_t)v->double_val; *is_int = false; return true; }
    if (v->type == ENLNG_VAL_STRING && v->str_val && *v->str_val) {
        char* end = NULL;
        long long l = strtoll(v->str_val, &end, 10);
        if (*end == '\0') { *d = (double)l; *iv = l; *is_int = true; return true; }
        double x = strtod(v->str_val, &end);
        if (*end == '\0') { *d = x; *iv = (int64_t)x; *is_int = false; return true; }
    }
    return false;
}

static void q_val_text(const EnlngVal* v, char* buf, size_t n) {
    if (v->type == ENLNG_VAL_INT) snprintf(buf, n, "%lld", (long long)v->int_val);
    else if (v->type == ENLNG_VAL_DOUBLE) snprintf(buf, n, "%g", v->double_val);
    else if (v->type == ENLNG_VAL_STRING) snprintf(buf, n, "%s", v->str_val ? v->str_val : "");
    else if (v->type == ENLNG_VAL_BOOL) snprintf(buf, n, "%s", v->bool_val ? "true" : "false");
    else snprintf(buf, n, "null");
}

static bool q_truth(const EnlngVal* v) {
    switch (v->type) {
        case ENLNG_VAL_BOOL: return v->bool_val;
        case ENLNG_VAL_INT: return v->int_val != 0;
        case ENLNG_VAL_DOUBLE: return v->double_val != 0;
        case ENLNG_VAL_STRING: return v->str_val && *v->str_val && str_ci_cmp(v->str_val, "false") != 0;
        default: return false;
    }
}

static bool q_vals_equal(const EnlngVal* a, const EnlngVal* b) {
    if (a->type == ENLNG_VAL_NULL || b->type == ENLNG_VAL_NULL) return a->type == b->type;
    return enlng_compare_vals(a, b) == 0;
}

/* a name: the column in the current row, else in the row outside (for a subquery) */
static const EnlngVal* q_lookup(const QEnv* E, const char* name, int* status) {
    int depth = 0;
    for (const QEnv* e = E; e; e = e->outer, depth++) {
        if (!e->set || !e->row) continue;
        int ci = q_find_col(e->set, name);
        if (ci >= 0) {
            if (depth > 0) E->R->outer_hits++;
            return &e->row[ci];
        }
        if (ci == -2) { *status = -2; return NULL; }
    }
    *status = -1;
    return NULL;
}

static EnlngVal q_arith(QRun* R, int op, const EnlngVal* a, const EnlngVal* b) {
    if (a->type == ENLNG_VAL_NULL || b->type == ENLNG_VAL_NULL) return enlng_null();
    double da, db;
    bool ia, ib;
    int64_t la, lb;
    bool na = q_num(a, &da, &ia, &la), nb = q_num(b, &db, &ib, &lb);
    /* two texts are joined, even when both look like numbers: "80" + "1" is "801" */
    if (op == '+' && a->type == ENLNG_VAL_STRING && b->type == ENLNG_VAL_STRING) na = false;
    if (!(na && nb)) {
        if (op == '+') {
            /* text + text (or a word and a number) joins them */
            char x[512], y[512], z[1024];
            q_val_text(a, x, sizeof(x));
            q_val_text(b, y, sizeof(y));
            snprintf(z, sizeof(z), "%s%s", x, y);
            return enlng_str(z);
        }
        char o[2] = {(char)op, 0};
        q_run_error(R, "cannot work out '%s' on text that is not a number (use + to join texts, or check the column)", o, "");
        return enlng_null();
    }
    bool both_int = ia && ib;
    switch (op) {
        case '+': return both_int ? enlng_int(la + lb) : enlng_double(da + db);
        case '-': return both_int ? enlng_int(la - lb) : enlng_double(da - db);
        case '*': return both_int ? enlng_int(la * lb) : enlng_double(da * db);
        case '/':
            if (db == 0) { q_run_error(R, "division by zero%s%s", "", ""); return enlng_null(); }
            if (both_int && la % lb == 0) return enlng_int(la / lb);
            return enlng_double(da / db);
        case '%':
            if (db == 0) { q_run_error(R, "division by zero%s%s", "", ""); return enlng_null(); }
            return both_int ? enlng_int(la % lb) : enlng_double(fmod(da, db));
    }
    return enlng_null();
}

/* ---- dates: YYYY-MM-DD with an optional time ---- */
static bool q_parse_date(const char* s, int* y, int* m, int* d) {
    if (!s) return false;
    int n = 0;
    if (sscanf(s, "%d-%d-%d%n", y, m, d, &n) != 3) return false;
    if (*m < 1 || *m > 12 || *d < 1 || *d > 31) return false;
    return true;
}

static long q_days(int y, int m, int d) {
    y -= m <= 2;
    long era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long)doe - 719468;
}

static bool q_is_date_text(const char* s) {
    int y, m, d, n = 0;
    if (!s || sscanf(s, "%d-%d-%d%n", &y, &m, &d, &n) != 3 || n < 8) return false;
    static const int days[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (y < 1 || m < 1 || m > 12 || d < 1 || d > days[m - 1]) return false;
    if (m == 2 && d == 29 && !((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return false;
    const char* rest = s + n;
    if (*rest == '\0') return true;
    if (*rest != ' ' && *rest != 'T') return false;
    int hh = 0, mm = 0, ss = 0, k = 0;
    int got = sscanf(rest + 1, "%d:%d%n", &hh, &mm, &k);
    if (got != 2 || hh < 0 || hh > 23 || mm < 0 || mm > 59) return false;
    const char* tail = rest + 1 + k;
    if (*tail == ':') {
        int k2 = 0;
        if (sscanf(tail + 1, "%d%n", &ss, &k2) != 1 || ss < 0 || ss > 59) return false;
        tail += 1 + k2;
    }
    return *tail == '\0';
}

/* ---- functions ---- */
static EnlngVal q_func(QRun* R, QExpr* e, EnlngVal* a, int n) {
    const char* f = e->text;
    char buf[1024];
    if (strcmp(f, "cast") == 0) {
        const char* type = e->text + 5;
        if (a[0].type == ENLNG_VAL_NULL) return enlng_null();
        double d; bool isint; int64_t iv;
        if (str_ci_cmp(type, "text") == 0 || str_ci_cmp(type, "string") == 0 || str_ci_cmp(type, "varchar") == 0) {
            q_val_text(&a[0], buf, sizeof(buf));
            return enlng_str(buf);
        }
        if (str_ci_cmp(type, "integer") == 0 || str_ci_cmp(type, "int") == 0) {
            if (!q_num(&a[0], &d, &isint, &iv)) { q_val_text(&a[0], buf, sizeof(buf)); q_run_error(R, "cannot turn '%s' into an integer%s", buf, ""); return enlng_null(); }
            return enlng_int(isint ? iv : (int64_t)d);
        }
        if (str_ci_cmp(type, "real") == 0 || str_ci_cmp(type, "float") == 0 || str_ci_cmp(type, "double") == 0 || str_ci_cmp(type, "decimal") == 0) {
            if (!q_num(&a[0], &d, &isint, &iv)) { q_val_text(&a[0], buf, sizeof(buf)); q_run_error(R, "cannot turn '%s' into a number%s", buf, ""); return enlng_null(); }
            return enlng_double(d);
        }
        if (str_ci_cmp(type, "boolean") == 0 || str_ci_cmp(type, "bool") == 0) return enlng_bool(q_truth(&a[0]));
        q_run_error(R, "cast to '%s' is not possible; use text, integer, real or boolean%s", type, "");
        return enlng_null();
    }
    if (strcmp(f, "coalesce") == 0 || strcmp(f, "ifnull") == 0) {
        for (int i = 0; i < n; i++) if (a[i].type != ENLNG_VAL_NULL) return enlng_copy_val(&a[i]);
        return enlng_null();
    }
    if (strcmp(f, "concat") == 0) {
        char z[1024] = "";
        for (int i = 0; i < n; i++) {
            if (a[i].type == ENLNG_VAL_NULL) continue;
            q_val_text(&a[i], buf, sizeof(buf));
            size_t k = strlen(z);
            snprintf(z + k, sizeof(z) - k, "%s", buf);
        }
        return enlng_str(z);
    }
    if (strcmp(f, "least") == 0 || strcmp(f, "greatest") == 0) {
        const EnlngVal* best = NULL;
        for (int i = 0; i < n; i++) {
            if (a[i].type == ENLNG_VAL_NULL) return enlng_null();
            if (!best || (f[0] == 'l' ? enlng_compare_vals(&a[i], best) < 0 : enlng_compare_vals(&a[i], best) > 0)) best = &a[i];
        }
        return best ? enlng_copy_val(best) : enlng_null();
    }
    if (strcmp(f, "if") == 0 || strcmp(f, "iif") == 0) {
        if (n != 3) { q_run_error(R, "%s needs 3 values: if(condition, then, else)", f, ""); return enlng_null(); }
        return enlng_copy_val(q_truth(&a[0]) ? &a[1] : &a[2]);
    }
    if (strcmp(f, "now") == 0 || strcmp(f, "today") == 0 || strcmp(f, "current_date") == 0 || strcmp(f, "current_time") == 0) {
        time_t t = time(NULL);
        struct tm* tmv = localtime(&t);
        if (strcmp(f, "now") == 0) strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tmv);
        else if (strcmp(f, "current_time") == 0) strftime(buf, sizeof(buf), "%H:%M:%S", tmv);
        else strftime(buf, sizeof(buf), "%Y-%m-%d", tmv);
        return enlng_str(buf);
    }
    /* the rest return null when a value is null */
    for (int i = 0; i < n; i++) if (a[i].type == ENLNG_VAL_NULL && strcmp(f, "nullif") != 0) return enlng_null();

    if (strcmp(f, "nullif") == 0) {
        if (n != 2) { q_run_error(R, "nullif needs 2 values%s", "", ""); return enlng_null(); }
        return q_vals_equal(&a[0], &a[1]) ? enlng_null() : enlng_copy_val(&a[0]);
    }
    if (strcmp(f, "upper") == 0 || strcmp(f, "lower") == 0 || strcmp(f, "trim") == 0 || strcmp(f, "ltrim") == 0 ||
        strcmp(f, "rtrim") == 0 || strcmp(f, "reverse") == 0 || strcmp(f, "length") == 0 || strcmp(f, "len") == 0) {
        if (n != 1) { q_run_error(R, "%s needs 1 value", f, ""); return enlng_null(); }
        q_val_text(&a[0], buf, sizeof(buf));
        if (f[0] == 'l' && (f[1] == 'e')) return enlng_int((int64_t)strlen(buf));
        if (strcmp(f, "upper") == 0) { for (char* c = buf; *c; c++) *c = (char)toupper((unsigned char)*c); }
        else if (strcmp(f, "lower") == 0) { for (char* c = buf; *c; c++) *c = (char)tolower((unsigned char)*c); }
        else if (strcmp(f, "reverse") == 0) { size_t l = strlen(buf); for (size_t i = 0; i < l / 2; i++) { char t = buf[i]; buf[i] = buf[l - 1 - i]; buf[l - 1 - i] = t; } }
        else {
            char* s = buf;
            if (f[0] != 'r') while (isspace((unsigned char)*s)) s++;
            if (f[0] != 'l') { size_t l = strlen(s); while (l > 0 && isspace((unsigned char)s[l - 1])) s[--l] = '\0'; }
            return enlng_str(s);
        }
        return enlng_str(buf);
    }
    if (strcmp(f, "substr") == 0 || strcmp(f, "substring") == 0) {
        if (n < 2 || n > 3) { q_run_error(R, "%s needs: %s(text, start[, length])", f, f); return enlng_null(); }
        char s[1024];
        q_val_text(&a[0], s, sizeof(s));
        double d; bool ii; int64_t start, len = 1 << 30;
        if (!q_num(&a[1], &d, &ii, &start) || (n == 3 && !q_num(&a[2], &d, &ii, &len))) { q_run_error(R, "%s needs numbers for the start and the length%s", f, ""); return enlng_null(); }
        int64_t sl = (int64_t)strlen(s);
        if (start < 1) { len += start - 1; start = 1; }
        if (start > sl || len <= 0) return enlng_str("");
        snprintf(buf, sizeof(buf), "%.*s", (int)(len > sl ? sl : len), s + start - 1);
        return enlng_str(buf);
    }
    if (strcmp(f, "left") == 0 || strcmp(f, "right") == 0) {
        if (n != 2) { q_run_error(R, "%s needs: %s(text, count)", f, f); return enlng_null(); }
        char s[1024];
        q_val_text(&a[0], s, sizeof(s));
        double d; bool ii; int64_t k;
        if (!q_num(&a[1], &d, &ii, &k) || k < 0) { q_run_error(R, "%s needs a count of 0 or more", f, ""); return enlng_null(); }
        int64_t sl = (int64_t)strlen(s);
        if (k > sl) k = sl;
        snprintf(buf, sizeof(buf), "%s", f[0] == 'l' ? s : s + sl - k);
        if (f[0] == 'l') buf[k] = '\0';
        return enlng_str(buf);
    }
    if (strcmp(f, "replace") == 0) {
        if (n != 3) { q_run_error(R, "replace needs: replace(text, old, new)%s", "", ""); return enlng_null(); }
        char s[1024], from[256], to[256], out[2048];
        q_val_text(&a[0], s, sizeof(s));
        q_val_text(&a[1], from, sizeof(from));
        q_val_text(&a[2], to, sizeof(to));
        size_t fl = strlen(from), k = 0;
        out[0] = '\0';
        for (const char* p = s; *p && k < sizeof(out) - 300;) {
            if (fl > 0 && strncmp(p, from, fl) == 0) { k += (size_t)snprintf(out + k, sizeof(out) - k, "%s", to); p += fl; }
            else out[k++] = *p++;
            out[k] = '\0';
        }
        return enlng_str(out);
    }
    if (strcmp(f, "instr") == 0) {
        if (n != 2) { q_run_error(R, "instr needs: instr(text, part)%s", "", ""); return enlng_null(); }
        char s[1024], p[256];
        q_val_text(&a[0], s, sizeof(s));
        q_val_text(&a[1], p, sizeof(p));
        const char* at = strstr(s, p);
        return enlng_int(at ? (int64_t)(at - s) + 1 : 0);
    }
    if (strcmp(f, "year") == 0 || strcmp(f, "month") == 0 || strcmp(f, "day") == 0 || strcmp(f, "date") == 0) {
        q_val_text(&a[0], buf, sizeof(buf));
        int y, m, d;
        if (!q_parse_date(buf, &y, &m, &d)) { q_run_error(R, "'%s' is not a date; write it like 2024-03-15", buf, ""); return enlng_null(); }
        if (f[0] == 'y') return enlng_int(y);
        if (f[0] == 'm') return enlng_int(m);
        if (strcmp(f, "day") == 0) return enlng_int(d);
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y, m, d);
        return enlng_str(buf);
    }
    if (strcmp(f, "datediff") == 0) {
        if (n != 2) { q_run_error(R, "datediff needs 2 dates%s", "", ""); return enlng_null(); }
        char s1[64], s2[64];
        q_val_text(&a[0], s1, sizeof(s1));
        q_val_text(&a[1], s2, sizeof(s2));
        int y1, m1, d1, y2, m2, d2;
        if (!q_parse_date(s1, &y1, &m1, &d1) || !q_parse_date(s2, &y2, &m2, &d2)) { q_run_error(R, "datediff needs dates like 2024-03-15%s", "", ""); return enlng_null(); }
        return enlng_int(q_days(y1, m1, d1) - q_days(y2, m2, d2));
    }
    /* numbers */
    double d0 = 0, d1 = 0;
    bool i0 = false, i1 = false;
    int64_t l0 = 0, l1 = 0;
    bool known = strcmp(f, "abs") == 0 || strcmp(f, "round") == 0 || strcmp(f, "floor") == 0 || strcmp(f, "ceil") == 0 ||
                 strcmp(f, "ceiling") == 0 || strcmp(f, "sqrt") == 0 || strcmp(f, "power") == 0 || strcmp(f, "pow") == 0 ||
                 strcmp(f, "mod") == 0 || strcmp(f, "sign") == 0;
    if (!known) {
        q_run_error(R, "I don't know a function called '%s'. Functions: upper lower length trim substr replace concat coalesce nullif abs round floor ceil sqrt power mod sign least greatest left right instr reverse year month day date datediff now today cast if", f, "");
        return enlng_null();
    }
    if (n < 1 || !q_num(&a[0], &d0, &i0, &l0)) { q_run_error(R, "%s needs a number%s", f, ""); return enlng_null(); }
    if (n >= 2 && !q_num(&a[1], &d1, &i1, &l1)) { q_run_error(R, "%s needs a number as its second value%s", f, ""); return enlng_null(); }
    if (strcmp(f, "abs") == 0) return i0 ? enlng_int(l0 < 0 ? -l0 : l0) : enlng_double(fabs(d0));
    if (strcmp(f, "sign") == 0) return enlng_int(d0 > 0 ? 1 : d0 < 0 ? -1 : 0);
    if (strcmp(f, "floor") == 0) return enlng_int((int64_t)floor(d0));
    if (strcmp(f, "ceil") == 0 || strcmp(f, "ceiling") == 0) return enlng_int((int64_t)ceil(d0));
    if (strcmp(f, "sqrt") == 0) {
        if (d0 < 0) { q_run_error(R, "sqrt of a negative number%s", "", ""); return enlng_null(); }
        return enlng_double(sqrt(d0));
    }
    if (strcmp(f, "power") == 0 || strcmp(f, "pow") == 0) return enlng_double(pow(d0, n >= 2 ? d1 : 2));
    if (strcmp(f, "mod") == 0) {
        if (n < 2 || d1 == 0) { q_run_error(R, "mod needs two numbers and the second cannot be 0%s", "", ""); return enlng_null(); }
        return (i0 && i1) ? enlng_int(l0 % l1) : enlng_double(fmod(d0, d1));
    }
    /* round(x[, digits]) */
    int digits = n >= 2 ? (int)l1 : 0;
    double scale = pow(10, digits);
    double r = floor(d0 * scale + 0.5) / scale;
    if (n < 2) return enlng_int((int64_t)r);
    return enlng_double(r);
}

/* ---- aggregates ---- */
static EnlngVal q_fold(QAgg agg, EnlngVal* vals, size_t n, bool star, bool distinct, const char* sep) {
    if (agg == QA_COUNT && star) return enlng_int((int64_t)n);
    /* distinct: keep the first of each value */
    bool* skip = NULL;
    if (distinct) {
        skip = (bool*)calloc(n + 1, sizeof(bool));
        for (size_t i = 0; i < n; i++) {
            if (vals[i].type == ENLNG_VAL_NULL) { skip[i] = true; continue; }
            for (size_t j = 0; j < i && !skip[i]; j++) if (!skip[j] && q_vals_equal(&vals[j], &vals[i])) skip[i] = true;
        }
    }
    EnlngVal result = enlng_null();
    if (agg == QA_COUNT) {
        int64_t k = 0;
        for (size_t i = 0; i < n; i++) if (vals[i].type != ENLNG_VAL_NULL && !(skip && skip[i])) k++;
        result = enlng_int(k);
    } else if (agg == QA_MIN || agg == QA_MAX) {
        const EnlngVal* best = NULL;
        for (size_t i = 0; i < n; i++) {
            if (vals[i].type == ENLNG_VAL_NULL) continue;
            int cmp = best ? enlng_compare_vals(&vals[i], best) : 0;
            if (!best || (agg == QA_MIN ? cmp < 0 : cmp > 0)) best = &vals[i];
        }
        if (best) result = enlng_copy_val(best);
    } else if (agg == QA_CONCAT) {
        size_t cap = 256, k = 0;
        char* out = (char*)malloc(cap);
        out[0] = '\0';
        bool first = true;
        for (size_t i = 0; i < n; i++) {
            if (vals[i].type == ENLNG_VAL_NULL || (skip && skip[i])) continue;
            char t[512];
            q_val_text(&vals[i], t, sizeof(t));
            size_t need = k + strlen(t) + strlen(sep) + 2;
            if (need > cap) { cap = need * 2; out = (char*)realloc(out, cap); }
            if (!first) k += (size_t)snprintf(out + k, cap - k, "%s", sep);
            k += (size_t)snprintf(out + k, cap - k, "%s", t);
            first = false;
        }
        result = first ? enlng_null() : enlng_str(out);
        free(out);
    } else {
        /* sum / average of the numbers */
        int64_t isum = 0;
        double dsum = 0;
        bool any_double = false;
        size_t k = 0;
        for (size_t i = 0; i < n; i++) {
            if (skip && skip[i]) continue;
            const EnlngVal* v = &vals[i];
            if (v->type == ENLNG_VAL_INT) { isum += v->int_val; dsum += (double)v->int_val; k++; }
            else if (v->type == ENLNG_VAL_DOUBLE) { dsum += v->double_val; any_double = true; k++; }
        }
        if (k > 0) {
            if (agg == QA_AVG) result = enlng_double(dsum / (double)k);
            else result = any_double ? enlng_double(dsum) : enlng_int(isum);
        }
    }
    free(skip);
    return result;
}

static EnlngVal q_eval_agg(const QEnv* E, QExpr* e) {
    if (!E->has_grp) {
        q_run_error(E->R, "%s() can only be used where rows are totalled: in the select list, 'having' or 'order by' (not in 'where')", e->text, "");
        return enlng_null();
    }
    size_t n = E->ngrp;
    EnlngVal* vals = (EnlngVal*)malloc((n ? n : 1) * sizeof(EnlngVal));
    char sep[64] = ",";
    for (size_t i = 0; i < n; i++) {
        vals[i] = enlng_null();
        if (e->star || e->nargs == 0) continue;
        QEnv m = *E;
        m.row = E->grp[i];
        m.has_grp = false;
        vals[i] = q_eval(&m, e->args[0]);
    }
    if (e->agg == QA_CONCAT && e->nargs >= 2) {
        QEnv m = *E;
        m.has_grp = false;
        EnlngVal sv = q_eval(&m, e->args[1]);
        q_val_text(&sv, sep, sizeof(sep));
        enlng_free_val(&sv);
    }
    if (!e->star && e->nargs == 0) {
        q_run_error(E->R, "%s( ) needs a column name, like %s(marks)", e->text, e->text);
        free(vals);
        return enlng_null();
    }
    EnlngVal r = q_fold(e->agg, vals, n, e->star, e->distinct, sep);
    for (size_t i = 0; i < n; i++) enlng_free_val(&vals[i]);
    free(vals);
    return r;
}

/* a subquery, run for the row it is in. One that never looks at that row runs once. */
static QSet* q_run_sub(const QEnv* E, QExpr* e) {
    if (e->cache) return e->cache;
    QSet* out = qs_new();
    int hits = E->R->outer_hits;
    if (!q_exec(E->R, e->sub, E, out)) {
        q_set_free(out);
        if (!E->R->err[0]) snprintf(E->R->err, sizeof(E->R->err), "the subquery failed");
        return NULL;
    }
    if (E->R->outer_hits == hits) {
        e->cache = out;         /* it did not use the outer row: the answer is the same for every row */
    }
    return out;
}

static void q_done_sub(QExpr* e, QSet* s) {
    if (s && s != e->cache) q_set_free(s);
}

static EnlngVal q_eval(const QEnv* E, QExpr* e) {
    QRun* R = E->R;
    if (R->err[0]) return enlng_null();
    switch (e->kind) {
        case EK_LIT:
            return enlng_copy_val(&e->val);
        case EK_NAME: {
            int status = -1;
            const EnlngVal* v = (e->mode == NM_SOFT_LIT && !strchr(e->text, '.')) ? NULL : q_lookup(E, e->text, &status);
            if (v) return enlng_copy_val(v);
            if (e->mode == NM_COL) {
                if (status == -2) q_run_error(R, "more than one table has a column '%s'; write it as table.%s", e->text, e->text);
                else if (E->set) { char m[300]; q_col_error(E->set, e->text, status, m, sizeof(m)); q_run_error(R, "%s%s", m, ""); }
                else q_run_error(R, "there is no column '%s' here", e->text, "");
                return enlng_null();
            }
            return enlng_copy_val(&e->val);
        }
        case EK_NEG: {
            EnlngVal v = q_eval(E, e->a);
            double d; bool ii; int64_t l;
            if (v.type == ENLNG_VAL_NULL) return v;
            if (!q_num(&v, &d, &ii, &l)) { enlng_free_val(&v); q_run_error(R, "a minus sign needs a number%s", "", ""); return enlng_null(); }
            enlng_free_val(&v);
            return ii ? enlng_int(-l) : enlng_double(-d);
        }
        case EK_NOT: {
            EnlngVal v = q_eval(E, e->a);
            bool null = v.type == ENLNG_VAL_NULL;
            bool t = q_truth(&v);
            enlng_free_val(&v);
            return null ? enlng_bool(false) : enlng_bool(!t);
        }
        case EK_BIN: {
            EnlngVal a = q_eval(E, e->a), b = q_eval(E, e->b);
            EnlngVal r = q_arith(R, e->op, &a, &b);
            enlng_free_val(&a);
            enlng_free_val(&b);
            return r;
        }
        case EK_CMP: {
            EnlngVal a = q_eval(E, e->a), b = q_eval(E, e->b);
            bool unknown = a.type == ENLNG_VAL_NULL || b.type == ENLNG_VAL_NULL;
            bool r = !unknown && enlng_eval_condition(&a, (EnlngOp)e->op, &b);
            enlng_free_val(&a);
            enlng_free_val(&b);
            return unknown ? enlng_null() : enlng_bool(r);
        }
        case EK_AND: {
            EnlngVal a = q_eval(E, e->a);
            bool t = q_truth(&a);
            enlng_free_val(&a);
            if (!t) return enlng_bool(false);
            EnlngVal b = q_eval(E, e->b);
            t = q_truth(&b);
            enlng_free_val(&b);
            return enlng_bool(t);
        }
        case EK_OR: {
            EnlngVal a = q_eval(E, e->a);
            bool t = q_truth(&a);
            enlng_free_val(&a);
            if (t) return enlng_bool(true);
            EnlngVal b = q_eval(E, e->b);
            t = q_truth(&b);
            enlng_free_val(&b);
            return enlng_bool(t);
        }
        case EK_ISNULL: {
            EnlngVal a = q_eval(E, e->a);
            bool null = a.type == ENLNG_VAL_NULL;
            enlng_free_val(&a);
            return enlng_bool(e->neg ? !null : null);
        }
        case EK_BETWEEN: {
            EnlngVal a = q_eval(E, e->a), lo = q_eval(E, e->b), hi = q_eval(E, e->c);
            bool r = false;
            bool unknown = a.type == ENLNG_VAL_NULL || lo.type == ENLNG_VAL_NULL || hi.type == ENLNG_VAL_NULL;
            if (!unknown) {
                r = enlng_compare_vals(&a, &lo) >= 0 && enlng_compare_vals(&a, &hi) <= 0;
                if (e->neg) r = !r;
            }
            enlng_free_val(&a);
            enlng_free_val(&lo);
            enlng_free_val(&hi);
            return unknown ? enlng_null() : enlng_bool(r);
        }
        case EK_LIKE: {
            EnlngVal a = q_eval(E, e->a), p = q_eval(E, e->b);
            bool r = false;
            bool unknown = a.type == ENLNG_VAL_NULL || p.type == ENLNG_VAL_NULL;
            if (!unknown) {
                char at[512], pt[512];
                q_val_text(&a, at, sizeof(at));
                q_val_text(&p, pt, sizeof(pt));
                EnlngVal as = enlng_str(at), ps = enlng_str(pt);
                r = enlng_eval_condition(&as, OP_LIKE, &ps);
                enlng_free_val(&as);
                enlng_free_val(&ps);
                if (e->neg) r = !r;
            }
            enlng_free_val(&a);
            enlng_free_val(&p);
            return unknown ? enlng_null() : enlng_bool(r);
        }
        case EK_INLIST: {
            EnlngVal a = q_eval(E, e->a);
            bool found = false;
            if (a.type != ENLNG_VAL_NULL) {
                for (int i = 0; i < e->nargs && !found; i++) {
                    EnlngVal v = q_eval(E, e->args[i]);
                    if (v.type != ENLNG_VAL_NULL && enlng_eval_condition(&a, OP_EQ, &v)) found = true;
                    enlng_free_val(&v);
                }
            }
            bool null = a.type == ENLNG_VAL_NULL;
            enlng_free_val(&a);
            return null ? enlng_null() : enlng_bool(e->neg ? !found : found);
        }
        case EK_INSUB: {
            EnlngVal a = q_eval(E, e->a);
            bool found = false;
            QSet* s = a.type == ENLNG_VAL_NULL ? NULL : q_run_sub(E, e);
            if (s) {
                if (s->ncols != 1) q_run_error(R, "a subquery after 'in' must give one column, not %s", s->ncols ? "several" : "none", "");
                for (size_t r = 0; r < s->nrows && s->ncols == 1 && !found; r++) {
                    if (s->rows[r][0].type != ENLNG_VAL_NULL && enlng_eval_condition(&a, OP_EQ, &s->rows[r][0])) found = true;
                }
                q_done_sub(e, s);
            }
            bool null = a.type == ENLNG_VAL_NULL;
            enlng_free_val(&a);
            return null ? enlng_null() : enlng_bool(e->neg ? !found : found);
        }
        case EK_EXISTS: {
            QSet* s = q_run_sub(E, e);
            bool any = s && s->nrows > 0;
            q_done_sub(e, s);
            return enlng_bool(any);
        }
        case EK_SCALAR: {
            QSet* s = q_run_sub(E, e);
            EnlngVal r = enlng_null();
            if (s) {
                if (s->ncols != 1) q_run_error(R, "a subquery used as a value must give one column, not %s", s->ncols ? "several" : "none", "");
                else if (s->nrows > 1) q_run_error(R, "a subquery used as a value gave more than one row (%s); add 'limit 1' or use 'in'", s->nrows > 9 ? "many" : "2 or more", "");
                else if (s->nrows == 1) r = enlng_copy_val(&s->rows[0][0]);
                q_done_sub(e, s);
            }
            return r;
        }
        case EK_CASE: {
            EnlngVal base = enlng_null();
            if (e->a) base = q_eval(E, e->a);
            for (int i = 0; i + 1 < e->nargs; i += 2) {
                EnlngVal w = q_eval(E, e->args[i]);
                bool hit = e->a ? (base.type != ENLNG_VAL_NULL && w.type != ENLNG_VAL_NULL && enlng_eval_condition(&base, OP_EQ, &w)) : q_truth(&w);
                enlng_free_val(&w);
                if (hit) {
                    enlng_free_val(&base);
                    return q_eval(E, e->args[i + 1]);
                }
            }
            enlng_free_val(&base);
            return e->b ? q_eval(E, e->b) : enlng_null();
        }
        case EK_AGG:
            return q_eval_agg(E, e);
        case EK_WIN:
            if (!e->wval || E->unit >= e->wn) {
                q_run_error(R, "%s(...) over (...) can only be used in the select list or 'order by'", e->text, "");
                return enlng_null();
            }
            return enlng_copy_val(&e->wval[E->unit]);
        case EK_FUNC: {
            EnlngVal args[Q_MAX_ARGS];
            for (int i = 0; i < e->nargs; i++) args[i] = q_eval(E, e->args[i]);
            EnlngVal r = R->err[0] ? enlng_null() : q_func(R, e, args, e->nargs);
            for (int i = 0; i < e->nargs; i++) enlng_free_val(&args[i]);
            return r;
        }
    }
    return enlng_null();
}

/* does the expression hold an aggregate (outside a window function)? */
static bool q_has_agg(const QExpr* e) {
    if (!e) return false;
    if (e->kind == EK_AGG) return true;
    if (e->kind == EK_WIN) return false;
    if (q_has_agg(e->a) || q_has_agg(e->b) || q_has_agg(e->c)) return true;
    for (int i = 0; i < e->nargs; i++) if (q_has_agg(e->args[i])) return true;
    return false;
}

/* check the names in an expression against the columns, before any row is read */
static bool q_check(QRun* R, QExpr* e, const QSet* set, const QEnv* outer, bool allow_agg, bool allow_win, char* err, size_t en) {
    if (!e) return true;
    if (e->kind == EK_NAME && e->mode == NM_COL) {
        int ci = q_find_col(set, e->text);
        if (ci >= 0) return true;
        if (ci == -1) {
            for (const QEnv* o = outer; o; o = o->outer) {
                if (o->set && q_find_col(o->set, e->text) >= 0) return true;
            }
        }
        q_col_error(set, e->text, ci, err, en);
        return false;
    }
    if (e->kind == EK_AGG) {
        if (!allow_agg) {
            snprintf(err, en, "%s() can only be used where rows are totalled: in the select list, 'having' or 'order by' (not in 'where')", e->text);
            return false;
        }
        for (int i = 0; i < e->nargs; i++) {
            if (q_has_agg(e->args[i])) { snprintf(err, en, "%s() cannot hold another aggregate", e->text); return false; }
            if (!q_check(R, e->args[i], set, outer, false, false, err, en)) return false;
        }
        return true;
    }
    if (e->kind == EK_WIN) {
        if (!allow_win) {
            snprintf(err, en, "%s(...) over (...) can only be used in the select list or 'order by'", e->text);
            return false;
        }
        for (int i = 0; i < e->nargs; i++) if (!q_check(R, e->args[i], set, outer, allow_agg, false, err, en)) return false;
        for (int i = 0; i < e->npart; i++) if (!q_check(R, e->part[i], set, outer, allow_agg, false, err, en)) return false;
        for (int i = 0; i < e->nord; i++) if (!q_check(R, e->ord[i], set, outer, allow_agg, false, err, en)) return false;
        return true;
    }
    if (!q_check(R, e->a, set, outer, allow_agg, allow_win, err, en)) return false;
    if (!q_check(R, e->b, set, outer, allow_agg, allow_win, err, en)) return false;
    if (!q_check(R, e->c, set, outer, allow_agg, allow_win, err, en)) return false;
    for (int i = 0; i < e->nargs; i++) if (!q_check(R, e->args[i], set, outer, allow_agg, allow_win, err, en)) return false;
    return true;
}

/* ---- Running a query ---- */

static bool q_missing(QRun* R, const char* name) {
    if (R->print) print_missing_table(R->db, name);
    R->err_printed = true;
    snprintf(R->err, sizeof(R->err), "Table '%s' does not exist", name);
    return false;
}

static void q_normalize_rows(EnlngTable* tbl) {
    for (size_t r = 0; r < tbl->row_count; r++) {
        EnlngRow* row = &tbl->rows[r];
        if (row->cell_count >= tbl->col_count) continue;
        row->cells = (EnlngVal*)realloc(row->cells, (size_t)tbl->col_count * sizeof(EnlngVal));
        for (int c = row->cell_count; c < tbl->col_count; c++) row->cells[c] = enlng_null();
        row->cell_count = tbl->col_count;
    }
}

static bool q_load_table(QSet* S, EnlngTable* tbl, const char* alias) {
    if (S->ncols + tbl->col_count > Q_MAX_COLS) return false;
    q_normalize_rows(tbl);
    for (int c = 0; c < tbl->col_count; c++) snprintf(S->names[S->ncols + c], sizeof(S->names[0]), "%s.%s", alias, tbl->columns[c].name);
    S->ncols += tbl->col_count;
    for (size_t r = 0; r < tbl->row_count; r++) {
        EnlngVal* row = q_row_new(S->ncols);
        for (int c = 0; c < tbl->col_count; c++) row[c] = enlng_copy_val(&tbl->rows[r].cells[c]);
        q_set_add(S, row);
    }
    return true;
}

/* the column named by a result title: student.name is name, sum(x) stays */
static void q_title_name(const char* title, char* out, size_t n) {
    const char* dot = strchr(title, '.');
    bool plain = dot != NULL;
    for (const char* c = title; *c && plain; c++) if (!(isalnum((unsigned char)*c) || *c == '_' || *c == '.')) plain = false;
    snprintf(out, n, "%s", plain ? dot + 1 : title);
}

/* take the rows of a finished query, as a source called alias */
static void q_adopt(QSet* dst, QSet* res, const char* alias) {
    dst->ncols = res->ncols;
    for (int c = 0; c < res->ncols; c++) {
        char nm[130];
        q_title_name(res->names[c], nm, sizeof(nm));
        snprintf(dst->names[c], sizeof(dst->names[0]), "%s.%s", alias, nm);
    }
    dst->rows = res->rows;
    dst->nrows = res->nrows;
    dst->cap = res->cap;
    res->rows = NULL;
    res->nrows = res->cap = 0;
}

static bool q_exec_text(QRun* R, const char* sql, const QEnv* outer, QSet* out);

/* a table, a view, a common table from 'with', or a bracketed query, as the rows of one source */
static bool q_load_source(QRun* R, QSrc* s, const QEnv* outer, QSet* dst) {
    if (s->sub) {
        QSet* res = qs_new();
        if (!q_exec(R, s->sub, outer, res)) { q_set_free(res); return false; }
        q_adopt(dst, res, s->alias);
        q_set_free(res);
        return true;
    }
    for (int i = R->nctes - 1; i >= 0; i--) {
        if (str_ci_cmp(R->ctes[i].name, s->table) != 0) continue;
        int saved = R->nctes;
        R->nctes = i;           /* a common table sees only the ones before it */
        QSet* res = qs_new();
        bool ok = q_exec(R, R->ctes[i].q, outer, res);
        R->nctes = saved;
        if (ok) q_adopt(dst, res, s->alias);
        q_set_free(res);
        return ok;
    }
    EnlngTable* tbl = enlngdb_get_table(R->db, s->table);
    if (!tbl) return q_missing(R, s->table);
    if (tbl->view_sql[0]) {
        if (R->depth > 8) { snprintf(R->err, sizeof(R->err), "views refer to each other in a circle (at '%s')", s->table); return false; }
        QSet* res = qs_new();
        R->depth++;
        int saved = R->nctes;
        R->nctes = 0;
        bool ok = q_exec_text(R, tbl->view_sql, NULL, res);
        R->nctes = saved;
        R->depth--;
        if (ok) q_adopt(dst, res, s->alias);
        q_set_free(res);
        return ok;
    }
    if (!q_load_table(dst, tbl, s->alias)) {
        snprintf(R->err, sizeof(R->err), "the joined tables have more than %d columns", Q_MAX_COLS);
        return false;
    }
    return true;
}

/* col is "<table>_id" (a table called students also has student_id) */
static bool q_key_for(const char* table, const char* col) {
    char want[140];
    snprintf(want, sizeof(want), "%s_id", table);
    if (str_ci_cmp(col, want) == 0) return true;
    size_t n = strlen(table);
    if (n > 1 && tolower((unsigned char)table[n - 1]) == 's') {
        snprintf(want, sizeof(want), "%.*s_id", (int)(n - 1), table);
        return str_ci_cmp(col, want) == 0;
    }
    return false;
}

/* a name that is a key: roll_no, user_id, zip_code, api_key. 'name' and 'title' are not */
static bool q_key_like(const char* col) {
    static const char* ends[] = {"_id", "_no", "_code", "_key", NULL};
    size_t n = strlen(col);
    for (int i = 0; ends[i]; i++) {
        size_t e = strlen(ends[i]);
        if (n > e && str_ci_cmp(col + n - e, ends[i]) == 0) return true;
    }
    return false;
}

typedef struct { char l[130], r[130]; } QPair;

/* The columns that can be matched when a join has no 'on': a table's id with the other's <table>_id,
 * or the same key-like name in both. Returns how many ways there are. */
static int q_guess_on(const QSet* S, const char reals[][64], const char aliases[][64], int nprev,
                      const QSet* B, const char* b_alias, const char* b_real, QPair* out, int max) {
    int n = 0;
    for (int c = 0; c < S->ncols; c++) {
        const char* full = S->names[c];
        const char* dot = strchr(full, '.');
        char tbl[64];
        snprintf(tbl, sizeof(tbl), "%.*s", dot ? (int)(dot - full) : 0, full);
        const char* col = dot ? dot + 1 : full;
        const char* real = tbl;
        for (int i = 0; i < nprev; i++) if (str_ci_cmp(aliases[i], tbl) == 0) real = reals[i];
        for (int d = 0; d < B->ncols; d++) {
            const char* col2 = q_short(B->names[d]);
            bool hit = (str_ci_cmp(col, "id") == 0 && q_key_for(real, col2)) ||
                       (str_ci_cmp(col2, "id") == 0 && q_key_for(b_real, col)) ||
                       (str_ci_cmp(col, col2) == 0 && q_key_like(col));
            if (!hit) continue;
            if (n < max) {
                snprintf(out[n].l, sizeof(out[n].l), "%s", full);
                snprintf(out[n].r, sizeof(out[n].r), "%s.%s", b_alias, col2);
            }
            n++;
        }
    }
    return n;
}

static void q_guess_error(char* err, size_t n, const QSet* S, const char* b_name, const QPair* pairs, int count) {
    if (count == 0) {
        snprintf(err, n, "I can't tell which columns of %s match the tables before it, so I will not guess. "
                         "Write it with 'on', like: on %s is %s.column",
                 b_name, S->ncols ? S->names[0] : "a_column", b_name);
        return;
    }
    size_t k = (size_t)snprintf(err, n, "there is more than one way to match %s, so I will not guess:", b_name);
    for (int i = 0; i < count && i < 3 && k < n; i++) k += (size_t)snprintf(err + k, n - k, " %s is %s;", pairs[i].l, pairs[i].r);
    if (k < n) snprintf(err + k, n - k, " pick one with 'on', like: on %s is %s", pairs[0].l, pairs[0].r);
}

static QExpr* q_eq_names(const char* l, const char* r) {
    QExpr* e = qe_new(EK_CMP);
    e->op = OP_EQ;
    e->a = qe_new(EK_NAME);
    snprintf(e->a->text, sizeof(e->a->text), "%s", l);
    e->b = qe_new(EK_NAME);
    snprintf(e->b->text, sizeof(e->b->text), "%s", r);
    return e;
}

typedef struct {
    QExpr* e;
    char title[130];
    bool owned;
} QItem;

typedef struct {
    EnlngVal* row;
    EnlngVal** mem;
    size_t nmem;
} QUnit;

typedef struct {
    int n;
    bool desc[Q_MAX_ORD];
    EnlngVal* keys;             /* keys[row * n + k] */
} QSortCtx;

static int q_cmp_rows(const QSortCtx* c, size_t a, size_t b) {
    for (int k = 0; k < c->n; k++) {
        int cmp = enlng_compare_vals(&c->keys[a * c->n + k], &c->keys[b * c->n + k]);
        if (cmp != 0) return c->desc[k] ? -cmp : cmp;
    }
    return 0;
}

/* a stable merge sort of the row numbers */
static void q_sort_idx(size_t* idx, size_t n, const QSortCtx* c) {
    if (n < 2) return;
    size_t* tmp = (size_t*)malloc(n * sizeof(size_t));
    for (size_t width = 1; width < n; width *= 2) {
        for (size_t lo = 0; lo < n; lo += 2 * width) {
            size_t mid = lo + width < n ? lo + width : n, hi = lo + 2 * width < n ? lo + 2 * width : n;
            size_t i = lo, j = mid, k = lo;
            while (i < mid && j < hi) tmp[k++] = q_cmp_rows(c, idx[i], idx[j]) <= 0 ? idx[i++] : idx[j++];
            while (i < mid) tmp[k++] = idx[i++];
            while (j < hi) tmp[k++] = idx[j++];
        }
        memcpy(idx, tmp, n * sizeof(size_t));
    }
    free(tmp);
}

static bool q_rows_same(const EnlngVal* a, const EnlngVal* b, int n) {
    for (int c = 0; c < n; c++) {
        if (a[c].type != b[c].type || enlng_compare_vals(&a[c], &b[c]) != 0) return false;
        if (a[c].type == ENLNG_VAL_STRING && strcmp(a[c].str_val ? a[c].str_val : "", b[c].str_val ? b[c].str_val : "") != 0) return false;
    }
    return true;
}

static void q_dedupe(QSet* O) {
    size_t keep = 0;
    for (size_t r = 0; r < O->nrows; r++) {
        bool seen = false;
        for (size_t k = 0; k < keep && !seen; k++) seen = q_rows_same(O->rows[k], O->rows[r], O->ncols);
        if (seen) {
            for (int c = 0; c < O->ncols; c++) enlng_free_val(&O->rows[r][c]);
            free(O->rows[r]);
        } else {
            O->rows[keep++] = O->rows[r];
        }
    }
    O->nrows = keep;
}

/* ---- window functions ---- */
static void q_collect_wins(QExpr* e, QExpr** wins, int* n, int max) {
    if (!e) return;
    if (e->kind == EK_WIN) {
        if (*n < max) wins[(*n)++] = e;
        return;
    }
    q_collect_wins(e->a, wins, n, max);
    q_collect_wins(e->b, wins, n, max);
    q_collect_wins(e->c, wins, n, max);
    for (int i = 0; i < e->nargs; i++) q_collect_wins(e->args[i], wins, n, max);
}

static bool q_compute_window(QRun* R, QExpr* w, const QSet* S, QUnit* units, size_t U, bool grouped, const QEnv* outer) {
    int np = w->npart, no = w->nord;
    EnlngVal* pk = (EnlngVal*)calloc(U * (np ? np : 1) + 1, sizeof(EnlngVal));
    QSortCtx ctx;
    ctx.n = no;
    ctx.keys = (EnlngVal*)calloc(U * (no ? no : 1) + 1, sizeof(EnlngVal));
    for (int k = 0; k < no; k++) ctx.desc[k] = w->ord_desc[k];
    EnlngVal* arg = (EnlngVal*)calloc(U + 1, sizeof(EnlngVal));
    for (size_t i = 0; i < U; i++) {
        QEnv E = {S, units[i].row, units[i].mem, units[i].nmem, grouped, i, outer, R};
        for (int k = 0; k < np; k++) pk[i * np + k] = q_eval(&E, w->part[k]);
        for (int k = 0; k < no; k++) ctx.keys[i * no + k] = q_eval(&E, w->ord[k]);
        arg[i] = enlng_null();
        if (w->nargs > 0 && !w->star) arg[i] = q_eval(&E, w->args[0]);
    }
    if (w->wval) { for (size_t i = 0; i < w->wn; i++) enlng_free_val(&w->wval[i]); free(w->wval); }
    w->wval = (EnlngVal*)calloc(U + 1, sizeof(EnlngVal));
    w->wn = U;
    size_t* rn = (size_t*)calloc(U + 1, sizeof(size_t));

#define SAME_PART(i, j) q_rows_same(&pk[(i) * np], &pk[(j) * np], np)
    /* the row number inside the partition, in the order asked for */
    for (size_t i = 0; i < U; i++) {
        size_t before = 0;
        for (size_t j = 0; j < U; j++) {
            if (np && !SAME_PART(i, j)) continue;
            int c = no ? q_cmp_rows(&ctx, j, i) : (j < i ? -1 : j > i ? 1 : 0);
            if (c < 0 || (c == 0 && j < i)) before++;
        }
        rn[i] = before + 1;
    }
    for (size_t i = 0; i < U && !R->err[0]; i++) {
        EnlngVal r = enlng_null();
        if (w->win == QW_ROW_NUMBER) {
            r = enlng_int((int64_t)rn[i]);
        } else if (w->win == QW_RANK || w->win == QW_DENSE_RANK) {
            int64_t rank = 1;
            for (size_t j = 0; j < U; j++) {
                if (np && !SAME_PART(i, j)) continue;
                if (q_cmp_rows(&ctx, j, i) >= 0) continue;
                if (w->win == QW_RANK) { rank++; continue; }
                bool first_of_group = true;
                for (size_t k = 0; k < j && first_of_group; k++) {
                    if ((!np || SAME_PART(i, k)) && q_cmp_rows(&ctx, k, j) == 0) first_of_group = false;
                }
                if (first_of_group) rank++;
            }
            r = enlng_int(rank);
        } else if (w->win == QW_LAG || w->win == QW_LEAD) {
            int64_t off = 1;
            QEnv E = {S, units[i].row, units[i].mem, units[i].nmem, grouped, i, outer, R};
            if (w->nargs >= 2) {
                EnlngVal o = q_eval(&E, w->args[1]);
                double d; bool ii;
                if (!q_num(&o, &d, &ii, &off)) q_run_error(R, "%s needs a number of rows as its second value", w->text, "");
                enlng_free_val(&o);
            }
            int64_t target = (int64_t)rn[i] + (w->win == QW_LAG ? -off : off);
            bool found = false;
            for (size_t j = 0; j < U && !found; j++) {
                if ((!np || SAME_PART(i, j)) && (int64_t)rn[j] == target) { r = enlng_copy_val(&arg[j]); found = true; }
            }
            if (!found && w->nargs >= 3) r = q_eval(&E, w->args[2]);
        } else {
            /* an aggregate over the rows of the partition (up to this row, when there is an order) */
            size_t m = 0;
            EnlngVal* vals = (EnlngVal*)malloc((U + 1) * sizeof(EnlngVal));
            for (size_t j = 0; j < U; j++) {
                if (np && !SAME_PART(i, j)) continue;
                if (no && q_cmp_rows(&ctx, j, i) > 0) continue;
                vals[m++] = arg[j];
            }
            r = q_fold(w->agg, vals, m, w->star, w->distinct, ",");
            free(vals);
        }
        w->wval[i] = r;
    }
#undef SAME_PART
    for (size_t i = 0; i < U * (size_t)np; i++) enlng_free_val(&pk[i]);
    for (size_t i = 0; i < U * (size_t)no; i++) enlng_free_val(&ctx.keys[i]);
    for (size_t i = 0; i < U; i++) enlng_free_val(&arg[i]);
    free(pk);
    free(ctx.keys);
    free(arg);
    free(rn);
    return !R->err[0];
}

/* a group by / order by item that is a number or a name from the select list means that select item */
static QExpr* q_resolve_item(QExpr* e, QItem* items, int nitems, const QSet* S, bool prefer_alias, int* ordinal_err) {
    if (e->kind == EK_LIT && e->val.type == ENLNG_VAL_INT) {
        int64_t k = e->val.int_val;
        if (k < 1 || k > nitems) { *ordinal_err = (int)k; return NULL; }
        return items[k - 1].e;
    }
    if (e->kind == EK_NAME && e->mode == NM_COL && !strchr(e->text, '.')) {
        bool is_column = S && q_find_col(S, e->text) != -1;
        if (prefer_alias || !is_column) {
            for (int i = 0; i < nitems; i++) {
                if (str_ci_cmp(items[i].title, e->text) == 0 && items[i].e != e) {
                    /* a plain column with the same name resolves the same either way */
                    return items[i].e;
                }
            }
        }
    }
    return e;
}

static bool q_exec_one(QRun* R, QQuery* q, const QEnv* outer, QSet* O) {
    bool ok = false;
    QSet* S = qs_new();
    QItem items[Q_MAX_COLS];
    int nitems = 0;
    QUnit* units = NULL;
    size_t nunits = 0;
    EnlngVal* nullrow = NULL;
    EnlngVal* keys = NULL;
    char reals[Q_MAX_SRC][64], aliases[Q_MAX_SRC][64];
    QExpr* owned_on[Q_MAX_SRC] = {0};
    char err[300] = "";
    memset(items, 0, sizeof(items));

    if (!q->has_from) q_set_add(S, q_row_new(0));       /* select 1 + 1: one row with no columns */
    if (q->has_from) {
        if (!q_load_source(R, &q->src[0], outer, S)) goto done;
        snprintf(reals[0], sizeof(reals[0]), "%s", q->src[0].table[0] ? q->src[0].table : q->src[0].alias);
        snprintf(aliases[0], sizeof(aliases[0]), "%s", q->src[0].alias);
        for (int si = 1; si < q->nsrc; si++) {
            QSrc* s = &q->src[si];
            snprintf(reals[si], sizeof(reals[0]), "%s", s->table[0] ? s->table : s->alias);
            snprintf(aliases[si], sizeof(aliases[0]), "%s", s->alias);
            for (int p = 0; p < si; p++) {
                if (str_ci_cmp(aliases[p], s->alias) == 0) {
                    snprintf(R->err, sizeof(R->err), "the name '%s' is used for two tables; give one of them another name: from %s as other_name", s->alias, s->table[0] ? s->table : "...");
                    goto done;
                }
            }
            QSet* B = qs_new();
            if (!q_load_source(R, s, outer, B)) { q_set_free(B); goto done; }
            if (S->ncols + B->ncols > Q_MAX_COLS) {
                q_set_free(B);
                snprintf(R->err, sizeof(R->err), "the joined tables have more than %d columns", Q_MAX_COLS);
                goto done;
            }
            QSet* J = qs_new();
            J->ncols = S->ncols + B->ncols;
            for (int c = 0; c < S->ncols; c++) strcpy(J->names[c], S->names[c]);
            for (int c = 0; c < B->ncols; c++) strcpy(J->names[S->ncols + c], B->names[c]);

            /* how the rows match: 'on ...', 'using (...)', or the key columns by their names; none for a cross join */
            QExpr* on = s->on;
            JoinKind jk = (JoinKind)s->jk;
            if (jk != JK_CROSS && !on) {
                if (s->using_col[0]) {
                    int left = -1, nleft = 0;
                    for (int c = 0; c < S->ncols; c++) if (str_ci_cmp(q_short(S->names[c]), s->using_col) == 0) { left = c; nleft++; }
                    char rname[130];
                    snprintf(rname, sizeof(rname), "%s.%s", s->alias, s->using_col);
                    if (nleft != 1 || q_find_col(B, rname) < 0) {
                        snprintf(R->err, sizeof(R->err), "'using (%s)' needs exactly one column %s before the join and one in %s", s->using_col, s->using_col, s->alias);
                        q_set_free(B); q_set_free(J);
                        goto done;
                    }
                    on = owned_on[si] = q_eq_names(S->names[left], rname);
                } else {
                    QPair pairs[4];
                    int ways = q_guess_on(S, reals, aliases, si, B, s->alias, reals[si], pairs, 4);
                    if (ways != 1) {
                        q_guess_error(R->err, sizeof(R->err), S, s->alias, pairs, ways > 4 ? 4 : ways);
                        q_set_free(B); q_set_free(J);
                        goto done;
                    }
                    on = owned_on[si] = q_eq_names(pairs[0].l, pairs[0].r);
                }
            }
            if (on && !q_check(R, on, J, outer, false, false, err, sizeof(err))) {
                snprintf(R->err, sizeof(R->err), "%s", err);
                q_set_free(B); q_set_free(J);
                goto done;
            }

            bool* b_hit = (bool*)calloc(B->nrows + 1, sizeof(bool));
            EnlngVal* both = q_row_new(J->ncols);
            for (size_t a = 0; a < S->nrows && !R->err[0]; a++) {
                bool matched = false;
                for (size_t b = 0; b < B->nrows; b++) {
                    memcpy(both, S->rows[a], (size_t)S->ncols * sizeof(EnlngVal));
                    memcpy(both + S->ncols, B->rows[b], (size_t)B->ncols * sizeof(EnlngVal));
                    if (on) {
                        QEnv E = {J, both, NULL, 0, false, 0, outer, R};
                        EnlngVal t = q_eval(&E, on);
                        bool hit = q_truth(&t);
                        enlng_free_val(&t);
                        if (!hit) continue;
                    }
                    matched = true;
                    b_hit[b] = true;
                    EnlngVal* row = q_row_new(J->ncols);
                    for (int c = 0; c < J->ncols; c++) row[c] = enlng_copy_val(&both[c]);
                    q_set_add(J, row);
                }
                if (!matched && (jk == JK_LEFT || jk == JK_FULL)) {
                    EnlngVal* row = q_row_new(J->ncols);
                    for (int c = 0; c < S->ncols; c++) row[c] = enlng_copy_val(&S->rows[a][c]);
                    for (int c = S->ncols; c < J->ncols; c++) row[c] = enlng_null();
                    q_set_add(J, row);
                }
            }
            if (jk == JK_RIGHT || jk == JK_FULL) {
                for (size_t b = 0; b < B->nrows; b++) {
                    if (b_hit[b]) continue;
                    EnlngVal* row = q_row_new(J->ncols);
                    for (int c = 0; c < S->ncols; c++) row[c] = enlng_null();
                    for (int c = 0; c < B->ncols; c++) row[S->ncols + c] = enlng_copy_val(&B->rows[b][c]);
                    q_set_add(J, row);
                }
            }
            free(b_hit);
            free(both);
            q_set_free(B);
            q_set_free(S);
            S = J;
            if (R->err[0]) goto done;
        }
    }

    /* where */
    if (q->where) {
        if (!q_check(R, q->where, S, outer, false, false, err, sizeof(err))) { snprintf(R->err, sizeof(R->err), "%s", err); goto done; }
        size_t keep = 0;
        for (size_t r = 0; r < S->nrows; r++) {
            QEnv E = {S, S->rows[r], NULL, 0, false, 0, outer, R};
            EnlngVal t = q_eval(&E, q->where);
            bool hit = q_truth(&t);
            enlng_free_val(&t);
            if (R->err[0]) goto done;
            if (hit) {
                S->rows[keep++] = S->rows[r];
            } else {
                for (int c = 0; c < S->ncols; c++) enlng_free_val(&S->rows[r][c]);
                free(S->rows[r]);
            }
        }
        S->nrows = keep;
    }

    /* what to show */
    bool joined = q->nsrc > 1;
    QExpr* made[Q_MAX_COLS * 2];
    int nmade = 0;
    (void)made; (void)nmade;
    if (q->count_style || (q->all_cols && q->ngroup)) {
        for (int g = 0; g < q->ngroup && nitems < Q_MAX_COLS - 1; g++) {
            items[nitems].e = q->group[g];
            if (q->group[g]->kind == EK_NAME) snprintf(items[nitems].title, sizeof(items[0].title), "%s", q->group[g]->text);
            else snprintf(items[nitems].title, sizeof(items[0].title), "group %d", g + 1);
            nitems++;
        }
        QExpr* cnt = qe_new(EK_AGG);
        cnt->agg = QA_COUNT;
        cnt->star = true;
        snprintf(cnt->text, sizeof(cnt->text), "count");
        items[nitems].e = cnt;
        items[nitems].owned = true;
        snprintf(items[nitems].title, sizeof(items[0].title), "count(*)");
        nitems++;
    } else if (q->all_cols) {
        for (int c = 0; c < S->ncols && nitems < Q_MAX_COLS; c++, nitems++) {
            QExpr* n = qe_new(EK_NAME);
            snprintf(n->text, sizeof(n->text), "%s", S->names[c]);
            items[nitems].e = n;
            items[nitems].owned = true;
            snprintf(items[nitems].title, sizeof(items[0].title), "%s", joined ? S->names[c] : q_short(S->names[c]));
        }
    } else {
        for (int i = 0; i < q->nsel; i++) {
            QSel* sel = &q->sel[i];
            if (sel->star) {
                bool any = false;
                for (int c = 0; c < S->ncols && nitems < Q_MAX_COLS; c++) {
                    if (sel->star_table[0]) {
                        size_t tl = strlen(sel->star_table);
                        if (!(strncmp(S->names[c], sel->star_table, tl) == 0 && S->names[c][tl] == '.')) continue;
                    }
                    any = true;
                    QExpr* n = qe_new(EK_NAME);
                    snprintf(n->text, sizeof(n->text), "%s", S->names[c]);
                    items[nitems].e = n;
                    items[nitems].owned = true;
                    snprintf(items[nitems].title, sizeof(items[0].title), "%s", (joined || sel->star_table[0]) ? S->names[c] : q_short(S->names[c]));
                    nitems++;
                }
                if (!any && sel->star_table[0]) { snprintf(R->err, sizeof(R->err), "there is no table called '%s' in this query", sel->star_table); goto done; }
            } else {
                if (nitems >= Q_MAX_COLS) { snprintf(R->err, sizeof(R->err), "the result has more than %d columns; name the ones to show", Q_MAX_COLS); goto done; }
                items[nitems].e = sel->e;
                snprintf(items[nitems].title, sizeof(items[0].title), "%s", sel->title);
                nitems++;
            }
        }
    }
    if (S->ncols > 0 && nitems == 0 && q->has_from) { snprintf(R->err, sizeof(R->err), "nothing to show"); goto done; }

    for (int i = 0; i < nitems; i++) {
        if (!q_check(R, items[i].e, S, outer, true, true, err, sizeof(err))) { snprintf(R->err, sizeof(R->err), "%s", err); goto done; }
    }

    /* group by, order by: numbers and names from the select list mean those items */
    QExpr* gx[Q_MAX_GROUP];
    for (int g = 0; g < q->ngroup; g++) {
        int bad = 0;
        gx[g] = q_resolve_item(q->group[g], items, nitems, S, false, &bad);
        if (!gx[g]) { snprintf(R->err, sizeof(R->err), "'group by %d' is not a column of the result", bad); goto done; }
        if (q_has_agg(gx[g])) { snprintf(R->err, sizeof(R->err), "'group by' cannot use a total such as sum() or count()"); goto done; }
        if (!q_check(R, gx[g], S, outer, false, false, err, sizeof(err))) { snprintf(R->err, sizeof(R->err), "%s", err); goto done; }
    }
    bool grouped = q->ngroup > 0 || q->having != NULL || q->count_style;
    for (int i = 0; i < nitems && !grouped; i++) grouped = q_has_agg(items[i].e);
    QExpr* ox[Q_MAX_ORD];
    for (int k = 0; k < q->nord; k++) {
        int bad = 0;
        ox[k] = q_resolve_item(q->ord[k].e, items, nitems, S, true, &bad);
        if (!ox[k]) { snprintf(R->err, sizeof(R->err), "'order by %d' is not a column of the result", bad); goto done; }
        if (q_has_agg(ox[k])) grouped = grouped || true;
        if (!q_check(R, ox[k], S, outer, true, true, err, sizeof(err))) { snprintf(R->err, sizeof(R->err), "%s", err); goto done; }
    }
    if (q->having && !q_check(R, q->having, S, outer, true, false, err, sizeof(err))) { snprintf(R->err, sizeof(R->err), "%s", err); goto done; }

    /* the rows, or the groups of rows, that the result is made of */
    if (grouped) {
        nullrow = q_row_new(S->ncols);
        for (int c = 0; c < S->ncols; c++) nullrow[c] = enlng_null();
        size_t cap = 16;
        units = (QUnit*)calloc(cap, sizeof(QUnit));
        EnlngVal* gk = (EnlngVal*)calloc((size_t)(q->ngroup ? q->ngroup : 1) * (S->nrows + 1), sizeof(EnlngVal));
        size_t* first = (size_t*)calloc(S->nrows + 1, sizeof(size_t));
        size_t* of = (size_t*)calloc(S->nrows + 1, sizeof(size_t));
        int ng = q->ngroup;
        for (size_t r = 0; r < S->nrows; r++) {
            QEnv E = {S, S->rows[r], NULL, 0, false, 0, outer, R};
            for (int g = 0; g < ng; g++) gk[r * ng + g] = q_eval(&E, gx[g]);
            if (R->err[0]) { free(gk); free(first); free(of); goto done; }
            size_t u = nunits;
            if (ng > 0) {
                for (size_t k = 0; k < nunits; k++) {
                    if (q_rows_same(&gk[first[k] * ng], &gk[r * ng], ng)) { u = k; break; }
                }
            } else if (nunits > 0) {
                u = 0;
            }
            if (u == nunits) {
                if (nunits >= cap) { cap *= 2; units = (QUnit*)realloc(units, cap * sizeof(QUnit)); }
                first[nunits] = r;
                units[nunits].row = S->rows[r];
                units[nunits].mem = NULL;
                units[nunits].nmem = 0;
                nunits++;
            }
            of[r] = u;
        }
        for (size_t r = 0; r < S->nrows; r++) units[of[r]].nmem++;
        for (size_t u = 0; u < nunits; u++) { units[u].mem = (EnlngVal**)malloc((units[u].nmem + 1) * sizeof(EnlngVal*)); units[u].nmem = 0; }
        for (size_t r = 0; r < S->nrows; r++) units[of[r]].mem[units[of[r]].nmem++] = S->rows[r];
        for (size_t i = 0; i < S->nrows * (size_t)ng; i++) enlng_free_val(&gk[i]);
        free(gk); free(first); free(of);
        if (ng == 0 && nunits == 0) {
            /* a total of no rows is still one answer */
            units[0].row = nullrow;
            units[0].mem = (EnlngVal**)malloc(sizeof(EnlngVal*));
            units[0].nmem = 0;
            nunits = 1;
        }
        /* having */
        if (q->having) {
            size_t keep = 0;
            for (size_t u = 0; u < nunits; u++) {
                QEnv E = {S, units[u].row, units[u].mem, units[u].nmem, true, u, outer, R};
                EnlngVal t = q_eval(&E, q->having);
                bool hit = q_truth(&t);
                enlng_free_val(&t);
                if (R->err[0]) goto done;
                if (hit) units[keep++] = units[u];
                else free(units[u].mem);
            }
            nunits = keep;
        }
    } else {
        units = (QUnit*)calloc(S->nrows + 1, sizeof(QUnit));
        for (size_t r = 0; r < S->nrows; r++) units[r].row = S->rows[r];
        nunits = S->nrows;
    }

    /* window functions: one value for each row or group */
    QExpr* wins[16];
    int nwins = 0;
    for (int i = 0; i < nitems; i++) q_collect_wins(items[i].e, wins, &nwins, 16);
    for (int k = 0; k < q->nord; k++) q_collect_wins(ox[k], wins, &nwins, 16);
    for (int i = 0; i < nwins; i++) {
        if (!q_compute_window(R, wins[i], S, units, nunits, grouped, outer)) goto done;
    }

    /* project the items and the sort keys */
    O->ncols = nitems;
    for (int i = 0; i < nitems; i++) snprintf(O->names[i], sizeof(O->names[0]), "%s", items[i].title);
    int nord = q->nord;
    keys = (EnlngVal*)calloc(nunits * (size_t)(nord ? nord : 1) + 1, sizeof(EnlngVal));
    for (size_t u = 0; u < nunits; u++) {
        QEnv E = {S, units[u].row, units[u].mem, units[u].nmem, grouped, u, outer, R};
        EnlngVal* row = q_row_new(nitems);
        for (int i = 0; i < nitems; i++) row[i] = q_eval(&E, items[i].e);
        for (int k = 0; k < nord; k++) keys[u * nord + k] = q_eval(&E, ox[k]);
        q_set_add(O, row);
        if (R->err[0]) goto done;
    }

    /* distinct */
    if (q->distinct) {
        size_t keep = 0;
        for (size_t r = 0; r < O->nrows; r++) {
            bool seen = false;
            for (size_t k = 0; k < keep && !seen; k++) seen = q_rows_same(O->rows[k], O->rows[r], O->ncols);
            if (seen) {
                for (int c = 0; c < O->ncols; c++) enlng_free_val(&O->rows[r][c]);
                free(O->rows[r]);
                O->rows[r] = NULL;
            } else {
                if (keep != r) {
                    O->rows[keep] = O->rows[r];
                    for (int k = 0; k < nord; k++) {
                        enlng_free_val(&keys[keep * nord + k]);
                        keys[keep * nord + k] = keys[r * nord + k];
                        keys[r * nord + k] = enlng_null();
                    }
                }
                keep++;
            }
        }
        O->nrows = keep;
    }

    /* order by */
    if (nord > 0 && O->nrows > 1) {
        QSortCtx ctx;
        ctx.n = nord;
        ctx.keys = keys;
        for (int k = 0; k < nord; k++) ctx.desc[k] = q->ord[k].desc;
        size_t* idx = (size_t*)malloc(O->nrows * sizeof(size_t));
        for (size_t i = 0; i < O->nrows; i++) idx[i] = i;
        q_sort_idx(idx, O->nrows, &ctx);
        EnlngVal** sorted = (EnlngVal**)malloc(O->nrows * sizeof(EnlngVal*));
        for (size_t i = 0; i < O->nrows; i++) sorted[i] = O->rows[idx[i]];
        memcpy(O->rows, sorted, O->nrows * sizeof(EnlngVal*));
        free(sorted);
        free(idx);
    }

    /* top / limit / offset */
    {
        size_t from_row = (size_t)q->offset < O->nrows ? (size_t)q->offset : O->nrows;
        size_t to_row = O->nrows;
        if (q->limit >= 0 && from_row + (size_t)q->limit < to_row) to_row = from_row + (size_t)q->limit;
        for (size_t r = 0; r < O->nrows; r++) {
            if (r >= from_row && r < to_row) continue;
            for (int c = 0; c < O->ncols; c++) enlng_free_val(&O->rows[r][c]);
            free(O->rows[r]);
            O->rows[r] = NULL;
        }
        size_t keep = 0;
        for (size_t r = from_row; r < to_row; r++) O->rows[keep++] = O->rows[r];
        O->nrows = keep;
    }
    ok = true;

done:
    if (!ok) {
        if (!R->err[0] && err[0]) snprintf(R->err, sizeof(R->err), "%s", err);
        if (O->nrows) q_set_clear(O);
    }
    if (keys) {
        for (size_t i = 0; i < nunits * (size_t)(q->nord ? q->nord : 1); i++) enlng_free_val(&keys[i]);
        free(keys);
    }
    if (units) {
        for (size_t u = 0; u < nunits; u++) free(units[u].mem);
        free(units);
    }
    if (nullrow) {
        for (int c = 0; c < S->ncols; c++) enlng_free_val(&nullrow[c]);
        free(nullrow);
    }
    for (int i = 0; i < nitems; i++) if (items[i].owned) q_free_expr(items[i].e);
    for (int i = 0; i < Q_MAX_SRC; i++) q_free_expr(owned_on[i]);
    q_set_free(S);
    return ok;
}

static bool q_exec(QRun* R, QQuery* q, const QEnv* outer, QSet* out) {
    int saved = R->nctes;
    for (int i = 0; i < q->nwith; i++) {
        if (R->nctes >= 32) break;
        R->ctes[R->nctes].name = q->with[i].name;
        R->ctes[R->nctes].q = q->with[i].q;
        R->nctes++;
    }
    bool ok = q_exec_one(R, q, outer, out);
    for (QQuery* cur = q; ok && cur->next; cur = cur->next) {
        QSet* T = qs_new();
        ok = q_exec_one(R, cur->next, outer, T);
        if (ok && T->ncols != out->ncols) {
            snprintf(R->err, sizeof(R->err), "the queries of a union must show the same number of columns (%d and %d)", out->ncols, T->ncols);
            ok = false;
        }
        if (ok) {
            int op = cur->setop;
            if (op == SO_UNION_ALL || op == SO_UNION) {
                for (size_t r = 0; r < T->nrows; r++) q_set_add(out, T->rows[r]);
                T->nrows = 0;
                if (op == SO_UNION) q_dedupe(out);
            } else {
                size_t keep = 0;
                for (size_t r = 0; r < out->nrows; r++) {
                    bool in_t = false;
                    for (size_t k = 0; k < T->nrows && !in_t; k++) in_t = q_rows_same(out->rows[r], T->rows[k], out->ncols);
                    if (in_t == (op == SO_INTERSECT)) {
                        out->rows[keep++] = out->rows[r];
                    } else {
                        for (int c = 0; c < out->ncols; c++) enlng_free_val(&out->rows[r][c]);
                        free(out->rows[r]);
                    }
                }
                out->nrows = keep;
                q_dedupe(out);
            }
        }
        q_set_free(T);
    }
    /* order by / limit after the last part of a union */
    if (ok && q->next && (q->ncord || q->climit >= 0 || q->coffset)) {
        int n = q->ncord;
        if (n > 0 && out->nrows > 1) {
            EnlngVal* keys = (EnlngVal*)calloc(out->nrows * (size_t)n + 1, sizeof(EnlngVal));
            QItem items[Q_MAX_COLS];
            memset(items, 0, sizeof(items));
            for (int i = 0; i < out->ncols && i < Q_MAX_COLS; i++) snprintf(items[i].title, sizeof(items[0].title), "%s", out->names[i]);
            for (size_t r = 0; r < out->nrows && ok; r++) {
                QEnv E = {out, out->rows[r], NULL, 0, false, 0, NULL, R};
                for (int k = 0; k < n && ok; k++) {
                    QExpr* x = q->cord[k].e;
                    if (x->kind == EK_LIT && x->val.type == ENLNG_VAL_INT) {
                        int64_t pos = x->val.int_val;
                        if (pos < 1 || pos > out->ncols) { snprintf(R->err, sizeof(R->err), "'order by %lld' is not a column of the result", (long long)pos); ok = false; break; }
                        keys[r * n + k] = enlng_copy_val(&out->rows[r][pos - 1]);
                    } else {
                        char err[300];
                        if (!q_check(R, x, out, NULL, false, false, err, sizeof(err))) { snprintf(R->err, sizeof(R->err), "%s", err); ok = false; break; }
                        keys[r * n + k] = q_eval(&E, x);
                        if (R->err[0]) ok = false;
                    }
                }
            }
            if (ok) {
                QSortCtx ctx;
                ctx.n = n;
                ctx.keys = keys;
                for (int k = 0; k < n; k++) ctx.desc[k] = q->cord[k].desc;
                size_t* idx = (size_t*)malloc(out->nrows * sizeof(size_t));
                for (size_t i = 0; i < out->nrows; i++) idx[i] = i;
                q_sort_idx(idx, out->nrows, &ctx);
                EnlngVal** sorted = (EnlngVal**)malloc(out->nrows * sizeof(EnlngVal*));
                for (size_t i = 0; i < out->nrows; i++) sorted[i] = out->rows[idx[i]];
                memcpy(out->rows, sorted, out->nrows * sizeof(EnlngVal*));
                free(sorted);
                free(idx);
            }
            for (size_t i = 0; i < out->nrows * (size_t)n; i++) enlng_free_val(&keys[i]);
            free(keys);
        }
        if (ok) {
            size_t from_row = (size_t)q->coffset < out->nrows ? (size_t)q->coffset : out->nrows;
            size_t to_row = out->nrows;
            if (q->climit >= 0 && from_row + (size_t)q->climit < to_row) to_row = from_row + (size_t)q->climit;
            for (size_t r = 0; r < out->nrows; r++) {
                if (r >= from_row && r < to_row) continue;
                for (int c = 0; c < out->ncols; c++) enlng_free_val(&out->rows[r][c]);
                free(out->rows[r]);
                out->rows[r] = NULL;
            }
            size_t keep = 0;
            for (size_t r = from_row; r < to_row; r++) out->rows[keep++] = out->rows[r];
            out->nrows = keep;
        }
    }
    R->nctes = saved;
    return ok;
}

/* a stored query (a view), parsed and run */
static bool q_exec_text(QRun* R, const char* sql, const QEnv* outer, QSet* out) {
    QLex* L = (QLex*)malloc(sizeof(QLex));
    q_lex(L, sql);
    QP P = {L, NM_SOFT_LIT, false, false, false, false};
    QQuery* q = qp_query(&P);
    bool ok = false;
    if (!q) {
        snprintf(R->err, sizeof(R->err), "%s", L->err[0] ? L->err : "the stored query could not be read");
    } else if (q_peek(L, 0)->kind != QT_END) {
        snprintf(R->err, sizeof(R->err), "I don't understand '%s' here", q_peek(L, 0)->text);
    } else {
        ok = q_exec(R, q, outer, out);
    }
    q_free_query(q);
    free(L);
    return ok;
}

/* the old count box, for a plain "count records in t" */
static void q_print_count_box(long long k) {
    printf("+----------+\n");
    printf("| count(*) |\n");
    printf("+----------+\n");
    printf("| %-8lld |\n", k);
    printf("+----------+\n");
    printf("1 row in set (0.01 ms)\n\n");
}

/* the rows of a finished query, shown as a table */
static void q_show_set(QSet* O, double ms, bool print_output) {
    EnlngQueryResult* res = (EnlngQueryResult*)calloc(1, sizeof(EnlngQueryResult));
    res->col_count = O->ncols;
    for (int c = 0; c < O->ncols; c++) snprintf(res->columns[c].name, sizeof(res->columns[c].name), "%s", O->names[c]);
    res->rows = (EnlngRow*)calloc(O->nrows + 1, sizeof(EnlngRow));
    for (size_t r = 0; r < O->nrows; r++) {
        EnlngRow* dest = &res->rows[res->row_count++];
        dest->cell_count = O->ncols;
        dest->cells = O->rows[r];
        O->rows[r] = NULL;
    }
    res->execution_time_ms = ms;
    if (print_output) enlngdb_print_result(res);
    enlngdb_free_result(res);
}

static void q_print_plan(EnlngDatabase* db, QQuery* q) {
    int part = 1;
    for (QQuery* c = q; c; c = c->next, part++) {
        if (q->next) printf("Part %d of the union:\n", part);
        for (int i = 0; i < c->nsrc; i++) {
            QSrc* s = &c->src[i];
            const char* kind = "inner";
            switch (s->jk) { case JK_LEFT: kind = "left"; break; case JK_RIGHT: kind = "right"; break; case JK_FULL: kind = "full"; break; case JK_CROSS: kind = "cross"; break; default: break; }
            EnlngTable* t = s->table[0] ? enlngdb_get_table(db, s->table) : NULL;
            char what[160];
            if (s->sub) snprintf(what, sizeof(what), "a bracketed query");
            else if (t && t->view_sql[0]) snprintf(what, sizeof(what), "view %s", s->table);
            else if (t) snprintf(what, sizeof(what), "table %s (%lu rows)", s->table, (unsigned long)t->row_count);
            else snprintf(what, sizeof(what), "%s", s->table);
            if (i == 0) printf("  1. read every row of %s\n", what);
            else printf("  %d. %s join %s, %s\n", i + 1, kind, what, s->jk == JK_CROSS ? "every row with every row" : s->on ? "matching the 'on' condition" : s->using_col[0] ? "matching the 'using' column" : "matching the key columns by their names");
        }
        int step = c->nsrc + 1;
        if (!c->has_from) printf("  1. no table: one row of values\n");
        if (c->where) printf("  %d. keep the rows that meet 'where'\n", step++);
        if (c->ngroup) printf("  %d. group the rows by %d expression(s) and total each group\n", step++, c->ngroup);
        else if (c->count_style) printf("  %d. count the rows\n", step++);
        if (c->having) printf("  %d. keep the groups that meet 'having'\n", step++);
        if (c->distinct) printf("  %d. drop repeated rows\n", step++);
        if (c->nord || c->ncord) printf("  %d. sort the rows (%d key%s)\n", step++, c->nord + c->ncord, c->nord + c->ncord == 1 ? "" : "s");
        if (c->limit >= 0 || c->offset || c->climit >= 0 || c->coffset) printf("  %d. keep only the rows asked for with top / limit / offset\n", step++);
        if (c->next) printf("  then: %s\n", c->setop == SO_UNION ? "union (repeated rows dropped)" : c->setop == SO_UNION_ALL ? "union all" : c->setop == SO_INTERSECT ? "intersect" : "except");
    }
    printf("(EnlngDB reads tables from memory; indexes are recorded but every query reads its tables in full.)\n\n");
}

/* find ... / select ... / with ... / count ... : parse, run and show */
static bool run_query(EnlngDatabase* db, const char* stmt, bool print_output, bool explain) {
    clock_t t0 = clock();
    QLex* L = (QLex*)malloc(sizeof(QLex));
    q_lex(L, stmt);
    QP P = {L, NM_SOFT_LIT, false, false, false, false};
    QQuery* q = qp_query(&P);
    bool ok = false;
    QRun R;
    memset(&R, 0, sizeof(R));
    R.db = db;
    R.print = print_output;
    if (q && q_peek(L, 0)->kind != QT_END) {
        snprintf(L->err, sizeof(L->err), "I don't understand '%s' here", q_peek(L, 0)->text);
        q_free_query(q);
        q = NULL;
    }
    if (!q) {
        if (print_output) printf("ERROR: %s.\n\n", L->err[0] ? L->err : "I could not read this query");
        free(L);
        return false;
    }
    if (explain) {
        if (print_output) q_print_plan(db, q);
        q_free_query(q);
        free(L);
        return true;
    }
    QSet* O = qs_new();
    ok = q_exec(&R, q, NULL, O);
    if (ok) {
        bool plain_count = q->count_style && !q->ngroup;
        if (plain_count) {
            if (print_output) q_print_count_box(O->nrows ? (long long)O->rows[0][0].int_val : 0);
        } else {
            q_show_set(O, (double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC, print_output);
        }
    } else if (!R.err_printed && print_output) {
        printf("ERROR: %s.\n\n", R.err[0] ? R.err : "the query failed");
    }
    q_set_free(O);
    q_free_query(q);
    free(L);
    return ok;
}

/* ---- Tables: what a column holds, and what a row may be ---------------------------------------- */

static int q_col_index(const EnlngTable* t, const char* name) {
    for (int c = 0; c < t->col_count; c++) if (str_ci_cmp(t->columns[c].name, name) == 0) return c;
    return -1;
}

static void q_print_columns_of(const EnlngTable* t) {
    printf("       Its columns are:");
    for (int c = 0; c < t->col_count; c++) printf(" %s", t->columns[c].name);
    printf("\n");
}

static const char* q_kind_name(int kind) {
    switch (kind) {
        case EK_INTEGER: return "integer";
        case EK_REAL: return "real";
        case EK_TEXT: return "text";
        case EK_BOOLEAN: return "boolean";
        case EK_DATE: return "date";
        default: return "any";
    }
}

/* a type word: integer, int, real, float, text, varchar, boolean, date ... ; -1 if it is not one */
static int q_kind_word(const char* w, bool* auto_col) {
    static const struct { const char* word; int kind; bool auto_col; } kinds[] = {
        {"integer", EK_INTEGER, false}, {"int", EK_INTEGER, false}, {"bigint", EK_INTEGER, false}, {"smallint", EK_INTEGER, false},
        {"whole", EK_INTEGER, false}, {"serial", EK_INTEGER, true},
        {"real", EK_REAL, false}, {"float", EK_REAL, false}, {"double", EK_REAL, false}, {"decimal", EK_REAL, false},
        {"numeric", EK_REAL, false}, {"number", EK_REAL, false},
        {"text", EK_TEXT, false}, {"string", EK_TEXT, false}, {"varchar", EK_TEXT, false}, {"char", EK_TEXT, false},
        {"boolean", EK_BOOLEAN, false}, {"bool", EK_BOOLEAN, false},
        {"date", EK_DATE, false}, {"datetime", EK_DATE, false}, {"timestamp", EK_DATE, false},
        {NULL, 0, false}
    };
    for (int i = 0; kinds[i].word; i++) {
        if (str_ci_cmp(w, kinds[i].word) == 0) { if (kinds[i].auto_col) *auto_col = true; return kinds[i].kind; }
    }
    return -1;
}

static void q_quote_val(const EnlngVal* v, char* buf, size_t n) {
    if (v->type == ENLNG_VAL_STRING) snprintf(buf, n, "'%s'", v->str_val ? v->str_val : "");
    else q_val_text(v, buf, n);
}

/* make a value the kind its column holds; false (and a reason) if it cannot be */
static bool q_coerce(const EnlngColumn* col, EnlngVal* v, char* err, size_t en) {
    if (v->type == ENLNG_VAL_NULL || col->kind == EK_ANY) return true;
    char shown[200];
    q_quote_val(v, shown, sizeof(shown));
    double d; bool ii; int64_t l;
    switch (col->kind) {
        case EK_INTEGER:
            if (v->type == ENLNG_VAL_INT) return true;
            if (q_num(v, &d, &ii, &l) && v->type != ENLNG_VAL_BOOL) {
                if (!ii && d != floor(d)) { snprintf(err, en, "column '%s' holds whole numbers, and %s has a fraction", col->name, shown); return false; }
                enlng_free_val(v);
                *v = enlng_int(ii ? l : (int64_t)d);
                return true;
            }
            snprintf(err, en, "column '%s' holds whole numbers, and %s is not one", col->name, shown);
            return false;
        case EK_REAL:
            if (v->type == ENLNG_VAL_DOUBLE) return true;
            if (q_num(v, &d, &ii, &l)) {
                enlng_free_val(v);
                *v = enlng_double(d);
                return true;
            }
            snprintf(err, en, "column '%s' holds numbers, and %s is not one", col->name, shown);
            return false;
        case EK_TEXT:
            if (v->type == ENLNG_VAL_STRING) return true;
            {
                char t[256];
                q_val_text(v, t, sizeof(t));
                enlng_free_val(v);
                *v = enlng_str(t);
            }
            return true;
        case EK_BOOLEAN:
            if (v->type == ENLNG_VAL_BOOL) return true;
            if (v->type == ENLNG_VAL_INT && (v->int_val == 0 || v->int_val == 1)) { bool b = v->int_val == 1; *v = enlng_bool(b); return true; }
            if (v->type == ENLNG_VAL_STRING && v->str_val) {
                const char* s = v->str_val;
                bool yes = str_ci_cmp(s, "true") == 0 || str_ci_cmp(s, "yes") == 0 || strcmp(s, "1") == 0;
                bool no = str_ci_cmp(s, "false") == 0 || str_ci_cmp(s, "no") == 0 || strcmp(s, "0") == 0;
                if (yes || no) { enlng_free_val(v); *v = enlng_bool(yes); return true; }
            }
            snprintf(err, en, "column '%s' holds true or false, and %s is neither", col->name, shown);
            return false;
        case EK_DATE:
            if (v->type == ENLNG_VAL_STRING && q_is_date_text(v->str_val)) return true;
            snprintf(err, en, "column '%s' holds a date like 2024-03-15, and %s is not one", col->name, shown);
            return false;
    }
    return true;
}

static int q_key_cols(const EnlngTable* t, int* cols) {
    int n = 0;
    for (int c = 0; c < t->col_count && n < 4; c++) if (t->columns[c].is_primary_key) cols[n++] = c;
    if (n == 0) {
        int id = q_col_index(t, "id");
        if (id >= 0) cols[n++] = id;
    }
    return n;
}

static bool q_has_declared_key(const EnlngTable* t) {
    for (int c = 0; c < t->col_count; c++) if (t->columns[c].is_primary_key) return true;
    return false;
}

static void q_row_env_set(const EnlngTable* t, QSet* S) {
    S->ncols = t->col_count;
    for (int c = 0; c < t->col_count; c++) snprintf(S->names[c], sizeof(S->names[0]), "%s.%s", t->name, t->columns[c].name);
}

/* does a column's check hold for this row? */
static bool q_check_holds(QRun* R, const EnlngTable* t, const char* text, EnlngVal* cells, char* err, size_t en) {
    QLex* L = (QLex*)malloc(sizeof(QLex));
    q_lex(L, text);
    QP P = {L, NM_SOFT_LIT, false, false, false, false};
    QExpr* e = qp_condition(&P, NM_SOFT_LIT, false);
    bool holds = true;
    if (!e || q_peek(L, 0)->kind != QT_END) {
        snprintf(err, en, "the check '%s' cannot be read: %s", text, L->err[0] ? L->err : "something is left over");
        holds = false;
    } else {
        QSet* S = qs_new();
        q_row_env_set(t, S);
        QEnv E = {S, cells, NULL, 0, false, 0, NULL, R};
        EnlngVal v = q_eval(&E, e);
        if (R->err[0]) { snprintf(err, en, "the check '%s' failed to run: %s", text, R->err); R->err[0] = '\0'; holds = false; }
        else holds = q_truth(&v);
        enlng_free_val(&v);
        q_set_free(S);
    }
    q_free_expr(e);
    free(L);
    return holds;
}

static bool q_parent_has(EnlngDatabase* db, const EnlngColumn* col, const EnlngVal* v) {
    EnlngTable* p = enlngdb_get_table(db, col->ref_table);
    if (!p) return false;
    int pc = q_col_index(p, col->ref_col);
    if (pc < 0) return false;
    for (size_t r = 0; r < p->row_count; r++) {
        if (pc < p->rows[r].cell_count && enlng_compare_vals(&p->rows[r].cells[pc], v) == 0 && p->rows[r].cells[pc].type != ENLNG_VAL_NULL) return true;
    }
    return false;
}

/* what every row of the table must meet: not null, unique, key, check, references. skip is the row being replaced. */
static bool q_check_row(QRun* R, EnlngTable* t, EnlngVal* cells, int skip, char* err, size_t en) {
    for (int c = 0; c < t->col_count; c++) {
        const EnlngColumn* col = &t->columns[c];
        if (cells[c].type == ENLNG_VAL_NULL) {
            if (col->not_null) { snprintf(err, en, "column '%s' cannot be empty (it is marked not null)", col->name); return false; }
            continue;
        }
        bool single_unique = col->is_unique;
        for (int i = 0; i < t->index_count; i++) {
            if (t->indexes[i].is_unique && t->indexes[i].col_count == 1 && str_ci_cmp(t->indexes[i].cols[0], col->name) == 0) single_unique = true;
        }
        if (single_unique) {
            for (size_t r = 0; r < t->row_count; r++) {
                if ((int)r == skip || c >= t->rows[r].cell_count) continue;
                if (t->rows[r].cells[c].type != ENLNG_VAL_NULL && enlng_compare_vals(&t->rows[r].cells[c], &cells[c]) == 0) {
                    char shown[200];
                    q_quote_val(&cells[c], shown, sizeof(shown));
                    snprintf(err, en, "%s is already in column '%s' of '%s', and that column must be different in every row", shown, col->name, t->name);
                    return false;
                }
            }
        }
        if (col->check_text[0] && !q_check_holds(R, t, col->check_text, cells, err, en)) {
            if (!err[0] || strstr(err, "cannot be read") == NULL) snprintf(err, en, "column '%s' does not allow this value: the check '%s' is not met", col->name, col->check_text);
            return false;
        }
        if (col->ref_table[0] && !q_parent_has(R->db, col, &cells[c])) {
            char shown[200];
            q_quote_val(&cells[c], shown, sizeof(shown));
            snprintf(err, en, "%s is not in %s.%s, which column '%s' refers to", shown, col->ref_table, col->ref_col, col->name);
            return false;
        }
    }
    /* a key of several columns, and unique indexes of several columns */
    int keys[4];
    int nk = q_key_cols(t, keys);
    struct { int cols[4]; int n; const char* what; } groups[ENLNGDB_MAX_INDEXES + 1];
    int ng = 0;
    if (nk > 1) { memcpy(groups[ng].cols, keys, sizeof(keys)); groups[ng].n = nk; groups[ng].what = "key"; ng++; }
    for (int i = 0; i < t->index_count; i++) {
        if (!t->indexes[i].is_unique || t->indexes[i].col_count < 2) continue;
        groups[ng].n = 0;
        for (int k = 0; k < t->indexes[i].col_count; k++) groups[ng].cols[groups[ng].n++] = q_col_index(t, t->indexes[i].cols[k]);
        groups[ng].what = "unique index";
        ng++;
    }
    for (int g = 0; g < ng; g++) {
        bool any_null = false;
        for (int k = 0; k < groups[g].n; k++) any_null = any_null || cells[groups[g].cols[k]].type == ENLNG_VAL_NULL;
        if (any_null) continue;
        for (size_t r = 0; r < t->row_count; r++) {
            if ((int)r == skip) continue;
            bool same = true;
            for (int k = 0; k < groups[g].n && same; k++) {
                int c = groups[g].cols[k];
                same = c < t->rows[r].cell_count && enlng_compare_vals(&t->rows[r].cells[c], &cells[c]) == 0;
            }
            if (same) {
                snprintf(err, en, "a row with the same values in the %s columns already exists in '%s'", groups[g].what, t->name);
                return false;
            }
        }
    }
    /* a single-column key: the same value twice */
    if (nk == 1 && cells[keys[0]].type != ENLNG_VAL_NULL) {
        for (size_t r = 0; r < t->row_count; r++) {
            if ((int)r == skip || keys[0] >= t->rows[r].cell_count) continue;
            if (enlng_compare_vals(&t->rows[r].cells[keys[0]], &cells[keys[0]]) == 0 && t->rows[r].cells[keys[0]].type != ENLNG_VAL_NULL) {
                char shown[200];
                q_quote_val(&cells[keys[0]], shown, sizeof(shown));
                snprintf(err, en, "%s is already the %s of another row of '%s'", shown, t->columns[keys[0]].name, t->name);
                return false;
            }
        }
    }
    return true;
}

/* a copy of a table's rows, so a statement that fails half way can put them back */
typedef struct {
    EnlngRow* rows;
    size_t n;
} QSnap;

static QSnap q_snap(const EnlngTable* t) {
    QSnap s;
    s.n = t->row_count;
    s.rows = (EnlngRow*)calloc(s.n + 1, sizeof(EnlngRow));
    for (size_t r = 0; r < s.n; r++) {
        s.rows[r].cell_count = t->rows[r].cell_count;
        s.rows[r].cells = (EnlngVal*)malloc((size_t)(t->rows[r].cell_count ? t->rows[r].cell_count : 1) * sizeof(EnlngVal));
        for (int c = 0; c < t->rows[r].cell_count; c++) s.rows[r].cells[c] = enlng_copy_val(&t->rows[r].cells[c]);
    }
    return s;
}

static void q_snap_drop(QSnap* s) {
    for (size_t r = 0; r < s->n; r++) {
        for (int c = 0; c < s->rows[r].cell_count; c++) enlng_free_val(&s->rows[r].cells[c]);
        free(s->rows[r].cells);
    }
    free(s->rows);
    s->rows = NULL;
    s->n = 0;
}

static void q_snap_restore(EnlngTable* t, QSnap* s) {
    enlngdb_free_rows(t);
    t->rows = s->rows;
    t->row_count = s->n;
    t->row_capacity = s->n;
    s->rows = NULL;
    s->n = 0;
}

/* a row of the table was removed or its key changed: is another table's row still pointing at it? */
static bool q_referenced(EnlngDatabase* db, EnlngTable* parent, const EnlngVal* cells, int only_col, char* err, size_t en) {
    for (int i = 0; i < db->table_count; i++) {
        EnlngTable* child = &db->tables[i];
        for (int c = 0; c < child->col_count; c++) {
            const EnlngColumn* col = &child->columns[c];
            if (strcmp(col->ref_table, parent->name) != 0) continue;
            int pc = q_col_index(parent, col->ref_col);
            if (pc < 0 || (only_col >= 0 && pc != only_col)) continue;
            if (cells[pc].type == ENLNG_VAL_NULL) continue;
            size_t k = 0;
            for (size_t r = 0; r < child->row_count; r++) {
                if (c < child->rows[r].cell_count && enlng_compare_vals(&child->rows[r].cells[c], &cells[pc]) == 0 && child->rows[r].cells[c].type != ENLNG_VAL_NULL) k++;
            }
            if (k > 0) {
                char shown[200];
                q_quote_val(&cells[pc], shown, sizeof(shown));
                snprintf(err, en, "%lu row(s) of '%s' still refer to %s in %s.%s (%s.%s)", (unsigned long)k, child->name, shown, parent->name, col->ref_col, child->name, col->name);
                return false;
            }
        }
    }
    return true;
}

static void q_print_error(bool print_output, const char* msg, const char* after) {
    if (print_output) {
        printf("ERROR: %s.\n", msg);
        if (after && *after) printf("       %s\n", after);
        printf("\n");
    }
}

/* ---- Inserting ---- */
typedef struct {
    EnlngVal v[ENLNGDB_MAX_COLS];
    bool given[ENLNGDB_MAX_COLS];
} QInRow;

static void q_inrow_init(QInRow* r) {
    for (int i = 0; i < ENLNGDB_MAX_COLS; i++) { r->v[i] = enlng_null(); r->given[i] = false; }
}

static void q_inrow_free(QInRow* r) {
    for (int i = 0; i < ENLNGDB_MAX_COLS; i++) enlng_free_val(&r->v[i]);
}

/* add the rows; all of them go in, or none. note says which key a replaced row had. */
static bool q_insert_rows(QRun* R, EnlngTable* t, QInRow* rows, int nrows, int* replaced, char* note, size_t nn, char* err, size_t en) {
    QSnap snap = q_snap(t);
    int64_t saved_auto[ENLNGDB_MAX_COLS];
    memcpy(saved_auto, t->auto_next, sizeof(saved_auto));
    *replaced = 0;
    note[0] = '\0';
    q_normalize_rows(t);
    for (int i = 0; i < nrows; i++) {
        EnlngVal* cells = rows[i].v;
        for (int c = 0; c < t->col_count; c++) {
            const EnlngColumn* col = &t->columns[c];
            if (!rows[i].given[c] && col->default_text[0]) {
                enlng_free_val(&cells[c]);
                cells[c] = parse_val_token(col->default_text);
            }
            if (col->is_auto && cells[c].type == ENLNG_VAL_NULL) {
                int64_t next = t->auto_next[c];
                for (size_t r = 0; r < t->row_count; r++) {
                    const EnlngVal* v = &t->rows[r].cells[c];
                    if (v->type == ENLNG_VAL_INT && v->int_val >= next) next = v->int_val + 1;
                }
                if (next < 1) next = 1;
                cells[c] = enlng_int(next);
                t->auto_next[c] = next + 1;
            }
            if (!q_coerce(col, &cells[c], err, en)) goto fail;
        }
        int keys[4];
        int nk = q_key_cols(t, keys);
        int target = -1;
        if (nk > 0) {
            bool all_set = true;
            for (int k = 0; k < nk; k++) all_set = all_set && cells[keys[k]].type != ENLNG_VAL_NULL;
            for (size_t r = 0; all_set && r < t->row_count && target < 0; r++) {
                bool same = true;
                for (int k = 0; k < nk && same; k++) same = enlng_compare_vals(&t->rows[r].cells[keys[k]], &cells[keys[k]]) == 0;
                if (same) target = (int)r;
            }
        }
        if (!q_check_row(R, t, cells, target, err, en)) goto fail;
        if (target >= 0) {
            EnlngRow* row = &t->rows[target];
            for (int c = 0; c < t->col_count; c++) {
                enlng_free_val(&row->cells[c]);
                row->cells[c] = enlng_copy_val(&cells[c]);
            }
            row->version++;
            t->version++;
            (*replaced)++;
            if (!note[0]) {
                size_t k = 0;
                for (int j = 0; j < nk; j++) {
                    char shown[120];
                    q_quote_val(&cells[keys[j]], shown, sizeof(shown));
                    if (cells[keys[j]].type == ENLNG_VAL_STRING) snprintf(shown, sizeof(shown), "\"%s\"", cells[keys[j]].str_val);
                    k += (size_t)snprintf(note + k, nn - k, "%s%s %s", j ? ", " : "", t->columns[keys[j]].name, shown);
                }
            }
        } else if (!enlngdb_append_row(t, cells, t->col_count)) {
            snprintf(err, en, "out of memory");
            goto fail;
        }
    }
    q_snap_drop(&snap);
    return true;
fail:
    q_snap_restore(t, &snap);
    memcpy(t->auto_next, saved_auto, sizeof(saved_auto));
    return false;
}

static void q_say_inserted(bool print_output, EnlngTable* t, int nrows, int replaced, const char* note) {
    if (!print_output) return;
    if (nrows == 1 && replaced == 1) printf("Query OK, 1 row replaced: '%s' already had a row with %s. (0.04 ms)\n\n", t->name, note);
    else if (replaced > 0) printf("Query OK, %d rows affected, %d of them replaced a row with the same key (0.04 ms)\n\n", nrows, replaced);
    else if (nrows == 1) printf("Query OK, 1 row affected (0.04 ms)\n\n");
    else printf("Query OK, %d rows affected (0.04 ms)\n\n", nrows);
}

/* insert into T with a 1, b "x"  (the table and the columns are made if they are not there yet) */
static bool q_insert_with(EnlngDatabase* db, const char* stmt, bool print_output) {
    const char* start = str_starts_with_ci(stmt, "insert record into ") ? (stmt + 19) : (stmt + 12);
    const char* with_pos = strstr(start, " with ");
    if (!with_pos) with_pos = strstr(start, " WITH ");
    char tbl_name[64];
    int nlen = (int)(with_pos - start);
    if (nlen >= (int)sizeof(tbl_name)) nlen = (int)sizeof(tbl_name) - 1;
    strncpy(tbl_name, start, nlen);
    tbl_name[nlen] = '\0';
    char* tn = trim(tbl_name);
    EnlngTable* tbl = enlngdb_get_table(db, tn);
    if (!tbl) tbl = enlngdb_create_table(db, tn, NULL, 0);
    if (!tbl) { q_print_error(print_output, "the database already has the most tables it can hold", ""); return false; }
    if (tbl->view_sql[0]) { q_print_error(print_output, "this is a view, and rows cannot be inserted into a view", ""); return false; }

    char* pairs_copy = strdup(with_pos + 6);
    QInRow* row = (QInRow*)malloc(sizeof(QInRow));
    q_inrow_init(row);
    char err[300] = "";
    bool ok = true;
    char* p = pairs_copy;
    while (*p && ok) {
        while (isspace((unsigned char)*p)) p++;
        if (!*p) break;
        char* item_start = p;
        bool in_q = false;
        char q_char = 0;
        while (*p) {
            if (!in_q && (*p == '"' || *p == '\'')) { in_q = true; q_char = *p; }
            else if (in_q && *p == q_char) in_q = false;
            else if (!in_q && *p == ',') break;
            p++;
        }
        char saved = *p;
        *p = '\0';
        char* pair = trim(item_start);
        char* sep = strpbrk(pair, " :=");
        if (!sep || !*pair) {
            snprintf(err, sizeof(err), *pair ? "'%s' has no value; write it like: with %s 5" : "a value is missing between two commas%s", *pair ? pair : "", pair);
            ok = false;
        } else {
            char col_k[64];
            int klen = (int)(sep - pair);
            if (klen >= (int)sizeof(col_k)) klen = (int)sizeof(col_k) - 1;
            strncpy(col_k, pair, klen);
            col_k[klen] = '\0';
            while (*sep == ' ' || *sep == ':' || *sep == '=') sep++;
            int ci = q_col_index(tbl, col_k);
            if (ci < 0) {
                if (tbl->col_count >= ENLNGDB_MAX_COLS) { snprintf(err, sizeof(err), "table '%s' already has %d columns, the most a table can have", tbl->name, ENLNGDB_MAX_COLS); ok = false; }
                else if (!enlngdb_add_column(tbl, col_k, NULL)) { snprintf(err, sizeof(err), "could not add the column '%s'", col_k); ok = false; }
                else ci = tbl->col_count - 1;
            }
            if (ok && ci >= 0) {
                enlng_free_val(&row->v[ci]);
                row->v[ci] = parse_val_token(sep);
                row->given[ci] = true;
            }
        }
        if (saved == ',') {
            p++;
            while (isspace((unsigned char)*p)) p++;
            if (!*p && ok) { snprintf(err, sizeof(err), "a value is missing after the last comma"); ok = false; }
        }
    }
    int replaced = 0;
    char note[96];
    if (ok) {
        QRun R;
        memset(&R, 0, sizeof(R));
        R.db = db;
        ok = q_insert_rows(&R, tbl, row, 1, &replaced, note, sizeof(note), err, sizeof(err));
    }
    if (ok) {
        q_say_inserted(print_output, tbl, 1, replaced, note);
        snprintf(enlngdb_last_insert_replaced, sizeof(enlngdb_last_insert_replaced), "%s", replaced ? note : "");
    } else {
        q_print_error(print_output, err, "Nothing was inserted.");
    }
    q_inrow_free(row);
    free(row);
    free(pairs_copy);
    return ok;
}

/* insert into T [(a, b)] values (1, 2), (3, 4)   and   insert into T [(a, b)] find ... */
static bool q_insert_values(EnlngDatabase* db, const char* stmt, bool print_output) {
    QLex* L = (QLex*)malloc(sizeof(QLex));
    q_lex(L, stmt);
    QP P = {L, NM_SOFT_LIT, false, false, false, false};
    QRun R;
    memset(&R, 0, sizeof(R));
    R.db = db;
    R.print = print_output;
    bool ok = false;
    char err[300] = "";
    QInRow* rows = NULL;
    int nrows = 0, cap = 0;
    int cols[ENLNGDB_MAX_COLS], ncols = 0;
    QQuery* sel = NULL;
    QSet* res = NULL;

    q_take(L, "insert");
    q_take(L, "record") || q_take(L, "records") || q_take(L, "row") || q_take(L, "rows");
    if (!q_take(L, "into")) { snprintf(err, sizeof(err), "expected 'insert into <table> ...'"); goto done; }
    q_take(L, "table");
    q_take(L, "the");
    QTok* tn = q_peek(L, 0);
    if (tn->kind != QT_WORD) { snprintf(err, sizeof(err), "expected a table name after 'insert into'"); goto done; }
    EnlngTable* t = enlngdb_get_table(db, tn->text);
    if (!t) { q_missing(&R, tn->text); goto done; }
    if (t->view_sql[0]) { snprintf(err, sizeof(err), "'%s' is a view, and rows cannot be inserted into a view", t->name); goto done; }
    L->pos++;
    if (q_take(L, "(")) {
        for (;;) {
            QTok* c = q_peek(L, 0);
            if (c->kind != QT_WORD) { snprintf(err, sizeof(err), "expected a column name in the list after '%s'", t->name); goto done; }
            int ci = q_col_index(t, c->text);
            if (ci < 0) { snprintf(err, sizeof(err), "table '%s' has no column '%s'", t->name, c->text); goto done; }
            if (ncols >= ENLNGDB_MAX_COLS) goto done;
            cols[ncols++] = ci;
            L->pos++;
            if (!q_take(L, ",")) break;
        }
        if (!q_take(L, ")")) { snprintf(err, sizeof(err), "the column list has no ')'"); goto done; }
    } else {
        for (int c = 0; c < t->col_count; c++) cols[ncols++] = c;
    }
    if (q_take(L, "values") || q_take(L, "value")) {
        for (;;) {
            if (!q_take(L, "(")) { snprintf(err, sizeof(err), "each row of 'values' is written in brackets: values (1, \"Asha\")"); goto done; }
            if (nrows >= cap) { cap = cap ? cap * 2 : 4; rows = (QInRow*)realloc(rows, (size_t)cap * sizeof(QInRow)); }
            QInRow* row = &rows[nrows];
            q_inrow_init(row);
            nrows++;
            int k = 0;
            for (;;) {
                if (k >= ncols) { snprintf(err, sizeof(err), "this row has more values than the %d column(s) it fills", ncols); goto done; }
                QExpr* e = qp_value(&P, NM_SOFT_COL);
                if (!e) { snprintf(err, sizeof(err), "%s", L->err); goto done; }
                QEnv E = {NULL, NULL, NULL, 0, false, 0, NULL, &R};
                EnlngVal v = q_eval(&E, e);
                q_free_expr(e);
                if (R.err[0]) { snprintf(err, sizeof(err), "%s", R.err); enlng_free_val(&v); goto done; }
                row->v[cols[k]] = v;
                row->given[cols[k]] = true;
                k++;
                if (!q_take(L, ",")) break;
            }
            if (!q_take(L, ")")) { snprintf(err, sizeof(err), "a row of 'values' has no ')'"); goto done; }
            if (k != ncols) { snprintf(err, sizeof(err), "this row has %d value(s), but %d column(s) are being filled", k, ncols); goto done; }
            if (!q_take(L, ",")) break;
        }
        if (q_peek(L, 0)->kind != QT_END) { snprintf(err, sizeof(err), "I don't understand '%s' here", q_peek(L, 0)->text); goto done; }
    } else if (q_query_start(L, 0)) {
        sel = qp_query(&P);
        if (!sel) { snprintf(err, sizeof(err), "%s", L->err); goto done; }
        if (q_peek(L, 0)->kind != QT_END) { snprintf(err, sizeof(err), "I don't understand '%s' here", q_peek(L, 0)->text); goto done; }
        res = qs_new();
        if (!q_exec(&R, sel, NULL, res)) { snprintf(err, sizeof(err), "%s", R.err); goto done; }
        if (res->ncols != ncols) { snprintf(err, sizeof(err), "the query gives %d column(s), but %d column(s) are being filled", res->ncols, ncols); goto done; }
        cap = (int)res->nrows;
        rows = (QInRow*)malloc(((size_t)cap + 1) * sizeof(QInRow));
        for (size_t r = 0; r < res->nrows; r++) {
            q_inrow_init(&rows[r]);
            nrows++;
            for (int k = 0; k < ncols; k++) {
                rows[r].v[cols[k]] = enlng_copy_val(&res->rows[r][k]);
                rows[r].given[cols[k]] = true;
            }
        }
    } else {
        snprintf(err, sizeof(err), "after the table, write 'with <column> <value>, ...', 'values (...)' or a query");
        goto done;
    }
    if (nrows == 0) {
        if (print_output) printf("Query OK, 0 rows affected (0.04 ms)\n\n");
        ok = true;
        goto done;
    }
    {
        int replaced = 0;
        char note[96];
        ok = q_insert_rows(&R, t, rows, nrows, &replaced, note, sizeof(note), err, sizeof(err));
        if (ok) q_say_inserted(print_output, t, nrows, replaced, note);
    }
done:
    if (!ok) {
        if (!R.err_printed) q_print_error(print_output, err[0] ? err : (R.err[0] ? R.err : L->err), "Nothing was inserted.");
    }
    for (int i = 0; i < nrows; i++) q_inrow_free(&rows[i]);
    free(rows);
    q_set_free(res);
    q_free_query(sel);
    free(L);
    return ok;
}

/* ---- Update and delete ---- */

/* the rows that a 'where' picks, as row numbers */
static bool q_matching_rows(QRun* R, EnlngTable* t, QExpr* where, size_t** out, size_t* n, char* err, size_t en) {
    QSet* S = qs_new();
    q_row_env_set(t, S);
    *out = (size_t*)malloc((t->row_count + 1) * sizeof(size_t));
    *n = 0;
    bool ok = true;
    if (where && !q_check(R, where, S, NULL, false, false, err, en)) ok = false;
    for (size_t r = 0; ok && r < t->row_count; r++) {
        bool hit = true;
        if (where) {
            QEnv E = {S, t->rows[r].cells, NULL, 0, false, 0, NULL, R};
            EnlngVal v = q_eval(&E, where);
            hit = q_truth(&v);
            enlng_free_val(&v);
            if (R->err[0]) { snprintf(err, en, "%s", R->err); ok = false; }
        }
        if (hit) (*out)[(*n)++] = r;
    }
    q_set_free(S);
    return ok;
}

typedef struct {
    int col;
    QExpr* e;
} QAssign;

/* update T set a = 1, b = a + 1 [where ...] [limit N]  /  change a to 1 in T  /  in T change a to 1 */
static bool q_update(EnlngDatabase* db, const char* stmt, bool print_output) {
    QLex* L = (QLex*)malloc(sizeof(QLex));
    q_lex(L, stmt);
    QP P = {L, NM_SOFT_LIT, false, false, false, false};
    QRun R;
    memset(&R, 0, sizeof(R));
    R.db = db;
    R.print = print_output;
    char err[300] = "";
    bool ok = false;
    QAssign as[ENLNGDB_MAX_COLS];
    int nas = 0;
    QExpr* where = NULL;
    long limit = -1;
    size_t* rows = NULL;
    size_t nrows = 0;
    EnlngTable* t = NULL;
    QSnap snap = {NULL, 0};
    bool have_snap = false;

    bool in_form = q_take(L, "in");
    if (!in_form && !(q_take(L, "update") || q_take(L, "change") || q_take(L, "modify"))) { snprintf(err, sizeof(err), "expected update, change or modify"); goto done; }
    if (!in_form) {
        q_take(L, "records") || q_take(L, "rows") || q_take(L, "record") || q_take(L, "row");
        q_take(L, "in");
    }
    q_take(L, "the");
    q_take(L, "table");
    q_take(L, "the");
    QTok* tn = q_peek(L, 0);
    if (tn->kind != QT_WORD) { snprintf(err, sizeof(err), "expected a table name"); goto done; }
    t = enlngdb_get_table(db, tn->text);
    if (!t) { q_missing(&R, tn->text); goto done; }
    if (t->view_sql[0]) { snprintf(err, sizeof(err), "'%s' is a view, and a view cannot be changed", t->name); goto done; }
    L->pos++;
    if (in_form && !(q_is(L, 0, "update") || q_is(L, 0, "change") || q_is(L, 0, "set") || q_is(L, 0, "modify"))) {
        snprintf(err, sizeof(err), "after 'in %s' write what to do: update, change or set", t->name);
        goto done;
    }
    while (q_take(L, "update") || q_take(L, "change") || q_take(L, "set") || q_take(L, "modify")) {}
    for (;;) {
        QTok* c = q_peek(L, 0);
        if (c->kind != QT_WORD) { snprintf(err, sizeof(err), "expected a column name to change, like: update %s set marks = 90", t->name); goto done; }
        int ci = q_col_index(t, c->text);
        if (ci < 0) {
            char cols[400] = "";
            for (int k = 0; k < t->col_count; k++) { size_t l = strlen(cols); snprintf(cols + l, sizeof(cols) - l, " %s", t->columns[k].name); }
            snprintf(err, sizeof(err), "table '%s' has no column '%s'. Its columns are:%s", t->name, c->text, cols);
            goto done;
        }
        L->pos++;
        if (!(q_take(L, "=") || q_take(L, "to") || q_take(L, "is") || q_take(L, ":"))) { snprintf(err, sizeof(err), "after '%s' write '= <value>' or 'to <value>'", c->text); goto done; }
        if (nas >= ENLNGDB_MAX_COLS) goto done;
        QTok* first = q_peek(L, 0);
        bool bare_word = first->kind == QT_WORD && !q_is_number_word(first->text) && !q_is(L, 1, "(");
        QExpr* e = qp_value(&P, NM_SOFT_COL);
        if (!e) { snprintf(err, sizeof(err), "%s", L->err); goto done; }
        /* a value of several bare words, like: change city to New Delhi */
        if (bare_word && e->kind == EK_NAME && q_peek(L, 0)->kind == QT_WORD && !q_is(L, 0, "where") && !q_is(L, 0, "limit") && !q_is(L, 0, "and") && !q_is(L, 0, "confirmed")) {
            char joined[256];
            snprintf(joined, sizeof(joined), "%s", e->text);
            while (q_peek(L, 0)->kind == QT_WORD && !q_is(L, 0, "where") && !q_is(L, 0, "limit")) {
                size_t l = strlen(joined);
                snprintf(joined + l, sizeof(joined) - l, " %s", q_peek(L, 0)->text);
                L->pos++;
            }
            q_free_expr(e);
            e = qe_new(EK_LIT);
            e->val = enlng_str(joined);
        }
        as[nas].col = ci;
        as[nas].e = e;
        nas++;
        if (!q_take(L, ",")) break;
    }
    if (q_take(L, "where")) {
        where = qp_condition(&P, NM_SOFT_LIT, false);
        if (!where) { snprintf(err, sizeof(err), "%s", L->err); goto done; }
    }
    if (q_take(L, "limit")) {
        QTok* n = q_peek(L, 0);
        if (n->kind != QT_WORD || !isdigit((unsigned char)n->text[0])) { snprintf(err, sizeof(err), "'limit' needs a number"); goto done; }
        limit = atol(n->text);
        L->pos++;
    }
    if (q_peek(L, 0)->kind != QT_END) { snprintf(err, sizeof(err), "I don't understand '%s' here. After the values can come: where <condition>, limit N", q_peek(L, 0)->text); goto done; }

    q_normalize_rows(t);
    {
        QSet* S = qs_new();
        q_row_env_set(t, S);
        for (int i = 0; i < nas; i++) {
            if (!q_check(&R, as[i].e, S, NULL, false, false, err, sizeof(err))) { q_set_free(S); goto done; }
        }
        q_set_free(S);
    }
    if (!q_matching_rows(&R, t, where, &rows, &nrows, err, sizeof(err))) goto done;
    if (limit >= 0 && (size_t)limit < nrows) nrows = (size_t)limit;

    /* new values first, from the old rows; then all of them are written */
    {
        EnlngVal* fresh = (EnlngVal*)calloc(nrows * (size_t)nas + 1, sizeof(EnlngVal));
        QSet* S = qs_new();
        q_row_env_set(t, S);
        bool good = true;
        for (size_t r = 0; r < nrows && good; r++) {
            QEnv E = {S, t->rows[rows[r]].cells, NULL, 0, false, 0, NULL, &R};
            for (int i = 0; i < nas && good; i++) {
                fresh[r * nas + i] = q_eval(&E, as[i].e);
                if (R.err[0]) { snprintf(err, sizeof(err), "%s", R.err); good = false; }
            }
        }
        q_set_free(S);
        if (good) {
            snap = q_snap(t);
            have_snap = true;
            for (size_t r = 0; r < nrows; r++) {
                EnlngRow* row = &t->rows[rows[r]];
                for (int i = 0; i < nas; i++) {
                    enlng_free_val(&row->cells[as[i].col]);
                    row->cells[as[i].col] = enlng_copy_val(&fresh[r * nas + i]);
                }
            }
            for (size_t r = 0; r < nrows && good; r++) {
                EnlngRow* row = &t->rows[rows[r]];
                for (int c = 0; c < t->col_count && good; c++) good = q_coerce(&t->columns[c], &row->cells[c], err, sizeof(err));
                if (good) good = q_check_row(&R, t, row->cells, (int)rows[r], err, sizeof(err));
                /* a key that other tables point to cannot change */
                for (int i = 0; i < nas && good; i++) {
                    EnlngRow* old = &snap.rows[rows[r]];
                    if (enlng_compare_vals(&old->cells[as[i].col], &row->cells[as[i].col]) != 0) {
                        good = q_referenced(db, t, old->cells, as[i].col, err, sizeof(err));
                    }
                }
            }
            if (!good) {
                q_snap_restore(t, &snap);
                have_snap = false;
            }
        }
        for (size_t i = 0; i < nrows * (size_t)nas; i++) enlng_free_val(&fresh[i]);
        free(fresh);
        if (!good) goto done;
    }
    t->version++;
    if (print_output) printf("Query OK, %lu row(s) updated. (0.03 ms)\n\n", (unsigned long)nrows);
    ok = true;
done:
    if (!ok && !R.err_printed) q_print_error(print_output, err[0] ? err : (R.err[0] ? R.err : L->err), "Nothing was changed.");
    if (have_snap) q_snap_drop(&snap);
    for (int i = 0; i < nas; i++) q_free_expr(as[i].e);
    q_free_expr(where);
    free(rows);
    free(L);
    return ok;
}

/* delete [all] [records] from T [where ...] [confirmed] */
static bool q_delete(EnlngDatabase* db, const char* stmt, bool print_output) {
    QLex* L = (QLex*)malloc(sizeof(QLex));
    q_lex(L, stmt);
    QP P = {L, NM_SOFT_LIT, false, false, false, false};
    QRun R;
    memset(&R, 0, sizeof(R));
    R.db = db;
    R.print = print_output;
    char err[300] = "";
    bool ok = false;
    QExpr* where = NULL;
    size_t* rows = NULL;
    size_t nrows = 0;
    bool confirmed = false;
    EnlngTable* t = NULL;

    if (!(q_take(L, "delete") || q_take(L, "remove"))) goto done;
    while (q_take(L, "all") || q_take(L, "records") || q_take(L, "record") || q_take(L, "rows") || q_take(L, "row") || q_take(L, "the") || q_take(L, "data")) {}
    if (!(q_take(L, "from") || q_take(L, "in"))) { snprintf(err, sizeof(err), "expected 'delete from <table> where <condition>'"); goto done; }
    q_take(L, "the");
    q_take(L, "table");
    QTok* tn = q_peek(L, 0);
    if (tn->kind != QT_WORD) { snprintf(err, sizeof(err), "expected a table name after 'delete from'"); goto done; }
    t = enlngdb_get_table(db, tn->text);
    if (!t) { q_missing(&R, tn->text); goto done; }
    if (t->view_sql[0]) { snprintf(err, sizeof(err), "'%s' is a view, and rows cannot be deleted from a view", t->name); goto done; }
    L->pos++;
    for (;;) {
        if (q_take(L, "confirmed")) { confirmed = true; continue; }
        if (q_take(L, "where")) {
            if (where) { snprintf(err, sizeof(err), "only one 'where' is possible; join conditions with 'and' / 'or'"); goto done; }
            where = qp_condition(&P, NM_SOFT_LIT, false);
            if (!where) { snprintf(err, sizeof(err), "%s", L->err); goto done; }
            continue;
        }
        break;
    }
    if (q_peek(L, 0)->kind != QT_END) { snprintf(err, sizeof(err), "I don't understand '%s' here. After the table can come: where <condition>, confirmed", q_peek(L, 0)->text); goto done; }

    if (!where && !confirmed) {
        if (print_output) {
            printf("REFUSED: 'delete from %s' with no 'where' would remove every row.\n", t->name);
            printf("         Nothing was deleted. To go ahead, add the word 'confirmed':\n");
            printf("         delete all from %s confirmed;\n\n", t->name);
        }
        R.err_printed = true;
        goto done;
    }
    q_normalize_rows(t);
    if (!q_matching_rows(&R, t, where, &rows, &nrows, err, sizeof(err))) goto done;
    for (size_t i = 0; i < nrows; i++) {
        if (!q_referenced(db, t, t->rows[rows[i]].cells, -1, err, sizeof(err))) {
            snprintf(err + strlen(err), sizeof(err) - strlen(err), "; nothing was deleted");
            goto done;
        }
    }
    {
        bool* gone = (bool*)calloc(t->row_count + 1, sizeof(bool));
        for (size_t i = 0; i < nrows; i++) gone[rows[i]] = true;
        size_t w = 0;
        for (size_t r = 0; r < t->row_count; r++) {
            if (gone[r]) {
                for (int c = 0; c < t->rows[r].cell_count; c++) enlng_free_val(&t->rows[r].cells[c]);
                free(t->rows[r].cells);
            } else {
                if (w != r) t->rows[w] = t->rows[r];
                w++;
            }
        }
        t->row_count = w;
        if (nrows) t->version++;
        free(gone);
    }
    if (print_output) printf("Query OK, %lu row(s) deleted. (0.02 ms)\n\n", (unsigned long)nrows);
    ok = true;
done:
    if (!ok && !R.err_printed) q_print_error(print_output, err[0] ? err : (R.err[0] ? R.err : L->err), "");
    q_free_expr(where);
    free(rows);
    free(L);
    return ok;
}

/* truncate [table] T confirmed */
static bool q_truncate(EnlngDatabase* db, const char* stmt, bool print_output) {
    QLex* L = (QLex*)malloc(sizeof(QLex));
    q_lex(L, stmt);
    char err[300] = "";
    bool ok = false;
    q_take(L, "truncate");
    q_take(L, "table");
    QTok* tn = q_peek(L, 0);
    EnlngTable* t = tn->kind == QT_WORD ? enlngdb_get_table(db, tn->text) : NULL;
    if (tn->kind != QT_WORD) { snprintf(err, sizeof(err), "expected: truncate table <name> confirmed"); goto done; }
    if (!t) {
        QRun R;
        memset(&R, 0, sizeof(R));
        R.db = db;
        R.print = print_output;
        q_missing(&R, tn->text);
        free(L);
        return false;
    }
    L->pos++;
    if (!q_take(L, "confirmed")) {
        if (print_output) {
            printf("REFUSED: 'truncate table %s' would remove every row.\n", t->name);
            printf("         Nothing was deleted. To go ahead, add the word 'confirmed':\n");
            printf("         truncate table %s confirmed;\n\n", t->name);
        }
        free(L);
        return false;
    }
    for (size_t r = 0; r < t->row_count; r++) {
        if (!q_referenced(db, t, t->rows[r].cells, -1, err, sizeof(err))) goto done;
    }
    {
        size_t n = t->row_count;
        enlngdb_free_rows(t);
        t->version++;
        for (int c = 0; c < t->col_count; c++) t->auto_next[c] = 0;
        if (print_output) printf("Query OK, %lu row(s) deleted. (0.02 ms)\n\n", (unsigned long)n);
    }
    ok = true;
done:
    if (!ok) q_print_error(print_output, err, "Nothing was deleted.");
    free(L);
    return ok;
}

/* ---- Tables, views, indexes ------------------------------------------------------------------ */

/* the words after a column's name: a type, primary key, not null, unique, auto_increment, default, check, references */
static bool q_parse_attrs(QLex* L, int from, int to, EnlngColumn* col, char* err, size_t en) {
    int i = from;
    while (i < to) {
        QTok* t = &L->t[i];
        if (t->kind != QT_WORD) {
            snprintf(err, en, "I don't understand '%s' in the definition of column '%s'", t->text, col->name);
            return false;
        }
        const char* w = t->text;
        bool auto_col = false;
        int kind = q_kind_word(w, &auto_col);
        if (str_ci_cmp(w, "as") == 0 || str_ci_cmp(w, "null") == 0) {
            i++;
        } else if (kind >= 0) {
            col->kind = (uint8_t)kind;
            if (auto_col) col->is_auto = true;
            i++;
            if (i < to && L->t[i].kind == QT_SYM && L->t[i].text[0] == '(') {       /* varchar(20), decimal(10, 2) */
                while (i < to && !(L->t[i].kind == QT_SYM && L->t[i].text[0] == ')')) i++;
                i++;
            }
        } else if (str_ci_cmp(w, "primary") == 0 && i + 1 < to && str_ci_cmp(L->t[i + 1].text, "key") == 0) {
            col->is_primary_key = true;
            i += 2;
        } else if (str_ci_cmp(w, "not") == 0 && i + 1 < to && str_ci_cmp(L->t[i + 1].text, "null") == 0) {
            col->not_null = true;
            i += 2;
        } else if (str_ci_cmp(w, "required") == 0 || str_ci_cmp(w, "mandatory") == 0) {
            col->not_null = true;
            i++;
        } else if (str_ci_cmp(w, "unique") == 0) {
            col->is_unique = true;
            i++;
        } else if (str_ci_cmp(w, "auto_increment") == 0 || str_ci_cmp(w, "autoincrement") == 0 || str_ci_cmp(w, "auto") == 0) {
            col->is_auto = true;
            i++;
        } else if (str_ci_cmp(w, "default") == 0) {
            i++;
            if (i >= to) { snprintf(err, en, "'default' needs a value, like: default 0"); return false; }
            if (L->t[i].kind == QT_SYM && L->t[i].text[0] == '(') {
                int j = i + 1;
                while (j < to && !(L->t[j].kind == QT_SYM && L->t[j].text[0] == ')')) j++;
                q_text(L, i + 1, j, col->default_text, sizeof(col->default_text));
                i = j + 1;
            } else if (L->t[i].kind == QT_SYM && L->t[i].text[0] == '-' && i + 1 < to) {
                snprintf(col->default_text, sizeof(col->default_text), "-%s", L->t[i + 1].text);
                i += 2;
            } else {
                snprintf(col->default_text, sizeof(col->default_text), "%s", L->t[i].text);
                i++;
            }
        } else if (str_ci_cmp(w, "check") == 0) {
            i++;
            if (i < to && L->t[i].kind == QT_SYM && L->t[i].text[0] == '(') {
                int depth = 0, j = i;
                for (; j < to; j++) {
                    if (L->t[j].kind == QT_SYM && L->t[j].text[0] == '(') depth++;
                    if (L->t[j].kind == QT_SYM && L->t[j].text[0] == ')' && --depth == 0) break;
                }
                q_text(L, i + 1, j, col->check_text, sizeof(col->check_text));
                i = j + 1;
            } else {
                q_text(L, i, to, col->check_text, sizeof(col->check_text));
                i = to;
            }
            if (!col->check_text[0]) { snprintf(err, en, "'check' needs a condition, like: check (marks >= 0)"); return false; }
        } else if (str_ci_cmp(w, "references") == 0) {
            i++;
            if (i >= to || L->t[i].kind != QT_WORD) { snprintf(err, en, "'references' needs a table, like: references student(id)"); return false; }
            snprintf(col->ref_table, sizeof(col->ref_table), "%s", L->t[i].text);
            i++;
            if (i < to && L->t[i].kind == QT_SYM && L->t[i].text[0] == '(' && i + 2 < to + 1) {
                snprintf(col->ref_col, sizeof(col->ref_col), "%s", L->t[i + 1].text);
                i += 3;
            }
            if (i < to && str_ci_cmp(L->t[i].text, "on") == 0) {
                snprintf(err, en, "'on delete' / 'on update' is not supported: a row that other rows refer to simply cannot be deleted");
                return false;
            }
        } else {
            snprintf(err, en, "I don't understand '%s' in the definition of column '%s'. A column can have: a type (integer, real, text, boolean, date), primary key, not null, unique, auto_increment, default <value>, check (<condition>), references <table>(<column>)", w, col->name);
            return false;
        }
    }
    return true;
}

/* the column a 'references' points at, when it was not named: the key of that table */
static bool q_resolve_reference(EnlngDatabase* db, EnlngTable* self, EnlngColumn* col, char* err, size_t en) {
    if (!col->ref_table[0]) return true;
    EnlngTable* p = strcmp(col->ref_table, self->name) == 0 ? self : enlngdb_get_table(db, col->ref_table);
    if (!p) { snprintf(err, en, "column '%s' refers to table '%s', which does not exist", col->name, col->ref_table); return false; }
    if (p->view_sql[0]) { snprintf(err, en, "column '%s' refers to '%s', which is a view", col->name, col->ref_table); return false; }
    if (!col->ref_col[0]) {
        int keys[4];
        int nk = q_key_cols(p, keys);
        if (nk != 1) { snprintf(err, en, "column '%s' refers to '%s', which has no single key column; name the column: references %s(<column>)", col->name, col->ref_table, col->ref_table); return false; }
        snprintf(col->ref_col, sizeof(col->ref_col), "%s", p->columns[keys[0]].name);
    }
    if (q_col_index(p, col->ref_col) < 0) { snprintf(err, en, "table '%s' has no column '%s' for column '%s' to refer to", col->ref_table, col->ref_col, col->name); return false; }
    return true;
}

/* a check must read, and name only the table's columns */
static bool q_validate_check(EnlngTable* t, const char* text, char* err, size_t en) {
    QLex* L = (QLex*)malloc(sizeof(QLex));
    q_lex(L, text);
    QP P = {L, NM_SOFT_LIT, false, false, false, false};
    QExpr* e = qp_condition(&P, NM_SOFT_LIT, false);
    bool ok = e && q_peek(L, 0)->kind == QT_END;
    if (!ok) snprintf(err, en, "the check '%s' cannot be read: %s", text, L->err[0] ? L->err : "something is left over");
    if (ok) {
        QSet* S = qs_new();
        q_row_env_set(t, S);
        QRun R;
        memset(&R, 0, sizeof(R));
        ok = q_check(&R, e, S, NULL, false, false, err, en);
        q_set_free(S);
    }
    q_free_expr(e);
    free(L);
    return ok;
}

static bool q_name_ok(const char* name, char* err, size_t en) {
    if (!name[0] || strlen(name) >= ENLNGDB_MAX_NAME) { snprintf(err, en, "a name must be 1 to %d characters", ENLNGDB_MAX_NAME - 1); return false; }
    return true;
}

/* create [temporary] table [if not exists] T with a, b integer ... / (a, b ...) / as <query> */
static bool q_create_table(EnlngDatabase* db, const char* stmt, bool print_output) {
    QLex* L = (QLex*)malloc(sizeof(QLex));
    q_lex(L, stmt);
    QRun R;
    memset(&R, 0, sizeof(R));
    R.db = db;
    R.print = print_output;
    char err[300] = "";
    bool ok = false;
    EnlngColumn* cols = (EnlngColumn*)calloc(ENLNGDB_MAX_COLS + 1, sizeof(EnlngColumn));
    int ncols = 0;
    EnlngIndex uniq[ENLNGDB_MAX_INDEXES];
    int nuniq = 0;
    QQuery* sel = NULL;
    QSet* res = NULL;

    q_take(L, "create");
    bool temp = q_take(L, "temporary") || q_take(L, "temp");
    q_take(L, "table");
    bool if_not_exists = false;
    if (q_is(L, 0, "if") && q_is(L, 1, "not") && (q_is(L, 2, "exists") || q_is(L, 2, "exist"))) { L->pos += 3; if_not_exists = true; }
    QTok* nt = q_peek(L, 0);
    if (nt->kind != QT_WORD) { snprintf(err, sizeof(err), "expected a table name: create table users with id, name"); goto done; }
    char tname[64];
    snprintf(tname, sizeof(tname), "%s", nt->text);
    if (!q_name_ok(tname, err, sizeof(err))) goto done;
    L->pos++;

    if (enlngdb_get_table(db, tname)) {
        if (print_output) printf("Query OK, 0 rows affected. Table '%s' already exists. (0.01 ms)\n\n", tname);
        ok = true;
        (void)if_not_exists;
        goto done;
    }

    if (q_take(L, "as")) {
        sel = qp_query(&(QP){L, NM_SOFT_LIT, false, false, false, false});
        if (!sel) { snprintf(err, sizeof(err), "%s", L->err); goto done; }
        if (q_peek(L, 0)->kind != QT_END) { snprintf(err, sizeof(err), "I don't understand '%s' here", q_peek(L, 0)->text); goto done; }
        res = qs_new();
        if (!q_exec(&R, sel, NULL, res)) { snprintf(err, sizeof(err), "%s", R.err); goto done; }
        if (res->ncols > ENLNGDB_MAX_COLS) { snprintf(err, sizeof(err), "a table can have at most %d columns", ENLNGDB_MAX_COLS); goto done; }
        for (int c = 0; c < res->ncols; c++) {
            char nm[130];
            q_title_name(res->names[c], nm, sizeof(nm));
            for (int k = 0; k < c; k++) if (str_ci_cmp(cols[k].name, nm) == 0) { snprintf(err, sizeof(err), "two columns of the query are called '%s'; give one a name with 'as'", nm); goto done; }
            if (!q_name_ok(nm, err, sizeof(err))) goto done;
            snprintf(cols[c].name, sizeof(cols[c].name), "%s", nm);
            cols[c].type = ENLNG_VAL_STRING;
        }
        ncols = res->ncols;
    } else {
        q_take(L, "with");
        bool paren = q_take(L, "(");
        for (;;) {
            if (q_peek(L, 0)->kind == QT_END) break;
            int a = L->pos, depth = 0;
            while (q_peek(L, 0)->kind != QT_END) {
                if (q_is(L, 0, "(")) depth++;
                else if (q_is(L, 0, ")")) { if (depth == 0 && paren) break; depth--; }
                else if (q_is(L, 0, ",") && depth == 0) break;
                L->pos++;
            }
            int b = L->pos;
            if (a == b) { snprintf(err, sizeof(err), "a column definition is empty (a stray comma?)"); goto done; }
            QTok* first = &L->t[a];
            int i = a;
            if (str_ci_cmp(first->text, "constraint") == 0 && first->kind == QT_WORD) { i += 2; first = &L->t[i]; }
            bool table_level = first->kind == QT_WORD && i + 1 < b && L->t[i + 1].kind != QT_END &&
                               ((str_ci_cmp(first->text, "primary") == 0 && str_ci_cmp(L->t[i + 1].text, "key") == 0 && i + 2 < b && L->t[i + 2].text[0] == '(') ||
                                (str_ci_cmp(first->text, "unique") == 0 && L->t[i + 1].text[0] == '(') ||
                                (str_ci_cmp(first->text, "foreign") == 0 && str_ci_cmp(L->t[i + 1].text, "key") == 0) ||
                                (str_ci_cmp(first->text, "check") == 0 && L->t[i + 1].text[0] == '('));
            if (table_level) {
                bool pk = str_ci_cmp(first->text, "primary") == 0, un = str_ci_cmp(first->text, "unique") == 0, fk = str_ci_cmp(first->text, "foreign") == 0;
                if (!pk && !un && !fk) { snprintf(err, sizeof(err), "put 'check' on the column it is about: marks integer check (marks >= 0)"); goto done; }
                int j = i + (un ? 1 : 2);
                if (!(j < b && L->t[j].text[0] == '(')) { snprintf(err, sizeof(err), "expected a column list in brackets after '%s'", first->text); goto done; }
                j++;
                int named[4], nn = 0;
                for (; j < b && L->t[j].text[0] != ')'; j++) {
                    if (L->t[j].kind == QT_SYM) continue;
                    int ci = -1;
                    for (int c = 0; c < ncols; c++) if (str_ci_cmp(cols[c].name, L->t[j].text) == 0) ci = c;
                    if (ci < 0 || nn >= 4) { snprintf(err, sizeof(err), "'%s' is not one of the columns defined before it (at most 4 columns)", L->t[j].text); goto done; }
                    named[nn++] = ci;
                }
                j++;
                if (nn == 0) { snprintf(err, sizeof(err), "the column list is empty"); goto done; }
                if (pk) for (int k = 0; k < nn; k++) cols[named[k]].is_primary_key = true;
                if (un) {
                    if (nn == 1) cols[named[0]].is_unique = true;
                    else if (nuniq < ENLNGDB_MAX_INDEXES) {
                        memset(&uniq[nuniq], 0, sizeof(EnlngIndex));
                        uniq[nuniq].is_unique = true;
                        uniq[nuniq].col_count = nn;
                        snprintf(uniq[nuniq].name, sizeof(uniq[0].name), "%s_unique_%d", tname, nuniq + 1);
                        for (int k = 0; k < nn; k++) snprintf(uniq[nuniq].cols[k], sizeof(uniq[0].cols[0]), "%s", cols[named[k]].name);
                        nuniq++;
                    }
                }
                if (fk) {
                    if (nn != 1 || !(j < b && str_ci_cmp(L->t[j].text, "references") == 0)) { snprintf(err, sizeof(err), "write it like: foreign key (student_id) references student(id)"); goto done; }
                    EnlngColumn tmp;
                    memset(&tmp, 0, sizeof(tmp));
                    snprintf(tmp.name, sizeof(tmp.name), "%s", cols[named[0]].name);
                    if (!q_parse_attrs(L, j, b, &tmp, err, sizeof(err))) goto done;
                    snprintf(cols[named[0]].ref_table, sizeof(cols[0].ref_table), "%s", tmp.ref_table);
                    snprintf(cols[named[0]].ref_col, sizeof(cols[0].ref_col), "%s", tmp.ref_col);
                }
            } else {
                if (ncols >= ENLNGDB_MAX_COLS) { snprintf(err, sizeof(err), "a table can have at most %d columns", ENLNGDB_MAX_COLS); goto done; }
                if (first->kind != QT_WORD && first->kind != QT_STR) { snprintf(err, sizeof(err), "expected a column name, found '%s'", first->text); goto done; }
                if (first->kind == QT_WORD && strchr(first->text, ':')) { snprintf(err, sizeof(err), "'%s' is not a column name. Write the columns after 'with' on one line, separated by commas: create table %s with id integer, name text", first->text, tname); goto done; }
                EnlngColumn* col = &cols[ncols];
                memset(col, 0, sizeof(*col));
                EnlngVal nv = parse_val_token(first->text);
                snprintf(col->name, sizeof(col->name), "%s", nv.type == ENLNG_VAL_STRING && nv.str_val ? nv.str_val : first->text);
                enlng_free_val(&nv);
                col->type = ENLNG_VAL_STRING;
                if (!q_name_ok(col->name, err, sizeof(err))) goto done;
                for (int c = 0; c < ncols; c++) if (str_ci_cmp(cols[c].name, col->name) == 0) { snprintf(err, sizeof(err), "two columns are called '%s'", col->name); goto done; }
                if (!q_parse_attrs(L, i + 1, b, col, err, sizeof(err))) goto done;
                ncols++;
            }
            if (!q_take(L, ",")) break;
            if (q_peek(L, 0)->kind == QT_END || (paren && q_is(L, 0, ")"))) { snprintf(err, sizeof(err), "a column definition is missing after the last comma"); goto done; }
        }
        if (paren && !q_take(L, ")")) { snprintf(err, sizeof(err), "the column list has no ')'"); goto done; }
        if (q_peek(L, 0)->kind != QT_END) { snprintf(err, sizeof(err), "I don't understand '%s' here", q_peek(L, 0)->text); goto done; }
        if (ncols == 0) { snprintf(err, sizeof(err), "a table needs at least one column: create table users with id, name"); goto done; }
    }

    {
        if (db->table_count >= ENLNGDB_MAX_TABLES) { snprintf(err, sizeof(err), "the database already has the most tables it can hold (%d)", ENLNGDB_MAX_TABLES); goto done; }
        EnlngTable* t = enlngdb_create_table(db, tname, cols, ncols);
        if (!t) { snprintf(err, sizeof(err), "the table could not be created"); goto done; }
        t->is_temp = temp;
        for (int i = 0; i < nuniq; i++) t->indexes[t->index_count++] = uniq[i];
        bool good = true;
        for (int c = 0; c < t->col_count && good; c++) {
            good = q_resolve_reference(db, t, &t->columns[c], err, sizeof(err));
            if (good && t->columns[c].check_text[0]) good = q_validate_check(t, t->columns[c].check_text, err, sizeof(err));
            if (good && t->columns[c].default_text[0]) {
                EnlngVal d = parse_val_token(t->columns[c].default_text);
                good = q_coerce(&t->columns[c], &d, err, sizeof(err));
                enlng_free_val(&d);
            }
        }
        if (good && res) {
            QInRow* row = (QInRow*)malloc(sizeof(QInRow));
            for (size_t r = 0; r < res->nrows; r++) {
                EnlngVal cells[ENLNGDB_MAX_COLS];
                for (int c = 0; c < ncols; c++) cells[c] = res->rows[r][c];
                enlngdb_append_row(t, cells, ncols);
            }
            free(row);
        }
        if (!good) {
            enlngdb_drop_table(db, tname, true);
            goto done;
        }
        if (print_output) printf("Query OK, 0 rows affected. Table '%s' created with %d columns. (0.05 ms)\n\n", tname, ncols);
    }
    ok = true;
done:
    if (!ok && !R.err_printed) q_print_error(print_output, err[0] ? err : (R.err[0] ? R.err : L->err), "");
    q_set_free(res);
    q_free_query(sel);
    free(cols);
    free(L);
    return ok;
}

/* alter table T add column ... | drop column ... | rename to ... | rename column a to b | modify column c ... */
static bool q_alter(EnlngDatabase* db, const char* stmt, bool print_output, bool* handled_drop) {
    QLex* L = (QLex*)malloc(sizeof(QLex));
    q_lex(L, stmt);
    char err[300] = "";
    bool ok = false;
    QRun R;
    memset(&R, 0, sizeof(R));
    R.db = db;
    R.print = print_output;
    char tname[64] = {0}, cname[64] = {0};
    EnlngColumn col;
    memset(&col, 0, sizeof(col));
    col.type = ENLNG_VAL_STRING;
    int action = 0;     /* 1 add, 2 drop, 3 rename table, 4 rename column, 5 modify */
    char newname[64] = {0};
    *handled_drop = false;

    if (q_take(L, "alter")) {
        q_take(L, "table");
        if (q_peek(L, 0)->kind != QT_WORD) { snprintf(err, sizeof(err), "expected a table name: alter table users add column age"); goto done; }
        snprintf(tname, sizeof(tname), "%s", q_peek(L, 0)->text);
        L->pos++;
        if (q_take(L, "add")) {
            action = 1;
            q_take(L, "column");
        } else if (q_take(L, "drop") || q_take(L, "delete") || q_take(L, "remove")) {
            action = 2;
            q_take(L, "column");
        } else if (q_take(L, "rename")) {
            if (q_take(L, "column")) action = 4;
            else if (q_take(L, "to")) action = 3;
            else if (q_peek(L, 1)->kind == QT_WORD && q_is(L, 1, "to")) action = 4;
            else action = 3;
        } else if (q_take(L, "modify") || q_take(L, "alter") || q_take(L, "change")) {
            action = 5;
            q_take(L, "column");
        } else {
            snprintf(err, sizeof(err), "after the table write: add column, drop column, rename to, rename column or modify column");
            goto done;
        }
        if (action == 3) {
            if (q_peek(L, 0)->kind != QT_WORD) { snprintf(err, sizeof(err), "expected the new name of the table"); goto done; }
            snprintf(newname, sizeof(newname), "%s", q_peek(L, 0)->text);
            L->pos++;
        } else {
            if (q_peek(L, 0)->kind != QT_WORD) { snprintf(err, sizeof(err), "expected a column name"); goto done; }
            snprintf(cname, sizeof(cname), "%s", q_peek(L, 0)->text);
            L->pos++;
            if (action == 4) {
                if (!q_take(L, "to") || q_peek(L, 0)->kind != QT_WORD) { snprintf(err, sizeof(err), "write it like: alter table %s rename column %s to new_name", tname, cname); goto done; }
                snprintf(newname, sizeof(newname), "%s", q_peek(L, 0)->text);
                L->pos++;
            }
        }
    } else {
        /* add column <c> to <t> [as <kind>] [default <v>] */
        q_take(L, "add");
        q_take(L, "column");
        if (q_peek(L, 0)->kind != QT_WORD) { snprintf(err, sizeof(err), "expected a column name: add column age to student"); goto done; }
        snprintf(cname, sizeof(cname), "%s", q_peek(L, 0)->text);
        L->pos++;
        if (!(q_take(L, "to") || q_take(L, "in")) || q_peek(L, 0)->kind != QT_WORD) { snprintf(err, sizeof(err), "write it like: add column age to student"); goto done; }
        snprintf(tname, sizeof(tname), "%s", q_peek(L, 0)->text);
        L->pos++;
        action = 1;
    }
    if (action == 1 || action == 5) {
        snprintf(col.name, sizeof(col.name), "%s", cname);
        if (!q_name_ok(cname, err, sizeof(err))) goto done;
        if (!q_parse_attrs(L, L->pos, L->n, &col, err, sizeof(err))) goto done;
        L->pos = L->n;
    }
    if (action == 2) q_take(L, "confirmed");
    if (q_peek(L, 0)->kind != QT_END) { snprintf(err, sizeof(err), "I don't understand '%s' here", q_peek(L, 0)->text); goto done; }

    EnlngTable* t = enlngdb_get_table(db, tname);
    if (!t) { q_missing(&R, tname); goto done; }
    if (t->view_sql[0]) { snprintf(err, sizeof(err), "'%s' is a view; change it with 'create or replace view'", t->name); goto done; }

    if (action == 2) {
        for (int i = 0; i < db->table_count; i++) {
            for (int c = 0; c < db->tables[i].col_count; c++) {
                if (strcmp(db->tables[i].columns[c].ref_table, t->name) == 0 && str_ci_cmp(db->tables[i].columns[c].ref_col, cname) == 0) {
                    snprintf(err, sizeof(err), "column '%s' cannot be deleted: %s.%s refers to it", cname, db->tables[i].name, db->tables[i].columns[c].name);
                    goto done;
                }
            }
        }
        *handled_drop = true;
        free(L);
        return false;       /* the caller deletes the column the way 'delete column' does */
    }
    if (action == 3) {
        if (!q_name_ok(newname, err, sizeof(err))) goto done;
        if (enlngdb_get_table(db, newname)) { snprintf(err, sizeof(err), "a table called '%s' already exists", newname); goto done; }
        enlngdb_rename_table(db, tname, newname);
        if (print_output) printf("Query OK, table '%s' renamed to '%s'. (0.02 ms)\n\n", tname, newname);
        ok = true;
        goto done;
    }
    if (action == 4) {
        if (q_col_index(t, cname) < 0) { snprintf(err, sizeof(err), "table '%s' has no column '%s'", tname, cname); goto done; }
        if (!q_name_ok(newname, err, sizeof(err))) goto done;
        if (q_col_index(t, newname) >= 0) { snprintf(err, sizeof(err), "table '%s' already has a column called '%s'", tname, newname); goto done; }
        for (int i = 0; i < db->table_count; i++) {
            for (int c = 0; c < db->tables[i].col_count; c++) {
                if (strcmp(db->tables[i].columns[c].ref_table, t->name) == 0 && str_ci_cmp(db->tables[i].columns[c].ref_col, cname) == 0) {
                    snprintf(db->tables[i].columns[c].ref_col, sizeof(db->tables[i].columns[c].ref_col), "%s", newname);
                }
            }
        }
        enlngdb_rename_column(t, t->columns[q_col_index(t, cname)].name, newname);
        if (print_output) printf("Query OK, column '%s' of '%s' is now called '%s'. (0.02 ms)\n\n", cname, tname, newname);
        ok = true;
        goto done;
    }
    if (action == 1) {
        if (q_col_index(t, cname) >= 0) { snprintf(err, sizeof(err), "Table '%s' already has a column called '%s'", tname, cname); goto done; }
        if (t->col_count >= ENLNGDB_MAX_COLS) { snprintf(err, sizeof(err), "Table '%s' already has %d columns, the most a table can have", tname, ENLNGDB_MAX_COLS); goto done; }
        EnlngTable tmp_cols;
        (void)tmp_cols;
        QSnap snap = q_snap(t);
        EnlngVal def = col.default_text[0] ? parse_val_token(col.default_text) : enlng_null();
        bool good = true;
        if (col.default_text[0]) good = q_coerce(&col, &def, err, sizeof(err));
        if (good && col.not_null && def.type == ENLNG_VAL_NULL && t->row_count > 0) {
            snprintf(err, sizeof(err), "column '%s' is 'not null', so rows that are already there need a value: add 'default <value>'", cname);
            good = false;
        }
        if (good) {
            enlngdb_add_column(t, cname, &def);
            int idx = t->col_count - 1;
            EnlngColumn keep = col;
            t->columns[idx] = keep;
            t->columns[idx].type = ENLNG_VAL_STRING;
            good = q_resolve_reference(db, t, &t->columns[idx], err, sizeof(err));
            if (good && col.check_text[0]) good = q_validate_check(t, col.check_text, err, sizeof(err));
            if (good && col.is_auto) {
                int64_t n = 1;
                for (size_t r = 0; r < t->row_count; r++) { enlng_free_val(&t->rows[r].cells[idx]); t->rows[r].cells[idx] = enlng_int(n++); }
                t->auto_next[idx] = n;
            }
            for (size_t r = 0; r < t->row_count && good; r++) good = q_check_row(&R, t, t->rows[r].cells, (int)r, err, sizeof(err));
            if (!good) {
                q_snap_restore(t, &snap);
                t->col_count--;
                memset(&t->columns[t->col_count], 0, sizeof(EnlngColumn));
                for (size_t r = 0; r < t->row_count; r++) {
                    if (t->rows[r].cell_count > t->col_count) { enlng_free_val(&t->rows[r].cells[t->col_count]); t->rows[r].cell_count = t->col_count; }
                }
            }
        }
        q_snap_drop(&snap);
        enlng_free_val(&def);
        if (!good) goto done;
        if (print_output) printf("Query OK, column '%s' added to table '%s'. (0.02 ms)\n\n", cname, tname);
        ok = true;
        goto done;
    }
    if (action == 5) {
        int ci = q_col_index(t, cname);
        if (ci < 0) { snprintf(err, sizeof(err), "table '%s' has no column '%s'", tname, cname); goto done; }
        EnlngColumn old = t->columns[ci];
        QSnap snap = q_snap(t);
        t->columns[ci] = col;
        snprintf(t->columns[ci].name, sizeof(t->columns[ci].name), "%s", old.name);
        t->columns[ci].type = ENLNG_VAL_STRING;
        bool good = q_resolve_reference(db, t, &t->columns[ci], err, sizeof(err));
        if (good && col.check_text[0]) good = q_validate_check(t, col.check_text, err, sizeof(err));
        q_normalize_rows(t);
        for (size_t r = 0; r < t->row_count && good; r++) good = q_coerce(&t->columns[ci], &t->rows[r].cells[ci], err, sizeof(err));
        for (size_t r = 0; r < t->row_count && good; r++) good = q_check_row(&R, t, t->rows[r].cells, (int)r, err, sizeof(err));
        if (!good) {
            q_snap_restore(t, &snap);
            t->columns[ci] = old;
        }
        q_snap_drop(&snap);
        if (!good) { snprintf(err + strlen(err), sizeof(err) - strlen(err), "; the column was not changed"); goto done; }
        if (print_output) printf("Query OK, column '%s' of '%s' is now %s. (0.02 ms)\n\n", cname, tname, q_kind_name(t->columns[ci].kind));
        ok = true;
    }
done:
    if (!ok && !*handled_drop && !R.err_printed) q_print_error(print_output, err[0] ? err : (R.err[0] ? R.err : L->err), "");
    free(L);
    return ok;
}

/* create [unique] index [name] on T (a, b) */
static bool q_create_index(EnlngDatabase* db, const char* stmt, bool print_output) {
    QLex* L = (QLex*)malloc(sizeof(QLex));
    q_lex(L, stmt);
    char err[300] = "";
    bool ok = false;
    QRun R;
    memset(&R, 0, sizeof(R));
    R.db = db;
    R.print = print_output;
    EnlngIndex ix;
    memset(&ix, 0, sizeof(ix));
    q_take(L, "create");
    ix.is_unique = q_take(L, "unique");
    q_take(L, "index");
    if (q_peek(L, 0)->kind == QT_WORD && !q_is(L, 0, "on")) {
        snprintf(ix.name, sizeof(ix.name), "%s", q_peek(L, 0)->text);
        L->pos++;
    }
    if (!q_take(L, "on")) { snprintf(err, sizeof(err), "write it like: create index on student (name)"); goto done; }
    if (q_peek(L, 0)->kind != QT_WORD) { snprintf(err, sizeof(err), "expected a table name after 'on'"); goto done; }
    EnlngTable* t = enlngdb_get_table(db, q_peek(L, 0)->text);
    if (!t) { q_missing(&R, q_peek(L, 0)->text); goto done; }
    L->pos++;
    if (t->view_sql[0]) { snprintf(err, sizeof(err), "'%s' is a view, and a view cannot have an index", t->name); goto done; }
    if (!q_take(L, "(")) { snprintf(err, sizeof(err), "name the columns in brackets: create index on %s (name)", t->name); goto done; }
    for (;;) {
        QTok* c = q_peek(L, 0);
        if (c->kind != QT_WORD) { snprintf(err, sizeof(err), "expected a column name"); goto done; }
        int ci = q_col_index(t, c->text);
        if (ci < 0) { snprintf(err, sizeof(err), "table '%s' has no column '%s'", t->name, c->text); goto done; }
        if (ix.col_count >= 4) { snprintf(err, sizeof(err), "an index can cover at most 4 columns"); goto done; }
        snprintf(ix.cols[ix.col_count++], sizeof(ix.cols[0]), "%s", t->columns[ci].name);
        L->pos++;
        if (!q_take(L, ",")) break;
    }
    if (!q_take(L, ")")) { snprintf(err, sizeof(err), "the column list has no ')'"); goto done; }
    if (q_peek(L, 0)->kind != QT_END) { snprintf(err, sizeof(err), "I don't understand '%s' here", q_peek(L, 0)->text); goto done; }
    if (!ix.name[0]) {
        snprintf(ix.name, sizeof(ix.name), "%s_%s_idx", t->name, ix.cols[0]);
    }
    for (int i = 0; i < db->table_count; i++) {
        for (int k = 0; k < db->tables[i].index_count; k++) {
            if (str_ci_cmp(db->tables[i].indexes[k].name, ix.name) == 0) { snprintf(err, sizeof(err), "an index called '%s' already exists", ix.name); goto done; }
        }
    }
    if (t->index_count >= ENLNGDB_MAX_INDEXES) { snprintf(err, sizeof(err), "a table can have at most %d indexes", ENLNGDB_MAX_INDEXES); goto done; }
    if (ix.is_unique) {
        /* the rows that are there must already be different */
        for (size_t a = 0; a < t->row_count; a++) {
            for (size_t b = a + 1; b < t->row_count; b++) {
                bool same = true, any_null = false;
                for (int k = 0; k < ix.col_count && same; k++) {
                    int c = q_col_index(t, ix.cols[k]);
                    if (t->rows[a].cells[c].type == ENLNG_VAL_NULL) any_null = true;
                    same = enlng_compare_vals(&t->rows[a].cells[c], &t->rows[b].cells[c]) == 0;
                }
                if (same && !any_null) { snprintf(err, sizeof(err), "a unique index cannot be made: rows %lu and %lu of '%s' have the same value", (unsigned long)a + 1, (unsigned long)b + 1, t->name); goto done; }
            }
        }
    }
    t->indexes[t->index_count++] = ix;
    t->version++;
    if (print_output) {
        printf("Query OK, %sindex '%s' made on %s (", ix.is_unique ? "unique " : "", ix.name, t->name);
        for (int k = 0; k < ix.col_count; k++) printf("%s%s", k ? ", " : "", ix.cols[k]);
        printf("). (0.02 ms)\n");
        if (!ix.is_unique) printf("       Note: EnlngDB reads tables from memory, so a plain index is recorded but does not change how fast queries run.\n");
        printf("\n");
    }
    ok = true;
done:
    if (!ok && !R.err_printed) q_print_error(print_output, err[0] ? err : L->err, "");
    free(L);
    return ok;
}

/* create [or replace] view V as <query> */
static bool q_create_view(EnlngDatabase* db, const char* stmt, bool print_output) {
    char err[300] = "";
    bool ok = false;
    const char* p = stmt + 6;               /* after "create " */
    while (isspace((unsigned char)*p)) p++;
    bool replace = false;
    if (str_starts_with_ci(p, "or replace ")) { replace = true; p += 11; }
    while (isspace((unsigned char)*p)) p++;
    p += 4;                                  /* "view" */
    while (isspace((unsigned char)*p)) p++;
    char name[64] = {0};
    size_t k = 0;
    while (*p && !isspace((unsigned char)*p) && k < sizeof(name) - 1) name[k++] = *p++;
    while (isspace((unsigned char)*p)) p++;
    if (!name[0]) { snprintf(err, sizeof(err), "expected a view name: create view top_students as find ..."); goto done; }
    if (!str_starts_with_ci(p, "as ") ) { snprintf(err, sizeof(err), "write it like: create view %s as find name from student where marks is at least 80", name); goto done; }
    p += 3;
    while (isspace((unsigned char)*p)) p++;
    if (strlen(p) >= sizeof(((EnlngTable*)0)->view_sql)) { snprintf(err, sizeof(err), "the query of a view can be at most %d characters", (int)sizeof(((EnlngTable*)0)->view_sql) - 1); goto done; }
    if (!q_name_ok(name, err, sizeof(err))) goto done;
    EnlngTable* existing = enlngdb_get_table(db, name);
    if (existing && !(replace && existing->view_sql[0])) {
        snprintf(err, sizeof(err), existing->view_sql[0] ? "a view called '%s' already exists; use 'create or replace view' to change it" : "a table called '%s' already exists", name);
        goto done;
    }
    /* run it once, so a wrong view is found now and not later */
    {
        QRun R;
        memset(&R, 0, sizeof(R));
        R.db = db;
        R.print = false;
        QSet* res = qs_new();
        char keep[1024] = "";
        if (existing) { snprintf(keep, sizeof(keep), "%s", existing->view_sql); existing->view_sql[0] = '\0'; }
        bool good = q_exec_text(&R, p, NULL, res);
        if (existing) snprintf(existing->view_sql, sizeof(existing->view_sql), "%s", keep);
        q_set_free(res);
        if (!good) { snprintf(err, sizeof(err), "the query of the view does not work: %s", R.err); goto done; }
    }
    if (!existing) {
        if (db->table_count >= ENLNGDB_MAX_TABLES) { snprintf(err, sizeof(err), "the database already has the most tables it can hold (%d)", ENLNGDB_MAX_TABLES); goto done; }
        existing = enlngdb_create_table(db, name, NULL, 0);
    }
    snprintf(existing->view_sql, sizeof(existing->view_sql), "%s", p);
    existing->version++;
    if (print_output) printf("Query OK, view '%s' %s. (0.02 ms)\n\n", name, replace ? "saved" : "created");
    ok = true;
done:
    if (!ok) q_print_error(print_output, err, "");
    return ok;
}

/* a result made of text only, shown as a table */
static void q_show_text_table(const char** heads, int nheads, char cells[][6][130], int nrows) {
    QSet* O = qs_new();
    O->ncols = nheads;
    for (int c = 0; c < nheads; c++) snprintf(O->names[c], sizeof(O->names[0]), "%s", heads[c]);
    for (int r = 0; r < nrows; r++) {
        EnlngVal* row = q_row_new(nheads);
        for (int c = 0; c < nheads; c++) row[c] = enlng_str(cells[r][c]);
        q_set_add(O, row);
    }
    q_show_set(O, 0.01, true);
    q_set_free(O);
}

/* describe T / show columns from T */
static bool q_describe(EnlngDatabase* db, const char* name, bool print_output) {
    EnlngTable* t = enlngdb_get_table(db, name);
    QRun R;
    memset(&R, 0, sizeof(R));
    R.db = db;
    R.print = print_output;
    if (!t) { q_missing(&R, name); return false; }
    if (!print_output) return true;
    static char cells[ENLNGDB_MAX_COLS + 1][6][130];
    int n = 0;
    if (t->view_sql[0]) {
        QSet* res = qs_new();
        bool ok = q_exec_text(&R, t->view_sql, NULL, res);
        if (!ok) { q_print_error(print_output, R.err, ""); q_set_free(res); return false; }
        for (int c = 0; c < res->ncols && n < ENLNGDB_MAX_COLS; c++, n++) {
            q_title_name(res->names[c], cells[n][0], sizeof(cells[0][0]));
            snprintf(cells[n][1], 130, "(view)");
            cells[n][2][0] = cells[n][3][0] = cells[n][4][0] = cells[n][5][0] = '\0';
        }
        q_set_free(res);
    } else {
        int keys[4];
        int nk = q_key_cols(t, keys);
        for (int c = 0; c < t->col_count; c++, n++) {
            const EnlngColumn* col = &t->columns[c];
            snprintf(cells[n][0], 130, "%s", col->name);
            snprintf(cells[n][1], 130, "%s", q_kind_name(col->kind));
            snprintf(cells[n][2], 130, "%s", col->not_null ? "no" : "yes");
            bool is_key = false;
            for (int k = 0; k < nk; k++) if (keys[k] == c) is_key = true;
            const char* key = is_key ? (q_has_declared_key(t) ? "primary" : "id") : (col->is_unique ? "unique" : "");
            snprintf(cells[n][3], 130, "%s", key);
            snprintf(cells[n][4], 130, "%s", col->default_text);
            cells[n][5][0] = '\0';
            size_t k = 0;
            if (col->is_auto) k += (size_t)snprintf(cells[n][5] + k, 130 - k, "auto_increment ");
            if (col->ref_table[0]) k += (size_t)snprintf(cells[n][5] + k, 130 - k, "refers to %s.%s ", col->ref_table, col->ref_col);
            if (col->check_text[0]) k += (size_t)snprintf(cells[n][5] + k, 130 - k, "check (%s)", col->check_text);
        }
    }
    const char* heads[] = {"Column", "Type", "Null", "Key", "Default", "Notes"};
    q_show_text_table(heads, 6, cells, n);
    return true;
}

static bool q_show_indexes(EnlngDatabase* db, const char* only, bool print_output) {
    if (!print_output) return true;
    static char cells[ENLNGDB_MAX_TABLES * (ENLNGDB_MAX_INDEXES + 1)][6][130];
    int n = 0;
    for (int i = 0; i < db->table_count && n < ENLNGDB_MAX_TABLES * ENLNGDB_MAX_INDEXES; i++) {
        EnlngTable* t = &db->tables[i];
        if (t->view_sql[0] || (only && only[0] && strcmp(only, t->name) != 0)) continue;
        int keys[4];
        int nk = q_key_cols(t, keys);
        if (nk > 0) {
            snprintf(cells[n][0], 130, "%s", t->name);
            snprintf(cells[n][1], 130, "%s", q_has_declared_key(t) ? "(primary key)" : "(id)");
            cells[n][2][0] = '\0';
            for (int k = 0; k < nk; k++) { size_t l = strlen(cells[n][2]); snprintf(cells[n][2] + l, 130 - l, "%s%s", k ? ", " : "", t->columns[keys[k]].name); }
            snprintf(cells[n][3], 130, "yes");
            n++;
        }
        for (int k = 0; k < t->index_count; k++) {
            snprintf(cells[n][0], 130, "%s", t->name);
            snprintf(cells[n][1], 130, "%s", t->indexes[k].name);
            cells[n][2][0] = '\0';
            for (int j = 0; j < t->indexes[k].col_count; j++) { size_t l = strlen(cells[n][2]); snprintf(cells[n][2] + l, 130 - l, "%s%s", j ? ", " : "", t->indexes[k].cols[j]); }
            snprintf(cells[n][3], 130, "%s", t->indexes[k].is_unique ? "yes" : "no");
            n++;
        }
    }
    const char* heads[] = {"Table", "Index", "Columns", "Unique"};
    if (n == 0) { printf("No indexes.\n\n"); return true; }
    q_show_text_table(heads, 4, cells, n);
    return true;
}

static bool q_show_views(EnlngDatabase* db, bool print_output) {
    if (!print_output) return true;
    static char cells[ENLNGDB_MAX_TABLES][6][130];
    int n = 0;
    for (int i = 0; i < db->table_count; i++) {
        if (!db->tables[i].view_sql[0]) continue;
        snprintf(cells[n][0], 130, "%s", db->tables[i].name);
        snprintf(cells[n][1], 130, "%.120s", db->tables[i].view_sql);
        n++;
    }
    if (n == 0) { printf("No views.\n\n"); return true; }
    const char* heads[] = {"View", "Query"};
    q_show_text_table(heads, 2, cells, n);
    return true;
}

/* ---- Database files ---- */

static bool q_db_file_exists(const char* name, char* path, size_t n) {
    snprintf(path, n, "%s.edb", name);
    FILE* f = fopen(path, "rb");
    if (!f) {
        snprintf(path, n, "tests/%s.edb", name);
        f = fopen(path, "rb");
    }
    if (f) fclose(f);
    return f != NULL;
}

#define Q_MAX_STATEMENT 8192

bool enlngdb_execute_statement(EnlngDatabase* db, const char* statement, bool print_output) {
    if (!db || !statement) return false;
    if (strlen(statement) >= Q_MAX_STATEMENT) {
        q_print_error(print_output, "this statement is too long (the most is 8191 characters)", "");
        return false;
    }
    char line[Q_MAX_STATEMENT];
    strcpy(line, statement);

    char* stmt = trim(line);
    // Remove trailing semicolon
    size_t slen = strlen(stmt);
    while (slen > 0 && stmt[slen - 1] == ';') {
        stmt[--slen] = '\0';
    }
    stmt = trim(stmt);
    // "please find ..." / "kindly find ...": the polite word is skipped
    while (str_starts_with_ci(stmt, "please ") || str_starts_with_ci(stmt, "kindly ")) stmt = trim(stmt + 7);
    if (strlen(stmt) == 0) return true;

    // transactions
    if (str_ci_cmp(stmt, "begin") == 0 || str_ci_cmp(stmt, "begin transaction") == 0 || str_ci_cmp(stmt, "start transaction") == 0) {
        if (db->txn) { q_print_error(print_output, "a transaction is already open; 'commit' or 'rollback' it first", ""); return false; }
        if (!enlngdb_begin(db)) { q_print_error(print_output, "a transaction could not be started (out of memory)", ""); return false; }
        if (print_output) printf("Query OK, transaction started. 'commit' keeps what follows, 'rollback' undoes it.\n\n");
        return true;
    }
    if (str_ci_cmp(stmt, "commit") == 0 || str_ci_cmp(stmt, "commit transaction") == 0 || str_ci_cmp(stmt, "end transaction") == 0) {
        if (!db->txn) { q_print_error(print_output, "there is no open transaction to commit; start one with 'begin'", ""); return false; }
        enlngdb_commit(db);
        if (print_output) printf("Query OK, transaction committed.\n\n");
        return true;
    }
    if (str_ci_cmp(stmt, "rollback") == 0 || str_ci_cmp(stmt, "rollback transaction") == 0) {
        if (!db->txn) { q_print_error(print_output, "there is no open transaction to roll back; start one with 'begin'", ""); return false; }
        enlngdb_rollback(db);
        if (print_output) printf("Query OK, transaction rolled back: everything since 'begin' is undone.\n\n");
        return true;
    }

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

    // create database <name> [if not exists]
    if (str_starts_with_ci(stmt, "create database ")) {
        char name[64] = {0};
        bool if_not = strstr(stmt, "if not exist") != NULL || strstr(stmt, "IF NOT EXIST") != NULL;
        const char* p = stmt + 16;
        while (isspace((unsigned char)*p)) p++;
        if (str_starts_with_ci(p, "if not exists ")) p += 14;
        else if (str_starts_with_ci(p, "if not exist ")) p += 13;
        if (sscanf(p, "%63[^; \t\r\n]", name) != 1) { q_print_error(print_output, "expected a name: create database shop", ""); return false; }
        char path[256];
        if (q_db_file_exists(name, path, sizeof(path)) || strcmp(db->name, name) == 0) {
            if (if_not) { if (print_output) printf("Query OK, database '%s' already exists. (0.01 ms)\n\n", name); return true; }
            q_print_error(print_output, "a database with this name already exists", "Open it with: use database <name>");
            return false;
        }
        enlngdb_clear(db);
        snprintf(db->name, sizeof(db->name), "%s", name);
        db->dropped = false;
        if (print_output) printf("Query OK, database '%s' created and now in use. (0.01 ms)\n\n", name);
        return true;
    }

    // drop database <name> confirmed
    if (str_starts_with_ci(stmt, "drop database ")) {
        char name[64] = {0};
        if (sscanf(stmt + 14, "%63[^; \t\r\n]", name) != 1) { q_print_error(print_output, "expected a name: drop database shop confirmed", ""); return false; }
        bool confirmed = strstr(stmt, " confirmed") != NULL || strstr(stmt, " CONFIRMED") != NULL;
        char path[256];
        bool has_file = q_db_file_exists(name, path, sizeof(path));
        bool is_open = strcmp(db->name, name) == 0;
        if (!has_file && !is_open) { q_print_error(print_output, "there is no database with this name", ""); return false; }
        if (!confirmed) {
            if (print_output) {
                printf("REFUSED: dropping database '%s' deletes all its tables and rows.\n", name);
                printf("         Nothing was changed. To go ahead, add the word 'confirmed':\n");
                printf("         drop database %s confirmed;\n\n", name);
            }
            return false;
        }
        if (has_file) remove(path);
        if (is_open) {
            enlngdb_clear(db);
            db->dropped = true;
        }
        if (print_output) printf("Query OK, database '%s' dropped. (0.02 ms)\n\n", name);
        return true;
    }

    // show indexes / show views / show columns from T
    if (str_starts_with_ci(stmt, "show indexes") || str_starts_with_ci(stmt, "show indices") || str_starts_with_ci(stmt, "show index")) {
        char only[64] = {0};
        const char* from = strstr(stmt, " from ");
        if (!from) from = strstr(stmt, " in ");
        if (from) sscanf(from + (from[1] == 'f' ? 6 : 4), "%63[^; \t\r\n]", only);
        return q_show_indexes(db, only, print_output);
    }
    if (str_starts_with_ci(stmt, "show views")) return q_show_views(db, print_output);
    if (str_starts_with_ci(stmt, "show columns ") || str_starts_with_ci(stmt, "show fields ")) {
        char name[64] = {0};
        const char* p = stmt + 13;
        if (str_starts_with_ci(p, "from ")) p += 5;
        else if (str_starts_with_ci(p, "in ")) p += 3;
        if (sscanf(p, "%63[^; \t\r\n]", name) != 1) { q_print_error(print_output, "write it like: show columns from student", ""); return false; }
        return q_describe(db, name, print_output);
    }
    if (str_starts_with_ci(stmt, "describe ") || str_starts_with_ci(stmt, "desc ")) {
        char name[64] = {0};
        const char* p = stmt + (str_starts_with_ci(stmt, "describe ") ? 9 : 5);
        if (str_starts_with_ci(p, "table ")) p += 6;
        if (sscanf(p, "%63[^; \t\r\n]", name) != 1) { q_print_error(print_output, "write it like: describe student", ""); return false; }
        return q_describe(db, name, print_output);
    }

    // 4. create table / view / index
    if (str_starts_with_ci(stmt, "create ")) {
        const char* p = stmt + 7;
        while (isspace((unsigned char)*p)) p++;
        if (str_starts_with_ci(p, "or replace view ") || str_starts_with_ci(p, "view ")) return q_create_view(db, stmt, print_output);
        if (str_starts_with_ci(p, "index ") || str_starts_with_ci(p, "unique index ")) return q_create_index(db, stmt, print_output);
        if (str_starts_with_ci(p, "table ") || str_starts_with_ci(p, "temporary table ") || str_starts_with_ci(p, "temp table ")) return q_create_table(db, stmt, print_output);
    }

    // 5. insert into <name> with ... / values (...) / find ...
    if (str_starts_with_ci(stmt, "insert ")) {
        const char* p = stmt + 7;
        while (isspace((unsigned char)*p)) p++;
        if (str_starts_with_ci(p, "record ")) p += 7;
        else if (str_starts_with_ci(p, "records ")) p += 8;
        else if (str_starts_with_ci(p, "row ")) p += 4;
        else if (str_starts_with_ci(p, "rows ")) p += 5;
        if (str_starts_with_ci(p, "into ")) {
            p += 5;
            while (isspace((unsigned char)*p)) p++;
            if (str_starts_with_ci(p, "table ")) p += 6;
            while (isspace((unsigned char)*p) ) p++;
            while (*p && !isspace((unsigned char)*p) && *p != '(') p++;
            while (isspace((unsigned char)*p)) p++;
            if (str_starts_with_ci(p, "with ") && (str_starts_with_ci(stmt, "insert into ") || str_starts_with_ci(stmt, "insert record into "))) return q_insert_with(db, stmt, print_output);
            return q_insert_values(db, stmt, print_output);
        }
    }

    // 6. find / fetch / select / get / show all records from, count records in, with ... find ..., explain ...
    if (str_starts_with_ci(stmt, "find ") || str_starts_with_ci(stmt, "fetch ") || str_starts_with_ci(stmt, "select ") ||
        str_starts_with_ci(stmt, "get ") || str_starts_with_ci(stmt, "show all ") || str_starts_with_ci(stmt, "count ") ||
        str_starts_with_ci(stmt, "with ")) {
        return run_query(db, stmt, print_output, false);
    }
    if (str_starts_with_ci(stmt, "explain ")) {
        const char* p = stmt + 8;
        while (isspace((unsigned char)*p)) p++;
        if (str_starts_with_ci(p, "plan ")) p += 5;
        return run_query(db, p, print_output, true);
    }

    // alter table <t> add [column] <c> ... / add column <c> to <t> ... / rename / modify
    // alter table <t> drop|delete|remove column <c>
    if (str_starts_with_ci(stmt, "alter table ") || str_starts_with_ci(stmt, "add column ")) {
        bool drop = false;
        bool ok = q_alter(db, stmt, print_output, &drop);
        if (!drop) return ok;
        char tname[64] = {0}, cname[64] = {0};
        char word[16] = {0}, word2[16] = {0};
        sscanf(stmt, "%*s %*s %63s %15s %15s %63s", tname, word, word2, cname);
        if (str_ci_cmp(word2, "column") != 0) snprintf(cname, sizeof(cname), "%s", word2);
        char del_stmt[256];
        snprintf(del_stmt, sizeof(del_stmt), "delete column %s from %s", cname, tname);
        return enlngdb_execute_statement(db, del_stmt, print_output);
    }
    if (str_starts_with_ci(stmt, "rename table ") || str_starts_with_ci(stmt, "rename ")) {
        char a[64] = {0}, b[64] = {0};
        const char* p = stmt + (str_starts_with_ci(stmt, "rename table ") ? 13 : 7);
        if (sscanf(p, "%63s to %63[^; \t\r\n]", a, b) == 2) {
            char alter[256];
            snprintf(alter, sizeof(alter), "alter table %s rename to %s", a, b);
            bool drop = false;
            return q_alter(db, alter, print_output, &drop);
        }
        q_print_error(print_output, "write it like: rename table student to pupils", "");
        return false;
    }

    // truncate table <name> confirmed
    if (str_starts_with_ci(stmt, "truncate ")) return q_truncate(db, stmt, print_output);

    // drop view / drop index
    if (str_starts_with_ci(stmt, "drop view ")) {
        char name[64] = {0};
        sscanf(stmt + 10, "%63[^; \t\r\n]", name);
        EnlngTable* t = enlngdb_get_table(db, name);
        if (!t || !t->view_sql[0]) { q_print_error(print_output, t ? "this is a table, not a view; use 'drop table ... confirmed'" : "there is no view with this name", ""); return false; }
        enlngdb_drop_table(db, name, true);
        if (print_output) printf("Query OK, view '%s' dropped. (0.02 ms)\n\n", name);
        return true;
    }
    if (str_starts_with_ci(stmt, "drop index ")) {
        char name[64] = {0};
        sscanf(stmt + 11, "%63[^; \t\r\n]", name);
        for (int i = 0; i < db->table_count; i++) {
            EnlngTable* t = &db->tables[i];
            for (int k = 0; k < t->index_count; k++) {
                if (str_ci_cmp(t->indexes[k].name, name) != 0) continue;
                for (int j = k; j < t->index_count - 1; j++) t->indexes[j] = t->indexes[j + 1];
                t->index_count--;
                t->version++;
                if (print_output) printf("Query OK, index '%s' dropped. (0.02 ms)\n\n", name);
                return true;
            }
        }
        q_print_error(print_output, "there is no index with this name", "See them with: show indexes");
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

    // delete / remove records from <table> [where ...] [confirmed]
    if (str_starts_with_ci(stmt, "delete ") || str_starts_with_ci(stmt, "remove ")) return q_delete(db, stmt, print_output);

    // 8. drop table <name> confirmed
    if (str_starts_with_ci(stmt, "drop table ")) {
        char tbl_name[64];
        bool confirmed = (strstr(stmt, " confirmed") != NULL || strstr(stmt, " CONFIRMED") != NULL);
        if (sscanf(stmt + 11, "%63s", tbl_name) == 1) {
            EnlngTable* t = enlngdb_get_table(db, tbl_name);
            if (t && t->view_sql[0]) { q_print_error(print_output, "this is a view, not a table; use 'drop view <name>'", ""); return false; }
            if (!confirmed && t) {
                if (print_output) {
                    printf("REFUSED: dropping table '%s' deletes the table and all its rows.\n", tbl_name);
                    printf("         Nothing was changed. To go ahead, add the word 'confirmed':\n");
                    printf("         drop table %s confirmed;\n\n", tbl_name);
                }
                return false;
            }
            if (t) {
                for (int i = 0; i < db->table_count; i++) {
                    for (int c = 0; c < db->tables[i].col_count; c++) {
                        if (strcmp(db->tables[i].columns[c].ref_table, t->name) == 0 && &db->tables[i] != t) {
                            char msg[200];
                            snprintf(msg, sizeof(msg), "table '%s' cannot be dropped: %s.%s refers to it", tbl_name, db->tables[i].name, db->tables[i].columns[c].name);
                            q_print_error(print_output, msg, "Drop that table first, or the column that refers to it.");
                            return false;
                        }
                    }
                }
            }
            bool ok = enlngdb_drop_table(db, tbl_name, confirmed);
            if (ok && print_output) {
                printf("Query OK, table '%s' dropped successfully. (0.02 ms)\n\n", tbl_name);
            } else if (!ok && print_output && !t) {
                QRun R;
                memset(&R, 0, sizeof(R));
                R.db = db;
                R.print = true;
                q_missing(&R, tbl_name);
            }
            return ok;
        }
    }

    // 9. update / change / in <table_name> update/change/set
    if (str_starts_with_ci(stmt, "update ") || str_starts_with_ci(stmt, "change ") || str_starts_with_ci(stmt, "modify ") || str_starts_with_ci(stmt, "in ")) {
        return q_update(db, stmt, print_output);
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
            fprintf(stderr, "  * Supported commands: use database, show tables, create table, create view, create index, insert into, find, with, update, change, delete, truncate, alter table, drop table, describe, begin, commit, rollback, explain\n");
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
        int depth = 0;      /* a statement can go on past the end of a line inside brackets */

        while (*ptr) {
            if (!in_q && (*ptr == '"' || *ptr == '\'')) {
                in_q = true;
                q_char = *ptr;
            } else if (in_q && *ptr == q_char) {
                in_q = false;
            } else if (!in_q && (*ptr == '#' || (ptr[0] == '-' && ptr[1] == '-')) && (ptr == stmt_start || isspace((unsigned char)ptr[-1]))) {
                /* a comment at the end of a statement line: blanked out, the statement is what came before it */
                while (*ptr && *ptr != '\n' && *ptr != '\r') *ptr++ = ' ';
                continue;
            } else if (!in_q && *ptr == '(') {
                depth++;
            } else if (!in_q && *ptr == ')' && depth > 0) {
                depth--;
            } else if (!in_q && *ptr == ';') {
                break;
            } else if (!in_q && (*ptr == '\n' || *ptr == '\r')) {
                /* inside brackets the statement goes on when the line ends with '(' or ',' or the next one starts with ')' */
                bool goes_on = false;
                if (depth > 0) {
                    const char* back = ptr;
                    while (back > stmt_start && isspace((unsigned char)back[-1])) back--;
                    const char* ahead = ptr;
                    while (*ahead && isspace((unsigned char)*ahead)) ahead++;
                    goes_on = (back > stmt_start && (back[-1] == '(' || back[-1] == ',')) || *ahead == ')';
                }
                if (!goes_on) break;
            }
            ptr++;
        }

        if (*ptr) {
            *ptr = '\0';
            ptr++;
        }

        // a newline inside brackets was kept: make it a space
        for (char* c = stmt_start; *c; c++) if (*c == '\n' || *c == '\r') *c = ' ';

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
    dbname[sizeof(dbname) - 1] = '\0';
    char* dot = strrchr(dbname, '.');
    if (dot) *dot = '\0';

    EnlngDatabase* db = enlngdb_create(dbname);

    // Try auto-loading existing binary database file
    char edb_file[256];
    snprintf(edb_file, sizeof(edb_file), "%s.edb", dbname);
    enlngdb_load(db, edb_file);

    int count = enlngdb_execute_script(db, buffer, true);

    if (db->txn) {
        printf("NOTE: a transaction was still open at the end, so it was rolled back (add 'commit' to keep it).\n\n");
        enlngdb_rollback(db);
    }

    // Auto-save to the database that is active at the end: 'use database X' moves the
    // target from the script-named file to X's file (current folder first, then tests/,
    // the same order 'use database' loads it in). Nothing is saved if any statement failed.
    if (db->dropped) {
        /* 'drop database' on the open database: its file is gone and stays gone */
    } else if (count >= 0) {
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
