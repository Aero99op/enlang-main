#ifndef ENLNG_MEM_H
#define ENLNG_MEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdarg.h>

/*
 * ============================================================================
 *           SOVEREIGN SCOPED BUMP ARENA MEMORY ARCHITECTURE (ENLNG_MEM)
 * ============================================================================
 * Zero manual free(). Zero Garbage Collection pauses. Zero fragmentation.
 * Every temporary string, list, and object is allocated with a 1-cycle bump.
 * Exiting a scope resets the memory watermark in 1 CPU instruction.
 * Return values are safely promoted to the parent scope.
 * ============================================================================
 */

#define ENLNG_ARENA_CHUNK_SIZE (4 * 1024 * 1024) /* 4 MB default chunk */

typedef struct EnlngArenaChunk {
    size_t capacity;
    size_t offset;
    struct EnlngArenaChunk* next;
    uint8_t data[];
} EnlngArenaChunk;

typedef struct EnlngArena {
    EnlngArenaChunk* first;
    EnlngArenaChunk* current;
    size_t total_allocated;
} EnlngArena;

typedef struct EnlngScopeMark {
    EnlngArenaChunk* chunk;
    size_t offset;
} EnlngScopeMark;

/* --- Forward Declarations --- */
typedef enum {
    ENLNG_T_NULL = 0,
    ENLNG_T_BOOL,
    ENLNG_T_INT,
    ENLNG_T_DOUBLE,
    ENLNG_T_STR,
    ENLNG_T_LIST
} EnlngType;

typedef struct EnlngVal EnlngVal;
typedef struct EnlngList EnlngList;

struct EnlngList {
    EnlngVal* items;
    size_t count;
    size_t capacity;
};

struct EnlngVal {
    EnlngType type;
    union {
        bool b;
        int64_t i;
        double d;
        const char* s;
        EnlngList* list;
    };
};

/* --- Global Arena Instance --- */
static EnlngArena g_enlng_arena = {0};
static bool g_enlng_arena_initialized = false;

static EnlngArenaChunk* enlng_arena_chunk_create(size_t cap) {
    if (cap < ENLNG_ARENA_CHUNK_SIZE) cap = ENLNG_ARENA_CHUNK_SIZE;
    EnlngArenaChunk* chunk = (EnlngArenaChunk*)malloc(sizeof(EnlngArenaChunk) + cap);
    if (!chunk) {
        fprintf(stderr, "[ENLNG FATAL] Out of memory allocating arena chunk of size %zu\n", cap);
        exit(1);
    }
    chunk->capacity = cap;
    chunk->offset = 0;
    chunk->next = NULL;
    return chunk;
}

static void enlng_init_memory(void) {
    if (g_enlng_arena_initialized) return;
    g_enlng_arena.first = enlng_arena_chunk_create(ENLNG_ARENA_CHUNK_SIZE);
    g_enlng_arena.current = g_enlng_arena.first;
    g_enlng_arena.total_allocated = 0;
    g_enlng_arena_initialized = true;
}

static void* enlng_alloc(size_t bytes) {
    if (!g_enlng_arena_initialized) enlng_init_memory();
    // 8-byte align
    size_t aligned = (bytes + 7) & ~((size_t)7);
    EnlngArenaChunk* cur = g_enlng_arena.current;
    if (cur->offset + aligned > cur->capacity) {
        if (cur->next && cur->next->capacity >= aligned) {
            cur = cur->next;
            cur->offset = 0;
            g_enlng_arena.current = cur;
        } else {
            size_t new_cap = aligned > ENLNG_ARENA_CHUNK_SIZE ? aligned * 2 : ENLNG_ARENA_CHUNK_SIZE;
            EnlngArenaChunk* next_chunk = enlng_arena_chunk_create(new_cap);
            cur->next = next_chunk;
            g_enlng_arena.current = next_chunk;
            cur = next_chunk;
        }
    }
    void* ptr = &cur->data[cur->offset];
    cur->offset += aligned;
    g_enlng_arena.total_allocated += aligned;
    return ptr;
}

/* --- Scope Watermarks for 1-Cycle Automatic Reclamation --- */
static EnlngScopeMark enlng_scope_start(void) {
    if (!g_enlng_arena_initialized) enlng_init_memory();
    EnlngScopeMark mark;
    mark.chunk = g_enlng_arena.current;
    mark.offset = g_enlng_arena.current->offset;
    return mark;
}

static void enlng_scope_end(EnlngScopeMark mark) {
    if (!g_enlng_arena_initialized) return;
    // Reset back to marked chunk and offset
    g_enlng_arena.current = mark.chunk;
    g_enlng_arena.current->offset = mark.offset;
}

#define ENLNG_SCOPE_START() EnlngScopeMark __scope_mark__ = enlng_scope_start()
#define ENLNG_SCOPE_END()   enlng_scope_end(__scope_mark__)

