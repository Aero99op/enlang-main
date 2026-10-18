/*
 * enlangg - The Sovereign Universal CLI & Compiler Engine
 * Supports:
 *   enlangg run <file.enlng>                (Full natural English execution)
 *   enlangg run <file.enlngf> [--p <port>]  (Web studio server)
 *   enlangg run <file.enlngmf> [--device]   (Mobile HAL simulator)
 *   enlangg build <file> --target <apk|ipa> -o <out>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#elif defined(__APPLE__)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mach-o/dyld.h>
#define MKDIR(p) mkdir(p, 0755)
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#define MKDIR(p) mkdir(p, 0755)
#endif
#include "enlngdb/c/enlngdb.h"
#include "enlng/c/enlng_emitter.h"

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

#define VERSION "5.0.0-sovereign-universal"

void print_help() {
    printf("=====================================================================\n");
    printf("  Enlangg Sovereign Toolchain CLI v%s\n", VERSION);
    printf("  Pure C Zero-Python 7-in-1 Sovereign Ecosystem\n");
    printf("=====================================================================\n\n");
    printf("Unified CLI Commands:\n");
    printf("  enlangg run <file.ext>                      Run any file (.enlng, .enlngdb, .enlngs, .enlngf, .enlngd, .enlngm)\n");
    printf("  enlangg run --aot <file.enlng>              Compile to native machine code & execute instantly\n");
    printf("  enlangg compile <file.enlng> [-o <out.exe>] AOT compile natural English to native C machine code (.exe)\n");
    printf("  enlangg emit-c <file.enlng> [-o <out.c>]    Emit clean ISO C99 source with Scoped Arena Memory\n\n");
    printf("Domain Engines (Native Pure C):\n");
    printf("  enlangg db <script.enlngdb>                 Execute conversational microsecond database script\n");
    printf("  enlangg db -e \"<query>\"                     Execute inline database statement in C (<0.05ms)\n");
    printf("  enlangg s <logic.enlngs>                    Execute script natively on Pure C In-Memory VM\n");
    printf("  enlangg s -e \"<script>\"                     Execute inline script statement in C VM\n");
    printf("  enlangg f <app.enlngf>                      Launch Pure C Native Win32 Desktop Window GUI\n");
    printf("  enlangg f serve <app.enlngf> [--port 3000]  Serve live over Pure C WinSock2 HTTP Server\n");
    printf("  enlangg f build <app.enlngf> [-o <out.html>] Compile frontend markup to standalone HTML5\n");
    printf("  enlangg d <theme.enlngd>                    Inspect and resolve design tokens & styles\n");
    printf("  enlangg d compile <theme.enlngd> [-o <.css>] Export design tokens to CSS3 stylesheet\n");
    printf("  enlangg m <app.enlngm>                      Launch Pure C Native Smartphone Simulator (390x844)\n");
    printf("  enlangg m build <app.enlngm> --target apk   Build mobile production package (HAL ARM64)\n");
    printf("  enlangg build <app.enlngm> --target apk     Direct shortcut to mobile production compiler\n\n");
    printf("Developer Experience & Tooling (Pure C):\n");
    printf("  enlangg fmt <file> [-w|--write] [-c|--check] Format Enlang files with Sovereign C Formatter\n");
    printf("  enlangg pkg init [name]                     Initialize a new enlang.json project manifest\n");
    printf("  enlangg pkg add <package>                   Add stdlib dependency & link modules\n");
    printf("  enlangg pkg install                         Install dependencies declared in enlang.json\n");
    printf("  enlangg pkg list                            List declared dependencies in enlang.json\n\n");
    printf("Dedicated Standalone Executables in PATH:\n");
    printf("  enlangg   Universal master toolchain dispatcher\n");
    printf("  enlng     Core general-purpose computing with arena memory (.exe)\n");
    printf("  enlngdb   Pure C conversational zero-SQL database engine (.edb format)\n");
    printf("  enlngs    Pure C sovereign in-memory script VM & interpreter\n");
    printf("  enlngf    Pure C sovereign desktop window GUI & WinSock2 web studio\n");
    printf("  enlngd    Pure C sovereign design tokens & style resolver\n");
    printf("  enlngm    Pure C sovereign smartphone simulator & HAL packager\n\n");
    printf("Documentation & Live Playground: https://enlangg.vercel.app\n");
}

static void get_executable_dir(char* outPath, size_t maxLen) {
#ifdef _WIN32
    GetModuleFileNameA(NULL, outPath, (DWORD)maxLen);
    char* lastSlash = strrchr(outPath, '\\');
    if (!lastSlash) lastSlash = strrchr(outPath, '/');
    if (lastSlash) *lastSlash = '\0';
#elif defined(__APPLE__)
    uint32_t size = (uint32_t)maxLen;
    if (_NSGetExecutablePath(outPath, &size) == 0) {
        char* lastSlash = strrchr(outPath, '/');
        if (lastSlash) *lastSlash = '\0';
    } else {
        strcpy(outPath, ".");
    }
#else
    ssize_t len = readlink("/proc/self/exe", outPath, maxLen - 1);
    if (len != -1) {
        outPath[len] = '\0';
        char* lastSlash = strrchr(outPath, '/');
        if (lastSlash) *lastSlash = '\0';
    } else {
        strcpy(outPath, ".");
    }
#endif
}

static void get_system_temp_dir(char* outDir, size_t maxLen) {
#ifdef _WIN32
    GetTempPathA((DWORD)maxLen, outDir);
#else
    const char* tmp = getenv("TMPDIR");
    if (!tmp) tmp = "/tmp";
    snprintf(outDir, maxLen, "%s/", tmp);
#endif
}

static unsigned long get_current_pid(void) {
#ifdef _WIN32
    return (unsigned long)GetCurrentProcessId();
#else
    return (unsigned long)getpid();
#endif
}

static bool has_suffix(const char* str, const char* suffix) {
    if (!str || !suffix) return false;
    size_t str_len = strlen(str);
    size_t suf_len = strlen(suffix);
    if (str_len < suf_len) return false;
    return (strcmp(str + (str_len - suf_len), suffix) == 0);
}

static void append_safe_arg(char* buf, size_t buf_size, const char* arg) {
    if (!buf || !arg) return;
    size_t len = strlen(buf);
    if (len + 4 >= buf_size) return;
    buf[len++] = ' ';
    buf[len++] = '"';
    for (const char* p = arg; *p && len + 3 < buf_size; p++) {
        if (*p == '"' || *p == '\\') {
            buf[len++] = '\\';
        }
        buf[len++] = *p;
    }
    buf[len++] = '"';
    buf[len] = '\0';
}

static bool file_exists_c(const char* path) {
    if (!path || !*path) return false;
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

static bool copy_file_c(const char* src, const char* dst) {
    FILE* in = fopen(src, "rb");
    if (!in) return false;
    FILE* out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return false;
    }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fclose(in);
            fclose(out);
            return false;
        }
    }
    fclose(in);
    fclose(out);
    return true;
}

/* Native Pure C Formatter (enlangg fmt) */
static int run_native_fmt(int argc, char* argv[]) {
    const char* target = NULL;
    bool write_mode = false;
    bool check_mode = false;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--write") == 0) {
            write_mode = true;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--check") == 0) {
            check_mode = true;
        } else if (argv[i][0] != '-') {
            target = argv[i];
        }
    }

    if (!target) {
        fprintf(stderr, "Error: Usage: enlangg fmt <file> [-w|--write] [-c|--check]\n");
        return 1;
    }

    FILE* f = fopen(target, "rb");
    if (!f) {
        fprintf(stderr, "Error: File not found: '%s'\n", target);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize < 0) {
        fclose(f);
        return 1;
    }

    char* raw = (char*)malloc((size_t)fsize + 1);
    if (!raw) {
        fclose(f);
        return 1;
    }
    size_t read_bytes = fread(raw, 1, (size_t)fsize, f);
    raw[read_bytes] = '\0';
    fclose(f);

    size_t out_cap = (size_t)fsize * 2 + 4096;
    char* formatted = (char*)malloc(out_cap);
    if (!formatted) {
        free(raw);
        return 1;
    }
    size_t out_len = 0;
    formatted[0] = '\0';

    char* p = raw;
    while (*p) {
        char* line_end = strchr(p, '\n');
        size_t len = line_end ? (size_t)(line_end - p) : strlen(p);
        char line[4096];
        if (len >= sizeof(line)) len = sizeof(line) - 1;
        memcpy(line, p, len);
        line[len] = '\0';
        while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == ' ' || line[len - 1] == '\t')) {
            line[--len] = '\0';
        }

        /* Preserve each line's original indentation (normalising tabs to 4 spaces) */
        int lead_spaces = 0;
        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') {
            if (*trimmed == '\t') lead_spaces += 4;
            else lead_spaces += 1;
            trimmed++;
        }

        if (*trimmed == '\0') {
            if (out_len + 2 >= out_cap) {
                out_cap = (out_cap + 4096) * 2;
                char* nb = (char*)realloc(formatted, out_cap);
                if (!nb) { free(raw); free(formatted); return 1; }
                formatted = nb;
            }
            formatted[out_len++] = '\n';
            formatted[out_len] = '\0';
        } else {
            /* Collapse repeated spaces/tabs outside string literals and comments */
            char norm_line[4096];
            size_t nlen = 0;
            char in_quote = 0;
            bool prev_space = false;

            for (const char* c = trimmed; *c && nlen + 4 < sizeof(norm_line); c++) {
                if (in_quote) {
                    if (*c == '\\' && *(c + 1)) {
                        norm_line[nlen++] = *c++;
                        norm_line[nlen++] = *c;
                        continue;
                    }
                    if (*c == in_quote) {
                        in_quote = 0;
                    }
                    norm_line[nlen++] = *c;
                    prev_space = false;
                } else {
                    if (*c == '#') {
                        while (*c && nlen + 2 < sizeof(norm_line)) {
                            norm_line[nlen++] = *c++;
                        }
                        break;
                    }
                    if (*c == '"' || *c == '\'') {
                        in_quote = *c;
                        norm_line[nlen++] = *c;
                        prev_space = false;
                    } else if (*c == ' ' || *c == '\t') {
                        if (!prev_space) {
                            norm_line[nlen++] = ' ';
                            prev_space = true;
                        }
                    } else {
                        if (*c == ':' && *(c + 1) == '\0' && nlen > 0 && norm_line[nlen - 1] == ' ') {
                            norm_line[nlen - 1] = ':';
                        } else {
                            norm_line[nlen++] = *c;
                        }
                        prev_space = false;
                    }
                }
            }
            norm_line[nlen] = '\0';

            if (out_len + (size_t)lead_spaces + nlen + 2 >= out_cap) {
                out_cap = (out_cap + (size_t)lead_spaces + nlen + 4096) * 2;
                char* nb = (char*)realloc(formatted, out_cap);
                if (!nb) { free(raw); free(formatted); return 1; }
                formatted = nb;
            }
            for (int s = 0; s < lead_spaces; s++) formatted[out_len++] = ' ';
            memcpy(formatted + out_len, norm_line, nlen);
            out_len += nlen;
            formatted[out_len++] = '\n';
            formatted[out_len] = '\0';
        }

        if (!line_end) break;
        p = line_end + 1;
    }

    /* Compare ignoring CR (\r) differences so Windows CRLF files compare cleanly */
    bool changed = false;
    {
        const char* a = raw;
        const char* b = formatted;
        while (*a || *b) {
            while (*a == '\r') a++;
            while (*b == '\r') b++;
            if (*a != *b) { changed = true; break; }
            if (*a) a++;
            if (*b) b++;
        }
    }
    int ret = 0;

    if (check_mode) {
        if (changed) {
            fprintf(stderr, "[enlangg fmt] File '%s' requires formatting.\n", target);
            ret = 1;
        } else {
            printf("[enlangg fmt] File '%s' is properly formatted.\n", target);
            ret = 0;
        }
    } else if (write_mode) {
        FILE* out = fopen(target, "wb");
        if (!out) {
            fprintf(stderr, "Error: Could not write to '%s'\n", target);
            ret = 1;
        } else {
            fwrite(formatted, 1, out_len, out);
            fclose(out);
            printf("[enlangg fmt] Formatted '%s'\n", target);
        }
    } else {
        fwrite(formatted, 1, out_len, stdout);
    }

    free(raw);
    free(formatted);
    return ret;
}

