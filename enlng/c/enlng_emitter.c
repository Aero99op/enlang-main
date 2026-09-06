#include "enlng_emitter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

/*
 * ============================================================================
 *           SOVEREIGN AHEAD-OF-TIME (AOT) C-EMITTER COMPILER
 * ============================================================================
 * Translates Natural English (.enlng) into clean ISO C99 source code.
 * Integrates Scoped Bump Arena Memory Management (enlng_mem.h) automatically.
 * Zero manual free(). Zero Python dependencies for pure native execution.
 * ============================================================================
 */

#define MAX_VARS 512
#define MAX_LINE 2048

typedef struct {
    char name[64];
    int scope_depth;
} VarSymbol;

typedef struct {
    VarSymbol vars[MAX_VARS];
    int var_count;
    int current_depth;
    int indent_stack[64];
    int indent_stack_ptr;
    bool in_loop_stack[64];
    bool in_func_stack[64];
} EmitterContext;

static char* trim_str(char* s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char* end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

static bool starts_with_ci(const char* s, const char* prefix) {
    while (*prefix) {
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) return false;
        s++;
        prefix++;
    }
    return true;
}

bool enlng_has_python_dependency(const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return false;
    char line[MAX_LINE];
    bool has_py = false;
    while (fgets(line, sizeof(line), f)) {
        char* t = trim_str(line);
        if (starts_with_ci(t, "import python") ||
            strstr(t, "\"numpy\"") || strstr(t, "'numpy'") ||
            strstr(t, "\"torch\"") || strstr(t, "'torch'") ||
            strstr(t, "\"pandas\"") || strstr(t, "'pandas'") ||
            strstr(t, "\"scipy\"") || strstr(t, "'scipy'") ||
            strstr(t, "\"sklearn\"") || strstr(t, "'sklearn'")) {
            has_py = true;
            break;
        }
    }
    fclose(f);
    return has_py;
}

static bool var_is_defined(EmitterContext* ctx, const char* name) {
    for (int i = 0; i < ctx->var_count; i++) {
        if (strcmp(ctx->vars[i].name, name) == 0) return true;
    }
    return false;
}

static void var_define(EmitterContext* ctx, const char* name) {
    if (ctx->var_count < MAX_VARS) {
        strncpy(ctx->vars[ctx->var_count].name, name, 63);
        ctx->vars[ctx->var_count].name[63] = '\0';
        ctx->vars[ctx->var_count].scope_depth = ctx->current_depth;
        ctx->var_count++;
    }
}

/*
 * Translates atomic natural English expressions into C runtime calls
 */
static void translate_atomic_expr(const char* expr, char* out, size_t out_sz) {
    char clean[1024];
    strncpy(clean, expr, sizeof(clean) - 1);
    clean[sizeof(clean) - 1] = '\0';
    char* t = trim_str(clean);

    if (strlen(t) == 0) {
        snprintf(out, out_sz, "enlng_null()");
        return;
    }

    // String literal
    if ((t[0] == '"' && t[strlen(t) - 1] == '"') || (t[0] == '\'' && t[strlen(t) - 1] == '\'')) {
        snprintf(out, out_sz, "enlng_str(%s)", t);
        return;
    }

    // Boolean
    if (strcmp(t, "true") == 0 || strcmp(t, "True") == 0) {
        snprintf(out, out_sz, "enlng_bool(true)");
        return;
    }
    if (strcmp(t, "false") == 0 || strcmp(t, "False") == 0) {
        snprintf(out, out_sz, "enlng_bool(false)");
        return;
    }
    if (strcmp(t, "null") == 0 || strcmp(t, "None") == 0) {
        snprintf(out, out_sz, "enlng_null()");
        return;
    }

    // Number
    char* endp = NULL;
    strtoll(t, &endp, 10);
    if (endp && *endp == '\0' && !strchr(t, '.')) {
        snprintf(out, out_sz, "enlng_int(%sLL)", t);
        return;
    }
    strtod(t, &endp);
    if (endp && *endp == '\0' && strchr(t, '.')) {
        snprintf(out, out_sz, "enlng_double(%s)", t);
        return;
    }

    // List literal [a, b, c]
    if (t[0] == '[' && t[strlen(t) - 1] == ']') {
        snprintf(out, out_sz, "enlng_list_create(8)");
        return;
    }

    // Default: treat as variable name or raw C identifier
    snprintf(out, out_sz, "%s", t);
}