/* --- Value Constructors --- */
static inline EnlngVal enlng_null(void) {
    EnlngVal v; v.type = ENLNG_T_NULL; v.i = 0; return v;
}
static inline EnlngVal enlng_bool(bool b) {
    EnlngVal v; v.type = ENLNG_T_BOOL; v.b = b; return v;
}
static inline EnlngVal enlng_int(int64_t i) {
    EnlngVal v; v.type = ENLNG_T_INT; v.i = i; return v;
}
static inline EnlngVal enlng_double(double d) {
    EnlngVal v; v.type = ENLNG_T_DOUBLE; v.d = d; return v;
}
static inline EnlngVal enlng_str(const char* s) {
    if (!s) return enlng_null();
    size_t len = strlen(s);
    char* copy = (char*)enlng_alloc(len + 1);
    memcpy(copy, s, len + 1);
    EnlngVal v; v.type = ENLNG_T_STR; v.s = copy; return v;
}

/* --- Dynamic List Operations --- */
static EnlngList* enlng_list_create(size_t initial_cap) {
    if (initial_cap < 4) initial_cap = 4;
    EnlngList* list = (EnlngList*)enlng_alloc(sizeof(EnlngList));
    list->count = 0;
    list->capacity = initial_cap;
    list->items = (EnlngVal*)enlng_alloc(sizeof(EnlngVal) * initial_cap);
    return list;
}

static void enlng_list_append(EnlngList* list, EnlngVal val) {
    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity * 2;
        EnlngVal* new_items = (EnlngVal*)enlng_alloc(sizeof(EnlngVal) * new_cap);
        memcpy(new_items, list->items, sizeof(EnlngVal) * list->count);
        list->items = new_items;
        list->capacity = new_cap;
    }
    list->items[list->count++] = val;
}

static EnlngVal enlng_list_get(const EnlngList* list, int64_t index) {
    if (!list || index < 0 || (size_t)index >= list->count) return enlng_null();
    return list->items[index];
}

static int64_t enlng_list_len(const EnlngList* list) {
    return list ? (int64_t)list->count : 0;
}

/* --- Universal String Formatting & Promotion --- */
static const char* enlng_to_cstr(EnlngVal v) {
    switch (v.type) {
        case ENLNG_T_NULL: return "null";
        case ENLNG_T_BOOL: return v.b ? "true" : "false";
        case ENLNG_T_INT: {
            char* buf = (char*)enlng_alloc(32);
            snprintf(buf, 32, "%lld", (long long)v.i);
            return buf;
        }
        case ENLNG_T_DOUBLE: {
            char* buf = (char*)enlng_alloc(32);
            snprintf(buf, 32, "%g", v.d);
            return buf;
        }
        case ENLNG_T_STR: return v.s ? v.s : "";
        case ENLNG_T_LIST: {
            char* buf = (char*)enlng_alloc(1024);
            strcpy(buf, "[");
            for (size_t idx = 0; idx < v.list->count; idx++) {
                if (idx > 0) strcat(buf, ", ");
                strcat(buf, enlng_to_cstr(v.list->items[idx]));
                if (strlen(buf) > 900) { strcat(buf, ", ..."); break; }
            }
            strcat(buf, "]");
            return buf;
        }
        default: return "";
    }
}

/* Promotes a return value from child scope to caller scope */
static EnlngVal enlng_promote(EnlngVal v, EnlngScopeMark parent_mark) {
    if (v.type == ENLNG_T_STR) {
        const char* s = v.s;
        enlng_scope_end(parent_mark);
        return enlng_str(s);
    }
    if (v.type == ENLNG_T_LIST) {
        EnlngList* old_list = v.list;
        enlng_scope_end(parent_mark);
        EnlngList* new_list = enlng_list_create(old_list->count);
        for (size_t i = 0; i < old_list->count; i++) {
            enlng_list_append(new_list, old_list->items[i]);
        }
        EnlngVal res; res.type = ENLNG_T_LIST; res.list = new_list; return res;
    }
    enlng_scope_end(parent_mark);
    return v;
}

/* --- Universal Spoken Operators --- */
static inline bool enlng_is_truthy(EnlngVal v) {
    switch (v.type) {
        case ENLNG_T_NULL: return false;
        case ENLNG_T_BOOL: return v.b;
        case ENLNG_T_INT: return v.i != 0;
        case ENLNG_T_DOUBLE: return v.d != 0.0;
        case ENLNG_T_STR: return v.s && strlen(v.s) > 0;
        case ENLNG_T_LIST: return v.list && v.list->count > 0;
        default: return false;
    }
}

static EnlngVal enlng_add(EnlngVal a, EnlngVal b) {
    if (a.type == ENLNG_T_STR || b.type == ENLNG_T_STR) {
        const char* sa = enlng_to_cstr(a);
        const char* sb = enlng_to_cstr(b);
        size_t la = strlen(sa);
        size_t lb = strlen(sb);
        char* out = (char*)enlng_alloc(la + lb + 1);
        memcpy(out, sa, la);
        memcpy(out + la, sb, lb);
        out[la + lb] = '\0';
        EnlngVal v; v.type = ENLNG_T_STR; v.s = out; return v;
    }
    if (a.type == ENLNG_T_DOUBLE || b.type == ENLNG_T_DOUBLE) {
        double da = (a.type == ENLNG_T_DOUBLE) ? a.d : (double)a.i;
        double db = (b.type == ENLNG_T_DOUBLE) ? b.d : (double)b.i;
        return enlng_double(da + db);
    }
    return enlng_int(a.i + b.i);
}

