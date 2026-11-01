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

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#ifndef __USE_MINGW_ANSI_STDIO
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include <ctype.h>
#include <math.h>
#include <setjmp.h>
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
  bool frozen; /* set by `freeze`: no change allowed, wherever it is held */
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
  bool frozen;
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
  if (v.type == ENLNG_VAL_STRING && v.as.s)
    return (int64_t)atoll(v.as.s);
  return 0;
}

/* --- Dynamic List Operations --- */
static inline EnlngList *enlng_list_create(void) {
  EnlngList *list = (EnlngList *)malloc(sizeof(EnlngList));
  list->count = 0;
  list->frozen = false;
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
  m->frozen = false;
  m->buckets = (EnlngMapEntry **)calloc(m->capacity, sizeof(EnlngMapEntry *));
  return m;
}

static inline EnlngVal enlng_make_map(EnlngMap *m) {
  EnlngVal val;
  val.type = ENLNG_VAL_MAP;
  val.as.m = m;
  return val;
}

/* --- freeze: a frozen list or map refuses every change --- */
static inline bool enlng_is_frozen(EnlngVal v) {
  if (v.type == ENLNG_VAL_LIST && v.as.l)
    return v.as.l->frozen;
  if (v.type == ENLNG_VAL_MAP && v.as.m)
    return v.as.m->frozen;
  return false;
}

/* --- try / on error: a thrown error jumps to the innermost open `try`,
   or stops the program with a message when there is none --- */
#define ENLNG_MAX_TRY 256
static struct {
  jmp_buf eg_jb[ENLNG_MAX_TRY];
  int eg_depth; /* how many `try` blocks are open right now */
  char eg_msg[512];
} enlng_try_stack;

static inline void enlng_throw(const char *msg) {
  if (!msg)
    msg = "error";
  if (enlng_try_stack.eg_depth > 0) {
    snprintf(enlng_try_stack.eg_msg, sizeof(enlng_try_stack.eg_msg), "%s", msg);
    longjmp(enlng_try_stack.eg_jb[enlng_try_stack.eg_depth - 1], 1);
  }
  fflush(stdout);
  fprintf(stderr,
          "\n-- Enlang RuntimeError --------------------------------------------------\n"
          "What: %s\n"
          "------------------------------------------------------------------------\n\n",
          msg);
  exit(1);
}

static inline void enlng_frozen_refuse(void) {
  enlng_throw("This list or map was made with 'freeze', so it cannot be changed, "
              "not even inside a function. Use 'remember' if it needs to change.");
}

#define ENLNG_NOT_FROZEN(v)\
  do {\
    if (enlng_is_frozen(v))\
      enlng_frozen_refuse();\
  } while (0)

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
  ENLNG_NOT_FROZEN(v);
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

/* `freeze x as <value>`: a shallow copy, marked frozen, so freezing never
   locks a list that some other name still uses. */
static inline EnlngVal enlng_freeze_val(EnlngVal v) {
  if (v.type == ENLNG_VAL_LIST && v.as.l) {
    EnlngList *c = enlng_list_create();
    for (int i = 0; i < v.as.l->count; i++)
      enlng_list_push(c, v.as.l->items[i]);
    c->frozen = true;
    return enlng_make_list(c);
  }
  if (v.type == ENLNG_VAL_MAP && v.as.m) {
    EnlngMap *src = v.as.m;
    EnlngVal out = enlng_make_map(enlng_map_create());
    for (int b = 0; b < src->capacity; b++)
      for (EnlngMapEntry *e = src->buckets[b]; e; e = e->next)
        enlng_map_set(out, e->key, e->val);
    out.as.m->frozen = true;
    return out;
  }
  return v;
}

/* `for each k in m`: a map is walked by its keys, a list or text item by item */
static inline EnlngVal enlng_each_source(EnlngVal v) {
  if (v.type == ENLNG_VAL_MAP)
    return enlng_map_keys(v);
  return v;
}

/* `for i from a to b by s`: a positive step counts up to b, a negative one
   counts down to b, and a step of 0 would never end. */
