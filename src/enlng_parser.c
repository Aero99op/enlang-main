/*
 * =====================================================================
 *   ENLANG SOVEREIGN PROGRAMMING LANGUAGE COMPILER (Pure C99 Native)
 *   src/enlng_parser.c - Deterministic Recursive Descent Parser (CFG)
 * =====================================================================
 */

#include "enlng_parser.h"

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

/* Forward declarations */
static ASTNode *parse_statement(Parser *p);
static ASTNode *parse_expression(Parser *p);
static ASTNode *parse_additive(Parser *p);

static ASTNode *ast_new(ASTNodeType type, int line) {
  ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
  node->type = type;
  node->line = line;
  return node;
}

/* Expression parsing with Precedence */
static ASTNode *parse_primary(Parser *p) {
  Token *t = peek(p, 0);

  /* Int Literal */
  if (match(p, ENLNG_TOKEN_INT_LITERAL)) {
    ASTNode *n = ast_new(AST_EXPR_LITERAL_INT, t->line);
    n->as.literal.int_val = t->int_val;
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
      Token *obj_tok = peek(p, 0);
      if (match(p, ENLNG_TOKEN_IDENTIFIER) || match(p, ENLNG_TOKEN_PAIR)) {
        ASTNode *n = ast_new(AST_EXPR_FIELD, t->line);
        n->as.field_expr.obj_name = enlng_strdup(obj_tok->text);
        n->as.field_expr.field_name = str;
        return n;
      }
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
    ASTNode *n = ast_new(AST_EXPR_COUNT_OF, t->line);
    n->as.single_target_expr.target = parse_primary(p);
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

    /* Field access via 'of': field of object (e.g. resource of request, left of pair) */
    if (match(p, ENLNG_TOKEN_OF)) {
      Token *obj_tok = peek(p, 0);
      if (match(p, ENLNG_TOKEN_IDENTIFIER) || match(p, ENLNG_TOKEN_PAIR)) {
        ASTNode *n = ast_new(AST_EXPR_FIELD, t->line);
        n->as.field_expr.obj_name = enlng_strdup(obj_tok->text);
        n->as.field_expr.field_name = name;
        return n;
      } else {
        set_error(p, "Expected object name after 'of'", t->line);
        return NULL;
      }
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
    if (match(p, ENLNG_TOKEN_LBRACKET)) {
      ASTNode *idx_expr = parse_expression(p);
      if (!match(p, ENLNG_TOKEN_RBRACKET)) {
        set_error(p, "Expected ']' after array index", t->line);
      }
      ASTNode *n = ast_new(AST_EXPR_INDEX, t->line);
      n->as.index_expr.arr_name = name;
      n->as.index_expr.index_expr = idx_expr;
      return n;
    }

    /* Array / Map index with English 'at': coll at index */
    if (match(p, ENLNG_TOKEN_AT)) {
      ASTNode *idx_expr = parse_primary(p);
      ASTNode *n = ast_new(AST_EXPR_INDEX, t->line);
      n->as.index_expr.arr_name = name;
      n->as.index_expr.index_expr = idx_expr;
      return n;
    }

    /* Function Call with English 'with': func with a, b */
    if (match(p, ENLNG_TOKEN_WITH)) {
      ASTNode *n = ast_new(AST_EXPR_CALL, t->line);
      n->as.call_expr.func_name = name;
      int cap = 4;
      int count = 0;
      ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *) * cap);

      do {
        if (count >= cap) {
          cap *= 2;
          args = (ASTNode **)realloc(args, sizeof(ASTNode *) * cap);
        }
        args[count++] = parse_additive(p);
      } while (match(p, ENLNG_TOKEN_COMMA));

      n->as.call_expr.args = args;
      n->as.call_expr.arg_count = count;
      return n;
    }

    /* Function Call: func(a, b) */
    if (match(p, ENLNG_TOKEN_LPAREN)) {
      ASTNode *n = ast_new(AST_EXPR_CALL, t->line);
      n->as.call_expr.func_name = name;
      int cap = 4;
      int count = 0;
      ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *) * cap);

      if (!check(p, ENLNG_TOKEN_RPAREN)) {
        do {
          if (count >= cap) {
            cap *= 2;
            args = (ASTNode **)realloc(args, sizeof(ASTNode *) * cap);
          }
          args[count++] = parse_expression(p);
        } while (match(p, ENLNG_TOKEN_COMMA));
      }

      if (!match(p, ENLNG_TOKEN_RPAREN)) {
        set_error(p, "Expected ')' after function arguments", t->line);
      }
      n->as.call_expr.args = args;
      n->as.call_expr.arg_count = count;
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