static char* find_op_outside_parens(char* s, const char* op) {
    size_t op_len = strlen(op);
    int p_depth = 0;
    bool in_q = false;
    char qc = 0;

    for (char* p = s; *p; p++) {
        if (!in_q && (*p == '"' || *p == '\'')) { in_q = true; qc = *p; }
        else if (in_q && *p == qc) { in_q = false; }
        else if (!in_q) {
            if (*p == '(' || *p == '[') p_depth++;
            else if (*p == ')' || *p == ']') p_depth--;
            else if (p_depth == 0) {
                if (strncmp(p, op, op_len) == 0) {
                    return p;
                }
            }
        }
    }
    return NULL;
}

/*
 * High-level expression translator with operator precedence
 */
static void translate_expression(const char* expr, char* out, size_t out_sz) {
    char clean[1024];
    strncpy(clean, expr, sizeof(clean) - 1);
    clean[sizeof(clean) - 1] = '\0';
    char* s = trim_str(clean);

    // Strip balanced enclosing parentheses if present
    size_t slen = strlen(s);
    while (slen >= 2 && s[0] == '(' && s[slen - 1] == ')') {
        int depth = 0;
        bool matching = true;
        for (size_t i = 0; i < slen - 1; i++) {
            if (s[i] == '(') depth++;
            else if (s[i] == ')') depth--;
            if (depth == 0) { matching = false; break; }
        }
        if (matching) {
            s[slen - 1] = '\0';
            s++;
            s = trim_str(s);
            slen = strlen(s);
        } else {
            break;
        }
    }

    // Operator table in order of precedence
    static const struct {
        const char* phrase;
        const char* func;
    } OP_MAP[] = {
        {" is greater than or equal to ", "enlng_gte"},
        {" is less than or equal to ",    "enlng_lte"},
        {" is not equal to ",            "enlng_neq"},
        {" is equal to ",                "enlng_eq"},
        {" is greater than ",            "enlng_gt"},
        {" is less than ",               "enlng_lt"},
        {" >= ",                         "enlng_gte"},
        {" <= ",                         "enlng_lte"},
        {" != ",                         "enlng_neq"},
        {" == ",                         "enlng_eq"},
        {" > ",                          "enlng_gt"},
        {" < ",                          "enlng_lt"},
        {" plus ",                       "enlng_add"},
        {" minus ",                      "enlng_sub"},
        {" multiplied by ",              "enlng_mul"},
        {" divided by ",                 "enlng_div"},
        {" modulo ",                     "enlng_mod"},
        {" + ",                          "enlng_add"},
        {" - ",                          "enlng_sub"},
        {" * ",                          "enlng_mul"},
        {" / ",                          "enlng_div"},
        {" % ",                          "enlng_mod"},
        {NULL, NULL}
    };

    for (int i = 0; OP_MAP[i].phrase != NULL; i++) {
        char* pos = find_op_outside_parens(s, OP_MAP[i].phrase);
        if (pos) {
            *pos = '\0';
            char* left = trim_str(s);
            char* right = trim_str(pos + strlen(OP_MAP[i].phrase));

            char left_c[512], right_c[512];
            translate_expression(left, left_c, sizeof(left_c));
            translate_expression(right, right_c, sizeof(right_c));

            if (strncmp(OP_MAP[i].func, "enlng_is_", 9) == 0 ||
                strcmp(OP_MAP[i].func, "enlng_gte") == 0 ||
                strcmp(OP_MAP[i].func, "enlng_lte") == 0 ||
                strcmp(OP_MAP[i].func, "enlng_neq") == 0 ||
                strcmp(OP_MAP[i].func, "enlng_eq") == 0 ||
                strcmp(OP_MAP[i].func, "enlng_gt") == 0 ||
                strcmp(OP_MAP[i].func, "enlng_lt") == 0) {
                snprintf(out, out_sz, "enlng_bool(%s(%s, %s))", OP_MAP[i].func, left_c, right_c);
            } else {
                snprintf(out, out_sz, "%s(%s, %s)", OP_MAP[i].func, left_c, right_c);
            }
            return;
        }
    }

    // Atomic fallback
    translate_atomic_expr(s, out, out_sz);
}

