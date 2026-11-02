/*
 * =====================================================================
 *   ENLANG SOVEREIGN PROGRAMMING LANGUAGE (Pure C99 Native)
 *   src/enlng_interp.c - Built-in interpreter: runs Enlang with no C compiler
 * =====================================================================
 *
 * Walks the same AST the C code generator uses, and runs it on the same
 * value model and built-ins (src/enlng_runtime.h). That keeps `enlng run`
 * and `enlng run --native` producing the same output, while the default
 * path needs no gcc.
 *
 * Scoping follows the generated C: every block is its own scope, a name
 * declared in a block disappears when the block ends, functions cannot see
 * the caller's variables, and assigning to a name that does not exist yet
 * declares it.
 */

#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#define INTERP_MAX_DEPTH 20000
#define INTERP_STACK_BYTES ((size_t)512 * 1024 * 1024)

typedef struct {
  const char *name; /* borrowed from the AST */
  EnlngVal val;
} IVar;

typedef struct {
  IVar *v;
  int n, cap;
  int scope; /* index where the innermost scope starts */
  bool owned; /* v was malloc'd here (a call starts on a stack buffer) */
  const char *gl[32]; /* names this function declared with `global` */
  int gln;
} IFrame;

typedef struct {
  ASTNode *prog;
  const char *file;
  const char *source;
  IFrame *globals; /* the top-level names, readable from inside functions */
  bool failed;
  char err[512];
  int err_line;
  int depth;
  volatile int line; /* the statement running now, for errors the runtime throws */
  bool *sorted; /* the innermost `repeat until sorted` flag */
  EnlngVal ret;
  /* Roots for the collector: the frames of the calls running now, and
     values an expression holds while it evaluates more (see iroot). */
  IFrame **frames;
  int nframes, capframes;
  EnlngVal *tmp;
  int ntmp, captmp;
} Interp;

enum { EX_OK = 0, EX_BREAK, EX_CONTINUE, EX_RETURN };

static EnlngVal ieval(Interp *it, IFrame *f, ASTNode *n);
static EnlngVal ibinary(Interp *it, IFrame *f, ASTNode *n);
static int iexec(Interp *it, IFrame *f, ASTNode *n);

/* Keep v alive while the expression that holds it evaluates something else
   (which may run statements, and so a collection). Callers reset it->ntmp. */
static void iroot(Interp *it, EnlngVal v) {
  if (it->ntmp >= it->captmp) {
    it->captmp = it->captmp ? it->captmp * 2 : 64;
    it->tmp = (EnlngVal *)realloc(it->tmp, sizeof(EnlngVal) * it->captmp);
  }
  it->tmp[it->ntmp++] = v;
}

static void ipush_frame(Interp *it, IFrame *f) {
  if (it->nframes >= it->capframes) {
    it->capframes = it->capframes ? it->capframes * 2 : 64;
    it->frames = (IFrame **)realloc(it->frames, sizeof(IFrame *) * it->capframes);
  }
  it->frames[it->nframes++] = f;
}

/* Free everything the program can no longer reach. Runs only at the start
   of a statement, when every live value is in a frame, on the root stack,
   or about to be returned. */
/* Every Interp running now: the program's, and one for each thread of a
   run together block. */
static Interp *g_live[256];
static int g_nlive = 0;

static void ilive_add(Interp *it) {
  if (g_nlive < (int)(sizeof(g_live) / sizeof(g_live[0])))
    g_live[g_nlive++] = it;
}

static void ilive_remove(Interp *it) {
  for (int i = 0; i < g_nlive; i++)
    if (g_live[i] == it) {
      g_live[i] = g_live[--g_nlive];
      return;
    }
}

static void igc_mark(Interp *it) {
  for (int i = 0; i < it->nframes; i++) {
    IFrame *f = it->frames[i];
    for (int k = 0; k < f->n; k++)
      enlng_gc_mark_val(f->v[k].val);
  }
  for (int i = 0; i < it->ntmp; i++)
    enlng_gc_mark_val(it->tmp[i]);
  enlng_gc_mark_val(it->ret);
}

static void igc(Interp *it) {
  igc_mark(it);
  for (int i = 0; i < g_nlive; i++)
    if (g_live[i] != it)
      igc_mark(g_live[i]);
  for (int i = 0; i < enlng_gc_nextra; i++)
    enlng_gc_mark_val(enlng_gc_extra[i]);
  enlng_gc_sweep();
}

static void ifail(Interp *it, int line, const char *fmt, ...) {
  if (it->failed)
    return;
  it->failed = true;
  it->err_line = line;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(it->err, sizeof(it->err), fmt, ap);
  va_end(ap);
}

static IVar *ilookup(IFrame *f, const char *name) {
  for (int i = f->n - 1; i >= 0; i--) {
    const char *vn = f->v[i].name;
    if (vn == name || (vn[0] == name[0] && strcmp(vn, name) == 0))
      return &f->v[i];
  }
  return NULL;
}

/* New name in the current scope (or overwrite one declared in this scope). */
static void idefine(IFrame *f, const char *name, EnlngVal val) {
  for (int i = f->n - 1; i >= f->scope; i--) {
    const char *vn = f->v[i].name;
    if (vn == name || (vn[0] == name[0] && strcmp(vn, name) == 0)) {
      f->v[i].val = val;
      return;
    }
  }
  if (f->n >= f->cap) {
    int ncap = f->cap ? f->cap * 2 : 16;
    if (f->owned || !f->v) {
      f->v = (IVar *)realloc(f->v, sizeof(IVar) * ncap);
    } else {
      IVar *nv = (IVar *)malloc(sizeof(IVar) * ncap);
      memcpy(nv, f->v, sizeof(IVar) * f->n);
      f->v = nv;
    }
    f->owned = true;
    f->cap = ncap;
  }
  f->v[f->n].name = name;
  f->v[f->n].val = val;
  f->n++;
}

static ASTNode *ifind_func(Interp *it, const char *name);

/* A name in this frame, or else at the top of the program. */
static IVar *ifind_var(Interp *it, IFrame *f, const char *name) {
  IVar *v = ilookup(f, name);
  if (!v && it->globals && f != it->globals)
    v = ilookup(it->globals, name);
  return v;
}

