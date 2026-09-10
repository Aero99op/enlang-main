/*
 * =====================================================================
 *   ENLANG SOVEREIGN PROGRAMMING LANGUAGE COMPILER (Pure C99 Native)
 *   src/enlng_runtime.h - Universal Embedded Zero-Dependency Native C99 Runtime
 *   Turing-Complete Foundation: Polymorphic Types, HashMaps, Lists,
 *   File I/O, OS/System, Math & Standard Library Built-ins.
 * =====================================================================
 */

#ifndef ENLNG_RUNTIME_H
#define ENLNG_RUNTIME_H

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/* Forward declarations */
typedef struct EnlngVal EnlngVal;
typedef struct EnlngList EnlngList;
typedef struct EnlngMap EnlngMap;
typedef struct EnlngMapEntry EnlngMapEntry;

/* --- Value Types --- */
typedef enum {
  ENLNG_VAL_INT,
  ENLNG_VAL_FLOAT,
  ENLNG_VAL_STRING,
  ENLNG_VAL_BOOL,
  ENLNG_VAL_LIST,
  ENLNG_VAL_MAP,
  ENLNG_VAL_NULL
} EnlngValType;

struct EnlngVal {
  EnlngValType type;
  union {
    int64_t i;
    double f;
    char *s;
    bool b;
    EnlngList *l;
    EnlngMap *m;
  } as;
};

struct EnlngList {
  EnlngVal *items;
  int count;
  int capacity;
};

struct EnlngMapEntry {
  char *key;
  EnlngVal val;
  EnlngMapEntry *next;
};

struct EnlngMap {
  EnlngMapEntry **buckets;
  int capacity;
  int count;
};

/* --- Constructors --- */
static inline EnlngVal enlng_make_int(int64_t v) {
  EnlngVal val;
  val.type = ENLNG_VAL_INT;
  val.as.i = v;
  return val;
}

static inline EnlngVal enlng_make_float(double v) {
  EnlngVal val;
  val.type = ENLNG_VAL_FLOAT;
  val.as.f = v;
  return val;
}

static inline EnlngVal enlng_make_bool(bool v) {
  EnlngVal val;
  val.type = ENLNG_VAL_BOOL;
  val.as.b = v;
  return val;
}

static inline EnlngVal enlng_make_null(void) {
  EnlngVal val;
  val.type = ENLNG_VAL_NULL;
  val.as.i = 0;
  return val;
}

static inline EnlngVal enlng_make_string(const char *s) {
  EnlngVal val;
  val.type = ENLNG_VAL_STRING;
  if (!s) {
    val.as.s = (char *)malloc(1);
    val.as.s[0] = '\0';
    return val;
  }
  size_t len = strlen(s);
  val.as.s = (char *)malloc(len + 1);
  memcpy(val.as.s, s, len + 1);
  return val;
}

static inline int64_t enlng_val_to_int(EnlngVal v) {
  if (v.type == ENLNG_VAL_INT)
    return v.as.i;
  if (v.type == ENLNG_VAL_FLOAT)
    return (int64_t)v.as.f;
  return 0;
}

/* --- Dynamic List Operations --- */
static inline EnlngList *enlng_list_create(void) {
  EnlngList *list = (EnlngList *)malloc(sizeof(EnlngList));
  list->count = 0;
  list->capacity = 8;
  list->items = (EnlngVal *)malloc(sizeof(EnlngVal) * list->capacity);
  return list;
}

static inline void enlng_list_push(EnlngList *list, EnlngVal v) {
  if (list->count >= list->capacity) {
    list->capacity *= 2;
    list->items =
        (EnlngVal *)realloc(list->items, sizeof(EnlngVal) * list->capacity);
  }
  list->items[list->count++] = v;
}

static inline EnlngVal enlng_make_list(EnlngList *list) {
  EnlngVal val;
  val.type = ENLNG_VAL_LIST;
  val.as.l = list;
  return val;
}