bool enlng_emit_c_from_file(const char* in_filepath, const char* out_c_filepath) {
    FILE* fin = fopen(in_filepath, "r");
    if (!fin) {
        fprintf(stderr, "[ENLNG ERROR] Could not open source file: %s\n", in_filepath);
        return false;
    }

    FILE* fout = fopen(out_c_filepath, "w");
    if (!fout) {
        fclose(fin);
        fprintf(stderr, "[ENLNG ERROR] Could not open destination C file: %s\n", out_c_filepath);
        return false;
    }

    // Emit Header and Scoped Arena Memory Engine
    fprintf(fout, "/* Generated by Enlangg Sovereign Ahead-Of-Time (AOT) C Compiler */\n");
    fprintf(fout, "#include <stdio.h>\n");
    fprintf(fout, "#include <stdlib.h>\n");
    fprintf(fout, "#include <string.h>\n");
    fprintf(fout, "#include <stdint.h>\n");
    fprintf(fout, "#include <stdbool.h>\n");
    fprintf(fout, "#include <math.h>\n\n");
    fprintf(fout, "#include \"enlng/c/enlng_mem.h\"\n\n");

    EmitterContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.indent_stack[0] = 0;
    ctx.indent_stack_ptr = 0;

    char line[MAX_LINE];
    char main_body[65536];
    char funcs_body[65536];
    main_body[0] = '\0';
    funcs_body[0] = '\0';

    bool inside_function = false;

    while (fgets(line, sizeof(line), fin)) {
        // Measure indentation
        int indent = 0;
        while (line[indent] == ' ') indent++;
        if (line[indent] == '\t') indent += 4;

        char* trimmed = trim_str(line);
        if (strlen(trimmed) == 0) continue;
        if (trimmed[0] == '#' || (trimmed[0] == '/' && trimmed[1] == '/')) continue;

        // Skip type enlng declarations
        if (starts_with_ci(trimmed, "type enlng")) continue;

        bool is_else = (strcmp(trimmed, "else:") == 0 || strcmp(trimmed, "otherwise:") == 0 ||
                        starts_with_ci(trimmed, "else if ") || starts_with_ci(trimmed, "otherwise if ") || starts_with_ci(trimmed, "elif "));

        // Indentation unwind
        while (ctx.indent_stack_ptr > 0 && indent < ctx.indent_stack[ctx.indent_stack_ptr]) {
            char* target = inside_function ? funcs_body : main_body;
            if (!is_else || (indent + 4 < ctx.indent_stack[ctx.indent_stack_ptr])) {
                if (ctx.in_loop_stack[ctx.indent_stack_ptr]) {
                    strcat(target, "    ENLNG_SCOPE_END();\n    }\n");
                } else if (ctx.in_func_stack[ctx.indent_stack_ptr]) {
                    strcat(target, "    ENLNG_SCOPE_END();\n    return enlng_null();\n}\n\n");
                    inside_function = false;
                } else {
                    strcat(target, "    }\n");
                }
            }
            ctx.indent_stack_ptr--;
        }

        char* target = inside_function ? funcs_body : main_body;

        // 1. Variable Assignment / Declaration: set x to ... / create x of ...
        if (starts_with_ci(trimmed, "set ") || starts_with_ci(trimmed, "create ")) {
            bool is_create = starts_with_ci(trimmed, "create ");
            const char* rest = is_create ? (trimmed + 7) : (trimmed + 4);
            char var_name[64];
            char expr_part[512];

            const char* to_pos = strstr(rest, is_create ? " of " : " to ");
            if (to_pos) {
                int vlen = (int)(to_pos - rest);
                if (vlen >= (int)sizeof(var_name)) vlen = (int)sizeof(var_name) - 1;
                strncpy(var_name, rest, vlen);
                var_name[vlen] = '\0';
                trim_str(var_name);

                const char* expr_src = to_pos + (is_create ? 4 : 4);
                char c_expr[512];
                translate_expression(expr_src, c_expr, sizeof(c_expr));

                if (!var_is_defined(&ctx, var_name)) {
                    var_define(&ctx, var_name);
                    char buf[1024];
                    snprintf(buf, sizeof(buf), "    EnlngVal %s = %s;\n", var_name, c_expr);
                    strcat(target, buf);
                } else {
                    char buf[1024];
                    snprintf(buf, sizeof(buf), "    %s = %s;\n", var_name, c_expr);
                    strcat(target, buf);
                }
            }
            continue;
        }

        // 2. Display Statements
        if (starts_with_ci(trimmed, "display ") || starts_with_ci(trimmed, "print ")) {
            const char* args_str = starts_with_ci(trimmed, "display ") ? (trimmed + 8) : (trimmed + 6);
            char args_copy[512];
            strncpy(args_copy, args_str, sizeof(args_copy) - 1);
            args_copy[sizeof(args_copy) - 1] = '\0';

            char* p = args_copy;
            char arg_c_tokens[16][256];
            int arg_count = 0;

            while (*p && arg_count < 16) {
                while (isspace((unsigned char)*p)) p++;
                if (!*p) break;

                char* item_start = p;
                bool in_q = false;
                char qc = 0;

                while (*p) {
                    if (!in_q && (*p == '"' || *p == '\'')) { in_q = true; qc = *p; }
                    else if (in_q && *p == qc) { in_q = false; }
                    else if (!in_q && *p == ',') break;
                    p++;
                }

                if (*p == ',') {
                    *p = '\0';
                    p++;
                }

                char* item = trim_str(item_start);
                char c_arg[256];
                translate_expression(item, c_arg, sizeof(c_arg));
                strncpy(arg_c_tokens[arg_count++], c_arg, 255);
            }

            char disp_buf[1024];
            snprintf(disp_buf, sizeof(disp_buf), "    enlng_display(%d", arg_count);
            for (int a = 0; a < arg_count; a++) {
                strcat(disp_buf, ", ");
                strcat(disp_buf, arg_c_tokens[a]);
            }
            strcat(disp_buf, ");\n");
            strcat(target, disp_buf);
            continue;
        }

        // 3. Conditionals: if / when / else if / otherwise if / elif
        if (starts_with_ci(trimmed, "if ") || starts_with_ci(trimmed, "when ")) {
            const char* cond_str = starts_with_ci(trimmed, "if ") ? (trimmed + 3) : (trimmed + 5);
            char cond_copy[512];
            strncpy(cond_copy, cond_str, sizeof(cond_copy) - 1);
            char* colon = strrchr(cond_copy, ':');
            if (colon) *colon = '\0';

            char c_cond[512];
            translate_expression(cond_copy, c_cond, sizeof(c_cond));

            char buf[1024];
            snprintf(buf, sizeof(buf), "    if (enlng_is_truthy(%s)) {\n", c_cond);
            strcat(target, buf);

            ctx.indent_stack_ptr++;
            ctx.indent_stack[ctx.indent_stack_ptr] = indent + 4;
            ctx.in_loop_stack[ctx.indent_stack_ptr] = false;
            ctx.in_func_stack[ctx.indent_stack_ptr] = false;
            continue;
        }

        if (starts_with_ci(trimmed, "else if ") || starts_with_ci(trimmed, "otherwise if ") || starts_with_ci(trimmed, "elif ")) {
            const char* cond_str = strstr(trimmed, " ");
            cond_str = strstr(cond_str + 1, " ") + 1;
            char cond_copy[512];
            strncpy(cond_copy, cond_str, sizeof(cond_copy) - 1);
            char* colon = strrchr(cond_copy, ':');
            if (colon) *colon = '\0';

            char c_cond[512];
            translate_expression(cond_copy, c_cond, sizeof(c_cond));

            char buf[1024];
            snprintf(buf, sizeof(buf), "    } else if (enlng_is_truthy(%s)) {\n", c_cond);
            strcat(target, buf);
            continue;
        }

        if (strcmp(trimmed, "else:") == 0 || strcmp(trimmed, "otherwise:") == 0) {
            strcat(target, "    } else {\n");
            ctx.indent_stack_ptr++;
            ctx.indent_stack[ctx.indent_stack_ptr] = indent + 4;
            ctx.in_loop_stack[ctx.indent_stack_ptr] = false;
            ctx.in_func_stack[ctx.indent_stack_ptr] = false;
            continue;
        }

        // 4. Loops: while / repeat while (WITH AUTOMATIC ARENA SCOPE RECLAMATION)
        if (starts_with_ci(trimmed, "while ") || starts_with_ci(trimmed, "repeat while ")) {
            const char* cond_str = starts_with_ci(trimmed, "while ") ? (trimmed + 6) : (trimmed + 13);
            char cond_copy[512];
            strncpy(cond_copy, cond_str, sizeof(cond_copy) - 1);
            char* colon = strrchr(cond_copy, ':');
            if (colon) *colon = '\0';

            char c_cond[512];
            translate_expression(cond_copy, c_cond, sizeof(c_cond));

            char buf[1024];
            snprintf(buf, sizeof(buf), "    while (enlng_is_truthy(%s)) {\n        ENLNG_SCOPE_START();\n", c_cond);
            strcat(target, buf);

            ctx.indent_stack_ptr++;
            ctx.indent_stack[ctx.indent_stack_ptr] = indent + 4;
            ctx.in_loop_stack[ctx.indent_stack_ptr] = true;
            ctx.in_func_stack[ctx.indent_stack_ptr] = false;
            continue;
        }

        // 5. Functions: define function / function / def
        if (starts_with_ci(trimmed, "define function ") || starts_with_ci(trimmed, "function ") || starts_with_ci(trimmed, "def ")) {
            const char* fstart = strstr(trimmed, " ") + 1;
            if (starts_with_ci(fstart, "function ")) fstart += 9;

            char fname[64];
            char fargs[256];
            fargs[0] = '\0';

            const char* with_pos = strstr(fstart, " with ");
            if (with_pos) {
                int nlen = (int)(with_pos - fstart);
                strncpy(fname, fstart, nlen);
                fname[nlen] = '\0';
                strncpy(fargs, with_pos + 6, sizeof(fargs) - 1);
            } else {
                sscanf(fstart, "%63[^:( ]", fname);
            }
            trim_str(fname);
            char* col = strrchr(fargs, ':');
            if (col) *col = '\0';
            trim_str(fargs);

            char fbuf[1024];
            if (strlen(fargs) > 0) {
                snprintf(fbuf, sizeof(fbuf), "EnlngVal func_%s(EnlngVal %s) {\n    ENLNG_SCOPE_START();\n", fname, fargs);
                var_define(&ctx, fargs);
            } else {
                snprintf(fbuf, sizeof(fbuf), "EnlngVal func_%s(void) {\n    ENLNG_SCOPE_START();\n", fname);
            }
            strcat(funcs_body, fbuf);

            inside_function = true;
            ctx.indent_stack_ptr++;
            ctx.indent_stack[ctx.indent_stack_ptr] = indent + 4;
            ctx.in_loop_stack[ctx.indent_stack_ptr] = false;
            ctx.in_func_stack[ctx.indent_stack_ptr] = true;
            continue;
        }

        // 6. Return statement (with Escape Promotion)
        if (starts_with_ci(trimmed, "return ")) {
            const char* ret_expr = trimmed + 7;
            char c_ret[512];
            translate_expression(ret_expr, c_ret, sizeof(c_ret));
            char rbuf[1024];
            snprintf(rbuf, sizeof(rbuf), "    return enlng_promote(%s, __scope_mark__);\n", c_ret);
            strcat(target, rbuf);
            continue;
        }

        // Generic statement fallback
        char gen_c[512];
        translate_expression(trimmed, gen_c, sizeof(gen_c));
        char gbuf[1024];
        snprintf(gbuf, sizeof(gbuf), "    %s;\n", gen_c);
        strcat(target, gbuf);
    }

    // Close any remaining open indents
    while (ctx.indent_stack_ptr > 0) {
        char* target = inside_function ? funcs_body : main_body;
        if (ctx.in_loop_stack[ctx.indent_stack_ptr]) {
            strcat(target, "    ENLNG_SCOPE_END();\n    }\n");
        } else if (ctx.in_func_stack[ctx.indent_stack_ptr]) {
            strcat(target, "    ENLNG_SCOPE_END();\n    return enlng_null();\n}\n\n");
            inside_function = false;
        } else {
            strcat(target, "    }\n");
        }
        ctx.indent_stack_ptr--;
    }

    fclose(fin);

    // Write functions and main entry point to output C file
    fprintf(fout, "%s\n", funcs_body);
    fprintf(fout, "int main(int argc, char* argv[]) {\n");
    fprintf(fout, "    enlng_init_memory();\n");
    fprintf(fout, "    ENLNG_SCOPE_START();\n\n");
    fprintf(fout, "%s\n", main_body);
    fprintf(fout, "    ENLNG_SCOPE_END();\n");
    fprintf(fout, "    return 0;\n");
    fprintf(fout, "}\n");

    fclose(fout);
    return true;
}

bool enlng_compile_file_to_exe(const char* in_filepath, const char* out_exe_filepath) {
    char temp_c_file[512];
    snprintf(temp_c_file, sizeof(temp_c_file), "%s.c", in_filepath);

    if (!enlng_emit_c_from_file(in_filepath, temp_c_file)) {
        return false;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "gcc -O3 -I. \"%s\" -o \"%s\"", temp_c_file, out_exe_filepath);
    int res = system(cmd);

    // Cleanup temp C file unless debug requested
    remove(temp_c_file);
    return res == 0;
}
