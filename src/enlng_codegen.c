/*
 * =====================================================================
 *   ENLANG SOVEREIGN PROGRAMMING LANGUAGE COMPILER (Pure C99 Native)
 *   src/enlng_codegen.c - Pure ANSI C99 Code Generator
 * =====================================================================
 */

#include "enlng_codegen.h"
#include <stdarg.h>

void codegen_init(CodeGen *cg) {
  cg->capacity = 4096;
  cg->size = 0;
  cg->buffer = (char *)malloc(cg->capacity);
  cg->buffer[0] = '\0';
  cg->indent_level = 0;
  cg->current_pair_list[0] = '\0';
  cg->in_pair_loop = false;
  cg->temp_var_id = 0;
  cg->program = NULL;
  cg->scope_vars = NULL;
  cg->scope_var_count = 0;
  cg->scope_var_cap = 0;
}

static void scope_clear(CodeGen *cg) {
  for (int i = 0; i < cg->scope_var_count; i++) {
    free(cg->scope_vars[i]);
  }
  cg->scope_var_count = 0;
}

static void scope_add(CodeGen *cg, const char *name) {
  if (!name)
    return;
  for (int i = 0; i < cg->scope_var_count; i++) {
    if (strcmp(cg->scope_vars[i], name) == 0)
      return;
  }
  if (cg->scope_var_count >= cg->scope_var_cap) {
    cg->scope_var_cap = (cg->scope_var_cap == 0) ? 32 : cg->scope_var_cap * 2;
    cg->scope_vars =
        (char **)realloc(cg->scope_vars, sizeof(char *) * cg->scope_var_cap);
  }
  cg->scope_vars[cg->scope_var_count++] = enlng_strdup(name);
}

static bool scope_has(CodeGen *cg, const char *name) {
  if (!name)
    return false;
  for (int i = 0; i < cg->scope_var_count; i++) {
    if (strcmp(cg->scope_vars[i], name) == 0)
      return true;
  }
  return false;
}

static bool is_user_function(ASTNode *program, const char *name) {
  if (!program || program->type != AST_PROGRAM)
    return false;
  for (int i = 0; i < program->as.program.count; i++) {
    ASTNode *stmt = program->as.program.statements[i];
    if (stmt && stmt->type == AST_FUNCTION) {
      if (strcmp(stmt->as.func_def.name, name) == 0)
        return true;
    }
  }
  return false;
}

static bool is_builtin_func(const char *name) {
  static const char *builtins[] = {
      "sqrt",       "pow",         "abs",         "floor",         "ceil",
      "round",      "min",         "max",         "random_number", "read_file",
      "write_file", "append_file", "file_exists", "time_now",      "sleep",
      "append",     "pop",         "keys",        "values",        "has_key",
      "len",        "split",       "join",        "contains",      NULL};
  for (int i = 0; builtins[i]; i++) {
    if (strcmp(name, builtins[i]) == 0)
      return true;
  }
  return false;
}

void codegen_free(CodeGen *cg) {
  if (cg->buffer) {
    free(cg->buffer);
    cg->buffer = NULL;
  }
  scope_clear(cg);
  if (cg->scope_vars) {
    free(cg->scope_vars);
    cg->scope_vars = NULL;
  }
  cg->scope_var_count = 0;
  cg->scope_var_cap = 0;
}

static void emit(CodeGen *cg, const char *fmt, ...) {
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (n < 0)
    return;

  if (cg->size + n + 1 >= cg->capacity) {
    cg->capacity = (cg->capacity + n) * 2;
    cg->buffer = (char *)realloc(cg->buffer, cg->capacity);
  }
  memcpy(cg->buffer + cg->size, buf, n);
  cg->size += n;
  cg->buffer[cg->size] = '\0';
}

static void emit_indent(CodeGen *cg) {
  for (int i = 0; i < cg->indent_level; i++) {
    emit(cg, "    ");
  }
}

static void emit_escaped_string(CodeGen *cg, const char *str) {
  emit(cg, "\"");
  if (str) {
    for (const char *p = str; *p; p++) {
      if (*p == '\\')
        emit(cg, "\\\\");
      else if (*p == '\"')
        emit(cg, "\\\"");
      else if (*p == '\n')
        emit(cg, "\\n");
      else if (*p == '\r')
        emit(cg, "\\r");
      else if (*p == '\t')
        emit(cg, "\\t");
      else
        emit(cg, "%c", *p);
    }
  }
  emit(cg, "\"");
}

