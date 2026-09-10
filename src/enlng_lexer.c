/*
 * =====================================================================
 *   ENLANG SOVEREIGN PROGRAMMING LANGUAGE COMPILER (Pure C99 Native)
 *   src/enlng_lexer.c - Lexical Scanner, Indentation & Silent Words
 * =====================================================================
 */

#include "enlng_lexer.h"
#include <ctype.h>

void lexer_init(Lexer *lexer, const char *source) {
  lexer->source = source;
  lexer->length = strlen(source);
  lexer->cursor = 0;
  lexer->line = 1;
  lexer->col = 1;

  lexer->indent_stack[0] = 0;
  lexer->indent_level = 0;
  lexer->pending_dedents = 0;
  lexer->at_line_start = true;

  lexer->token_capacity = 256;
  lexer->token_count = 0;
  lexer->tokens = (Token *)malloc(sizeof(Token) * lexer->token_capacity);
}

void token_free(Token *token) {
  if (token->text) {
    free(token->text);
    token->text = NULL;
  }
}

void lexer_free(Lexer *lexer) {
  for (int i = 0; i < lexer->token_count; i++) {
    token_free(&lexer->tokens[i]);
  }
  if (lexer->tokens) {
    free(lexer->tokens);
    lexer->tokens = NULL;
  }
}

static void add_token(Lexer *lexer, EnlngTokenType type, const char *text,
                      int line, int col) {
  if (lexer->token_count >= lexer->token_capacity) {
    lexer->token_capacity *= 2;
    lexer->tokens =
        (Token *)realloc(lexer->tokens, sizeof(Token) * lexer->token_capacity);
  }
  Token *t = &lexer->tokens[lexer->token_count++];
  t->type = type;
  t->text = text ? enlng_strdup(text) : NULL;
  t->int_val = 0;
  t->float_val = 0.0;
  t->line = line;
  t->col = col;
}

static char peek(Lexer *lexer, int offset) {
  if (lexer->cursor + offset >= lexer->length)
    return '\0';
  return lexer->source[lexer->cursor + offset];
}

static char advance(Lexer *lexer) {
  if (lexer->cursor >= lexer->length)
    return '\0';
  char c = lexer->source[lexer->cursor++];
  if (c == '\n') {
    lexer->line++;
    lexer->col = 1;
  } else {
    lexer->col++;
  }
  return c;
}

static bool match_word(Lexer *lexer, const char *word) {
  size_t len = strlen(word);
  if (lexer->cursor + len > lexer->length)
    return false;
  if (strncmp(&lexer->source[lexer->cursor], word, len) == 0) {
    char next = lexer->source[lexer->cursor + len];
    if (!isalnum((unsigned char)next) && next != '_') {
      for (size_t i = 0; i < len; i++)
        advance(lexer);
      return true;
    }
  }
  return false;
}

static void skip_spaces_only(Lexer *lexer) {
  while (peek(lexer, 0) == ' ' || peek(lexer, 0) == '\t') {
    advance(lexer);
  }
}