/* The variable `name = value` changes: a local of this frame, or the
   top-level name when the function said `global name`. NULL means the
   assignment makes a new local. */
static IVar *iassign_var(Interp *it, IFrame *f, const char *name, int line) {
  IVar *v = ilookup(f, name);
  if (v || f == it->globals)
    return v;
  for (int i = 0; i < f->gln && i < 32; i++) {
    if (strcmp(f->gl[i], name) == 0) {
      v = it->globals ? ilookup(it->globals, name) : NULL;
      if (!v)
        ifail(it, line,
              "There is no name '%s' at the top of the program to change. "
              "Make it there first, for example: remember %s as 0",
              name, name);
      return v;
    }
  }
  return NULL;
}

static bool iis_builtin(const char *name);

static EnlngVal iget(Interp *it, IFrame *f, const char *name, int line) {
  IVar *v = ifind_var(it, f, name);
  if (!v) {
    /* the name of a function is a value too: remember f as double */
    ASTNode *fn = ifind_func(it, name);
    if (fn)
      return enlng_make_func(fn->as.func_def.name);
    if (strcmp(name, "pi") == 0)
      return enlng_make_float(3.14159265358979323846);
    if (iis_builtin(name))
      return enlng_make_builtin_func(name);
    ifail(it, line,
          "I don't know a name '%s' here. Define it first, for example: remember %s as 0",
          name, name);
    return enlng_make_null();
  }
  return v->val;
}

/* ------------------------------------------------------------- built-ins */

/* The table of built-ins lives in the runtime (ENLNG_BUILTINS), shared with
 * the C generator. */
static bool iis_builtin(const char *name) { return enlng_builtin_arity(name) >= 0; }

/* ----------------------------------------------------------------- calls */

static ASTNode *ifind_func(Interp *it, const char *name) {
  ASTNode *p = it->prog;
  for (int i = 0; i < p->as.program.count; i++) {
    ASTNode *s = p->as.program.statements[i];
    if (s && s->type == AST_FUNCTION && strcmp(s->as.func_def.name, name) == 0)
      return s;
  }
  return NULL;
}

/* Runs a function of the program. nargs inputs were given; ncap values an
 * inner function kept follow them. An input left out takes its default. */
static EnlngVal irun_func(Interp *it, ASTNode *fn, const char *name,
                          EnlngVal *args, int nargs, EnlngVal *caps, int ncap,
                          int line) {
  EnlngVal result = enlng_make_null();
  int frame_base = it->nframes;
  int own = fn->as.func_def.param_count - fn->as.func_def.hidden_count;
  int lo = fn->as.func_def.has_defaults ? fn->as.func_def.min_count : own;
  if (nargs < lo || nargs > own) {
    if (lo == own)
      ifail(it, line, "'%s' needs %d input%s but was given %d.", name, own,
            own == 1 ? "" : "s", nargs);
    else
      ifail(it, line, "'%s' needs %d to %d inputs but was given %d.", name, lo,
            own, nargs);
    return result;
  }
  if (it->depth >= INTERP_MAX_DEPTH) {
    ifail(it, line,
          "'%s' called itself too many times (over %d levels deep). Check that it has a way to stop.",
          name, INTERP_MAX_DEPTH);
    return result;
  }
  IVar stackv[16];
  IFrame nf = {stackv, 0, 16, 0, false};
  for (int p = 0; p < own; p++)
    idefine(&nf, fn->as.func_def.params[p], p < nargs ? args[p] : enlng_make_null());
  for (int c = 0; c < ncap; c++)
    idefine(&nf, fn->as.func_def.params[own + c], caps[c]);
  bool *saved_sorted = it->sorted;
  it->sorted = NULL;
  it->depth++;
  ipush_frame(it, &nf);
  int count = fn->as.func_def.body_count;
  for (int b = 0; b < count && !it->failed; b++) {
    ASTNode *s = fn->as.func_def.body[b];
    if (b == count - 1 && s->type == AST_EXPR_STMT) {
      result = ieval(it, &nf, s->as.expr_stmt.expr);
      break;
    }
    int code = iexec(it, &nf, s);
    if (code == EX_RETURN) {
      result = it->ret;
      break;
    }
  }
  it->depth--;
  it->sorted = saved_sorted;
  it->nframes = frame_base;
  if (nf.owned)
    free(nf.v);
  return result;
}

/* Calls the function a value holds: a built-in, a function of the program,
 * or an inner function, which also gets the outer values it kept. */
static EnlngVal icall_value(Interp *it, EnlngFunc *h, EnlngVal *args, int argc,
                            int line) {
  if (h->builtin) {
    int need = enlng_builtin_arity(h->name);
    if (argc != need) {
      ifail(it, line, "'%s' needs %d input%s but was given %d.", h->name, need,
            need == 1 ? "" : "s", argc);
      return enlng_make_null();
    }
    return enlng_call_builtin(h->name, args);
  }
  ASTNode *fn = ifind_func(it, h->name);
  if (!fn) {
    ifail(it, line, "I don't know a function called '%s'.", h->shown);
    return enlng_make_null();
  }
  return irun_func(it, fn, h->shown, args, argc, h->caps, h->ncap, line);
}