static inline void enlng_check_step(int64_t step) {
  if (step == 0)
    enlng_throw("This loop would never end because the step is 0. Use a "
                "step such as 1, or -1 to count down.");
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
    ENLNG_NOT_FROZEN(v);
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
    if (key.type == ENLNG_VAL_STRING && key.as.s) {
      if (strcmp(key.as.s, "at_last") == 0) {
        int64_t cnt = enlng_count_of(c);
        return enlng_list_get(c, cnt > 0 ? cnt - 1 : 0);
      }
      if (strcmp(key.as.s, "at_first") == 0) {
        return enlng_list_get(c, 0);
      }
    }
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
    ENLNG_NOT_FROZEN(v);
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

/* Permutation / arrangement expression */
static inline EnlngVal enlng_val_arrange(EnlngVal target, EnlngVal indices) {
  if (target.type == ENLNG_VAL_STRING && target.as.s) {
    int n = (int)strlen(target.as.s);
    if (indices.type == ENLNG_VAL_LIST && indices.as.l) {
      int count = indices.as.l->count;
      char *buf = (char *)malloc(count + 1);
      int out_len = 0;
      for (int i = 0; i < count; i++) {
        int64_t idx = enlng_val_to_int(indices.as.l->items[i]);
        if (idx < 0) idx = n + idx;
        if (idx >= 0 && idx < n) {
          buf[out_len++] = target.as.s[idx];
        }
      }
      buf[out_len] = '\0';
      EnlngVal res;
      res.type = ENLNG_VAL_STRING;
      res.as.s = buf;
      return res;
    }
    if (indices.type == ENLNG_VAL_INT) {
      int64_t idx = indices.as.i;
      if (idx < 0) idx = n + idx;
      char *buf = (char *)malloc(2);
      int out_len = 0;
      if (idx >= 0 && idx < n) {
        buf[out_len++] = target.as.s[idx];
      }
      buf[out_len] = '\0';
      EnlngVal res;
      res.type = ENLNG_VAL_STRING;
      res.as.s = buf;
      return res;
    }
  }
  if (target.type == ENLNG_VAL_LIST && target.as.l) {
    int n = target.as.l->count;
    if (indices.type == ENLNG_VAL_LIST && indices.as.l) {
      EnlngList *copy = enlng_list_create();
      int count = indices.as.l->count;
      for (int i = 0; i < count; i++) {
        int64_t idx = enlng_val_to_int(indices.as.l->items[i]);
        if (idx < 0) idx = n + idx;
        if (idx >= 0 && idx < n) {
          enlng_list_push(copy, target.as.l->items[idx]);
        }
      }
      return enlng_make_list(copy);
    }
    if (indices.type == ENLNG_VAL_INT) {
      EnlngList *copy = enlng_list_create();
      int64_t idx = indices.as.i;
      if (idx < 0) idx = n + idx;
      if (idx >= 0 && idx < n) {
        enlng_list_push(copy, target.as.l->items[idx]);
      }
      return enlng_make_list(copy);
    }
  }
  return target;
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
  /* Lists and maps are equal when they hold equal items. */
  if (a.type == ENLNG_VAL_LIST && b.type == ENLNG_VAL_LIST) {
    if (a.as.l == b.as.l)
      return true;
    if (!a.as.l || !b.as.l || a.as.l->count != b.as.l->count)
      return false;
    for (int i = 0; i < a.as.l->count; i++)
      if (!enlng_vals_equal(a.as.l->items[i], b.as.l->items[i]))
        return false;
    return true;
  }
  if (a.type == ENLNG_VAL_MAP && b.type == ENLNG_VAL_MAP) {
    if (a.as.m == b.as.m)
      return true;
    if (!a.as.m || !b.as.m || a.as.m->count != b.as.m->count)
      return false;
    for (int i = 0; i < a.as.m->capacity; i++)
      for (EnlngMapEntry *e = a.as.m->buckets[i]; e; e = e->next)
        if (!enlng_map_has(b, e->key) ||
            !enlng_vals_equal(e->val, enlng_map_get(b, e->key)))
          return false;
    return true;
  }
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
    ENLNG_NOT_FROZEN(v);
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
static inline bool enlng_is_num(EnlngVal v) {
  return v.type == ENLNG_VAL_INT || v.type == ENLNG_VAL_FLOAT;
}

static inline double enlng_num_d(EnlngVal v) {
  return v.type == ENLNG_VAL_FLOAT ? v.as.f : (double)v.as.i;
}

static inline const char *enlng_kind(EnlngVal v) {
  switch (v.type) {
  case ENLNG_VAL_INT:
  case ENLNG_VAL_FLOAT: return "a number";
  case ENLNG_VAL_STRING: return "text";
  case ENLNG_VAL_BOOL: return "true/false";
  case ENLNG_VAL_LIST: return "a list";
  case ENLNG_VAL_MAP: return "a map";
  default: return "null";
  }
}

/* An operator was given values it cannot work on, such as "a" minus 1. */
static inline void enlng_bad_operands(const char *op, EnlngVal a, EnlngVal b) {
  char msg[300];
  snprintf(msg, sizeof(msg), "'%s' needs two numbers, but got %s and %s.%s", op,
           enlng_kind(a), enlng_kind(b),
           (a.type == ENLNG_VAL_NULL || b.type == ENLNG_VAL_NULL)
               ? " A missing key or position gives null; check it with has_key "
                 "or 'is null' first."
               : "");
  enlng_throw(msg);
}

/* A built-in such as sqrt was given something that is not a number. */
static inline double enlng_need_num(const char *fn, EnlngVal v) {
  if (!enlng_is_num(v)) {
    char msg[200];
    snprintf(msg, sizeof(msg), "'%s' needs a number, but got %s.", fn, enlng_kind(v));
    enlng_throw(msg);
  }
  return enlng_num_d(v);
}

static inline EnlngVal enlng_val_add(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT)
    return enlng_make_int(a.as.i + b.as.i);
  if (enlng_is_num(a) && enlng_is_num(b))
    return enlng_make_float(enlng_num_d(a) + enlng_num_d(b));
  if ((a.type == ENLNG_VAL_STRING || b.type == ENLNG_VAL_STRING) &&
      a.type != ENLNG_VAL_LIST && a.type != ENLNG_VAL_MAP &&
      b.type != ENLNG_VAL_LIST && b.type != ENLNG_VAL_MAP) {
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
  enlng_bad_operands("plus", a, b);
  return enlng_make_null();
}

static inline EnlngVal enlng_val_sub(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT)
    return enlng_make_int(a.as.i - b.as.i);
  if (!enlng_is_num(a) || !enlng_is_num(b)) {
    enlng_bad_operands("minus", a, b);
    return enlng_make_null();
  }
  return enlng_make_float(enlng_num_d(a) - enlng_num_d(b));
}

static inline EnlngVal enlng_val_mul(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT)
    return enlng_make_int(a.as.i * b.as.i);
  /* text times a whole number repeats the text */
  if ((a.type == ENLNG_VAL_STRING && b.type == ENLNG_VAL_INT) ||
      (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_STRING)) {
    const char *str = (a.type == ENLNG_VAL_STRING) ? (a.as.s ? a.as.s : "") : (b.as.s ? b.as.s : "");
    int64_t count = (a.type == ENLNG_VAL_STRING) ? enlng_val_to_int(b) : enlng_val_to_int(a);
    if (count <= 0)
      return enlng_make_string("");
    size_t slen = strlen(str);
    size_t total_len = slen * (size_t)count;
    char *buf = (char *)malloc(total_len + 1);
    if (!buf) return enlng_make_string("");
    for (int64_t i = 0; i < count; i++) {
      memcpy(buf + (i * slen), str, slen);
    }
    buf[total_len] = '\0';
    EnlngVal res;
    res.type = ENLNG_VAL_STRING;
    res.as.s = buf;
    return res;
  }
  if (!enlng_is_num(a) || !enlng_is_num(b)) {
    enlng_bad_operands("times", a, b);
    return enlng_make_null();
  }
  return enlng_make_float(enlng_num_d(a) * enlng_num_d(b));
}

