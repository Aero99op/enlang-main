/*
 * =====================================================================
 *   ENLANG SOVEREIGN PROGRAMMING LANGUAGE COMPILER (Pure C99 Native)
 *   src/enlng_parser.c - Deterministic Recursive Descent Parser (CFG)
 * =====================================================================
 */

#include "enlng_parser.h"
#include <ctype.h>
#include <stdarg.h>

void parser_init(Parser *parser, Token *tokens, int count) {
  parser->tokens = tokens;
  parser->token_count = count;
  parser->cursor = 0;
  parser->has_error = false;
  parser->error_msg[0] = '\0';
  parser->error_line = 0;
}

#define peek parser_peek
#define advance parser_advance

static Token *parser_peek(Parser *p, int offset) {
  if (p->cursor + offset >= p->token_count) {
    return &p->tokens[p->token_count - 1]; /* EOF */
  }
  return &p->tokens[p->cursor + offset];
}

static Token *advance(Parser *p) {
  if (p->cursor < p->token_count) {
    return &p->tokens[p->cursor++];
  }
  return &p->tokens[p->token_count - 1];
}

static bool check(Parser *p, EnlngTokenType type) {
  return peek(p, 0)->type == type;
}

static bool match(Parser *p, EnlngTokenType type) {
  if (check(p, type)) {
    advance(p);
    return true;
  }
  return false;
}

static void set_error(Parser *p, const char *msg, int line) {
  if (!p->has_error) {
    p->has_error = true;
    p->error_line = line;
    snprintf(p->error_msg, sizeof(p->error_msg), "%s", msg);
  }
}

/* A word of the language (stop, times, slc, ...) where a name was expected.
 * The lexer gives these their own token types, so the plain message would
 * not say why an ordinary-looking word was refused. */
static void name_error(Parser *p, const char *msg, int line) {
  Token *t = peek(p, 0);
  if (t && t->text && isalpha((unsigned char)t->text[0]) &&
      t->type != ENLNG_TOKEN_IDENTIFIER &&
      t->type != ENLNG_TOKEN_STRING_LITERAL &&
      t->type != ENLNG_TOKEN_AS && /* "remember as 5": the name is missing */
      !strchr(t->text, ' ')) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "'%s' is a word of the language and cannot be used as a name. "
             "Pick another name, such as my_%s",
             t->text, t->text);
    set_error(p, buf, line);
    return;
  }
  /* plus, times, and, ... reach the parser as their symbols */
  if (t && (t->type == ENLNG_TOKEN_ADD || t->type == ENLNG_TOKEN_SUB ||
            t->type == ENLNG_TOKEN_MUL || t->type == ENLNG_TOKEN_DIV ||
            t->type == ENLNG_TOKEN_MOD || t->type == ENLNG_TOKEN_AND ||
            t->type == ENLNG_TOKEN_OR || t->type == ENLNG_TOKEN_NOT)) {
    set_error(p, "Words such as plus, minus, times, divided, mod, and, or "
                 "and not are words of the language and cannot be used as "
                 "a name. Pick another name", line);
    return;
  }
  set_error(p, msg, line);
}

static void skip_newlines(Parser *p) {
  while (check(p, ENLNG_TOKEN_NEWLINE)) {
    advance(p);
  }
}

static void skip_ignorable(Parser *p) {
  while (check(p, ENLNG_TOKEN_NEWLINE) || check(p, ENLNG_TOKEN_INDENT) ||
         check(p, ENLNG_TOKEN_DEDENT)) {
    advance(p);
  }
}

static bool has_token_on_line(Parser *p, EnlngTokenType type) {
  int i = 0;
  while (true) {
    Token *tok = peek(p, i);
    if (tok->type == ENLNG_TOKEN_NEWLINE || tok->type == ENLNG_TOKEN_EOF ||
        tok->type == ENLNG_TOKEN_DEDENT) {
      return false;
    }
    if (tok->type == type) {
      return true;
    }
    i++;
  }
}

static bool is_operator_or_closing(EnlngTokenType t) {
  return t == ENLNG_TOKEN_ADD || t == ENLNG_TOKEN_SUB || t == ENLNG_TOKEN_MUL ||
         t == ENLNG_TOKEN_DIV || t == ENLNG_TOKEN_MOD || t == ENLNG_TOKEN_EQ ||
         t == ENLNG_TOKEN_NEQ || t == ENLNG_TOKEN_GT || t == ENLNG_TOKEN_LT ||
         t == ENLNG_TOKEN_GTE || t == ENLNG_TOKEN_LTE || t == ENLNG_TOKEN_AND ||
         t == ENLNG_TOKEN_OR || t == ENLNG_TOKEN_RPAREN || t == ENLNG_TOKEN_RBRACKET ||
         t == ENLNG_TOKEN_RBRACE || t == ENLNG_TOKEN_COLON || t == ENLNG_TOKEN_COMMA ||
         t == ENLNG_TOKEN_NEWLINE || t == ENLNG_TOKEN_EOF || t == ENLNG_TOKEN_ASSIGN ||
         t == ENLNG_TOKEN_INC_BY || t == ENLNG_TOKEN_DEC_BY || t == ENLNG_TOKEN_DOT;
}

/* Forward declarations */
static ASTNode *parse_statement(Parser *p);
static ASTNode *ast_new(ASTNodeType type, int line);
static int mod_resolve(Parser *p, const char *alias, const char *member,
                       char *out, size_t outsz, int line);
static bool oop_starts_call(Parser *p);
static bool oop_starts_new(Parser *p);
static ASTNode *parse_oop_call(Parser *p);
static ASTNode *parse_oop_new(Parser *p);

/* ---------------------------------------------------------------------
 * Walking the tree
 * ------------------------------------------------------------------ */
typedef struct {
  char **v;
  int n, cap;
} NameSet;

static bool ns_has(const NameSet *s, const char *name) {
  if (!name)
    return false;
  for (int i = 0; i < s->n; i++)
    if (strcmp(s->v[i], name) == 0)
      return true;
  return false;
}

static void ns_add(NameSet *s, const char *name) {
  if (!name || ns_has(s, name))
    return;
  if (s->n >= s->cap) {
    s->cap = s->cap ? s->cap * 2 : 16;
    s->v = (char **)realloc(s->v, sizeof(char *) * s->cap);
  }
  s->v[s->n++] = (char *)name;
}

typedef void (*AstSlotFn)(ASTNode **slot, void *ctx);

static void ast_body_slots(ASTNode **body, int count, AstSlotFn fn, void *ctx) {
  for (int i = 0; i < count; i++)
    fn(&body[i], ctx);
}

/* Calls fn with the place of every node directly inside n, so a walker can
 * also replace a node. */
static void ast_each_child(ASTNode *n, AstSlotFn fn, void *ctx) {
  switch (n->type) {
  case AST_PROGRAM:
    ast_body_slots(n->as.program.statements, n->as.program.count, fn, ctx);
    break;
  case AST_DECLARATION:
    fn(&n->as.decl.init_expr, ctx);
    break;
  case AST_MUTATION:
    fn(&n->as.mutation.val_expr, ctx);
    break;
  case AST_WHEN:
    fn(&n->as.when_stmt.condition, ctx);
    ast_body_slots(n->as.when_stmt.then_body, n->as.when_stmt.then_count, fn, ctx);
    ast_body_slots(n->as.when_stmt.else_body, n->as.when_stmt.else_count, fn, ctx);
    break;
  case AST_TRY:
    ast_body_slots(n->as.try_stmt.body, n->as.try_stmt.body_count, fn, ctx);
    ast_body_slots(n->as.try_stmt.handler, n->as.try_stmt.handler_count, fn, ctx);
    break;
  case AST_FAIL:
    fn(&n->as.fail_stmt.message, ctx);
    break;
  case AST_FOR_RANGE:
    fn(&n->as.for_range.start_expr, ctx);
    fn(&n->as.for_range.end_expr, ctx);
    fn(&n->as.for_range.step_expr, ctx);
    ast_body_slots(n->as.for_range.body, n->as.for_range.body_count, fn, ctx);
    break;
  case AST_FOR_PAIR:
    ast_body_slots(n->as.for_pair.body, n->as.for_pair.body_count, fn, ctx);
    break;
  case AST_FOR_EACH:
    fn(&n->as.for_each.list_expr, ctx);
    ast_body_slots(n->as.for_each.body, n->as.for_each.body_count, fn, ctx);
    break;
  case AST_REPEAT_WHILE:
  case AST_REPEAT_UNTIL:
    fn(&n->as.repeat_loop.condition, ctx);
    ast_body_slots(n->as.repeat_loop.body, n->as.repeat_loop.body_count, fn, ctx);
    break;
  case AST_REPEAT_TIMES:
    fn(&n->as.repeat_times.count_expr, ctx);
    ast_body_slots(n->as.repeat_times.body, n->as.repeat_times.body_count, fn, ctx);
    break;
  case AST_FUNCTION:
    ast_body_slots(n->as.func_def.body, n->as.func_def.body_count, fn, ctx);
    break;
  case AST_GIVE:
    fn(&n->as.give_stmt.expr, ctx);
    break;
  case AST_SWAP:
    fn(&n->as.swap_stmt.left, ctx);
    fn(&n->as.swap_stmt.right, ctx);
    break;
  case AST_ARRANGE_STMT:
    fn(&n->as.arrange_stmt.indices_expr, ctx);
    break;
  case AST_INDEX_MUTATION:
    fn(&n->as.index_mutation.target_expr, ctx);
    fn(&n->as.index_mutation.index_expr, ctx);
    fn(&n->as.index_mutation.val_expr, ctx);
    break;
  case AST_SHOW:
    ast_body_slots(n->as.show_stmt.items, n->as.show_stmt.count, fn, ctx);
    break;
  case AST_EXPR_STMT:
    fn(&n->as.expr_stmt.expr, ctx);
    break;
  case AST_EXPR_BINARY:
    fn(&n->as.binary_expr.left, ctx);
    fn(&n->as.binary_expr.right, ctx);
    break;
  case AST_EXPR_UNARY:
    fn(&n->as.unary_expr.operand, ctx);
    break;
  case AST_EXPR_INDEX:
    fn(&n->as.index_expr.target, ctx);
    fn(&n->as.index_expr.index_expr, ctx);
    break;
  case AST_EXPR_FIELD:
    fn(&n->as.field_expr.target, ctx);
    break;
  case AST_EXPR_CALL:
    ast_body_slots(n->as.call_expr.args, n->as.call_expr.arg_count, fn, ctx);
    break;
  case AST_EXPR_COUNT_OF:
  case AST_EXPR_REVERSE:
  case AST_EXPR_TRIM:
  case AST_EXPR_TO_UPPER:
  case AST_EXPR_TO_LOWER:
  case AST_EXPR_SUM:
  case AST_EXPR_AVG:
  case AST_EXPR_MAX:
  case AST_EXPR_MIN:
    fn(&n->as.single_target_expr.target, ctx);
    break;
  case AST_EXPR_COUNT_IN:
  case AST_EXPR_SPLIT:
  case AST_EXPR_JOIN:
  case AST_EXPR_FIND:
    fn(&n->as.count_in_expr.target, ctx);
    fn(&n->as.count_in_expr.container, ctx);
    break;
  case AST_EXPR_REPLACE:
    fn(&n->as.replace_expr.target, ctx);
    fn(&n->as.replace_expr.replacement, ctx);
    fn(&n->as.replace_expr.container, ctx);
    break;
  case AST_EXPR_LIST_LITERAL:
    ast_body_slots(n->as.list_literal.elements, n->as.list_literal.count, fn, ctx);
    break;
  case AST_EXPR_MAP_LITERAL:
    ast_body_slots(n->as.map_literal.values, n->as.map_literal.count, fn, ctx);
    break;
  case AST_EXPR_LAMBDA:
    fn(&n->as.lambda_expr.fn, ctx);
    break;
  case AST_EXPR_TERNARY:
    fn(&n->as.ternary.cond, ctx);
    fn(&n->as.ternary.yes, ctx);
    fn(&n->as.ternary.no, ctx);
    break;
  default:
    break;
  }
}

/* The places in a node that hold a name (not the nodes below it). */
typedef void (*AstNameFn)(char **name, void *ctx);

static void ast_each_name(ASTNode *n, AstNameFn fn, void *ctx) {
  switch (n->type) {
  case AST_DECLARATION: fn(&n->as.decl.name, ctx); break;
  case AST_MUTATION: fn(&n->as.mutation.name, ctx); break;
  case AST_FOR_RANGE: fn(&n->as.for_range.var_name, ctx); break;
  case AST_FOR_PAIR:
    fn(&n->as.for_pair.pair_name, ctx);
    fn(&n->as.for_pair.list_name, ctx);
    break;
  case AST_FOR_EACH:
    fn(&n->as.for_each.item_name, ctx);
    fn(&n->as.for_each.list_name, ctx);
    break;
  case AST_TRY: fn(&n->as.try_stmt.err_name, ctx); break;
  case AST_SWAP: fn(&n->as.swap_stmt.pair_name, ctx); break;
  case AST_REVERSE_STMT: fn(&n->as.reverse_stmt.target_name, ctx); break;
  case AST_SORT: fn(&n->as.sort_stmt.target_name, ctx); break;
  case AST_ARRANGE_STMT: fn(&n->as.arrange_stmt.target_name, ctx); break;
  case AST_INDEX_MUTATION: fn(&n->as.index_mutation.arr_name, ctx); break;
  case AST_ASK: fn(&n->as.ask_stmt.var_name, ctx); break;
  case AST_EXPR_VARIABLE: fn(&n->as.variable.name, ctx); break;
  case AST_EXPR_INDEX: fn(&n->as.index_expr.arr_name, ctx); break;
  case AST_EXPR_FIELD: fn(&n->as.field_expr.obj_name, ctx); break;
  case AST_EXPR_CALL: fn(&n->as.call_expr.func_name, ctx); break;
  case AST_GLOBAL:
    for (int i = 0; i < n->as.global_stmt.count; i++)
      fn(&n->as.global_stmt.names[i], ctx);
    break;
  case AST_FUNCTION: fn(&n->as.func_def.name, ctx); break;
  case AST_EXPR_LAMBDA:
    for (int i = 0; i < n->as.lambda_expr.ncap; i++)
      fn(&n->as.lambda_expr.caps[i], ctx);
    break;
  default:
    break;
  }
}

static void ns_add_name(char **name, void *ctx) {
  if (*name)
    ns_add((NameSet *)ctx, *name);
}

/* Every name used anywhere inside a node, inner functions included. */
static void mention_slot(ASTNode **slot, void *ctx) {
  if (!*slot)
    return;
  ast_each_name(*slot, ns_add_name, ctx);
  ast_each_child(*slot, mention_slot, ctx);
}

/* Names a body gives a value to itself, not inside its inner functions. */
static void assigned_slot(ASTNode **slot, void *ctx) {
  ASTNode *n = *slot;
  NameSet *s = (NameSet *)ctx;
  if (!n)
    return;
  switch (n->type) {
  case AST_DECLARATION: ns_add(s, n->as.decl.name); break;
  case AST_MUTATION: ns_add(s, n->as.mutation.name); break;
  case AST_FOR_RANGE: ns_add(s, n->as.for_range.var_name); break;
  case AST_FOR_PAIR: ns_add(s, n->as.for_pair.pair_name); break;
  case AST_FOR_EACH: ns_add(s, n->as.for_each.item_name); break;
  case AST_TRY: ns_add(s, n->as.try_stmt.err_name); break;
  case AST_ASK: ns_add(s, n->as.ask_stmt.var_name); break;
  case AST_FUNCTION: ns_add(s, n->as.func_def.name); return;
  case AST_EXPR_LAMBDA: return;
  default: break;
  }
  ast_each_child(n, assigned_slot, ctx);
}

/* Names a body declares with `global`, not inside its inner functions. */
static void global_slot(ASTNode **slot, void *ctx) {
  ASTNode *n = *slot;
  if (!n || n->type == AST_FUNCTION || n->type == AST_EXPR_LAMBDA)
    return;
  if (n->type == AST_GLOBAL)
    for (int i = 0; i < n->as.global_stmt.count; i++)
      ns_add((NameSet *)ctx, n->as.global_stmt.names[i]);
  ast_each_child(n, global_slot, ctx);
}

/* ---------------------------------------------------------------------
 * Inner functions and `function with x: ...`
 *
 * Each one becomes a function at the top of the program with a hidden name.
 * The outer names it uses become extra inputs at the end, and the place
 * where it was written makes a function value that keeps their values at
 * that moment. `function inner with x:` inside a function also sets the
 * name inner to that value, and inner can call itself.
 * ------------------------------------------------------------------ */
typedef struct {
  const char *owner;
  NameSet *visible; /* outer names an inner function may keep */
} LiftCtx;

static ASTNode **g_lifted = NULL;
static int g_nlifted = 0, g_lift_counter = 0;

static void lift_slot(ASTNode **slot, void *ctx);

static void lift_body(ASTNode **body, int count, const char *owner, NameSet *visible) {
  LiftCtx c = {owner, visible};
  ast_body_slots(body, count, lift_slot, &c);
}

static void lift_one(ASTNode *lam, const char *self, LiftCtx *c) {
  ASTNode *fn = lam->as.lambda_expr.fn;
  NameSet own = {0}, used = {0}, globals = {0};
  for (int i = 0; i < fn->as.func_def.param_count; i++)
    ns_add(&own, fn->as.func_def.params[i]);
  if (self)
    ns_add(&own, self);
  ast_body_slots(fn->as.func_def.body, fn->as.func_def.body_count, mention_slot, &used);
  ast_body_slots(fn->as.func_def.body, fn->as.func_def.body_count, global_slot, &globals);

  char **caps = (char **)malloc(sizeof(char *) * (used.n + 1));
  int ncap = 0;
  for (int i = 0; i < used.n; i++)
    if (!ns_has(&own, used.v[i]) && !ns_has(&globals, used.v[i]) &&
        ns_has(c->visible, used.v[i]))
      caps[ncap++] = used.v[i];

  char hidden[300];
  snprintf(hidden, sizeof(hidden), "%s__%s_%d", c->owner, self ? self : "fn",
           ++g_lift_counter);
  fn->as.func_def.name = enlng_strdup(hidden);
  fn->as.func_def.shown = enlng_strdup(self ? self : "unnamed");
  int pc = fn->as.func_def.param_count;
  fn->as.func_def.params =
      (char **)realloc(fn->as.func_def.params, sizeof(char *) * (pc + ncap + 1));
  for (int i = 0; i < ncap; i++)
    fn->as.func_def.params[pc + i] = caps[i];
  fn->as.func_def.param_count = pc + ncap;
  fn->as.func_def.hidden_count = ncap;
  lam->as.lambda_expr.caps = caps;
  lam->as.lambda_expr.ncap = ncap;

  if (self) { /* inner = the function itself, so it can call itself */
    ASTNode *again = ast_new(AST_EXPR_LAMBDA, lam->line);
    again->as.lambda_expr.fn = fn;
    again->as.lambda_expr.caps = caps;
    again->as.lambda_expr.ncap = ncap;
    ASTNode *set = ast_new(AST_MUTATION, lam->line);
    set->as.mutation.name = (char *)self;
    set->as.mutation.op = ENLNG_TOKEN_ASSIGN;
    set->as.mutation.val_expr = again;
    int bc = fn->as.func_def.body_count;
    ASTNode **nb = (ASTNode **)malloc(sizeof(ASTNode *) * (bc + 1));
    nb[0] = set;
    for (int i = 0; i < bc; i++)
      nb[i + 1] = fn->as.func_def.body[i];
    fn->as.func_def.body = nb;
    fn->as.func_def.body_count = bc + 1;
  }

  NameSet inner = {0};
  for (int i = 0; i < fn->as.func_def.param_count; i++)
    ns_add(&inner, fn->as.func_def.params[i]);
  ast_body_slots(fn->as.func_def.body, fn->as.func_def.body_count, assigned_slot, &inner);
  lift_body(fn->as.func_def.body, fn->as.func_def.body_count, fn->as.func_def.name, &inner);

  g_lifted = (ASTNode **)realloc(g_lifted, sizeof(ASTNode *) * (g_nlifted + 1));
  g_lifted[g_nlifted++] = fn;
  free(own.v);
  free(used.v);
  free(globals.v);
}

static void lift_slot(ASTNode **slot, void *ctx) {
  ASTNode *n = *slot;
  LiftCtx *c = (LiftCtx *)ctx;
  if (!n)
    return;
  if (n->type == AST_FUNCTION) { /* an inner function: inner = its value */
    char *self = n->as.func_def.name;
    n->as.func_def.name = NULL;
    ASTNode *lam = ast_new(AST_EXPR_LAMBDA, n->line);
    lam->as.lambda_expr.fn = n;
    ASTNode *set = ast_new(AST_MUTATION, n->line);
    set->as.mutation.name = self;
    set->as.mutation.op = ENLNG_TOKEN_ASSIGN;
    set->as.mutation.val_expr = lam;
    *slot = set;
    lift_one(lam, self, c);
    return;
  }
  if (n->type == AST_EXPR_LAMBDA) {
    if (!n->as.lambda_expr.fn->as.func_def.name) /* not moved yet */
      lift_one(n, NULL, c);
    return;
  }
  ast_each_child(n, lift_slot, ctx);
}

