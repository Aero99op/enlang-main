/*
 * =====================================================================
 *   ENLANG SOVEREIGN PROGRAMMING LANGUAGE COMPILER (Pure C99 Native)
 *   src/enlng_main.c - CLI Driver, Pipeline Runner & Native Compiler
 * =====================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "enlng_lexer.h"
#include "enlng_parser.h"
#include "enlng_codegen.h"
#include "enlng_runtime_embed.h"

static void print_banner(void) {
    printf("Enlang Sovereign Programming Language Compiler v%s\n", ENLNG_VERSION);
    printf("Bare-Metal Native C99 Execution Engine (Zero Python, Zero Node.js)\n\n");
}

static void print_help(void) {
    print_banner();
    printf("Usage:\n");
    printf("  enlng run <file.enlng>              Compile and execute immediately\n");
    printf("  enlng build <file.enlng> -o <out>   Compile to standalone native executable (.exe)\n");
    printf("  enlng emit-c <file.enlng>           Emit generated ANSI C99 source code to stdout\n");
    printf("  enlng <file.enlng>                  Direct run\n");
    printf("  enlng --version                     Display version information\n");
}

static char* read_file_string(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buf = (char*)malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

static void print_human_error_c(const char* filepath, const char* source, int target_line, const char* raw_msg) {
    fprintf(stderr, "\n-- Enlang SyntaxError ---------------------------------------------------\n");
    fprintf(stderr, "Location: %s:%d\n\n", filepath, target_line > 0 ? target_line : 1);

    if (source && target_line > 0) {
        const char* p = source;
        const char* line_starts[2048];
        int line_lens[2048];
        int total_lines = 0;

        line_starts[0] = p;
        while (*p && total_lines < 2040) {
            if (*p == '\n') {
                line_lens[total_lines] = (int)(p - line_starts[total_lines]);
                total_lines++;
                line_starts[total_lines] = p + 1;
            }
            p++;
        }
        if (p > line_starts[total_lines]) {
            line_lens[total_lines] = (int)(p - line_starts[total_lines]);
            total_lines++;
        }

        int target_idx = target_line - 1;
        int start_idx = target_idx > 1 ? target_idx - 1 : 0;
        int end_idx = target_idx + 1 < total_lines ? target_idx + 1 : total_lines - 1;

        for (int i = start_idx; i <= end_idx; i++) {
            char buf[512] = {0};
            int len = line_lens[i] < 511 ? line_lens[i] : 511;
            strncpy(buf, line_starts[i], len);
            if (len > 0 && buf[len - 1] == '\r') buf[len - 1] = '\0';

            if (i == target_idx) {
                fprintf(stderr, "  %3d | %s\n", i + 1, buf);
                int caret_col = 1;
                char* eq = strstr(buf, " = ");
                if (eq) {
                    caret_col = (int)(eq - buf) + 2;
                } else if (strlen(buf) > 0) {
                    char* trim_p = buf;
                    while (*trim_p && isspace((unsigned char)*trim_p)) trim_p++;
                    caret_col = (int)(trim_p - buf) + 1;
                }
                fprintf(stderr, "      | ");
                for (int c = 1; c < caret_col; c++) fputc(' ', stderr);
                fprintf(stderr, "^\n");
            } else {
                fprintf(stderr, "  %3d | %s\n", i + 1, buf);
            }
        }
        fprintf(stderr, "\n");
    }

    fprintf(stderr, "What: %s\n", raw_msg);
    if (strstr(raw_msg, "Expected ':'") != NULL) {
        fprintf(stderr, "Why:  Block headers in Enlang (when, if, while, for, function) must terminate with ':'.\n\n");
        fprintf(stderr, "Suggestions:\n");
        fprintf(stderr, "  * Did you mean to end the condition with a colon, e.g. 'when score == 90:'?\n");
        fprintf(stderr, "  * Ensure comparison operators use '==' or 'is' instead of '=' inside conditions.\n");
    } else if (strstr(raw_msg, "Expected 'as'") != NULL) {
        fprintf(stderr, "Why:  Variable bindings require 'as' or '=' to specify the initial value.\n\n");
        fprintf(stderr, "Suggestions:\n");
        fprintf(stderr, "  * Bind initial value with 'as', e.g. 'remember count as 0'.\n");
    } else if (strstr(raw_msg, "type enlng") != NULL) {
        fprintf(stderr, "Why:  Sovereign native Enlang compilation units must declare domain type on line 1.\n\n");
        fprintf(stderr, "Suggestions:\n");
        fprintf(stderr, "  * Add 'type enlng' at the very top of your file.\n");
    } else {
        fprintf(stderr, "Why:  The compiler encountered an unexpected token during syntax evaluation.\n\n");
        fprintf(stderr, "Suggestions:\n");
        fprintf(stderr, "  * Check statement syntax against Enlang language conventions.\n");
    }
    fprintf(stderr, "------------------------------------------------------------------------\n\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("enlng version %s (Native C99)\n", ENLNG_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        return 0;
    }

    const char* mode = "run";
    const char* input_file = NULL;
    const char* output_exe = NULL;

    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) { fprintf(stderr, "[ENLANG ERROR] Missing file for 'run'.\n"); return 1; }
        input_file = argv[2];
    } else if (strcmp(argv[1], "emit-c") == 0) {
        if (argc < 3) { fprintf(stderr, "[ENLANG ERROR] Missing file for 'emit-c'.\n"); return 1; }
        mode = "emit-c";
        input_file = argv[2];
    } else if (strcmp(argv[1], "build") == 0) {
        if (argc < 3) { fprintf(stderr, "[ENLANG ERROR] Missing file for 'build'.\n"); return 1; }
        mode = "build";
        input_file = argv[2];
        if (argc >= 5 && strcmp(argv[3], "-o") == 0) {
            output_exe = argv[4];
        } else {
            output_exe = "output.exe";
        }
    } else {
        input_file = argv[1];
    }

    char* source = read_file_string(input_file);
    if (!source) {
        fprintf(stderr, "[ENLANG ERROR] Could not open file: %s\n", input_file);
        return 1;
    }

    /* 1. Lexical Analysis */
    Lexer lexer;
    lexer_init(&lexer, source);
    lexer_tokenize(&lexer);

    /* 2. AST Parsing */
    Parser parser;
    parser_init(&parser, lexer.tokens, lexer.token_count);
    ASTNode* prog = parser_parse_program(&parser);

    if (parser.has_error || !prog) {
        print_human_error_c(input_file, source, parser.error_line, parser.error_msg);
        lexer_free(&lexer);
        free(source);
        return 1;
    }

    /* 3. Code Generation (Pure C99) */
    CodeGen codegen;
    codegen_init(&codegen);
    char* c_code = codegen_generate(&codegen, prog);

    if (strcmp(mode, "emit-c") == 0) {
        printf("%s\n", c_code);
        codegen_free(&codegen);
        lexer_free(&lexer);
        free(source);
        return 0;
    }

    /* 4. Native Compilation via GCC */
    char temp_dir[MAX_PATH];
    GetTempPathA(MAX_PATH, temp_dir);
    size_t td_len = strlen(temp_dir);
    if (td_len > 0 && (temp_dir[td_len - 1] == '\\' || temp_dir[td_len - 1] == '/')) {
        temp_dir[td_len - 1] = '\0';
    }
    DWORD pid = GetCurrentProcessId();

    char temp_c[MAX_PATH];
    char temp_exe[MAX_PATH];
    char temp_runtime[MAX_PATH];

    snprintf(temp_c, sizeof(temp_c), "%s\\enlng_build_%lu.c", temp_dir, pid);
    snprintf(temp_exe, sizeof(temp_exe), "%s\\enlng_build_%lu.exe", temp_dir, pid);
    snprintf(temp_runtime, sizeof(temp_runtime), "%s\\enlng_runtime.h", temp_dir);

    /* Write embedded runtime header */
    FILE* frh = fopen(temp_runtime, "w");
    if (frh) {
        fputs(ENLNG_EMBEDDED_RUNTIME, frh);
        fclose(frh);
    }

    /* Write generated C file */
    FILE* fc = fopen(temp_c, "w");
    if (!fc) {
        fprintf(stderr, "[ENLANG ERROR] Could not write temporary C source.\n");
        return 1;
    }
    fputs(c_code, fc);
    fclose(fc);

    char cmd[MAX_PATH * 4];
    const char* final_exe_target = (strcmp(mode, "build") == 0 && output_exe) ? output_exe : temp_exe;

    snprintf(cmd, sizeof(cmd), "gcc -O2 -std=c99 -I\"%s\" -I\"src\" -I\"d:\\enlangg\\src\" \"%s\" -o \"%s\"", temp_dir, temp_c, final_exe_target);
    int compile_ret = system(cmd);

    if (compile_ret != 0) {
        fprintf(stderr, "[ENLANG ERROR] Native C compilation failed.\n");
        remove(temp_c);
        return compile_ret;
    }

    int exec_ret = 0;
    if (strcmp(mode, "run") == 0) {
        /* Run native executable */
        char run_cmd[MAX_PATH * 2];
        snprintf(run_cmd, sizeof(run_cmd), "\"%s\"", temp_exe);
        exec_ret = system(run_cmd);
        remove(temp_exe);
    } else if (strcmp(mode, "build") == 0) {
        printf("[ENLANG SUCCESS] Standalone native executable built: %s\n", final_exe_target);
    }

    remove(temp_c);
    codegen_free(&codegen);
    lexer_free(&lexer);
    free(source);

    return exec_ret;
}