static inline void enlng_zero_divisor(EnlngVal a, EnlngVal b, const char *op) {
  if (enlng_is_num(b) && enlng_num_d(b) == 0.0) {
    char buf[64], msg[200];
    snprintf(msg, sizeof(msg), "Cannot divide by zero: %s %s 0.",
             enlng_val_to_str_buf(a, buf, sizeof(buf)), op);
    enlng_throw(msg);
  }
}

static inline EnlngVal enlng_val_div(EnlngVal a, EnlngVal b) {
  if (!enlng_is_num(a) || !enlng_is_num(b)) {
    enlng_bad_operands("divided by", a, b);
    return enlng_make_null();
  }
  enlng_zero_divisor(a, b, "divided by");
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT)
    return enlng_make_int(b.as.i == -1 ? (int64_t)(0 - (uint64_t)a.as.i)
                                       : a.as.i / b.as.i);
  return enlng_make_float(enlng_num_d(a) / enlng_num_d(b));
}

static inline EnlngVal enlng_val_mod(EnlngVal a, EnlngVal b) {
  if (!enlng_is_num(a) || !enlng_is_num(b)) {
    enlng_bad_operands("mod", a, b);
    return enlng_make_null();
  }
  enlng_zero_divisor(a, b, "mod");
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT)
    return enlng_make_int(b.as.i == -1 ? 0 : a.as.i % b.as.i);
  return enlng_make_float(fmod(enlng_num_d(a), enlng_num_d(b)));
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
  double d = enlng_need_num("sqrt", v);
  return enlng_make_float(sqrt(d));
}