static void lift_program(ASTNode *prog) {
  int count = prog->as.program.count;
  g_nlifted = 0;
  /* At the top of the program, names made with remember are seen by every
   * function anyway; the others (loop names and the like) are kept. */
  NameSet top = {0}, decls = {0};
  for (int i = 0; i < count; i++) {
    ASTNode *s = prog->as.program.statements[i];
    if (!s || s->type == AST_FUNCTION)
      continue;
    if (s->type == AST_DECLARATION)
      ns_add(&decls, s->as.decl.name);
    assigned_slot(&prog->as.program.statements[i], &top);
  }
  NameSet main_vis = {0};
  for (int i = 0; i < top.n; i++)
    if (!ns_has(&decls, top.v[i]))
      ns_add(&main_vis, top.v[i]);

  for (int i = 0; i < count; i++) {
    ASTNode *s = prog->as.program.statements[i];
    if (!s)
      continue;
    if (s->type == AST_FUNCTION) {
      NameSet vis = {0}, glob = {0}, all = {0};
      for (int k = 0; k < s->as.func_def.param_count; k++)
        ns_add(&all, s->as.func_def.params[k]);
      ast_body_slots(s->as.func_def.body, s->as.func_def.body_count, assigned_slot, &all);
      ast_body_slots(s->as.func_def.body, s->as.func_def.body_count, global_slot, &glob);
      for (int k = 0; k < all.n; k++)
        if (!ns_has(&glob, all.v[k]))
          ns_add(&vis, all.v[k]);
      lift_body(s->as.func_def.body, s->as.func_def.body_count, s->as.func_def.name, &vis);
    } else {
      LiftCtx c = {"main", &main_vis};
      lift_slot(&prog->as.program.statements[i], &c);
    }
  }
  for (int i = 0; i < g_nlifted; i++) {
    if (prog->as.program.count >= prog->as.program.capacity) {
      prog->as.program.capacity = prog->as.program.capacity * 2 + 1;
      prog->as.program.statements = (ASTNode **)realloc(
          prog->as.program.statements, sizeof(ASTNode *) * prog->as.program.capacity);
    }
    prog->as.program.statements[prog->as.program.count++] = g_lifted[i];
  }
  g_nlifted = 0;
}
/* ---------------------------------------------------------------------
 *  OOP: class / method / new / call / tell / ask
 *  A class is rewritten while parsing into ordinary functions and map
 *  expressions, so the interpreter and the native build need no changes.
 * --------------------------------------------------------------------- */
typedef struct {
  char *name;
  int parents[8];
  int nparents;
  char **fields;
  ASTNode **defaults;
  int nfields;
  char **methods;
  int *margc;
  int nmethods;
  int mro[32];
  int nmro;
} OopClass;

typedef struct {
  char *name;
  int argc;
  int line;
} OopDisp;

static OopClass *g_cls = NULL;
static int g_ncls = 0, g_capcls = 0;
static OopDisp *g_disp = NULL;
static int g_ndisp = 0, g_capdisp = 0;
static char **g_isnames = NULL;
static int *g_isline = NULL;
static int g_nis = 0, g_capis = 0;
static ASTNode **g_extra = NULL;
static int g_nextra = 0, g_capextra = 0;
static int g_parse_depth = 0;
static int g_block_depth = 0; /* how many blocks deep the parser is */
static ASTNode **g_gen = NULL;
static int g_ngen = 0, g_capgen = 0;

static void oop_reset(void) {
  g_block_depth = 0;
  g_ncls = 0;
  g_ndisp = 0;
  g_nis = 0;
  g_nextra = 0;
  g_ngen = 0;
}

static int oop_find_class(const char *name) {
  for (int i = 0; i < g_ncls; i++)
    if (strcmp(g_cls[i].name, name) == 0)
      return i;
  return -1;
}

static bool oop_is_ident(Token *t, const char *word) {
  return t->type == ENLNG_TOKEN_IDENTIFIER && t->text &&
         strcmp(t->text, word) == 0;
}

static bool oop_build_mro(int self_idx) {
  OopClass *c = &g_cls[self_idx];
  int seq[9][40], len[9], ns = 0;
  for (int i = 0; i < c->nparents; i++) {
    OopClass *pc = &g_cls[c->parents[i]];
    memcpy(seq[ns], pc->mro, sizeof(int) * pc->nmro);
    len[ns++] = pc->nmro;
  }
  if (c->nparents > 0) {
    memcpy(seq[ns], c->parents, sizeof(int) * c->nparents);
    len[ns++] = c->nparents;
  }
  int n = 0;
  c->mro[n++] = self_idx;
  for (;;) {
    bool any = false;
    int cand = -1;
    for (int s = 0; s < ns && cand < 0; s++) {
      if (len[s] == 0)
        continue;
      any = true;
      int h = seq[s][0];
      bool bad = false;
      for (int t = 0; t < ns && !bad; t++)
        for (int k = 1; k < len[t]; k++)
          if (seq[t][k] == h) {
            bad = true;
            break;
          }
      if (!bad)
        cand = h;
    }
    if (!any)
      break;
    if (cand < 0 || n >= 32)
      return false;
    c->mro[n++] = cand;
    for (int s = 0; s < ns; s++)
      if (len[s] > 0 && seq[s][0] == cand) {
        memmove(seq[s], seq[s] + 1, sizeof(int) * (len[s] - 1));
        len[s]--;
      }
  }
  c->nmro = n;
  return true;
}

static bool oop_find_method(int ci, const char *m, int *argc, int *owner) {
  OopClass *c = &g_cls[ci];
  for (int k = 0; k < c->nmro; k++) {
    OopClass *o = &g_cls[c->mro[k]];
    for (int j = 0; j < o->nmethods; j++)
      if (strcmp(o->methods[j], m) == 0) {
        *argc = o->margc[j];
        *owner = c->mro[k];
        return true;
      }
  }
  return false;
}

static void oop_queue_extra(ASTNode *n) {
  if (g_nextra >= g_capextra) {
    g_capextra = g_capextra ? g_capextra * 2 : 16;
    g_extra = (ASTNode **)realloc(g_extra, sizeof(ASTNode *) * g_capextra);
  }
  g_extra[g_nextra++] = n;
}

static ASTNode *oop_string_node(const char *s, int line) {
  ASTNode *n = ast_new(AST_EXPR_LITERAL_STRING, line);
  n->as.literal.string_val = enlng_strdup(s);
  return n;
}

static ASTNode *oop_call_node(const char *fname, ASTNode **args, int argc,
                              int line) {
  ASTNode *n = ast_new(AST_EXPR_CALL, line);
  n->as.call_expr.func_name = enlng_strdup(fname);
  n->as.call_expr.args = args;
  n->as.call_expr.arg_count = argc;
  return n;
}

/* ---------------------------------------------------------------------
 * Small builders for the sentences that are written in terms of others
 * ------------------------------------------------------------------ */
static ASTNode *call1(const char *fname, ASTNode *a, int line) {
  ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *));
  args[0] = a;
  return oop_call_node(fname, args, 1, line);
}

static ASTNode *call2(const char *fname, ASTNode *a, ASTNode *b, int line) {
  ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *) * 2);
  args[0] = a;
  args[1] = b;
  return oop_call_node(fname, args, 2, line);
}

static ASTNode *call3(const char *fname, ASTNode *a, ASTNode *b, ASTNode *c,
                      int line) {
  ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *) * 3);
  args[0] = a;
  args[1] = b;
  args[2] = c;
  return oop_call_node(fname, args, 3, line);
}

static ASTNode *var_node(const char *name, int line) {
  ASTNode *n = ast_new(AST_EXPR_VARIABLE, line);
  n->as.variable.name = enlng_strdup(name);
  return n;
}

static ASTNode *str_node(const char *s, int line) {
  ASTNode *n = ast_new(AST_EXPR_LITERAL_STRING, line);
  n->as.literal.string_val = enlng_strdup(s);
  return n;
}

static ASTNode *int_node(int64_t v, int line) {
  ASTNode *n = ast_new(AST_EXPR_LITERAL_INT, line);
  n->as.literal.int_val = v;
  return n;
}

static ASTNode *bool_node(bool v, int line) {
  ASTNode *n = ast_new(AST_EXPR_LITERAL_BOOL, line);
  n->as.literal.bool_val = v;
  return n;
}

static ASTNode *binary_node(EnlngTokenType op, ASTNode *l, ASTNode *r, int line) {
  ASTNode *n = ast_new(AST_EXPR_BINARY, line);
  n->as.binary_expr.op = op;
  n->as.binary_expr.left = l;
  n->as.binary_expr.right = r;
  return n;
}

static ASTNode *not_node(ASTNode *e, int line) {
  ASTNode *n = ast_new(AST_EXPR_UNARY, line);
  n->as.unary_expr.op = ENLNG_TOKEN_NOT;
  n->as.unary_expr.operand = e;
  return n;
}

static ASTNode *decl_node(const char *name, ASTNode *init, bool frozen, int line) {
  ASTNode *n = ast_new(AST_DECLARATION, line);
  n->as.decl.name = enlng_strdup(name);
  n->as.decl.init_expr = init;
  n->as.decl.is_frozen = frozen;
  return n;
}

static ASTNode *expr_stmt_node(ASTNode *e, int line) {
  ASTNode *n = ast_new(AST_EXPR_STMT, line);
  n->as.expr_stmt.expr = e;
  return n;
}

/* function with param: body -- for list building */
static ASTNode *lambda1(const char *param, ASTNode *body, int line) {
  ASTNode *fn = ast_new(AST_FUNCTION, line);
  ASTNode *give = ast_new(AST_GIVE, line);
  give->as.give_stmt.expr = body;
  fn->as.func_def.body = (ASTNode **)malloc(sizeof(ASTNode *));
  fn->as.func_def.body[0] = give;
  fn->as.func_def.body_count = 1;
  fn->as.func_def.params = (char **)malloc(sizeof(char *));
  fn->as.func_def.params[0] = enlng_strdup(param);
  fn->as.func_def.param_count = 1;
  ASTNode *lam = ast_new(AST_EXPR_LAMBDA, line);
  lam->as.lambda_expr.fn = fn;
  return lam;
}

/* A hidden name for a value a sentence needs to keep for a moment. */
static int g_hidden_n = 0;
static char *hidden_name(const char *what) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%s__%d", what, ++g_hidden_n);
  return enlng_strdup(buf);
}

static bool tok_is(Token *t, const char *word) {
  return t && t->type == ENLNG_TOKEN_IDENTIFIER && t->text && strcmp(t->text, word) == 0;
}

static ASTNode *parse_expression(Parser *p);

/* The call behind x[a:b] and slc(x, a, b). It always reaches the built-in,
 * even when a program defines a function named @slc. */
static ASTNode *slc_call(ASTNode **args, int line) {
  ASTNode *n = oop_call_node("slc", args, 3, line);
  n->as.call_expr.builtin_only = true;
  return n;
}

/* After '[' has been read: either an index, x[i], or a slice, x[a:b],
 * x[a:] or x[:b]. A slice becomes a call to the slc built-in, with null
 * for a bound that was left out. */
static ASTNode *parse_index_or_slice(Parser *p, ASTNode *target,
                                     const char *arr_name, int line) {
  ASTNode *start = NULL;
  if (!check(p, ENLNG_TOKEN_COLON))
    start = parse_expression(p);
  if (match(p, ENLNG_TOKEN_COLON)) {
    ASTNode *end = NULL;
    if (!check(p, ENLNG_TOKEN_RBRACKET))
      end = parse_expression(p);
    if (!match(p, ENLNG_TOKEN_RBRACKET))
      set_error(p, "Expected ']' after a slice such as items[1:3]", line);
    if (!target) {
      target = ast_new(AST_EXPR_VARIABLE, line);
      target->as.variable.name = enlng_strdup(arr_name);
    }
    ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *) * 3);
    args[0] = target;
    args[1] = start ? start : ast_new(AST_EXPR_LITERAL_NULL, line);
    args[2] = end ? end : ast_new(AST_EXPR_LITERAL_NULL, line);
    return slc_call(args, line);
  }
  if (!match(p, ENLNG_TOKEN_RBRACKET))
    set_error(p, "Expected ']' after array index", line);
  ASTNode *n = ast_new(AST_EXPR_INDEX, line);
  n->as.index_expr.target = target;
  n->as.index_expr.arr_name = arr_name ? enlng_strdup(arr_name) : NULL;
  n->as.index_expr.index_expr = start;
  return n;
}

static void oop_note_dispatch(const char *m, int argc, int line) {
  for (int i = 0; i < g_ndisp; i++)
    if (g_disp[i].argc == argc && strcmp(g_disp[i].name, m) == 0)
      return;
  if (g_ndisp >= g_capdisp) {
    g_capdisp = g_capdisp ? g_capdisp * 2 : 16;
    g_disp = (OopDisp *)realloc(g_disp, sizeof(OopDisp) * g_capdisp);
  }
  g_disp[g_ndisp].name = enlng_strdup(m);
  g_disp[g_ndisp].argc = argc;
  g_disp[g_ndisp].line = line;
  g_ndisp++;
}

static void oop_note_is(const char *cls, int line) {
  for (int i = 0; i < g_nis; i++)
    if (strcmp(g_isnames[i], cls) == 0)
      return;
  if (g_nis >= g_capis) {
    g_capis = g_capis ? g_capis * 2 : 8;
    g_isnames = (char **)realloc(g_isnames, sizeof(char *) * g_capis);
    g_isline = (int *)realloc(g_isline, sizeof(int) * g_capis);
  }
  g_isnames[g_nis] = enlng_strdup(cls);
  g_isline[g_nis] = line;
  g_nis++;
}

static ASTNode *parse_additive(Parser *p);
static ASTNode *parse_primary_internal(Parser *p, bool allow_with);
static void parse_block(Parser *p, ASTNode ***out_stmts, int *out_count);
static ASTNode *parse_expression(Parser *p);
static ASTNode *parse_expression(Parser *p);
static ASTNode *parse_additive(Parser *p);
static ASTNode *parse_unary(Parser *p);

static ASTNode *ast_new(ASTNodeType type, int line) {
  ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
  node->type = type;
  node->line = line;
  return node;
}

/* Expression parsing with Precedence */
/* The inputs of a function: "with a, b as 5" or "(a, b = 5)". An input may
 * have a default, which is used when the call leaves it out (or gives null).
 * Inputs with a default come last. */
typedef struct {
  char **names;
  ASTNode **defaults;
  int count;
  bool has_defaults;
  int min_count;
} ParamList;

static void parse_param_list(Parser *p, ParamList *pl, bool paren, int line) {
  int cap = 4;
  pl->names = (char **)malloc(sizeof(char *) * cap);
  pl->defaults = (ASTNode **)calloc((size_t)cap, sizeof(ASTNode *));
  pl->count = 0;
  pl->has_defaults = false;
  pl->min_count = 0;
  if (paren && check(p, ENLNG_TOKEN_RPAREN))
    return;
  do {
    Token *pt = peek(p, 0);
    if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
      name_error(p, paren ? "Expected a name for an input of the function"
                          : "Expected a parameter name after 'with'",
                 line);
      return;
    }
    if (pl->count >= cap) {
      cap *= 2;
      pl->names = (char **)realloc(pl->names, sizeof(char *) * cap);
      pl->defaults = (ASTNode **)realloc(pl->defaults, sizeof(ASTNode *) * cap);
    }
    pl->names[pl->count] = enlng_strdup(pt->text);
    pl->defaults[pl->count] = NULL;
    if (match(p, ENLNG_TOKEN_AS) || match(p, ENLNG_TOKEN_ASSIGN)) {
      pl->defaults[pl->count] = parse_expression(p);
      pl->has_defaults = true;
    } else if (pl->has_defaults) {
      set_error(p, "Inputs with a default value must come last. Move this one "
                   "before the inputs that have a default.", line);
      return;
    } else {
      pl->min_count = pl->count + 1;
    }
    pl->count++;
  } while (match(p, ENLNG_TOKEN_COMMA));
}

/* Gives the function the inputs, and at the top of its body puts
 * "when name is null: name = default" for each input with a default. */
static void apply_params(ASTNode *fn, ParamList *pl) {
  fn->as.func_def.params = pl->names;
  fn->as.func_def.param_count = pl->count;
  fn->as.func_def.has_defaults = pl->has_defaults;
  fn->as.func_def.min_count = pl->min_count;
  if (!pl->has_defaults)
    return;
  int nd = pl->count - pl->min_count;
  int bc = fn->as.func_def.body_count;
  ASTNode **nb = (ASTNode **)malloc(sizeof(ASTNode *) * (size_t)(bc + nd));
  for (int k = 0; k < nd; k++) {
    int i = pl->min_count + k;
    int line = pl->defaults[i]->line;
    ASTNode *var = ast_new(AST_EXPR_VARIABLE, line);
    var->as.variable.name = enlng_strdup(pl->names[i]);
    ASTNode *cond = ast_new(AST_EXPR_BINARY, line);
    cond->as.binary_expr.op = ENLNG_TOKEN_EQ;
    cond->as.binary_expr.left = var;
    cond->as.binary_expr.right = ast_new(AST_EXPR_LITERAL_NULL, line);
    ASTNode *set = ast_new(AST_MUTATION, line);
    set->as.mutation.name = enlng_strdup(pl->names[i]);
    set->as.mutation.op = ENLNG_TOKEN_ASSIGN;
    set->as.mutation.val_expr = pl->defaults[i];
    ASTNode *w = ast_new(AST_WHEN, line);
    w->as.when_stmt.condition = cond;
    w->as.when_stmt.then_body = (ASTNode **)malloc(sizeof(ASTNode *));
    w->as.when_stmt.then_body[0] = set;
    w->as.when_stmt.then_count = 1;
    nb[k] = w;
  }
  for (int i = 0; i < bc; i++)
    nb[nd + i] = fn->as.func_def.body[i];
  fn->as.func_def.body = nb;
  fn->as.func_def.body_count = bc + nd;
}

static ASTNode *parse_atom(Parser *p, bool allow_with);
static ASTNode *parse_or_expr(Parser *p);
static ASTNode *parse_list_source(Parser *p, const char *var, int line);
static ASTNode *parse_primary_internal(Parser *p, bool allow_with);
static inline ASTNode *parse_primary(Parser *p) {
  return parse_primary_internal(p, true);
}

/* One input of a call. `width as 3` (or `width = 3` in brackets) gives the
 * input by name; names[i] is set then. */
static ASTNode *parse_call_arg(Parser *p, char ***names, int i, int cap,
                               bool paren) {
  if (peek(p, 0)->type == ENLNG_TOKEN_IDENTIFIER &&
      (peek(p, 1)->type == ENLNG_TOKEN_AS ||
       (paren && peek(p, 1)->type == ENLNG_TOKEN_ASSIGN))) {
    if (!*names)
      *names = (char **)calloc((size_t)cap, sizeof(char *));
    (*names)[i] = enlng_strdup(advance(p)->text);
    advance(p);
  }
  return paren ? parse_expression(p) : parse_additive(p);
}

