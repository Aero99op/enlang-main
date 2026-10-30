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
} IFrame;

typedef struct {
  ASTNode *prog;
  const char *file;
  const char *source;
  bool failed;
  char err[512];
  int err_line;
  int depth;
  bool *sorted; /* the innermost `repeat until sorted` flag */
  EnlngVal ret;
} Interp;

enum { EX_OK = 0, EX_BREAK, EX_CONTINUE, EX_RETURN };

static EnlngVal ieval(Interp *it, IFrame *f, ASTNode *n);
static EnlngVal ibinary(Interp *it, IFrame *f, ASTNode *n);
static int iexec(Interp *it, IFrame *f, ASTNode *n);

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
    if (strcmp(f->v[i].name, name) == 0)
      return &f->v[i];
  }
  return NULL;
}

/* New name in the current scope (or overwrite one declared in this scope). */
static void idefine(IFrame *f, const char *name, EnlngVal val) {
  for (int i = f->n - 1; i >= f->scope; i--) {
    if (strcmp(f->v[i].name, name) == 0) {
      f->v[i].val = val;
      return;
    }
  }
  if (f->n >= f->cap) {
    f->cap = f->cap ? f->cap * 2 : 16;
    f->v = (IVar *)realloc(f->v, sizeof(IVar) * f->cap);
  }
  f->v[f->n].name = name;
  f->v[f->n].val = val;
  f->n++;
}

static EnlngVal iget(Interp *it, IFrame *f, const char *name, int line) {
  IVar *v = ilookup(f, name);
  if (!v) {
    ifail(it, line,
          "I don't know a name '%s' here. Define it first, for example: remember %s as 0",
          name, name);
    return enlng_make_null();
  }
  return v->val;
}

/* ------------------------------------------------------------- built-ins */

typedef struct {
  const char *name;
  int arity;
} IBuiltin;

static const IBuiltin ibuiltins[] = {
    {"sqrt", 1},         {"pow", 2},         {"abs", 1},
    {"floor", 1},        {"ceil", 1},        {"round", 1},
    {"min", 2},          {"max", 2},         {"random_number", 2},
    {"read_file", 1},    {"write_file", 2},  {"append_file", 2},
    {"file_exists", 1},  {"time_now", 0},    {"sleep", 1},
    {"append", 2},       {"pop", 1},         {"keys", 1},
    {"values", 1},       {"has_key", 2},     {"len", 1},
    {"split", 2},        {"join", 2},        {"contains", 2},
    {"string_add", 3},   {"string_insert", 3}, {"string_remove_at", 2},
    {"string_remove", 2}, {"string_set_at", 3}, {"string_replace", 3},
    {NULL, 0}};

static const IBuiltin *ifind_builtin(const char *name) {
  for (int i = 0; ibuiltins[i].name; i++) {
    if (strcmp(ibuiltins[i].name, name) == 0)
      return &ibuiltins[i];
  }
  return NULL;
}

static EnlngVal icall_builtin(const char *name, EnlngVal *a) {
#define B0(nm) if (strcmp(name, #nm) == 0) return enlng_builtin_##nm();
#define B1(nm) if (strcmp(name, #nm) == 0) return enlng_builtin_##nm(a[0]);
#define B2(nm) if (strcmp(name, #nm) == 0) return enlng_builtin_##nm(a[0], a[1]);
#define B3(nm) if (strcmp(name, #nm) == 0) return enlng_builtin_##nm(a[0], a[1], a[2]);
  B1(sqrt) B2(pow) B1(abs) B1(floor) B1(ceil) B1(round) B2(min) B2(max)
  B2(random_number) B1(read_file) B2(write_file) B2(append_file)
  B1(file_exists) B0(time_now) B1(sleep) B2(append) B1(pop) B1(keys)
  B1(values) B2(has_key) B1(len) B2(split) B2(join) B2(contains)
  B3(string_add) B3(string_insert) B2(string_remove_at) B2(string_remove)
  B3(string_set_at) B3(string_replace)
#undef B0
#undef B1
#undef B2
#undef B3
  return enlng_make_null();
}

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