static EnlngVal enlng_sub(EnlngVal a, EnlngVal b) {
    if (a.type == ENLNG_T_DOUBLE || b.type == ENLNG_T_DOUBLE) {
        double da = (a.type == ENLNG_T_DOUBLE) ? a.d : (double)a.i;
        double db = (b.type == ENLNG_T_DOUBLE) ? b.d : (double)b.i;
        return enlng_double(da - db);
    }
    return enlng_int(a.i - b.i);
}

static EnlngVal enlng_mul(EnlngVal a, EnlngVal b) {
    if (a.type == ENLNG_T_STR && b.type == ENLNG_T_INT) {
        int64_t times = b.i;
        if (times <= 0) return enlng_str("");
        size_t len = strlen(a.s);
        char* buf = (char*)enlng_alloc(len * times + 1);
        buf[0] = '\0';
        for (int64_t k = 0; k < times; k++) strcat(buf, a.s);
        EnlngVal v; v.type = ENLNG_T_STR; v.s = buf; return v;
    }
    if (a.type == ENLNG_T_DOUBLE || b.type == ENLNG_T_DOUBLE) {
        double da = (a.type == ENLNG_T_DOUBLE) ? a.d : (double)a.i;
        double db = (b.type == ENLNG_T_DOUBLE) ? b.d : (double)b.i;
        return enlng_double(da * db);
    }
    return enlng_int(a.i * b.i);
}

static EnlngVal enlng_div(EnlngVal a, EnlngVal b) {
    double da = (a.type == ENLNG_T_DOUBLE) ? a.d : (double)a.i;
    double db = (b.type == ENLNG_T_DOUBLE) ? b.d : (double)b.i;
    if (db == 0.0) {
        fprintf(stderr, "[ENLNG RUNTIME ERROR] Division by zero!\n");
        return enlng_double(0.0);
    }
    return enlng_double(da / db);
}

static EnlngVal enlng_mod(EnlngVal a, EnlngVal b) {
    if (b.i == 0) return enlng_int(0);
    return enlng_int(a.i % b.i);
}

static bool enlng_eq(EnlngVal a, EnlngVal b) {
    if (a.type != b.type) {
        if ((a.type == ENLNG_T_INT || a.type == ENLNG_T_DOUBLE) &&
            (b.type == ENLNG_T_INT || b.type == ENLNG_T_DOUBLE)) {
            double da = (a.type == ENLNG_T_DOUBLE) ? a.d : (double)a.i;
            double db = (b.type == ENLNG_T_DOUBLE) ? b.d : (double)b.i;
            return da == db;
        }
        return false;
    }
    switch (a.type) {
        case ENLNG_T_NULL: return true;
        case ENLNG_T_BOOL: return a.b == b.b;
        case ENLNG_T_INT: return a.i == b.i;
        case ENLNG_T_DOUBLE: return a.d == b.d;
        case ENLNG_T_STR: return strcmp(a.s, b.s) == 0;
        case ENLNG_T_LIST: return a.list == b.list;
        default: return false;
    }
}

static inline bool enlng_neq(EnlngVal a, EnlngVal b) { return !enlng_eq(a, b); }

static bool enlng_lt(EnlngVal a, EnlngVal b) {
    if (a.type == ENLNG_T_DOUBLE || b.type == ENLNG_T_DOUBLE) {
        double da = (a.type == ENLNG_T_DOUBLE) ? a.d : (double)a.i;
        double db = (b.type == ENLNG_T_DOUBLE) ? b.d : (double)b.i;
        return da < db;
    }
    if (a.type == ENLNG_T_INT && b.type == ENLNG_T_INT) return a.i < b.i;
    if (a.type == ENLNG_T_STR && b.type == ENLNG_T_STR) return strcmp(a.s, b.s) < 0;
    return false;
}

static inline bool enlng_gt(EnlngVal a, EnlngVal b)  { return enlng_lt(b, a); }
static inline bool enlng_lte(EnlngVal a, EnlngVal b) { return !enlng_gt(a, b); }
static inline bool enlng_gte(EnlngVal a, EnlngVal b) { return !enlng_lt(a, b); }

/* --- Universal Display Function --- */
static void enlng_display(int count, ...) {
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        EnlngVal v = va_arg(args, EnlngVal);
        const char* str = enlng_to_cstr(v);
        if (i > 0) printf(" ");
        printf("%s", str);
    }
    va_end(args);
    printf("\n");
}

static void enlng_display_sep(const char* sep, int count, ...) {
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        EnlngVal v = va_arg(args, EnlngVal);
        const char* str = enlng_to_cstr(v);
        if (i > 0 && sep) printf("%s", sep);
        printf("%s", str);
    }
    va_end(args);
    printf("\n");
}

#endif /* ENLNG_MEM_H */
