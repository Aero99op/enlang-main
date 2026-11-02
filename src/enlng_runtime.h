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
#include <errno.h>
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
typedef struct EnlngFunc EnlngFunc;

/* --- Console: text is UTF-8, so the Windows console must show UTF-8 while
   the program runs; the code page it had is put back when the program ends. */
#ifdef _WIN32
static UINT enlng_saved_cp = 0;
static void enlng_restore_console(void) {
  if (enlng_saved_cp)
    SetConsoleOutputCP(enlng_saved_cp);
}
static inline void enlng_console_utf8(void) {
  enlng_saved_cp = GetConsoleOutputCP();
  if (enlng_saved_cp && enlng_saved_cp != 65001) {
    SetConsoleOutputCP(65001);
    atexit(enlng_restore_console);
  }
}
#else
static inline void enlng_console_utf8(void) {}
#endif

/* --- Heap objects ---------------------------------------------------------
   Every text, list and map the program makes starts with this header and is
   linked into one list. The built-in interpreter uses it to find and free the
   ones the program can no longer reach (enlng_gc_* below); a program built
   with --native never collects, and the header only costs a few bytes. */
#define ENLNG_GC_MAGIC 0xE61C
enum { ENLNG_OBJ_STRING = 1, ENLNG_OBJ_LIST, ENLNG_OBJ_MAP, ENLNG_OBJ_FUNC };

typedef struct EnlngObj {
  struct EnlngObj *next;
  unsigned short magic;
  unsigned char kind;
  unsigned char mark;
} EnlngObj;

static EnlngObj *enlng_gc_all = NULL;  /* every live object */
static size_t enlng_gc_count = 0;      /* how many there are */
static size_t enlng_gc_new = 0;        /* made since the last collection */
static size_t enlng_gc_live = 0;       /* still in use after the last collection */

static inline void enlng_gc_link(EnlngObj *o, int kind) {
  o->magic = ENLNG_GC_MAGIC;
  o->kind = (unsigned char)kind;
  o->mark = 0;
  o->next = enlng_gc_all;
  enlng_gc_all = o;
  enlng_gc_count++;
  enlng_gc_new++;
}

/* --- Value Types --- */
typedef enum {
  ENLNG_VAL_INT,
  ENLNG_VAL_FLOAT,
  ENLNG_VAL_STRING,
  ENLNG_VAL_BOOL,
  ENLNG_VAL_LIST,
  ENLNG_VAL_MAP,
  ENLNG_VAL_NULL,
  ENLNG_VAL_FUNC /* a function used as a value; as.fn says which */
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
    EnlngFunc *fn;
  } as;
};

/* A function used as a value. An inner function or `function with x: ...`
   keeps the values its outer names had when it was made (caps); they are
   handed to it after the caller's inputs. */
struct EnlngFunc {
  EnlngObj gc;
  const char *name;  /* the function that runs, or the built-in (not owned) */
  const char *shown; /* the name `show` prints (not owned) */
  bool builtin;
  int ncap;
  EnlngVal *caps;
};

struct EnlngList {
  EnlngObj gc;
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
  EnlngObj gc;
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
  size_t len = s ? strlen(s) : 0;
  EnlngObj *o = (EnlngObj *)malloc(sizeof(EnlngObj) + len + 1);
  enlng_gc_link(o, ENLNG_OBJ_STRING);
  val.as.s = (char *)(o + 1);
  if (s)
    memcpy(val.as.s, s, len + 1);
  else
    val.as.s[0] = '\0';
  return val;
}

static inline EnlngVal enlng_make_closure(const char *name, const char *shown,
                                          bool builtin, int ncap,
                                          const EnlngVal *caps) {
  EnlngFunc *fn = (EnlngFunc *)malloc(sizeof(EnlngFunc));
  enlng_gc_link(&fn->gc, ENLNG_OBJ_FUNC);
  fn->name = name;
  fn->shown = shown;
  fn->builtin = builtin;
  fn->ncap = ncap;
  fn->caps = NULL;
  if (ncap > 0) {
    fn->caps = (EnlngVal *)malloc(sizeof(EnlngVal) * (size_t)ncap);
    memcpy(fn->caps, caps, sizeof(EnlngVal) * (size_t)ncap);
  }
  EnlngVal val;
  val.type = ENLNG_VAL_FUNC;
  val.as.fn = fn;
  return val;
}

static inline EnlngVal enlng_make_func(const char *name) {
  return enlng_make_closure(name, name, false, 0, NULL);
}

static inline EnlngVal enlng_make_builtin_func(const char *name) {
  return enlng_make_closure(name, name, true, 0, NULL);
}

/* --- Text is UTF-8: positions and counts are in characters, not bytes --- */
static inline int enlng_u8_clen(unsigned char c) {
  if (c < 0x80)
    return 1;
  if ((c & 0xE0) == 0xC0)
    return 2;
  if ((c & 0xF0) == 0xE0)
    return 3;
  if ((c & 0xF8) == 0xF0)
    return 4;
  return 1; /* a stray byte counts as one character */
}

/* bytes of the character at s, never past the end of the text */
static inline int enlng_u8_step(const char *s) {
  int k = enlng_u8_clen((unsigned char)s[0]);
  for (int i = 1; i < k; i++)
    if (s[i] == '\0')
      return i;
  return k;
}

static inline int64_t enlng_u8_count(const char *s) {
  int64_t n = 0;
  while (*s) {
    s += enlng_u8_step(s);
    n++;
  }
  return n;
}

