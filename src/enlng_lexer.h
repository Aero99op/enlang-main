/*
 * =====================================================================
 *   ENLANG SOVEREIGN PROGRAMMING LANGUAGE COMPILER (Pure C99 Native)
 *   src/enlng_lexer.h - Lexical Scanner & Indentation Tracker Interface
 * =====================================================================
 */

#ifndef ENLNG_LEXER_H
#define ENLNG_LEXER_H

#include "enlng_common.h"

typedef struct {
    const char* source;
    size_t length;
    size_t cursor;
    int line;
    int col;

    /* Off-side Indentation Stack */
    int indent_stack[128];
    int indent_level;

    /* Pending Dedent count */
    int pending_dedents;
    bool at_line_start;

    /* Tokens array */
    Token* tokens;
    int token_count;
    int token_capacity;
} Lexer;

void lexer_init(Lexer* lexer, const char* source);
void lexer_free(Lexer* lexer);
bool lexer_tokenize(Lexer* lexer);
void token_free(Token* token);

#endif /* ENLNG_LEXER_H */
