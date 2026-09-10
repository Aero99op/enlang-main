/*
 * =====================================================================
 *   ENLANG SOVEREIGN PROGRAMMING LANGUAGE COMPILER (Pure C99 Native)
 *   src/enlng_common.h - Token Types, AST Nodes & Compiler Common Types
 * =====================================================================
 */

#ifndef ENLNG_COMMON_H
#define ENLNG_COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENLNG_VERSION "4.0.0-sovereign-native"

/* Safe strdup implementation for ANSI C99 */
static inline char *enlng_strdup(const char *s) {
  if (!s)
    return NULL;
  size_t len = strlen(s);
  char *copy = (char *)malloc(len + 1);
  if (copy) {
    memcpy(copy, s, len + 1);
  }
  return copy;
}

static inline char *enlng_read_file_string(const char *filepath) {
  FILE *f = fopen(filepath, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc(len + 1);
  if (buf) {
    fread(buf, 1, len, f);
    buf[len] = '\0';
  }
  fclose(f);
  return buf;
}

/* --- Token Enumeration (Prefixed to avoid Windows header collisions) --- */
typedef enum {
  ENLNG_TOKEN_EOF = 0,
  ENLNG_TOKEN_NEWLINE,
  ENLNG_TOKEN_INDENT,
  ENLNG_TOKEN_DEDENT,

  /* Domain Guard */
  ENLNG_TOKEN_KW_TYPE,      /* 'type' */
  ENLNG_TOKEN_DOMAIN_ENLNG, /* 'enlng' */

  /* Identifiers & Literals */
  ENLNG_TOKEN_IDENTIFIER,     /* variable / function names */
  ENLNG_TOKEN_INT_LITERAL,    /* 42 */
  ENLNG_TOKEN_FLOAT_LITERAL,  /* 3.14 */
  ENLNG_TOKEN_STRING_LITERAL, /* "hello" */
  ENLNG_TOKEN_BOOL_LITERAL,   /* true / false */
  ENLNG_TOKEN_NULL,           /* null / none / nothing */

  /* Declarations & Bindings */
  ENLNG_TOKEN_REMEMBER, /* 'remember' */
  ENLNG_TOKEN_FREEZE,   /* 'freeze' */
  ENLNG_TOKEN_AS,       /* 'as' */
  ENLNG_TOKEN_CHANGE,   /* 'change' */
  ENLNG_TOKEN_TO,       /* 'to' */
  ENLNG_TOKEN_FORGET,   /* 'forget' */
  ENLNG_TOKEN_ASSIGN,   /* '=' */

  /* Control Flow */
  ENLNG_TOKEN_WHEN,      /* 'when' / 'if' */
  ENLNG_TOKEN_OTHERWISE, /* 'otherwise' / 'else' */
  ENLNG_TOKEN_STOP,      /* 'stop' / 'break' */
  ENLNG_TOKEN_SKIP,      /* 'skip' / 'continue' */

  /* Loops & Iterators */
  ENLNG_TOKEN_FOR,    /* 'for' */
  ENLNG_TOKEN_EACH,   /* 'each' / 'every' */
  ENLNG_TOKEN_IN,     /* 'in' */
  ENLNG_TOKEN_FROM,   /* 'from' */
  ENLNG_TOKEN_BY,     /* 'by' / 'step' */
  ENLNG_TOKEN_REPEAT, /* 'repeat' */
  ENLNG_TOKEN_WHILE,  /* 'while' */
  ENLNG_TOKEN_UNTIL,  /* 'until' */
  ENLNG_TOKEN_TIMES,  /* 'times' */
  ENLNG_TOKEN_PAIR,   /* 'pair' */

  /* Functions */
  ENLNG_TOKEN_FUNCTION, /* 'function' / 'define' */
  ENLNG_TOKEN_WITH,     /* 'with' / 'needs' */
  ENLNG_TOKEN_GIVE,     /* 'give' / 'return' */

  /* Actions & Intent Primitives */
  ENLNG_TOKEN_SWAP,     /* 'swap' */
  ENLNG_TOKEN_REVERSE,  /* 'reverse' */
  ENLNG_TOKEN_SORT,     /* 'sort' */
  ENLNG_TOKEN_KEEP,     /* 'keep' */
  ENLNG_TOKEN_DISCARD,  /* 'discard' */
  ENLNG_TOKEN_WHERE,    /* 'where' */
  ENLNG_TOKEN_SHOW,     /* 'show' / 'display' / 'print' */
  ENLNG_TOKEN_ASK,      /* 'ask' / 'input' */
  ENLNG_TOKEN_COUNT_OF, /* 'count of' / 'length of' / 'len' */

  /* Dual Operators: Comparisons */
  ENLNG_TOKEN_EQ,  /* '==' / 'is' / 'equals' */
  ENLNG_TOKEN_NEQ, /* '!=' / 'is not' */
  ENLNG_TOKEN_GT,  /* '>' / 'is greater than' */
  ENLNG_TOKEN_LT,  /* '<' / 'is less than' */
  ENLNG_TOKEN_GTE, /* '>=' / 'is at least' */
  ENLNG_TOKEN_LTE, /* '<=' / 'is at most' */

  /* Dual Operators: Arithmetic & Logic */
  ENLNG_TOKEN_ADD,    /* '+' / 'plus' */
  ENLNG_TOKEN_SUB,    /* '-' / 'minus' */
  ENLNG_TOKEN_MUL,    /* '*' / 'times' / 'multiplied by' */
  ENLNG_TOKEN_DIV,    /* '/' / 'divided by' */
  ENLNG_TOKEN_MOD,    /* '%' / 'mod' / 'modulo' */
  ENLNG_TOKEN_INC_BY, /* '+=' / 'increases by' */
  ENLNG_TOKEN_DEC_BY, /* '-=' / 'decreases by' */
  ENLNG_TOKEN_AND,    /* 'and' / '&&' */
  ENLNG_TOKEN_OR,     /* 'or' / '||' */
  ENLNG_TOKEN_NOT,    /* 'not' / '!' */

  /* Punctuation */
  ENLNG_TOKEN_COLON,    /* ':' */
  ENLNG_TOKEN_COMMA,    /* ',' */
  ENLNG_TOKEN_LPAREN,   /* '(' */
  ENLNG_TOKEN_RPAREN,   /* ')' */
  ENLNG_TOKEN_LBRACKET, /* '[' */
  ENLNG_TOKEN_RBRACKET, /* ']' */
  ENLNG_TOKEN_LBRACE,   /* '{' */
  ENLNG_TOKEN_RBRACE,   /* '}' */
  ENLNG_TOKEN_DOT,      /* '.' */
  ENLNG_TOKEN_USE,      /* 'use' / 'import' */
  ENLNG_TOKEN_AT,       /* 'at' */
  ENLNG_TOKEN_OF        /* 'of' */
} EnlngTokenType;

/* --- Token Structure --- */
typedef struct {
  EnlngTokenType type;
  char *text;
  int64_t int_val;
  double float_val;
  int line;
  int col;
} Token;

/* --- AST Node Types --- */
typedef enum {
  AST_PROGRAM,
  AST_DECLARATION,    /* remember x as 10 / x = 10 */
  AST_MUTATION,       /* x increases by 1 / change x to 20 */
  AST_WHEN,           /* when cond: block otherwise: block */
  AST_FOR_RANGE,      /* for i from 0 to n by 1: block */
  AST_FOR_PAIR,       /* for each pair in list: block */
  AST_FOR_EACH,       /* for each item in list: block */
  AST_REPEAT_WHILE,   /* repeat while cond: block */
  AST_REPEAT_UNTIL,   /* repeat until cond: block */
  AST_REPEAT_TIMES,   /* repeat n times: block */
  AST_FUNCTION,       /* function name with params: block */
  AST_GIVE,           /* give / return expr */
  AST_STOP,           /* stop / break */
  AST_SKIP,           /* skip / continue */
  AST_SWAP,           /* swap a and b / swap pair */
  AST_REVERSE_STMT,   /* reverse data */
  AST_SORT,           /* sort list [ascending/descending] */
  AST_INDEX_MUTATION, /* arr[i] = val */
  AST_SHOW,           /* show stream... */
  AST_ASK,            /* ask var "prompt" */
  AST_EXPR_STMT,      /* standalone expression / implicit return */

  /* Expressions */
  AST_EXPR_BINARY, /* a + b, a == b */
  AST_EXPR_UNARY,  /* not x, -x */
  AST_EXPR_LITERAL_INT,
  AST_EXPR_LITERAL_FLOAT,
  AST_EXPR_LITERAL_STRING,
  AST_EXPR_LITERAL_BOOL,
  AST_EXPR_LITERAL_NULL, /* null */
  AST_EXPR_VARIABLE,     /* x */
  AST_EXPR_INDEX,        /* arr[i] */
  AST_EXPR_FIELD,        /* pair.left, pair.right */
  AST_EXPR_CALL,         /* func(a, b) */
  AST_EXPR_COUNT_OF,     /* count of list */
  AST_EXPR_REVERSE,      /* reverse word (expression) */
  AST_EXPR_LIST_LITERAL, /* [1, 2, 3] */
  AST_EXPR_MAP_LITERAL,  /* {"key": "val"} */
  AST_USE                /* use "file.enlng" */
} ASTNodeType;

/* Forward declaration */
typedef struct ASTNode ASTNode;

/* AST Node Structure */
struct ASTNode {
  ASTNodeType type;
  int line;

  union {
    /* AST_PROGRAM */
    struct {
      ASTNode **statements;
      int count;
      int capacity;
    } program;

    /* AST_DECLARATION */
    struct {
      char *name;
      bool is_frozen;
      ASTNode *init_expr;
    } decl;

    /* AST_MUTATION */
    struct {
      char *name;
      EnlngTokenType
          op; /* ENLNG_TOKEN_ASSIGN, ENLNG_TOKEN_INC_BY, ENLNG_TOKEN_DEC_BY */
      ASTNode *val_expr;
    } mutation;

    /* AST_WHEN */
    struct {
      ASTNode *condition;
      ASTNode **then_body;
      int then_count;
      ASTNode **else_body;
      int else_count;
    } when_stmt;

    /* AST_FOR_RANGE */
    struct {
      char *var_name;
      ASTNode *start_expr;
      ASTNode *end_expr;
      ASTNode *step_expr; /* can be NULL -> 1 */
      ASTNode **body;
      int body_count;
    } for_range;

    /* AST_FOR_PAIR */
    struct {
      char *pair_name; /* e.g. "pair" */
      char *list_name; /* e.g. "numbers" */
      ASTNode **body;
      int body_count;
    } for_pair;

    /* AST_FOR_EACH */
    struct {
      char *item_name;
      char *list_name;
      ASTNode **body;
      int body_count;
    } for_each;

    /* AST_REPEAT_WHILE / AST_REPEAT_UNTIL */
    struct {
      ASTNode *condition;
      ASTNode **body;
      int body_count;
    } repeat_loop;

    /* AST_REPEAT_TIMES */
    struct {
      ASTNode *count_expr;
      ASTNode **body;
      int body_count;
    } repeat_times;

    /* AST_FUNCTION */
    struct {
      char *name;
      char **params;
      int param_count;
      ASTNode **body;
      int body_count;
    } func_def;

    /* AST_GIVE */
    struct {
      ASTNode *expr; /* can be NULL */
    } give_stmt;

    /* AST_SWAP */
    struct {
      bool is_pair;
      char *pair_name; /* if is_pair: e.g. "pair" */
      ASTNode *left;   /* if not pair: e.g. arr[i] */
      ASTNode *right;  /* if not pair: e.g. arr[j] */
    } swap_stmt;

    /* AST_REVERSE_STMT */
    struct {
      char *target_name;
    } reverse_stmt;

    /* AST_SORT */
    struct {
      char *target_name;
      bool descending;
    } sort_stmt;

    /* AST_INDEX_MUTATION */
    struct {
      char *arr_name;
      ASTNode *index_expr;
      EnlngTokenType op;
      ASTNode *val_expr;
    } index_mutation;

    /* AST_SHOW */
    struct {
      ASTNode **items;
      int count;
    } show_stmt;

    /* AST_ASK */
    struct {
      char *var_name;
      char *prompt;
    } ask_stmt;

    /* AST_EXPR_STMT */
    struct {
      ASTNode *expr;
    } expr_stmt;

    /* AST_EXPR_BINARY */
    struct {
      EnlngTokenType op;
      ASTNode *left;
      ASTNode *right;
    } binary_expr;

    /* AST_EXPR_UNARY */
    struct {
      EnlngTokenType op;
      ASTNode *operand;
    } unary_expr;

    /* AST_EXPR_LITERAL */
    struct {
      int64_t int_val;
      double float_val;
      char *string_val;
      bool bool_val;
    } literal;

    /* AST_EXPR_VARIABLE */
    struct {
      char *name;
    } variable;

    /* AST_EXPR_INDEX */
    struct {
      char *arr_name;
      ASTNode *index_expr;
    } index_expr;

    /* AST_EXPR_FIELD */
    struct {
      char *obj_name;   /* e.g. "pair" */
      char *field_name; /* "left" or "right" */
    } field_expr;

    /* AST_EXPR_CALL */
    struct {
      char *func_name;
      ASTNode **args;
      int arg_count;
    } call_expr;

    /* AST_EXPR_COUNT_OF / AST_EXPR_REVERSE */
    struct {
      ASTNode *target;
    } single_target_expr;

    /* AST_EXPR_LIST_LITERAL */
    struct {
      ASTNode **elements;
      int count;
    } list_literal;

    /* AST_EXPR_MAP_LITERAL */
    struct {
      char **keys;
      ASTNode **values;
      int count;
    } map_literal;

    /* AST_USE */
    struct {
      char *filename;
    } use_stmt;
  } as;
};

#endif /* ENLNG_COMMON_H */