static ASTNode *parse_multiplicative(Parser *p) {
  ASTNode *left = parse_unary(p);
  while (check(p, ENLNG_TOKEN_MUL) || check(p, ENLNG_TOKEN_DIV) ||
         check(p, ENLNG_TOKEN_MOD)) {
    Token *op = advance(p);
    ASTNode *right = parse_unary(p);
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
         check(p, ENLNG_TOKEN_GTE) || check(p, ENLNG_TOKEN_LTE)) {
    Token *op = advance(p);
    ASTNode *right = parse_additive(p);
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

static ASTNode *parse_expression(Parser *p) {
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

/* Parse a block of statements governed by INDENT ... DEDENT */
static void parse_block(Parser *p, ASTNode ***out_stmts, int *out_count) {
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
    skip_newlines(p);
  }

  match(p, ENLNG_TOKEN_DEDENT);
  *out_stmts = stmts;
  *out_count = count;
}

static ASTNode *parse_statement(Parser *p) {
  skip_newlines(p);
  Token *t = peek(p, 0);

  /* 0. Module Import: use "filename.enlng" / import "filename.enlng" */
  if (match(p, ENLNG_TOKEN_USE)) {
    Token *file_tok = peek(p, 0);
    if (!match(p, ENLNG_TOKEN_STRING_LITERAL)) {
      set_error(p, "Expected string literal path after 'use'", t->line);
      return NULL;
    }
    ASTNode *n = ast_new(AST_USE, t->line);
    n->as.use_stmt.filename = enlng_strdup(file_tok->text);
    return n;
  }

  /* 1. Declarations: remember x as 10 / freeze PI as 3.14 */
  if (check(p, ENLNG_TOKEN_REMEMBER) || check(p, ENLNG_TOKEN_FREEZE)) {
    bool is_frozen = (t->type == ENLNG_TOKEN_FREEZE);
    advance(p);

    Token *id_tok = peek(p, 0);
    if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
      set_error(p, "Expected variable name after remember/freeze", t->line);
      return NULL;
    }
    char *name = enlng_strdup(id_tok->text);

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
      set_error(p, "Expected variable name after 'change'", t->line);
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

  /* 3. Conditionals: when cond: block [otherwise: block] */
  if (match(p, ENLNG_TOKEN_WHEN)) {
    ASTNode *cond = parse_expression(p);
    ASTNode *n = ast_new(AST_WHEN, t->line);
    n->as.when_stmt.condition = cond;
    parse_block(p, &n->as.when_stmt.then_body, &n->as.when_stmt.then_count);

    skip_newlines(p);
    if (match(p, ENLNG_TOKEN_OTHERWISE)) {
      parse_block(p, &n->as.when_stmt.else_body, &n->as.when_stmt.else_count);
    }
    return n;
  }

  /* 4. Loops: for each pair in list / for item in list: block */
  if (match(p, ENLNG_TOKEN_FOR)) {
    bool has_each = match(p, ENLNG_TOKEN_EACH);
    bool has_ident_in = (peek(p, 0)->type == ENLNG_TOKEN_IDENTIFIER &&
                         peek(p, 1)->type == ENLNG_TOKEN_IN);

    if (has_each || has_ident_in) {
      if (match(p, ENLNG_TOKEN_PAIR)) {
        /* for each pair in list: */
        char *pair_name = enlng_strdup("pair");
        if (match(p, ENLNG_TOKEN_LPAREN)) {
          /* optional (a, b) notation */
          while (!match(p, ENLNG_TOKEN_RPAREN) && !check(p, ENLNG_TOKEN_EOF))
            advance(p);
        }
        if (!match(p, ENLNG_TOKEN_IN)) {
          set_error(p, "Expected 'in' after 'pair'", t->line);
        }
        Token *list_tok = peek(p, 0);
        if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
          set_error(p, "Expected collection name after 'in'", t->line);
        }
        char *list_name = enlng_strdup(list_tok->text);

        ASTNode *n = ast_new(AST_FOR_PAIR, t->line);
        n->as.for_pair.pair_name = pair_name;
        n->as.for_pair.list_name = list_name;
        parse_block(p, &n->as.for_pair.body, &n->as.for_pair.body_count);
        return n;
      } else {
        /* for each item in list: OR for item in list: */
        Token *item_tok = peek(p, 0);
        if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
          set_error(p, "Expected variable name after 'for'", t->line);
        }
        char *item_name = enlng_strdup(item_tok->text);
        if (!match(p, ENLNG_TOKEN_IN)) {
          set_error(p, "Expected 'in' after loop variable", t->line);
        }
        Token *list_tok = peek(p, 0);
        if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
          set_error(p, "Expected collection name after 'in'", t->line);
        }
        char *list_name = enlng_strdup(list_tok->text);

        ASTNode *n = ast_new(AST_FOR_EACH, t->line);
        n->as.for_each.item_name = item_name;
        n->as.for_each.list_name = list_name;
        parse_block(p, &n->as.for_each.body, &n->as.for_each.body_count);
        return n;
      }
    } else {
      /* for i from start to end [by step]: */
      Token *var_tok = peek(p, 0);
      if (!match(p, ENLNG_TOKEN_IDENTIFIER)) {
        set_error(p, "Expected variable name in for range loop", t->line);
      }
      char *var_name = enlng_strdup(var_tok->text);
      if (!match(p, ENLNG_TOKEN_FROM)) {
        set_error(p, "Expected 'from' after loop variable", t->line);
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
      set_error(p, "Expected function name", t->line);
      return NULL;
    }
    char *name = enlng_strdup(name_tok->text);

    int param_cap = 4;
    int param_count = 0;
    char **params = (char **)malloc(sizeof(char *) * param_cap);

    if (match(p, ENLNG_TOKEN_WITH)) {
      do {
        Token *pt = peek(p, 0);
        if (match(p, ENLNG_TOKEN_IDENTIFIER)) {
          if (param_count >= param_cap) {
            param_cap *= 2;
            params = (char **)realloc(params, sizeof(char *) * param_cap);
          }
          params[param_count++] = enlng_strdup(pt->text);
        }
      } while (match(p, ENLNG_TOKEN_COMMA));
    } else if (match(p, ENLNG_TOKEN_LPAREN)) {
      if (!check(p, ENLNG_TOKEN_RPAREN)) {
        do {
          Token *pt = peek(p, 0);
          if (match(p, ENLNG_TOKEN_IDENTIFIER)) {
            if (param_count >= param_cap) {
              param_cap *= 2;
              params = (char **)realloc(params, sizeof(char *) * param_cap);
            }
            params[param_count++] = enlng_strdup(pt->text);
          }
        } while (match(p, ENLNG_TOKEN_COMMA));
      }
      if (!match(p, ENLNG_TOKEN_RPAREN)) {
        set_error(p, "Expected ')' after function parameters", t->line);
      }
    }

    ASTNode *n = ast_new(AST_FUNCTION, t->line);
    n->as.func_def.name = name;
    n->as.func_def.params = params;
    n->as.func_def.param_count = param_count;
    parse_block(p, &n->as.func_def.body, &n->as.func_def.body_count);
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
    n->as.swap_stmt.left = parse_primary(p);
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
      items[count++] = parse_primary(p);
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
      set_error(p, "Expected variable name after 'ask'", t->line);
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

    /* Case A: arr[i] = ... or arr[i] += ... */
    if (next_tok->type == ENLNG_TOKEN_LBRACKET) {
      advance(p); /* consume ident */
      advance(p); /* consume '[' */
      ASTNode *index_expr = parse_expression(p);
      if (!match(p, ENLNG_TOKEN_RBRACKET)) {
        set_error(p, "Expected ']' after index in assignment", t->line);
      }
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

    /* Case D: coll at index = ... or coll at index increases by ... */
    if (next_tok->type == ENLNG_TOKEN_AT) {
      advance(p); /* consume ident */
      advance(p); /* consume 'at' */
      ASTNode *index_expr = parse_primary(p);
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
  ASTNode *n = ast_new(AST_EXPR_STMT, t->line);
  n->as.expr_stmt.expr = expr;
  return n;
}

ASTNode *parser_parse_program(Parser *p) {
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
        char *sub_src = enlng_read_file_string(stmt->as.use_stmt.filename);
        if (sub_src) {
          Lexer sub_lexer;
          lexer_init(&sub_lexer, sub_src);
          lexer_tokenize(&sub_lexer);
          Parser sub_parser;
          parser_init(&sub_parser, sub_lexer.tokens, sub_lexer.token_count);
          ASTNode *sub_prog = parser_parse_program(&sub_parser);
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
    skip_newlines(p);
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