static inline EnlngVal enlng_builtin_pow(EnlngVal base, EnlngVal exp) {
  double b = enlng_need_num("pow", base);
  double e = enlng_need_num("pow", exp);
  return enlng_make_float(pow(b, e));
}

static inline EnlngVal enlng_builtin_abs(EnlngVal v) {
  if (v.type == ENLNG_VAL_INT) {
    return enlng_make_int(v.as.i < 0 ? -v.as.i : v.as.i);
  }
  return enlng_make_float(fabs(enlng_need_num("abs", v)));
}

static inline EnlngVal enlng_builtin_floor(EnlngVal v) {
  double d = enlng_need_num("floor", v);
  return enlng_make_int((int64_t)floor(d));
}

static inline EnlngVal enlng_builtin_ceil(EnlngVal v) {
  double d = enlng_need_num("ceil", v);
  return enlng_make_int((int64_t)ceil(d));
}

static inline EnlngVal enlng_builtin_round(EnlngVal v) {
  double d = enlng_need_num("round", v);
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
  usleep((unsigned int)(ms * 1000));
#endif
  return enlng_make_null();
}

/* --- Container Operations Built-ins --- */
static inline EnlngVal enlng_builtin_append(EnlngVal list, EnlngVal item) {
  if (list.type == ENLNG_VAL_LIST && list.as.l) {
    ENLNG_NOT_FROZEN(list);
    enlng_list_push(list.as.l, item);
  }
  return list;
}