/* Forward declarations */
static void generate_expression(CodeGen *cg, ASTNode *node);
static void generate_statement(CodeGen *cg, ASTNode *node);

static void generate_expression(CodeGen *cg, ASTNode *n) {
  if (!n) {
    emit(cg, "enlng_make_null()");
    return;
  }

  switch (n->type) {
  case AST_EXPR_LITERAL_INT:
    emit(cg, "enlng_make_int(%lldLL)", (long long)n->as.literal.int_val);
    break;

  case AST_EXPR_LITERAL_FLOAT:
    emit(cg, "enlng_make_float(%f)", n->as.literal.float_val);
    break;

  case AST_EXPR_LITERAL_BOOL:
    emit(cg, "enlng_make_bool(%s)", n->as.literal.bool_val ? "true" : "false");
    break;

  case AST_EXPR_LITERAL_NULL:
    emit(cg, "enlng_make_null()");
    break;

  case AST_EXPR_LITERAL_STRING:
    emit(cg, "enlng_make_string(");
    emit_escaped_string(cg, n->as.literal.string_val ? n->as.literal.string_val
                                                     : "");
    emit(cg, ")");
    break;

  case AST_EXPR_VARIABLE:
    emit(cg, "%s", n->as.variable.name);
    break;

  case AST_EXPR_INDEX:
    emit(cg, "enlng_container_get(%s, ", n->as.index_expr.arr_name);
    generate_expression(cg, n->as.index_expr.index_expr);
    emit(cg, ")");
    break;

  case AST_EXPR_FIELD:
    if (strcmp(n->as.field_expr.obj_name, "pair") == 0) {
      if (strcmp(n->as.field_expr.field_name, "left") == 0)
        emit(cg, "pair_left");
      else if (strcmp(n->as.field_expr.field_name, "right") == 0)
        emit(cg, "pair_right");
      else
        emit(cg, "enlng_make_null()");
    } else {
      emit(cg, "enlng_map_get(%s, \"%s\")", n->as.field_expr.obj_name,
           n->as.field_expr.field_name);
    }
    break;

  case AST_EXPR_COUNT_OF:
    emit(cg, "enlng_make_int(enlng_count_of(");
    generate_expression(cg, n->as.single_target_expr.target);
    emit(cg, "))");
    break;

  case AST_EXPR_REVERSE:
    emit(cg, "enlng_val_reverse(");
    generate_expression(cg, n->as.single_target_expr.target);
    emit(cg, ")");
    break;

  case AST_EXPR_LIST_LITERAL: {
    int lid = cg->temp_var_id++;
    emit(cg, "({ EnlngList* _nl%d = enlng_list_create(); ", lid);
    for (int i = 0; i < n->as.list_literal.count; i++) {
      emit(cg, "enlng_list_push(_nl%d, ", lid);
      generate_expression(cg, n->as.list_literal.elements[i]);
      emit(cg, "); ");
    }
    emit(cg, "enlng_make_list(_nl%d); })", lid);
    break;
  }

  case AST_EXPR_MAP_LITERAL: {
    int mid = cg->temp_var_id++;
    emit(cg, "({ EnlngVal _nm%d = enlng_make_map(enlng_map_create()); ", mid);
    for (int i = 0; i < n->as.map_literal.count; i++) {
      emit(cg, "enlng_map_set(_nm%d, \"%s\", ", mid, n->as.map_literal.keys[i]);
      generate_expression(cg, n->as.map_literal.values[i]);
      emit(cg, "); ");
    }
    emit(cg, "_nm%d; })", mid);
    break;
  }

  case AST_EXPR_BINARY:
    switch (n->as.binary_expr.op) {
    case ENLNG_TOKEN_ADD:
      emit(cg, "enlng_val_add(");
      generate_expression(cg, n->as.binary_expr.left);
      emit(cg, ", ");
      generate_expression(cg, n->as.binary_expr.right);
      emit(cg, ")");
      break;
    case ENLNG_TOKEN_SUB:
      emit(cg, "enlng_val_sub(");
      generate_expression(cg, n->as.binary_expr.left);
      emit(cg, ", ");
      generate_expression(cg, n->as.binary_expr.right);
      emit(cg, ")");
      break;
    case ENLNG_TOKEN_MUL:
      emit(cg, "enlng_val_mul(");
      generate_expression(cg, n->as.binary_expr.left);
      emit(cg, ", ");
      generate_expression(cg, n->as.binary_expr.right);
      emit(cg, ")");
      break;
    case ENLNG_TOKEN_DIV:
      emit(cg, "enlng_val_div(");
      generate_expression(cg, n->as.binary_expr.left);
      emit(cg, ", ");
      generate_expression(cg, n->as.binary_expr.right);
      emit(cg, ")");
      break;
    case ENLNG_TOKEN_MOD:
      emit(cg, "enlng_val_mod(");
      generate_expression(cg, n->as.binary_expr.left);
      emit(cg, ", ");
      generate_expression(cg, n->as.binary_expr.right);
      emit(cg, ")");
      break;
    case ENLNG_TOKEN_EQ:
      emit(cg, "enlng_make_bool(enlng_vals_equal(");
      generate_expression(cg, n->as.binary_expr.left);
      emit(cg, ", ");
      generate_expression(cg, n->as.binary_expr.right);
      emit(cg, "))");
      break;
    case ENLNG_TOKEN_NEQ:
      emit(cg, "enlng_make_bool(!enlng_vals_equal(");
      generate_expression(cg, n->as.binary_expr.left);
      emit(cg, ", ");
      generate_expression(cg, n->as.binary_expr.right);
      emit(cg, "))");
      break;
    case ENLNG_TOKEN_GT:
      emit(cg, "enlng_make_bool(enlng_vals_gt(");
      generate_expression(cg, n->as.binary_expr.left);
      emit(cg, ", ");
      generate_expression(cg, n->as.binary_expr.right);
      emit(cg, "))");
      break;
    case ENLNG_TOKEN_LT:
      emit(cg, "enlng_make_bool(enlng_vals_lt(");
      generate_expression(cg, n->as.binary_expr.left);
      emit(cg, ", ");
      generate_expression(cg, n->as.binary_expr.right);
      emit(cg, "))");
      break;
    case ENLNG_TOKEN_GTE:
      emit(cg, "enlng_make_bool(enlng_vals_gte(");
      generate_expression(cg, n->as.binary_expr.left);
      emit(cg, ", ");
      generate_expression(cg, n->as.binary_expr.right);
      emit(cg, "))");
      break;
    case ENLNG_TOKEN_LTE:
      emit(cg, "enlng_make_bool(enlng_vals_lte(");
      generate_expression(cg, n->as.binary_expr.left);
      emit(cg, ", ");
      generate_expression(cg, n->as.binary_expr.right);
      emit(cg, "))");
      break;
    case ENLNG_TOKEN_AND:
      emit(cg, "enlng_make_bool(enlng_is_truthy(");
      generate_expression(cg, n->as.binary_expr.left);
      emit(cg, ") && enlng_is_truthy(");
      generate_expression(cg, n->as.binary_expr.right);
      emit(cg, "))");
      break;
    case ENLNG_TOKEN_OR:
      emit(cg, "enlng_make_bool(enlng_is_truthy(");
      generate_expression(cg, n->as.binary_expr.left);
      emit(cg, ") || enlng_is_truthy(");
      generate_expression(cg, n->as.binary_expr.right);
      emit(cg, "))");
      break;
    default:
      emit(cg, "enlng_make_null()");
      break;
    }
    break;

  case AST_EXPR_UNARY:
    if (n->as.unary_expr.op == ENLNG_TOKEN_NOT) {
      emit(cg, "enlng_make_bool(!enlng_is_truthy(");
      generate_expression(cg, n->as.unary_expr.operand);
      emit(cg, "))");
    } else if (n->as.unary_expr.op == ENLNG_TOKEN_SUB) {
      emit(cg, "enlng_val_sub(enlng_make_int(0), ");
      generate_expression(cg, n->as.unary_expr.operand);
      emit(cg, ")");
    }
    break;

  case AST_EXPR_CALL:
    if (!is_user_function(cg->program, n->as.call_expr.func_name) &&
        is_builtin_func(n->as.call_expr.func_name)) {
      emit(cg, "enlng_builtin_%s(", n->as.call_expr.func_name);
    } else {
      emit(cg, "enlng_user_%s(", n->as.call_expr.func_name);
    }
    for (int i = 0; i < n->as.call_expr.arg_count; i++) {
      generate_expression(cg, n->as.call_expr.args[i]);
      if (i < n->as.call_expr.arg_count - 1)
        emit(cg, ", ");
    }
    emit(cg, ")");
    break;

  default:
    emit(cg, "enlng_make_null()");
    break;
  }
}