static EnlngVal icall(Interp *it, IFrame *f, ASTNode *n) {
  const char *name = n->as.call_expr.func_name;
  int argc = n->as.call_expr.arg_count;
  EnlngVal argbuf[8];
  EnlngVal *args = argc <= 8 ? argbuf : (EnlngVal *)malloc(sizeof(EnlngVal) * argc);
  EnlngVal result = enlng_make_null();
  int tmp_base = it->ntmp, frame_base = it->nframes;

  for (int i = 0; i < argc; i++) {
    args[i] = ieval(it, f, n->as.call_expr.args[i]);
    iroot(it, args[i]);
  }

  if (!it->failed) {
    /* The functions of a program never change while it runs, so each call
     * site looks its function up once. */
    if (!n->as.call_expr.resolved) {
      n->as.call_expr.fn = n->as.call_expr.builtin_only ? NULL : ifind_func(it, name);
      n->as.call_expr.resolved = true;
    }
    ASTNode *fn = n->as.call_expr.fn;
    EnlngFunc *held = NULL;
    /* No function has this name: a name that holds a function can be called,
     * as in apply(inc, 1) where apply calls g(x). */
    if (!fn && !n->as.call_expr.builtin_only) {
      IVar *v = ifind_var(it, f, name);
      if (v && v->val.type == ENLNG_VAL_FUNC) {
        held = v->val.as.fn;
        iroot(it, v->val);
      } else if (v && !iis_builtin(name)) {
        ifail(it, n->line, "'%s' is %s, not a function, so it cannot be called.",
              name, enlng_kind(v->val));
      }
    }
    if (it->failed) {
    } else if (held) {
      result = icall_value(it, held, args, argc, n->line);
    } else if (fn) {
      result = irun_func(it, fn, name, args, argc, NULL, 0, n->line);
    } else {
      int need = enlng_builtin_arity(name);
      if (need < 0) {
        ifail(it, n->line,
              "I don't know a function called '%s'. Check the spelling, or define it with: action %s(...):",
              name, name);
      } else if (argc != need) {
        ifail(it, n->line, "'%s' needs %d input%s but was given %d.", name,
              need, need == 1 ? "" : "s", argc);
      } else {
        result = enlng_call_builtin(name, args);
      }
    }
  }
  if (args != argbuf)
    free(args);
  it->ntmp = tmp_base;
  it->nframes = frame_base;
  return result;
}

/* ----------------------------------------------------------- expressions */

static EnlngVal ibinary(Interp *it, IFrame *f, ASTNode *n) {
    EnlngTokenType op = n->as.binary_expr.op;
    if (op == ENLNG_TOKEN_AND) {
      if (!enlng_is_truthy(ieval(it, f, n->as.binary_expr.left)))
        return enlng_make_bool(false);
      return enlng_make_bool(enlng_is_truthy(ieval(it, f, n->as.binary_expr.right)));
    }
    if (op == ENLNG_TOKEN_OR) {
      if (enlng_is_truthy(ieval(it, f, n->as.binary_expr.left)))
        return enlng_make_bool(true);
      return enlng_make_bool(enlng_is_truthy(ieval(it, f, n->as.binary_expr.right)));
    }
    int base = it->ntmp;
    EnlngVal a = ieval(it, f, n->as.binary_expr.left);
    iroot(it, a);
    EnlngVal b = ieval(it, f, n->as.binary_expr.right);
    it->ntmp = base;
    switch (op) {
    case ENLNG_TOKEN_ADD: return enlng_val_add(a, b);
    case ENLNG_TOKEN_SUB: return enlng_val_sub(a, b);
    case ENLNG_TOKEN_MUL: return enlng_val_mul(a, b);
    case ENLNG_TOKEN_DIV: return enlng_val_div(a, b);
    case ENLNG_TOKEN_MOD: return enlng_val_mod(a, b);
    case ENLNG_TOKEN_EQ:  return enlng_make_bool(enlng_vals_equal(a, b));
    case ENLNG_TOKEN_NEQ: return enlng_make_bool(!enlng_vals_equal(a, b));
    case ENLNG_TOKEN_GT:  return enlng_make_bool(enlng_vals_gt(a, b));
    case ENLNG_TOKEN_LT:  return enlng_make_bool(enlng_vals_lt(a, b));
    case ENLNG_TOKEN_GTE: return enlng_make_bool(enlng_vals_gte(a, b));
    case ENLNG_TOKEN_LTE: return enlng_make_bool(enlng_vals_lte(a, b));
    default: return enlng_make_null();
    }
}