/* --- Hash Map Operations (DJB2 Hash Table) --- */
static inline unsigned long enlng_hash_str(const char *str) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}

static inline EnlngMap *enlng_map_create(void) {
  EnlngMap *m = (EnlngMap *)malloc(sizeof(EnlngMap));
  m->capacity = 16;
  m->count = 0;
  m->buckets = (EnlngMapEntry **)calloc(m->capacity, sizeof(EnlngMapEntry *));
  return m;
}

static inline EnlngVal enlng_make_map(EnlngMap *m) {
  EnlngVal val;
  val.type = ENLNG_VAL_MAP;
  val.as.m = m;
  return val;
}

static inline EnlngVal enlng_map_get(EnlngVal v, const char *key) {
  if (v.type != ENLNG_VAL_MAP || !v.as.m || !key)
    return enlng_make_null();
  EnlngMap *m = v.as.m;
  unsigned long h = enlng_hash_str(key) % m->capacity;
  EnlngMapEntry *e = m->buckets[h];
  while (e) {
    if (strcmp(e->key, key) == 0)
      return e->val;
    e = e->next;
  }
  return enlng_make_null();
}

static inline void enlng_map_set(EnlngVal v, const char *key, EnlngVal item) {
  if (v.type != ENLNG_VAL_MAP || !v.as.m || !key)
    return;
  EnlngMap *m = v.as.m;
  unsigned long h = enlng_hash_str(key) % m->capacity;
  EnlngMapEntry *e = m->buckets[h];
  while (e) {
    if (strcmp(e->key, key) == 0) {
      e->val = item;
      return;
    }
    e = e->next;
  }
  /* Add new key-value pair */
  EnlngMapEntry *ne = (EnlngMapEntry *)malloc(sizeof(EnlngMapEntry));
  size_t klen = strlen(key);
  ne->key = (char *)malloc(klen + 1);
  memcpy(ne->key, key, klen + 1);
  ne->val = item;
  ne->next = m->buckets[h];
  m->buckets[h] = ne;
  m->count++;
}

static inline bool enlng_map_has(EnlngVal v, const char *key) {
  if (v.type != ENLNG_VAL_MAP || !v.as.m || !key)
    return false;
  EnlngMap *m = v.as.m;
  unsigned long h = enlng_hash_str(key) % m->capacity;
  EnlngMapEntry *e = m->buckets[h];
  while (e) {
    if (strcmp(e->key, key) == 0)
      return true;
    e = e->next;
  }
  return false;
}

static inline EnlngVal enlng_map_keys(EnlngVal v) {
  EnlngList *l = enlng_list_create();
  if (v.type == ENLNG_VAL_MAP && v.as.m) {
    EnlngMap *m = v.as.m;
    for (int i = 0; i < m->capacity; i++) {
      EnlngMapEntry *e = m->buckets[i];
      while (e) {
        enlng_list_push(l, enlng_make_string(e->key));
        e = e->next;
      }
    }
  }
  return enlng_make_list(l);
}

static inline EnlngVal enlng_map_values(EnlngVal v) {
  EnlngList *l = enlng_list_create();
  if (v.type == ENLNG_VAL_MAP && v.as.m) {
    EnlngMap *m = v.as.m;
    for (int i = 0; i < m->capacity; i++) {
      EnlngMapEntry *e = m->buckets[i];
      while (e) {
        enlng_list_push(l, e->val);
        e = e->next;
      }
    }
  }
  return enlng_make_list(l);
}

/* --- Container Length --- */
static inline int64_t enlng_count_of(EnlngVal v) {
  if (v.type == ENLNG_VAL_LIST && v.as.l)
    return v.as.l->count;
  if (v.type == ENLNG_VAL_STRING && v.as.s)
    return (int64_t)strlen(v.as.s);
  if (v.type == ENLNG_VAL_MAP && v.as.m)
    return (int64_t)v.as.m->count;
  return 0;
}