static ASTNode *parse_atom(Parser *p, bool allow_with) {
  Token *t = peek(p, 0);

  /* every n in numbers where n > 10 */
  if (tok_is(t, "every") && peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER &&
      peek(p, 2)->type == ENLNG_TOKEN_IN) {
    advance(p);
    const char *var = advance(p)->text;
    return call1("_keep_all", parse_list_source(p, var, t->line), t->line);
  }
  /* read bytes from "photo.png" */
  if (tok_is(t, "read") && tok_is(peek(p, 1), "bytes") &&
      peek(p, 2)->type == ENLNG_TOKEN_FROM) {
    advance(p);
    advance(p);
    advance(p);
    return call1("read_bytes", parse_additive(p), t->line);
  }
  /* listen on port 8080 */
  if (tok_is(t, "listen") && peek(p, 1)->type == ENLNG_TOKEN_ON &&
      tok_is(peek(p, 2), "port")) {
    advance(p);
    advance(p);
    advance(p);
    return call1("net_listen", parse_additive(p), t->line);
  }
  /* accept from the server / receive from the client */
  if ((tok_is(t, "accept") || tok_is(t, "receive")) &&
      peek(p, 1)->type == ENLNG_TOKEN_FROM) {
    advance(p);
    advance(p);
    return call1(t->text[0] == 'a' ? "net_accept" : "net_receive",
                 parse_additive(p), t->line);
  }
  /* connect to "example.com" at port 80 */
  if (tok_is(t, "connect") && peek(p, 1)->type == ENLNG_TOKEN_TO) {
    advance(p);
    advance(p);
    ASTNode *host;
    Token *h = peek(p, 0);
    if (h->type == ENLNG_TOKEN_STRING_LITERAL) {
      advance(p);
      host = str_node(h->text, h->line);
    } else if (h->type == ENLNG_TOKEN_IDENTIFIER) {
      advance(p);
      host = var_node(h->text, h->line);
    } else {
      host = parse_unary(p);
    }
    if (!match(p, ENLNG_TOKEN_AT) || !tok_is(peek(p, 0), "port")) {
      set_error(p, "Expected 'at port' and a number, as in: connect to \"example.com\" at port 80",
                t->line);
      return ast_new(AST_EXPR_LITERAL_NULL, t->line);
    }
    advance(p);
    return call2("net_connect", host, parse_additive(p), t->line);
  }

  if (oop_starts_call(p))
    return parse_oop_call(p);
  if (oop_starts_new(p))
    return parse_oop_new(p);

  /* function with x, y: x plus y -- a function made where it is used. With
   * nothing after the colon, the steps follow on indented lines. */
  if (match(p, ENLNG_TOKEN_FUNCTION)) {
    ParamList pl = {0};
    bool paren = false;
    if (match(p, ENLNG_TOKEN_WITH) || (paren = match(p, ENLNG_TOKEN_LPAREN))) {
      parse_param_list(p, &pl, paren, t->line);
      if (paren && !match(p, ENLNG_TOKEN_RPAREN))
        set_error(p, "Expected ')' after the inputs of the function", t->line);
    }
    ASTNode *fn = ast_new(AST_FUNCTION, t->line);
    if (check(p, ENLNG_TOKEN_COLON) && peek(p, 1)->type == ENLNG_TOKEN_NEWLINE) {
      parse_block(p, &fn->as.func_def.body, &fn->as.func_def.body_count);
    } else {
      if (!match(p, ENLNG_TOKEN_COLON)) {
        set_error(p,
                  "Expected ':' and then what the function gives, as in: "
                  "function with x: x times 2",
                  t->line);
        return ast_new(AST_EXPR_LITERAL_NULL, t->line);
      }
      ASTNode *give = ast_new(AST_GIVE, t->line);
      give->as.give_stmt.expr = parse_expression(p);
      fn->as.func_def.body = (ASTNode **)malloc(sizeof(ASTNode *));
      fn->as.func_def.body[0] = give;
      fn->as.func_def.body_count = 1;
    }
    apply_params(fn, &pl);
    ASTNode *lam = ast_new(AST_EXPR_LAMBDA, t->line);
    lam->as.lambda_expr.fn = fn;
    return lam;
  }

  /* slc(items, a) or slc(items, a, b): the same as items[a:] and items[a:b] */
  if (match(p, ENLNG_TOKEN_SLC)) {
    if (!match(p, ENLNG_TOKEN_LPAREN)) {
      set_error(p, "'slc' is a word of the language and cannot be used as a "
                   "name. Use it as slc(items, 1, 3) to take a part of a "
                   "list or text", t->line);
      return ast_new(AST_EXPR_LITERAL_NULL, t->line);
    }
    ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *) * 3);
    int argc = 0;
    if (!check(p, ENLNG_TOKEN_RPAREN)) {
      do {
        ASTNode *a = parse_expression(p);
        if (argc < 3)
          args[argc] = a;
        argc++;
      } while (match(p, ENLNG_TOKEN_COMMA));
    }
    if (!match(p, ENLNG_TOKEN_RPAREN))
      set_error(p, "Expected ')' after the inputs of slc", t->line);
    if (argc < 2 || argc > 3)
      set_error(p, "slc needs 2 or 3 inputs: slc(items, start) or "
                   "slc(items, start, end)", t->line);
    if (argc == 2)
      args[argc++] = ast_new(AST_EXPR_LITERAL_NULL, t->line);
    return slc_call(args, t->line);
  }

  /* Int Literal */
  if (match(p, ENLNG_TOKEN_INT_LITERAL)) {
    ASTNode *n = ast_new(AST_EXPR_LITERAL_INT, t->line);
    n->as.literal.int_val = t->int_val;
    /* 9223372036854775807 is the largest whole number */
    if (t->text && (strlen(t->text) > 19 ||
                    (strlen(t->text) == 19 &&
                     strcmp(t->text, "9223372036854775807") > 0))) {
      char buf[300];
      snprintf(buf, sizeof(buf),
               "The number %s is too big for a whole number (the largest is "
               "9223372036854775807). Write it as a decimal, such as %s.0, "
               "if an approximate value is enough.", t->text, t->text);
      set_error(p, buf, t->line);
    }
    return n;
  }

  /* Float Literal */
  if (match(p, ENLNG_TOKEN_FLOAT_LITERAL)) {
    ASTNode *n = ast_new(AST_EXPR_LITERAL_FLOAT, t->line);
    n->as.literal.float_val = t->float_val;
    return n;
  }

  /* String Literal */
  if (match(p, ENLNG_TOKEN_STRING_LITERAL)) {
    char *str = enlng_strdup(t->text);
    if (match(p, ENLNG_TOKEN_OF)) {
      ASTNode *tgt = parse_primary_internal(p, allow_with);
      ASTNode *n = ast_new(AST_EXPR_FIELD, t->line);
      n->as.field_expr.target = tgt;
      n->as.field_expr.obj_name =
          (tgt && tgt->type == AST_EXPR_VARIABLE)
              ? enlng_strdup(tgt->as.variable.name)
              : NULL;
      n->as.field_expr.field_name = str;
      return n;
    }
    ASTNode *n = ast_new(AST_EXPR_LITERAL_STRING, t->line);
    n->as.literal.string_val = str;
    return n;
  }

  /* Bool Literal */
  if (match(p, ENLNG_TOKEN_BOOL_LITERAL)) {
    ASTNode *n = ast_new(AST_EXPR_LITERAL_BOOL, t->line);
    n->as.literal.bool_val = (t->int_val != 0);
    return n;
  }

  /* Null Literal */
  if (match(p, ENLNG_TOKEN_NULL)) {
    return ast_new(AST_EXPR_LITERAL_NULL, t->line);
  }

  /* Parentheses: (expr) */
  if (match(p, ENLNG_TOKEN_LPAREN)) {
    ASTNode *n = parse_expression(p);
    if (!match(p, ENLNG_TOKEN_RPAREN)) {
      set_error(p, "Expected ')' after parenthesized expression", t->line);
    }
    return n;
  }

  /* List Literal: [1, 2, 3] */
  if (match(p, ENLNG_TOKEN_LBRACKET)) {
    ASTNode *n = ast_new(AST_EXPR_LIST_LITERAL, t->line);
    int cap = 8;
    int count = 0;
    ASTNode **elems = (ASTNode **)malloc(sizeof(ASTNode *) * cap);

    skip_ignorable(p);
    if (!check(p, ENLNG_TOKEN_RBRACKET)) {
      do {
        skip_ignorable(p);
        if (check(p, ENLNG_TOKEN_RBRACKET))
          break;
        if (count >= cap) {
          cap *= 2;
          elems = (ASTNode **)realloc(elems, sizeof(ASTNode *) * cap);
        }
        elems[count++] = parse_expression(p);
        skip_ignorable(p);
      } while (match(p, ENLNG_TOKEN_COMMA));
    }

    skip_ignorable(p);
    if (!match(p, ENLNG_TOKEN_RBRACKET)) {
      set_error(p, "Expected ']' at end of list literal", t->line);
    }
    n->as.list_literal.elements = elems;
    n->as.list_literal.count = count;
    return n;
  }

  /* Map Literal: {"key": val, key2: val2} */
  if (match(p, ENLNG_TOKEN_LBRACE)) {
    ASTNode *n = ast_new(AST_EXPR_MAP_LITERAL, t->line);
    int cap = 8;
    int count = 0;
    char **keys = (char **)malloc(sizeof(char *) * cap);
    ASTNode **values = (ASTNode **)malloc(sizeof(ASTNode *) * cap);

    skip_ignorable(p);
    if (!check(p, ENLNG_TOKEN_RBRACE)) {
      do {
        skip_ignorable(p);
        if (check(p, ENLNG_TOKEN_RBRACE))
          break;
        Token *kt = peek(p, 0);
        char *k = NULL;
        if (match(p, ENLNG_TOKEN_STRING_LITERAL) ||
            match(p, ENLNG_TOKEN_IDENTIFIER)) {
          k = enlng_strdup(kt->text);
        } else {
          set_error(p, "Expected string or identifier as map key", kt->line);
          break;
        }

        skip_ignorable(p);
        if (!match(p, ENLNG_TOKEN_COLON)) {
          set_error(p, "Expected ':' after map key", kt->line);
        }
        skip_ignorable(p);

        ASTNode *val = parse_expression(p);
        if (count >= cap) {
          cap *= 2;
          keys = (char **)realloc(keys, sizeof(char *) * cap);
          values = (ASTNode **)realloc(values, sizeof(ASTNode *) * cap);
        }
        keys[count] = k;
        values[count] = val;
        count++;
        skip_ignorable(p);
      } while (match(p, ENLNG_TOKEN_COMMA));
    }

    skip_ignorable(p);
    if (!match(p, ENLNG_TOKEN_RBRACE)) {
      set_error(p, "Expected '}' at end of map literal", t->line);
    }
    n->as.map_literal.keys = keys;
    n->as.map_literal.values = values;
    n->as.map_literal.count = count;
    return n;
  }

  /* Count of */
  if (match(p, ENLNG_TOKEN_COUNT_OF)) {
    ASTNode *tgt = parse_primary(p);
    if (match(p, ENLNG_TOKEN_IN)) {
      ASTNode *cont = parse_primary(p);
      ASTNode *n = ast_new(AST_EXPR_COUNT_IN, t->line);
      n->as.count_in_expr.target = tgt;
      n->as.count_in_expr.container = cont;
      return n;
    }
    ASTNode *n = ast_new(AST_EXPR_COUNT_OF, t->line);
    n->as.single_target_expr.target = tgt;
    return n;
  }

  /* Reverse (expression): reverse word */
  if (match(p, ENLNG_TOKEN_REVERSE)) {
    ASTNode *n = ast_new(AST_EXPR_REVERSE, t->line);
    n->as.single_target_expr.target = parse_primary(p);
    return n;
  }

  /* Identifiers, Indexing, and Calls */
  if (match(p, ENLNG_TOKEN_IDENTIFIER) || match(p, ENLNG_TOKEN_PAIR)) {
    char *name = enlng_strdup(t->text);

    /* m.helper: a function or name of the file used as m */
    if (check(p, ENLNG_TOKEN_DOT) && peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER) {
      char mangled[300];
      if (mod_resolve(p, name, peek(p, 1)->text, mangled, sizeof(mangled), t->line)) {
        advance(p);
        advance(p);
        free(name);
        name = enlng_strdup(mangled);
      }
    }

    /* Natural count target in container: count "a" in word */
    if (strcmp(name, "count") == 0 && has_token_on_line(p, ENLNG_TOKEN_IN) &&
        !is_operator_or_closing(peek(p, 0)->type) && !check(p, ENLNG_TOKEN_LPAREN) &&
        !check(p, ENLNG_TOKEN_DOT) && !check(p, ENLNG_TOKEN_LBRACKET) &&
        !check(p, ENLNG_TOKEN_OF)) {
      free(name);
      ASTNode *tgt = parse_primary(p);
      if (match(p, ENLNG_TOKEN_IN)) {
        ASTNode *cont = parse_primary(p);
        ASTNode *n = ast_new(AST_EXPR_COUNT_IN, t->line);
        n->as.count_in_expr.target = tgt;
        n->as.count_in_expr.container = cont;
        return n;
      }
      set_error(p, "Expected 'in' after count target expression", t->line);
      return NULL;
    }

    /* split target by/with/on sep */
    if (strcmp(name, "split") == 0 &&
        (has_token_on_line(p, ENLNG_TOKEN_BY) || has_token_on_line(p, ENLNG_TOKEN_WITH) ||
         has_token_on_line(p, ENLNG_TOKEN_ON)) &&
        !is_operator_or_closing(peek(p, 0)->type) && !check(p, ENLNG_TOKEN_LPAREN) &&
        !check(p, ENLNG_TOKEN_DOT) && !check(p, ENLNG_TOKEN_LBRACKET) &&
        !check(p, ENLNG_TOKEN_OF)) {
      free(name);
      ASTNode *tgt = parse_primary_internal(p, false);
      if (match(p, ENLNG_TOKEN_BY) || match(p, ENLNG_TOKEN_WITH) || match(p, ENLNG_TOKEN_ON)) {
        ASTNode *sep = parse_primary(p);
        ASTNode *n = ast_new(AST_EXPR_SPLIT, t->line);
        n->as.count_in_expr.target = tgt;
        n->as.count_in_expr.container = sep;
        return n;
      }
      set_error(p, "Expected 'by', 'with', or 'on' after split target", t->line);
      return NULL;
    }

    /* join list with sep */
    if (strcmp(name, "join") == 0 &&
        (has_token_on_line(p, ENLNG_TOKEN_WITH) || has_token_on_line(p, ENLNG_TOKEN_BY)) &&
        !is_operator_or_closing(peek(p, 0)->type) && !check(p, ENLNG_TOKEN_LPAREN) &&
        !check(p, ENLNG_TOKEN_DOT) && !check(p, ENLNG_TOKEN_LBRACKET) &&
        !check(p, ENLNG_TOKEN_OF)) {
      free(name);
      ASTNode *lst = parse_primary_internal(p, false);
      if (match(p, ENLNG_TOKEN_WITH) || match(p, ENLNG_TOKEN_BY)) {
        ASTNode *sep = parse_primary(p);
        ASTNode *n = ast_new(AST_EXPR_JOIN, t->line);
        n->as.count_in_expr.target = lst;
        n->as.count_in_expr.container = sep;
        return n;
      }
      set_error(p, "Expected 'with' or 'by' after join target", t->line);
      return NULL;
    }

    /* replace target with repl in container */
    if (strcmp(name, "replace") == 0 &&
        has_token_on_line(p, ENLNG_TOKEN_WITH) && has_token_on_line(p, ENLNG_TOKEN_IN) &&
        !is_operator_or_closing(peek(p, 0)->type) && !check(p, ENLNG_TOKEN_LPAREN) &&
        !check(p, ENLNG_TOKEN_DOT) && !check(p, ENLNG_TOKEN_LBRACKET) &&
        !check(p, ENLNG_TOKEN_OF)) {
      free(name);
      ASTNode *tgt = parse_primary_internal(p, false);
      if (match(p, ENLNG_TOKEN_WITH)) {
        ASTNode *repl = parse_primary_internal(p, false);
        if (match(p, ENLNG_TOKEN_IN)) {
          ASTNode *cont = parse_primary(p);
          ASTNode *n = ast_new(AST_EXPR_REPLACE, t->line);
          n->as.replace_expr.target = tgt;
          n->as.replace_expr.replacement = repl;
          n->as.replace_expr.container = cont;
          return n;
        }
        set_error(p, "Expected 'in' after replace replacement expression", t->line);
        return NULL;
      }
      set_error(p, "Expected 'with' after replace target expression", t->line);
      return NULL;
    }

    /* find target in container */
    if (strcmp(name, "find") == 0 && has_token_on_line(p, ENLNG_TOKEN_IN) &&
        !is_operator_or_closing(peek(p, 0)->type) && !check(p, ENLNG_TOKEN_LPAREN) &&
        !check(p, ENLNG_TOKEN_DOT) && !check(p, ENLNG_TOKEN_LBRACKET) &&
        !check(p, ENLNG_TOKEN_OF)) {
      free(name);
      ASTNode *tgt = parse_primary(p);
      if (match(p, ENLNG_TOKEN_IN)) {
        ASTNode *cont = parse_primary(p);
        ASTNode *n = ast_new(AST_EXPR_FIND, t->line);
        n->as.count_in_expr.target = tgt;
        n->as.count_in_expr.container = cont;
        return n;
      }
      set_error(p, "Expected 'in' after find target expression", t->line);
      return NULL;
    }

    /* trim target */
    if (strcmp(name, "trim") == 0 &&
        !is_operator_or_closing(peek(p, 0)->type) && !check(p, ENLNG_TOKEN_LPAREN) &&
        !check(p, ENLNG_TOKEN_DOT) && !check(p, ENLNG_TOKEN_LBRACKET) &&
        !check(p, ENLNG_TOKEN_OF)) {
      free(name);
      if (check(p, ENLNG_TOKEN_IDENTIFIER) && strcmp(peek(p, 0)->text, "spaces") == 0) {
        advance(p);
        match(p, ENLNG_TOKEN_FROM);
      }
      ASTNode *tgt = parse_primary(p);
      ASTNode *n = ast_new(AST_EXPR_TRIM, t->line);
      n->as.single_target_expr.target = tgt;
      return n;
    }

    /* uppercase of / lowercase of */
    if (strcmp(name, "uppercase") == 0 && match(p, ENLNG_TOKEN_OF)) {
      free(name);
      ASTNode *tgt = parse_primary(p);
      ASTNode *n = ast_new(AST_EXPR_TO_UPPER, t->line);
      n->as.single_target_expr.target = tgt;
      return n;
    }
    if (strcmp(name, "lowercase") == 0 && match(p, ENLNG_TOKEN_OF)) {
      free(name);
      ASTNode *tgt = parse_primary(p);
      ASTNode *n = ast_new(AST_EXPR_TO_LOWER, t->line);
      n->as.single_target_expr.target = tgt;
      return n;
    }

    /* sum of / average of / highest of / lowest of */
    if (strcmp(name, "sum") == 0 && match(p, ENLNG_TOKEN_OF)) {
      free(name);
      ASTNode *tgt = parse_primary(p);
      ASTNode *n = ast_new(AST_EXPR_SUM, t->line);
      n->as.single_target_expr.target = tgt;
      return n;
    }
    if ((strcmp(name, "average") == 0 || strcmp(name, "avg") == 0) && match(p, ENLNG_TOKEN_OF)) {
      free(name);
      ASTNode *tgt = parse_primary(p);
      ASTNode *n = ast_new(AST_EXPR_AVG, t->line);
      n->as.single_target_expr.target = tgt;
      return n;
    }
    if ((strcmp(name, "highest") == 0 || strcmp(name, "max") == 0) && match(p, ENLNG_TOKEN_OF)) {
      free(name);
      ASTNode *tgt = parse_primary(p);
      ASTNode *n = ast_new(AST_EXPR_MAX, t->line);
      n->as.single_target_expr.target = tgt;
      return n;
    }
    if ((strcmp(name, "lowest") == 0 || strcmp(name, "min") == 0) && match(p, ENLNG_TOKEN_OF)) {
      free(name);
      ASTNode *tgt = parse_primary(p);
      ASTNode *n = ast_new(AST_EXPR_MIN, t->line);
      n->as.single_target_expr.target = tgt;
      return n;
    }
    if (strcmp(name, "first") == 0 && match(p, ENLNG_TOKEN_OF)) {
      free(name);
      ASTNode *tgt = parse_primary(p);
      ASTNode *idx_expr = ast_new(AST_EXPR_LITERAL_STRING, t->line);
      idx_expr->as.literal.string_val = enlng_strdup("at_first");
      ASTNode *n = ast_new(AST_EXPR_INDEX, t->line);
      n->as.index_expr.target = tgt;
      n->as.index_expr.arr_name = (tgt && tgt->type == AST_EXPR_VARIABLE) ? enlng_strdup(tgt->as.variable.name) : NULL;
      n->as.index_expr.index_expr = idx_expr;
      return n;
    }
    if (strcmp(name, "last") == 0 && match(p, ENLNG_TOKEN_OF)) {
      free(name);
      ASTNode *tgt = parse_primary(p);
      ASTNode *idx_expr = ast_new(AST_EXPR_LITERAL_STRING, t->line);
      idx_expr->as.literal.string_val = enlng_strdup("at_last");
      ASTNode *n = ast_new(AST_EXPR_INDEX, t->line);
      n->as.index_expr.target = tgt;
      n->as.index_expr.arr_name = (tgt && tgt->type == AST_EXPR_VARIABLE) ? enlng_strdup(tgt->as.variable.name) : NULL;
      n->as.index_expr.index_expr = idx_expr;
      return n;
    }

    /* Special position keywords: at_first and at_last evaluate to string literals */
    if ((strcmp(name, "at_first") == 0 || strcmp(name, "at_last") == 0) &&
        !check(p, ENLNG_TOKEN_LPAREN) && !check(p, ENLNG_TOKEN_DOT) &&
        !check(p, ENLNG_TOKEN_LBRACKET) && !check(p, ENLNG_TOKEN_OF)) {
      ASTNode *n = ast_new(AST_EXPR_LITERAL_STRING, t->line);
      n->as.literal.string_val = name;
      return n;
    }

    /* Field access via 'of': field of object (e.g. resource of request, left of pair, name of user) */
    if (match(p, ENLNG_TOKEN_OF)) {
      ASTNode *tgt = parse_primary_internal(p, allow_with);
      ASTNode *n = ast_new(AST_EXPR_FIELD, t->line);
      n->as.field_expr.target = tgt;
      n->as.field_expr.obj_name =
          (tgt && tgt->type == AST_EXPR_VARIABLE)
              ? enlng_strdup(tgt->as.variable.name)
              : NULL;
      n->as.field_expr.field_name = name;
      return n;
    }

    /* Field access: pair.left / pair.right */
    if (match(p, ENLNG_TOKEN_DOT)) {
      Token *field_tok = peek(p, 0);
      if (match(p, ENLNG_TOKEN_IDENTIFIER)) {
        ASTNode *n = ast_new(AST_EXPR_FIELD, t->line);
        n->as.field_expr.obj_name = name;
        n->as.field_expr.field_name = enlng_strdup(field_tok->text);
        return n;
      }
    }

    /* Array index: arr[i] */
    if (match(p, ENLNG_TOKEN_LBRACKET))
      return parse_index_or_slice(p, NULL, name, t->line);

    /* Direct s at_last or s at_first */
    if (check(p, ENLNG_TOKEN_IDENTIFIER) &&
        (strcmp(peek(p, 0)->text, "at_last") == 0 || strcmp(peek(p, 0)->text, "at_first") == 0)) {
      Token *ptok = advance(p);
      ASTNode *idx_expr = ast_new(AST_EXPR_LITERAL_STRING, t->line);
      idx_expr->as.literal.string_val = enlng_strdup(ptok->text);
      ASTNode *n = ast_new(AST_EXPR_INDEX, t->line);
      n->as.index_expr.arr_name = name;
      n->as.index_expr.index_expr = idx_expr;
      return n;
    }

    /* Array / Map index with English 'at': coll at index */
    if (match(p, ENLNG_TOKEN_AT)) {
      ASTNode *idx_expr = parse_unary(p);
      ASTNode *n = ast_new(AST_EXPR_INDEX, t->line);
      n->as.index_expr.arr_name = name;
      n->as.index_expr.index_expr = idx_expr;
      return n;
    }

    /* Function Call with English 'with': func with a, b */
    if (allow_with && match(p, ENLNG_TOKEN_WITH)) {
      ASTNode *n = ast_new(AST_EXPR_CALL, t->line);
      n->as.call_expr.func_name = name;
      int cap = 4;
      int count = 0;
      ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *) * cap);
      char **names = NULL;

      do {
        if (count >= cap) {
          cap *= 2;
          args = (ASTNode **)realloc(args, sizeof(ASTNode *) * cap);
          if (names)
            names = (char **)realloc(names, sizeof(char *) * cap);
        }
        if (names)
          names[count] = NULL;
        args[count] = parse_call_arg(p, &names, count, cap, false);
        count++;
      } while (match(p, ENLNG_TOKEN_COMMA) ||
               (check(p, ENLNG_TOKEN_AND) && peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER &&
                peek(p, 2)->type == ENLNG_TOKEN_AS && match(p, ENLNG_TOKEN_AND)));

      n->as.call_expr.args = args;
      n->as.call_expr.arg_count = count;
      n->as.call_expr.arg_names = names;
      return n;
    }

    /* Function Call: func(a, b) */
    if (match(p, ENLNG_TOKEN_LPAREN)) {
      ASTNode *n = ast_new(AST_EXPR_CALL, t->line);
      n->as.call_expr.func_name = name;
      int cap = 4;
      int count = 0;
      ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *) * cap);

      char **names = NULL;
      if (!check(p, ENLNG_TOKEN_RPAREN)) {
        do {
          if (count >= cap) {
            cap *= 2;
            args = (ASTNode **)realloc(args, sizeof(ASTNode *) * cap);
            if (names)
              names = (char **)realloc(names, sizeof(char *) * cap);
          }
          if (names)
            names[count] = NULL;
          args[count] = parse_call_arg(p, &names, count, cap, true);
          count++;
        } while (match(p, ENLNG_TOKEN_COMMA));
      }

      if (!match(p, ENLNG_TOKEN_RPAREN)) {
        set_error(p, "Expected ')' after function arguments", t->line);
      }
      n->as.call_expr.args = args;
      n->as.call_expr.arg_count = count;
      n->as.call_expr.arg_names = names;
      return n;
    }

    /* Simple Variable */
    ASTNode *n = ast_new(AST_EXPR_VARIABLE, t->line);
    n->as.variable.name = name;
    return n;
  }

  set_error(p, "Unexpected token in expression", t->line);
  advance(p);
  return ast_new(AST_EXPR_LITERAL_INT, t->line);
}