static EnlngVal ieval(Interp *it, IFrame *f, ASTNode *n) {
  if (!n || it->failed)
    return enlng_make_null();

  switch (n->type) {
  case AST_EXPR_LITERAL_INT:
    return enlng_make_int(n->as.literal.int_val);
  case AST_EXPR_LITERAL_FLOAT:
    return enlng_make_float(n->as.literal.float_val);
  case AST_EXPR_LITERAL_BOOL:
    return enlng_make_bool(n->as.literal.bool_val);
  case AST_EXPR_TERNARY:
    return enlng_is_truthy(ieval(it, f, n->as.ternary.cond))
               ? ieval(it, f, n->as.ternary.yes)
               : ieval(it, f, n->as.ternary.no);
  case AST_EXPR_LITERAL_NULL:
    return enlng_make_null();
  case AST_EXPR_LITERAL_STRING:
    return enlng_make_string(n->as.literal.string_val ? n->as.literal.string_val : "");

  case AST_EXPR_VARIABLE:
    return iget(it, f, n->as.variable.name, n->line);

  case AST_EXPR_INDEX: {
    int base = it->ntmp;
    EnlngVal c = n->as.index_expr.target
                     ? ieval(it, f, n->as.index_expr.target)
                     : iget(it, f, n->as.index_expr.arr_name, n->line);
    iroot(it, c);
    EnlngVal k = ieval(it, f, n->as.index_expr.index_expr);
    it->ntmp = base;
    return enlng_container_get(c, k);
  }

  case AST_EXPR_FIELD: {
    const char *obj = n->as.field_expr.obj_name;
    const char *fld = n->as.field_expr.field_name;
    if (obj && strcmp(obj, "pair") == 0) {
      if (strcmp(fld, "left") == 0)
        return iget(it, f, "pair_left", n->line);
      if (strcmp(fld, "right") == 0)
        return iget(it, f, "pair_right", n->line);
      return enlng_make_null();
    }
    EnlngVal m = n->as.field_expr.target
                     ? ieval(it, f, n->as.field_expr.target)
                     : iget(it, f, obj ? obj : "", n->line);
    return enlng_map_get(m, fld);
  }

  case AST_EXPR_COUNT_OF:
    return enlng_make_int(enlng_count_of(ieval(it, f, n->as.single_target_expr.target)));
  case AST_EXPR_COUNT_IN:
  case AST_EXPR_SPLIT:
  case AST_EXPR_JOIN:
  case AST_EXPR_FIND: {
    int base = it->ntmp;
    EnlngVal t = ieval(it, f, n->as.count_in_expr.target);
    iroot(it, t);
    EnlngVal c = ieval(it, f, n->as.count_in_expr.container);
    it->ntmp = base;
    if (n->type == AST_EXPR_COUNT_IN)
      return enlng_make_int(enlng_count_in(t, c));
    if (n->type == AST_EXPR_SPLIT)
      return enlng_builtin_split(t, c);
    if (n->type == AST_EXPR_JOIN)
      return enlng_builtin_join(t, c);
    return enlng_builtin_find(t, c);
  }
  case AST_EXPR_REPLACE: {
    int base = it->ntmp;
    EnlngVal t = ieval(it, f, n->as.replace_expr.target);
    iroot(it, t);
    EnlngVal r = ieval(it, f, n->as.replace_expr.replacement);
    iroot(it, r);
    EnlngVal c = ieval(it, f, n->as.replace_expr.container);
    it->ntmp = base;
    return enlng_phrase_replace(t, r, c);
  }
  case AST_EXPR_TRIM:
    return enlng_builtin_trim(ieval(it, f, n->as.single_target_expr.target));
  case AST_EXPR_TO_UPPER:
    return enlng_builtin_to_upper(ieval(it, f, n->as.single_target_expr.target));
  case AST_EXPR_TO_LOWER:
    return enlng_builtin_to_lower(ieval(it, f, n->as.single_target_expr.target));
  case AST_EXPR_SUM:
    return enlng_builtin_sum(ieval(it, f, n->as.single_target_expr.target));
  case AST_EXPR_AVG:
    return enlng_builtin_avg(ieval(it, f, n->as.single_target_expr.target));
  case AST_EXPR_MAX:
    return enlng_builtin_list_max(ieval(it, f, n->as.single_target_expr.target));
  case AST_EXPR_MIN:
    return enlng_builtin_list_min(ieval(it, f, n->as.single_target_expr.target));
  case AST_EXPR_REVERSE:
    return enlng_val_reverse(ieval(it, f, n->as.single_target_expr.target));

  case AST_EXPR_LAMBDA: { /* a function value that keeps outer values */
    ASTNode *fn = n->as.lambda_expr.fn;
    int k = n->as.lambda_expr.ncap;
    EnlngVal buf[16];
    EnlngVal *vals = k <= 16 ? buf : (EnlngVal *)malloc(sizeof(EnlngVal) * k);
    for (int i = 0; i < k; i++) {
      IVar *v = ifind_var(it, f, n->as.lambda_expr.caps[i]);
      vals[i] = v ? v->val : enlng_make_null();
    }
    EnlngVal r = enlng_make_closure(fn->as.func_def.name, fn->as.func_def.shown,
                                    false, k, vals);
    if (vals != buf)
      free(vals);
    return r;
  }
  case AST_EXPR_LIST_LITERAL: {
    int base = it->ntmp;
    EnlngList *l = enlng_list_create();
    iroot(it, enlng_make_list(l));
    for (int i = 0; i < n->as.list_literal.count; i++) {
      EnlngVal item = ieval(it, f, n->as.list_literal.elements[i]);
      enlng_list_push(l, item);
    }
    it->ntmp = base;
    return enlng_make_list(l);
  }

  case AST_EXPR_MAP_LITERAL: {
    int base = it->ntmp;
    EnlngVal m = enlng_make_map(enlng_map_create());
    iroot(it, m);
    for (int i = 0; i < n->as.map_literal.count; i++) {
      EnlngVal item = ieval(it, f, n->as.map_literal.values[i]);
      enlng_map_set(m, n->as.map_literal.keys[i], item);
    }
    it->ntmp = base;
    return m;
  }

  case AST_EXPR_BINARY:
    return ibinary(it, f, n);

  case AST_EXPR_UNARY: {
    EnlngVal v = ieval(it, f, n->as.unary_expr.operand);
    if (n->as.unary_expr.op == ENLNG_TOKEN_NOT)
      return enlng_make_bool(!enlng_is_truthy(v));
    if (n->as.unary_expr.op == ENLNG_TOKEN_SUB)
      return enlng_val_sub(enlng_make_int(0), v);
    return enlng_make_null();
  }

  case AST_EXPR_CALL:
    return icall(it, f, n);

  default:
    return enlng_make_null();
  }
}

/* ------------------------------------------------------------ statements */

static int iexec_block(Interp *it, IFrame *f, ASTNode **body, int count) {
  int saved = f->scope;
  f->scope = f->n;
  int code = EX_OK;
  for (int i = 0; i < count && !it->failed; i++) {
    code = iexec(it, f, body[i]);
    if (code != EX_OK)
      break;
  }
  f->n = f->scope;
  f->scope = saved;
  return code;
}

/* One loop pass finished with `code`. Returns true if the loop must end,
 * and sets *out to what the loop statement itself returns. */
static bool iloop_done(int code, int *out) {
  if (code == EX_BREAK) {
    *out = EX_OK;
    return true;
  }
  if (code == EX_RETURN) {
    *out = EX_RETURN;
    return true;
  }
  return false;
}

static bool is_sorted_cond(ASTNode *c) {
  return c && c->type == AST_EXPR_VARIABLE &&
         strcmp(c->as.variable.name, "sorted") == 0;
}

static void iassign(Interp *it, IFrame *f, ASTNode *target, EnlngVal val) {
  if (target->type == AST_EXPR_VARIABLE) {
    IVar *v = iassign_var(it, f, target->as.variable.name, target->line);
    if (it->failed)
      return;
    if (v)
      v->val = val;
    else
      idefine(f, target->as.variable.name, val);
  } else if (target->type == AST_EXPR_INDEX) {
    int base = it->ntmp;
    EnlngVal c = target->as.index_expr.target
                     ? ieval(it, f, target->as.index_expr.target)
                     : iget(it, f, target->as.index_expr.arr_name, target->line);
    iroot(it, c);
    EnlngVal k = ieval(it, f, target->as.index_expr.index_expr);
    it->ntmp = base;
    enlng_container_set(c, k, val);
  } else {
    ifail(it, target->line, "I can't change that with 'swap'. Use names or list items like items[0].");
  }
}

/* Runs the body of a `try`. Gives 1 when the body failed, with the message in
 * `msg`: either an error the interpreter found (it->failed) or one thrown by
 * the runtime (frozen list, `fail` inside native-style code), which unwinds
 * with longjmp to here. */
