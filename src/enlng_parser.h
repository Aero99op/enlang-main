/*
 * =====================================================================
 *   ENLANG SOVEREIGN PROGRAMMING LANGUAGE COMPILER (Pure C99 Native)
 *   src/enlng_parser.h - Recursive Descent AST Parser Interface
 * =====================================================================
 */

#ifndef ENLNG_PARSER_H
#define ENLNG_PARSER_H

#include "enlng_common.h"
#include "enlng_lexer.h"

typedef struct {
    Token* tokens;
    int token_count;
    int cursor;

    bool has_error;
    char error_msg[512];
    int error_line;
} Parser;

void parser_init(Parser* parser, Token* tokens, int count);
ASTNode* parser_parse_program(Parser* parser);
void ast_free(ASTNode* node);

#endif /* ENLNG_PARSER_H */