static ASTNode *parse_primary_internal(Parser *p, bool allow_with) {
  ASTNode *node = parse_atom(p, allow_with);
  if (!node || p->has_error)
    return node;

  /* Universal Postfix Chaining: node[index], node.field, node at index, node at_first/at_last */
  while (!p->has_error) {
    if (match(p, ENLNG_TOKEN_LBRACKET)) {
      node = parse_index_or_slice(
          p, node,
          node->type == AST_EXPR_VARIABLE ? node->as.variable.name : NULL,
          node->line);
    } else if (match(p, ENLNG_TOKEN_DOT)) {
      Token *field_tok = peek(p, 0);
      if (match(p, ENLNG_TOKEN_IDENTIFIER)) {
        ASTNode *f_node = ast_new(AST_EXPR_FIELD, node->line);
        f_node->as.field_expr.target = node;
        f_node->as.field_expr.obj_name =
            (node->type == AST_EXPR_VARIABLE)
                ? enlng_strdup(node->as.variable.name)
                : NULL;
        f_node->as.field_expr.field_name = enlng_strdup(field_tok->text);
        node = f_node;
      } else {
        set_error(p, "Expected field name after '.'", node->line);
        break;
      }
    } else if (check(p, ENLNG_TOKEN_IDENTIFIER) &&
               (strcmp(peek(p, 0)->text, "at_last") == 0 ||
                strcmp(peek(p, 0)->text, "at_first") == 0)) {
      Token *ptok = advance(p);
      ASTNode *idx_expr = ast_new(AST_EXPR_LITERAL_STRING, node->line);
      idx_expr->as.literal.string_val = enlng_strdup(ptok->text);
      ASTNode *idx_node = ast_new(AST_EXPR_INDEX, node->line);
      idx_node->as.index_expr.target = node;
      idx_node->as.index_expr.arr_name =
          (node->type == AST_EXPR_VARIABLE)
              ? enlng_strdup(node->as.variable.name)
              : NULL;
      idx_node->as.index_expr.index_expr = idx_expr;
      node = idx_node;
    } else if (match(p, ENLNG_TOKEN_AT)) {
      ASTNode *idx_expr = parse_unary(p);
      ASTNode *idx_node = ast_new(AST_EXPR_INDEX, node->line);
      idx_node->as.index_expr.target = node;
      idx_node->as.index_expr.arr_name =
          (node->type == AST_EXPR_VARIABLE)
              ? enlng_strdup(node->as.variable.name)
              : NULL;
      idx_node->as.index_expr.index_expr = idx_expr;
      node = idx_node;
    } else {
      break;
    }
  }

  return node;
}

static ASTNode *parse_unary(Parser *p) {
  if (check(p, ENLNG_TOKEN_NOT) || check(p, ENLNG_TOKEN_SUB)) {
    Token *op = advance(p);
    ASTNode *n = ast_new(AST_EXPR_UNARY, op->line);
    n->as.unary_expr.op = op->type;
    n->as.unary_expr.operand = parse_unary(p);
    return n;
  }
  return parse_primary(p);
}

/* 2 to the power of 10, 5 squared */
static ASTNode *parse_power(Parser *p) {
  ASTNode *base = parse_unary(p);
  for (;;) {
    if (check(p, ENLNG_TOKEN_TO) && tok_is(peek(p, 1), "power") &&
        peek(p, 2)->type == ENLNG_TOKEN_OF) {
      int line = advance(p)->line;
      advance(p);
      advance(p);
      base = call2("power", base, parse_unary(p), line);
    } else if (tok_is(peek(p, 0), "squared")) {
      int line = advance(p)->line;
      base = call2("power", base, int_node(2, line), line);
    } else {
      return base;
    }
  }
}

static ASTNode *parse_multiplicative(Parser *p) {
  ASTNode *left = parse_power(p);
  while (check(p, ENLNG_TOKEN_MUL) || check(p, ENLNG_TOKEN_DIV) ||
         check(p, ENLNG_TOKEN_MOD)) {
    Token *op = advance(p);
    ASTNode *right = parse_power(p);
    ASTNode *n = ast_new(AST_EXPR_BINARY, op->line);
    n->as.binary_expr.op = op->type;
    n->as.binary_expr.left = left;
    n->as.binary_expr.right = right;
    left = n;
  }
  return left;
}

static ASTNode *parse_additive(Parser *p) {
  ASTNode *left = parse_multiplicative(p);
  while (check(p, ENLNG_TOKEN_ADD) || check(p, ENLNG_TOKEN_SUB)) {
    Token *op = advance(p);
    ASTNode *right = parse_multiplicative(p);
    ASTNode *n = ast_new(AST_EXPR_BINARY, op->line);
    n->as.binary_expr.op = op->type;
    n->as.binary_expr.left = left;
    n->as.binary_expr.right = right;
    left = n;
  }
  return left;
}

static ASTNode *parse_comparison(Parser *p) {
  ASTNode *left = parse_additive(p);
  while (check(p, ENLNG_TOKEN_EQ) || check(p, ENLNG_TOKEN_NEQ) ||
         check(p, ENLNG_TOKEN_GT) || check(p, ENLNG_TOKEN_LT) ||
         check(p, ENLNG_TOKEN_GTE) || check(p, ENLNG_TOKEN_LTE) ||
         (check(p, ENLNG_TOKEN_IDENTIFIER) && strcmp(peek(p, 0)->text, "contains") == 0)) {
    if (check(p, ENLNG_TOKEN_IDENTIFIER) && strcmp(peek(p, 0)->text, "contains") == 0) {
      Token *op = advance(p);
      ASTNode *right = parse_additive(p);
      ASTNode *call = ast_new(AST_EXPR_CALL, op->line);
      call->as.call_expr.func_name = enlng_strdup("contains");
      call->as.call_expr.arg_count = 2;
      call->as.call_expr.args = (ASTNode **)malloc(sizeof(ASTNode *) * 2);
      call->as.call_expr.args[0] = left;
      call->as.call_expr.args[1] = right;
      left = call;
      continue;
    }
    Token *op = advance(p);
    bool negated = op->type == ENLNG_TOKEN_NEQ;
    /* 3 is in numbers / "x" is not in word */
    if ((op->type == ENLNG_TOKEN_EQ || negated) && match(p, ENLNG_TOKEN_IN)) {
      ASTNode *found = call2("contains", parse_additive(p), left, op->line);
      left = negated ? not_node(found, op->line) : found;
      continue;
    }
    /* x is a number / x is not a whole number */
    if ((op->type == ENLNG_TOKEN_EQ || negated) &&
        (oop_is_ident(peek(p, 0), "a") || oop_is_ident(peek(p, 0), "an"))) {
      const char *kind = NULL;
      int words = 0;
      Token *k1 = peek(p, 1);
      if (tok_is(k1, "whole") && tok_is(peek(p, 2), "number")) {
        kind = "whole number";
        words = 2;
      } else if (tok_is(k1, "number") || tok_is(k1, "decimal") ||
                 tok_is(k1, "list") || tok_is(k1, "map") ||
                 k1->type == ENLNG_TOKEN_FUNCTION ||
                 (k1->text && strcmp(k1->text, "text") == 0)) {
        kind = k1->type == ENLNG_TOKEN_FUNCTION ? "function" : k1->text;
        words = 1;
      }
      if (kind) {
        advance(p); /* a / an */
        for (int w = 0; w < words; w++)
          advance(p);
        ASTNode *is = call2("is_kind", left, str_node(kind, op->line), op->line);
        left = negated ? not_node(is, op->line) : is;
        continue;
      }
    }
    /* "x is a Dog": the article a / an is skipped when a word follows it */
    if (op->type == ENLNG_TOKEN_EQ &&
        (oop_is_ident(peek(p, 0), "a") || oop_is_ident(peek(p, 0), "an")) &&
        peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER)
      advance(p);
    ASTNode *right = parse_additive(p);
    if (op->type == ENLNG_TOKEN_EQ && right && right->type == AST_EXPR_VARIABLE &&
        oop_find_class(right->as.variable.name) >= 0) {
      char fname[300];
      oop_note_is(right->as.variable.name, op->line);
      snprintf(fname, sizeof(fname), "oop_is_%s", right->as.variable.name);
      ASTNode **cargs = (ASTNode **)malloc(sizeof(ASTNode *));
      cargs[0] = left;
      left = oop_call_node(fname, cargs, 1, op->line);
      continue;
    }
    ASTNode *n = ast_new(AST_EXPR_BINARY, op->line);
    n->as.binary_expr.op = op->type;
    n->as.binary_expr.left = left;
    n->as.binary_expr.right = right;
    left = n;
  }
  return left;
}

static ASTNode *parse_logical_and(Parser *p) {
  ASTNode *left = parse_comparison(p);
  while (check(p, ENLNG_TOKEN_AND)) {
    Token *op = advance(p);
    ASTNode *right = parse_comparison(p);
    ASTNode *n = ast_new(AST_EXPR_BINARY, op->line);
    n->as.binary_expr.op = op->type;
    n->as.binary_expr.left = left;
    n->as.binary_expr.right = right;
    left = n;
  }
  return left;
}

static ASTNode *parse_or_expr(Parser *p) {
  ASTNode *left = parse_logical_and(p);
  while (check(p, ENLNG_TOKEN_OR)) {
    Token *op = advance(p);
    ASTNode *right = parse_logical_and(p);
    ASTNode *n = ast_new(AST_EXPR_BINARY, op->line);
    n->as.binary_expr.op = op->type;
    n->as.binary_expr.left = left;
    n->as.binary_expr.right = right;
    left = n;
  }
  return left;
}

/* "every n in numbers where n > 10" and "n times 2 for every n in numbers":
   the loop part, after the name. Gives the list to work on, already kept to
   the items that pass `where`. */
static ASTNode *parse_list_source(Parser *p, const char *var, int line) {
  if (!match(p, ENLNG_TOKEN_IN)) {
    set_error(p, "Expected 'in' and a list, as in: every n in numbers", line);
    return ast_new(AST_EXPR_LITERAL_NULL, line);
  }
  ASTNode *coll = parse_or_expr(p);
  if (match(p, ENLNG_TOKEN_WHERE))
    coll = call2("_keep_where", coll, lambda1(var, parse_or_expr(p), line), line);
  return coll;
}

static ASTNode *parse_expression(Parser *p) {
  ASTNode *left = parse_or_expr(p);
  /* n times 2 for every n in numbers [where n > 0] */
  if (check(p, ENLNG_TOKEN_FOR) && peek(p, 1)->type == ENLNG_TOKEN_EACH &&
      peek(p, 2)->type == ENLNG_TOKEN_IDENTIFIER &&
      peek(p, 3)->type == ENLNG_TOKEN_IN) {
    int line = advance(p)->line;
    advance(p);
    const char *var = advance(p)->text;
    ASTNode *src = parse_list_source(p, var, line);
    left = call2("_map_each", src, lambda1(var, left, line), line);
  }
  /* "adult" if age >= 18 otherwise "child" */
  if (check(p, ENLNG_TOKEN_WHEN) && has_token_on_line(p, ENLNG_TOKEN_OTHERWISE)) {
    int line = advance(p)->line;
    ASTNode *n = ast_new(AST_EXPR_TERNARY, line);
    n->as.ternary.cond = parse_or_expr(p);
    if (!match(p, ENLNG_TOKEN_OTHERWISE))
      set_error(p, "Expected 'otherwise' and the other value, as in: "
                   "\"adult\" if age >= 18 otherwise \"child\"", line);
    n->as.ternary.yes = left;
    n->as.ternary.no = parse_expression(p);
    left = n;
  }
  return left;
}

/* Parse a block of statements governed by INDENT ... DEDENT */
static void parse_block_inner(Parser *p, ASTNode ***out_stmts, int *out_count) {
  if (!match(p, ENLNG_TOKEN_COLON)) {
    set_error(p, "Expected ':' before block", peek(p, 0)->line);
    return;
  }

  /* Skip optional newlines before indent */
  skip_newlines(p);

  int cap = 8;
  int count = 0;
  ASTNode **stmts = (ASTNode **)malloc(sizeof(ASTNode *) * cap);

  /* Single line block: when x > 0: swap a, b */
  if (!check(p, ENLNG_TOKEN_INDENT)) {
    if (!check(p, ENLNG_TOKEN_NEWLINE) && !check(p, ENLNG_TOKEN_EOF)) {
      stmts[count++] = parse_statement(p);
      for (int e = 0; e < g_nextra; e++) {
        if (count >= cap) {
          cap *= 2;
          stmts = (ASTNode **)realloc(stmts, sizeof(ASTNode *) * cap);
        }
        stmts[count++] = g_extra[e];
      }
      g_nextra = 0;
    }
    *out_stmts = stmts;
    *out_count = count;
    return;
  }

  /* Indented block */
  match(p, ENLNG_TOKEN_INDENT);
  skip_newlines(p);

  while (!check(p, ENLNG_TOKEN_DEDENT) && !check(p, ENLNG_TOKEN_EOF)) {
    skip_newlines(p);
    if (check(p, ENLNG_TOKEN_DEDENT) || check(p, ENLNG_TOKEN_EOF))
      break;

    ASTNode *stmt = parse_statement(p);
    if (stmt) {
      if (count >= cap) {
        cap *= 2;
        stmts = (ASTNode **)realloc(stmts, sizeof(ASTNode *) * cap);
      }
      stmts[count++] = stmt;
    }
    for (int e = 0; e < g_nextra; e++) {
      if (count >= cap) {
        cap *= 2;
        stmts = (ASTNode **)realloc(stmts, sizeof(ASTNode *) * cap);
      }
      stmts[count++] = g_extra[e];
    }
    g_nextra = 0;
    skip_newlines(p);
  }

  match(p, ENLNG_TOKEN_DEDENT);
  *out_stmts = stmts;
  *out_count = count;
}

static void parse_block(Parser *p, ASTNode ***out_stmts, int *out_count) {
  g_block_depth++;
  parse_block_inner(p, out_stmts, out_count);
  g_block_depth--;
}

/* ---------------- OOP parsing ---------------- */

static bool oop_starts_class(Parser *p) {
  Token *t = peek(p, 0);
  if (!(oop_is_ident(t, "class") || oop_is_ident(t, "blueprint") ||
        oop_is_ident(t, "model")))
    return false;
  if (peek(p, 1)->type != ENLNG_TOKEN_IDENTIFIER)
    return false;
  Token *t2 = peek(p, 2);
  return t2->type == ENLNG_TOKEN_COLON || oop_is_ident(t2, "inherits") ||
         oop_is_ident(t2, "extends");
}

static bool oop_starts_call(Parser *p) {
  Token *t = peek(p, 0);
  if (oop_is_ident(t, "call") && peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER &&
      (peek(p, 2)->type == ENLNG_TOKEN_ON ||
       peek(p, 2)->type == ENLNG_TOKEN_FROM))
    return true;
  if (oop_is_ident(t, "tell") && peek(p, 1)->type != ENLNG_TOKEN_ASSIGN &&
      has_token_on_line(p, ENLNG_TOKEN_TO))
    return true;
  if (t->type == ENLNG_TOKEN_ASK &&
      peek(p, 1)->type != ENLNG_TOKEN_STRING_LITERAL &&
      has_token_on_line(p, ENLNG_TOKEN_TO))
    return true;
  return false;
}

static bool oop_starts_new(Parser *p) {
  Token *t = peek(p, 0);
  if (!(oop_is_ident(t, "new") || oop_is_ident(t, "instantiate")))
    return false;
  Token *c = peek(p, 1);
  return c->type == ENLNG_TOKEN_IDENTIFIER && c->text &&
         oop_find_class(c->text) >= 0;
}

static void oop_parse_args(Parser *p, ASTNode ***out, int *count) {
  int cap = 4, n = 0;
  ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *) * cap);
  if (match(p, ENLNG_TOKEN_WITH)) {
    do {
      if (n >= cap) {
        cap *= 2;
        args = (ASTNode **)realloc(args, sizeof(ASTNode *) * cap);
      }
      args[n++] = parse_additive(p);
    } while (match(p, ENLNG_TOKEN_COMMA));
  }
  *out = args;
  *count = n;
}

/* call m on obj [with ...] | call m from Base on obj [with ...]
   tell obj to m [with ...] | ask obj to m [with ...] */
static ASTNode *parse_oop_call(Parser *p) {
  Token *first = advance(p);
  int line = first->line;
  char mname[128];
  char base[128];
  base[0] = '\0';
  ASTNode *obj = NULL;

  if (oop_is_ident(first, "call")) {
    Token *m = advance(p);
    snprintf(mname, sizeof(mname), "%s", m->text);
    if (match(p, ENLNG_TOKEN_FROM)) {
      Token *b = peek(p, 0);
      if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
        set_error(p, "Expected a class name after 'from'", line);
        return ast_new(AST_EXPR_LITERAL_INT, line);
      }
      snprintf(base, sizeof(base), "%s", b->text);
    }
    if (!match(p, ENLNG_TOKEN_ON)) {
      set_error(p, "Expected 'on' and an object after the method name", line);
      return ast_new(AST_EXPR_LITERAL_INT, line);
    }
    obj = parse_primary_internal(p, false);
  } else {
    obj = parse_primary_internal(p, false);
    if (!match(p, ENLNG_TOKEN_TO)) {
      set_error(p, "Expected 'to' and a method name after the object", line);
      return ast_new(AST_EXPR_LITERAL_INT, line);
    }
    Token *m = peek(p, 0);
    if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
      set_error(p, "Expected a method name after 'to'", line);
      return ast_new(AST_EXPR_LITERAL_INT, line);
    }
    snprintf(mname, sizeof(mname), "%s", m->text);
  }

  ASTNode **extra = NULL;
  int nextra = 0;
  oop_parse_args(p, &extra, &nextra);
  ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *) * (nextra + 1));
  args[0] = obj;
  for (int i = 0; i < nextra; i++)
    args[i + 1] = extra[i];
  free(extra);

  char fname[300];
  if (base[0]) {
    int ci = oop_find_class(base);
    if (ci < 0) {
      char err[256];
      snprintf(err, sizeof(err), "I don't know a class named '%s'", base);
      set_error(p, err, line);
      return ast_new(AST_EXPR_LITERAL_INT, line);
    }
    int argc = 0, owner = 0;
    if (!oop_find_method(ci, mname, &argc, &owner)) {
      char err[256];
      snprintf(err, sizeof(err), "Class '%s' has no method '%s'", base, mname);
      set_error(p, err, line);
      return ast_new(AST_EXPR_LITERAL_INT, line);
    }
    if (argc != nextra) {
      char err[256];
      snprintf(err, sizeof(err),
               "Method '%s' of '%s' takes %d input(s) but got %d", mname, base,
               argc, nextra);
      set_error(p, err, line);
      return ast_new(AST_EXPR_LITERAL_INT, line);
    }
    snprintf(fname, sizeof(fname), "%s__%s", g_cls[owner].name, mname);
  } else {
    oop_note_dispatch(mname, nextra, line);
    snprintf(fname, sizeof(fname), "oop_call_%s_%d", mname, nextra);
  }
  return oop_call_node(fname, args, nextra + 1, line);
}

/* new Class [with field as value, ...] */
static ASTNode *parse_oop_new(Parser *p) {
  Token *nw = advance(p);
  Token *ct = advance(p);
  int line = nw->line;
  int ci = oop_find_class(ct->text);
  OopClass *c = &g_cls[ci];

  /* all fields, base classes first, a later class overrides an earlier one */
  int cap = 16, nf = 0;
  char **keys = (char **)malloc(sizeof(char *) * cap);
  ASTNode **vals = (ASTNode **)malloc(sizeof(ASTNode *) * cap);
  for (int k = c->nmro - 1; k >= 0; k--) {
    OopClass *o = &g_cls[c->mro[k]];
    for (int j = 0; j < o->nfields; j++) {
      int at = -1;
      for (int q = 0; q < nf; q++)
        if (strcmp(keys[q], o->fields[j]) == 0)
          at = q;
      if (at < 0) {
        if (nf >= cap) {
          cap *= 2;
          keys = (char **)realloc(keys, sizeof(char *) * cap);
          vals = (ASTNode **)realloc(vals, sizeof(ASTNode *) * cap);
        }
        at = nf++;
        keys[at] = o->fields[j];
      }
      vals[at] = o->defaults[j];
    }
  }

  if (match(p, ENLNG_TOKEN_WITH)) {
    bool more = true;
    while (more) {
      Token *ft = peek(p, 0);
      if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
        set_error(p, "Expected a field name after 'with'", line);
        break;
      }
      if (!match(p, ENLNG_TOKEN_AS) && !match(p, ENLNG_TOKEN_ASSIGN)) {
        set_error(p, "Expected 'as' and a value after the field name", line);
        break;
      }
      ASTNode *v = parse_additive(p);
      int at = -1;
      for (int q = 0; q < nf; q++)
        if (strcmp(keys[q], ft->text) == 0)
          at = q;
      if (at < 0) {
        char err[256];
        snprintf(err, sizeof(err), "Class '%s' has no field named '%s'",
                 c->name, ft->text);
        set_error(p, err, line);
        break;
      }
      vals[at] = v;
      /* a comma continues the list only when another "field as" follows, so
         [new A with x as 1, new B] still reads as two items */
      more = check(p, ENLNG_TOKEN_COMMA) &&
             peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER &&
             (peek(p, 2)->type == ENLNG_TOKEN_AS ||
              peek(p, 2)->type == ENLNG_TOKEN_ASSIGN);
      if (more)
        advance(p);
    }
  }

  ASTNode *m = ast_new(AST_EXPR_MAP_LITERAL, line);
  m->as.map_literal.count = nf + 1;
  m->as.map_literal.keys = (char **)malloc(sizeof(char *) * (nf + 1));
  m->as.map_literal.values = (ASTNode **)malloc(sizeof(ASTNode *) * (nf + 1));
  m->as.map_literal.keys[0] = enlng_strdup("__class");
  m->as.map_literal.values[0] = oop_string_node(c->name, line);
  for (int i = 0; i < nf; i++) {
    m->as.map_literal.keys[i + 1] = enlng_strdup(keys[i]);
    m->as.map_literal.values[i + 1] = vals[i];
  }
  free(keys);
  free(vals);
  return m;
}