static int itry_run(Interp *it, IFrame *f, ASTNode *n, int *code, char *msg,
                    size_t msgsz) {
  int base = enlng_try_stack.eg_depth;
  int saved_depth = it->depth;
  bool *saved_sorted = it->sorted;
  int saved_tmp = it->ntmp, saved_frames = it->nframes;
  if (base >= ENLNG_MAX_TRY) {
    ifail(it, n->line, "Too many 'try' blocks inside each other.");
    return 0;
  }
  enlng_try_stack.eg_depth = base + 1;
  if (setjmp(enlng_try_stack.eg_jb[base]) == 0) {
    *code = iexec_block(it, f, n->as.try_stmt.body, n->as.try_stmt.body_count);
    enlng_try_stack.eg_depth = base;
    if (it->failed) {
      snprintf(msg, msgsz, "%s", it->err);
      it->failed = false;
      it->ntmp = saved_tmp;
      it->nframes = saved_frames;
      return 1;
    }
    return 0;
  }
  enlng_try_stack.eg_depth = base;
  it->depth = saved_depth;
  it->sorted = saved_sorted;
  it->ntmp = saved_tmp;
  it->nframes = saved_frames;
  snprintf(msg, msgsz, "%s", enlng_try_stack.eg_msg);
  return 1;
}