static inline EnlngVal enlng_builtin_pop(EnlngVal list) {
  if (list.type == ENLNG_VAL_LIST && list.as.l && list.as.l->count > 0) {
    ENLNG_NOT_FROZEN(list);
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

/* items[a:b] and slc(items, a, b): the part from position a up to, not including, b. A null bound
 * means the start or the end; a negative one counts from the end. Bounds
 * outside the value are clamped, so a slice never fails. */
static inline int64_t enlng_slice_bound(EnlngVal b, int64_t len, int64_t dflt) {
  if (b.type == ENLNG_VAL_NULL)
    return dflt;
  int64_t i = enlng_val_to_int(b);
  if (i < 0)
    i += len;
  if (i < 0)
    i = 0;
  if (i > len)
    i = len;
  return i;
}

static inline EnlngVal enlng_builtin_slc(EnlngVal v, EnlngVal from, EnlngVal to) {
  int64_t len = enlng_count_of(v);
  int64_t a = enlng_slice_bound(from, len, 0);
  int64_t b = enlng_slice_bound(to, len, len);
  if (b < a)
    b = a;
  if (v.type == ENLNG_VAL_LIST && v.as.l) {
    EnlngList *res = enlng_list_create();
    for (int64_t i = a; i < b; i++)
      enlng_list_push(res, v.as.l->items[i]);
    return enlng_make_list(res);
  }
  if (v.type == ENLNG_VAL_STRING && v.as.s) {
    char *buf = (char *)malloc((size_t)(b - a) + 1);
    memcpy(buf, v.as.s + a, (size_t)(b - a));
    buf[b - a] = '\0';
    EnlngVal res = enlng_make_string(buf);
    free(buf);
    return res;
  }
  return enlng_make_null();
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
  /* Split on the exact delimiter text. Empty pieces are kept, so
   * "a,,b" gives three items and "-a-" gives ["", "a", ""]. */
  if (s[0] == '\0')
    return enlng_make_list(res);
  const char *start = s;
  const char *hit;
  while ((hit = strstr(start, d)) != NULL) {
    size_t n = (size_t)(hit - start);
    char *piece = (char *)malloc(n + 1);
    memcpy(piece, start, n);
    piece[n] = '\0';
    enlng_list_push(res, enlng_make_string(piece));
    free(piece);
    start = hit + dlen;
  }
  enlng_list_push(res, enlng_make_string(start));
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

static inline int64_t enlng_count_in(EnlngVal target, EnlngVal container) {
  if (container.type == ENLNG_VAL_STRING && container.as.s) {
    if (target.type != ENLNG_VAL_STRING || !target.as.s) return 0;
    const char *s = container.as.s;
    const char *sub = target.as.s;
    size_t sub_len = strlen(sub);
    if (sub_len == 0) return 0;
    int64_t count = 0;
    const char *p = s;
    while ((p = strstr(p, sub)) != NULL) {
      count++;
      p += sub_len;
    }
    return count;
  }
  if (container.type == ENLNG_VAL_LIST && container.as.l) {
    int64_t count = 0;
    for (int i = 0; i < container.as.l->count; i++) {
      if (enlng_vals_equal(container.as.l->items[i], target)) {
        count++;
      }
    }
    return count;
  }
  return 0;
}

static inline EnlngVal enlng_builtin_replace(EnlngVal target, EnlngVal repl, EnlngVal container) {
  if (container.type != ENLNG_VAL_STRING || !container.as.s) return container;
  if (target.type != ENLNG_VAL_STRING || !target.as.s) return container;
  const char *s = container.as.s;
  const char *t = target.as.s;
  const char *r = (repl.type == ENLNG_VAL_STRING && repl.as.s) ? repl.as.s : "";
  size_t tlen = strlen(t);
  if (tlen == 0) return container;
  size_t rlen = strlen(r);

  int count = 0;
  const char *p = s;
  while ((p = strstr(p, t)) != NULL) { count++; p += tlen; }
  if (count == 0) return container;

  size_t new_len = strlen(s) + count * (rlen > tlen ? (rlen - tlen) : 0) + 1;
  char *buf = (char *)malloc(new_len + 64);
  buf[0] = '\0';
  p = s;
  const char *next;
  while ((next = strstr(p, t)) != NULL) {
    size_t chunk = (size_t)(next - p);
    strncat(buf, p, chunk);
    strcat(buf, r);
    p = next + tlen;
  }
  strcat(buf, p);
  EnlngVal res = enlng_make_string(buf);
  free(buf);
  return res;
}

static inline EnlngVal enlng_builtin_find(EnlngVal target, EnlngVal container) {
  if (container.type == ENLNG_VAL_STRING && container.as.s) {
    if (target.type != ENLNG_VAL_STRING || !target.as.s) return enlng_make_int(-1);
    const char *p = strstr(container.as.s, target.as.s);
    if (!p) return enlng_make_int(-1);
    return enlng_make_int((int64_t)(p - container.as.s));
  }
  if (container.type == ENLNG_VAL_LIST && container.as.l) {
    for (int i = 0; i < container.as.l->count; i++) {
      if (enlng_vals_equal(container.as.l->items[i], target))
        return enlng_make_int(i);
    }
    return enlng_make_int(-1);
  }
  return enlng_make_int(-1);
}

static inline EnlngVal enlng_builtin_trim(EnlngVal v) {
  if (v.type != ENLNG_VAL_STRING || !v.as.s) return v;
  const char *s = v.as.s;
  while (*s && (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')) s++;
  if (*s == '\0') return enlng_make_string("");
  const char *e = s + strlen(s) - 1;
  while (e > s && (*e == ' ' || *e == '\t' || *e == '\n' || *e == '\r')) e--;
  size_t len = (size_t)(e - s + 1);
  char *buf = (char *)malloc(len + 1);
  strncpy(buf, s, len);
  buf[len] = '\0';
  EnlngVal res = enlng_make_string(buf);
  free(buf);
  return res;
}

static inline EnlngVal enlng_builtin_to_upper(EnlngVal v) {
  if (v.type != ENLNG_VAL_STRING || !v.as.s) return v;
  char *buf = (char *)malloc(strlen(v.as.s) + 1);
  for (size_t i = 0; v.as.s[i]; i++) buf[i] = (char)toupper((unsigned char)v.as.s[i]);
  buf[strlen(v.as.s)] = '\0';
  EnlngVal res = enlng_make_string(buf);
  free(buf);
  return res;
}

static inline EnlngVal enlng_builtin_to_lower(EnlngVal v) {
  if (v.type != ENLNG_VAL_STRING || !v.as.s) return v;
  char *buf = (char *)malloc(strlen(v.as.s) + 1);
  for (size_t i = 0; v.as.s[i]; i++) buf[i] = (char)tolower((unsigned char)v.as.s[i]);
  buf[strlen(v.as.s)] = '\0';
  EnlngVal res = enlng_make_string(buf);
  free(buf);
  return res;
}

static inline EnlngVal enlng_builtin_sum(EnlngVal list) {
  if (list.type != ENLNG_VAL_LIST || !list.as.l) return enlng_make_int(0);
  double total = 0;
  bool is_float = false;
  for (int i = 0; i < list.as.l->count; i++) {
    EnlngVal v = list.as.l->items[i];
    if (v.type == ENLNG_VAL_FLOAT) { total += v.as.f; is_float = true; }
    else if (v.type == ENLNG_VAL_INT) { total += (double)v.as.i; }
  }
  return is_float ? enlng_make_float(total) : enlng_make_int((int64_t)total);
}

static inline EnlngVal enlng_builtin_avg(EnlngVal list) {
  if (list.type != ENLNG_VAL_LIST || !list.as.l || list.as.l->count == 0) return enlng_make_float(0.0);
  double total = 0;
  for (int i = 0; i < list.as.l->count; i++) {
    EnlngVal v = list.as.l->items[i];
    if (v.type == ENLNG_VAL_FLOAT) total += v.as.f;
    else if (v.type == ENLNG_VAL_INT) total += (double)v.as.i;
  }
  return enlng_make_float(total / list.as.l->count);
}

static inline EnlngVal enlng_builtin_list_max(EnlngVal list) {
  if (list.type != ENLNG_VAL_LIST || !list.as.l || list.as.l->count == 0) return enlng_make_null();
  EnlngVal m = list.as.l->items[0];
  for (int i = 1; i < list.as.l->count; i++) {
    if (enlng_vals_gt(list.as.l->items[i], m)) m = list.as.l->items[i];
  }
  return m;
}

static inline EnlngVal enlng_builtin_list_min(EnlngVal list) {
  if (list.type != ENLNG_VAL_LIST || !list.as.l || list.as.l->count == 0) return enlng_make_null();
  EnlngVal m = list.as.l->items[0];
  for (int i = 1; i < list.as.l->count; i++) {
    if (enlng_vals_lt(list.as.l->items[i], m)) m = list.as.l->items[i];
  }
  return m;
}

/* Native String Mutation Primitives: insert, remove, replace, set */
static inline int64_t enlng_parse_position(EnlngVal pos, int64_t slen, bool is_add) {
  if (pos.type == ENLNG_VAL_STRING && pos.as.s) {
    const char *p = pos.as.s;
    if (strcmp(p, "at_first") == 0)
      return 0;
    if (strcmp(p, "at_last") == 0)
      return is_add ? slen : (slen > 0 ? slen - 1 : 0);
    int64_t val = (int64_t)atoll(p);
    if (val < 0) {
      if (is_add)
        return 0; /* Add rule: "-1" prepends to beginning */
      val = slen + val;
      if (val < 0)
        val = 0;
      return val;
    }
    if (is_add && val >= slen) {
      return slen; /* Add rule: overflow appends to end */
    }
    if (!is_add && val >= slen) {
      return slen > 0 ? slen - 1 : 0;
    }
    return val;
  }
  if (pos.type == ENLNG_VAL_INT) {
    int64_t val = pos.as.i;
    if (val < 0) {
      if (is_add)
        return 0; /* Add rule: -1 prepends to beginning */
      val = slen + val;
      if (val < 0)
        val = 0;
      return val;
    }
    if (is_add && val >= slen) {
      return slen; /* Add rule: overflow appends to end */
    }
    if (!is_add && val >= slen) {
      return slen > 0 ? slen - 1 : 0;
    }
    return val;
  }
  return 0;
}

static inline EnlngVal enlng_builtin_string_add(EnlngVal arg1, EnlngVal arg2, EnlngVal pos) {
  EnlngVal item = arg1;
  EnlngVal target = arg2;

  if (arg1.type == ENLNG_VAL_STRING && arg2.type == ENLNG_VAL_STRING && arg1.as.s && arg2.as.s) {
    size_t l1 = strlen(arg1.as.s);
    size_t l2 = strlen(arg2.as.s);
    if (l1 == 1 && l2 > 1) {
      item = arg1;
      target = arg2;
    } else if (l1 > 1 && l2 == 1) {
      target = arg1;
      item = arg2;
    } else if (l1 > l2) {
      target = arg1;
      item = arg2;
    } else if (l1 < l2) {
      int64_t pval = (pos.type == ENLNG_VAL_INT) ? pos.as.i : (pos.type == ENLNG_VAL_STRING && pos.as.s ? atoll(pos.as.s) : 0);
      if (pval <= (int64_t)l1) {
        target = arg1;
        item = arg2;
      } else {
        item = arg1;
        target = arg2;
      }
    }
  }

  if (target.type != ENLNG_VAL_STRING || !target.as.s)
    return target;

  const char *s = target.as.s;
  size_t slen = strlen(s);

  char temp[64];
  const char *to_insert = "";
  if (item.type == ENLNG_VAL_STRING && item.as.s) {
    to_insert = item.as.s;
  } else if (item.type == ENLNG_VAL_INT) {
    snprintf(temp, sizeof(temp), "%lld", (long long)item.as.i);
    to_insert = temp;
  }
  size_t ilen = strlen(to_insert);

  int64_t idx = enlng_parse_position(pos, (int64_t)slen, true);

  char *buf = (char *)malloc(slen + ilen + 1);
  memcpy(buf, s, (size_t)idx);
  memcpy(buf + idx, to_insert, ilen);
  strcpy(buf + idx + ilen, s + idx);

  EnlngVal res = enlng_make_string(buf);
  free(buf);
  return res;
}

static inline EnlngVal enlng_builtin_string_insert(EnlngVal arg1, EnlngVal arg2, EnlngVal pos) {
  return enlng_builtin_string_add(arg1, arg2, pos);
}

static inline EnlngVal enlng_builtin_string_remove_at(EnlngVal str, EnlngVal pos) {
  if (str.type != ENLNG_VAL_STRING || !str.as.s)
    return str;
  const char *s = str.as.s;
  size_t slen = strlen(s);
  if (slen == 0)
    return str;

  int64_t idx = enlng_parse_position(pos, (int64_t)slen, false);
  if (idx < 0 || idx >= (int64_t)slen)
    return str;

  char *buf = (char *)malloc(slen);
  memcpy(buf, s, (size_t)idx);
  strcpy(buf + idx, s + idx + 1);

  EnlngVal res = enlng_make_string(buf);
  free(buf);
  return res;
}

static inline EnlngVal enlng_builtin_string_remove(EnlngVal str, EnlngVal pos) {
  if (str.type != ENLNG_VAL_STRING || !str.as.s)
    return str;
  if (pos.type == ENLNG_VAL_INT) {
    return enlng_builtin_string_remove_at(str, pos);
  }
  if (pos.type == ENLNG_VAL_STRING && pos.as.s) {
    const char *p = pos.as.s;
    if (strcmp(p, "at_first") == 0 || strcmp(p, "at_last") == 0) {
      return enlng_builtin_string_remove_at(str, pos);
    }
    bool is_num = true;
    int i = 0;
    if (p[0] == '-' && p[1] != '\0') i = 1;
    if (p[i] == '\0') is_num = false;
    for (; p[i] != '\0'; i++) {
      if (p[i] < '0' || p[i] > '9') {
        is_num = false;
        break;
      }
    }
    if (is_num) {
      return enlng_builtin_string_remove_at(str, pos);
    }
    char *match_ptr = strstr(str.as.s, pos.as.s);
    if (!match_ptr)
      return str;
    size_t before_len = (size_t)(match_ptr - str.as.s);
    size_t t_len = strlen(pos.as.s);
    size_t after_len = strlen(match_ptr + t_len);
    char *buf = (char *)malloc(before_len + after_len + 1);
    memcpy(buf, str.as.s, before_len);
    strcpy(buf + before_len, match_ptr + t_len);
    EnlngVal res = enlng_make_string(buf);
    free(buf);
    return res;
  }
  return str;
}

static inline EnlngVal enlng_builtin_string_replace(EnlngVal arg1, EnlngVal arg2, EnlngVal pos) {
  EnlngVal new_val = arg1;
  EnlngVal target = arg2;

  if (arg1.type == ENLNG_VAL_STRING && arg2.type == ENLNG_VAL_STRING && arg1.as.s && arg2.as.s) {
    size_t l1 = strlen(arg1.as.s);
    size_t l2 = strlen(arg2.as.s);
    if (l1 == 1 && l2 > 1) {
      new_val = arg1;
      target = arg2;
    } else if (l1 > 1 && l2 == 1) {
      target = arg1;
      new_val = arg2;
    } else if (l1 > l2) {
      target = arg1;
      new_val = arg2;
    } else if (l1 < l2) {
      int64_t pval = (pos.type == ENLNG_VAL_INT) ? pos.as.i : (pos.type == ENLNG_VAL_STRING && pos.as.s ? atoll(pos.as.s) : 0);
      if (pval < (int64_t)l1) {
        target = arg1;
        new_val = arg2;
      } else {
        new_val = arg1;
        target = arg2;
      }
    }
  }

  if (target.type != ENLNG_VAL_STRING || !target.as.s)
    return target;

  const char *s = target.as.s;
  size_t slen = strlen(s);
  if (slen == 0)
    return target;

  char temp[64];
  const char *rep = "";
  if (new_val.type == ENLNG_VAL_STRING && new_val.as.s) {
    rep = new_val.as.s;
  } else if (new_val.type == ENLNG_VAL_INT) {
    snprintf(temp, sizeof(temp), "%lld", (long long)new_val.as.i);
    rep = temp;
  }
  size_t rlen = strlen(rep);

  int64_t idx = enlng_parse_position(pos, (int64_t)slen, false);

  char *buf = (char *)malloc(slen + rlen + 1);
  memcpy(buf, s, (size_t)idx);
  memcpy(buf + idx, rep, rlen);
  strcpy(buf + idx + rlen, s + idx + 1);

  EnlngVal res = enlng_make_string(buf);
  free(buf);
  return res;
}

static inline EnlngVal enlng_builtin_string_set_at(EnlngVal str, EnlngVal pos, EnlngVal new_char) {
  return enlng_builtin_string_replace(new_char, str, pos);
}

#endif /* ENLNG_RUNTIME_H */