static void oop_parse_params(Parser *p, char ***out, int *count, int line) {
  int cap = 6, n = 0;
  char **params = (char **)malloc(sizeof(char *) * cap);
  params[n++] = enlng_strdup("self");
  if (check(p, ENLNG_TOKEN_WITH) || check(p, ENLNG_TOKEN_LPAREN)) {
    bool paren = check(p, ENLNG_TOKEN_LPAREN);
    advance(p);
    if (!(paren && check(p, ENLNG_TOKEN_RPAREN))) {
      do {
        Token *pt = peek(p, 0);
        if (match(p, ENLNG_TOKEN_IDENTIFIER)) {
          if (n >= cap) {
            cap *= 2;
            params = (char **)realloc(params, sizeof(char *) * cap);
          }
          params[n++] = enlng_strdup(pt->text);
        }
      } while (match(p, ENLNG_TOKEN_COMMA));
    }
    if (paren && !match(p, ENLNG_TOKEN_RPAREN))
      set_error(p, "Expected ')' after method parameters", line);
  }
  *out = params;
  *count = n;
}

static ASTNode *parse_class_def(Parser *p) {
  Token *kw = advance(p);
  Token *nt = advance(p);
  int line = kw->line;
  if (g_block_depth > 0) {
    set_error(p, "A class must be written at the top level of the file, not inside a function or a block", line);
    return NULL;
  }
  if (oop_find_class(nt->text) >= 0) {
    char err[256];
    snprintf(err, sizeof(err), "A class named '%s' already exists", nt->text);
    set_error(p, err, line);
    return NULL;
  }
  if (g_ncls >= g_capcls) {
    g_capcls = g_capcls ? g_capcls * 2 : 16;
    g_cls = (OopClass *)realloc(g_cls, sizeof(OopClass) * g_capcls);
  }
  int ci = g_ncls++;
  memset(&g_cls[ci], 0, sizeof(OopClass));
  g_cls[ci].name = enlng_strdup(nt->text);

  if (oop_is_ident(peek(p, 0), "inherits") ||
      oop_is_ident(peek(p, 0), "extends")) {
    advance(p);
    match(p, ENLNG_TOKEN_FROM);
    do {
      Token *pt = peek(p, 0);
      if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
        set_error(p, "Expected a class name after 'inherits'", line);
        return NULL;
      }
      int pi = oop_find_class(pt->text);
      if (pi < 0 || pi == ci) {
        char err[256];
        snprintf(err, sizeof(err),
                 "I don't know a class named '%s'. Define it before '%s'.",
                 pt->text, nt->text);
        set_error(p, err, line);
        return NULL;
      }
      if (g_cls[ci].nparents >= 8) {
        set_error(p, "A class can inherit from at most 8 classes", line);
        return NULL;
      }
      g_cls[ci].parents[g_cls[ci].nparents++] = pi;
    } while (match(p, ENLNG_TOKEN_COMMA));
  }
  if (!oop_build_mro(ci)) {
    set_error(p, "These base classes cannot be combined: their order conflicts",
              line);
    return NULL;
  }

  if (!match(p, ENLNG_TOKEN_COLON)) {
    set_error(p, "Expected ':' after the class name", line);
    return NULL;
  }
  skip_newlines(p);
  ASTNode **fns = NULL;
  int nfns = 0;
  if (match(p, ENLNG_TOKEN_INDENT)) {
    skip_newlines(p);
    while (!check(p, ENLNG_TOKEN_DEDENT) && !check(p, ENLNG_TOKEN_EOF) &&
           !p->has_error) {
      Token *t = peek(p, 0);
      if (t->type == ENLNG_TOKEN_REMEMBER) {
        advance(p);
        Token *ft = peek(p, 0);
        if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
          name_error(p, "Expected a field name after 'remember' in a class",
                    t->line);
          return NULL;
        }
        if (!match(p, ENLNG_TOKEN_AS) && !match(p, ENLNG_TOKEN_ASSIGN)) {
          set_error(p, "Expected 'as' and a default value for the field",
                    t->line);
          return NULL;
        }
        ASTNode *dv = parse_expression(p);
        OopClass *c = &g_cls[ci];
        int at = -1;
        for (int q = 0; q < c->nfields; q++)
          if (strcmp(c->fields[q], ft->text) == 0)
            at = q;
        if (at < 0) {
          at = c->nfields++;
          c->fields = (char **)realloc(c->fields, sizeof(char *) * c->nfields);
          c->defaults =
              (ASTNode **)realloc(c->defaults, sizeof(ASTNode *) * c->nfields);
          c->fields[at] = enlng_strdup(ft->text);
        }
        c->defaults[at] = dv;
      } else if (oop_is_ident(t, "method")) {
        advance(p);
        Token *mt = peek(p, 0);
        if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
          name_error(p, "Expected a method name after 'method'", t->line);
          return NULL;
        }
        char **params = NULL;
        int pc = 0;
        oop_parse_params(p, &params, &pc, t->line);
        OopClass *c = &g_cls[ci];
        int at = -1;
        for (int q = 0; q < c->nmethods; q++)
          if (strcmp(c->methods[q], mt->text) == 0)
            at = q;
        if (at < 0) {
          at = c->nmethods++;
          c->methods =
              (char **)realloc(c->methods, sizeof(char *) * c->nmethods);
          c->margc = (int *)realloc(c->margc, sizeof(int) * c->nmethods);
          c->methods[at] = enlng_strdup(mt->text);
        }
        c->margc[at] = pc - 1;
        ASTNode *fn = ast_new(AST_FUNCTION, t->line);
        char fname[300];
        snprintf(fname, sizeof(fname), "%s__%s", c->name, mt->text);
        fn->as.func_def.name = enlng_strdup(fname);
        fn->as.func_def.params = params;
        fn->as.func_def.param_count = pc;
        parse_block(p, &fn->as.func_def.body, &fn->as.func_def.body_count);
        fns = (ASTNode **)realloc(fns, sizeof(ASTNode *) * (nfns + 1));
        fns[nfns++] = fn;
      } else {
        set_error(p,
                  "Inside a class, write 'remember field as value' or "
                  "'method name:'",
                  t->line);
        return NULL;
      }
      skip_newlines(p);
    }
    match(p, ENLNG_TOKEN_DEDENT);
  }
  if (nfns == 0)
    return NULL;
  for (int i = 1; i < nfns; i++)
    oop_queue_extra(fns[i]);
  ASTNode *first = fns[0];
  free(fns);
  return first;
}

/* Walk the small AST shapes the generated helpers use and stamp one line. */
static void oop_set_line(ASTNode *n, int line) {
  if (!n)
    return;
  n->line = line;
  switch (n->type) {
  case AST_FUNCTION:
    for (int i = 0; i < n->as.func_def.body_count; i++)
      oop_set_line(n->as.func_def.body[i], line);
    break;
  case AST_WHEN:
    oop_set_line(n->as.when_stmt.condition, line);
    for (int i = 0; i < n->as.when_stmt.then_count; i++)
      oop_set_line(n->as.when_stmt.then_body[i], line);
    for (int i = 0; i < n->as.when_stmt.else_count; i++)
      oop_set_line(n->as.when_stmt.else_body[i], line);
    break;
  case AST_DECLARATION:
    oop_set_line(n->as.decl.init_expr, line);
    break;
  case AST_GIVE:
    oop_set_line(n->as.give_stmt.expr, line);
    break;
  case AST_FAIL:
    oop_set_line(n->as.fail_stmt.message, line);
    break;
  case AST_EXPR_STMT:
    oop_set_line(n->as.expr_stmt.expr, line);
    break;
  case AST_EXPR_BINARY:
    oop_set_line(n->as.binary_expr.left, line);
    oop_set_line(n->as.binary_expr.right, line);
    break;
  case AST_EXPR_CALL:
    for (int i = 0; i < n->as.call_expr.arg_count; i++)
      oop_set_line(n->as.call_expr.args[i], line);
    break;
  case AST_EXPR_FIELD:
    oop_set_line(n->as.field_expr.target, line);
    break;
  default:
    break;
  }
}

static void oop_append(char **buf, size_t *len, size_t *cap, const char *fmt,
                       ...) {
  char tmp[1024];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(tmp, sizeof(tmp), fmt, ap);
  va_end(ap);
  size_t add = strlen(tmp);
  if (*len + add + 1 > *cap) {
    while (*len + add + 1 > *cap)
      *cap = *cap ? *cap * 2 : 4096;
    *buf = (char *)realloc(*buf, *cap);
  }
  memcpy(*buf + *len, tmp, add + 1);
  *len += add;
}

static void oop_parse_generated(char *buf, int line) {
  Lexer lx;
  lexer_init(&lx, buf);
  lexer_tokenize(&lx);
  Parser sp;
  parser_init(&sp, lx.tokens, lx.token_count);
  ASTNode *sub = parser_parse_program(&sp);
  if (sub && sub->type == AST_PROGRAM && !sp.has_error) {
    for (int s = 0; s < sub->as.program.count; s++) {
      oop_set_line(sub->as.program.statements[s], line);
      if (g_ngen >= g_capgen) {
        g_capgen = g_capgen ? g_capgen * 2 : 16;
        g_gen = (ASTNode **)realloc(g_gen, sizeof(ASTNode *) * g_capgen);
      }
      g_gen[g_ngen++] = sub->as.program.statements[s];
    }
  }
  lexer_free(&lx);
}

/* Build the dispatcher and "is a" helpers as plain Enlng source, parse them,
   and put them in front of the program. */
static void oop_generate_helpers(ASTNode *prog) {
  if (g_ndisp == 0 && g_nis == 0)
    return;
  for (int d = 0; d < g_ndisp; d++) {
    char *buf = NULL;
    size_t len = 0, cap = 0;
    int argc = g_disp[d].argc;
    oop_append(&buf, &len, &cap,
               "type enlng\nfunction oop_call_%s_%d with oop_obj",
               g_disp[d].name, argc);
    for (int i = 1; i <= argc; i++)
      oop_append(&buf, &len, &cap, ", oop_a%d", i);
    oop_append(&buf, &len, &cap, ":\n    remember oop_c as __class of oop_obj\n");
    for (int ci = 0; ci < g_ncls; ci++) {
      int margc = 0, owner = 0;
      if (!oop_find_method(ci, g_disp[d].name, &margc, &owner))
        continue;
      oop_append(&buf, &len, &cap, "    when oop_c == \"%s\":\n",
                 g_cls[ci].name);
      if (margc != argc) {
        oop_append(&buf, &len, &cap,
                   "        fail \"Method '%s' of '%s' takes %d input(s) but "
                   "got %d\"\n",
                   g_disp[d].name, g_cls[ci].name, margc, argc);
      } else {
        oop_append(&buf, &len, &cap, "        give %s__%s(oop_obj",
                   g_cls[owner].name, g_disp[d].name);
        for (int i = 1; i <= argc; i++)
          oop_append(&buf, &len, &cap, ", oop_a%d", i);
        oop_append(&buf, &len, &cap, ")\n");
      }
    }
    oop_append(&buf, &len, &cap,
               "    when oop_c == null:\n"
               "        fail \"This value is not an object, so it has no "
               "method '%s'\"\n"
               "    remember oop_msg as \"Class \" + oop_c + \" has no method "
               "'%s'\"\n"
               "    fail oop_msg\n",
               g_disp[d].name, g_disp[d].name);
    oop_parse_generated(buf, g_disp[d].line);
    free(buf);
  }
  for (int k = 0; k < g_nis; k++) {
    int target = oop_find_class(g_isnames[k]);
    char *buf = NULL;
    size_t len = 0, cap = 0;
    oop_append(&buf, &len, &cap,
               "type enlng\nfunction oop_is_%s with oop_obj:\n"
               "    remember oop_c as __class of oop_obj\n",
               g_isnames[k]);
    for (int ci = 0; ci < g_ncls; ci++) {
      bool is_sub = false;
      for (int m = 0; m < g_cls[ci].nmro; m++)
        if (g_cls[ci].mro[m] == target)
          is_sub = true;
      if (is_sub)
        oop_append(&buf, &len, &cap,
                   "    when oop_c == \"%s\":\n        give true\n",
                   g_cls[ci].name);
    }
    oop_append(&buf, &len, &cap, "    give false\n");
    oop_parse_generated(buf, g_isline[k]);
    free(buf);
  }
  int total = prog->as.program.count + g_ngen;
  ASTNode **all = (ASTNode **)malloc(sizeof(ASTNode *) * (total + 1));
  int n = 0;
  for (int i = 0; i < g_ngen; i++)
    all[n++] = g_gen[i];
  for (int i = 0; i < prog->as.program.count; i++)
    all[n++] = prog->as.program.statements[i];
  free(prog->as.program.statements);
  prog->as.program.statements = all;
  prog->as.program.count = n;
  prog->as.program.capacity = total + 1;
  g_ngen = 0;
}

/* when the color is: then one line per value. Written as a `when` for each
 * value, so the interpreter and the C generator need nothing new. */
static ASTNode *parse_choose_by_value(Parser *p) {
  Token *t = advance(p); /* when */
  ASTNode *subject = parse_additive(p);
  match(p, ENLNG_TOKEN_EQ);
  match(p, ENLNG_TOKEN_COLON);
  skip_newlines(p);
  if (!match(p, ENLNG_TOKEN_INDENT)) {
    set_error(p, "After 'when ... is:' write one value per line, indented, as in:  \"red\": show \"stop\"",
              t->line);
    return NULL;
  }
  char *tmp = hidden_name("value");
  ASTNode *first = decl_node(tmp, subject, false, t->line);
  ASTNode *head = NULL, *last = NULL;
  for (;;) {
    skip_newlines(p);
    if (match(p, ENLNG_TOKEN_DEDENT) || check(p, ENLNG_TOKEN_EOF))
      break;
    Token *ct = peek(p, 0);
    if (match(p, ENLNG_TOKEN_OTHERWISE)) {
      ASTNode **body = NULL;
      int count = 0;
      parse_block(p, &body, &count);
      if (!last) {
        set_error(p, "'otherwise' comes after the values, as the last line", ct->line);
        return NULL;
      }
      last->as.when_stmt.else_body = body;
      last->as.when_stmt.else_count = count;
      continue;
    }
    ASTNode *cond = NULL;
    do {
      ASTNode *eq = binary_node(ENLNG_TOKEN_EQ, var_node(tmp, ct->line), parse_additive(p), ct->line);
      cond = cond ? binary_node(ENLNG_TOKEN_OR, cond, eq, ct->line) : eq;
    } while (match(p, ENLNG_TOKEN_OR) || match(p, ENLNG_TOKEN_COMMA));
    ASTNode *w = ast_new(AST_WHEN, ct->line);
    w->as.when_stmt.condition = cond;
    parse_block(p, &w->as.when_stmt.then_body, &w->as.when_stmt.then_count);
    if (p->has_error)
      return NULL;
    if (!head) {
      head = w;
    } else {
      last->as.when_stmt.else_body = (ASTNode **)malloc(sizeof(ASTNode *));
      last->as.when_stmt.else_body[0] = w;
      last->as.when_stmt.else_count = 1;
    }
    last = w;
  }
  if (head)
    oop_queue_extra(head);
  return first;
}