static int iexec(Interp *it, IFrame *f, ASTNode *n) {
  if (!n || it->failed)
    return EX_OK;
  it->line = n->line;
  if (enlng_gc_due())
    igc(it);

  switch (n->type) {
  case AST_DECLARATION: {
    EnlngVal v = ieval(it, f, n->as.decl.init_expr);
    if (n->as.decl.is_frozen)
      v = enlng_freeze_val(v);
    idefine(f, n->as.decl.name, v);
    return EX_OK;
  }

  case AST_MUTATION: {
    EnlngVal v = ieval(it, f, n->as.mutation.val_expr);
    const char *name = n->as.mutation.name;
    if (n->as.mutation.op == ENLNG_TOKEN_INC_BY ||
        n->as.mutation.op == ENLNG_TOKEN_DEC_BY) {
      IVar *var = ifind_var(it, f, name);
      if (!var) {
        ifail(it, n->line, "I don't know a name '%s' here. Define it first, for example: remember %s as 0", name, name);
        return EX_OK;
      }
      var->val = n->as.mutation.op == ENLNG_TOKEN_INC_BY
                     ? enlng_val_add(var->val, v)
                     : enlng_val_sub(var->val, v);
    } else {
      /* x = v changes a local x, or the top-level x after `global x`;
         otherwise it makes a new local */
      IVar *var = iassign_var(it, f, name, n->line);
      if (it->failed)
        return EX_OK;
      if (var)
        var->val = v;
      else
        idefine(f, name, v);
    }
    return EX_OK;
  }

  case AST_INDEX_MUTATION: {
    int base = it->ntmp;
    EnlngVal c = n->as.index_mutation.target_expr
                     ? ieval(it, f, n->as.index_mutation.target_expr)
                     : iget(it, f, n->as.index_mutation.arr_name, n->line);
    iroot(it, c);
    EnlngVal k = ieval(it, f, n->as.index_mutation.index_expr);
    iroot(it, k);
    EnlngVal v = ieval(it, f, n->as.index_mutation.val_expr);
    it->ntmp = base;
    if (n->as.index_mutation.op == ENLNG_TOKEN_INC_BY)
      v = enlng_val_add(enlng_container_get(c, k), v);
    else if (n->as.index_mutation.op == ENLNG_TOKEN_DEC_BY)
      v = enlng_val_sub(enlng_container_get(c, k), v);
    if (!it->failed)
      enlng_container_set(c, k, v);
    return EX_OK;
  }

  case AST_GLOBAL:
    for (int i = 0; i < n->as.global_stmt.count; i++) {
      if (f->gln < 32)
        f->gl[f->gln] = n->as.global_stmt.names[i];
      f->gln++;
    }
    return EX_OK;

  case AST_FAIL: {
    EnlngVal m = ieval(it, f, n->as.fail_stmt.message);
    char buf[64];
    ifail(it, n->line, "%s", enlng_val_to_str_buf(m, buf, sizeof(buf)));
    return EX_OK;
  }

  case AST_TRY: {
    char msg[512];
    int code = EX_OK;
    if (itry_run(it, f, n, &code, msg, sizeof(msg))) {
      /* the handler runs in a scope of its own, with the message bound */
      int saved = f->scope;
      f->scope = f->n;
      idefine(f, n->as.try_stmt.err_name, enlng_make_string(msg));
      code = EX_OK;
      for (int i = 0; i < n->as.try_stmt.handler_count && !it->failed; i++) {
        code = iexec(it, f, n->as.try_stmt.handler[i]);
        if (code != EX_OK)
          break;
      }
      f->n = f->scope;
      f->scope = saved;
    }
    return code;
  }

  case AST_WHEN:
    if (enlng_is_truthy(ieval(it, f, n->as.when_stmt.condition)))
      return iexec_block(it, f, n->as.when_stmt.then_body, n->as.when_stmt.then_count);
    if (n->as.when_stmt.else_count > 0)
      return iexec_block(it, f, n->as.when_stmt.else_body, n->as.when_stmt.else_count);
    return EX_OK;

  case AST_FOR_RANGE: {
    int64_t start = enlng_val_to_int(ieval(it, f, n->as.for_range.start_expr));
    int64_t end = enlng_val_to_int(ieval(it, f, n->as.for_range.end_expr));
    int64_t step = n->as.for_range.step_expr
                       ? enlng_val_to_int(ieval(it, f, n->as.for_range.step_expr))
                       : 1;
    if (it->failed)
      return EX_OK;
    if (step == 0) {
      ifail(it, n->line, "This loop would never end because the step is 0. Use a step such as 1, or -1 to count down.");
      return EX_OK;
    }
    int out = EX_OK;
    for (int64_t i = start; (step > 0 ? i <= end : i >= end) && !it->failed; i += step) {
      int saved = f->scope;
      f->scope = f->n;
      idefine(f, n->as.for_range.var_name, enlng_make_int(i));
      int code = EX_OK;
      for (int b = 0; b < n->as.for_range.body_count && !it->failed; b++) {
        code = iexec(it, f, n->as.for_range.body[b]);
        if (code != EX_OK)
          break;
      }
      f->n = f->scope;
      f->scope = saved;
      if (iloop_done(code, &out))
        return out;
    }
    return EX_OK;
  }

  case AST_FOR_PAIR: {
    EnlngVal list = iget(it, f, n->as.for_pair.list_name, n->line);
    int64_t cnt = enlng_count_of(list);
    int out = EX_OK;
    for (int64_t i = 0; i < cnt - 1 && !it->failed; i++) {
      int saved = f->scope;
      f->scope = f->n;
      idefine(f, "pair_left", enlng_list_get(list, i));
      idefine(f, "pair_right", enlng_list_get(list, i + 1));
      /* `swap pair` needs the position; keep it in a hidden variable. */
      idefine(f, "\x01pair_i", enlng_make_int(i));
      idefine(f, "\x01pair_list", list);
      int code = EX_OK;
      for (int b = 0; b < n->as.for_pair.body_count && !it->failed; b++) {
        code = iexec(it, f, n->as.for_pair.body[b]);
        if (code != EX_OK)
          break;
      }
      f->n = f->scope;
      f->scope = saved;
      if (iloop_done(code, &out))
        return out;
    }
    return EX_OK;
  }

  case AST_FOR_EACH: {
    EnlngVal coll = enlng_each_source(
        n->as.for_each.list_expr ? ieval(it, f, n->as.for_each.list_expr)
                                 : iget(it, f, n->as.for_each.list_name, n->line));
    int64_t cnt = enlng_count_of(coll);
    int out = EX_OK;
    int base = it->ntmp;
    iroot(it, coll); /* the loop walks it, whoever else holds it */
    for (int64_t i = 0; i < cnt && !it->failed; i++) {
      int saved = f->scope;
      f->scope = f->n;
      idefine(f, n->as.for_each.item_name, enlng_list_get(coll, i));
      int code = EX_OK;
      for (int b = 0; b < n->as.for_each.body_count && !it->failed; b++) {
        code = iexec(it, f, n->as.for_each.body[b]);
        if (code != EX_OK)
          break;
      }
      f->n = f->scope;
      f->scope = saved;
      if (iloop_done(code, &out)) {
        it->ntmp = base;
        return out;
      }
    }
    it->ntmp = base;
    return EX_OK;
  }

  case AST_REPEAT_WHILE: {
    int out = EX_OK;
    while (!it->failed && enlng_is_truthy(ieval(it, f, n->as.repeat_loop.condition))) {
      int code = iexec_block(it, f, n->as.repeat_loop.body, n->as.repeat_loop.body_count);
      if (iloop_done(code, &out))
        return out;
    }
    return EX_OK;
  }

  case AST_REPEAT_UNTIL: {
    int out = EX_OK;
    if (is_sorted_cond(n->as.repeat_loop.condition)) {
      bool flag = false;
      bool *saved = it->sorted;
      it->sorted = &flag;
      while (!flag && !it->failed) {
        flag = true;
        int code = iexec_block(it, f, n->as.repeat_loop.body, n->as.repeat_loop.body_count);
        if (iloop_done(code, &out)) {
          it->sorted = saved;
          return out;
        }
      }
      it->sorted = saved;
      return EX_OK;
    }
    while (!it->failed && !enlng_is_truthy(ieval(it, f, n->as.repeat_loop.condition))) {
      int code = iexec_block(it, f, n->as.repeat_loop.body, n->as.repeat_loop.body_count);
      if (iloop_done(code, &out))
        return out;
    }
    return EX_OK;
  }

  case AST_REPEAT_TIMES: {
    int64_t lim = enlng_val_to_int(ieval(it, f, n->as.repeat_times.count_expr));
    int out = EX_OK;
    for (int64_t i = 0; i < lim && !it->failed; i++) {
      int code = iexec_block(it, f, n->as.repeat_times.body, n->as.repeat_times.body_count);
      if (iloop_done(code, &out))
        return out;
    }
    return EX_OK;
  }

  case AST_GIVE:
    it->ret = n->as.give_stmt.expr ? ieval(it, f, n->as.give_stmt.expr)
                                   : enlng_make_null();
    return EX_RETURN;

  case AST_STOP:
    return EX_BREAK;

  case AST_SKIP:
    return EX_CONTINUE;

  case AST_SWAP: {
    if (n->as.swap_stmt.is_pair) {
      IVar *pi = ilookup(f, "\x01pair_i");
      IVar *pl = ilookup(f, "pair_left");
      if (!pi || !pl) {
        ifail(it, n->line, "'swap pair' only works inside: for each pair in <list>:");
        return EX_OK;
      }
      IVar *lv = ilookup(f, "\x01pair_list");
      EnlngVal list = lv ? lv->val : enlng_make_null();
      enlng_list_swap(list, pi->val.as.i, pi->val.as.i + 1);
      if (it->sorted)
        *it->sorted = false;
      return EX_OK;
    }
    ASTNode *l = n->as.swap_stmt.left, *r = n->as.swap_stmt.right;
    if (l->type == AST_EXPR_INDEX && r->type == AST_EXPR_INDEX && l->as.index_expr.arr_name &&
        r->as.index_expr.arr_name && strcmp(l->as.index_expr.arr_name, r->as.index_expr.arr_name) == 0) {
      EnlngVal c = iget(it, f, l->as.index_expr.arr_name, n->line);
      int64_t a = enlng_val_to_int(ieval(it, f, l->as.index_expr.index_expr));
      int64_t b = enlng_val_to_int(ieval(it, f, r->as.index_expr.index_expr));
      enlng_list_swap(c, a, b);
      return EX_OK;
    }
    int base = it->ntmp;
    EnlngVal lv = ieval(it, f, l);
    iroot(it, lv);
    EnlngVal rv = ieval(it, f, r);
    iroot(it, rv);
    iassign(it, f, l, rv);
    iassign(it, f, r, lv);
    it->ntmp = base;
    return EX_OK;
  }

  case AST_REVERSE_STMT: {
    IVar *v = ilookup(f, n->as.reverse_stmt.target_name);
    if (!v) {
      ifail(it, n->line, "I don't know a name '%s' here.", n->as.reverse_stmt.target_name);
      return EX_OK;
    }
    v->val = enlng_val_reverse(v->val);
    return EX_OK;
  }

  case AST_ARRANGE_STMT: {
    EnlngVal idx = ieval(it, f, n->as.arrange_stmt.indices_expr);
    IVar *v = ilookup(f, n->as.arrange_stmt.target_name);
    if (!v) {
      ifail(it, n->line, "I don't know a name '%s' here.", n->as.arrange_stmt.target_name);
      return EX_OK;
    }
    v->val = enlng_val_arrange(v->val, idx);
    return EX_OK;
  }

  case AST_SORT: {
    IVar *v = ilookup(f, n->as.sort_stmt.target_name);
    if (!v) {
      ifail(it, n->line, "I don't know a name '%s' here.", n->as.sort_stmt.target_name);
      return EX_OK;
    }
    enlng_list_sort(v->val, n->as.sort_stmt.descending);
    return EX_OK;
  }

  case AST_SHOW:
    for (int i = 0; i < n->as.show_stmt.count; i++) {
      EnlngVal v = ieval(it, f, n->as.show_stmt.items[i]);
      if (it->failed)
        return EX_OK;
      enlng_print_val(v);
      if (i < n->as.show_stmt.count - 1)
        printf(" ");
    }
    printf("\n");
    return EX_OK;

  case AST_ASK:
    idefine(f, n->as.ask_stmt.var_name,
            enlng_ask(n->as.ask_stmt.prompt ? n->as.ask_stmt.prompt : ""));
    return EX_OK;

  case AST_EXPR_STMT:
    (void)ieval(it, f, n->as.expr_stmt.expr);
    return EX_OK;

  default: /* AST_USE is already merged by the parser; functions are collected up front */
    return EX_OK;
  }
}