/* --- List Get & Set --- */
static inline EnlngVal enlng_list_get(EnlngVal v, int64_t idx) {
  if (v.type == ENLNG_VAL_LIST && v.as.l) {
    if (idx < 0 || idx >= v.as.l->count)
      return enlng_make_null();
    return v.as.l->items[idx];
  }
  if (v.type == ENLNG_VAL_STRING && v.as.s) {
    int64_t len = (int64_t)strlen(v.as.s);
    if (idx < 0 || idx >= len)
      return enlng_make_null();
    char buf[2] = {v.as.s[idx], '\0'};
    return enlng_make_string(buf);
  }
  return enlng_make_null();
}

static inline void enlng_list_set(EnlngVal v, int64_t idx, EnlngVal item) {
  if (v.type == ENLNG_VAL_LIST && v.as.l && idx >= 0) {
    while (v.as.l->count <= idx) {
      enlng_list_push(v.as.l, enlng_make_null());
    }
    v.as.l->items[idx] = item;
  }
}

/* --- Universal Container Get & Set (Handles Lists, Strings, and Maps) --- */
static inline EnlngVal enlng_container_get(EnlngVal c, EnlngVal key) {
  if (c.type == ENLNG_VAL_MAP) {
    if (key.type == ENLNG_VAL_STRING)
      return enlng_map_get(c, key.as.s);
    char buf[64];
    snprintf(buf, sizeof(buf), "%lld", (long long)enlng_val_to_int(key));
    return enlng_map_get(c, buf);
  }
  if (c.type == ENLNG_VAL_LIST || c.type == ENLNG_VAL_STRING) {
    int64_t idx = enlng_val_to_int(key);
    int64_t cnt = enlng_count_of(c);
    if (idx < 0)
      idx += cnt;
    return enlng_list_get(c, idx);
  }
  return enlng_make_null();
}

static inline void enlng_container_set(EnlngVal c, EnlngVal key, EnlngVal val) {
  if (c.type == ENLNG_VAL_MAP) {
    if (key.type == ENLNG_VAL_STRING) {
      enlng_map_set(c, key.as.s, val);
    } else {
      char buf[64];
      snprintf(buf, sizeof(buf), "%lld", (long long)enlng_val_to_int(key));
      enlng_map_set(c, buf, val);
    }
    return;
  }
  if (c.type == ENLNG_VAL_LIST) {
    int64_t idx = enlng_val_to_int(key);
    int64_t cnt = enlng_count_of(c);
    if (idx < 0)
      idx += cnt;
    enlng_list_set(c, idx, val);
    return;
  }
}

/* In-place list swap */
static inline void enlng_list_swap(EnlngVal v, int64_t i, int64_t j) {
  if (v.type == ENLNG_VAL_LIST && v.as.l) {
    if (i >= 0 && i < v.as.l->count && j >= 0 && j < v.as.l->count) {
      EnlngVal temp = v.as.l->items[i];
      v.as.l->items[i] = v.as.l->items[j];
      v.as.l->items[j] = temp;
    }
  }
}

/* In-place list reverse */
static inline void enlng_list_reverse(EnlngVal v) {
  if (v.type == ENLNG_VAL_LIST && v.as.l) {
    int left = 0;
    int right = v.as.l->count - 1;
    while (left < right) {
      EnlngVal temp = v.as.l->items[left];
      v.as.l->items[left] = v.as.l->items[right];
      v.as.l->items[right] = temp;
      left++;
      right--;
    }
  }
}