static ASTNode *parse_statement(Parser *p) {
  skip_newlines(p);
  Token *t = peek(p, 0);

  /* class Name [inherits from A, B]: fields and methods */
  if (oop_starts_class(p))
    return parse_class_def(p);

  /* global total, count: inside a function, `total = ...` then changes the
     name made at the top of the program instead of making a new one */
  if (t->type == ENLNG_TOKEN_IDENTIFIER && strcmp(t->text, "global") == 0 &&
      peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER) {
    advance(p);
    ASTNode *g = ast_new(AST_GLOBAL, t->line);
    int cap = 4;
    g->as.global_stmt.names = (char **)malloc(sizeof(char *) * cap);
    do {
      Token *nt = peek(p, 0);
      if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
        name_error(p, "Expected a name after 'global'", t->line);
        return g;
      }
      if (g->as.global_stmt.count >= cap) {
        cap *= 2;
        g->as.global_stmt.names =
            (char **)realloc(g->as.global_stmt.names, sizeof(char *) * cap);
      }
      g->as.global_stmt.names[g->as.global_stmt.count++] = enlng_strdup(nt->text);
    } while (match(p, ENLNG_TOKEN_COMMA));
    return g;
  }

  /* stop the program */
  if (t->type == ENLNG_TOKEN_STOP && tok_is(peek(p, 1), "program")) {
    advance(p);
    advance(p);
    return expr_stmt_node(oop_call_node("stop_program", NULL, 0, t->line), t->line);
  }

  /* check that total is 10 -- stops with a message when it does not hold */
  if (tok_is(t, "check") && peek(p, 1)->type != ENLNG_TOKEN_LPAREN &&
      peek(p, 1)->type != ENLNG_TOKEN_ASSIGN && peek(p, 1)->type != ENLNG_TOKEN_AS &&
      peek(p, 1)->type != ENLNG_TOKEN_NEWLINE) {
    advance(p);
    ASTNode *cond = parse_expression(p);
    ASTNode *w = ast_new(AST_WHEN, t->line);
    ASTNode *f = ast_new(AST_FAIL, t->line);
    if (cond && cond->type == AST_EXPR_BINARY && cond->as.binary_expr.op == ENLNG_TOKEN_EQ) {
      /* keep both sides, so the message can say what came out */
      char *l = hidden_name("check_got");
      char *r = hidden_name("check_want");
      ASTNode *first = decl_node(l, cond->as.binary_expr.left, false, t->line);
      oop_queue_extra(decl_node(r, cond->as.binary_expr.right, false, t->line));
      w->as.when_stmt.condition =
          not_node(binary_node(ENLNG_TOKEN_EQ, var_node(l, t->line), var_node(r, t->line), t->line), t->line);
      ASTNode *msg = binary_node(ENLNG_TOKEN_ADD, str_node("Expected ", t->line),
                                 call1("to_text", var_node(r, t->line), t->line), t->line);
      msg = binary_node(ENLNG_TOKEN_ADD, msg, str_node(" but got ", t->line), t->line);
      msg = binary_node(ENLNG_TOKEN_ADD, msg, call1("to_text", var_node(l, t->line), t->line), t->line);
      f->as.fail_stmt.message = msg;
      w->as.when_stmt.then_body = (ASTNode **)malloc(sizeof(ASTNode *));
      w->as.when_stmt.then_body[0] = f;
      w->as.when_stmt.then_count = 1;
      oop_queue_extra(w);
      return first;
    }
    w->as.when_stmt.condition = not_node(cond, t->line);
    f->as.fail_stmt.message = str_node("This check did not hold.", t->line);
    w->as.when_stmt.then_body = (ASTNode **)malloc(sizeof(ASTNode *));
    w->as.when_stmt.then_body[0] = f;
    w->as.when_stmt.then_count = 1;
    return w;
  }

  /* test "adding works": a named group of checks */
  if (tok_is(t, "test") && peek(p, 1)->type == ENLNG_TOKEN_STRING_LITERAL &&
      peek(p, 2)->type == ENLNG_TOKEN_COLON) {
    advance(p);
    const char *name = advance(p)->text;
    ASTNode *n = ast_new(AST_TRY, t->line);
    ASTNode **body = NULL;
    int count = 0;
    parse_block(p, &body, &count);
    body = (ASTNode **)realloc(body, sizeof(ASTNode *) * (size_t)(count + 1));
    body[count++] = expr_stmt_node(
        call3("test_result", str_node(name, t->line), bool_node(true, t->line),
              str_node("", t->line), t->line), t->line);
    n->as.try_stmt.body = body;
    n->as.try_stmt.body_count = count;
    n->as.try_stmt.err_name = hidden_name("test_problem");
    n->as.try_stmt.handler = (ASTNode **)malloc(sizeof(ASTNode *));
    n->as.try_stmt.handler[0] = expr_stmt_node(
        call3("test_result", str_node(name, t->line), bool_node(false, t->line),
              var_node(n->as.try_stmt.err_name, t->line), t->line), t->line);
    n->as.try_stmt.handler_count = 1;
    return n;
  }

  /* write the data to "copy.png" / send "hello" to the client */
  if ((tok_is(t, "write") || tok_is(t, "send")) &&
      peek(p, 1)->type != ENLNG_TOKEN_ASSIGN && peek(p, 1)->type != ENLNG_TOKEN_AS &&
      peek(p, 1)->type != ENLNG_TOKEN_LPAREN && has_token_on_line(p, ENLNG_TOKEN_TO)) {
    advance(p);
    ASTNode *what = parse_or_expr(p);
    if (!match(p, ENLNG_TOKEN_TO)) {
      set_error(p, t->text[0] == 'w' ? "Expected 'to' and a file name, as in: write the data to \"copy.png\""
                                     : "Expected 'to' and a connection, as in: send \"hello\" to the client",
                t->line);
      return NULL;
    }
    ASTNode *where = parse_or_expr(p);
    return expr_stmt_node(t->text[0] == 'w' ? call2("write_data", what, where, t->line)
                                            : call2("net_send", what, where, t->line),
                          t->line);
  }

  /* close the client */
  if (tok_is(t, "close") && (peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER) &&
      (peek(p, 2)->type == ENLNG_TOKEN_NEWLINE || peek(p, 2)->type == ENLNG_TOKEN_EOF ||
       peek(p, 2)->type == ENLNG_TOKEN_DEDENT)) {
    advance(p);
    return expr_stmt_node(call1("net_close", var_node(advance(p)->text, t->line), t->line), t->line);
  }

  /* serve answer on port 8080 */
  if (tok_is(t, "serve") && peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER &&
      peek(p, 2)->type == ENLNG_TOKEN_ON && tok_is(peek(p, 3), "port")) {
    advance(p);
    const char *fn = advance(p)->text;
    advance(p);
    advance(p);
    return expr_stmt_node(call2("serve", var_node(fn, t->line), parse_additive(p), t->line), t->line);
  }

  /* call m on obj / tell obj to m / ask obj to m, as a statement */
  if (oop_starts_call(p)) {
    ASTNode *e = ast_new(AST_EXPR_STMT, t->line);
    e->as.expr_stmt.expr = parse_oop_call(p);
    return e;
  }

  /* 0. Module Import: use "filename.enlng" / import "filename.enlng" */
  if (match(p, ENLNG_TOKEN_USE)) {
    Token *file_tok = peek(p, 0);
    if (!match(p, ENLNG_TOKEN_STRING_LITERAL)) {
      set_error(p, "Expected string literal path after 'use'", t->line);
      return NULL;
    }
    ASTNode *n = ast_new(AST_USE, t->line);
    n->as.use_stmt.filename = enlng_strdup(file_tok->text);
    if (match(p, ENLNG_TOKEN_AS)) {
      Token *at = peek(p, 0);
      if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
        name_error(p, "Expected a name after 'as', as in: use \"tools.enlng\" as tools",
                   t->line);
        return NULL;
      }
      n->as.use_stmt.alias = enlng_strdup(at->text);
    }
    return n;
  }

  /* 1. Declarations: remember x as 10 / freeze PI as 3.14 */
  if (check(p, ENLNG_TOKEN_REMEMBER) || check(p, ENLNG_TOKEN_FREEZE)) {
    bool is_frozen = (t->type == ENLNG_TOKEN_FREEZE);
    advance(p);

    Token *id_tok = peek(p, 0);
    if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
      name_error(p, "Expected variable name after remember/freeze", t->line);
      return NULL;
    }
    char *name = enlng_strdup(id_tok->text);

    /* remember the name and the age from the person */
    if ((check(p, ENLNG_TOKEN_AND) || check(p, ENLNG_TOKEN_COMMA)) &&
        peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER) {
      int cap = 4, count = 1;
      char **names = (char **)malloc(sizeof(char *) * cap);
      names[0] = name;
      while ((check(p, ENLNG_TOKEN_AND) || check(p, ENLNG_TOKEN_COMMA)) &&
             peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER) {
        advance(p);
        if (count >= cap) {
          cap *= 2;
          names = (char **)realloc(names, sizeof(char *) * cap);
        }
        names[count++] = enlng_strdup(advance(p)->text);
      }
      if (!match(p, ENLNG_TOKEN_FROM) && !match(p, ENLNG_TOKEN_AS) &&
          !match(p, ENLNG_TOKEN_ASSIGN)) {
        set_error(p, "Expected 'from' and a list or map, as in: remember name and age from person",
                  t->line);
        return NULL;
      }
      char *whole = hidden_name("parts");
      ASTNode *first = decl_node(whole, parse_expression(p), false, t->line);
      for (int i = 0; i < count; i++)
        oop_queue_extra(decl_node(names[i],
                                  call3("take_part", var_node(whole, t->line),
                                        int_node(i, t->line), str_node(names[i], t->line), t->line),
                                  is_frozen, t->line));
      return first;
    }

    if (!match(p, ENLNG_TOKEN_AS) && !match(p, ENLNG_TOKEN_ASSIGN)) {
      set_error(p, "Expected 'as' or '=' in declaration", t->line);
    }

    ASTNode *init_expr = parse_expression(p);
    ASTNode *n = ast_new(AST_DECLARATION, t->line);
    n->as.decl.name = name;
    n->as.decl.is_frozen = is_frozen;
    n->as.decl.init_expr = init_expr;
    return n;
  }

  /* 2. Variable Assignment: change x to 20 */
  if (match(p, ENLNG_TOKEN_CHANGE)) {
    Token *id_tok = peek(p, 0);
    if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
      name_error(p, "Expected variable name after 'change'", t->line);
      return NULL;
    }
    char *name = enlng_strdup(id_tok->text);
    if (match(p, ENLNG_TOKEN_OF)) {
      Token *obj_tok = peek(p, 0);
      if (!match(p, ENLNG_TOKEN_IDENTIFIER) && !match(p, ENLNG_TOKEN_PAIR)) {
        set_error(p, "Expected object name after 'of' in change", t->line);
        return NULL;
      }
      char *obj_name = enlng_strdup(obj_tok->text);
      if (!match(p, ENLNG_TOKEN_TO) && !match(p, ENLNG_TOKEN_ASSIGN)) {
        set_error(p, "Expected 'to' or '=' after object in change", t->line);
      }
      ASTNode *val_expr = parse_expression(p);
      ASTNode *n = ast_new(AST_INDEX_MUTATION, t->line);
      n->as.index_mutation.arr_name = obj_name;
      ASTNode *key_node = ast_new(AST_EXPR_LITERAL_STRING, t->line);
      key_node->as.literal.string_val = name;
      n->as.index_mutation.index_expr = key_node;
      n->as.index_mutation.op = ENLNG_TOKEN_ASSIGN;
      n->as.index_mutation.val_expr = val_expr;
      return n;
    }
    if (match(p, ENLNG_TOKEN_AT)) {
      ASTNode *idx = parse_primary(p);
      if (!match(p, ENLNG_TOKEN_TO) && !match(p, ENLNG_TOKEN_ASSIGN)) {
        set_error(p, "Expected 'to' or '=' after index in change", t->line);
      }
      ASTNode *val_expr = parse_expression(p);
      ASTNode *n = ast_new(AST_INDEX_MUTATION, t->line);
      n->as.index_mutation.arr_name = name;
      n->as.index_mutation.index_expr = idx;
      n->as.index_mutation.op = ENLNG_TOKEN_ASSIGN;
      n->as.index_mutation.val_expr = val_expr;
      return n;
    }
    if (!match(p, ENLNG_TOKEN_TO) && !match(p, ENLNG_TOKEN_ASSIGN)) {
      set_error(p, "Expected 'to' or '=' after variable name in change",
                t->line);
    }
    ASTNode *val_expr = parse_expression(p);
    ASTNode *n = ast_new(AST_MUTATION, t->line);
    n->as.mutation.name = name;
    n->as.mutation.op = ENLNG_TOKEN_ASSIGN;
    n->as.mutation.val_expr = val_expr;
    return n;
  }

  /* try: block, then `on error [as name]:` block. `try` and `error` are read
     as words only here, so they stay usable as names elsewhere. */
  if (t->type == ENLNG_TOKEN_IDENTIFIER && t->text &&
      strcmp(t->text, "try") == 0 && peek(p, 1)->type == ENLNG_TOKEN_COLON) {
    advance(p); /* try */
    ASTNode *n = ast_new(AST_TRY, t->line);
    parse_block(p, &n->as.try_stmt.body, &n->as.try_stmt.body_count);
    skip_newlines(p);
    Token *on_tok = peek(p, 0);
    Token *err_tok = peek(p, 1);
    if (on_tok->type != ENLNG_TOKEN_ON ||
        err_tok->type != ENLNG_TOKEN_IDENTIFIER || !err_tok->text ||
        strcmp(err_tok->text, "error") != 0) {
      set_error(p, "A 'try' block needs an 'on error:' block right after it, for example:  on error as problem:",
                t->line);
      return NULL;
    }
    advance(p); /* on */
    advance(p); /* error */
    n->as.try_stmt.err_name = enlng_strdup("error");
    if (match(p, ENLNG_TOKEN_AS)) {
      Token *name_tok = peek(p, 0);
      if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
        name_error(p, "Expected a name after 'on error as'", t->line);
        return NULL;
      }
      free(n->as.try_stmt.err_name);
      n->as.try_stmt.err_name = enlng_strdup(name_tok->text);
    }
    parse_block(p, &n->as.try_stmt.handler, &n->as.try_stmt.handler_count);
    return n;
  }

  /* fail "message": stop the current try block (or the program) with an error */
  if (t->type == ENLNG_TOKEN_IDENTIFIER && t->text &&
      strcmp(t->text, "fail") == 0 &&
      (peek(p, 1)->type == ENLNG_TOKEN_STRING_LITERAL ||
       peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER ||
       peek(p, 1)->type == ENLNG_TOKEN_INT_LITERAL)) {
    advance(p); /* fail */
    ASTNode *n = ast_new(AST_FAIL, t->line);
    n->as.fail_stmt.message = parse_expression(p);
    return n;
  }

  /* when the color is:
         "red": show "stop"
         "green" or "blue": show "go"
         otherwise: show "wait"            */
  if (check(p, ENLNG_TOKEN_WHEN)) {
    int k = 1;
    while (peek(p, k)->type != ENLNG_TOKEN_COLON && peek(p, k)->type != ENLNG_TOKEN_NEWLINE &&
           peek(p, k)->type != ENLNG_TOKEN_EOF)
      k++;
    if (peek(p, k)->type == ENLNG_TOKEN_COLON && k >= 3 &&
        peek(p, k - 1)->type == ENLNG_TOKEN_EQ && peek(p, k + 1)->type == ENLNG_TOKEN_NEWLINE)
      return parse_choose_by_value(p);
  }

  /* 3. Conditionals: when cond: block [otherwise: block] */
  if (match(p, ENLNG_TOKEN_WHEN)) {
    ASTNode *cond = parse_expression(p);
    ASTNode *n = ast_new(AST_WHEN, t->line);
    n->as.when_stmt.condition = cond;
    parse_block(p, &n->as.when_stmt.then_body, &n->as.when_stmt.then_count);

    skip_newlines(p);
    if (match(p, ENLNG_TOKEN_OTHERWISE)) {
      if (check(p, ENLNG_TOKEN_WHEN)) {
        ASTNode *nested_when = parse_statement(p);
        n->as.when_stmt.else_body = (ASTNode **)malloc(sizeof(ASTNode *));
        n->as.when_stmt.else_body[0] = nested_when;
        n->as.when_stmt.else_count = 1;
      } else {
        parse_block(p, &n->as.when_stmt.else_body, &n->as.when_stmt.else_count);
      }
    }
    return n;
  }

  /* 4. Loops: for each pair in list / for item in list: block */
  if (match(p, ENLNG_TOKEN_FOR)) {
    bool has_each = match(p, ENLNG_TOKEN_EACH);

    if (has_each && match(p, ENLNG_TOKEN_PAIR)) {
      /* for each pair in <expr>: */
      char *pair_name = enlng_strdup("pair");
      if (match(p, ENLNG_TOKEN_LPAREN)) {
        /* optional (a, b) notation */
        while (!match(p, ENLNG_TOKEN_RPAREN) && !check(p, ENLNG_TOKEN_EOF))
          advance(p);
      }
      if (!match(p, ENLNG_TOKEN_IN)) {
        set_error(p, "Expected 'in' after 'pair'", t->line);
      }
      ASTNode *list_expr = parse_expression(p);
      char *list_name = NULL;
      if (list_expr && list_expr->type == AST_EXPR_VARIABLE) {
        list_name = enlng_strdup(list_expr->as.variable.name);
      }

      ASTNode *n = ast_new(AST_FOR_PAIR, t->line);
      n->as.for_pair.pair_name = pair_name;
      n->as.for_pair.list_name = list_name ? list_name : enlng_strdup("_pair_list");
      parse_block(p, &n->as.for_pair.body, &n->as.for_pair.body_count);
      return n;
    }

    Token *var_tok = peek(p, 0);
    if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
      name_error(p, "Expected variable name after 'for'", t->line);
      return NULL;
    }
    char *var_name = enlng_strdup(var_tok->text);

    /* for each name and score in results / for each key and value in prices */
    if ((check(p, ENLNG_TOKEN_AND) || check(p, ENLNG_TOKEN_COMMA)) &&
        peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER) {
      int cap = 4, count = 1;
      char **names = (char **)malloc(sizeof(char *) * cap);
      names[0] = var_name;
      while ((check(p, ENLNG_TOKEN_AND) || check(p, ENLNG_TOKEN_COMMA)) &&
             peek(p, 1)->type == ENLNG_TOKEN_IDENTIFIER) {
        advance(p);
        if (count >= cap) {
          cap *= 2;
          names = (char **)realloc(names, sizeof(char *) * cap);
        }
        names[count++] = enlng_strdup(advance(p)->text);
      }
      if (!match(p, ENLNG_TOKEN_IN)) {
        set_error(p, "Expected 'in' and a list, as in: for each name and score in results",
                  t->line);
        return NULL;
      }
      ASTNode *coll = parse_expression(p);
      char *item = hidden_name("item");
      ASTNode *n = ast_new(AST_FOR_EACH, t->line);
      n->as.for_each.item_name = item;
      n->as.for_each.list_name = enlng_strdup("_each_coll");
      n->as.for_each.list_expr = call1("pairs_of", coll, t->line);
      ASTNode **body = NULL;
      int bc = 0;
      parse_block(p, &body, &bc);
      ASTNode **nb = (ASTNode **)malloc(sizeof(ASTNode *) * (size_t)(bc + count));
      for (int i = 0; i < count; i++)
        nb[i] = decl_node(names[i],
                          call3("take_part", var_node(item, t->line), int_node(i, t->line),
                                str_node(names[i], t->line), t->line),
                          false, t->line);
      for (int i = 0; i < bc; i++)
        nb[count + i] = body[i];
      n->as.for_each.body = nb;
      n->as.for_each.body_count = bc + count;
      return n;
    }

    /* Case A: Range loop: for i from 1 to 5 [by 1]: OR for i in 1 to 5 [by 1]: */
    if (match(p, ENLNG_TOKEN_FROM) || (check(p, ENLNG_TOKEN_IN) && has_token_on_line(p, ENLNG_TOKEN_TO))) {
      if (check(p, ENLNG_TOKEN_IN)) {
        advance(p); /* consume 'in' */
      }
      ASTNode *start_expr = parse_expression(p);
      if (!match(p, ENLNG_TOKEN_TO)) {
        set_error(p, "Expected 'to' in for range loop", t->line);
      }
      ASTNode *end_expr = parse_expression(p);
      ASTNode *step_expr = NULL;
      if (match(p, ENLNG_TOKEN_BY)) {
        step_expr = parse_expression(p);
      }

      ASTNode *n = ast_new(AST_FOR_RANGE, t->line);
      n->as.for_range.var_name = var_name;
      n->as.for_range.start_expr = start_expr;
      n->as.for_range.end_expr = end_expr;
      n->as.for_range.step_expr = step_expr;
      parse_block(p, &n->as.for_range.body, &n->as.for_range.body_count);
      return n;
    }

    /* Case B: Collection loop: for [each] item in <expr>: */
    if (!match(p, ENLNG_TOKEN_IN)) {
      set_error(p, "Expected 'in' or 'from' after loop variable", t->line);
      return NULL;
    }

    ASTNode *coll_expr = parse_expression(p);
    char *list_name = NULL;
    if (coll_expr && coll_expr->type == AST_EXPR_VARIABLE) {
      list_name = enlng_strdup(coll_expr->as.variable.name);
    }

    ASTNode *n = ast_new(AST_FOR_EACH, t->line);
    n->as.for_each.item_name = var_name;
    n->as.for_each.list_name = list_name ? list_name : enlng_strdup("_each_coll");
    n->as.for_each.list_expr = coll_expr;
    parse_block(p, &n->as.for_each.body, &n->as.for_each.body_count);
    return n;
  }

  /* 5. Loops: repeat while / repeat until */
  if (match(p, ENLNG_TOKEN_REPEAT)) {
    if (match(p, ENLNG_TOKEN_WHILE)) {
      ASTNode *cond = parse_expression(p);
      ASTNode *n = ast_new(AST_REPEAT_WHILE, t->line);
      n->as.repeat_loop.condition = cond;
      parse_block(p, &n->as.repeat_loop.body, &n->as.repeat_loop.body_count);
      return n;
    }
    if (match(p, ENLNG_TOKEN_UNTIL)) {
      ASTNode *cond = parse_expression(p);
      ASTNode *n = ast_new(AST_REPEAT_UNTIL, t->line);
      n->as.repeat_loop.condition = cond;
      parse_block(p, &n->as.repeat_loop.body, &n->as.repeat_loop.body_count);
      return n;
    }
    /* repeat N times: */
    ASTNode *cnt = parse_expression(p);
    bool is_times = false;
    if (match(p, ENLNG_TOKEN_TIMES)) {
      is_times = true;
    } else if (check(p, ENLNG_TOKEN_IDENTIFIER) && strcmp(peek(p, 0)->text, "times") == 0) {
      advance(p);
      is_times = true;
    }
    if (is_times) {
      ASTNode *n = ast_new(AST_REPEAT_TIMES, t->line);
      n->as.repeat_times.count_expr = cnt;
      parse_block(p, &n->as.repeat_times.body, &n->as.repeat_times.body_count);
      return n;
    }
  }

  /* Direct while: while left < right: or 'repeat while' */
  if (match(p, ENLNG_TOKEN_WHILE)) {
    ASTNode *cond = parse_expression(p);
    ASTNode *n = ast_new(AST_REPEAT_WHILE, t->line);
    n->as.repeat_loop.condition = cond;
    parse_block(p, &n->as.repeat_loop.body, &n->as.repeat_loop.body_count);
    return n;
  }

  /* Direct until: until sorted: or 'repeat until' */
  if (match(p, ENLNG_TOKEN_UNTIL)) {
    ASTNode *cond = parse_expression(p);
    ASTNode *n = ast_new(AST_REPEAT_UNTIL, t->line);
    n->as.repeat_loop.condition = cond;
    parse_block(p, &n->as.repeat_loop.body, &n->as.repeat_loop.body_count);
    return n;
  }

  /* 6. Functions: function name with params: block */
  if (match(p, ENLNG_TOKEN_FUNCTION)) {
    Token *name_tok = peek(p, 0);
    if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
      name_error(p, "Expected function name", t->line);
      return NULL;
    }
    char *name = enlng_strdup(name_tok->text);

    ParamList pl = {0};
    if (match(p, ENLNG_TOKEN_WITH)) {
      parse_param_list(p, &pl, false, t->line);
    } else if (match(p, ENLNG_TOKEN_LPAREN)) {
      parse_param_list(p, &pl, true, t->line);
      if (!match(p, ENLNG_TOKEN_RPAREN)) {
        set_error(p, "Expected ')' after function parameters", t->line);
      }
    }

    ASTNode *n = ast_new(AST_FUNCTION, t->line);
    n->as.func_def.name = name;
    parse_block(p, &n->as.func_def.body, &n->as.func_def.body_count);
    apply_params(n, &pl);
    return n;
  }

  /* 7. Return / Give */
  if (match(p, ENLNG_TOKEN_GIVE)) {
    ASTNode *n = ast_new(AST_GIVE, t->line);
    if (!check(p, ENLNG_TOKEN_NEWLINE) && !check(p, ENLNG_TOKEN_DEDENT) &&
        !check(p, ENLNG_TOKEN_EOF)) {
      n->as.give_stmt.expr = parse_expression(p);
    }
    return n;
  }

  /* 8. Loop Control: stop / skip */
  if (match(p, ENLNG_TOKEN_STOP)) {
    return ast_new(AST_STOP, t->line);
  }
  if (match(p, ENLNG_TOKEN_SKIP)) {
    return ast_new(AST_SKIP, t->line);
  }

  /* 9. Swap: swap a and b / swap pair */
  if (match(p, ENLNG_TOKEN_SWAP)) {
    ASTNode *n = ast_new(AST_SWAP, t->line);
    if (match(p, ENLNG_TOKEN_PAIR)) {
      n->as.swap_stmt.is_pair = true;
      n->as.swap_stmt.pair_name = enlng_strdup("pair");
      return n;
    }
    n->as.swap_stmt.is_pair = false;
    n->as.swap_stmt.left = parse_primary_internal(p, false);
    if (match(p, ENLNG_TOKEN_AND) || match(p, ENLNG_TOKEN_COMMA) ||
        match(p, ENLNG_TOKEN_WITH)) {
      /* optional 'and', ',', or 'with' */
    }
    n->as.swap_stmt.right = parse_primary(p);
    return n;
  }

  /* 10. Reverse Statement: reverse data */
  if (match(p, ENLNG_TOKEN_REVERSE)) {
    Token *target_tok = peek(p, 0);
    if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
      set_error(p, "Expected identifier after 'reverse'", t->line);
      return NULL;
    }
    ASTNode *n = ast_new(AST_REVERSE_STMT, t->line);
    n->as.reverse_stmt.target_name = enlng_strdup(target_tok->text);
    return n;
  }

  /* 10b. Sort Statement: sort list [ascending/descending] */
  if (match(p, ENLNG_TOKEN_SORT)) {
    Token *target_tok = peek(p, 0);
    if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
      set_error(p, "Expected identifier after 'sort'", t->line);
      return NULL;
    }
    char *target_name = enlng_strdup(target_tok->text);
    bool descending = false;
    if (check(p, ENLNG_TOKEN_IDENTIFIER)) {
      if (strcmp(peek(p, 0)->text, "descending") == 0) {
        descending = true;
        advance(p);
      } else if (strcmp(peek(p, 0)->text, "ascending") == 0) {
        descending = false;
        advance(p);
      }
    }
    ASTNode *n = ast_new(AST_SORT, t->line);
    n->as.sort_stmt.target_name = target_name;
    n->as.sort_stmt.descending = descending;
    return n;
  }

  /* 10c. Arrange Statement: arrange target [by|with|to|in|at|of|as] idx1, idx2, ... or [indices] or var */
  if (match(p, ENLNG_TOKEN_ARRANGE)) {
    Token *target_tok = peek(p, 0);
    if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
      set_error(p, "Expected identifier after 'arrange'", t->line);
      return NULL;
    }
    char *target_name = enlng_strdup(target_tok->text);
    if (match(p, ENLNG_TOKEN_BY) || match(p, ENLNG_TOKEN_WITH) ||
        match(p, ENLNG_TOKEN_TO) || match(p, ENLNG_TOKEN_IN) ||
        match(p, ENLNG_TOKEN_AT) || match(p, ENLNG_TOKEN_OF) ||
        match(p, ENLNG_TOKEN_AS)) {
      /* optional connector */
    }
    ASTNode *first_expr = parse_expression(p);
    ASTNode *indices_expr = NULL;
    if (match(p, ENLNG_TOKEN_COMMA)) {
      ASTNode *list_node = ast_new(AST_EXPR_LIST_LITERAL, t->line);
      int cap = 8;
      int count = 0;
      ASTNode **elems = (ASTNode **)malloc(sizeof(ASTNode *) * cap);
      elems[count++] = first_expr;
      do {
        if (count >= cap) {
          cap *= 2;
          elems = (ASTNode **)realloc(elems, sizeof(ASTNode *) * cap);
        }
        elems[count++] = parse_expression(p);
      } while (match(p, ENLNG_TOKEN_COMMA));
      list_node->as.list_literal.elements = elems;
      list_node->as.list_literal.count = count;
      indices_expr = list_node;
    } else {
      indices_expr = first_expr;
    }
    ASTNode *n = ast_new(AST_ARRANGE_STMT, t->line);
    n->as.arrange_stmt.target_name = target_name;
    n->as.arrange_stmt.indices_expr = indices_expr;
    return n;
  }

  /* 11. Output: show / display / print item1 item2 ... (No curly braces!) */
  if (match(p, ENLNG_TOKEN_SHOW)) {
    ASTNode *n = ast_new(AST_SHOW, t->line);
    int cap = 8;
    int count = 0;
    ASTNode **items = (ASTNode **)malloc(sizeof(ASTNode *) * cap);

    while (!check(p, ENLNG_TOKEN_NEWLINE) && !check(p, ENLNG_TOKEN_DEDENT) &&
           !check(p, ENLNG_TOKEN_EOF)) {
      if (count >= cap) {
        cap *= 2;
        items = (ASTNode **)realloc(items, sizeof(ASTNode *) * cap);
      }
      items[count++] = parse_expression(p);
      match(p, ENLNG_TOKEN_COMMA); /* optional comma between stream items */
    }
    n->as.show_stmt.items = items;
    n->as.show_stmt.count = count;
    return n;
  }

  /* 12. Input: ask var "Prompt: " */
  if (match(p, ENLNG_TOKEN_ASK)) {
    Token *var_tok = peek(p, 0);
    if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
      name_error(p, "Expected variable name after 'ask'", t->line);
      return NULL;
    }
    char *var_name = enlng_strdup(var_tok->text);
    char *prompt = enlng_strdup("");
    if (check(p, ENLNG_TOKEN_STRING_LITERAL)) {
      prompt = enlng_strdup(peek(p, 0)->text);
      advance(p);
    }
    ASTNode *n = ast_new(AST_ASK, t->line);
    n->as.ask_stmt.var_name = var_name;
    n->as.ask_stmt.prompt = prompt;
    return n;
  }

  /* 13. Mutation: x increases by 1 / x += 1 / x = 10 / arr[i] = val */
  if (check(p, ENLNG_TOKEN_IDENTIFIER)) {
    Token *id_tok = peek(p, 0);
    Token *next_tok = peek(p, 1);

    /* String Mutation Statement: string_remove from s at "1" */
    if (strcmp(id_tok->text, "string_remove") == 0 && next_tok->type != ENLNG_TOKEN_LPAREN) {
      advance(p); /* consume 'string_remove' */
      if (match(p, ENLNG_TOKEN_FROM) || match(p, ENLNG_TOKEN_IN) ||
          match(p, ENLNG_TOKEN_AT) || match(p, ENLNG_TOKEN_OF)) {
        /* optional preposition */
      }
      Token *var_tok = peek(p, 0);
      if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
        set_error(p, "Expected string variable after 'string_remove'", t->line);
        return NULL;
      }
      char *var_name = enlng_strdup(var_tok->text);
      if (match(p, ENLNG_TOKEN_AT) || match(p, ENLNG_TOKEN_FROM) ||
          match(p, ENLNG_TOKEN_IN)) {
        /* optional 'at' */
      }
      ASTNode *pos_expr = parse_unary(p);

      ASTNode *call = ast_new(AST_EXPR_CALL, t->line);
      call->as.call_expr.func_name = enlng_strdup("string_remove");
      call->as.call_expr.arg_count = 2;
      call->as.call_expr.args = (ASTNode **)malloc(sizeof(ASTNode *) * 2);

      ASTNode *var_node = ast_new(AST_EXPR_VARIABLE, t->line);
      var_node->as.variable.name = enlng_strdup(var_name);
      call->as.call_expr.args[0] = var_node;
      call->as.call_expr.args[1] = pos_expr;

      ASTNode *n = ast_new(AST_MUTATION, t->line);
      n->as.mutation.name = var_name;
      n->as.mutation.op = ENLNG_TOKEN_ASSIGN;
      n->as.mutation.val_expr = call;
      return n;
    }

    /* String Mutation Statement: string_replace "s" in s at "1" */
    if (strcmp(id_tok->text, "string_replace") == 0 && next_tok->type != ENLNG_TOKEN_LPAREN) {
      advance(p); /* consume 'string_replace' */
      ASTNode *new_val = parse_primary(p);
      if (match(p, ENLNG_TOKEN_IN) || match(p, ENLNG_TOKEN_FROM) ||
          match(p, ENLNG_TOKEN_BY) || match(p, ENLNG_TOKEN_TO) ||
          match(p, ENLNG_TOKEN_WITH) || match(p, ENLNG_TOKEN_AT)) {
        /* optional preposition */
      }
      Token *var_tok = peek(p, 0);
      if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
        set_error(p, "Expected string variable in 'string_replace'", t->line);
        return NULL;
      }
      char *var_name = enlng_strdup(var_tok->text);
      if (match(p, ENLNG_TOKEN_AT) || match(p, ENLNG_TOKEN_IN) ||
          match(p, ENLNG_TOKEN_BY) || match(p, ENLNG_TOKEN_TO)) {
        /* optional 'at' */
      }
      ASTNode *pos_expr = parse_unary(p);

      ASTNode *call = ast_new(AST_EXPR_CALL, t->line);
      call->as.call_expr.func_name = enlng_strdup("string_replace");
      call->as.call_expr.arg_count = 3;
      call->as.call_expr.args = (ASTNode **)malloc(sizeof(ASTNode *) * 3);

      call->as.call_expr.args[0] = new_val;
      ASTNode *var_node = ast_new(AST_EXPR_VARIABLE, t->line);
      var_node->as.variable.name = enlng_strdup(var_name);
      call->as.call_expr.args[1] = var_node;
      call->as.call_expr.args[2] = pos_expr;

      ASTNode *n = ast_new(AST_MUTATION, t->line);
      n->as.mutation.name = var_name;
      n->as.mutation.op = ENLNG_TOKEN_ASSIGN;
      n->as.mutation.val_expr = call;
      return n;
    }

    /* String Mutation Statement: string_add "p" in s at "7" */
    if ((strcmp(id_tok->text, "string_add") == 0 || strcmp(id_tok->text, "string_insert") == 0) &&
        next_tok->type != ENLNG_TOKEN_LPAREN) {
      advance(p); /* consume 'string_add' */
      ASTNode *item = parse_primary(p);
      if (match(p, ENLNG_TOKEN_IN) || match(p, ENLNG_TOKEN_FROM) ||
          match(p, ENLNG_TOKEN_BY) || match(p, ENLNG_TOKEN_TO) ||
          match(p, ENLNG_TOKEN_WITH) || match(p, ENLNG_TOKEN_AT)) {
        /* optional preposition */
      }
      Token *var_tok = peek(p, 0);
      if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
        set_error(p, "Expected string variable in 'string_add'", t->line);
        return NULL;
      }
      char *var_name = enlng_strdup(var_tok->text);
      if (match(p, ENLNG_TOKEN_AT) || match(p, ENLNG_TOKEN_IN) ||
          match(p, ENLNG_TOKEN_BY) || match(p, ENLNG_TOKEN_TO)) {
        /* optional 'at' */
      }
      ASTNode *pos_expr = parse_unary(p);

      ASTNode *call = ast_new(AST_EXPR_CALL, t->line);
      call->as.call_expr.func_name = enlng_strdup("string_add");
      call->as.call_expr.arg_count = 3;
      call->as.call_expr.args = (ASTNode **)malloc(sizeof(ASTNode *) * 3);

      call->as.call_expr.args[0] = item;
      ASTNode *var_node = ast_new(AST_EXPR_VARIABLE, t->line);
      var_node->as.variable.name = enlng_strdup(var_name);
      call->as.call_expr.args[1] = var_node;
      call->as.call_expr.args[2] = pos_expr;

      ASTNode *n = ast_new(AST_MUTATION, t->line);
      n->as.mutation.name = var_name;
      n->as.mutation.op = ENLNG_TOKEN_ASSIGN;
      n->as.mutation.val_expr = call;
      return n;
    }

    /* Case A: arr[i] = ... or arr[i] += ... */
    if (next_tok->type == ENLNG_TOKEN_LBRACKET) {
      advance(p); /* consume ident */
      advance(p); /* consume '[' */
      ASTNode *index_expr = parse_expression(p);
      if (check(p, ENLNG_TOKEN_COLON)) {
        set_error(p, "A slice such as items[1:3] is a copy and cannot be assigned to", t->line);
      } else if (!match(p, ENLNG_TOKEN_RBRACKET)) {
        set_error(p, "Expected ']' after index in assignment", t->line);
      }
      /* Nested: g[1][2] = v  ->  the container is the expression g[1]. */
      ASTNode *target_expr = NULL;
      while (check(p, ENLNG_TOKEN_LBRACKET) && !p->has_error) {
        ASTNode *base = target_expr;
        if (!base) {
          base = ast_new(AST_EXPR_VARIABLE, t->line);
          base->as.variable.name = enlng_strdup(id_tok->text);
        }
        ASTNode *inner = ast_new(AST_EXPR_INDEX, t->line);
        inner->as.index_expr.target = base;
        inner->as.index_expr.arr_name =
            (base->type == AST_EXPR_VARIABLE)
                ? enlng_strdup(base->as.variable.name)
                : NULL;
        inner->as.index_expr.index_expr = index_expr;
        target_expr = inner;
        advance(p); /* consume '[' */
        index_expr = parse_expression(p);
        if (!match(p, ENLNG_TOKEN_RBRACKET))
          set_error(p, "Expected ']' after index in assignment", t->line);
      }
      if (check(p, ENLNG_TOKEN_ASSIGN) || check(p, ENLNG_TOKEN_INC_BY) ||
          check(p, ENLNG_TOKEN_DEC_BY)) {
        EnlngTokenType op = advance(p)->type;
        ASTNode *val_expr = parse_expression(p);
        ASTNode *n = ast_new(AST_INDEX_MUTATION, t->line);
        n->as.index_mutation.arr_name =
            target_expr ? NULL : enlng_strdup(id_tok->text);
        n->as.index_mutation.target_expr = target_expr;
        n->as.index_mutation.index_expr = index_expr;
        n->as.index_mutation.op = op;
        n->as.index_mutation.val_expr = val_expr;
        return n;
      }
    }

    /* Case D: coll at index = ... or coll at index increases by ... */
    if (next_tok->type == ENLNG_TOKEN_AT) {
      advance(p); /* consume ident */
      advance(p); /* consume 'at' */
      ASTNode *index_expr = parse_unary(p);
      if (check(p, ENLNG_TOKEN_ASSIGN) || check(p, ENLNG_TOKEN_INC_BY) ||
          check(p, ENLNG_TOKEN_DEC_BY)) {
        EnlngTokenType op = advance(p)->type;
        ASTNode *val_expr = parse_expression(p);
        ASTNode *n = ast_new(AST_INDEX_MUTATION, t->line);
        n->as.index_mutation.arr_name = enlng_strdup(id_tok->text);
        n->as.index_mutation.index_expr = index_expr;
        n->as.index_mutation.op = op;
        n->as.index_mutation.val_expr = val_expr;
        return n;
      }
    }

    /* Case E: field of obj = ... or field of obj += ... */
    if (next_tok->type == ENLNG_TOKEN_OF) {
      Token *obj_tok = peek(p, 2);
      if (obj_tok->type == ENLNG_TOKEN_IDENTIFIER ||
          obj_tok->type == ENLNG_TOKEN_PAIR) {
        Token *op_tok = peek(p, 3);
        if (op_tok->type == ENLNG_TOKEN_ASSIGN ||
            op_tok->type == ENLNG_TOKEN_INC_BY ||
            op_tok->type == ENLNG_TOKEN_DEC_BY) {
          advance(p);                           /* field ident */
          advance(p);                           /* 'of' */
          advance(p);                           /* obj */
          EnlngTokenType op = advance(p)->type; /* op */
          ASTNode *val_expr = parse_expression(p);
          ASTNode *n = ast_new(AST_INDEX_MUTATION, t->line);
          n->as.index_mutation.arr_name = enlng_strdup(obj_tok->text);
          ASTNode *key_node = ast_new(AST_EXPR_LITERAL_STRING, t->line);
          key_node->as.literal.string_val = enlng_strdup(id_tok->text);
          n->as.index_mutation.index_expr = key_node;
          n->as.index_mutation.op = op;
          n->as.index_mutation.val_expr = val_expr;
          return n;
        }
      }
    }

    /* Case C: obj.field = ... or obj.field += ... */
    if (next_tok->type == ENLNG_TOKEN_DOT) {
      Token *field_tok = peek(p, 2);
      if (field_tok->type == ENLNG_TOKEN_IDENTIFIER) {
        Token *op_tok = peek(p, 3);
        if (op_tok->type == ENLNG_TOKEN_ASSIGN ||
            op_tok->type == ENLNG_TOKEN_INC_BY ||
            op_tok->type == ENLNG_TOKEN_DEC_BY) {
          advance(p);                           /* ident */
          advance(p);                           /* '.' */
          advance(p);                           /* field */
          EnlngTokenType op = advance(p)->type; /* op */
          ASTNode *val_expr = parse_expression(p);
          ASTNode *n = ast_new(AST_INDEX_MUTATION, t->line);
          n->as.index_mutation.arr_name = enlng_strdup(id_tok->text);
          ASTNode *key_node = ast_new(AST_EXPR_LITERAL_STRING, t->line);
          key_node->as.literal.string_val = enlng_strdup(field_tok->text);
          n->as.index_mutation.index_expr = key_node;
          n->as.index_mutation.op = op;
          n->as.index_mutation.val_expr = val_expr;
          return n;
        }
      }
    }

    /* Case B: x = ... or x increases by ... */
    if (next_tok->type == ENLNG_TOKEN_INC_BY ||
        next_tok->type == ENLNG_TOKEN_DEC_BY ||
        next_tok->type == ENLNG_TOKEN_ASSIGN) {
      advance(p);                           /* consume ident */
      EnlngTokenType op = advance(p)->type; /* consume operator */
      ASTNode *val_expr = parse_expression(p);

      ASTNode *n = ast_new(AST_MUTATION, t->line);
      n->as.mutation.name = enlng_strdup(id_tok->text);
      n->as.mutation.op = op;
      n->as.mutation.val_expr = val_expr;
      return n;
    }
  }

  /* 14. Fallback: Standalone Expression / Function Call / Implicit Return */
  ASTNode *expr = parse_expression(p);

  /* A lone word followed by a value on the same line ("say "hi"", "write total") is a
   * power word we do not know. Without this check the value is silently dropped and the
   * word reaches the C compiler, which reports a confusing C error. */
  if (expr && expr->type == AST_EXPR_VARIABLE && !p->has_error) {
    EnlngTokenType nt = peek(p, 0)->type;
    if (nt == ENLNG_TOKEN_STRING_LITERAL || nt == ENLNG_TOKEN_INT_LITERAL ||
        nt == ENLNG_TOKEN_FLOAT_LITERAL || nt == ENLNG_TOKEN_IDENTIFIER ||
        nt == ENLNG_TOKEN_BOOL_LITERAL) {
      char err[256];
      snprintf(err, sizeof(err), "Unknown word '%s'", expr->as.variable.name);
      set_error(p, err, t->line);
    }
  }
  ASTNode *n = ast_new(AST_EXPR_STMT, t->line);
  n->as.expr_stmt.expr = expr;
  return n;
}