/* ------------------------------------------------------------------ run */

static void interp_report(Interp *it) {
  fflush(stdout);
  fprintf(stderr, "\n-- Enlang RuntimeError --------------------------------------------------\n");
  fprintf(stderr, "Location: %s:%d\n\n", it->file, it->err_line > 0 ? it->err_line : 1);
  if (it->source && it->err_line > 0) {
    const char *p = it->source;
    int line = 1;
    while (*p && line < it->err_line) {
      if (*p == '\n')
        line++;
      p++;
    }
    const char *e = p;
    while (*e && *e != '\n' && *e != '\r')
      e++;
    fprintf(stderr, "  %3d | %.*s\n\n", it->err_line, (int)(e - p), p);
  }
  fprintf(stderr, "What: %s\n", it->err);
  fprintf(stderr, "------------------------------------------------------------------------\n\n");
}

typedef struct {
  ASTNode *prog;
  const char *file;
  const char *source;
  int rc;
} InterpJob;

/* The runtime calls a function value through this (serve calls the
 * program's function for each request). */
static ENLNG_TLS Interp *g_hook_interp = NULL;
static Interp *g_main_interp = NULL;

/* A line of run together, in its own thread with its own Interp. */
static EnlngVal interp_thread_call(EnlngVal f, char *err, size_t errsz, int *failed) {
  Interp t;
  memset(&t, 0, sizeof(t));
  Interp *m = g_main_interp;
  t.prog = m->prog;
  t.file = m->file;
  t.source = m->source;
  t.globals = m->globals;
  t.line = m->line;
  g_hook_interp = &t;
  ilive_add(&t);
  EnlngVal r = enlng_make_null();
  enlng_try_stack.eg_depth = 1;
  if (setjmp(enlng_try_stack.eg_jb[0]) == 0)
    r = icall_value(&t, f.as.fn, NULL, 0, t.line);
  else
    ifail(&t, t.line, "%s", enlng_try_stack.eg_msg);
  enlng_try_stack.eg_depth = 0;
  if (t.failed) {
    *failed = 1;
    snprintf(err, errsz, "%s", t.err);
    r = enlng_make_null();
  }
  ilive_remove(&t);
  free(t.frames);
  free(t.tmp);
  return r;
}
static EnlngVal interp_call_hook(EnlngVal f, int argc, EnlngVal *a) {
  Interp *it = g_hook_interp;
  EnlngVal r = icall_value(it, f.as.fn, a, argc, it->line);
  if (it->failed) {
    fflush(stdout);
    exit(1);
  }
  return r;
}

static int interp_main(InterpJob *job) {
  Interp it;
  memset(&it, 0, sizeof(it));
  g_hook_interp = &it;
  g_main_interp = &it;
  ilive_add(&it);
  enlng_call_hook = interp_call_hook;
  enlng_thread_call_hook = interp_thread_call;
  it.prog = job->prog;
  it.file = job->file;
  it.source = job->source;

  enlng_console_utf8();
  IFrame top = {0};
  it.globals = &top;
  ipush_frame(&it, &top);
  ASTNode *p = job->prog;
  /* An error the runtime throws outside any `try` (such as "a" minus 1)
   * comes back here, so it is reported with its line like any other. */
  enlng_try_stack.eg_depth = 1;
  if (setjmp(enlng_try_stack.eg_jb[0]) == 0) {
    for (int i = 0; i < p->as.program.count && !it.failed; i++) {
      ASTNode *s = p->as.program.statements[i];
      if (!s || s->type == AST_FUNCTION)
        continue;
      if (iexec(&it, &top, s) == EX_RETURN)
        break;
    }
  } else {
    ifail(&it, it.line, "%s", enlng_try_stack.eg_msg);
  }
  enlng_try_stack.eg_depth = 0;
  free(top.v);
  fflush(stdout);
  if (it.failed) {
    interp_report(&it);
    return 1;
  }
  return 0;
}

#ifdef _WIN32
static DWORD WINAPI interp_thread(LPVOID arg) {
  InterpJob *job = (InterpJob *)arg;
  job->rc = interp_main(job);
  return 0;
}
#else
static void *interp_thread(void *arg) {
  InterpJob *job = (InterpJob *)arg;
  job->rc = interp_main(job);
  return NULL;
}
#endif

/* Deeply recursive programs need more stack than the default, so the
 * interpreter runs on a thread with a large reserved stack. */