bool lexer_tokenize(Lexer *lexer) {
  while (lexer->cursor < lexer->length) {
    /* Handle line beginning & indentation */
    if (lexer->at_line_start) {
      int indent = 0;
      size_t temp_cursor = lexer->cursor;
      int temp_line = lexer->line;
      int temp_col = lexer->col;

      while (temp_cursor < lexer->length) {
        char c = lexer->source[temp_cursor];
        if (c == ' ') {
          indent++;
          temp_cursor++;
        } else if (c == '\t') {
          indent += 4;
          temp_cursor++;
        } else
          break;
      }

      /* Blank line or comment line */
      if (temp_cursor < lexer->length && (lexer->source[temp_cursor] == '\n' ||
                                          lexer->source[temp_cursor] == '\r' ||
                                          lexer->source[temp_cursor] == '#')) {
        lexer->cursor = temp_cursor;
        lexer->line = temp_line;
        lexer->col = temp_col;
        while (peek(lexer, 0) != '\n' && peek(lexer, 0) != '\0')
          advance(lexer);
        if (peek(lexer, 0) == '\n')
          advance(lexer);
        continue;
      }

      /* Indentation change */
      lexer->cursor = temp_cursor;
      lexer->col += indent;

      int current_indent = lexer->indent_stack[lexer->indent_level];
      if (indent > current_indent) {
        if (lexer->indent_level < 127) {
          lexer->indent_level++;
          lexer->indent_stack[lexer->indent_level] = indent;
          add_token(lexer, ENLNG_TOKEN_INDENT, "INDENT", lexer->line,
                    lexer->col);
        }
      } else if (indent < current_indent) {
        while (lexer->indent_level > 0 &&
               lexer->indent_stack[lexer->indent_level] > indent) {
          lexer->indent_level--;
          add_token(lexer, ENLNG_TOKEN_DEDENT, "DEDENT", lexer->line,
                    lexer->col);
        }
      }
      lexer->at_line_start = false;
    }

    char c = peek(lexer, 0);

    /* Skip inline spaces */
    if (c == ' ' || c == '\t' || c == '\r') {
      advance(lexer);
      continue;
    }

    /* Comments */
    if (c == '#') {
      while (peek(lexer, 0) != '\n' && peek(lexer, 0) != '\0')
        advance(lexer);
      continue;
    }

    /* Newline */
    if (c == '\n') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_NEWLINE, "NEWLINE", lexer->line - 1,
                lexer->col);
      lexer->at_line_start = true;
      continue;
    }

    int start_line = lexer->line;
    int start_col = lexer->col;

    /* Punctuation & Operators */
    if (c == ':') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_COLON, ":", start_line, start_col);
      continue;
    }
    if (c == ',') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_COMMA, ",", start_line, start_col);
      continue;
    }
    if (c == '(') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_LPAREN, "(", start_line, start_col);
      continue;
    }
    if (c == ')') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_RPAREN, ")", start_line, start_col);
      continue;
    }
    if (c == '[') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_LBRACKET, "[", start_line, start_col);
      continue;
    }
    if (c == ']') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_RBRACKET, "]", start_line, start_col);
      continue;
    }
    if (c == '{') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_LBRACE, "{", start_line, start_col);
      continue;
    }
    if (c == '}') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_RBRACE, "}", start_line, start_col);
      continue;
    }
    if (c == '.') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_DOT, ".", start_line, start_col);
      continue;
    }

    if (c == '=' && peek(lexer, 1) == '=') {
      advance(lexer);
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_EQ, "==", start_line, start_col);
      continue;
    }
    if (c == '!' && peek(lexer, 1) == '=') {
      advance(lexer);
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_NEQ, "!=", start_line, start_col);
      continue;
    }
    if (c == '>' && peek(lexer, 1) == '=') {
      advance(lexer);
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_GTE, ">=", start_line, start_col);
      continue;
    }
    if (c == '<' && peek(lexer, 1) == '=') {
      advance(lexer);
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_LTE, "<=", start_line, start_col);
      continue;
    }
    if (c == '+') {
      if (peek(lexer, 1) == '=') {
        advance(lexer);
        advance(lexer);
        add_token(lexer, ENLNG_TOKEN_INC_BY, "+=", start_line, start_col);
        continue;
      }
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_ADD, "+", start_line, start_col);
      continue;
    }
    if (c == '-') {
      if (peek(lexer, 1) == '=') {
        advance(lexer);
        advance(lexer);
        add_token(lexer, ENLNG_TOKEN_DEC_BY, "-=", start_line, start_col);
        continue;
      }
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_SUB, "-", start_line, start_col);
      continue;
    }
    if (c == '*') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_MUL, "*", start_line, start_col);
      continue;
    }
    if (c == '/') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_DIV, "/", start_line, start_col);
      continue;
    }
    if (c == '%') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_MOD, "%", start_line, start_col);
      continue;
    }
    if (c == '>') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_GT, ">", start_line, start_col);
      continue;
    }
    if (c == '<') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_LT, "<", start_line, start_col);
      continue;
    }
    if (c == '=') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_ASSIGN, "=", start_line, start_col);
      continue;
    }
    if (c == '!') {
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_NOT, "!", start_line, start_col);
      continue;
    }
    if (c == '&' && peek(lexer, 1) == '&') {
      advance(lexer);
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_AND, "&&", start_line, start_col);
      continue;
    }
    if (c == '|' && peek(lexer, 1) == '|') {
      advance(lexer);
      advance(lexer);
      add_token(lexer, ENLNG_TOKEN_OR, "||", start_line, start_col);
      continue;
    }

    /* String Literals */
    if (c == '"' || c == '\'') {
      char quote = advance(lexer);
      size_t str_cap = 64;
      size_t str_len = 0;
      char *str_buf = (char *)malloc(str_cap);

      while (peek(lexer, 0) != quote && peek(lexer, 0) != '\0' &&
             peek(lexer, 0) != '\n') {
        char sc = advance(lexer);
        if (sc == '\\') {
          char esc = advance(lexer);
          if (esc == 'n')
            sc = '\n';
          else if (esc == 't')
            sc = '\t';
          else if (esc == 'r')
            sc = '\r';
          else if (esc == '\\')
            sc = '\\';
          else if (esc == quote)
            sc = quote;
          else
            sc = esc;
        }
        if (str_len + 2 >= str_cap) {
          str_cap *= 2;
          str_buf = (char *)realloc(str_buf, str_cap);
        }
        str_buf[str_len++] = sc;
      }
      if (peek(lexer, 0) == quote)
        advance(lexer);
      str_buf[str_len] = '\0';

      add_token(lexer, ENLNG_TOKEN_STRING_LITERAL, str_buf, start_line,
                start_col);
      free(str_buf);
      continue;
    }

    /* Number Literals */
    if (isdigit((unsigned char)c)) {
      char num_buf[64];
      int nlen = 0;
      bool is_float = false;
      while (
          isdigit((unsigned char)peek(lexer, 0)) ||
          (peek(lexer, 0) == '.' && isdigit((unsigned char)peek(lexer, 1)))) {
        if (peek(lexer, 0) == '.')
          is_float = true;
        if (nlen < 63)
          num_buf[nlen++] = advance(lexer);
        else
          advance(lexer);
      }
      num_buf[nlen] = '\0';

      if (is_float) {
        add_token(lexer, ENLNG_TOKEN_FLOAT_LITERAL, num_buf, start_line,
                  start_col);
        lexer->tokens[lexer->token_count - 1].float_val = atof(num_buf);
      } else {
        add_token(lexer, ENLNG_TOKEN_INT_LITERAL, num_buf, start_line,
                  start_col);
        lexer->tokens[lexer->token_count - 1].int_val = atoll(num_buf);
      }
      continue;
    }

    /* Silent Words Absorption (Articles, Fillers: 'the', 'a', 'an', 'that',
     * 'it') */
    if (match_word(lexer, "the") || match_word(lexer, "that") ||
        match_word(lexer, "it")) {
      /* Silently absorbed! */
      continue;
    }

    /* Multi-Word Compound Keywords */
    if (match_word(lexer, "increases")) {
      skip_spaces_only(lexer);
      if (match_word(lexer, "by")) {
        add_token(lexer, ENLNG_TOKEN_INC_BY, "increases by", start_line,
                  start_col);
        continue;
      }
      add_token(lexer, ENLNG_TOKEN_IDENTIFIER, "increases", start_line,
                start_col);
      continue;
    }
    if (match_word(lexer, "decreases")) {
      skip_spaces_only(lexer);
      if (match_word(lexer, "by")) {
        add_token(lexer, ENLNG_TOKEN_DEC_BY, "decreases by", start_line,
                  start_col);
        continue;
      }
      add_token(lexer, ENLNG_TOKEN_IDENTIFIER, "decreases", start_line,
                start_col);
      continue;
    }
    if (match_word(lexer, "repeat")) {
      skip_spaces_only(lexer);
      if (match_word(lexer, "while")) {
        add_token(lexer, ENLNG_TOKEN_WHILE, "repeat while", start_line,
                  start_col);
        continue;
      }
      if (match_word(lexer, "until")) {
        add_token(lexer, ENLNG_TOKEN_UNTIL, "repeat until", start_line,
                  start_col);
        continue;
      }
      add_token(lexer, ENLNG_TOKEN_REPEAT, "repeat", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "count")) {
      skip_spaces_only(lexer);
      if (match_word(lexer, "of")) {
        add_token(lexer, ENLNG_TOKEN_COUNT_OF, "count of", start_line,
                  start_col);
        continue;
      }
      add_token(lexer, ENLNG_TOKEN_IDENTIFIER, "count", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "length")) {
      skip_spaces_only(lexer);
      if (match_word(lexer, "of")) {
        add_token(lexer, ENLNG_TOKEN_COUNT_OF, "length of", start_line,
                  start_col);
        continue;
      }
      add_token(lexer, ENLNG_TOKEN_IDENTIFIER, "length", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "for")) {
      skip_spaces_only(lexer);
      if (match_word(lexer, "each") || match_word(lexer, "every")) {
        add_token(lexer, ENLNG_TOKEN_FOR, "for", start_line, start_col);
        add_token(lexer, ENLNG_TOKEN_EACH, "each", start_line, start_col);
        continue;
      }
      add_token(lexer, ENLNG_TOKEN_FOR, "for", start_line, start_col);
      continue;
    }

    /* Comparison Dual Operators */
    if (match_word(lexer, "is")) {
      skip_spaces_only(lexer);
      if (match_word(lexer, "greater")) {
        skip_spaces_only(lexer);
        if (match_word(lexer, "than")) {
          skip_spaces_only(lexer);
          if (match_word(lexer, "or") && match_word(lexer, "equal") &&
              match_word(lexer, "to")) {
            add_token(lexer, ENLNG_TOKEN_GTE, ">=", start_line, start_col);
            continue;
          }
          add_token(lexer, ENLNG_TOKEN_GT, ">", start_line, start_col);
          continue;
        }
      }
      if (match_word(lexer, "less")) {
        skip_spaces_only(lexer);
        if (match_word(lexer, "than")) {
          skip_spaces_only(lexer);
          if (match_word(lexer, "or") && match_word(lexer, "equal") &&
              match_word(lexer, "to")) {
            add_token(lexer, ENLNG_TOKEN_LTE, "<=", start_line, start_col);
            continue;
          }
          add_token(lexer, ENLNG_TOKEN_LT, "<", start_line, start_col);
          continue;
        }
      }
      if (match_word(lexer, "at")) {
        skip_spaces_only(lexer);
        if (match_word(lexer, "least")) {
          add_token(lexer, ENLNG_TOKEN_GTE, ">=", start_line, start_col);
          continue;
        }
        if (match_word(lexer, "most")) {
          add_token(lexer, ENLNG_TOKEN_LTE, "<=", start_line, start_col);
          continue;
        }
      }
      if (match_word(lexer, "not")) {
        skip_spaces_only(lexer);
        if (match_word(lexer, "equal") && match_word(lexer, "to")) {
          add_token(lexer, ENLNG_TOKEN_NEQ, "!=", start_line, start_col);
          continue;
        }
        add_token(lexer, ENLNG_TOKEN_NEQ, "!=", start_line, start_col);
        continue;
      }
      if (match_word(lexer, "equal")) {
        skip_spaces_only(lexer);
        if (match_word(lexer, "to")) {
          add_token(lexer, ENLNG_TOKEN_EQ, "==", start_line, start_col);
          continue;
        }
        add_token(lexer, ENLNG_TOKEN_EQ, "==", start_line, start_col);
        continue;
      }
      /* Lone 'is' -> ENLNG_TOKEN_EQ */
      add_token(lexer, ENLNG_TOKEN_EQ, "==", start_line, start_col);
      continue;
    }

    if (match_word(lexer, "at")) {
      skip_spaces_only(lexer);
      if (match_word(lexer, "least")) {
        add_token(lexer, ENLNG_TOKEN_GTE, ">=", start_line, start_col);
        continue;
      }
      if (match_word(lexer, "most")) {
        add_token(lexer, ENLNG_TOKEN_LTE, "<=", start_line, start_col);
        continue;
      }
      add_token(lexer, ENLNG_TOKEN_AT, "at", start_line, start_col);
      continue;
    }

    if (match_word(lexer, "of")) {
      add_token(lexer, ENLNG_TOKEN_OF, "of", start_line, start_col);
      continue;
    }

    /* Arithmetic Words */
    if (match_word(lexer, "plus")) {
      add_token(lexer, ENLNG_TOKEN_ADD, "+", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "minus")) {
      add_token(lexer, ENLNG_TOKEN_SUB, "-", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "times")) {
      add_token(lexer, ENLNG_TOKEN_MUL, "*", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "multiplied")) {
      skip_spaces_only(lexer);
      if (match_word(lexer, "by")) {
        add_token(lexer, ENLNG_TOKEN_MUL, "*", start_line, start_col);
        continue;
      }
      add_token(lexer, ENLNG_TOKEN_IDENTIFIER, "multiplied", start_line,
                start_col);
      continue;
    }
    if (match_word(lexer, "divided")) {
      skip_spaces_only(lexer);
      if (match_word(lexer, "by")) {
        add_token(lexer, ENLNG_TOKEN_DIV, "/", start_line, start_col);
        continue;
      }
      add_token(lexer, ENLNG_TOKEN_IDENTIFIER, "divided", start_line,
                start_col);
      continue;
    }
    if (match_word(lexer, "mod") || match_word(lexer, "modulo")) {
      add_token(lexer, ENLNG_TOKEN_MOD, "%", start_line, start_col);
      continue;
    }

    /* Standard Keywords */
    if (match_word(lexer, "type")) {
      add_token(lexer, ENLNG_TOKEN_KW_TYPE, "type", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "enlng")) {
      add_token(lexer, ENLNG_TOKEN_DOMAIN_ENLNG, "enlng", start_line,
                start_col);
      continue;
    }
    if (match_word(lexer, "remember")) {
      add_token(lexer, ENLNG_TOKEN_REMEMBER, "remember", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "freeze")) {
      add_token(lexer, ENLNG_TOKEN_FREEZE, "freeze", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "as")) {
      add_token(lexer, ENLNG_TOKEN_AS, "as", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "change")) {
      add_token(lexer, ENLNG_TOKEN_CHANGE, "change", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "to")) {
      add_token(lexer, ENLNG_TOKEN_TO, "to", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "forget")) {
      add_token(lexer, ENLNG_TOKEN_FORGET, "forget", start_line, start_col);
      continue;
    }

    if (match_word(lexer, "when") || match_word(lexer, "if")) {
      add_token(lexer, ENLNG_TOKEN_WHEN, "when", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "otherwise") || match_word(lexer, "else")) {
      add_token(lexer, ENLNG_TOKEN_OTHERWISE, "otherwise", start_line,
                start_col);
      continue;
    }
    if (match_word(lexer, "stop") || match_word(lexer, "break")) {
      add_token(lexer, ENLNG_TOKEN_STOP, "stop", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "skip") || match_word(lexer, "continue")) {
      add_token(lexer, ENLNG_TOKEN_SKIP, "skip", start_line, start_col);
      continue;
    }

    if (match_word(lexer, "each")) {
      add_token(lexer, ENLNG_TOKEN_EACH, "each", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "in")) {
      add_token(lexer, ENLNG_TOKEN_IN, "in", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "from")) {
      add_token(lexer, ENLNG_TOKEN_FROM, "from", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "by") || match_word(lexer, "step")) {
      add_token(lexer, ENLNG_TOKEN_BY, "by", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "while")) {
      add_token(lexer, ENLNG_TOKEN_WHILE, "while", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "until")) {
      add_token(lexer, ENLNG_TOKEN_UNTIL, "until", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "pair")) {
      add_token(lexer, ENLNG_TOKEN_PAIR, "pair", start_line, start_col);
      continue;
    }

    if (match_word(lexer, "function") || match_word(lexer, "define") ||
        match_word(lexer, "action")) {
      add_token(lexer, ENLNG_TOKEN_FUNCTION, "function", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "with") || match_word(lexer, "needs")) {
      add_token(lexer, ENLNG_TOKEN_WITH, "with", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "give") || match_word(lexer, "return")) {
      add_token(lexer, ENLNG_TOKEN_GIVE, "give", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "use") || match_word(lexer, "import")) {
      add_token(lexer, ENLNG_TOKEN_USE, "use", start_line, start_col);
      continue;
    }

    if (match_word(lexer, "swap")) {
      add_token(lexer, ENLNG_TOKEN_SWAP, "swap", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "reverse")) {
      add_token(lexer, ENLNG_TOKEN_REVERSE, "reverse", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "sort")) {
      add_token(lexer, ENLNG_TOKEN_SORT, "sort", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "keep")) {
      add_token(lexer, ENLNG_TOKEN_KEEP, "keep", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "discard")) {
      add_token(lexer, ENLNG_TOKEN_DISCARD, "discard", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "where")) {
      add_token(lexer, ENLNG_TOKEN_WHERE, "where", start_line, start_col);
      continue;
    }

    if (match_word(lexer, "show") || match_word(lexer, "display") ||
        match_word(lexer, "print")) {
      add_token(lexer, ENLNG_TOKEN_SHOW, "show", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "ask") || match_word(lexer, "input")) {
      add_token(lexer, ENLNG_TOKEN_ASK, "ask", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "len")) {
      add_token(lexer, ENLNG_TOKEN_COUNT_OF, "count of", start_line, start_col);
      continue;
    }

    if (match_word(lexer, "and")) {
      add_token(lexer, ENLNG_TOKEN_AND, "and", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "or")) {
      add_token(lexer, ENLNG_TOKEN_OR, "or", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "not")) {
      add_token(lexer, ENLNG_TOKEN_NOT, "not", start_line, start_col);
      continue;
    }
    if (match_word(lexer, "true")) {
      add_token(lexer, ENLNG_TOKEN_BOOL_LITERAL, "true", start_line, start_col);
      lexer->tokens[lexer->token_count - 1].int_val = 1;
      continue;
    }
    if (match_word(lexer, "false")) {
      add_token(lexer, ENLNG_TOKEN_BOOL_LITERAL, "false", start_line,
                start_col);
      lexer->tokens[lexer->token_count - 1].int_val = 0;
      continue;
    }
    if (match_word(lexer, "null") || match_word(lexer, "none") ||
        match_word(lexer, "nothing")) {
      add_token(lexer, ENLNG_TOKEN_NULL, "null", start_line, start_col);
      continue;
    }

    /* General Identifier */
    if (isalpha((unsigned char)c) || c == '_') {
      char id_buf[128];
      int id_len = 0;
      while (isalnum((unsigned char)peek(lexer, 0)) || peek(lexer, 0) == '_') {
        if (id_len < 127)
          id_buf[id_len++] = advance(lexer);
        else
          advance(lexer);
      }
      id_buf[id_len] = '\0';
      add_token(lexer, ENLNG_TOKEN_IDENTIFIER, id_buf, start_line, start_col);
      continue;
    }

    /* Unknown character fallback */
    advance(lexer);
  }

  /* Trailing Dedents */
  while (lexer->indent_level > 0) {
    lexer->indent_level--;
    add_token(lexer, ENLNG_TOKEN_DEDENT, "DEDENT", lexer->line, lexer->col);
  }

  add_token(lexer, ENLNG_TOKEN_EOF, "EOF", lexer->line, lexer->col);
  return true;
}