/* byte offset of character number idx (the end of the text if past it) */
static inline size_t enlng_u8_off(const char *s, int64_t idx) {
  const char *p = s;
  while (idx > 0 && *p) {
    p += enlng_u8_step(p);
    idx--;
  }
  return (size_t)(p - s);
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
  enlng_gc_link(&list->gc, ENLNG_OBJ_LIST);
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
  enlng_gc_link(&m->gc, ENLNG_OBJ_MAP);
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
  /* Grow before the chains get long, so lookups stay fast on big maps */
  if (m->count + 1 > m->capacity * 3 / 4) {
    int ncap = m->capacity * 2;
    EnlngMapEntry **nb = (EnlngMapEntry **)calloc(ncap, sizeof(EnlngMapEntry *));
    for (int i = 0; i < m->capacity; i++) {
      EnlngMapEntry *x = m->buckets[i];
      while (x) {
        EnlngMapEntry *next = x->next;
        unsigned long nh = enlng_hash_str(x->key) % ncap;
        x->next = nb[nh];
        nb[nh] = x;
        x = next;
      }
    }
    free(m->buckets);
    m->buckets = nb;
    m->capacity = ncap;
    h = enlng_hash_str(key) % m->capacity;
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
static inline EnlngVal enlng_builtin_split(EnlngVal str, EnlngVal sep);
static inline EnlngVal enlng_each_source(EnlngVal v) {
  if (v.type == ENLNG_VAL_MAP)
    return enlng_map_keys(v);
  if (v.type == ENLNG_VAL_STRING) /* characters, found in one pass */
    return enlng_builtin_split(v, enlng_make_string(""));
  return v;
}

/* `for i from a to b by s`: a positive step counts up to b, a negative one
   counts down to b, and a step of 0 would never end. */
static inline void enlng_check_step(int64_t step) {
  if (step == 0)
    enlng_throw("This loop would never end because the step is 0. Use a "
                "step such as 1, or -1 to count down.");
}

/* --- Collector (used by the built-in interpreter) ---
   enlng_gc_mark_val() marks what a root value reaches, with an explicit stack
   so deep nesting cannot overflow the C stack; enlng_gc_sweep() frees every
   object left unmarked. Set ENLNG_GC_STRESS=1 to collect at every chance and
   fill freed memory with junk, which makes a missing root show up at once. */
static EnlngObj **enlng_gc_stack = NULL;
static size_t enlng_gc_sp = 0, enlng_gc_cap = 0;
static int enlng_gc_stress = -1;

static inline EnlngObj *enlng_gc_obj_of(EnlngVal v) {
  if (v.type == ENLNG_VAL_STRING && v.as.s) {
    EnlngObj *o = ((EnlngObj *)v.as.s) - 1;
    return o->magic == ENLNG_GC_MAGIC && o->kind == ENLNG_OBJ_STRING ? o : NULL;
  }
  if (v.type == ENLNG_VAL_LIST && v.as.l)
    return &v.as.l->gc;
  if (v.type == ENLNG_VAL_MAP && v.as.m)
    return &v.as.m->gc;
  if (v.type == ENLNG_VAL_FUNC && v.as.fn)
    return &v.as.fn->gc;
  return NULL;
}

static inline void enlng_gc_push(EnlngVal v) {
  EnlngObj *o = enlng_gc_obj_of(v);
  if (!o || o->mark)
    return;
  o->mark = 1;
  if (o->kind == ENLNG_OBJ_STRING)
    return;
  if (enlng_gc_sp >= enlng_gc_cap) {
    enlng_gc_cap = enlng_gc_cap ? enlng_gc_cap * 2 : 256;
    enlng_gc_stack = (EnlngObj **)realloc(enlng_gc_stack, sizeof(EnlngObj *) * enlng_gc_cap);
  }
  enlng_gc_stack[enlng_gc_sp++] = o;
}

static inline void enlng_gc_mark_val(EnlngVal v) {
  enlng_gc_push(v);
  while (enlng_gc_sp > 0) {
    EnlngObj *o = enlng_gc_stack[--enlng_gc_sp];
    if (o->kind == ENLNG_OBJ_LIST) {
      EnlngList *l = (EnlngList *)o;
      for (int i = 0; i < l->count; i++)
        enlng_gc_push(l->items[i]);
    } else if (o->kind == ENLNG_OBJ_MAP) {
      EnlngMap *m = (EnlngMap *)o;
      for (int b = 0; b < m->capacity; b++)
        for (EnlngMapEntry *e = m->buckets[b]; e; e = e->next)
          enlng_gc_push(e->val);
    } else if (o->kind == ENLNG_OBJ_FUNC) {
      EnlngFunc *fn = (EnlngFunc *)o;
      for (int i = 0; i < fn->ncap; i++)
        enlng_gc_push(fn->caps[i]);
    }
  }
}

static inline void enlng_gc_free_obj(EnlngObj *o) {
  bool junk = enlng_gc_stress > 0;
  if (o->kind == ENLNG_OBJ_LIST) {
    EnlngList *l = (EnlngList *)o;
    if (junk)
      memset(l->items, 0xDD, sizeof(EnlngVal) * (size_t)l->capacity);
    free(l->items);
  } else if (o->kind == ENLNG_OBJ_MAP) {
    EnlngMap *m = (EnlngMap *)o;
    for (int b = 0; b < m->capacity; b++) {
      EnlngMapEntry *e = m->buckets[b];
      while (e) {
        EnlngMapEntry *next = e->next;
        free(e->key);
        free(e);
        e = next;
      }
    }
    free(m->buckets);
  } else if (o->kind == ENLNG_OBJ_FUNC) {
    free(((EnlngFunc *)o)->caps);
  } else if (junk) {
    char *s = (char *)(o + 1);
    memset(s, 0xDD, strlen(s));
  }
  if (junk)
    o->magic = 0;
  free(o);
}

static inline void enlng_gc_sweep(void) {
  EnlngObj **link = &enlng_gc_all;
  size_t alive = 0;
  while (*link) {
    EnlngObj *o = *link;
    if (o->mark) {
      o->mark = 0;
      alive++;
      link = &o->next;
    } else {
      *link = o->next;
      enlng_gc_free_obj(o);
    }
  }
  enlng_gc_count = alive;
  enlng_gc_live = alive;
  enlng_gc_new = 0;
}

/* Time to collect: many objects were made since the last time, relative to
   how many were still in use then. */
static inline bool enlng_gc_due(void) {
  if (enlng_gc_stress < 0) {
    const char *e = getenv("ENLNG_GC_STRESS");
    enlng_gc_stress = (e && e[0] == '1') ? 1 : 0;
  }
  if (enlng_gc_stress)
    return true;
  return enlng_gc_new > 100000 && enlng_gc_new > enlng_gc_live;
}

/* --- Container Length --- */
static inline int64_t enlng_count_of(EnlngVal v) {
  if (v.type == ENLNG_VAL_LIST && v.as.l)
    return v.as.l->count;
  if (v.type == ENLNG_VAL_STRING && v.as.s)
    return enlng_u8_count(v.as.s);
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
    if (idx < 0)
      return enlng_make_null();
    const char *p = v.as.s + enlng_u8_off(v.as.s, idx);
    if (*p == '\0')
      return enlng_make_null();
    char buf[8];
    int k = enlng_u8_step(p);
    memcpy(buf, p, (size_t)k);
    buf[k] = '\0';
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
    size_t at = len;
    for (const char *p = v.as.s; *p;) {
      int k = enlng_u8_step(p);
      at -= (size_t)k;
      memcpy(rev + at, p, (size_t)k);
      p += k;
    }
    rev[len] = '\0';
    EnlngVal res = enlng_make_string(rev);
    free(rev);
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
    int64_t n = enlng_u8_count(target.as.s);
    int count = 0;
    EnlngVal *picks = NULL;
    if (indices.type == ENLNG_VAL_LIST && indices.as.l) {
      count = indices.as.l->count;
      picks = indices.as.l->items;
    } else if (indices.type == ENLNG_VAL_INT) {
      count = 1;
      picks = &indices;
    }
    if (picks) {
      char *buf = (char *)malloc((size_t)count * 4 + 1);
      size_t out_len = 0;
      for (int i = 0; i < count; i++) {
        int64_t idx = enlng_val_to_int(picks[i]);
        if (idx < 0) idx = n + idx;
        if (idx >= 0 && idx < n) {
          const char *p = target.as.s + enlng_u8_off(target.as.s, idx);
          int k = enlng_u8_step(p);
          memcpy(buf + out_len, p, (size_t)k);
          out_len += (size_t)k;
        }
      }
      buf[out_len] = '\0';
      EnlngVal res = enlng_make_string(buf);
      free(buf);
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
  if (v.type == ENLNG_VAL_FUNC)
    return true;
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
  if (a.type == ENLNG_VAL_FUNC && b.type == ENLNG_VAL_FUNC)
    return a.as.fn == b.as.fn ||
           (a.as.fn->ncap == 0 && b.as.fn->ncap == 0 &&
            strcmp(a.as.fn->name, b.as.fn->name) == 0);
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

/* In-place list sort: a stable merge sort, so equal items keep their order */
static inline void enlng_merge_sort(EnlngVal *a, EnlngVal *tmp, int n,
                                    bool descending) {
  if (n < 2)
    return;
  int mid = n / 2;
  enlng_merge_sort(a, tmp, mid, descending);
  enlng_merge_sort(a + mid, tmp, n - mid, descending);
  int i = 0, j = mid, k = 0;
  while (i < mid && j < n) {
    /* take from the right half only when it must come first */
    bool right_first = descending ? enlng_vals_lt(a[i], a[j])
                                  : enlng_vals_gt(a[i], a[j]);
    tmp[k++] = right_first ? a[j++] : a[i++];
  }
  while (i < mid)
    tmp[k++] = a[i++];
  while (j < n)
    tmp[k++] = a[j++];
  memcpy(a, tmp, sizeof(EnlngVal) * (size_t)n);
}

static inline void enlng_list_sort(EnlngVal v, bool descending) {
  if (v.type == ENLNG_VAL_LIST && v.as.l) {
    ENLNG_NOT_FROZEN(v);
    int n = v.as.l->count;
    if (n < 2)
      return;
    EnlngVal *tmp = (EnlngVal *)malloc(sizeof(EnlngVal) * (size_t)n);
    enlng_merge_sort(v.as.l->items, tmp, n, descending);
    free(tmp);
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
  if (v.type == ENLNG_VAL_FUNC) {
    snprintf(buf, sz, "<function %s>", v.as.fn->shown);
    return buf;
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
  case ENLNG_VAL_FUNC: return "a function";
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

/* A whole number result that does not fit in 64 bits is an error, never a
   silently wrapped value. */
static inline void enlng_int_overflow(const char *op, int64_t a, int64_t b) {
  char msg[300];
  snprintf(msg, sizeof(msg),
           "The result of %lld %s %lld is too big for a whole number (the "
           "largest is 9223372036854775807). Write one of the numbers as a "
           "decimal, such as %lld.0, if an approximate answer is enough.",
           (long long)a, op, (long long)b, (long long)a);
  enlng_throw(msg);
}

static inline EnlngVal enlng_val_add(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT) {
    int64_t r;
    if (__builtin_add_overflow(a.as.i, b.as.i, &r))
      enlng_int_overflow("plus", a.as.i, b.as.i);
    return enlng_make_int(r);
  }
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
    EnlngVal res = enlng_make_string(comb);
    free(comb);
    return res;
  }
  enlng_bad_operands("plus", a, b);
  return enlng_make_null();
}

static inline EnlngVal enlng_val_sub(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT) {
    int64_t r;
    if (__builtin_sub_overflow(a.as.i, b.as.i, &r))
      enlng_int_overflow("minus", a.as.i, b.as.i);
    return enlng_make_int(r);
  }
  if (!enlng_is_num(a) || !enlng_is_num(b)) {
    enlng_bad_operands("minus", a, b);
    return enlng_make_null();
  }
  return enlng_make_float(enlng_num_d(a) - enlng_num_d(b));
}

static inline EnlngVal enlng_val_mul(EnlngVal a, EnlngVal b) {
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT) {
    int64_t r;
    if (__builtin_mul_overflow(a.as.i, b.as.i, &r))
      enlng_int_overflow("times", a.as.i, b.as.i);
    return enlng_make_int(r);
  }
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
    EnlngVal res = enlng_make_string(buf);
    free(buf);
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
  if (a.type == ENLNG_VAL_INT && b.type == ENLNG_VAL_INT) {
    if (b.as.i == -1 && a.as.i == INT64_MIN)
      enlng_int_overflow("divided by", a.as.i, b.as.i);
    return enlng_make_int(a.as.i / b.as.i);
  }
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
  case ENLNG_VAL_FUNC:
    printf("<function %s>", v.as.fn->shown);
    break;
  }
}

/* Calling a value that holds a function: the errors both modes give. */
static inline void enlng_not_a_function(const char *name, EnlngVal v) {
  char msg[300];
  snprintf(msg, sizeof(msg), "'%s' is %s, not a function, so it cannot be called.",
           name, enlng_kind(v));
  enlng_throw(msg);
}

static inline void enlng_wrong_arity(const char *name, int need, int got) {
  char msg[300];
  snprintf(msg, sizeof(msg), "'%s' needs %d input%s but was given %d.", name, need,
           need == 1 ? "" : "s", got);
  enlng_throw(msg);
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
    if (v.as.i == INT64_MIN)
      enlng_int_overflow("times", v.as.i, -1);
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

/* --- to_text / to_number --- */
typedef struct {
  char *s;
  size_t n, cap;
} EnlngBuf;

static inline void enlng_buf_add(EnlngBuf *b, const char *t) {
  size_t k = strlen(t);
  if (b->n + k + 1 > b->cap) {
    b->cap = (b->n + k + 1) * 2;
    b->s = (char *)realloc(b->s, b->cap);
  }
  memcpy(b->s + b->n, t, k + 1);
  b->n += k;
}

/* The text `show` would print for v. */
static inline void enlng_buf_val(EnlngBuf *b, EnlngVal v) {
  char tmp[64];
  switch (v.type) {
  case ENLNG_VAL_INT:
    snprintf(tmp, sizeof(tmp), "%lld", (long long)v.as.i);
    enlng_buf_add(b, tmp);
    break;
  case ENLNG_VAL_FLOAT:
    snprintf(tmp, sizeof(tmp), "%.6g", v.as.f);
    enlng_buf_add(b, tmp);
    break;
  case ENLNG_VAL_STRING:
    enlng_buf_add(b, v.as.s ? v.as.s : "");
    break;
  case ENLNG_VAL_BOOL:
    enlng_buf_add(b, v.as.b ? "true" : "false");
    break;
  case ENLNG_VAL_LIST:
    enlng_buf_add(b, "[");
    for (int i = 0; v.as.l && i < v.as.l->count; i++) {
      if (i > 0)
        enlng_buf_add(b, ", ");
      enlng_buf_val(b, v.as.l->items[i]);
    }
    enlng_buf_add(b, "]");
    break;
  case ENLNG_VAL_MAP: {
    enlng_buf_add(b, "{");
    int shown = 0;
    for (int i = 0; v.as.m && i < v.as.m->capacity; i++)
      for (EnlngMapEntry *e = v.as.m->buckets[i]; e; e = e->next) {
        if (shown++ > 0)
          enlng_buf_add(b, ", ");
        enlng_buf_add(b, "\"");
        enlng_buf_add(b, e->key);
        enlng_buf_add(b, "\": ");
        enlng_buf_val(b, e->val);
      }
    enlng_buf_add(b, "}");
    break;
  }
  case ENLNG_VAL_FUNC:
    snprintf(tmp, sizeof(tmp), "<function %.40s>", v.as.fn->shown);
    enlng_buf_add(b, tmp);
    break;
  default:
    enlng_buf_add(b, "null");
  }
}

static inline EnlngVal enlng_builtin_to_text(EnlngVal v) {
  if (v.type == ENLNG_VAL_STRING)
    return v;
  EnlngBuf b = {NULL, 0, 0};
  enlng_buf_add(&b, "");
  enlng_buf_val(&b, v);
  EnlngVal r = enlng_make_string(b.s);
  free(b.s);
  return r;
}

/* "42" -> 42, " 3.5 " -> 3.5; text that is not a number is an error. */
static inline EnlngVal enlng_builtin_to_number(EnlngVal v) {
  if (v.type == ENLNG_VAL_INT || v.type == ENLNG_VAL_FLOAT)
    return v;
  if (v.type == ENLNG_VAL_STRING && v.as.s) {
    const char *p = v.as.s;
    while (*p == ' ' || *p == '\t')
      p++;
    char *end = NULL;
    if (*p) {
      long long iv = strtoll(p, &end, 10);
      const char *q = end;
      while (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n')
        q++;
      if (end != p && *q == '\0')
        return enlng_make_int((int64_t)iv);
      double dv = strtod(p, &end);
      q = end;
      while (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n')
        q++;
      if (end != p && *q == '\0')
        return enlng_make_float(dv);
    }
    char msg[200];
    snprintf(msg, sizeof(msg), "'%.80s' is not a number.", v.as.s);
    enlng_throw(msg);
    return enlng_make_null();
  }
  char msg[200];
  snprintf(msg, sizeof(msg), "'to_number' needs text or a number, but got %s.",
           enlng_kind(v));
  enlng_throw(msg);
  return enlng_make_null();
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
    size_t ba = enlng_u8_off(v.as.s, a);
    size_t bb = ba + enlng_u8_off(v.as.s + ba, b - a);
    char *buf = (char *)malloc(bb - ba + 1);
    memcpy(buf, v.as.s + ba, bb - ba);
    buf[bb - ba] = '\0';
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
    while (*s) {
      char ch[8];
      int k = enlng_u8_step(s);
      memcpy(ch, s, (size_t)k);
      ch[k] = '\0';
      enlng_list_push(res, enlng_make_string(ch));
      s += k;
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
    int64_t chars = 0;
    for (const char *q = container.as.s; q < p; q += enlng_u8_step(q))
      chars++;
    return enlng_make_int(chars);
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
    size_t l1 = (size_t)enlng_u8_count(arg1.as.s);
    size_t l2 = (size_t)enlng_u8_count(arg2.as.s);
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

  int64_t idx = (int64_t)enlng_u8_off(
      s, enlng_parse_position(pos, enlng_u8_count(s), true));

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

  int64_t idx = (int64_t)enlng_u8_off(
      s, enlng_parse_position(pos, enlng_u8_count(s), false));
  int ck = s[idx] ? enlng_u8_step(s + idx) : 1;
  if (idx < 0 || idx >= (int64_t)slen)
    return str;

  char *buf = (char *)malloc(slen);
  memcpy(buf, s, (size_t)idx);
  strcpy(buf + idx, s + idx + ck);

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
    size_t l1 = (size_t)enlng_u8_count(arg1.as.s);
    size_t l2 = (size_t)enlng_u8_count(arg2.as.s);
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

  int64_t idx = (int64_t)enlng_u8_off(
      s, enlng_parse_position(pos, enlng_u8_count(s), false));
  int ck = s[idx] ? enlng_u8_step(s + idx) : 1;

  char *buf = (char *)malloc(slen + rlen + 1);
  memcpy(buf, s, (size_t)idx);
  memcpy(buf + idx, rep, rlen);
  strcpy(buf + idx + rlen, s + idx + ck);

  EnlngVal res = enlng_make_string(buf);
  free(buf);
  return res;
}

static inline EnlngVal enlng_builtin_string_set_at(EnlngVal str, EnlngVal pos, EnlngVal new_char) {
  return enlng_builtin_string_replace(new_char, str, pos);
}

/* --- Inputs to built-ins: the errors when a value has the wrong kind --- */
static inline const char *enlng_need_text(const char *fn, EnlngVal v) {
  if (v.type == ENLNG_VAL_STRING && v.as.s)
    return v.as.s;
  char msg[200];
  snprintf(msg, sizeof(msg), "'%s' needs text, but got %s.", fn, enlng_kind(v));
  enlng_throw(msg);
  return "";
}

static inline EnlngList *enlng_need_list(const char *fn, EnlngVal v) {
  if (v.type == ENLNG_VAL_LIST && v.as.l)
    return v.as.l;
  char msg[200];
  snprintf(msg, sizeof(msg), "'%s' needs a list, but got %s.", fn, enlng_kind(v));
  enlng_throw(msg);
  return NULL;
}

static inline int64_t enlng_need_whole(const char *fn, EnlngVal v) {
  if (v.type == ENLNG_VAL_INT)
    return v.as.i;
  char msg[200];
  snprintf(msg, sizeof(msg), "'%s' needs a whole number, but got %s.", fn,
           enlng_kind(v));
  enlng_throw(msg);
  return 0;
}

/* --- The inputs a program was started with: enlng run app.enlng a b --- */
static int enlng_prog_argc = 0;
static char **enlng_prog_argv = NULL;

static inline void enlng_set_args(int argc, char **argv) {
  enlng_prog_argc = argc;
  enlng_prog_argv = argv;
}

static inline EnlngVal enlng_builtin_arguments(void) {
  EnlngList *l = enlng_list_create();
  for (int i = 0; i < enlng_prog_argc; i++)
    enlng_list_push(l, enlng_make_string(enlng_prog_argv[i]));
  return enlng_make_list(l);
}

static inline EnlngVal enlng_builtin_env(EnlngVal name) {
  const char *v = getenv(enlng_need_text("env", name));
  return v ? enlng_make_string(v) : enlng_make_null();
}

static inline EnlngVal enlng_builtin_type_of(EnlngVal v) {
  switch (v.type) {
  case ENLNG_VAL_INT: return enlng_make_string("number");
  case ENLNG_VAL_FLOAT: return enlng_make_string("decimal");
  case ENLNG_VAL_STRING: return enlng_make_string("text");
  case ENLNG_VAL_BOOL: return enlng_make_string("true/false");
  case ENLNG_VAL_LIST: return enlng_make_string("list");
  case ENLNG_VAL_MAP: return enlng_make_string("map");
  case ENLNG_VAL_FUNC: return enlng_make_string("function");
  default: return enlng_make_string("null");
  }
}

/* --- Characters and their numbers (Unicode code points) --- */
static inline int64_t enlng_u8_decode(const unsigned char *s, int *len) {
  int n = enlng_u8_clen(s[0]);
  int64_t cp = n == 1 ? s[0] : n == 2 ? (s[0] & 0x1F) : n == 3 ? (s[0] & 0x0F) : (s[0] & 0x07);
  for (int i = 1; i < n; i++) {
    if ((s[i] & 0xC0) != 0x80) { /* broken UTF-8: take the byte alone */
      n = 1;
      cp = s[0];
      break;
    }
    cp = (cp << 6) | (s[i] & 0x3F);
  }
  *len = n;
  return cp;
}

static inline int enlng_u8_encode(int64_t cp, char *out) {
  if (cp < 0x80) { out[0] = (char)cp; return 1; }
  if (cp < 0x800) {
    out[0] = (char)(0xC0 | (cp >> 6));
    out[1] = (char)(0x80 | (cp & 0x3F));
    return 2;
  }
  if (cp < 0x10000) {
    out[0] = (char)(0xE0 | (cp >> 12));
    out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[2] = (char)(0x80 | (cp & 0x3F));
    return 3;
  }
  out[0] = (char)(0xF0 | (cp >> 18));
  out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
  out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
  out[3] = (char)(0x80 | (cp & 0x3F));
  return 4;
}

static inline EnlngVal enlng_builtin_char_code(EnlngVal v) {
  const char *s = enlng_need_text("char_code", v);
  if (!*s)
    enlng_throw("'char_code' needs text with at least one character.");
  int n;
  return enlng_make_int(enlng_u8_decode((const unsigned char *)s, &n));
}

static inline EnlngVal enlng_builtin_char_of(EnlngVal v) {
  int64_t cp = enlng_need_whole("char_of", v);
  if (cp < 1 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
    char msg[200];
    snprintf(msg, sizeof(msg), "%lld is not the number of a character.", (long long)cp);
    enlng_throw(msg);
  }
  char buf[5];
  int n = enlng_u8_encode(cp, buf);
  buf[n] = '\0';
  return enlng_make_string(buf);
}

/* --- Lists and maps --- */

/* A new list with each value once, in the order first seen. */
static inline EnlngVal enlng_builtin_unique(EnlngVal v) {
  EnlngList *in = enlng_need_list("unique", v);
  EnlngList *out = enlng_list_create();
  EnlngVal seen = enlng_make_map(enlng_map_create());
  for (int i = 0; i < in->count; i++) {
    EnlngVal x = in->items[i];
    char key[96];
    bool keyed = true;
    if (x.type == ENLNG_VAL_INT)
      snprintf(key, sizeof(key), "i%lld", (long long)x.as.i);
    else if (x.type == ENLNG_VAL_FLOAT && x.as.f == floor(x.as.f) &&
             fabs(x.as.f) < 9e18)
      snprintf(key, sizeof(key), "i%lld", (long long)x.as.f);
    else if (x.type == ENLNG_VAL_FLOAT)
      snprintf(key, sizeof(key), "f%.17g", x.as.f);
    else if (x.type == ENLNG_VAL_BOOL)
      snprintf(key, sizeof(key), "b%d", x.as.b ? 1 : 0);
    else if (x.type == ENLNG_VAL_NULL)
      snprintf(key, sizeof(key), "n");
    else
      keyed = false;
    if (x.type == ENLNG_VAL_STRING) {
      size_t n = strlen(x.as.s ? x.as.s : "");
      char *k = (char *)malloc(n + 2);
      k[0] = 's';
      memcpy(k + 1, x.as.s ? x.as.s : "", n + 1);
      bool dup = enlng_map_has(seen, k);
      if (!dup) {
        enlng_map_set(seen, k, enlng_make_bool(true));
        enlng_list_push(out, x);
      }
      free(k);
      continue;
    }
    if (keyed) {
      if (!enlng_map_has(seen, key)) {
        enlng_map_set(seen, key, enlng_make_bool(true));
        enlng_list_push(out, x);
      }
      continue;
    }
    bool dup = false;
    for (int j = 0; j < out->count && !dup; j++)
      dup = enlng_vals_equal(out->items[j], x);
    if (!dup)
      enlng_list_push(out, x);
  }
  return enlng_make_list(out);
}

static inline EnlngVal enlng_builtin_remove_key(EnlngVal map, EnlngVal key) {
  if (map.type != ENLNG_VAL_MAP || !map.as.m) {
    char msg[200];
    snprintf(msg, sizeof(msg), "'remove_key' needs a map, but got %s.", enlng_kind(map));
    enlng_throw(msg);
  }
  const char *k = enlng_need_text("remove_key", key);
  ENLNG_NOT_FROZEN(map);
  EnlngMap *m = map.as.m;
  unsigned long h = enlng_hash_str(k) % m->capacity;
  EnlngMapEntry **link = &m->buckets[h];
  while (*link) {
    if (strcmp((*link)->key, k) == 0) {
      EnlngMapEntry *e = *link;
      *link = e->next;
      free(e->key);
      free(e);
      m->count--;
      return enlng_make_bool(true);
    }
    link = &(*link)->next;
  }
  return enlng_make_bool(false);
}

/* A position in a list of `count` items; -1 is the last one. */
static inline int enlng_list_pos(const char *fn, EnlngVal pos, int count, bool end_ok) {
  int64_t i = enlng_need_whole(fn, pos);
  if (i < 0)
    i += count;
  if (i < 0 || i > count || (i == count && !end_ok)) {
    char msg[200];
    snprintf(msg, sizeof(msg), "Position %lld is outside the list, which has %d item%s.",
             (long long)enlng_need_whole(fn, pos), count, count == 1 ? "" : "s");
    enlng_throw(msg);
  }
  return (int)i;
}

static inline EnlngVal enlng_builtin_insert_at(EnlngVal list, EnlngVal pos, EnlngVal item) {
  EnlngList *l = enlng_need_list("insert_at", list);
  ENLNG_NOT_FROZEN(list);
  int at = enlng_list_pos("insert_at", pos, l->count, true);
  enlng_list_push(l, item);
  memmove(&l->items[at + 1], &l->items[at], sizeof(EnlngVal) * (size_t)(l->count - 1 - at));
  l->items[at] = item;
  return list;
}

static inline EnlngVal enlng_builtin_remove_at(EnlngVal list, EnlngVal pos) {
  EnlngList *l = enlng_need_list("remove_at", list);
  ENLNG_NOT_FROZEN(list);
  int at = enlng_list_pos("remove_at", pos, l->count, false);
  EnlngVal gone = l->items[at];
  memmove(&l->items[at], &l->items[at + 1], sizeof(EnlngVal) * (size_t)(l->count - 1 - at));
  l->count--;
  return gone;
}

/* --- Time in milliseconds, for measuring how long something takes --- */
static inline EnlngVal enlng_builtin_time_ms(void) {
#ifdef _WIN32
  LARGE_INTEGER f, c;
  QueryPerformanceFrequency(&f);
  QueryPerformanceCounter(&c);
  return enlng_make_int((int64_t)(c.QuadPart / (f.QuadPart / 1000)));
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return enlng_make_int((int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

/* --- check(condition, message): stop with the message when it is false --- */
static inline EnlngVal enlng_builtin_check(EnlngVal cond, EnlngVal message) {
  if (!enlng_is_truthy(cond)) {
    char buf[64], msg[400];
    const char *m = message.type == ENLNG_VAL_STRING && message.as.s
                        ? message.as.s
                        : enlng_val_to_str_buf(message, buf, sizeof(buf));
    snprintf(msg, sizeof(msg), "Check failed: %s", m);
    enlng_throw(msg);
  }
  return enlng_make_bool(true);
}

/* --- JSON: to_json(value), to_json_pretty(value), from_json(text) --- */
static inline void enlng_json_str(EnlngBuf *b, const char *s) {
  enlng_buf_add(b, "\"");
  char tmp[8];
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    switch (*p) {
    case '"': enlng_buf_add(b, "\\\""); break;
    case '\\': enlng_buf_add(b, "\\\\"); break;
    case '\n': enlng_buf_add(b, "\\n"); break;
    case '\r': enlng_buf_add(b, "\\r"); break;
    case '\t': enlng_buf_add(b, "\\t"); break;
    case '\b': enlng_buf_add(b, "\\b"); break;
    case '\f': enlng_buf_add(b, "\\f"); break;
    default:
      if (*p < 0x20) {
        snprintf(tmp, sizeof(tmp), "\\u%04x", *p);
        enlng_buf_add(b, tmp);
      } else {
        tmp[0] = (char)*p;
        tmp[1] = '\0';
        enlng_buf_add(b, tmp);
      }
    }
  }
  enlng_buf_add(b, "\"");
}

static inline void enlng_json_indent(EnlngBuf *b, int pretty, int depth) {
  if (!pretty)
    return;
  enlng_buf_add(b, "\n");
  for (int i = 0; i < depth; i++)
    enlng_buf_add(b, "  ");
}

static inline void enlng_json_val(EnlngBuf *b, EnlngVal v, int pretty, int depth) {
  char tmp[64];
  if (depth > 1000)
    enlng_throw("This value is nested too deeply to turn into JSON.");
  switch (v.type) {
  case ENLNG_VAL_INT:
    snprintf(tmp, sizeof(tmp), "%lld", (long long)v.as.i);
    enlng_buf_add(b, tmp);
    break;
  case ENLNG_VAL_FLOAT:
    if (v.as.f != v.as.f || v.as.f == HUGE_VAL || v.as.f == -HUGE_VAL)
      enlng_throw("JSON has no way to write an infinite or undefined number.");
    snprintf(tmp, sizeof(tmp), "%.15g", v.as.f);
    if (strtod(tmp, NULL) != v.as.f)
      snprintf(tmp, sizeof(tmp), "%.17g", v.as.f);
    if (!strpbrk(tmp, ".eEn"))
      strcat(tmp, ".0");
    enlng_buf_add(b, tmp);
    break;
  case ENLNG_VAL_STRING:
    enlng_json_str(b, v.as.s ? v.as.s : "");
    break;
  case ENLNG_VAL_BOOL:
    enlng_buf_add(b, v.as.b ? "true" : "false");
    break;
  case ENLNG_VAL_LIST:
    enlng_buf_add(b, "[");
    for (int i = 0; v.as.l && i < v.as.l->count; i++) {
      if (i > 0)
        enlng_buf_add(b, ",");
      enlng_json_indent(b, pretty, depth + 1);
      enlng_json_val(b, v.as.l->items[i], pretty, depth + 1);
    }
    if (v.as.l && v.as.l->count > 0)
      enlng_json_indent(b, pretty, depth);
    enlng_buf_add(b, "]");
    break;
  case ENLNG_VAL_MAP: {
    enlng_buf_add(b, "{");
    int n = 0;
    for (int i = 0; v.as.m && i < v.as.m->capacity; i++)
      for (EnlngMapEntry *e = v.as.m->buckets[i]; e; e = e->next) {
        if (n++ > 0)
          enlng_buf_add(b, ",");
        enlng_json_indent(b, pretty, depth + 1);
        enlng_json_str(b, e->key);
        enlng_buf_add(b, pretty ? ": " : ":");
        enlng_json_val(b, e->val, pretty, depth + 1);
      }
    if (n > 0)
      enlng_json_indent(b, pretty, depth);
    enlng_buf_add(b, "}");
    break;
  }
  case ENLNG_VAL_FUNC:
    enlng_throw("A function cannot be turned into JSON.");
    break;
  default:
    enlng_buf_add(b, "null");
  }
}

static inline EnlngVal enlng_json_text(EnlngVal v, int pretty) {
  EnlngBuf b = {NULL, 0, 0};
  enlng_buf_add(&b, "");
  enlng_json_val(&b, v, pretty, 0);
  EnlngVal r = enlng_make_string(b.s);
  free(b.s);
  return r;
}

static inline EnlngVal enlng_builtin_to_json(EnlngVal v) { return enlng_json_text(v, 0); }
static inline EnlngVal enlng_builtin_to_json_pretty(EnlngVal v) { return enlng_json_text(v, 1); }

typedef struct {
  const char *s;
  size_t i;
} EnlngJsonIn;

static inline void enlng_json_fail(EnlngJsonIn *in, const char *why) {
  char msg[300];
  snprintf(msg, sizeof(msg), "This is not valid JSON: %s at character %lu.", why,
           (unsigned long)in->i + 1);
  enlng_throw(msg);
}

static inline void enlng_json_ws(EnlngJsonIn *in) {
  while (in->s[in->i] == ' ' || in->s[in->i] == '\t' || in->s[in->i] == '\n' ||
         in->s[in->i] == '\r')
    in->i++;
}

static inline int enlng_json_hex4(EnlngJsonIn *in) {
  int v = 0;
  for (int k = 0; k < 4; k++) {
    char c = in->s[in->i++];
    v <<= 4;
    if (c >= '0' && c <= '9') v |= c - '0';
    else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
    else enlng_json_fail(in, "a \\u escape needs four hex digits");
  }
  return v;
}

/* The text of a JSON string; the caller frees it. */
static inline char *enlng_json_strval(EnlngJsonIn *in) {
  EnlngBuf b = {NULL, 0, 0};
  enlng_buf_add(&b, "");
  in->i++; /* the opening quote */
  char tmp[8];
  for (;;) {
    unsigned char c = (unsigned char)in->s[in->i];
    if (c == '\0') {
      free(b.s);
      enlng_json_fail(in, "a text has no closing quote");
    }
    if (c == '"') {
      in->i++;
      break;
    }
    if (c == '\\') {
      in->i++;
      char e = in->s[in->i++];
      switch (e) {
      case '"': enlng_buf_add(&b, "\""); break;
      case '\\': enlng_buf_add(&b, "\\"); break;
      case '/': enlng_buf_add(&b, "/"); break;
      case 'n': enlng_buf_add(&b, "\n"); break;
      case 'r': enlng_buf_add(&b, "\r"); break;
      case 't': enlng_buf_add(&b, "\t"); break;
      case 'b': enlng_buf_add(&b, "\b"); break;
      case 'f': enlng_buf_add(&b, "\f"); break;
      case 'u': {
        int64_t cp = enlng_json_hex4(in);
        if (cp >= 0xD800 && cp <= 0xDBFF && in->s[in->i] == '\\' && in->s[in->i + 1] == 'u') {
          in->i += 2;
          int lo = enlng_json_hex4(in);
          cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
        }
        int n = enlng_u8_encode(cp ? cp : 0xFFFD, tmp);
        tmp[n] = '\0';
        enlng_buf_add(&b, tmp);
        break;
      }
      default:
        free(b.s);
        enlng_json_fail(in, "unknown escape after a backslash");
      }
      continue;
    }
    tmp[0] = (char)c;
    tmp[1] = '\0';
    enlng_buf_add(&b, tmp);
    in->i++;
  }
  return b.s;
}

static inline EnlngVal enlng_json_parse(EnlngJsonIn *in, int depth) {
  if (depth > 1000)
    enlng_json_fail(in, "it is nested too deeply");
  enlng_json_ws(in);
  char c = in->s[in->i];
  if (c == '{') {
    in->i++;
    EnlngVal m = enlng_make_map(enlng_map_create());
    enlng_json_ws(in);
    if (in->s[in->i] == '}') {
      in->i++;
      return m;
    }
    for (;;) {
      enlng_json_ws(in);
      if (in->s[in->i] != '"')
        enlng_json_fail(in, "a key in quotes was expected");
      char *key = enlng_json_strval(in);
      enlng_json_ws(in);
      if (in->s[in->i] != ':') {
        free(key);
        enlng_json_fail(in, "':' was expected after a key");
      }
      in->i++;
      EnlngVal v = enlng_json_parse(in, depth + 1);
      enlng_map_set(m, key, v);
      free(key);
      enlng_json_ws(in);
      if (in->s[in->i] == ',') { in->i++; continue; }
      if (in->s[in->i] == '}') { in->i++; return m; }
      enlng_json_fail(in, "',' or '}' was expected");
    }
  }
  if (c == '[') {
    in->i++;
    EnlngList *l = enlng_list_create();
    enlng_json_ws(in);
    if (in->s[in->i] == ']') {
      in->i++;
      return enlng_make_list(l);
    }
    for (;;) {
      enlng_list_push(l, enlng_json_parse(in, depth + 1));
      enlng_json_ws(in);
      if (in->s[in->i] == ',') { in->i++; continue; }
      if (in->s[in->i] == ']') { in->i++; return enlng_make_list(l); }
      enlng_json_fail(in, "',' or ']' was expected");
    }
  }
  if (c == '"') {
    char *s = enlng_json_strval(in);
    EnlngVal r = enlng_make_string(s);
    free(s);
    return r;
  }
  if (strncmp(in->s + in->i, "true", 4) == 0) { in->i += 4; return enlng_make_bool(true); }
  if (strncmp(in->s + in->i, "false", 5) == 0) { in->i += 5; return enlng_make_bool(false); }
  if (strncmp(in->s + in->i, "null", 4) == 0) { in->i += 4; return enlng_make_null(); }
  if (c == '-' || (c >= '0' && c <= '9')) {
    const char *start = in->s + in->i;
    char *end;
    double d = strtod(start, &end);
    bool whole = true;
    for (const char *q = start; q < end; q++)
      if (*q == '.' || *q == 'e' || *q == 'E')
        whole = false;
    in->i += (size_t)(end - start);
    if (whole) {
      errno = 0;
      long long iv = strtoll(start, NULL, 10);
      if (errno == 0)
        return enlng_make_int((int64_t)iv);
    }
    return enlng_make_float(d);
  }
  enlng_json_fail(in, c ? "a value was expected" : "the text ended too soon");
  return enlng_make_null();
}

static inline EnlngVal enlng_builtin_from_json(EnlngVal text) {
  EnlngJsonIn in = {enlng_need_text("from_json", text), 0};
  EnlngVal v = enlng_json_parse(&in, 0);
  enlng_json_ws(&in);
  if (in.s[in.i])
    enlng_json_fail(&in, "there is more after the value");
  return v;
}

/* --- Patterns (regular expressions) ---------------------------------------
   A small backtracking engine: . [abc] [^a-z] \d \w \s \D \W \S \b ^ $
   ( ) (?: ) | * + ? {n} {n,} {n,m}, lazy *? +? ??, and (?i) at the start
   for ignoring upper and lower case. It works on characters, not bytes. */
enum { RX_CHAR, RX_ANY, RX_CLASS, RX_BOL, RX_EOL, RX_WORDB, RX_NWORDB, RX_GROUP };

typedef struct RxNode RxNode;
typedef struct {
  RxNode **items;
  int n;
} RxSeq;

struct RxNode {
  int kind;
  int64_t cp;
  int64_t (*ranges)[2];
  int nranges;
  bool neg;
  RxSeq *alts;
  int nalts;
  int group; /* -1: (?: ) */
  int min, max; /* max -1: no limit */
  bool lazy;
};

typedef struct {
  const char *pat;
  size_t i;
  void **mem;
  int nmem, capmem;
  int ngroups;
  bool icase;
  const char *text;
  size_t len;
  long gs[10], ge[10]; /* group start and end, -1 when unset */
  int depth;
} Rx;

static inline void *rx_alloc(Rx *r, size_t n) {
  void *p = calloc(1, n);
  if (r->nmem >= r->capmem) {
    r->capmem = r->capmem ? r->capmem * 2 : 32;
    r->mem = (void **)realloc(r->mem, sizeof(void *) * r->capmem);
  }
  r->mem[r->nmem++] = p;
  return p;
}

static inline void rx_free(Rx *r) {
  for (int k = 0; k < r->nmem; k++)
    free(r->mem[k]);
  free(r->mem);
  r->mem = NULL;
  r->nmem = r->capmem = 0;
}

static inline void rx_fail(Rx *r, const char *why) {
  char msg[400];
  snprintf(msg, sizeof(msg), "The pattern \"%.120s\" is not valid: %s.", r->pat, why);
  rx_free(r);
  enlng_throw(msg);
}

static inline void rx_add_range(Rx *r, RxNode *nd, int64_t a, int64_t b) {
  int64_t (*nr)[2] = (int64_t(*)[2])rx_alloc(r, sizeof(int64_t) * 2 * (nd->nranges + 1));
  if (nd->nranges)
    memcpy(nr, nd->ranges, sizeof(int64_t) * 2 * nd->nranges);
  nr[nd->nranges][0] = a;
  nr[nd->nranges][1] = b;
  nd->ranges = nr;
  nd->nranges++;
}

/* \d \w \s as ranges; the capital letters are the opposite set */
static inline void rx_add_shorthand(Rx *r, RxNode *nd, char c) {
  switch (c) {
  case 'd': rx_add_range(r, nd, '0', '9'); break;
  case 'w':
    rx_add_range(r, nd, '0', '9'); rx_add_range(r, nd, 'a', 'z');
    rx_add_range(r, nd, 'A', 'Z'); rx_add_range(r, nd, '_', '_');
    break;
  case 's':
    rx_add_range(r, nd, ' ', ' '); rx_add_range(r, nd, '\t', '\r');
    break;
  }
}

static inline int64_t rx_next_cp(Rx *r) {
  int n;
  int64_t cp = enlng_u8_decode((const unsigned char *)r->pat + r->i, &n);
  r->i += (size_t)n;
  return cp;
}

static inline int64_t rx_escape_cp(char c) {
  switch (c) {
  case 'n': return '\n';
  case 't': return '\t';
  case 'r': return '\r';
  case 'f': return '\f';
  case 'v': return '\v';
  case '0': return 0;
  default: return (unsigned char)c;
  }
}

static RxSeq *rx_parse_alt(Rx *r, RxNode *owner);

static inline RxNode *rx_parse_atom(Rx *r) {
  char c = r->pat[r->i];
  RxNode *nd = (RxNode *)rx_alloc(r, sizeof(RxNode));
  nd->min = nd->max = 1;
  if (c == '(') {
    r->i++;
    nd->kind = RX_GROUP;
    if (r->pat[r->i] == '?' && r->pat[r->i + 1] == ':') {
      r->i += 2;
      nd->group = -1;
    } else {
      nd->group = ++r->ngroups;
    }
    rx_parse_alt(r, nd);
    if (r->pat[r->i] != ')')
      rx_fail(r, "a '(' has no matching ')'");
    r->i++;
    return nd;
  }
  if (c == '[') {
    r->i++;
    nd->kind = RX_CLASS;
    if (r->pat[r->i] == '^') {
      nd->neg = true;
      r->i++;
    }
    bool first = true;
    while (r->pat[r->i] && (r->pat[r->i] != ']' || first)) {
      first = false;
      int64_t a;
      if (r->pat[r->i] == '\\') {
        char e = r->pat[r->i + 1];
        if (!e)
          rx_fail(r, "it ends with a backslash");
        r->i += 2;
        if (e == 'd' || e == 'w' || e == 's') {
          rx_add_shorthand(r, nd, e);
          continue;
        }
        a = rx_escape_cp(e);
      } else {
        a = rx_next_cp(r);
      }
      int64_t b = a;
      if (r->pat[r->i] == '-' && r->pat[r->i + 1] && r->pat[r->i + 1] != ']') {
        r->i++;
        if (r->pat[r->i] == '\\') {
          b = rx_escape_cp(r->pat[r->i + 1]);
          r->i += 2;
        } else {
          b = rx_next_cp(r);
        }
        if (b < a)
          rx_fail(r, "a range in [ ] goes backwards");
      }
      rx_add_range(r, nd, a, b);
    }
    if (r->pat[r->i] != ']')
      rx_fail(r, "a '[' has no matching ']'");
    r->i++;
    return nd;
  }
  if (c == '.') { r->i++; nd->kind = RX_ANY; return nd; }
  if (c == '^') { r->i++; nd->kind = RX_BOL; return nd; }
  if (c == '$') { r->i++; nd->kind = RX_EOL; return nd; }
  if (c == '\\') {
    char e = r->pat[r->i + 1];
    if (!e)
      rx_fail(r, "it ends with a backslash");
    r->i += 2;
    if (e == 'b') { nd->kind = RX_WORDB; return nd; }
    if (e == 'B') { nd->kind = RX_NWORDB; return nd; }
    if (e == 'd' || e == 'w' || e == 's' || e == 'D' || e == 'W' || e == 'S') {
      nd->kind = RX_CLASS;
      nd->neg = (e == 'D' || e == 'W' || e == 'S');
      rx_add_shorthand(r, nd, (char)tolower((unsigned char)e));
      return nd;
    }
    nd->kind = RX_CHAR;
    nd->cp = rx_escape_cp(e);
    return nd;
  }
  if (c == '*' || c == '+' || c == '?' || c == '{')
    rx_fail(r, "a repeat sign such as * + ? has nothing before it");
  nd->kind = RX_CHAR;
  nd->cp = rx_next_cp(r);
  return nd;
}

static inline int rx_number(Rx *r) {
  int v = 0;
  if (!isdigit((unsigned char)r->pat[r->i]))
    return -1;
  while (isdigit((unsigned char)r->pat[r->i]))
    v = v * 10 + (r->pat[r->i++] - '0');
  return v;
}

static inline RxSeq *rx_parse_seq(Rx *r) {
  RxSeq *seq = (RxSeq *)rx_alloc(r, sizeof(RxSeq));
  int cap = 0;
  while (r->pat[r->i] && r->pat[r->i] != '|' && r->pat[r->i] != ')') {
    RxNode *nd = rx_parse_atom(r);
    char q = r->pat[r->i];
    if (q == '*' || q == '+' || q == '?') {
      r->i++;
      nd->min = q == '+' ? 1 : 0;
      nd->max = q == '?' ? 1 : -1;
    } else if (q == '{' && isdigit((unsigned char)r->pat[r->i + 1])) {
      r->i++;
      nd->min = rx_number(r);
      nd->max = nd->min;
      if (r->pat[r->i] == ',') {
        r->i++;
        nd->max = rx_number(r);
      }
      if (r->pat[r->i] != '}')
        rx_fail(r, "a '{' repeat has no matching '}'");
      r->i++;
      if (nd->max >= 0 && nd->max < nd->min)
        rx_fail(r, "in {n,m} the second number is smaller than the first");
    }
    if ((nd->min != 1 || nd->max != 1) && r->pat[r->i] == '?') {
      r->i++;
      nd->lazy = true;
    }
    if (nd->kind == RX_BOL || nd->kind == RX_EOL || nd->kind == RX_WORDB || nd->kind == RX_NWORDB)
      if (nd->max != 1 && nd->max != 0)
        nd->max = 1;
    if (seq->n >= cap) {
      cap = cap ? cap * 2 : 8;
      RxNode **ni = (RxNode **)rx_alloc(r, sizeof(RxNode *) * cap);
      if (seq->n)
        memcpy(ni, seq->items, sizeof(RxNode *) * seq->n);
      seq->items = ni;
    }
    seq->items[seq->n++] = nd;
  }
  return seq;
}

static RxSeq *rx_parse_alt(Rx *r, RxNode *owner) {
  int cap = 4;
  owner->alts = (RxSeq *)rx_alloc(r, sizeof(RxSeq) * cap);
  owner->nalts = 0;
  for (;;) {
    RxSeq *s = rx_parse_seq(r);
    if (owner->nalts >= cap) {
      cap *= 2;
      RxSeq *na = (RxSeq *)rx_alloc(r, sizeof(RxSeq) * cap);
      memcpy(na, owner->alts, sizeof(RxSeq) * owner->nalts);
      owner->alts = na;
    }
    owner->alts[owner->nalts++] = *s;
    if (r->pat[r->i] != '|')
      break;
    r->i++;
  }
  return owner->alts;
}

/* What is left to match after the current item. */
enum { RXK_SEQ, RXK_REP, RXK_CLOSE };
typedef struct RxK {
  int kind;
  RxSeq *seq;
  int idx;
  RxNode *node;
  int count;
  long start;
  struct RxK *next;
} RxK;

static inline bool rx_word_at(Rx *r, long pos) {
  if (pos < 0 || (size_t)pos >= r->len)
    return false;
  unsigned char c = (unsigned char)r->text[pos];
  return isalnum(c) || c == '_';
}

static inline int64_t rx_fold(Rx *r, int64_t cp) {
  return r->icase && cp < 128 ? tolower((int)cp) : cp;
}

/* Does one character node match at pos? Gives its length in bytes, or 0. */
static inline int rx_char_len(Rx *r, RxNode *nd, long pos) {
  if ((size_t)pos >= r->len)
    return 0;
  int n;
  int64_t cp = enlng_u8_decode((const unsigned char *)r->text + pos, &n);
  if (nd->kind == RX_ANY)
    return cp == '\n' ? 0 : n;
  if (nd->kind == RX_CHAR)
    return rx_fold(r, cp) == rx_fold(r, nd->cp) ? n : 0;
  bool in = false;
  int64_t f = rx_fold(r, cp);
  for (int k = 0; k < nd->nranges && !in; k++) {
    in = cp >= nd->ranges[k][0] && cp <= nd->ranges[k][1];
    if (!in && r->icase && f != cp)
      in = f >= nd->ranges[k][0] && f <= nd->ranges[k][1];
    if (!in && r->icase && cp < 128 && isalpha((int)cp)) {
      int64_t u = toupper((int)cp);
      in = u >= nd->ranges[k][0] && u <= nd->ranges[k][1];
    }
  }
  return in != nd->neg ? n : 0;
}

static bool rx_cont(Rx *r, RxK *k, long pos);
static bool rx_rep(Rx *r, RxNode *nd, int count, long pos, RxK *rest);

static bool rx_one(Rx *r, RxNode *nd, long pos, RxK *k) {
  switch (nd->kind) {
  case RX_CHAR:
  case RX_ANY:
  case RX_CLASS: {
    int n = rx_char_len(r, nd, pos);
    return n > 0 && rx_cont(r, k, pos + n);
  }
  case RX_BOL: return pos == 0 && rx_cont(r, k, pos);
  case RX_EOL: return (size_t)pos == r->len && rx_cont(r, k, pos);
  case RX_WORDB:
  case RX_NWORDB: {
    bool edge = rx_word_at(r, pos - 1) != rx_word_at(r, pos);
    return edge == (nd->kind == RX_WORDB) && rx_cont(r, k, pos);
  }
  case RX_GROUP: {
    long old_s = 0, old_e = 0;
    int g = nd->group;
    if (g > 0 && g < 10) {
      old_s = r->gs[g];
      old_e = r->ge[g];
      r->gs[g] = pos;
    }
    RxK close = {RXK_CLOSE, NULL, 0, nd, 0, pos, k};
    for (int a = 0; a < nd->nalts; a++) {
      RxK s = {RXK_SEQ, &nd->alts[a], 0, NULL, 0, 0, &close};
      if (rx_cont(r, &s, pos))
        return true;
    }
    if (g > 0 && g < 10) {
      r->gs[g] = old_s;
      r->ge[g] = old_e;
    }
    return false;
  }
  }
  return false;
}

static bool rx_rep(Rx *r, RxNode *nd, int count, long pos, RxK *rest) {
  if (++r->depth > 20000) {
    rx_free(r);
    enlng_throw("The pattern needs too many steps on this text. Try a simpler pattern.");
  }
  bool ok = false;
  bool simple = nd->kind == RX_CHAR || nd->kind == RX_ANY || nd->kind == RX_CLASS;
  if (simple && !nd->lazy && (nd->max < 0 || nd->max > 1)) {
    /* a run of single characters: take as many as allowed, then give back
       one at a time, without a step per character */
    long p = pos;
    int got = count;
    while ((nd->max < 0 || got < nd->max)) {
      int n = rx_char_len(r, nd, p);
      if (n == 0)
        break;
      p += n;
      got++;
    }
    while (got >= nd->min) {
      if (rx_cont(r, rest, p)) {
        ok = true;
        break;
      }
      if (got == count)
        break;
      do
        p--;
      while (p > pos && ((unsigned char)r->text[p] & 0xC0) == 0x80);
      got--;
    }
  } else {
    bool can_more = nd->max < 0 || count < nd->max;
    bool can_stop = count >= nd->min;
    RxK again = {RXK_REP, NULL, 0, nd, count + 1, pos, rest};
    if (nd->lazy) {
      ok = (can_stop && rx_cont(r, rest, pos)) || (can_more && rx_one(r, nd, pos, &again));
    } else {
      ok = (can_more && rx_one(r, nd, pos, &again)) || (can_stop && rx_cont(r, rest, pos));
    }
  }
  r->depth--;
  return ok;
}

static bool rx_cont(Rx *r, RxK *k, long pos) {
  if (!k) {
    r->ge[0] = pos;
    return true;
  }
  if (k->kind == RXK_SEQ) {
    if (k->idx == k->seq->n)
      return rx_cont(r, k->next, pos);
    RxK rest = {RXK_SEQ, k->seq, k->idx + 1, NULL, 0, 0, k->next};
    return rx_rep(r, k->seq->items[k->idx], 0, pos, &rest);
  }
  if (k->kind == RXK_REP) {
    if (pos == k->start && k->count > k->node->min)
      return false; /* a repeat that matched nothing would loop forever */
    return rx_rep(r, k->node, k->count, pos, k->next);
  }
  /* RXK_CLOSE: the group ends here */
  int g = k->node->group;
  long old = 0;
  if (g > 0 && g < 10) {
    old = r->ge[g];
    r->ge[g] = pos;
  }
  if (rx_cont(r, k->next, pos))
    return true;
  if (g > 0 && g < 10)
    r->ge[g] = old;
  return false;
}

static inline RxNode *rx_compile(Rx *r, const char *pat) {
  memset(r, 0, sizeof(*r));
  r->pat = pat;
  if (strncmp(pat, "(?i)", 4) == 0) {
    r->icase = true;
    r->i = 4;
  }
  RxNode *top = (RxNode *)rx_alloc(r, sizeof(RxNode));
  top->kind = RX_GROUP;
  top->group = -1;
  top->min = top->max = 1;
  rx_parse_alt(r, top);
  if (r->pat[r->i] == ')')
    rx_fail(r, "a ')' has no matching '('");
  return top;
}

/* Finds the first match at or after `from`; group 0 is the whole match. */
static inline bool rx_search(Rx *r, RxNode *top, const char *text, long from) {
  r->text = text;
  r->len = strlen(text);
  for (long pos = from; (size_t)pos <= r->len;) {
    for (int g = 0; g < 10; g++)
      r->gs[g] = r->ge[g] = -1;
    r->depth = 0;
    r->gs[0] = pos;
    RxK s = {RXK_SEQ, &top->alts[0], 0, NULL, 0, 0, NULL};
    bool hit = false;
    for (int a = 0; a < top->nalts && !hit; a++) {
      s.seq = &top->alts[a];
      hit = rx_cont(r, &s, pos);
    }
    if (hit)
      return true;
    if ((size_t)pos == r->len)
      break;
    pos += enlng_u8_clen((unsigned char)text[pos]);
  }
  return false;
}

static inline EnlngVal rx_slice(const char *text, long a, long b) {
  if (a < 0 || b < a)
    return enlng_make_null();
  char *t = (char *)malloc((size_t)(b - a) + 1);
  memcpy(t, text + a, (size_t)(b - a));
  t[b - a] = '\0';
  EnlngVal v = enlng_make_string(t);
  free(t);
  return v;
}

static inline EnlngVal enlng_builtin_regex_match(EnlngVal pat, EnlngVal text) {
  const char *p = enlng_need_text("regex_match", pat);
  const char *t = enlng_need_text("regex_match", text);
  Rx r;
  RxNode *top = rx_compile(&r, p);
  bool hit = rx_search(&r, top, t, 0);
  rx_free(&r);
  return enlng_make_bool(hit);
}

static inline EnlngVal enlng_builtin_regex_find(EnlngVal pat, EnlngVal text) {
  const char *p = enlng_need_text("regex_find", pat);
  const char *t = enlng_need_text("regex_find", text);
  Rx r;
  RxNode *top = rx_compile(&r, p);
  EnlngVal out = enlng_make_null();
  if (rx_search(&r, top, t, 0))
    out = rx_slice(t, r.gs[0], r.ge[0]);
  rx_free(&r);
  return out;
}

static inline EnlngVal enlng_builtin_regex_find_all(EnlngVal pat, EnlngVal text) {
  const char *p = enlng_need_text("regex_find_all", pat);
  const char *t = enlng_need_text("regex_find_all", text);
  Rx r;
  RxNode *top = rx_compile(&r, p);
  EnlngList *l = enlng_list_create();
  long from = 0, len = (long)strlen(t);
  while (from <= len && rx_search(&r, top, t, from)) {
    enlng_list_push(l, rx_slice(t, r.gs[0], r.ge[0]));
    from = r.ge[0] > r.gs[0] ? r.ge[0] : r.ge[0] + (r.ge[0] < len ? enlng_u8_clen((unsigned char)t[r.ge[0]]) : 1);
  }
  rx_free(&r);
  return enlng_make_list(l);
}

/* The parts of the first match: [whole match, group 1, group 2, ...]. */
static inline EnlngVal enlng_builtin_regex_groups(EnlngVal pat, EnlngVal text) {
  const char *p = enlng_need_text("regex_groups", pat);
  const char *t = enlng_need_text("regex_groups", text);
  Rx r;
  RxNode *top = rx_compile(&r, p);
  EnlngVal out = enlng_make_null();
  if (rx_search(&r, top, t, 0)) {
    EnlngList *l = enlng_list_create();
    int n = r.ngroups < 9 ? r.ngroups : 9;
    for (int g = 0; g <= n; g++)
      enlng_list_push(l, rx_slice(t, r.gs[g], r.ge[g]));
    out = enlng_make_list(l);
  }
  rx_free(&r);
  return out;
}

/* Every match replaced; $0 is the match and $1 to $9 its groups, $$ is $. */
static inline EnlngVal enlng_builtin_regex_replace(EnlngVal pat, EnlngVal text, EnlngVal with) {
  const char *p = enlng_need_text("regex_replace", pat);
  const char *t = enlng_need_text("regex_replace", text);
  const char *w = enlng_need_text("regex_replace", with);
  Rx r;
  RxNode *top = rx_compile(&r, p);
  EnlngBuf b = {NULL, 0, 0};
  enlng_buf_add(&b, "");
  long from = 0, copied = 0, len = (long)strlen(t);
  char one[2] = {0, 0};
  while (from <= len && rx_search(&r, top, t, from)) {
    for (long k = copied; k < r.gs[0]; k++) {
      one[0] = t[k];
      enlng_buf_add(&b, one);
    }
    for (const char *q = w; *q; q++) {
      if (*q == '$' && q[1] == '$') {
        enlng_buf_add(&b, "$");
        q++;
      } else if (*q == '$' && isdigit((unsigned char)q[1])) {
        int g = q[1] - '0';
        if (g <= r.ngroups && r.gs[g] >= 0 && r.ge[g] >= r.gs[g])
          for (long k = r.gs[g]; k < r.ge[g]; k++) {
            one[0] = t[k];
            enlng_buf_add(&b, one);
          }
        q++;
      } else {
        one[0] = *q;
        enlng_buf_add(&b, one);
      }
    }
    copied = r.ge[0];
    if (r.ge[0] > r.gs[0]) {
      from = r.ge[0];
    } else {
      if (r.ge[0] < len) {
        int n = enlng_u8_clen((unsigned char)t[r.ge[0]]);
        for (int k = 0; k < n; k++) {
          one[0] = t[r.ge[0] + k];
          enlng_buf_add(&b, one);
        }
        copied = r.ge[0] + n;
        from = copied;
      } else {
        from = len + 1;
      }
    }
  }
  if (copied < len)
    enlng_buf_add(&b, t + copied);
  rx_free(&r);
  EnlngVal out = enlng_make_string(b.s);
  free(b.s);
  return out;
}

static inline EnlngVal enlng_builtin_regex_split(EnlngVal pat, EnlngVal text) {
  const char *p = enlng_need_text("regex_split", pat);
  const char *t = enlng_need_text("regex_split", text);
  Rx r;
  RxNode *top = rx_compile(&r, p);
  EnlngList *l = enlng_list_create();
  long from = 0, piece = 0, len = (long)strlen(t);
  while (from <= len && rx_search(&r, top, t, from)) {
    if (r.ge[0] == r.gs[0]) { /* an empty match does not split */
      if (r.ge[0] >= len)
        break;
      from = r.ge[0] + enlng_u8_clen((unsigned char)t[r.ge[0]]);
      continue;
    }
    enlng_list_push(l, rx_slice(t, piece, r.gs[0]));
    piece = from = r.ge[0];
  }
  enlng_list_push(l, rx_slice(t, piece, len));
  rx_free(&r);
  return enlng_make_list(l);
}

/* --- Dates and times (local time) --- */
static inline EnlngVal enlng_builtin_format_time(EnlngVal secs, EnlngVal pattern) {
  time_t t = (time_t)enlng_need_whole("format_time", secs);
  const char *f = enlng_need_text("format_time", pattern);
  struct tm *tm = localtime(&t);
  if (!tm)
    enlng_throw("'format_time' was given a time it cannot show.");
  char out[512];
  size_t n = strftime(out, sizeof(out), f, tm);
  if (n == 0 && *f)
    out[0] = '\0';
  return enlng_make_string(out);
}

static inline EnlngVal enlng_builtin_date_parts(EnlngVal secs) {
  static const char *days[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                               "Thursday", "Friday", "Saturday"};
  time_t t = (time_t)enlng_need_whole("date_parts", secs);
  struct tm *tm = localtime(&t);
  if (!tm)
    enlng_throw("'date_parts' was given a time it cannot show.");
  EnlngVal m = enlng_make_map(enlng_map_create());
  enlng_map_set(m, "year", enlng_make_int(tm->tm_year + 1900));
  enlng_map_set(m, "month", enlng_make_int(tm->tm_mon + 1));
  enlng_map_set(m, "day", enlng_make_int(tm->tm_mday));
  enlng_map_set(m, "hour", enlng_make_int(tm->tm_hour));
  enlng_map_set(m, "minute", enlng_make_int(tm->tm_min));
  enlng_map_set(m, "second", enlng_make_int(tm->tm_sec));
  enlng_map_set(m, "weekday", enlng_make_string(days[tm->tm_wday]));
  enlng_map_set(m, "day_of_year", enlng_make_int(tm->tm_yday + 1));
  return m;
}

static inline int enlng_part(EnlngVal m, const char *key, int dflt) {
  if (!enlng_map_has(m, key))
    return dflt;
  EnlngVal v = enlng_map_get(m, key);
  char fn[40];
  snprintf(fn, sizeof(fn), "make_time (%s)", key);
  return (int)enlng_need_whole(fn, v);
}

/* {"year": 2026, "month": 11, "day": 1} -> seconds; missing parts are 0
   (month and day are 1). */
static inline EnlngVal enlng_builtin_make_time(EnlngVal parts) {
  if (parts.type != ENLNG_VAL_MAP) {
    char msg[200];
    snprintf(msg, sizeof(msg),
             "'make_time' needs a map such as {\"year\": 2026, \"month\": 1, "
             "\"day\": 31}, but got %s.", enlng_kind(parts));
    enlng_throw(msg);
  }
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  tm.tm_year = enlng_part(parts, "year", 1970) - 1900;
  tm.tm_mon = enlng_part(parts, "month", 1) - 1;
  tm.tm_mday = enlng_part(parts, "day", 1);
  tm.tm_hour = enlng_part(parts, "hour", 0);
  tm.tm_min = enlng_part(parts, "minute", 0);
  tm.tm_sec = enlng_part(parts, "second", 0);
  tm.tm_isdst = -1;
  time_t t = mktime(&tm);
  if (t == (time_t)-1)
    enlng_throw("'make_time' was given a date it cannot handle.");
  return enlng_make_int((int64_t)t);
}

/* --- Folders --- */
#ifdef _WIN32
#include <direct.h>
#define enlng_mkdir(p) _mkdir(p)
#define enlng_rmdir(p) _rmdir(p)
#define enlng_popen _popen
#define enlng_pclose _pclose
#ifndef _MSC_VER
FILE *_popen(const char *, const char *);
int _pclose(FILE *);
#endif
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define enlng_mkdir(p) mkdir(p, 0777)
#define enlng_rmdir(p) rmdir(p)
#define enlng_popen popen
#define enlng_pclose pclose
#endif

static inline bool enlng_is_dir(const char *p) {
#ifdef _WIN32
  DWORD a = GetFileAttributesA(p);
  return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
#else
  struct stat st;
  return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

static inline EnlngVal enlng_builtin_is_folder(EnlngVal path) {
  return enlng_make_bool(enlng_is_dir(enlng_need_text("is_folder", path)));
}

static inline int enlng_cmp_cstr(const void *a, const void *b) {
  return strcmp(*(char *const *)a, *(char *const *)b);
}

/* The names in a folder, sorted. */
static inline EnlngVal enlng_builtin_list_folder(EnlngVal path) {
  const char *p = enlng_need_text("list_folder", path);
  if (!enlng_is_dir(p)) {
    char msg[600];
    snprintf(msg, sizeof(msg), "There is no folder called \"%.500s\".", p);
    enlng_throw(msg);
  }
  char **names = NULL;
  int n = 0, cap = 0;
#ifdef _WIN32
  char pattern[1100];
  snprintf(pattern, sizeof(pattern), "%s\\*", p);
  WIN32_FIND_DATAA fd;
  HANDLE h = FindFirstFileA(pattern, &fd);
  if (h != INVALID_HANDLE_VALUE) {
    do {
      const char *nm = fd.cFileName;
#else
  DIR *d = opendir(p);
  struct dirent *de;
  while (d && (de = readdir(d)) != NULL) {
    {
      const char *nm = de->d_name;
#endif
      if (strcmp(nm, ".") != 0 && strcmp(nm, "..") != 0) {
        if (n >= cap) {
          cap = cap ? cap * 2 : 32;
          names = (char **)realloc(names, sizeof(char *) * cap);
        }
        size_t k = strlen(nm);
        names[n] = (char *)malloc(k + 1);
        memcpy(names[n], nm, k + 1);
        n++;
      }
#ifdef _WIN32
    } while (FindNextFileA(h, &fd));
    FindClose(h);
  }
#else
    }
  }
  if (d)
    closedir(d);
#endif
  qsort(names, (size_t)n, sizeof(char *), enlng_cmp_cstr);
  EnlngList *l = enlng_list_create();
  for (int i = 0; i < n; i++) {
    enlng_list_push(l, enlng_make_string(names[i]));
    free(names[i]);
  }
  free(names);
  return enlng_make_list(l);
}

/* Makes a folder and any folders above it that are missing. */
static inline EnlngVal enlng_builtin_make_folder(EnlngVal path) {
  const char *p = enlng_need_text("make_folder", path);
  size_t k = strlen(p);
  char *buf = (char *)malloc(k + 1);
  memcpy(buf, p, k + 1);
  for (size_t i = 1; i <= k; i++) {
    if (buf[i] == '/' || buf[i] == '\\' || buf[i] == '\0') {
      char c = buf[i];
      buf[i] = '\0';
      if (!(i == 2 && buf[1] == ':') && !enlng_is_dir(buf))
        enlng_mkdir(buf);
      buf[i] = c;
    }
  }
  free(buf);
  return enlng_make_bool(enlng_is_dir(p));
}

/* Deletes a file, or a folder that is empty. */
static inline EnlngVal enlng_builtin_delete_file(EnlngVal path) {
  const char *p = enlng_need_text("delete_file", path);
  if (enlng_is_dir(p))
    return enlng_make_bool(enlng_rmdir(p) == 0);
  return enlng_make_bool(remove(p) == 0);
}

static inline EnlngVal enlng_builtin_rename_file(EnlngVal from, EnlngVal to) {
  const char *a = enlng_need_text("rename_file", from);
  const char *b = enlng_need_text("rename_file", to);
  return enlng_make_bool(rename(a, b) == 0);
}

/* --- Running other programs --- */

/* Everything a command prints, and the number it ends with. */
static inline EnlngVal enlng_run_shell(const char *cmd, int *code) {
  FILE *f = enlng_popen(cmd, "r");
  if (!f) {
    *code = -1;
    return enlng_make_string("");
  }
  EnlngBuf b = {NULL, 0, 0};
  enlng_buf_add(&b, "");
  char chunk[4096];
  size_t n;
  while ((n = fread(chunk, 1, sizeof(chunk) - 1, f)) > 0) {
    chunk[n] = '\0';
    enlng_buf_add(&b, chunk);
  }
  int st = enlng_pclose(f);
#ifdef _WIN32
  *code = st;
#else
  *code = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
#endif
  EnlngVal out = enlng_make_string(b.s);
  free(b.s);
  return out;
}

/* run_command("git status") -> {"code": 0, "output": "..."} */
static inline EnlngVal enlng_builtin_run_command(EnlngVal command) {
  const char *c = enlng_need_text("run_command", command);
  size_t k = strlen(c);
  char *full = (char *)malloc(k + 16);
#ifdef _WIN32
  snprintf(full, k + 16, "\"%s 2>&1\"", c); /* cmd.exe drops the outer quotes */
#else
  snprintf(full, k + 16, "%s 2>&1", c);
#endif
  fflush(stdout);
  int code;
  EnlngVal out = enlng_run_shell(full, &code);
  free(full);
  EnlngVal m = enlng_make_map(enlng_map_create());
  enlng_map_set(m, "code", enlng_make_int(code));
  enlng_map_set(m, "output", out);
  return m;
}

/* --- The web: http_get(url), http_post(url, body) ---
   These use the curl program, which comes with Windows 10 and later, macOS
   and most Linux systems. The result is {"status": 200, "body": "..."}. */
static inline void enlng_temp_path(char *out, size_t sz, const char *what) {
  static int counter = 0;
#ifdef _WIN32
  char dir[MAX_PATH];
  DWORD n = GetTempPathA(MAX_PATH, dir);
  if (n == 0 || n > MAX_PATH)
    strcpy(dir, ".\\");
  snprintf(out, sz, "%senlng_%lu_%d_%s", dir, (unsigned long)GetCurrentProcessId(),
           ++counter, what);
#else
  const char *dir = getenv("TMPDIR");
  snprintf(out, sz, "%s/enlng_%lu_%d_%s", dir && *dir ? dir : "/tmp",
           (unsigned long)getpid(), ++counter, what);
#endif
}

static inline char *enlng_slurp(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc((size_t)len + 1);
  size_t got = fread(buf, 1, (size_t)len, f);
  buf[got] = '\0';
  fclose(f);
  return buf;
}

static inline EnlngVal enlng_http(const char *fn, const char *url, EnlngVal *body) {
  for (const char *q = url; *q; q++)
    if (*q == '"' || *q == '\'' || *q == '\n' || *q == '\r' || *q == '`') {
      char msg[200];
      snprintf(msg, sizeof(msg), "'%s' was given an address with a character a web address cannot have.", fn);
      enlng_throw(msg);
    }
  if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
    char msg[200];
    snprintf(msg, sizeof(msg), "'%s' needs an address that starts with http:// or https://.", fn);
    enlng_throw(msg);
  }
  char out_path[600], err_path[600], in_path[600];
  enlng_temp_path(out_path, sizeof(out_path), "body");
  enlng_temp_path(err_path, sizeof(err_path), "err");
  in_path[0] = '\0';
  const char *ctype = "text/plain; charset=utf-8";
  if (body) {
    enlng_temp_path(in_path, sizeof(in_path), "send");
    EnlngVal send = *body;
    if (send.type == ENLNG_VAL_LIST || send.type == ENLNG_VAL_MAP) {
      send = enlng_builtin_to_json(send);
      ctype = "application/json";
    } else if (send.type != ENLNG_VAL_STRING) {
      send = enlng_builtin_to_text(send);
    }
    FILE *f = fopen(in_path, "wb");
    if (f) {
      fputs(send.as.s ? send.as.s : "", f);
      fclose(f);
    }
  }
  size_t k = strlen(url) + strlen(out_path) + strlen(err_path) + strlen(in_path) + 300;
  char *cmd = (char *)malloc(k);
  const char *q = "\"";
#ifndef _WIN32
  q = "'";
#endif
  if (body)
    snprintf(cmd, k, "%scurl -s -S -L --max-time 60 -o %s%s%s -w %%{http_code} "
             "-H %sContent-Type: %s%s --data-binary @%s%s%s %s%s%s 2>%s%s%s%s",
#ifdef _WIN32
             "\"",
#else
             "",
#endif
             q, out_path, q, q, ctype, q, q, in_path, q, q, url, q, q, err_path, q,
#ifdef _WIN32
             "\""
#else
             ""
#endif
    );
  else
    snprintf(cmd, k, "%scurl -s -S -L --max-time 60 -o %s%s%s -w %%{http_code} %s%s%s 2>%s%s%s%s",
#ifdef _WIN32
             "\"",
#else
             "",
#endif
             q, out_path, q, q, url, q, q, err_path, q,
#ifdef _WIN32
             "\""
#else
             ""
#endif
    );
  int code;
  fflush(stdout);
  EnlngVal status_text = enlng_run_shell(cmd, &code);
  free(cmd);
  char *resp = enlng_slurp(out_path);
  char *err = enlng_slurp(err_path);
  remove(out_path);
  remove(err_path);
  if (in_path[0])
    remove(in_path);
  int status = atoi(status_text.as.s ? status_text.as.s : "0");
  if (code != 0 || status == 0) {
    char msg[600];
    if (err)
      for (char *e = err; *e; e++)
        if (*e == '\n' || *e == '\r')
          *e = ' ';
    snprintf(msg, sizeof(msg), "Could not reach %.200s: %.300s", url,
             err && *err ? err : "the curl program did not run. Check that curl is installed.");
    free(resp);
    free(err);
    enlng_throw(msg);
  }
  EnlngVal m = enlng_make_map(enlng_map_create());
  enlng_map_set(m, "status", enlng_make_int(status));
  enlng_map_set(m, "body", enlng_make_string(resp ? resp : ""));
  free(resp);
  free(err);
  return m;
}

static inline EnlngVal enlng_builtin_http_get(EnlngVal url) {
  return enlng_http("http_get", enlng_need_text("http_get", url), NULL);
}

static inline EnlngVal enlng_builtin_http_post(EnlngVal url, EnlngVal body) {
  return enlng_http("http_post", enlng_need_text("http_post", url), &body);
}

#define ENLNG_MORE_BUILTINS(X)                                                \
  X(to_json, 1) X(to_json_pretty, 1) X(from_json, 1) X(regex_match, 2)        \
  X(regex_find, 2) X(regex_find_all, 2) X(regex_groups, 2)                    \
  X(regex_replace, 3) X(regex_split, 2) X(format_time, 2) X(date_parts, 1)    \
  X(make_time, 1) X(is_folder, 1) X(list_folder, 1) X(make_folder, 1)         \
  X(delete_file, 1) X(rename_file, 2) X(run_command, 1) X(http_get, 1)        \
  X(http_post, 2)

/* --- The table of built-ins ---------------------------------------------
   One row per built-in: its name and how many inputs it takes. The
   interpreter, the C generator and function values all read this table, so
   a built-in is added in one place. */
#define ENLNG_BUILTINS(X)                                                     \
  X(sqrt, 1) X(pow, 2) X(abs, 1) X(floor, 1) X(ceil, 1) X(round, 1)           \
  X(min, 2) X(max, 2) X(random_number, 2) X(read_file, 1) X(write_file, 2)    \
  X(append_file, 2) X(file_exists, 1) X(time_now, 0) X(sleep, 1)              \
  X(append, 2) X(pop, 1) X(keys, 1) X(values, 1) X(has_key, 2) X(len, 1)      \
  X(split, 2) X(join, 2) X(contains, 2) X(string_add, 3)                      \
  X(string_insert, 3) X(string_remove_at, 2) X(string_remove, 2)              \
  X(string_set_at, 3) X(string_replace, 3) X(slc, 3) X(to_number, 1)          \
  X(to_text, 1) X(arguments, 0) X(env, 1) X(type_of, 1) X(char_code, 1)       \
  X(char_of, 1) X(unique, 1) X(remove_key, 2) X(insert_at, 3)                 \
  X(remove_at, 2) X(time_ms, 0) X(check, 2) ENLNG_MORE_BUILTINS(X)

#define ENLNG_BI_ARITY(nm, n)                                                 \
  if (strcmp(name, #nm) == 0)                                                 \
    return n;
/* Inputs a built-in takes, or -1 when no built-in has this name. */
static inline int enlng_builtin_arity(const char *name) {
  ENLNG_BUILTINS(ENLNG_BI_ARITY)
  return -1;
}

#define ENLNG_BI_ARGS0
#define ENLNG_BI_ARGS1 a[0]
#define ENLNG_BI_ARGS2 a[0], a[1]
#define ENLNG_BI_ARGS3 a[0], a[1], a[2]
#define ENLNG_BI_CALL(nm, n)                                                  \
  if (strcmp(name, #nm) == 0)                                                 \
    return enlng_builtin_##nm(ENLNG_BI_ARGS##n);
/* Runs a built-in; the caller has checked the number of inputs. */
static inline EnlngVal enlng_call_builtin(const char *name, EnlngVal *a) {
  (void)a;
  ENLNG_BUILTINS(ENLNG_BI_CALL)
  return enlng_make_null();
}

/* A built-in held in a value, such as f after remember f as sqrt. */
static inline EnlngVal enlng_call_builtin_value(const char *name, int argc, EnlngVal *a) {
  int need = enlng_builtin_arity(name);
  if (argc != need)
    enlng_wrong_arity(name, need, argc);
  return enlng_call_builtin(name, a);
}

#endif /* ENLNG_RUNTIME_H */