int enlng_interp_run(ASTNode *prog, const char *file, const char *source) {
  InterpJob job;
  job.prog = prog;
  job.file = file;
  job.source = source;
  job.rc = 0;
#ifdef _WIN32
  HANDLE h = CreateThread(NULL, INTERP_STACK_BYTES, interp_thread, &job,
                          STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
  if (!h)
    return interp_main(&job);
  WaitForSingleObject(h, INFINITE);
  CloseHandle(h);
#else
  pthread_attr_t attr;
  pthread_t th;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, INTERP_STACK_BYTES);
  if (pthread_create(&th, &attr, interp_thread, &job) != 0) {
    pthread_attr_destroy(&attr);
    return interp_main(&job);
  }
  pthread_attr_destroy(&attr);
  pthread_join(th, NULL);
#endif
  return job.rc;
}

/* ---------------------------------------------------------------------
 * Interactive mode: `enlng` with no file. Each input is added to the
 * program so far, which is read again; only the new lines run, and the
 * names made before keep their values.
 * ------------------------------------------------------------------ */
static char *repl_read_line(const char *prompt) {
  printf("%s", prompt);
  fflush(stdout);
  char buf[4096];
  if (!fgets(buf, sizeof(buf), stdin))
    return NULL;
  size_t n = strlen(buf);
  while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
    buf[--n] = '\0';
  return enlng_strdup(buf);
}

static int repl_main(void) {
  Interp it;
  memset(&it, 0, sizeof(it));
  g_hook_interp = &it;
  g_main_interp = &it;
  ilive_add(&it);
  enlng_call_hook = interp_call_hook;
  enlng_thread_call_hook = interp_thread_call;
  it.file = "input";
  enlng_console_utf8();
  IFrame top = {0};
  it.globals = &top;
  ipush_frame(&it, &top);

  size_t cap = 4096, len = 0;
  char *src = (char *)malloc(cap);
  len = (size_t)sprintf(src, "type enlng\n");
  int done = 0; /* statements of the program so far that have run */

  printf("Enlng %s interactive mode. Write a line and press Enter; "
         "'exit' leaves.\n", ENLNG_VERSION);
  for (;;) {
    char *line = repl_read_line("enlng> ");
    if (!line)
      break;
    const char *s = line;
    while (*s == ' ' || *s == '\t')
      s++;
    if (strcmp(s, "exit") == 0 || strcmp(s, "quit") == 0) {
      free(line);
      break;
    }
    if (!*s) {
      free(line);
      continue;
    }
    /* a line ending in ':' opens a block: read until an empty line */
    size_t ilen = strlen(line);
    char *input = (char *)malloc(ilen + 2);
    sprintf(input, "%s\n", line);
    if (ilen > 0 && line[ilen - 1] == ':') {
      for (;;) {
        char *more = repl_read_line("  ...  ");
        if (!more || !more[0]) {
          free(more);
          break;
        }
        size_t ml = strlen(more);
        input = (char *)realloc(input, strlen(input) + ml + 2);
        strcat(input, more);
        strcat(input, "\n");
        free(more);
      }
    }
    free(line);

    size_t need = len + strlen(input) + 1;
    char *cand = (char *)malloc(need);
    memcpy(cand, src, len);
    strcpy(cand + len, input);

    Lexer lx;
    lexer_init(&lx, cand);
    lexer_tokenize(&lx);
    Parser ps;
    parser_init(&ps, lx.tokens, lx.token_count);
    g_lift_counter = 0;
    g_hidden_n = 0;
    ASTNode *prog = parser_parse_program(&ps);
    if (ps.has_error || !prog) {
      printf("Problem: %s\n", ps.error_msg);
      free(cand);
      free(input);
      continue;
    }

    it.prog = prog;
    it.source = cand;
    it.failed = false;
    int idx = 0, ran = done;
    int total = 0;
    for (int i = 0; i < prog->as.program.count; i++)
      if (prog->as.program.statements[i] &&
          prog->as.program.statements[i]->type != AST_FUNCTION)
        total++;
    enlng_try_stack.eg_depth = 1;
    if (setjmp(enlng_try_stack.eg_jb[0]) == 0) {
      for (int i = 0; i < prog->as.program.count && !it.failed; i++) {
        ASTNode *st = prog->as.program.statements[i];
        if (!st || st->type == AST_FUNCTION)
          continue;
        if (idx++ < done)
          continue;
        /* a line that is only a value shows the value */
        if (idx == total && st->type == AST_EXPR_STMT) {
          it.line = st->line;
          EnlngVal v = ieval(&it, &top, st->as.expr_stmt.expr);
          if (!it.failed && v.type != ENLNG_VAL_NULL) {
            enlng_print_val(v);
            printf("\n");
          }
        } else {
          iexec(&it, &top, st);
        }
        ran = idx;
      }
    } else {
      ifail(&it, it.line, "%s", enlng_try_stack.eg_msg);
    }
    enlng_try_stack.eg_depth = 0;
    fflush(stdout);
    if (it.failed) {
      printf("Problem: %s\n", it.err);
      it.failed = false;
      free(cand);
    } else {
      done = ran > total ? total : total;
      if (need > cap) {
        while (need > cap)
          cap *= 2;
        src = (char *)realloc(src, cap);
      }
      memcpy(src, cand, need);
      len = need - 1;
    }
    free(input);
  }
  printf("\n");
  return 0;
}

#ifdef _WIN32
static DWORD WINAPI repl_thread(LPVOID arg) {
  *(int *)arg = repl_main();
  return 0;
}
#else
static void *repl_thread(void *arg) {
  *(int *)arg = repl_main();
  return NULL;
}
#endif

int enlng_repl(void) {
  int rc = 0;
#ifdef _WIN32
  HANDLE h = CreateThread(NULL, INTERP_STACK_BYTES, repl_thread, &rc,
                          STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
  if (!h)
    return repl_main();
  WaitForSingleObject(h, INFINITE);
  CloseHandle(h);
#else
  pthread_t th;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, INTERP_STACK_BYTES);
  if (pthread_create(&th, &attr, repl_thread, &rc) != 0) {
    pthread_attr_destroy(&attr);
    return repl_main();
  }
  pthread_attr_destroy(&attr);
  pthread_join(th, NULL);
#endif
  return rc;
}