static void generate_statement(CodeGen *cg, ASTNode *n) {
  if (!n)
    return;

  switch (n->type) {
  case AST_DECLARATION:
    scope_add(cg, n->as.decl.name);
    emit_indent(cg);
    emit(cg, "EnlngVal %s = ", n->as.decl.name);
    generate_expression(cg, n->as.decl.init_expr);
    emit(cg, ";\n");
    break;

  case AST_MUTATION:
    emit_indent(cg);
    if (n->as.mutation.op == ENLNG_TOKEN_INC_BY) {
      emit(cg, "%s = enlng_val_add(%s, ", n->as.mutation.name,
           n->as.mutation.name);
      generate_expression(cg, n->as.mutation.val_expr);
      emit(cg, ");\n");
    } else if (n->as.mutation.op == ENLNG_TOKEN_DEC_BY) {
      emit(cg, "%s = enlng_val_sub(%s, ", n->as.mutation.name,
           n->as.mutation.name);
      generate_expression(cg, n->as.mutation.val_expr);
      emit(cg, ");\n");
    } else {
      if (!scope_has(cg, n->as.mutation.name)) {
        scope_add(cg, n->as.mutation.name);
        emit(cg, "EnlngVal %s = ", n->as.mutation.name);
      } else {
        emit(cg, "%s = ", n->as.mutation.name);
      }
      generate_expression(cg, n->as.mutation.val_expr);
      emit(cg, ";\n");
    }
    break;

  case AST_INDEX_MUTATION:
    emit_indent(cg);
    if (n->as.index_mutation.op == ENLNG_TOKEN_INC_BY) {
      emit(cg, "enlng_container_set(%s, ", n->as.index_mutation.arr_name);
      generate_expression(cg, n->as.index_mutation.index_expr);
      emit(cg, ", enlng_val_add(enlng_container_get(%s, ",
           n->as.index_mutation.arr_name);
      generate_expression(cg, n->as.index_mutation.index_expr);
      emit(cg, "), ");
      generate_expression(cg, n->as.index_mutation.val_expr);
      emit(cg, "));\n");
    } else if (n->as.index_mutation.op == ENLNG_TOKEN_DEC_BY) {
      emit(cg, "enlng_container_set(%s, ", n->as.index_mutation.arr_name);
      generate_expression(cg, n->as.index_mutation.index_expr);
      emit(cg, ", enlng_val_sub(enlng_container_get(%s, ",
           n->as.index_mutation.arr_name);
      generate_expression(cg, n->as.index_mutation.index_expr);
      emit(cg, "), ");
      generate_expression(cg, n->as.index_mutation.val_expr);
      emit(cg, "));\n");
    } else {
      emit(cg, "enlng_container_set(%s, ", n->as.index_mutation.arr_name);
      generate_expression(cg, n->as.index_mutation.index_expr);
      emit(cg, ", ");
      generate_expression(cg, n->as.index_mutation.val_expr);
      emit(cg, ");\n");
    }
    break;

  case AST_WHEN:
    emit_indent(cg);
    emit(cg, "if (enlng_is_truthy(");
    generate_expression(cg, n->as.when_stmt.condition);
    emit(cg, ")) {\n");

    cg->indent_level++;
    for (int i = 0; i < n->as.when_stmt.then_count; i++) {
      generate_statement(cg, n->as.when_stmt.then_body[i]);
    }
    cg->indent_level--;

    if (n->as.when_stmt.else_count > 0) {
      emit_indent(cg);
      emit(cg, "} else {\n");
      cg->indent_level++;
      for (int i = 0; i < n->as.when_stmt.else_count; i++) {
        generate_statement(cg, n->as.when_stmt.else_body[i]);
      }
      cg->indent_level--;
    }
    emit_indent(cg);
    emit(cg, "}\n");
    break;

  case AST_FOR_RANGE: {
    emit_indent(cg);
    emit(cg, "{\n");
    cg->indent_level++;

    emit_indent(cg);
    emit(cg, "int64_t _start = enlng_val_to_int(");
    generate_expression(cg, n->as.for_range.start_expr);
    emit(cg, ");\n");

    emit_indent(cg);
    emit(cg, "int64_t _end = enlng_val_to_int(");
    generate_expression(cg, n->as.for_range.end_expr);
    emit(cg, ");\n");

    emit_indent(cg);
    if (n->as.for_range.step_expr) {
      emit(cg, "int64_t _step = enlng_val_to_int(");
      generate_expression(cg, n->as.for_range.step_expr);
      emit(cg, ");\n");
    } else {
      emit(cg, "int64_t _step = 1;\n");
    }

    emit_indent(cg);
    emit(cg, "for (int64_t _it = _start; _it <= _end; _it += _step) {\n");
    cg->indent_level++;

    emit_indent(cg);
    emit(cg, "EnlngVal %s = enlng_make_int(_it);\n", n->as.for_range.var_name);

    for (int i = 0; i < n->as.for_range.body_count; i++) {
      generate_statement(cg, n->as.for_range.body[i]);
    }
    cg->indent_level--;
    emit_indent(cg);
    emit(cg, "}\n");

    cg->indent_level--;
    emit_indent(cg);
    emit(cg, "}\n");
    break;
  }

  case AST_FOR_PAIR: {
    emit_indent(cg);
    emit(cg, "{\n");
    cg->indent_level++;

    emit_indent(cg);
    emit(cg, "int64_t _pair_cnt = enlng_count_of(%s);\n",
         n->as.for_pair.list_name);

    emit_indent(cg);
    emit(cg,
         "for (int64_t _pair_i = 0; _pair_i < _pair_cnt - 1; _pair_i++) {\n");
    cg->indent_level++;

    emit_indent(cg);
    emit(cg, "EnlngVal pair_left = enlng_list_get(%s, _pair_i);\n",
         n->as.for_pair.list_name);
    emit_indent(cg);
    emit(cg, "EnlngVal pair_right = enlng_list_get(%s, _pair_i + 1);\n",
         n->as.for_pair.list_name);

    char prev_pair_list[64];
    strncpy(prev_pair_list, cg->current_pair_list, sizeof(prev_pair_list));
    strncpy(cg->current_pair_list, n->as.for_pair.list_name,
            sizeof(cg->current_pair_list));
    bool prev_in_pair = cg->in_pair_loop;
    cg->in_pair_loop = true;

    for (int i = 0; i < n->as.for_pair.body_count; i++) {
      generate_statement(cg, n->as.for_pair.body[i]);
    }

    strncpy(cg->current_pair_list, prev_pair_list,
            sizeof(cg->current_pair_list));
    cg->in_pair_loop = prev_in_pair;

    cg->indent_level--;
    emit_indent(cg);
    emit(cg, "}\n");

    cg->indent_level--;
    emit_indent(cg);
    emit(cg, "}\n");
    break;
  }

  case AST_FOR_EACH: {
    emit_indent(cg);
    emit(cg, "{\n");
    cg->indent_level++;

    emit_indent(cg);
    emit(cg, "int64_t _each_cnt = enlng_count_of(%s);\n",
         n->as.for_each.list_name);

    emit_indent(cg);
    emit(cg, "for (int64_t _each_i = 0; _each_i < _each_cnt; _each_i++) {\n");
    cg->indent_level++;

    emit_indent(cg);
    emit(cg, "EnlngVal %s = enlng_list_get(%s, _each_i);\n",
         n->as.for_each.item_name, n->as.for_each.list_name);

    for (int i = 0; i < n->as.for_each.body_count; i++) {
      generate_statement(cg, n->as.for_each.body[i]);
    }

    cg->indent_level--;
    emit_indent(cg);
    emit(cg, "}\n");

    cg->indent_level--;
    emit_indent(cg);
    emit(cg, "}\n");
    break;
  }

  case AST_REPEAT_WHILE: {
    emit_indent(cg);
    emit(cg, "while (enlng_is_truthy(");
    generate_expression(cg, n->as.repeat_loop.condition);
    emit(cg, ")) {\n");

    cg->indent_level++;
    for (int i = 0; i < n->as.repeat_loop.body_count; i++) {
      generate_statement(cg, n->as.repeat_loop.body[i]);
    }
    cg->indent_level--;

    emit_indent(cg);
    emit(cg, "}\n");
    break;
  }

  case AST_REPEAT_UNTIL: {
    emit_indent(cg);
    /* Check if until condition is "sorted" */
    bool is_until_sorted = false;
    if (n->as.repeat_loop.condition &&
        n->as.repeat_loop.condition->type == AST_EXPR_VARIABLE) {
      if (strcmp(n->as.repeat_loop.condition->as.variable.name, "sorted") ==
          0) {
        is_until_sorted = true;
      }
    }

    if (is_until_sorted) {
      emit(cg, "{\n");
      cg->indent_level++;
      emit_indent(cg);
      emit(cg, "bool _enlng_sorted = false;\n");
      emit_indent(cg);
      emit(cg, "while (!_enlng_sorted) {\n");
      cg->indent_level++;
      emit_indent(cg);
      emit(cg, "_enlng_sorted = true;\n");
    } else {
      emit(cg, "while (!enlng_is_truthy(");
      generate_expression(cg, n->as.repeat_loop.condition);
      emit(cg, ")) {\n");
      cg->indent_level++;
    }

    for (int i = 0; i < n->as.repeat_loop.body_count; i++) {
      generate_statement(cg, n->as.repeat_loop.body[i]);
    }
    cg->indent_level--;

    emit_indent(cg);
    emit(cg, "}\n");
    if (is_until_sorted) {
      cg->indent_level--;
      emit_indent(cg);
      emit(cg, "}\n");
    }
    break;
  }

  case AST_GIVE:
    emit_indent(cg);
    if (n->as.give_stmt.expr) {
      emit(cg, "return ");
      generate_expression(cg, n->as.give_stmt.expr);
      emit(cg, ";\n");
    } else {
      emit(cg, "return enlng_make_null();\n");
    }
    break;

  case AST_STOP:
    emit_indent(cg);
    emit(cg, "break;\n");
    break;

  case AST_SKIP:
    emit_indent(cg);
    emit(cg, "continue;\n");
    break;

  case AST_SWAP:
    emit_indent(cg);
    if (n->as.swap_stmt.is_pair) {
      if (cg->in_pair_loop && cg->current_pair_list[0] != '\0') {
        emit(cg, "enlng_list_swap(%s, _pair_i, _pair_i + 1);\n",
             cg->current_pair_list);
        emit_indent(cg);
        emit(cg, "_enlng_sorted = false;\n");
      }
    } else if (n->as.swap_stmt.left->type == AST_EXPR_VARIABLE &&
               n->as.swap_stmt.right->type == AST_EXPR_VARIABLE) {
      emit(cg, "{\n");
      cg->indent_level++;
      emit_indent(cg);
      emit(cg, "EnlngVal _swap_tmp = %s;\n",
           n->as.swap_stmt.left->as.variable.name);
      emit_indent(cg);
      emit(cg, "%s = %s;\n", n->as.swap_stmt.left->as.variable.name,
           n->as.swap_stmt.right->as.variable.name);
      emit_indent(cg);
      emit(cg, "%s = _swap_tmp;\n", n->as.swap_stmt.right->as.variable.name);
      cg->indent_level--;
      emit_indent(cg);
      emit(cg, "}\n");
    } else if (n->as.swap_stmt.left->type == AST_EXPR_INDEX &&
               n->as.swap_stmt.right->type == AST_EXPR_INDEX) {
      emit(cg, "enlng_list_swap(%s, enlng_val_to_int(",
           n->as.swap_stmt.left->as.index_expr.arr_name);
      generate_expression(cg, n->as.swap_stmt.left->as.index_expr.index_expr);
      emit(cg, "), enlng_val_to_int(");
      generate_expression(cg, n->as.swap_stmt.right->as.index_expr.index_expr);
      emit(cg, "));\n");
    } else {
      emit(cg, "{\n");
      cg->indent_level++;
      emit_indent(cg);
      emit(cg, "EnlngVal _swap_tmp = ");
      generate_expression(cg, n->as.swap_stmt.left);
      emit(cg, ";\n");
      emit_indent(cg);
      generate_expression(cg, n->as.swap_stmt.left);
      emit(cg, " = ");
      generate_expression(cg, n->as.swap_stmt.right);
      emit(cg, ";\n");
      emit_indent(cg);
      generate_expression(cg, n->as.swap_stmt.right);
      emit(cg, " = _swap_tmp;\n");
      cg->indent_level--;
      emit_indent(cg);
      emit(cg, "}\n");
    }
    break;

  case AST_REVERSE_STMT:
    emit_indent(cg);
    emit(cg, "%s = enlng_val_reverse(%s);\n", n->as.reverse_stmt.target_name,
         n->as.reverse_stmt.target_name);
    break;

  case AST_SORT:
    emit_indent(cg);
    emit(cg, "enlng_list_sort(%s, %s);\n", n->as.sort_stmt.target_name,
         n->as.sort_stmt.descending ? "true" : "false");
    break;

  case AST_SHOW:
    emit_indent(cg);
    for (int i = 0; i < n->as.show_stmt.count; i++) {
      emit(cg, "enlng_print_val(");
      generate_expression(cg, n->as.show_stmt.items[i]);
      emit(cg, "); ");
      if (i < n->as.show_stmt.count - 1) {
        emit(cg, "printf(\" \"); ");
      }
    }
    emit(cg, "printf(\"\\n\");\n");
    break;

  case AST_ASK:
    emit_indent(cg);
    emit(cg, "EnlngVal %s = enlng_ask(\"%s\");\n", n->as.ask_stmt.var_name,
         n->as.ask_stmt.prompt ? n->as.ask_stmt.prompt : "");
    break;

  case AST_EXPR_STMT:
    emit_indent(cg);
    generate_expression(cg, n->as.expr_stmt.expr);
    emit(cg, ";\n");
    break;

  case AST_USE:
    break;

  default:
    break;
  }
}