/* ---------------------------------------------------------------------
 * Frozen names: `freeze NAME as value` may not be assigned to afterwards.
 * Checked once on the finished tree so the interpreter and the C backend
 * behave the same. A function body starts with no frozen names, because a
 * function cannot see names made outside it.
 * ------------------------------------------------------------------- */
typedef struct {
  const char **names;
  int count;
  int cap;
  bool pair_frozen; /* inside `for each pair in <frozen list>` */
} FrozenSet;

static bool frozen_has(FrozenSet *f, const char *name) {
  for (int i = 0; i < f->count; i++)
    if (strcmp(f->names[i], name) == 0)
      return true;
  return false;
}

static void frozen_add(FrozenSet *f, const char *name) {
  if (f->count >= f->cap) {
    f->cap = f->cap ? f->cap * 2 : 8;
    f->names = (const char **)realloc(f->names, sizeof(char *) * f->cap);
  }
  f->names[f->count++] = name;
}

static void frozen_refuse(Parser *p, const char *name, int line) {
  char err[256];
  snprintf(err, sizeof(err),
           "'%s' was declared with freeze, so it cannot be changed. "
           "Use remember instead if the value needs to change.",
           name);
  set_error(p, err, line);
}

/* A frozen list or map also keeps its contents: element assignment, sort,
 * reverse, arrange, swap and the in-place builtins append and pop are refused.
 * Only calls written directly on the name are seen; a function that is handed
 * the list can still change it. */
static void frozen_refuse_contents(Parser *p, const char *name, int line) {
  char err[256];
  snprintf(err, sizeof(err),
           "'%s' was declared with freeze, so its contents cannot be changed "
           "either. Use remember instead if the list or map needs to change.",
           name);
  set_error(p, err, line);
}

static void frozen_check_contents(Parser *p, FrozenSet *f, const char *name,
                                  int line) {
  if (name && !p->has_error && frozen_has(f, name))
    frozen_refuse_contents(p, name, line);
}

static void frozen_check_expr(Parser *p, FrozenSet *f, ASTNode *e) {
  if (!e || p->has_error)
    return;
  switch (e->type) {
  case AST_EXPR_BINARY:
    frozen_check_expr(p, f, e->as.binary_expr.left);
    frozen_check_expr(p, f, e->as.binary_expr.right);
    break;
  case AST_EXPR_UNARY:
    frozen_check_expr(p, f, e->as.unary_expr.operand);
    break;
  case AST_EXPR_INDEX:
    frozen_check_expr(p, f, e->as.index_expr.target);
    frozen_check_expr(p, f, e->as.index_expr.index_expr);
    break;
  case AST_EXPR_CALL:
    if (e->as.call_expr.arg_count > 0 && e->as.call_expr.args[0] &&
        e->as.call_expr.args[0]->type == AST_EXPR_VARIABLE &&
        e->as.call_expr.func_name &&
        (strcmp(e->as.call_expr.func_name, "append") == 0 ||
         strcmp(e->as.call_expr.func_name, "pop") == 0))
      frozen_check_contents(p, f, e->as.call_expr.args[0]->as.variable.name,
                            e->line);
    for (int i = 0; i < e->as.call_expr.arg_count; i++)
      frozen_check_expr(p, f, e->as.call_expr.args[i]);
    break;
  case AST_EXPR_LIST_LITERAL:
    for (int i = 0; i < e->as.list_literal.count; i++)
      frozen_check_expr(p, f, e->as.list_literal.elements[i]);
    break;
  case AST_EXPR_MAP_LITERAL:
    for (int i = 0; i < e->as.map_literal.count; i++)
      frozen_check_expr(p, f, e->as.map_literal.values[i]);
    break;
  default:
    break;
  }
}

