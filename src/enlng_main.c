/*
 * =====================================================================
 *   ENLANG SOVEREIGN PROGRAMMING LANGUAGE COMPILER (Pure C99 Native)
 *   src/enlng_main.c - CLI Driver, Pipeline Runner & Native Compiler
 * =====================================================================
 */

#ifndef _WIN32
#include <sys/wait.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
  #include <windows.h>
  #include <process.h>
  #define ENLNG_PATH_SEP '\\'
  #define ENLNG_EXE_SUFFIX ".exe"
#else
  #include <unistd.h>
  #include <sys/types.h>
  #define ENLNG_PATH_SEP '/'
  #define ENLNG_EXE_SUFFIX ""
  #ifndef MAX_PATH
    #define MAX_PATH 4096
  #endif
#endif

#include "enlng_lexer.h"
#include "enlng_parser.h"
#include "enlng_codegen.h"
#include "enlng_runtime_embed.h"

static void get_temp_directory(char* buf, size_t buf_size) {
#ifdef _WIN32
    DWORD len = GetTempPathA((DWORD)buf_size, buf);
    if (len == 0 || len >= buf_size) {
        const char* env_t = getenv("TEMP");
        if (!env_t) env_t = getenv("TMP");
        if (!env_t) env_t = ".";
        snprintf(buf, buf_size, "%s", env_t);
    }
#else
    const char* env_t = getenv("TMPDIR");
    if (!env_t) env_t = "/tmp";
    snprintf(buf, buf_size, "%s", env_t);
#endif
    size_t td_len = strlen(buf);
    while (td_len > 0 && (buf[td_len - 1] == '\\' || buf[td_len - 1] == '/')) {
        buf[td_len - 1] = '\0';
        td_len--;
    }
}

static void print_banner(void) {
    printf("Enlang Sovereign Programming Language Compiler v%s\n", ENLNG_VERSION);
    printf("Bare-Metal Native C99 Execution Engine (Zero Python, Zero Node.js)\n\n");
}

static void print_help(void) {
    print_banner();
    printf("Usage:\n");
    printf("  enlng run <file.enlng>              Run immediately (built-in interpreter, no C compiler needed)\n");
    printf("  enlng run --native <file.enlng>     Compile with gcc, then run (faster for heavy programs)\n");
    printf("  enlng build <file.enlng> -o <out>   Compile to standalone native executable (.exe)\n");
    printf("  enlng add <folder|file|git url> [name]   Install a package into enlng_packages (then: use \"name\")\n");
    printf("  enlng install                       Install every package listed in enlng_packages.txt\n");
    printf("  enlng remove <name>                 Remove a package\n");
    printf("  enlng packages                      List the installed packages\n");
    printf("  enlng lsp                           Editor help over LSP: errors, hover, completion, go to definition\n");
    printf("  enlng debug <file.enlng>            Step through a program, with breakpoints and values\n");
    printf("  enlng emit-c <file.enlng>           Emit generated ANSI C99 source code to stdout\n");
    printf("  enlng <file.enlng>                  Direct run\n");
    printf("  enlng                               Interactive mode: write lines one by one\n");
    printf("  enlng test [file or folder]         Run every file ending in _test.enlng\n");
    printf("  enlng --version                     Display version information\n");
}

/* ---------------------------------------------------------------------
 * Packages: enlng add / install / remove / packages
 *
 * A package is a folder (with main.enlng) or one .enlng file, kept in the
 * folder enlng_packages next to the program. enlng_packages.txt lists
 * where each came from, one per line as `name source`, so that
 * `enlng install` can bring them all back.
 * ------------------------------------------------------------------ */
static char* read_file_string(const char* filepath);

#define PKG_DIR "enlng_packages"
#define PKG_LIST "enlng_packages.txt"

static int pkg_safe_name(const char* s) {
    if (!*s || strlen(s) > 80 || s[0] == '.') return 0;
    for (; *s; s++)
        if (!isalnum((unsigned char)*s) && *s != '_' && *s != '-' && *s != '.') return 0;
    return 1;
}

static int pkg_safe_source(const char* s) {
    return *s && strlen(s) < 900 && !strpbrk(s, "\"&|<>^%`$;\n\r") && s[0] != '-';
}

static void pkg_native_path(char* s) {
#ifdef _WIN32
    for (; *s; s++) if (*s == '/') *s = '\\';
#else
    (void)s;
#endif
}