static EnlngVal icall(Interp *it, IFrame *f, ASTNode *n) {
  const char *name = n->as.call_expr.func_name;
  int argc = n->as.call_expr.arg_count;
  EnlngVal argbuf[8];
  EnlngVal *args = argc <= 8 ? argbuf : (EnlngVal *)malloc(sizeof(EnlngVal) * argc);
  EnlngVal result = enlng_make_null();

  for (int i = 0; i < argc; i++)
    args[i] = ieval(it, f, n->as.call_expr.args[i]);

  if (!it->failed) {
    ASTNode *fn = ifind_func(it, name);
    if (fn) {
      if (argc != fn->as.func_def.param_count) {
        ifail(it, n->line, "'%s' needs %d input%s but was given %d.", name,
              fn->as.func_def.param_count,
              fn->as.func_def.param_count == 1 ? "" : "s", argc);
      } else if (it->depth >= INTERP_MAX_DEPTH) {
        ifail(it, n->line,
              "'%s' called itself too many times (over %d levels deep). Check that it has a way to stop.",
              name, INTERP_MAX_DEPTH);
      } else {
        IFrame nf = {0};
        for (int p = 0; p < argc; p++)
          idefine(&nf, fn->as.func_def.params[p], args[p]);
        bool *saved_sorted = it->sorted;
        it->sorted = NULL;
        it->depth++;
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
        free(nf.v);
      }
    } else {
      const IBuiltin *b = ifind_builtin(name);
      if (!b) {
        ifail(it, n->line,
              "I don't know a function called '%s'. Check the spelling, or define it with: action %s(...):",
              name, name);
      } else if (argc != b->arity) {
        ifail(it, n->line, "'%s' needs %d input%s but was given %d.", name,
              b->arity, b->arity == 1 ? "" : "s", argc);
      } else {
        result = icall_builtin(name, args);
      }
    }
  }
  if (args != argbuf)
    free(args);
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
    EnlngVal a = ieval(it, f, n->as.binary_expr.left);
    EnlngVal b = ieval(it, f, n->as.binary_expr.right);
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
  case AST_EXPR_LITERAL_NULL:
    return enlng_make_null();
  case AST_EXPR_LITERAL_STRING:
    return enlng_make_string(n->as.literal.string_val ? n->as.literal.string_val : "");

  case AST_EXPR_VARIABLE:
    return iget(it, f, n->as.variable.name, n->line);

  case AST_EXPR_INDEX: {
    EnlngVal c = n->as.index_expr.target
                     ? ieval(it, f, n->as.index_expr.target)
                     : iget(it, f, n->as.index_expr.arr_name, n->line);
    EnlngVal k = ieval(it, f, n->as.index_expr.index_expr);
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
  case AST_EXPR_COUNT_IN: {
    EnlngVal t = ieval(it, f, n->as.count_in_expr.target);
    EnlngVal c = ieval(it, f, n->as.count_in_expr.container);
    return enlng_make_int(enlng_count_in(t, c));
  }
  case AST_EXPR_SPLIT: {
    EnlngVal t = ieval(it, f, n->as.count_in_expr.target);
    EnlngVal c = ieval(it, f, n->as.count_in_expr.container);
    return enlng_builtin_split(t, c);
  }
  case AST_EXPR_JOIN: {
    EnlngVal t = ieval(it, f, n->as.count_in_expr.target);
    EnlngVal c = ieval(it, f, n->as.count_in_expr.container);
    return enlng_builtin_join(t, c);
  }
  case AST_EXPR_FIND: {
    EnlngVal t = ieval(it, f, n->as.count_in_expr.target);
    EnlngVal c = ieval(it, f, n->as.count_in_expr.container);
    return enlng_builtin_find(t, c);
  }
  case AST_EXPR_REPLACE: {
    EnlngVal t = ieval(it, f, n->as.replace_expr.target);
    EnlngVal r = ieval(it, f, n->as.replace_expr.replacement);
    EnlngVal c = ieval(it, f, n->as.replace_expr.container);
    return enlng_builtin_replace(t, r, c);
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

  case AST_EXPR_LIST_LITERAL: {
    EnlngList *l = enlng_list_create();
    for (int i = 0; i < n->as.list_literal.count; i++)
      enlng_list_push(l, ieval(it, f, n->as.list_literal.elements[i]));
    return enlng_make_list(l);
  }

  case AST_EXPR_MAP_LITERAL: {
    EnlngVal m = enlng_make_map(enlng_map_create());
    for (int i = 0; i < n->as.map_literal.count; i++)
      enlng_map_set(m, n->as.map_literal.keys[i],
                    ieval(it, f, n->as.map_literal.values[i]));
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
    IVar *v = ilookup(f, target->as.variable.name);
    if (v)
      v->val = val;
    else
      idefine(f, target->as.variable.name, val);
  } else if (target->type == AST_EXPR_INDEX) {
    EnlngVal c = target->as.index_expr.target
                     ? ieval(it, f, target->as.index_expr.target)
                     : iget(it, f, target->as.index_expr.arr_name, target->line);
    enlng_container_set(c, ieval(it, f, target->as.index_expr.index_expr), val);
  } else {
    ifail(it, target->line, "I can't change that with 'swap'. Use names or list items like items[0].");
  }
}

static int iexec(Interp *it, IFrame *f, ASTNode *n) {
  if (!n || it->failed)
    return EX_OK;

  switch (n->type) {
  case AST_DECLARATION: {
    EnlngVal v = ieval(it, f, n->as.decl.init_expr);
    idefine(f, n->as.decl.name, v);
    return EX_OK;
  }

  case AST_MUTATION: {
    EnlngVal v = ieval(it, f, n->as.mutation.val_expr);
    const char *name = n->as.mutation.name;
    if (n->as.mutation.op == ENLNG_TOKEN_INC_BY ||
        n->as.mutation.op == ENLNG_TOKEN_DEC_BY) {
      IVar *var = ilookup(f, name);
      if (!var) {
        ifail(it, n->line, "I don't know a name '%s' here. Define it first, for example: remember %s as 0", name, name);
        return EX_OK;
      }
      var->val = n->as.mutation.op == ENLNG_TOKEN_INC_BY
                     ? enlng_val_add(var->val, v)
                     : enlng_val_sub(var->val, v);
    } else {
      IVar *var = ilookup(f, name);
      if (var)
        var->val = v;
      else
        idefine(f, name, v);
    }
    return EX_OK;
  }

  case AST_INDEX_MUTATION: {
    EnlngVal c = iget(it, f, n->as.index_mutation.arr_name, n->line);
    EnlngVal k = ieval(it, f, n->as.index_mutation.index_expr);
    EnlngVal v = ieval(it, f, n->as.index_mutation.val_expr);
    if (n->as.index_mutation.op == ENLNG_TOKEN_INC_BY)
      v = enlng_val_add(enlng_container_get(c, k), v);
    else if (n->as.index_mutation.op == ENLNG_TOKEN_DEC_BY)
      v = enlng_val_sub(enlng_container_get(c, k), v);
    if (!it->failed)
      enlng_container_set(c, k, v);
    return EX_OK;
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
    if (step <= 0 && start <= end) {
      ifail(it, n->line, "This loop would never end because the step is %lld. Use a step of 1 or more.", (long long)step);
      return EX_OK;
    }
    int out = EX_OK;
    for (int64_t i = start; i <= end && !it->failed; i += step) {
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
    EnlngVal coll = n->as.for_each.list_expr
                        ? ieval(it, f, n->as.for_each.list_expr)
                        : iget(it, f, n->as.for_each.list_name, n->line);
    int64_t cnt = enlng_count_of(coll);
    int out = EX_OK;
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
      if (iloop_done(code, &out))
        return out;
    }
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
    EnlngVal lv = ieval(it, f, l);
    EnlngVal rv = ieval(it, f, r);
    iassign(it, f, l, rv);
    iassign(it, f, r, lv);
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

static int interp_main(InterpJob *job) {
  Interp it;
  memset(&it, 0, sizeof(it));
  it.prog = job->prog;
  it.file = job->file;
  it.source = job->source;

  IFrame top = {0};
  ASTNode *p = job->prog;
  for (int i = 0; i < p->as.program.count && !it.failed; i++) {
    ASTNode *s = p->as.program.statements[i];
    if (!s || s->type == AST_FUNCTION)
      continue;
    if (iexec(&it, &top, s) == EX_RETURN)
      break;
  }
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