/* Reversal expression */
static inline EnlngVal enlng_val_reverse(EnlngVal v) {
  if (v.type == ENLNG_VAL_STRING && v.as.s) {
    size_t len = strlen(v.as.s);
    char *rev = (char *)malloc(len + 1);
    for (size_t i = 0; i < len; i++) {
      rev[i] = v.as.s[len - 1 - i];
    }
    rev[len] = '\0';
    EnlngVal res;
    res.type = ENLNG_VAL_STRING;
    res.as.s = rev;
    return res;
  }
  if (v.type == ENLNG_VAL_LIST && v.as.l) {
    EnlngList *copy = enlng_list_create();
    for (int i = v.as.l->count - 1; i >= 0; i--) {
      enlng_list_push(copy, v.as.l->items[i]);
    }
    return enlng_make_list(copy);
  }
  return v;
}

/* --- Truthiness & Comparison --- */
static inline bool enlng_is_truthy(EnlngVal v) {
  if (v.type == ENLNG_VAL_BOOL)
    return v.as.b;
  if (v.type == ENLNG_VAL_INT)
    return v.as.i != 0;
  if (v.type == ENLNG_VAL_FLOAT)
    return v.as.f != 0.0;
  if (v.type == ENLNG_VAL_STRING)
    return v.as.s && v.as.s[0] != '\0';
  if (v.type == ENLNG_VAL_LIST)
    return v.as.l && v.as.l->count > 0;
  if (v.type == ENLNG_VAL_MAP)
    return v.as.m && v.as.m->count > 0;
  return false;
}

static inline bool enlng_vals_equal(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT)
    return a.as.i == b.as.i;
  if (a.type == ENLNG_VAL_FLOAT && b.type == ENLNG_VAL_FLOAT)
    return a.as.f == b.as.f;
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_FLOAT)
    return (double)a.as.i == b.as.f;
  if (a.type == ENLNG_VAL_FLOAT && b.type == ENLNG_VAL_INT)
    return a.as.f == (double)b.as.i;
  if (a.type == ENLNG_VAL_BOOL && b.type == ENLNG_VAL_BOOL)
    return a.as.b == b.as.b;
  if (a.type == ENLNG_VAL_STRING && b.type == ENLNG_VAL_STRING) {
    if (!a.as.s && !b.as.s)
      return true;
    if (!a.as.s || !b.as.s)
      return false;
    return strcmp(a.as.s, b.as.s) == 0;
  }
  if (a.type == ENLNG_VAL_NULL && b.type == ENLNG_VAL_NULL)
    return true;
  return false;
}

static inline bool enlng_vals_gt(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT)
    return a.as.i > b.as.i;
  if (a.type == ENLNG_VAL_FLOAT && b.type == ENLNG_VAL_FLOAT)
    return a.as.f > b.as.f;
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_FLOAT)
    return (double)a.as.i > b.as.f;
  if (a.type == ENLNG_VAL_FLOAT && b.type == ENLNG_VAL_INT)
    return a.as.f > (double)b.as.i;
  if (a.type == ENLNG_VAL_STRING && b.type == ENLNG_VAL_STRING)
    return strcmp(a.as.s, b.as.s) > 0;
  return false;
}

static inline bool enlng_vals_lt(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT)
    return a.as.i < b.as.i;
  if (a.type == ENLNG_VAL_FLOAT && b.type == ENLNG_VAL_FLOAT)
    return a.as.f < b.as.f;
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_FLOAT)
    return (double)a.as.i < b.as.f;
  if (a.type == ENLNG_VAL_FLOAT && b.type == ENLNG_VAL_INT)
    return a.as.f < (double)b.as.i;
  if (a.type == ENLNG_VAL_STRING && b.type == ENLNG_VAL_STRING)
    return strcmp(a.as.s, b.as.s) < 0;
  return false;
}

static inline bool enlng_vals_gte(EnlngVal a, EnlngVal b) {
  return enlng_vals_gt(a, b) || enlng_vals_equal(a, b);
}

static inline bool enlng_vals_lte(EnlngVal a, EnlngVal b) {
  return enlng_vals_lt(a, b) || enlng_vals_equal(a, b);
}