char *codegen_generate(CodeGen *cg, ASTNode *program) {
  if (!program || program->type != AST_PROGRAM)
    return NULL;
  cg->program = program;

  emit(cg, "/* "
           "==================================================================="
           "== */\n");
  emit(cg, "/*  NATIVE C99 EXECUTABLE - COMPILED BY ENLANG SOVEREIGN COMPILER "
           "v4.0.0 */\n");
  emit(cg, "/* "
           "==================================================================="
           "== */\n\n");

  emit(cg, "#include \"enlng_runtime.h\"\n\n");

  /* Forward declare all user functions */
  for (int i = 0; i < program->as.program.count; i++) {
    ASTNode *stmt = program->as.program.statements[i];
    if (stmt && stmt->type == AST_FUNCTION) {
      emit(cg, "EnlngVal enlng_user_%s(", stmt->as.func_def.name);
      for (int p = 0; p < stmt->as.func_def.param_count; p++) {
        emit(cg, "EnlngVal %s", stmt->as.func_def.params[p]);
        if (p < stmt->as.func_def.param_count - 1)
          emit(cg, ", ");
      }
      emit(cg, ");\n");
    }
  }
  emit(cg, "\n");

  /* Emit function definitions */
  for (int i = 0; i < program->as.program.count; i++) {
    ASTNode *stmt = program->as.program.statements[i];
    if (stmt && stmt->type == AST_FUNCTION) {
      emit(cg, "EnlngVal enlng_user_%s(", stmt->as.func_def.name);
      for (int p = 0; p < stmt->as.func_def.param_count; p++) {
        emit(cg, "EnlngVal %s", stmt->as.func_def.params[p]);
        if (p < stmt->as.func_def.param_count - 1)
          emit(cg, ", ");
      }
      emit(cg, ") {\n");
      scope_clear(cg);
      for (int p = 0; p < stmt->as.func_def.param_count; p++) {
        scope_add(cg, stmt->as.func_def.params[p]);
      }

      cg->indent_level = 1;
      for (int b = 0; b < stmt->as.func_def.body_count; b++) {
        /* Implicit return check: if last statement is an expression */
        ASTNode *bstmt = stmt->as.func_def.body[b];
        if (b == stmt->as.func_def.body_count - 1 &&
            bstmt->type == AST_EXPR_STMT) {
          emit_indent(cg);
          emit(cg, "return ");
          generate_expression(cg, bstmt->as.expr_stmt.expr);
          emit(cg, ";\n");
        } else {
          generate_statement(cg, bstmt);
        }
      }

      emit_indent(cg);
      emit(cg, "return enlng_make_null();\n");
      emit(cg, "}\n\n");
    }
  }

  /* Emit main() */
  emit(cg, "int main(int argc, char* argv[]) {\n");
  cg->indent_level = 1;
  scope_clear(cg);

  for (int i = 0; i < program->as.program.count; i++) {
    ASTNode *stmt = program->as.program.statements[i];
    if (stmt && stmt->type != AST_FUNCTION) {
      generate_statement(cg, stmt);
    }
  }

  emit_indent(cg);
  emit(cg, "return 0;\n");
  emit(cg, "}\n");

  return cg->buffer;
}