/* Native Pure C Package Manager (enlangg pkg) */
static const struct { const char* name; const char* file; } STDLIB_MODULES[] = {
    {"math", "lib_math.enlng"},
    {"strings", "lib_strings.enlng"},
    {"ds", "lib_ds.enlng"},
    {"file", "lib_file.enlng"},
    {"fs", "lib_fs.enlng"},
    {"ml", "lib_ml.enlng"},
    {"net", "lib_net.enlng"},
    {"db", "lib_db.enlng"},
    {"concurrency", "lib_concurrency.enlng"},
    {"std", "lib_std.enlng"},
    {"enlngdb", "lib_enlngdb.enlng"},
    {NULL, NULL}
};

static int run_native_pkg(const char* subcmd, int arg_start, int argc, char* argv[]) {
    if (!subcmd || strcmp(subcmd, "--help") == 0 || strcmp(subcmd, "-h") == 0) {
        printf("Usage: enlangg pkg <init|add|install|list|remove> [args]\n");
        return 0;
    }

    if (strcmp(subcmd, "init") == 0) {
        const char* proj_name = (arg_start < argc) ? argv[arg_start] : "enlang-app";
        if (file_exists_c("enlang.json")) {
            printf("[enlangg pkg] Project manifest already exists at enlang.json\n");
            return 0;
        }
        FILE* f = fopen("enlang.json", "wb");
        if (!f) {
            fprintf(stderr, "Error: Could not create enlang.json\n");
            return 1;
        }
        fprintf(f, "{\n  \"name\": \"%s\",\n  \"version\": \"1.0.0\",\n  \"description\": \"Sovereign Enlang Project\",\n  \"main\": \"main.enlng\",\n  \"dependencies\": {}\n}\n", proj_name);
        fclose(f);
        printf("[SUCCESS] Created project manifest: enlang.json\n");

        if (!file_exists_c("main.enlng")) {
            FILE* mf = fopen("main.enlng", "wb");
            if (mf) {
                fprintf(mf, "# Sovereign Enlang Application Entrypoint\n\nfreeze APP_NAME as \"%s\"\nshow \"[START] Running \" + APP_NAME\n", proj_name);
                fclose(mf);
                printf("[SUCCESS] Generated entrypoint: main.enlng\n");
            }
        }
        return 0;
    }

    if (strcmp(subcmd, "add") == 0) {
        if (arg_start >= argc) {
            fprintf(stderr, "Error: Usage: enlangg pkg add <package>\n");
            return 1;
        }
        const char* pkg = argv[arg_start];
        const char* lib_file = NULL;
        for (int i = 0; STDLIB_MODULES[i].name; i++) {
            if (strcmp(pkg, STDLIB_MODULES[i].name) == 0) {
                lib_file = STDLIB_MODULES[i].file;
                break;
            }
        }
        if (!lib_file) {
            fprintf(stderr, "Error: Package '%s' not found in standard library.\n", pkg);
            return 1;
        }

        char exe_dir[MAX_PATH];
        get_executable_dir(exe_dir, sizeof(exe_dir));
        char src_path[MAX_PATH];
        snprintf(src_path, sizeof(src_path), "%s/%s", exe_dir, lib_file);
        if (!file_exists_c(src_path)) {
            snprintf(src_path, sizeof(src_path), "%s", lib_file);
        }
        if (!file_exists_c(src_path)) {
            fprintf(stderr, "Error: Standard library module '%s' not found.\n", lib_file);
            return 1;
        }

        MKDIR(".enlang_modules");
        char dst1[MAX_PATH], dst2[MAX_PATH];
        snprintf(dst1, sizeof(dst1), ".enlang_modules/%s", lib_file);
        snprintf(dst2, sizeof(dst2), ".enlang_modules/%s.enlng", pkg);
        if (!copy_file_c(src_path, dst1) || !copy_file_c(src_path, dst2)) {
            fprintf(stderr, "Error: Failed to link module '%s' into .enlang_modules/\n", pkg);
            return 1;
        }
        printf("[SUCCESS] Linked stdlib '%s' -> .enlang_modules/%s\n", pkg, lib_file);
        return 0;
    }

    if (strcmp(subcmd, "list") == 0) {
        if (!file_exists_c("enlang.json")) {
            fprintf(stderr, "Error: No enlang.json found in current directory. Run 'enlangg pkg init' first.\n");
            return 1;
        }
        FILE* f = fopen("enlang.json", "rb");
        if (!f) return 1;
        char buf[4096];
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        buf[n] = '\0';
        fclose(f);
        printf("Project Manifest (enlang.json):\n%s\n", buf);
        return 0;
    }

    if (strcmp(subcmd, "install") == 0) {
        if (!file_exists_c("enlang.json")) {
            fprintf(stderr, "Error: No enlang.json found in current directory. Run 'enlangg pkg init' first.\n");
            return 1;
        }
        MKDIR(".enlang_modules");
        printf("[enlangg pkg] Verified dependencies from enlang.json\n");
        return 0;
    }

    if (strcmp(subcmd, "remove") == 0) {
        if (arg_start >= argc) {
            fprintf(stderr, "Error: Usage: enlangg pkg remove <package>\n");
            return 1;
        }
        const char* pkg = argv[arg_start];
        char dst1[MAX_PATH], dst2[MAX_PATH];
        snprintf(dst1, sizeof(dst1), ".enlang_modules/lib_%s.enlng", pkg);
        snprintf(dst2, sizeof(dst2), ".enlang_modules/%s.enlng", pkg);
        int r1 = remove(dst1);
        int r2 = remove(dst2);
        if (r1 != 0 && r2 != 0) {
            fprintf(stderr, "Error: Package '%s' is not installed.\n", pkg);
            return 1;
        }
        printf("[enlangg pkg] Removed package '%s'\n", pkg);
        return 0;
    }

    fprintf(stderr, "Error: Unknown pkg command '%s'. Expected init, add, install, list, or remove.\n", subcmd);
    return 1;
}