/* In-place list sort */
static inline void enlng_list_sort(EnlngVal v, bool descending) {
  if (v.type == ENLNG_VAL_LIST && v.as.l) {
    int n = v.as.l->count;
    for (int i = 0; i < n - 1; i++) {
      for (int j = 0; j < n - i - 1; j++) {
        bool should_swap =
            descending ? enlng_vals_lt(v.as.l->items[j], v.as.l->items[j + 1])
                       : enlng_vals_gt(v.as.l->items[j], v.as.l->items[j + 1]);
        if (should_swap) {
          EnlngVal tmp = v.as.l->items[j];
          v.as.l->items[j] = v.as.l->items[j + 1];
          v.as.l->items[j + 1] = tmp;
        }
      }
    }
  }
}

static inline const char *enlng_val_to_str_buf(EnlngVal v, char *buf,
                                               size_t sz) {
  if (v.type == ENLNG_VAL_STRING && v.as.s)
    return v.as.s;
  if (v.type == ENLNG_VAL_INT) {
    snprintf(buf, sz, "%lld", (long long)v.as.i);
    return buf;
  }
  if (v.type == ENLNG_VAL_FLOAT) {
    snprintf(buf, sz, "%g", v.as.f);
    return buf;
  }
  if (v.type == ENLNG_VAL_BOOL) {
    return v.as.b ? "true" : "false";
  }
  return "";
}

/* --- Arithmetic --- */
static inline EnlngVal enlng_val_add(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT)
    return enlng_make_int(a.as.i + b.as.i);
  if (a.type == ENLNG_VAL_FLOAT || b.type == ENLNG_VAL_FLOAT) {
    if (a.type != ENLNG_VAL_STRING && b.type != ENLNG_VAL_STRING) {
      double da = (a.type == ENLNG_VAL_FLOAT) ? a.as.f : (double)a.as.i;
      double db = (b.type == ENLNG_VAL_FLOAT) ? b.as.f : (double)b.as.i;
      return enlng_make_float(da + db);
    }
  }
  if (a.type == ENLNG_VAL_STRING || b.type == ENLNG_VAL_STRING) {
    char buf_a[128];
    char buf_b[128];
    const char *sa = enlng_val_to_str_buf(a, buf_a, sizeof(buf_a));
    const char *sb = enlng_val_to_str_buf(b, buf_b, sizeof(buf_b));
    size_t la = strlen(sa);
    size_t lb = strlen(sb);
    char *comb = (char *)malloc(la + lb + 1);
    memcpy(comb, sa, la);
    memcpy(comb + la, sb, lb);
    comb[la + lb] = '\0';
    EnlngVal res;
    res.type = ENLNG_VAL_STRING;
    res.as.s = comb;
    return res;
  }
  return enlng_make_int(0);
}

static inline EnlngVal enlng_val_sub(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT)
    return enlng_make_int(a.as.i - b.as.i);
  double da = (a.type == ENLNG_VAL_FLOAT) ? a.as.f : (double)a.as.i;
  double db = (b.type == ENLNG_VAL_FLOAT) ? b.as.f : (double)b.as.i;
  return enlng_make_float(da - db);
}

static inline EnlngVal enlng_val_mul(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT)
    return enlng_make_int(a.as.i * b.as.i);
  double da = (a.type == ENLNG_VAL_FLOAT) ? a.as.f : (double)a.as.i;
  double db = (b.type == ENLNG_VAL_FLOAT) ? b.as.f : (double)b.as.i;
  return enlng_make_float(da * db);
}

static inline EnlngVal enlng_val_div(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT) {
    if (b.as.i == 0)
      return enlng_make_int(0);
    return enlng_make_int(a.as.i / b.as.i);
  }
  double da = (a.type == ENLNG_VAL_FLOAT) ? a.as.f : (double)a.as.i;
  double db = (b.type == ENLNG_VAL_FLOAT) ? b.as.f : (double)b.as.i;
  if (db == 0.0)
    return enlng_make_float(0.0);
  return enlng_make_float(da / db);
}

