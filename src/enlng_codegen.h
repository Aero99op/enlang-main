/*
 * =====================================================================
 *   ENLANG SOVEREIGN PROGRAMMING LANGUAGE COMPILER (Pure C99 Native)
 *   src/enlng_codegen.h - C99 Code Generator Interface
 * =====================================================================
 */

#ifndef ENLNG_CODEGEN_H
#define ENLNG_CODEGEN_H

#include "enlng_common.h"

typedef struct {
    char* buffer;
    size_t size;
    size_t capacity;
    int indent_level;

    /* Context tracking */
    char current_pair_list[64];
    bool in_pair_loop;

    /* Scope tracking */
    char** scope_vars;
    int scope_var_count;
    int scope_var_cap;

    int temp_var_id;
    ASTNode* program;
} CodeGen;

void codegen_init(CodeGen* cg);
void codegen_free(CodeGen* cg);
char* codegen_generate(CodeGen* cg, ASTNode* program);

#endif /* ENLNG_CODEGEN_H */