static int pkg_is_dir(const char* path) {
    char cmdline[1100];
#ifdef _WIN32
    char p[1000];
    snprintf(p, sizeof(p), "%s", path);
    pkg_native_path(p);
    snprintf(cmdline, sizeof(cmdline), "if exist \"%s\\\" (exit 0) else (exit 1)", p);
#else
    snprintf(cmdline, sizeof(cmdline), "test -d \"%s\"", path);
#endif
    return system(cmdline) == 0;
}

static int pkg_exists(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

/* the name a source gets when none is given: the last part, without .git or .enlng */
static void pkg_default_name(const char* source, char* out, size_t sz) {
    const char* base = source;
    for (const char* p = source; *p; p++)
        if ((*p == '/' || *p == '\\') && p[1]) base = p + 1;
    snprintf(out, sz, "%s", base);
    size_t n = strlen(out);
    while (n > 0 && (out[n - 1] == '/' || out[n - 1] == '\\')) out[--n] = '\0';
    n = strlen(out);
    if (n > 5 && strcmp(out + n - 5, ".git") == 0) out[n - 5] = '\0';
    n = strlen(out);
    if (n > 6 && strcmp(out + n - 6, ".enlng") == 0) out[n - 6] = '\0';
}

static int pkg_remove_files(const char* name) {
    char path[300], cmdline[700];
    snprintf(path, sizeof(path), PKG_DIR "/%s", name);
    pkg_native_path(path);
#ifdef _WIN32
    snprintf(cmdline, sizeof(cmdline), "if exist \"%s\" rmdir /s /q \"%s\"", path, path);
    system(cmdline);
    snprintf(cmdline, sizeof(cmdline), "if exist \"%s.enlng\" del /q \"%s.enlng\"", path, path);
#else
    snprintf(cmdline, sizeof(cmdline), "rm -rf \"%s\" \"%s.enlng\"", path, path);
#endif
    system(cmdline);
    return 0;
}

/* Adds or replaces `name source` in the list. */
static void pkg_list_set(const char* name, const char* source) {
    char* old = read_file_string(PKG_LIST);
    FILE* f = fopen(PKG_LIST, "wb");
    if (!f) { free(old); return; }
    size_t nl = strlen(name);
    if (old) {
        char* line = strtok(old, "\n");
        while (line) {
            size_t len = strlen(line);
            if (len && line[len - 1] == '\r') line[len - 1] = '\0';
            if (!(strncmp(line, name, nl) == 0 && line[nl] == ' ') && *line) fprintf(f, "%s\n", line);
            line = strtok(NULL, "\n");
        }
        free(old);
    }
    if (source) fprintf(f, "%s %s\n", name, source);
    fclose(f);
}

static int pkg_install(const char* source, const char* name_in) {
    char name[100];
    if (!pkg_safe_source(source)) {
        fprintf(stderr, "[ENLANG ERROR] That source has characters that are not allowed: %s\n", source);
        return 1;
    }
    if (name_in) snprintf(name, sizeof(name), "%s", name_in);
    else pkg_default_name(source, name, sizeof(name));
    if (!pkg_safe_name(name)) {
        fprintf(stderr, "[ENLANG ERROR] '%s' is not a good package name. Use letters, digits, _ and -; "
                        "give one with: enlng add <source> <name>\n", name);
        return 1;
    }
    int is_git = strncmp(source, "http://", 7) == 0 || strncmp(source, "https://", 8) == 0 ||
                 strncmp(source, "git@", 4) == 0 || strncmp(source, "git://", 6) == 0 ||
                 strncmp(source, "file://", 7) == 0;
    pkg_remove_files(name);
    char dest[300], from[1000], cmdline[2400];
    snprintf(dest, sizeof(dest), PKG_DIR "/%s", name);
    pkg_native_path(dest);
#ifdef _WIN32
    system("if not exist " PKG_DIR " mkdir " PKG_DIR);
#else
    system("mkdir -p " PKG_DIR);
#endif
    int rc = 0;
    if (is_git) {
        snprintf(cmdline, sizeof(cmdline), "git clone --depth 1 -q \"%s\" \"%s\"", source, dest);
        rc = system(cmdline);
        if (rc == 0) {
            char git_dir[320];
            snprintf(git_dir, sizeof(git_dir), "%s/.git", dest);
            pkg_native_path(git_dir);
#ifdef _WIN32
            snprintf(cmdline, sizeof(cmdline), "if exist \"%s\" rmdir /s /q \"%s\"", git_dir, git_dir);
#else
            snprintf(cmdline, sizeof(cmdline), "rm -rf \"%s\"", git_dir);
#endif
            system(cmdline);
        }
    } else if (pkg_is_dir(source)) {
        snprintf(from, sizeof(from), "%s", source);
        pkg_native_path(from);
#ifdef _WIN32
        snprintf(cmdline, sizeof(cmdline), "xcopy \"%s\" \"%s\\\" /E /I /Y /Q >nul", from, dest);
#else
        snprintf(cmdline, sizeof(cmdline), "mkdir -p \"%s\" && cp -R \"%s/.\" \"%s/\"", dest, from, dest);
#endif
        rc = system(cmdline);
    } else if (pkg_exists(source)) {
        snprintf(from, sizeof(from), "%s", source);
        pkg_native_path(from);
#ifdef _WIN32
        snprintf(cmdline, sizeof(cmdline), "copy /Y \"%s\" \"%s.enlng\" >nul", from, dest);
#else
        snprintf(cmdline, sizeof(cmdline), "cp \"%s\" \"%s.enlng\"", from, dest);
#endif
        rc = system(cmdline);
    } else {
        fprintf(stderr, "[ENLANG ERROR] '%s' is not a folder, a file or a git address.\n", source);
        return 1;
    }
    if (rc != 0) {
        fprintf(stderr, "[ENLANG ERROR] Could not install '%s' from %s.\n", name, source);
        pkg_remove_files(name);
        return 1;
    }
    char main_file[400], single[400];
    snprintf(main_file, sizeof(main_file), PKG_DIR "/%s/main.enlng", name);
    snprintf(single, sizeof(single), PKG_DIR "/%s/%s.enlng", name, name);
    char flat[400];
    snprintf(flat, sizeof(flat), PKG_DIR "/%s.enlng", name);
    if (!pkg_exists(main_file) && !pkg_exists(single) && !pkg_exists(flat)) {
        fprintf(stderr, "[ENLANG ERROR] '%s' has no main.enlng, so 'use \"%s\"' could not find it. Nothing was installed.\n", name, name);
        pkg_remove_files(name);
        return 1;
    }
    pkg_list_set(name, source);
    printf("Installed '%s'. Use it with:  use \"%s\"\n", name, name);
    return 0;
}

static int pkg_install_all(void) {
    char* list = read_file_string(PKG_LIST);
    if (!list) {
        printf("No " PKG_LIST " here, so there is nothing to install.\n");
        return 0;
    }
    int bad = 0, count = 0;
    char* line = strtok(list, "\n");
    char* lines[256];
    int n = 0;
    while (line && n < 256) { lines[n++] = enlng_strdup(line); line = strtok(NULL, "\n"); }
    free(list);
    for (int i = 0; i < n; i++) {
        size_t len = strlen(lines[i]);
        if (len && lines[i][len - 1] == '\r') lines[i][len - 1] = '\0';
        char* sp = strchr(lines[i], ' ');
        if (!sp) continue;
        *sp = '\0';
        count++;
        if (pkg_install(sp + 1, lines[i]) != 0) bad++;
    }
    printf("%d package%s installed, %d failed.\n", count - bad, count - bad == 1 ? "" : "s", bad);
    return bad ? 1 : 0;
}

static int pkg_list(void) {
    char* list = read_file_string(PKG_LIST);
    if (!list || !*list) {
        printf("No packages. Add one with: enlng add <folder, file or git address>\n");
        free(list);
        return 0;
    }
    char* line = strtok(list, "\n");
    while (line) {
        size_t len = strlen(line);
        if (len && line[len - 1] == '\r') line[len - 1] = '\0';
        char* sp = strchr(line, ' ');
        if (sp) { *sp = '\0'; printf("  %-20s from %s\n", line, sp + 1); }
        line = strtok(NULL, "\n");
    }
    free(list);
    return 0;
}

static int pkg_remove(const char* name) {
    if (!pkg_safe_name(name)) {
        fprintf(stderr, "[ENLANG ERROR] '%s' is not a package name.\n", name);
        return 1;
    }
    pkg_remove_files(name);
    pkg_list_set(name, NULL);
    printf("Removed '%s'.\n", name);
    return 0;
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
    int kind_error = strstr(raw_msg, "' checks that its input '") != NULL;
    fprintf(stderr, kind_error ? "\n-- Enlang KindError -----------------------------------------------------\n"
                               : "\n-- Enlang SyntaxError ---------------------------------------------------\n");
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
    } else if (strncmp(raw_msg, "Unknown word '", 14) == 0) {
        char word[64] = {0};
        const char* q = raw_msg + 14;
        size_t wl = 0;
        while (q[wl] && q[wl] != '\'' && wl < sizeof(word) - 1) { word[wl] = q[wl]; wl++; }
        word[wl] = '\0';
        static const char* print_like[] = { "say", "write", "output", "echo", "puts", "println", "log", "console", "put", "tell", "speak", "out" };
        int is_print_like = 0;
        for (size_t i = 0; i < sizeof(print_like) / sizeof(print_like[0]); i++) {
            if (strcmp(word, print_like[i]) == 0) is_print_like = 1;
        }
        fprintf(stderr, "Why:  '%s' is not a power word in Enlang, and it is not a name you defined earlier.\n\n", word);
        fprintf(stderr, "Suggestions:\n");
        if (is_print_like) {
            fprintf(stderr, "  * To print, use 'show', 'display' or 'print', e.g.: show \"hello\"\n");
        } else {
            fprintf(stderr, "  * Check the spelling of '%s'.\n", word);
            fprintf(stderr, "  * Common power words: show, remember, change, when, for, while, repeat, action, return.\n");
            fprintf(stderr, "  * If '%s' is your own name, define it first, e.g.: remember %s as 0\n", word, word);
        }
    } else if (strstr(raw_msg, "declared with freeze") != NULL) {
        fprintf(stderr, "Why:  A name made with 'freeze' is a constant: the name may not be assigned again, and a list or map it holds may not be changed.\n\n");
        fprintf(stderr, "Suggestions:\n");
        fprintf(stderr, "  * Declare it with 'remember' if the value has to change.\n");
        fprintf(stderr, "  * Use a different name for the new value.\n");
    } else if (kind_error) {
        fprintf(stderr, "Why:  The kind of this value is clear before the program runs, and it is not the kind the function checks for. Nothing was run.\n\n");
        fprintf(stderr, "Suggestions:\n");
        fprintf(stderr, "  * Give the function a value of the kind it checks for.\n");
        fprintf(stderr, "  * Text that holds a number can be changed with to_number(...).\n");
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

/* enlng test [file or folder]: runs every file ending in _test.enlng */
static int test_is_file(const char *name) {
  size_t n = strlen(name), k = strlen("_test.enlng");
  return n >= k && strcmp(name + n - k, "_test.enlng") == 0;
}

static void test_collect(const char *dir, char ***files, int *nfiles, int *cap) {
  EnlngVal names = enlng_builtin_list_folder(enlng_make_string(dir));
  if (names.type != ENLNG_VAL_LIST || !names.as.l)
    return;
  for (int i = 0; i < names.as.l->count; i++) {
    const char *nm = names.as.l->items[i].as.s;
    if (!nm || nm[0] == '.')
      continue;
    EnlngVal full = enlng_builtin_path_join(enlng_make_string(dir), enlng_make_string(nm));
    if (enlng_is_truthy(enlng_builtin_is_folder(full))) {
      test_collect(full.as.s, files, nfiles, cap);
    } else if (test_is_file(nm)) {
      if (*nfiles >= *cap) {
        *cap = *cap ? *cap * 2 : 16;
        *files = (char **)realloc(*files, sizeof(char *) * (size_t)*cap);
      }
      (*files)[(*nfiles)++] = enlng_strdup(full.as.s);
    }
  }
}

static int run_tests(const char *self, const char *target) {
  char **files = NULL;
  int nfiles = 0, cap = 0;
  if (enlng_is_truthy(enlng_builtin_is_folder(enlng_make_string(target)))) {
    test_collect(target, &files, &nfiles, &cap);
  } else {
    files = (char **)malloc(sizeof(char *));
    files[nfiles++] = enlng_strdup(target);
  }
  if (nfiles == 0) {
    printf("No test files found. Test files end in _test.enlng.\n");
    return 1;
  }
  int bad = 0;
  for (int i = 0; i < nfiles; i++) {
    printf("== %s\n", files[i]);
    fflush(stdout);
    char cmd[4096];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "\"\"%s\" run \"%s\"\"", self, files[i]);
#else
    snprintf(cmd, sizeof(cmd), "\"%s\" run \"%s\"", self, files[i]);
#endif
    if (system(cmd) != 0)
      bad++;
    printf("\n");
  }
  printf("%d test file%s, %d with failures\n", nfiles, nfiles == 1 ? "" : "s", bad);
  return bad ? 1 : 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return enlng_repl();
    }

    if (strcmp(argv[1], "help") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "repl") == 0) {
        return enlng_repl();
    }

    if (strcmp(argv[1], "lsp") == 0) return enlng_lsp();

    if (strcmp(argv[1], "add") == 0) {
        if (argc < 3) { fprintf(stderr, "[ENLANG ERROR] Say what to add: enlng add <folder, file or git address> [name]\n"); return 1; }
        return pkg_install(argv[2], argc >= 4 ? argv[3] : NULL);
    }
    if (strcmp(argv[1], "install") == 0) return pkg_install_all();
    if (strcmp(argv[1], "packages") == 0) return pkg_list();
    if (strcmp(argv[1], "remove") == 0) {
        if (argc < 3) { fprintf(stderr, "[ENLANG ERROR] Say which package: enlng remove <name>\n"); return 1; }
        return pkg_remove(argv[2]);
    }

    if (strcmp(argv[1], "test") == 0) {
        return run_tests(argv[0], argc >= 3 ? argv[2] : ".");
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

    int native = 0;
    int breaks[64], nbreaks = 0;
    /* inputs for the program itself: enlng run app.enlng first second */
    char** prog_args = (char**)malloc(sizeof(char*) * (argc + 1));
    int prog_argc = 0;
    if (strcmp(argv[1], "run") == 0) {
        for (int ai = 2; ai < argc; ai++) {
            if (!input_file && (strcmp(argv[ai], "--native") == 0 || strcmp(argv[ai], "--compile") == 0)) native = 1;
            else if (input_file && prog_argc == 0 && (strcmp(argv[ai], "--native") == 0 || strcmp(argv[ai], "--compile") == 0)) native = 1;
            else if (!input_file) input_file = argv[ai];
            else prog_args[prog_argc++] = argv[ai];
        }
        if (!input_file) { fprintf(stderr, "[ENLANG ERROR] Missing file for 'run'.\n"); return 1; }
    } else if (strcmp(argv[1], "debug") == 0) {
        for (int ai = 2; ai < argc; ai++) {
            if (strcmp(argv[ai], "--break") == 0 && ai + 1 < argc && nbreaks < 64) breaks[nbreaks++] = atoi(argv[++ai]);
            else if (!input_file) input_file = argv[ai];
            else prog_args[prog_argc++] = argv[ai];
        }
        if (!input_file) { fprintf(stderr, "[ENLANG ERROR] Missing file for 'debug'.\n"); return 1; }
        mode = "debug";
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
            output_exe = "output" ENLNG_EXE_SUFFIX;
        }
    } else {
        input_file = argv[1];
        for (int ai = 2; ai < argc; ai++)
            prog_args[prog_argc++] = argv[ai];
    }
    enlng_set_args(prog_argc, prog_args);

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
    parser_set_source_file(input_file);
    ASTNode* prog = parser_parse_program(&parser);

    if (parser.has_error || !prog) {
        print_human_error_c(input_file, source, parser.error_line, parser.error_msg);
        lexer_free(&lexer);
        free(source);
        return 1;
    }

    /* 3a. Default `run`: interpret directly, no C compiler involved */
    if (strcmp(mode, "debug") == 0) {
        printf("Debugging %s. Type help to see the commands.\n", input_file);
        enlng_debug_setup(breaks, nbreaks);
        int rc = enlng_interp_run(prog, input_file, source);
        lexer_free(&lexer);
        free(source);
        return rc;
    }
    if (strcmp(mode, "run") == 0 && !native) {
        int rc = enlng_interp_run(prog, input_file, source);
        lexer_free(&lexer);
        free(source);
        return rc;
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
    get_temp_directory(temp_dir, sizeof(temp_dir));

#ifdef _WIN32
    unsigned long pid = (unsigned long)GetCurrentProcessId();
#else
    unsigned long pid = (unsigned long)getpid();
#endif

    char temp_c[MAX_PATH];
    char temp_exe[MAX_PATH];
    char temp_runtime[MAX_PATH];

    snprintf(temp_c, sizeof(temp_c), "%s%cenlng_build_%lu.c", temp_dir, ENLNG_PATH_SEP, pid);
    snprintf(temp_exe, sizeof(temp_exe), "%s%cenlng_build_%lu%s", temp_dir, ENLNG_PATH_SEP, pid, ENLNG_EXE_SUFFIX);
    snprintf(temp_runtime, sizeof(temp_runtime), "%s%cenlng_runtime.h", temp_dir, ENLNG_PATH_SEP);

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

    char extra_inc[1024] = "";
    char home_dir[ENLANGG_MAX_PATH];
    if (enlangg_get_home_dir(home_dir, sizeof(home_dir))) {
        snprintf(extra_inc, sizeof(extra_inc),
                 " -I\"%s%cinclude\" -I\"%s%csrc\" -I\"%s\"",
                 home_dir, ENLNG_PATH_SEP,
                 home_dir, ENLNG_PATH_SEP,
                 home_dir);
    }

    snprintf(cmd, sizeof(cmd), "gcc -O2 -std=c99 -D_DEFAULT_SOURCE -D_GNU_SOURCE -I\"%s\" -I\"src\" -I\".\"%s \"%s\" -o \"%s\" -lm%s",
             temp_dir, extra_inc, temp_c, final_exe_target,
#ifdef _WIN32
             "");
#else
             " -pthread");