static inline EnlngVal enlng_val_mod(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT && b.as.i != 0) {
    return enlng_make_int(a.as.i % b.as.i);
  }
  return enlng_make_int(0);
}

/* --- Stream Printing --- */
static inline void enlng_print_val(EnlngVal v) {
  switch (v.type) {
  case ENLNG_VAL_INT:
    printf("%lld", (long long)v.as.i);
    break;
  case ENLNG_VAL_FLOAT:
    printf("%.6g", v.as.f);
    break;
  case ENLNG_VAL_STRING:
    printf("%s", v.as.s ? v.as.s : "");
    break;
  case ENLNG_VAL_BOOL:
    printf("%s", v.as.b ? "true" : "false");
    break;
  case ENLNG_VAL_LIST:
    printf("[");
    if (v.as.l) {
      for (int i = 0; i < v.as.l->count; i++) {
        enlng_print_val(v.as.l->items[i]);
        if (i < v.as.l->count - 1)
          printf(", ");
      }
    }
    printf("]");
    break;
  case ENLNG_VAL_MAP:
    printf("{");
    if (v.as.m) {
      int shown = 0;
      for (int i = 0; i < v.as.m->capacity; i++) {
        EnlngMapEntry *e = v.as.m->buckets[i];
        while (e) {
          if (shown > 0)
            printf(", ");
          printf("\"%s\": ", e->key);
          enlng_print_val(e->val);
          shown++;
          e = e->next;
        }
      }
    }
    printf("}");
    break;
  case ENLNG_VAL_NULL:
    printf("null");
    break;
  }
}

/* --- Input --- */
static inline EnlngVal enlng_ask(const char *prompt) {
  if (prompt && prompt[0] != '\0') {
    printf("%s", prompt);
    fflush(stdout);
  }
  char buffer[1024];
  if (!fgets(buffer, sizeof(buffer), stdin)) {
    return enlng_make_string("");
  }
  size_t len = strlen(buffer);
  if (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
    buffer[len - 1] = '\0';
    if (len > 1 && buffer[len - 2] == '\r')
      buffer[len - 2] = '\0';
  }
  return enlng_make_string(buffer);
}

/* ===================================================================== */
/*   NATIVE BUILT-IN STANDARD LIBRARY FUNCTIONS                          */
/* ===================================================================== */

/* --- Math Built-ins --- */
static inline EnlngVal enlng_builtin_sqrt(EnlngVal v) {
  double d = (v.type == ENLNG_VAL_INT) ? (double)v.as.i : v.as.f;
  return enlng_make_float(sqrt(d));
}

static inline EnlngVal enlng_builtin_pow(EnlngVal base, EnlngVal exp) {
  double b = (base.type == ENLNG_VAL_INT) ? (double)base.as.i : base.as.f;
  double e = (exp.type == ENLNG_VAL_INT) ? (double)exp.as.i : exp.as.f;
  return enlng_make_float(pow(b, e));
}

static inline EnlngVal enlng_builtin_abs(EnlngVal v) {
  if (v.type == ENLNG_VAL_INT) {
    return enlng_make_int(v.as.i < 0 ? -v.as.i : v.as.i);
  }
  return enlng_make_float(fabs(v.as.f));
}

static inline EnlngVal enlng_builtin_floor(EnlngVal v) {
  double d = (v.type == ENLNG_VAL_INT) ? (double)v.as.i : v.as.f;
  return enlng_make_int((int64_t)floor(d));
}

static inline EnlngVal enlng_builtin_ceil(EnlngVal v) {
  double d = (v.type == ENLNG_VAL_INT) ? (double)v.as.i : v.as.f;
  return enlng_make_int((int64_t)ceil(d));
}

static inline EnlngVal enlng_builtin_round(EnlngVal v) {
  double d = (v.type == ENLNG_VAL_INT) ? (double)v.as.i : v.as.f;
  return enlng_make_int((int64_t)round(d));
}