static void frozen_check_block(Parser *p, ASTNode **stmts, int count,
                               FrozenSet *f) {
  int keep = f->count; /* names frozen inside this block end with it */
  for (int i = 0; i < count && !p->has_error; i++) {
    ASTNode *n = stmts[i];
    if (!n)
      continue;
    switch (n->type) {
    case AST_DECLARATION:
      frozen_check_expr(p, f, n->as.decl.init_expr);
      if (p->has_error)
        break;
      if (frozen_has(f, n->as.decl.name))
        frozen_refuse(p, n->as.decl.name, n->line);
      else if (n->as.decl.is_frozen)
        frozen_add(f, n->as.decl.name);
      break;
    case AST_MUTATION:
      frozen_check_expr(p, f, n->as.mutation.val_expr);
      if (p->has_error)
        break;
      if (frozen_has(f, n->as.mutation.name))
        frozen_refuse(p, n->as.mutation.name, n->line);
      break;
    case AST_INDEX_MUTATION:
      frozen_check_expr(p, f, n->as.index_mutation.index_expr);
      frozen_check_expr(p, f, n->as.index_mutation.val_expr);
      frozen_check_contents(p, f, n->as.index_mutation.arr_name, n->line);
      break;
    case AST_SORT:
      frozen_check_contents(p, f, n->as.sort_stmt.target_name, n->line);
      break;
    case AST_REVERSE_STMT:
      frozen_check_contents(p, f, n->as.reverse_stmt.target_name, n->line);
      break;
    case AST_ARRANGE_STMT:
      frozen_check_contents(p, f, n->as.arrange_stmt.target_name, n->line);
      break;
    case AST_SWAP:
      if (n->as.swap_stmt.is_pair) {
        if (f->pair_frozen)
          frozen_refuse_contents(p, "the list being looped over", n->line);
      } else {
        ASTNode *l = n->as.swap_stmt.left;
        ASTNode *r = n->as.swap_stmt.right;
        if (l && l->type == AST_EXPR_INDEX)
          frozen_check_contents(p, f, l->as.index_expr.arr_name, n->line);
        if (r && r->type == AST_EXPR_INDEX)
          frozen_check_contents(p, f, r->as.index_expr.arr_name, n->line);
      }
      break;
    case AST_EXPR_STMT:
      frozen_check_expr(p, f, n->as.expr_stmt.expr);
      break;
    case AST_GIVE:
      frozen_check_expr(p, f, n->as.give_stmt.expr);
      break;
    case AST_SHOW:
      for (int k = 0; k < n->as.show_stmt.count; k++)
        frozen_check_expr(p, f, n->as.show_stmt.items[k]);
      break;
    case AST_WHEN:
      frozen_check_expr(p, f, n->as.when_stmt.condition);
      frozen_check_block(p, n->as.when_stmt.then_body,
                         n->as.when_stmt.then_count, f);
      frozen_check_block(p, n->as.when_stmt.else_body,
                         n->as.when_stmt.else_count, f);
      break;
    case AST_TRY:
      frozen_check_block(p, n->as.try_stmt.body, n->as.try_stmt.body_count, f);
      frozen_check_block(p, n->as.try_stmt.handler,
                         n->as.try_stmt.handler_count, f);
      break;
    case AST_FAIL:
      frozen_check_expr(p, f, n->as.fail_stmt.message);
      break;
    case AST_FOR_RANGE:
      frozen_check_expr(p, f, n->as.for_range.start_expr);
      frozen_check_expr(p, f, n->as.for_range.end_expr);
      frozen_check_expr(p, f, n->as.for_range.step_expr);
      frozen_check_block(p, n->as.for_range.body, n->as.for_range.body_count,
                         f);
      break;
    case AST_FOR_PAIR: {
      bool saved = f->pair_frozen;
      f->pair_frozen = n->as.for_pair.list_name &&
                       frozen_has(f, n->as.for_pair.list_name);
      frozen_check_block(p, n->as.for_pair.body, n->as.for_pair.body_count, f);
      f->pair_frozen = saved;
      break;
    }
    case AST_FOR_EACH:
      frozen_check_expr(p, f, n->as.for_each.list_expr);
      frozen_check_block(p, n->as.for_each.body, n->as.for_each.body_count, f);
      break;
    case AST_REPEAT_WHILE:
    case AST_REPEAT_UNTIL:
      frozen_check_expr(p, f, n->as.repeat_loop.condition);
      frozen_check_block(p, n->as.repeat_loop.body,
                         n->as.repeat_loop.body_count, f);
      break;
    case AST_REPEAT_TIMES:
      frozen_check_expr(p, f, n->as.repeat_times.count_expr);
      frozen_check_block(p, n->as.repeat_times.body,
                         n->as.repeat_times.body_count, f);
      break;
    case AST_FUNCTION: {
      FrozenSet inner = {0, 0, 0};
      frozen_check_block(p, n->as.func_def.body, n->as.func_def.body_count,
                         &inner);
      free(inner.names);
      break;
    }
    default:
      break;
    }
  }
  f->count = keep;
}

/* ---------------------------------------------------------------------
 * Using other files: use "tools.enlng" and use "tools.enlng" as tools
 *
 * A file is looked for next to the file that uses it, then in the folder
 * the program runs in, then in the standard library. Used without a name,
 * its functions join the program (once, however often it is used). Used
 * with a name, its functions and top-level names get that name in front:
 * tools.clean(x) calls the clean of tools.enlng, and it cannot clash with
 * a clean of the program or of another file.
 * ------------------------------------------------------------------ */
static char g_src_dir[1024] = "";
static NameSet g_loaded = {0};

typedef struct {
  char *alias;
  NameSet names;
} ModAlias;
static ModAlias g_mods[64];
static int g_nmods = 0;

static void use_set_dir(const char *file) {
  snprintf(g_src_dir, sizeof(g_src_dir), "%s", file);
  char *cut = NULL;
  for (char *q = g_src_dir; *q; q++)
    if (*q == '/' || *q == '\\')
      cut = q;
  if (cut)
    cut[1] = '\0';
  else
    g_src_dir[0] = '\0';
}

void parser_set_source_file(const char *path) { use_set_dir(path ? path : ""); }

static char *use_read(const char *name, char *found, size_t sz) {
  bool absolute = name[0] == '/' || name[0] == '\\' || (name[0] && name[1] == ':');
  if (!absolute && g_src_dir[0]) {
    snprintf(found, sz, "%s%s", g_src_dir, name);
    FILE *f = fopen(found, "rb");
    if (f) {
      fclose(f);
      return enlng_read_file_string(found);
    }
  }
  snprintf(found, sz, "%s", name);
  return enlng_read_file_string(name);
}

/* Names made by the language itself (class methods and helpers) have a
 * double underscore and are never given a file's name. */
static bool mod_generated(const char *name) { return strstr(name, "__") != NULL; }

typedef struct {
  NameSet *top;
  NameSet *locals;
  const char *alias;
} RenameCtx;

static void rename_name(char **name, void *ctx) {
  RenameCtx *c = (RenameCtx *)ctx;
  if (!*name || !ns_has(c->top, *name) || ns_has(c->locals, *name))
    return;
  char buf[300];
  snprintf(buf, sizeof(buf), "%s__%s", c->alias, *name);
  *name = enlng_strdup(buf);
}

/* The names a function uses as its own: inputs and names it sets, except
 * those it declares global. */
static void fn_locals(ASTNode *fn, NameSet *out) {
  NameSet all = {0}, glob = {0};
  for (int k = 0; k < fn->as.func_def.param_count; k++)
    ns_add(&all, fn->as.func_def.params[k]);
  ast_body_slots(fn->as.func_def.body, fn->as.func_def.body_count, assigned_slot, &all);
  ast_body_slots(fn->as.func_def.body, fn->as.func_def.body_count, global_slot, &glob);
  for (int k = 0; k < all.n; k++)
    if (!ns_has(&glob, all.v[k]))
      ns_add(out, all.v[k]);
  free(all.v);
  free(glob.v);
}

static void rename_slot(ASTNode **slot, void *ctx) {
  ASTNode *n = *slot;
  RenameCtx *c = (RenameCtx *)ctx;
  if (!n)
    return;
  if (n->type == AST_FUNCTION) { /* an inner function has names of its own */
    NameSet inner = {0};
    for (int k = 0; k < c->locals->n; k++)
      ns_add(&inner, c->locals->v[k]);
    fn_locals(n, &inner);
    RenameCtx ic = {c->top, &inner, c->alias};
    ast_body_slots(n->as.func_def.body, n->as.func_def.body_count, rename_slot, &ic);
    free(inner.v);
    return;
  }
  ast_each_name(n, rename_name, ctx);
  ast_each_child(n, rename_slot, ctx);
}

static void mod_rename(ASTNode *prog, const char *alias) {
  NameSet top = {0};
  for (int i = 0; i < prog->as.program.count; i++) {
    ASTNode *s = prog->as.program.statements[i];
    if (!s)
      continue;
    if (s->type == AST_FUNCTION && !mod_generated(s->as.func_def.name))
      ns_add(&top, s->as.func_def.name);
    else if (s->type == AST_DECLARATION)
      ns_add(&top, s->as.decl.name);
    else if (s->type == AST_MUTATION)
      ns_add(&top, s->as.mutation.name);
  }
  NameSet none = {0};
  for (int i = 0; i < prog->as.program.count; i++) {
    ASTNode *s = prog->as.program.statements[i];
    if (!s)
      continue;
    if (s->type == AST_FUNCTION) {
      RenameCtx tc = {&top, &none, alias};
      rename_name(&s->as.func_def.name, &tc);
      NameSet locals = {0};
      fn_locals(s, &locals);
      RenameCtx fc = {&top, &locals, alias};
      ast_body_slots(s->as.func_def.body, s->as.func_def.body_count, rename_slot, &fc);
      free(locals.v);
    } else {
      RenameCtx tc = {&top, &none, alias};
      rename_slot(&prog->as.program.statements[i], &tc);
    }
  }
  if (g_nmods < 64) {
    g_mods[g_nmods].alias = enlng_strdup(alias);
    g_mods[g_nmods].names = top;
    g_nmods++;
  }
}

/* alias.member: 1 when alias names a used file (out is the member's full
 * name, or an error is set), 0 when it does not. */
static int mod_resolve(Parser *p, const char *alias, const char *member,
                       char *out, size_t outsz, int line) {
  for (int i = g_nmods - 1; i >= 0; i--) {
    if (strcmp(g_mods[i].alias, alias) != 0)
      continue;
    if (!ns_has(&g_mods[i].names, member)) {
      char err[400];
      snprintf(err, sizeof(err),
               "The file used as '%s' has no function or name called '%s'.",
               alias, member);
      set_error(p, err, line);
    }
    snprintf(out, outsz, "%s__%s", alias, member);
    return 1;
  }
  return 0;
}

/* Two functions with one name: the program would call only one of them. */
static void check_duplicate_functions(Parser *p, ASTNode *prog) {
  for (int i = 0; i < prog->as.program.count; i++) {
    ASTNode *a = prog->as.program.statements[i];
    if (!a || a->type != AST_FUNCTION)
      continue;
    for (int j = i + 1; j < prog->as.program.count; j++) {
      ASTNode *b = prog->as.program.statements[j];
      if (!b || b->type != AST_FUNCTION ||
          strcmp(a->as.func_def.name, b->as.func_def.name) != 0)
        continue;
      char err[600];
      snprintf(err, sizeof(err),
               "There are two functions called '%s'. Rename one of them. If "
               "one comes from a file you used, give that file a name "
               "instead: use \"file.enlng\" as tools, then call tools.%s(...).",
               a->as.func_def.name, a->as.func_def.name);
      set_error(p, err, b->line);
      return;
    }
  }
}

static ASTNode *parser_parse_program_inner(Parser *p) {
  for (int i = 0; i < p->token_count; i++) {
    if (p->tokens[i].type == ENLNG_TOKEN_BAD_CHAR) {
      char buf[300];
      char ch = p->tokens[i].text ? p->tokens[i].text[0] : '?';
      const char *hint = "";
      if (ch == '&')
        hint = " Write and, or use bit_and(a, b) for bits.";
      else if (ch == '|')
        hint = " Write or, or use bit_or(a, b) for bits.";
      else if (ch == '^')
        hint = " For bits use bit_xor(a, b); for a power use pow(a, b).";
      else if (ch == '~')
        hint = " For bits use bit_not(a).";
      snprintf(buf, sizeof(buf), "The character '%c' is not part of Enlng.%s",
               ch, hint);
      set_error(p, buf, p->tokens[i].line);
      return NULL;
    }
    if (p->tokens[i].type == ENLNG_TOKEN_LONE_AT) {
      set_error(p, "'@' must be followed directly by a name, as in @times",
                p->tokens[i].line);
      return NULL;
    }
  }
  skip_newlines(p);

  /* --- DOMAIN GUARD: First statement must be 'type enlng' --- */
  if (match(p, ENLNG_TOKEN_KW_TYPE)) {
    if (!match(p, ENLNG_TOKEN_DOMAIN_ENLNG)) {
      Token *bad_tok = peek(p, 0);
      char err[256];
      snprintf(err, sizeof(err),
               "[DOMAIN ERROR] Received domain '%s'. Expected 'type enlng'.",
               bad_tok->text ? bad_tok->text : "unknown");
      set_error(p, err, bad_tok->line);
      return NULL;
    }
  } else {
    set_error(p, "[DOMAIN ERROR] Source must begin with 'type enlng'.",
              peek(p, 0)->line);
    return NULL;
  }

  skip_newlines(p);

  ASTNode *prog = ast_new(AST_PROGRAM, 1);
  prog->as.program.capacity = 16;
  prog->as.program.count = 0;
  prog->as.program.statements =
      (ASTNode **)malloc(sizeof(ASTNode *) * prog->as.program.capacity);

  while (!check(p, ENLNG_TOKEN_EOF) && !p->has_error) {
    skip_newlines(p);
    if (check(p, ENLNG_TOKEN_EOF))
      break;

    ASTNode *stmt = parse_statement(p);
    if (stmt) {
      if (stmt->type == AST_USE) {
        char found[1024];
        const char *alias = stmt->as.use_stmt.alias;
        char *sub_src = use_read(stmt->as.use_stmt.filename, found, sizeof(found));
        if (sub_src && !alias && ns_has(&g_loaded, found)) {
          free(sub_src); /* used already: its functions are here */
          continue;
        }
        if (sub_src) {
          if (!alias)
            ns_add(&g_loaded, enlng_strdup(found));
          Lexer sub_lexer;
          lexer_init(&sub_lexer, sub_src);
          lexer_tokenize(&sub_lexer);
          Parser sub_parser;
          parser_init(&sub_parser, sub_lexer.tokens, sub_lexer.token_count);
          char saved_dir[1024];
          snprintf(saved_dir, sizeof(saved_dir), "%s", g_src_dir);
          use_set_dir(found);
          ASTNode *sub_prog = parser_parse_program(&sub_parser);
          snprintf(g_src_dir, sizeof(g_src_dir), "%s", saved_dir);
          if (sub_parser.has_error) {
            char err[1200];
            snprintf(err, sizeof(err), "In the file \"%s\", line %d: %s",
                     stmt->as.use_stmt.filename, sub_parser.error_line,
                     sub_parser.error_msg);
            set_error(p, err, stmt->line);
            sub_prog = NULL;
          }
          if (sub_prog && sub_prog->type == AST_PROGRAM && alias)
            mod_rename(sub_prog, alias);
          if (sub_prog && sub_prog->type == AST_PROGRAM) {
            for (int s = 0; s < sub_prog->as.program.count; s++) {
              if (prog->as.program.count >= prog->as.program.capacity) {
                prog->as.program.capacity *= 2;
                prog->as.program.statements = (ASTNode **)realloc(
                    prog->as.program.statements,
                    sizeof(ASTNode *) * prog->as.program.capacity);
              }
              prog->as.program.statements[prog->as.program.count++] =
                  sub_prog->as.program.statements[s];
            }
          }
          lexer_free(&sub_lexer);
          free(sub_src);
        } else {
          char err[256];
          snprintf(err, sizeof(err), "Could not load imported file: %s",
                   stmt->as.use_stmt.filename);
          set_error(p, err, stmt->line);
        }
        continue;
      }

      if (prog->as.program.count >= prog->as.program.capacity) {
        prog->as.program.capacity *= 2;
        prog->as.program.statements =
            (ASTNode **)realloc(prog->as.program.statements,
                                sizeof(ASTNode *) * prog->as.program.capacity);
      }
      prog->as.program.statements[prog->as.program.count++] = stmt;
    }
    for (int e = 0; e < g_nextra; e++) {
      if (prog->as.program.count >= prog->as.program.capacity) {
        prog->as.program.capacity *= 2;
        prog->as.program.statements = (ASTNode **)realloc(
            prog->as.program.statements, sizeof(ASTNode *) * prog->as.program.capacity);
      }
      prog->as.program.statements[prog->as.program.count++] = g_extra[e];
    }
    g_nextra = 0;
    skip_newlines(p);
  }

  if (!p->has_error) {
    FrozenSet top = {0, 0, 0};
    frozen_check_block(p, prog->as.program.statements, prog->as.program.count,
                       &top);
    free(top.names);
  }

  return prog;
}


/* The prelude is ordinary Enlng source (src/prelude.enlng) for everyday list
 * helpers such as map, filter and range. Only the functions a program
 * mentions, and does not define itself, are added to it. */
static void add_prelude(ASTNode *prog) {
  Lexer lx;
  lexer_init(&lx, (char *)ENLNG_PRELUDE_SRC);
  lexer_tokenize(&lx);
  Parser pp;
  parser_init(&pp, lx.tokens, lx.token_count);
  g_parse_depth++;
  ASTNode *pre = parser_parse_program(&pp);
  g_parse_depth--;
  if (!pre || pre->type != AST_PROGRAM || pp.has_error) {
    return;
  }
  int pc = pre->as.program.count;
  bool *used = (bool *)calloc((size_t)(pc > 0 ? pc : 1), sizeof(bool));
  NameSet user = {0}, seen = {0};
  for (int i = 0; i < prog->as.program.count; i++) {
    ASTNode *s = prog->as.program.statements[i];
    if (s->type == AST_FUNCTION)
      ns_add(&user, s->as.func_def.name);
    ASTNode *slot = s;
    mention_slot(&slot, &seen);
  }
  bool grew = true;
  while (grew) {
    grew = false;
    for (int i = 0; i < pc; i++) {
      ASTNode *f = pre->as.program.statements[i];
      if (used[i] || f->type != AST_FUNCTION)
        continue;
      const char *name = f->as.func_def.name;
      if (ns_has(&user, name) || !ns_has(&seen, name))
        continue;
      used[i] = true;
      grew = true;
      ASTNode *slot = f;
      mention_slot(&slot, &seen);
    }
  }
  int add = 0;
  for (int i = 0; i < pc; i++)
    add += used[i] ? 1 : 0;
  if (add > 0) {
    int old = prog->as.program.count;
    if (old + add > prog->as.program.capacity)
      prog->as.program.capacity = old + add;
    prog->as.program.statements = (ASTNode **)realloc(
        prog->as.program.statements,
        sizeof(ASTNode *) * (size_t)prog->as.program.capacity);
    for (int i = 0; i < pc; i++)
      if (used[i])
        prog->as.program.statements[prog->as.program.count++] =
            pre->as.program.statements[i];
  }
  free(used);
  /* the tree may point into the tokens, so they stay */
}

typedef struct {
  Parser *p;
  ASTNode *prog;
} NamedCtx;

static ASTNode *find_top_function(ASTNode *prog, const char *name) {
  for (int i = 0; i < prog->as.program.count; i++) {
    ASTNode *s = prog->as.program.statements[i];
    if (s && s->type == AST_FUNCTION && s->as.func_def.name &&
        strcmp(s->as.func_def.name, name) == 0)
      return s;
  }
  return NULL;
}

/* area(width as 3, height as 4): puts each input in the place of its name,
 * and gives null (so the default) for an input with a default left out. */
static void named_slot(ASTNode **slot, void *ctx) {
  ASTNode *n = *slot;
  NamedCtx *c = (NamedCtx *)ctx;
  if (!n || c->p->has_error)
    return;
  ast_each_child(n, named_slot, ctx);
  if (n->type != AST_EXPR_CALL || !n->as.call_expr.arg_names)
    return;
  char msg[400];
  const char *fname = n->as.call_expr.func_name;
  ASTNode *fn = find_top_function(c->prog, fname);
  if (!fn) {
    snprintf(msg, sizeof(msg),
             "Inputs by name work with functions written in the program, and '%s' is not one. "
             "Give its inputs in order instead.", fname);
    set_error(c->p, msg, n->line);
    return;
  }
  int own = fn->as.func_def.param_count;
  int lo = fn->as.func_def.has_defaults ? fn->as.func_def.min_count : own;
  ASTNode **args = (ASTNode **)calloc((size_t)(own > 0 ? own : 1), sizeof(ASTNode *));
  bool seen_name = false;
  for (int i = 0; i < n->as.call_expr.arg_count; i++) {
    const char *an = n->as.call_expr.arg_names[i];
    int at = i;
    if (an) {
      seen_name = true;
      at = -1;
      for (int k = 0; k < own; k++)
        if (strcmp(fn->as.func_def.params[k], an) == 0)
          at = k;
      if (at < 0) {
        snprintf(msg, sizeof(msg), "'%s' has no input called '%s'.", fname, an);
        set_error(c->p, msg, n->line);
        return;
      }
    } else if (seen_name) {
      set_error(c->p, "Inputs given by name come after the ones given in order.", n->line);
      return;
    }
    if (at >= own) {
      snprintf(msg, sizeof(msg), "'%s' takes %d input%s but was given more.", fname, own,
               own == 1 ? "" : "s");
      set_error(c->p, msg, n->line);
      return;
    }
    if (args[at]) {
      snprintf(msg, sizeof(msg), "The input '%s' of '%s' is given twice.",
               fn->as.func_def.params[at], fname);
      set_error(c->p, msg, n->line);
      return;
    }
    args[at] = n->as.call_expr.args[i];
  }
  int count = own;
  while (count > lo && !args[count - 1])
    count--;
  for (int k = 0; k < count; k++) {
    if (args[k])
      continue;
    if (k < lo) {
      snprintf(msg, sizeof(msg), "'%s' needs its input '%s'.", fname, fn->as.func_def.params[k]);
      set_error(c->p, msg, n->line);
      return;
    }
    args[k] = ast_new(AST_EXPR_LITERAL_NULL, n->line);
  }
  n->as.call_expr.args = args;
  n->as.call_expr.arg_count = count;
  n->as.call_expr.arg_names = NULL;
}

static void resolve_named_inputs(Parser *p, ASTNode *prog) {
  NamedCtx c = {p, prog};
  for (int i = 0; i < prog->as.program.count && !p->has_error; i++)
    named_slot(&prog->as.program.statements[i], &c);
}

ASTNode *parser_parse_program(Parser *p) {
  bool outer = (g_parse_depth == 0);
  if (outer)
    oop_reset();
  g_parse_depth++;
  ASTNode *prog = parser_parse_program_inner(p);
  g_parse_depth--;
  if (outer && prog && prog->type == AST_PROGRAM && !p->has_error) {
    g_parse_depth++; /* the helpers are parsed by this same function */
    oop_generate_helpers(prog);
    g_parse_depth--;
  }
  if (outer && prog && prog->type == AST_PROGRAM && !p->has_error)
    add_prelude(prog);
  if (outer && prog && prog->type == AST_PROGRAM && !p->has_error)
    resolve_named_inputs(p, prog);
  if (outer && prog && prog->type == AST_PROGRAM && !p->has_error)
    check_duplicate_functions(p, prog);
  if (outer && prog && prog->type == AST_PROGRAM && !p->has_error)
    lift_program(prog);
  if (outer) {
    g_nmods = 0;
    g_loaded.n = 0;
  }
  return prog;
}

void ast_free(ASTNode *node) {
  if (!node)
    return;
  /* Clean up recursive AST if needed */
  free(node);
}

#undef peek
#undef advance