#endif
    int compile_ret = system(cmd);

    if (compile_ret != 0) {
        fprintf(stderr, "[ENLANG ERROR] Native C compilation failed. Ensure a C compiler (gcc) is installed and on PATH.\n");
        remove(temp_c);
        remove(temp_runtime);
        return compile_ret;
    }

    int exec_ret = 0;
    if (strcmp(mode, "run") == 0) {
        /* Run native executable */
        char run_cmd[MAX_PATH * 2 + 16];
#ifdef _WIN32
        snprintf(run_cmd, sizeof(run_cmd), "\"%s\"", temp_exe);
#else
        if (temp_exe[0] != '/' && temp_exe[0] != '.') {
            snprintf(run_cmd, sizeof(run_cmd), "\"./%s\"", temp_exe);
        } else {
            snprintf(run_cmd, sizeof(run_cmd), "\"%s\"", temp_exe);
        }
#endif
        /* hand the program its inputs, each quoted */
        size_t full_len = strlen(run_cmd) + 16;
        for (int ai = 0; ai < prog_argc; ai++)
            full_len += strlen(prog_args[ai]) * 2 + 4;
        char* full_cmd = (char*)malloc(full_len);
        strcpy(full_cmd, run_cmd);
        for (int ai = 0; ai < prog_argc; ai++) {
            char* w = full_cmd + strlen(full_cmd);
            *w++ = ' ';
#ifdef _WIN32
            *w++ = '"';
            for (const char* q = prog_args[ai]; *q; q++) {
                if (*q == '"') *w++ = '\\';
                *w++ = *q;
            }
            *w++ = '"';
#else
            *w++ = '\'';
            for (const char* q = prog_args[ai]; *q; q++) {
                if (*q == '\'') { strcpy(w, "'\\''"); w += 4; }
                else *w++ = *q;
            }
            *w++ = '\'';
#endif
            *w = '\0';
        }
#ifdef _WIN32
        /* cmd.exe drops the outer quotes of a line that starts with one */
        char* wrapped = (char*)malloc(strlen(full_cmd) + 3);
        sprintf(wrapped, "\"%s\"", full_cmd);
        exec_ret = system(prog_argc ? wrapped : run_cmd);
        free(wrapped);
#else
        exec_ret = system(full_cmd);
        /* system() gives the wait status: the program's exit code is in it (as is, 256 for exit 1, which a shell
           sees as 0, so a program that stopped on an error looked as if it had worked) */
        if (exec_ret != -1 && WIFEXITED(exec_ret)) exec_ret = WEXITSTATUS(exec_ret);
        else if (exec_ret != -1 && WIFSIGNALED(exec_ret)) exec_ret = 128 + WTERMSIG(exec_ret);
        else if (exec_ret == -1) exec_ret = 1;
#endif
        free(full_cmd);
        remove(temp_exe);
    } else if (strcmp(mode, "build") == 0) {
        printf("[ENLANG SUCCESS] Standalone native executable built: %s\n", final_exe_target);
    }

    remove(temp_c);
    remove(temp_runtime);
    codegen_free(&codegen);
    lexer_free(&lexer);
    free(source);

    return exec_ret;
}