static inline EnlngVal enlng_builtin_min(EnlngVal a, EnlngVal b) {
  return enlng_vals_lt(a, b) ? a : b;
}

static inline EnlngVal enlng_builtin_max(EnlngVal a, EnlngVal b) {
  return enlng_vals_gt(a, b) ? a : b;
}

static inline EnlngVal enlng_builtin_random_number(EnlngVal low,
                                                   EnlngVal high) {
  int64_t l = enlng_val_to_int(low);
  int64_t h = enlng_val_to_int(high);
  if (h < l) {
    int64_t t = l;
    l = h;
    h = t;
  }
  int64_t diff = h - l + 1;
  if (diff <= 0)
    return enlng_make_int(l);
  return enlng_make_int(l + (rand() % diff));
}

/* --- File I/O Built-ins --- */
static inline EnlngVal enlng_builtin_read_file(EnlngVal path) {
  if (path.type != ENLNG_VAL_STRING || !path.as.s)
    return enlng_make_string("");
  FILE *f = fopen(path.as.s, "rb");
  if (!f)
    return enlng_make_string("");
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc(len + 1);
  if (buf) {
    fread(buf, 1, len, f);
    buf[len] = '\0';
  }
  fclose(f);
  EnlngVal res = enlng_make_string(buf ? buf : "");
  if (buf)
    free(buf);
  return res;
}

static inline EnlngVal enlng_builtin_write_file(EnlngVal path,
                                                EnlngVal content) {
  if (path.type != ENLNG_VAL_STRING || !path.as.s)
    return enlng_make_bool(false);
  FILE *f = fopen(path.as.s, "wb");
  if (!f)
    return enlng_make_bool(false);
  const char *data =
      (content.type == ENLNG_VAL_STRING && content.as.s) ? content.as.s : "";
  fwrite(data, 1, strlen(data), f);
  fclose(f);
  return enlng_make_bool(true);
}

static inline EnlngVal enlng_builtin_append_file(EnlngVal path,
                                                 EnlngVal content) {
  if (path.type != ENLNG_VAL_STRING || !path.as.s)
    return enlng_make_bool(false);
  FILE *f = fopen(path.as.s, "ab");
  if (!f)
    return enlng_make_bool(false);
  const char *data =
      (content.type == ENLNG_VAL_STRING && content.as.s) ? content.as.s : "";
  fwrite(data, 1, strlen(data), f);
  fclose(f);
  return enlng_make_bool(true);
}

static inline EnlngVal enlng_builtin_file_exists(EnlngVal path) {
  if (path.type != ENLNG_VAL_STRING || !path.as.s)
    return enlng_make_bool(false);
  FILE *f = fopen(path.as.s, "r");
  if (f) {
    fclose(f);
    return enlng_make_bool(true);
  }
  return enlng_make_bool(false);
}

/* --- System & OS Built-ins --- */
static inline EnlngVal enlng_builtin_time_now(void) {
  return enlng_make_int((int64_t)time(NULL));
}

static inline EnlngVal enlng_builtin_sleep(EnlngVal sec) {
  int64_t ms = (int64_t)(enlng_val_to_int(sec) * 1000);
#ifdef _WIN32
  Sleep((DWORD)ms);
#else
  usleep((useconds_t)(ms * 1000));
#endif
  return enlng_make_null();
}

/* --- Container Operations Built-ins --- */
static inline EnlngVal enlng_builtin_append(EnlngVal list, EnlngVal item) {
  if (list.type == ENLNG_VAL_LIST && list.as.l) {
    enlng_list_push(list.as.l, item);
  }
  return list;
}

static inline EnlngVal enlng_builtin_pop(EnlngVal list) {
  if (list.type == ENLNG_VAL_LIST && list.as.l && list.as.l->count > 0) {
    EnlngVal last = list.as.l->items[list.as.l->count - 1];
    list.as.l->count--;
    return last;
  }
  return enlng_make_null();
}