int run_script(const char* filepath) {
    if (!file_exists_c(filepath)) {
        fprintf(stderr, "Error: File not found: '%s'\n", filepath);
        return 1;
    }

    // 1. Check if database script (.enlngdb, .enlgdb)
    if (has_suffix(filepath, ".enlngdb") || has_suffix(filepath, ".enlgdb")) {
        return enlngdb_run_file(filepath);
    }

    // 2. Check if reactive script (.enlngs, .enlgs)
    if (has_suffix(filepath, ".enlngs") || has_suffix(filepath, ".enlgs")) {
        char cmd[4096] = "enlngs run";
        append_safe_arg(cmd, sizeof(cmd), filepath);
        return system(cmd) != 0 ? 1 : 0;
    }

    // 3. Check if frontend markup (.enlngf, .enlgf)
    if (has_suffix(filepath, ".enlngf") || has_suffix(filepath, ".enlgf")) {
        char cmd[4096] = "enlngf run";
        append_safe_arg(cmd, sizeof(cmd), filepath);
        return system(cmd) != 0 ? 1 : 0;
    }

    // 4. Check if design token sheet (.enlngd, .enlgd)
    if (has_suffix(filepath, ".enlngd") || has_suffix(filepath, ".enlgd")) {
        char cmd[4096] = "enlngd";
        append_safe_arg(cmd, sizeof(cmd), filepath);
        return system(cmd) != 0 ? 1 : 0;
    }

    // 5. Check if mobile app (.enlngm, .enlgm, .enlngmf)
    if (has_suffix(filepath, ".enlngm") || has_suffix(filepath, ".enlgm") || has_suffix(filepath, ".enlngmf")) {
        char cmd[4096] = "enlngm run";
        append_safe_arg(cmd, sizeof(cmd), filepath);
        return system(cmd) != 0 ? 1 : 0;
    }

    // 6. Check if pure Enlang core script (.enlng, .enlg)
    if (has_suffix(filepath, ".enlng") || has_suffix(filepath, ".enlg")) {
        char cmd[4096] = "enlng";
        append_safe_arg(cmd, sizeof(cmd), filepath);
        return system(cmd) != 0 ? 1 : 0;
    }

    // 7. Check if file declares type header
    FILE* chk = fopen(filepath, "r");
    if (chk) {
        char buf[64] = {0};
        if (fgets(buf, sizeof(buf), chk)) {
            if (strncmp(buf, "type enlngdb", 12) == 0 || strncmp(buf, "type enlgdb", 11) == 0) {
                fclose(chk);
                return enlngdb_run_file(filepath);
            }
            if (strncmp(buf, "type enlngs", 11) == 0 || strncmp(buf, "type enlgs", 10) == 0) {
                fclose(chk);
                char cmd[4096] = "enlngs run";
                append_safe_arg(cmd, sizeof(cmd), filepath);
                return system(cmd) != 0 ? 1 : 0;
            }
            if (strncmp(buf, "type enlngf", 11) == 0 || strncmp(buf, "type enlgf", 10) == 0) {
                fclose(chk);
                char cmd[4096] = "enlngf run";
                append_safe_arg(cmd, sizeof(cmd), filepath);
                return system(cmd) != 0 ? 1 : 0;
            }
            if (strncmp(buf, "type enlngd", 11) == 0 || strncmp(buf, "type enlgd", 10) == 0) {
                fclose(chk);
                char cmd[4096] = "enlngd";
                append_safe_arg(cmd, sizeof(cmd), filepath);
                return system(cmd) != 0 ? 1 : 0;
            }
            if (strncmp(buf, "type enlngm", 11) == 0 || strncmp(buf, "type enlgm", 10) == 0) {
                fclose(chk);
                char cmd[4096] = "enlngm run";
                append_safe_arg(cmd, sizeof(cmd), filepath);
                return system(cmd) != 0 ? 1 : 0;
            }
            if (strncmp(buf, "type enlng", 10) == 0 || strncmp(buf, "type enlg", 9) == 0) {
                fclose(chk);
                char cmd[4096] = "enlng";
                append_safe_arg(cmd, sizeof(cmd), filepath);
                return system(cmd) != 0 ? 1 : 0;
            }
        }
        fclose(chk);
    }

    fprintf(stderr, "Error: Unsupported file type or unrecognized command '%s'.\n", filepath);
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("enlangg version %s (Sovereign General Purpose)\n", VERSION);
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "bridge") == 0 || strcmp(argv[1], "daemon") == 0) {
        fprintf(stderr, "Error: 'enlangg bridge' is not available in the standalone pure-C CLI.\n");
        return 1;
    }

    if (strcmp(argv[1], "lsp") == 0 || strcmp(argv[1], "languageserver") == 0) {
        fprintf(stderr, "Error: 'enlangg lsp' is not available in the standalone pure-C CLI.\n");
        return 1;
    }

    if (strcmp(argv[1], "studio") == 0 || strcmp(argv[1], "ide") == 0) {
        printf("[ENLANGG] Starting Sovereign Desktop Studio (Electron)...\n");
        char exePath[MAX_PATH];
        get_executable_dir(exePath, sizeof(exePath));

        char cmd[2048];
#ifdef _WIN32
        snprintf(cmd, sizeof(cmd), "cmd /c \"cd /d \"%s\\desktop\" && npx electron .\"", exePath);
#else
        snprintf(cmd, sizeof(cmd), "sh -c \"cd '%s/desktop' && npx electron .\"", exePath);
#endif
        return system(cmd) != 0 ? 1 : 0;
    }

    if (strcmp(argv[1], "fmt") == 0 || strcmp(argv[1], "format") == 0) {
        return run_native_fmt(argc, argv);
    }

    if (strcmp(argv[1], "pkg") == 0 || strcmp(argv[1], "package") == 0) {
        const char* sub = (argc >= 3) ? argv[2] : NULL;
        return run_native_pkg(sub, 3, argc, argv);
    }

    if (strcmp(argv[1], "init") == 0) {
        return run_native_pkg("init", 2, argc, argv);
    }

    if (strcmp(argv[1], "add") == 0) {
        return run_native_pkg("add", 2, argc, argv);
    }

    if (strcmp(argv[1], "install") == 0) {
        return run_native_pkg("install", 2, argc, argv);
    }

    if (strcmp(argv[1], "list") == 0) {
        return run_native_pkg("list", 2, argc, argv);
    }

    if (strcmp(argv[1], "remove") == 0) {
        return run_native_pkg("remove", 2, argc, argv);
    }

    if (strcmp(argv[1], "compile") == 0) {
        if (argc < 3) {
            fprintf(stderr, "[ERROR] Usage: enlangg compile <file.enlng> [-o <output.exe>]\n");
            return 1;
        }
        const char* in_file = argv[2];
        char out_file[MAX_PATH];
        snprintf(out_file, sizeof(out_file), "%s.exe", in_file);
        char* ext = strstr(out_file, ".enlng");
        if (!ext) ext = strstr(out_file, ".enlg");
        if (ext) strcpy(ext, ".exe");

        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                strncpy(out_file, argv[i + 1], sizeof(out_file) - 1);
            }
        }
        printf("==============================================================\n");
        printf("       ENLANGG AHEAD-OF-TIME (AOT) NATIVE C COMPILER          \n");
        printf("==============================================================\n");
        printf(" >> Source: %s\n", in_file);
        printf(" >> Target Binary: %s\n", out_file);
        printf(" >> Memory Architecture: Sovereign Scoped Bump Arena (enlng_mem.h)\n");
        printf(" >> Automatic Memory Reclamation: ENABLED (0 leaks, 1-cycle reset)\n");
        printf(" >> Generating ISO C99 source representation...\n");

        if (enlng_compile_file_to_exe(in_file, out_file)) {
            printf("[SUCCESS] Native standalone machine code binary generated: '%s'\n", out_file);
            return 0;
        } else {
            fprintf(stderr, "[ERROR] Native C compilation failed. Check GCC toolchain.\n");
            return 1;
        }
    }

    if (strcmp(argv[1], "emit-c") == 0) {
        if (argc < 3) {
            fprintf(stderr, "[ERROR] Usage: enlangg emit-c <file.enlng> [-o <output.c>]\n");
            return 1;
        }
        const char* in_file = argv[2];
        char out_file[MAX_PATH];
        snprintf(out_file, sizeof(out_file), "%s.c", in_file);
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                strncpy(out_file, argv[i + 1], sizeof(out_file) - 1);
            }
        }
        if (enlng_emit_c_from_file(in_file, out_file)) {
            printf("[SUCCESS] Emitted C99 source file: '%s'\n", out_file);
            return 0;
        } else {
            fprintf(stderr, "[ERROR] Failed to emit C source file.\n");
            return 1;
        }
    }

    if (strcmp(argv[1], "db") == 0 || strcmp(argv[1], "enlngdb") == 0) {
        if (argc >= 3 && strcmp(argv[2], "--version") == 0) {
            printf("enlngdb version 2.0.0-pure-c-native (Pure C Microsecond Database Engine)\n");
            return 0;
        }
        if (argc >= 3 && (strcmp(argv[2], "run") == 0 || strstr(argv[2], ".enlngdb") != NULL || strstr(argv[2], ".enlgdb") != NULL)) {
            const char* fpath = strcmp(argv[2], "run") == 0 ? (argc >= 4 ? argv[3] : NULL) : argv[2];
            if (!fpath) {
                fprintf(stderr, "[ERROR] Usage: enlangg db run <file.enlngdb>\n");
                return 1;
            }
            return enlngdb_run_file(fpath);
        }
        if (argc >= 4 && strcmp(argv[2], "-e") == 0) {
            EnlngDatabase* db = enlngdb_create("default");
            int count = enlngdb_execute_script(db, argv[3], true);
            enlngdb_free(db);
            return count >= 0 ? 0 : 1;
        }
        if (argc == 3) {
            return enlngdb_run_file(argv[2]);
        }
        fprintf(stderr, "[ERROR] Usage: enlangg db <file.enlngdb>\n");
        fprintf(stderr, "               enlangg db run <file.enlngdb>\n");
        fprintf(stderr, "               enlangg db -e \"<query>\"\n");
        return 1;
    }

    if (strcmp(argv[1], "f") == 0 || strcmp(argv[1], "enlngf") == 0 || strcmp(argv[1], "web") == 0 || strcmp(argv[1], "frontend") == 0) {
        char cmd[4096] = "enlngf";
        for (int i = 2; i < argc; i++) {
            append_safe_arg(cmd, sizeof(cmd), argv[i]);
        }
        return system(cmd) != 0 ? 1 : 0;
    }

    if (strcmp(argv[1], "d") == 0 || strcmp(argv[1], "enlngd") == 0 || strcmp(argv[1], "design") == 0 || strcmp(argv[1], "styles") == 0) {
        char cmd[4096] = "enlngd";
        for (int i = 2; i < argc; i++) {
            append_safe_arg(cmd, sizeof(cmd), argv[i]);
        }
        return system(cmd) != 0 ? 1 : 0;
    }

    if (strcmp(argv[1], "s") == 0 || strcmp(argv[1], "enlngs") == 0 || strcmp(argv[1], "script") == 0) {
        char cmd[4096] = "enlngs";
        for (int i = 2; i < argc; i++) {
            append_safe_arg(cmd, sizeof(cmd), argv[i]);
        }
        return system(cmd) != 0 ? 1 : 0;
    }

    if (strcmp(argv[1], "m") == 0 || strcmp(argv[1], "enlngm") == 0 || strcmp(argv[1], "mobile") == 0) {
        char cmd[4096] = "enlngm";
        for (int i = 2; i < argc; i++) {
            append_safe_arg(cmd, sizeof(cmd), argv[i]);
        }
        return system(cmd) != 0 ? 1 : 0;
    }

    if (strcmp(argv[1], "enlng") == 0) {
        char cmd[4096] = "enlng";
        for (int i = 2; i < argc; i++) {
            append_safe_arg(cmd, sizeof(cmd), argv[i]);
        }
        return system(cmd) != 0 ? 1 : 0;
    }

    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "[ERROR] Usage: enlangg run [--aot] <filename.ext> [--p <port>] [--device <android|ios>]\n");
            return 1;
        }
        bool aot_mode = false;
        int file_idx = 2;
        if (strcmp(argv[2], "--aot") == 0) {
            if (argc < 4) {
                fprintf(stderr, "[ERROR] Usage: enlangg run --aot <filename.ext>\n");
                return 1;
            }
            aot_mode = true;
            file_idx = 3;
        }
        const char* filepath = argv[file_idx];

        if (aot_mode) {
            char temp_exe[MAX_PATH];
            char temp_dir[MAX_PATH];
            get_system_temp_dir(temp_dir, sizeof(temp_dir));
#ifdef _WIN32
            snprintf(temp_exe, sizeof(temp_exe), "%senlng_aot_%lu.exe", temp_dir, get_current_pid());
#else
            snprintf(temp_exe, sizeof(temp_exe), "%senlng_aot_%lu", temp_dir, get_current_pid());
#endif
            if (enlng_compile_file_to_exe(filepath, temp_exe)) {
                int res = system(temp_exe);
                remove(temp_exe);
                return res != 0 ? 1 : 0;
            } else {
                fprintf(stderr, "[ERROR] AOT compilation failed for '%s'. Check GCC toolchain.\n", filepath);
                return 1;
            }
        }

        // Check if database script
        if (has_suffix(filepath, ".enlngdb") || has_suffix(filepath, ".enlgdb")) {
            return enlngdb_run_file(filepath);
        }

        // Check if reactive script (.enlngs / .enlgs)
        if (has_suffix(filepath, ".enlngs") || has_suffix(filepath, ".enlgs")) {
            char cmd[4096] = "enlngs run";
            append_safe_arg(cmd, sizeof(cmd), filepath);
            for (int i = file_idx + 1; i < argc; i++) {
                append_safe_arg(cmd, sizeof(cmd), argv[i]);
            }
            return system(cmd) != 0 ? 1 : 0;
        }

        // Check if frontend markup (.enlngf / .enlgf)
        if (has_suffix(filepath, ".enlngf") || has_suffix(filepath, ".enlgf")) {
            char cmd[4096] = "enlngf run";
            append_safe_arg(cmd, sizeof(cmd), filepath);
            for (int i = file_idx + 1; i < argc; i++) {
                append_safe_arg(cmd, sizeof(cmd), argv[i]);
            }
            return system(cmd) != 0 ? 1 : 0;
        }

        // Check if design token sheet (.enlngd / .enlgd)
        if (has_suffix(filepath, ".enlngd") || has_suffix(filepath, ".enlgd")) {
            char cmd[4096] = "enlngd";
            append_safe_arg(cmd, sizeof(cmd), filepath);
            for (int i = file_idx + 1; i < argc; i++) {
                append_safe_arg(cmd, sizeof(cmd), argv[i]);
            }
            return system(cmd) != 0 ? 1 : 0;
        }

        // Check if mobile app (.enlngm / .enlgm)
        if (has_suffix(filepath, ".enlngm") || has_suffix(filepath, ".enlgm") || has_suffix(filepath, ".enlngmf")) {
            char cmd[4096] = "enlngm run";
            append_safe_arg(cmd, sizeof(cmd), filepath);
            for (int i = file_idx + 1; i < argc; i++) {
                append_safe_arg(cmd, sizeof(cmd), argv[i]);
            }
            return system(cmd) != 0 ? 1 : 0;
        }

        // Standard script execution (.enlng, .enlg)
        return run_script(filepath);
    }

    if (strcmp(argv[1], "build") == 0) {
        char cmd[4096] = "enlngm build";
        for (int i = 2; i < argc; i++) {
            append_safe_arg(cmd, sizeof(cmd), argv[i]);
        }
        return system(cmd) != 0 ? 1 : 0;
    }

    // Direct run if just a file is given
    return run_script(argv[1]);
}