static inline EnlngVal enlng_builtin_keys(EnlngVal map) {
  return enlng_map_keys(map);
}

static inline EnlngVal enlng_builtin_values(EnlngVal map) {
  return enlng_map_values(map);
}

static inline EnlngVal enlng_builtin_has_key(EnlngVal map, EnlngVal key) {
  const char *k = (key.type == ENLNG_VAL_STRING && key.as.s) ? key.as.s : "";
  return enlng_make_bool(enlng_map_has(map, k));
}

static inline EnlngVal enlng_builtin_len(EnlngVal v) {
  return enlng_make_int(enlng_count_of(v));
}

static inline EnlngVal enlng_builtin_split(EnlngVal str, EnlngVal sep) {
  EnlngList *res = enlng_list_create();
  if (str.type != ENLNG_VAL_STRING || !str.as.s)
    return enlng_make_list(res);
  const char *s = str.as.s;
  const char *d = (sep.type == ENLNG_VAL_STRING && sep.as.s) ? sep.as.s : " ";
  size_t dlen = strlen(d);
  if (dlen == 0) {
    /* Split into individual characters */
    size_t slen = strlen(s);
    for (size_t i = 0; i < slen; i++) {
      char ch[2] = {s[i], '\0'};
      enlng_list_push(res, enlng_make_string(ch));
    }
    return enlng_make_list(res);
  }
  char *copy = (char *)malloc(strlen(s) + 1);
  strcpy(copy, s);
  char *token = strtok(copy, d);
  while (token) {
    enlng_list_push(res, enlng_make_string(token));
    token = strtok(NULL, d);
  }
  free(copy);
  return enlng_make_list(res);
}

static inline EnlngVal enlng_builtin_join(EnlngVal list, EnlngVal sep) {
  if (list.type != ENLNG_VAL_LIST || !list.as.l)
    return enlng_make_string("");
  const char *d = (sep.type == ENLNG_VAL_STRING && sep.as.s) ? sep.as.s : "";
  size_t dlen = strlen(d);
  size_t total_len = 0;
  for (int i = 0; i < list.as.l->count; i++) {
    EnlngVal item = list.as.l->items[i];
    if (item.type == ENLNG_VAL_STRING && item.as.s)
      total_len += strlen(item.as.s);
    if (i < list.as.l->count - 1)
      total_len += dlen;
  }
  char *buf = (char *)malloc(total_len + 1);
  buf[0] = '\0';
  for (int i = 0; i < list.as.l->count; i++) {
    EnlngVal item = list.as.l->items[i];
    if (item.type == ENLNG_VAL_STRING && item.as.s)
      strcat(buf, item.as.s);
    if (i < list.as.l->count - 1)
      strcat(buf, d);
  }
  EnlngVal res = enlng_make_string(buf);
  free(buf);
  return res;
}

static inline EnlngVal enlng_builtin_contains(EnlngVal coll, EnlngVal item) {
  if (coll.type == ENLNG_VAL_STRING && coll.as.s &&
      item.type == ENLNG_VAL_STRING && item.as.s) {
    return enlng_make_bool(strstr(coll.as.s, item.as.s) != NULL);
  }
  if (coll.type == ENLNG_VAL_LIST && coll.as.l) {
    for (int i = 0; i < coll.as.l->count; i++) {
      if (enlng_vals_equal(coll.as.l->items[i], item))
        return enlng_make_bool(true);
    }
    return enlng_make_bool(false);
  }
  if (coll.type == ENLNG_VAL_MAP && coll.as.m &&
      item.type == ENLNG_VAL_STRING && item.as.s) {
    return enlng_make_bool(enlng_map_has(coll, item.as.s));
  }
  return enlng_make_bool(false);
}

#endif /* ENLNG_RUNTIME_H */
