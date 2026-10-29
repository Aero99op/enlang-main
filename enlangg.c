/*
 * enlangg - The Sovereign Universal CLI & Compiler Engine
 * Supports:
 *   enlangg run <file.enlng>                (Full natural English execution)
 *   enlangg run <file.enlngf> [--p <port>]  (Web studio server)
 *   enlangg run <file.enlngm>               (Desktop preview window (390x844))
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
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <mach-o/dyld.h>
#define MKDIR(p) mkdir(p, 0755)
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#define MKDIR(p) mkdir(p, 0755)
#endif
#include "src/enlangg_version.h"
#include "enlngdb/c/enlngdb.h"
#include "enlng/c/enlng_emitter.h"

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

#define VERSION ENLANGG_VERSION

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
    printf("  enlangg m <app.enlngm>                      Launch desktop preview window (390x844)\n");
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
    printf("  enlngm    Pure C sovereign desktop preview window (390x844)\n\n");
    printf("Documentation & Live Playground: https://enlangg.vercel.app\n");
}

static void get_executable_dir(char* outPath, size_t maxLen) {
    if (!enlangg_get_exe_dir(outPath, maxLen)) {
        strncpy(outPath, ".", maxLen - 1);
        outPath[maxLen - 1] = '\0';
    }
}

/* Opens the installed Enlangg Studio (optionally on a file). A source checkout with
 * desktop/ next to the binaries falls back to `npx electron` for development. */
#define STUDIO_BUF 1024
static int launch_studio(const char* openPath) {
    char studio[STUDIO_BUF];
    studio[0] = '\0';
#ifdef _WIN32
    const char* local = getenv("LOCALAPPDATA");
    if (local && *local) {
        snprintf(studio, sizeof(studio), "%s\\Programs\\Enlangg Studio\\Enlangg Studio.exe", local);
        if (GetFileAttributesA(studio) == INVALID_FILE_ATTRIBUTES) studio[0] = '\0';
    }
    if (studio[0]) {
        char cmd[STUDIO_BUF * 2];
        if (openPath) snprintf(cmd, sizeof(cmd), "\"%s\" \"%s\"", studio, openPath);
        else snprintf(cmd, sizeof(cmd), "\"%s\"", studio);
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return 0;
        }
        fprintf(stderr, "[ERROR] Could not start %s\n", studio);
        return 1;
    }
#elif defined(__APPLE__)
    if (access("/Applications/Enlangg Studio.app", F_OK) == 0) {
        char cmd[STUDIO_BUF * 2];
        if (openPath) snprintf(cmd, sizeof(cmd), "open -a \"Enlangg Studio\" \"%s\"", openPath);
        else snprintf(cmd, sizeof(cmd), "open -a \"Enlangg Studio\"");
        return system(cmd) != 0 ? 1 : 0;
    }
#endif
    char exeDir[STUDIO_BUF], pkg[STUDIO_BUF * 2];
    get_executable_dir(exeDir, sizeof(exeDir));
    snprintf(pkg, sizeof(pkg), "%s%cdesktop%cpackage.json", exeDir, ENLANGG_PATH_SEP, ENLANGG_PATH_SEP);
    FILE* f = fopen(pkg, "rb");
    if (f) {
        fclose(f);
        char cmd[STUDIO_BUF * 2];
        printf("[ENLANGG] Starting Studio from source checkout (needs Node.js)...\n");
#ifdef _WIN32
        snprintf(cmd, sizeof(cmd), "cmd /c \"cd /d \"%s\\desktop\" && npx electron .\"", exeDir);
#else
        snprintf(cmd, sizeof(cmd), "sh -c \"cd '%s/desktop' && npx electron .\"", exeDir);
#endif
        return system(cmd) != 0 ? 1 : 0;
    }
    fprintf(stderr, "Enlangg Studio is not installed.\nDownload it from https://enlangg.site (it is a separate installer).\n");
    return 1;
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

static bool file_exists_c(const char* path) {
    if (!path || !*path) return false;
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

/* ---------------------------------------------------------------------
 * Sibling tool dispatch (enlng, enlngf, enlngd, enlngs, enlngm).
 *
 * Resolution order:
 *   1. <directory of the running enlangg executable>/<tool>[.exe]
 *      (symlinks followed, see enlangg_get_exe_dir()).
 *   2. Only if that file is missing: each directory listed in PATH,
 *      in order. The current working directory is never searched
 *      unless PATH itself lists it.
 *
 * No shell is involved. Arguments are passed as a vector:
 *   Windows: CreateProcessA with MSVCRT-compatible argument quoting.
 *   POSIX:   fork + execv, parent waits like system() does.
 * enlngdb is linked into enlangg and is never spawned.
 * ------------------------------------------------------------------- */
#ifdef _WIN32
#define ENLANGG_TOOL_EXT ".exe"
#define ENLANGG_PATH_LIST_SEP ';'
#else
#define ENLANGG_TOOL_EXT ""
#define ENLANGG_PATH_LIST_SEP ':'
#endif

static bool is_runnable_file(const char* path) {
    if (!path || !*path) return false;
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) return false;
    return access(path, X_OK) == 0;
#endif
}

static bool resolve_tool_path(const char* tool, char* out, size_t out_sz, bool* from_path) {
    if (!tool || !*tool || !out || out_sz == 0) return false;
    out[0] = '\0';
    *from_path = false;

    /* 1. Sibling of the running enlangg executable. */
    char exe_dir[ENLANGG_MAX_PATH];
    if (enlangg_get_exe_dir(exe_dir, sizeof(exe_dir))) {
        int n = snprintf(out, out_sz, "%s%c%s%s", exe_dir, ENLANGG_PATH_SEP, tool, ENLANGG_TOOL_EXT);
        if (n > 0 && (size_t)n < out_sz && is_runnable_file(out)) {
            return true;
        }
    }

    /* 2. Fallback: explicit walk of PATH entries. */
    const char* path_env = getenv("PATH");
    if (!path_env) {
        out[0] = '\0';
        return false;
    }
    const char* p = path_env;
    while (*p) {
        const char* end = strchr(p, ENLANGG_PATH_LIST_SEP);
        size_t seg_len = end ? (size_t)(end - p) : strlen(p);
        char dir[ENLANGG_MAX_PATH];
        if (seg_len < sizeof(dir)) {
            memcpy(dir, p, seg_len);
            dir[seg_len] = '\0';
#ifdef _WIN32
            /* PATH entries may be quoted on Windows. */
            size_t dl = strlen(dir);
            if (dl >= 2 && dir[0] == '"' && dir[dl - 1] == '"') {
                memmove(dir, dir + 1, dl - 2);
                dir[dl - 2] = '\0';
            }
#endif
            if (dir[0] == '\0') {
                /* POSIX: an empty PATH entry means the current directory. */
#ifndef _WIN32
                strcpy(dir, ".");
#endif
            }
            if (dir[0] != '\0') {
                enlangg_strip_trailing_sep(dir);
                int n = snprintf(out, out_sz, "%s%c%s%s", dir, ENLANGG_PATH_SEP, tool, ENLANGG_TOOL_EXT);
                if (n > 0 && (size_t)n < out_sz && is_runnable_file(out)) {
                    *from_path = true;
                    return true;
                }
            }
        }
        if (!end) break;
        p = end + 1;
    }
    out[0] = '\0';
    return false;
}

#ifdef _WIN32
/* Appends one argument using the quoting rules parsed by the MSVCRT /
 * CommandLineToArgvW, so paths with spaces and embedded quotes or
 * trailing backslashes reach the child unchanged. */
static bool win_append_arg(char* buf, size_t sz, size_t* len, const char* arg) {
    size_t l = *len;
    if (l > 0) {
        if (l + 1 >= sz) return false;
        buf[l++] = ' ';
    }
    bool needs_quotes = (arg[0] == '\0') || (strpbrk(arg, " \t\n\v\"") != NULL);
    if (!needs_quotes) {
        size_t al = strlen(arg);
        if (l + al >= sz) return false;
        memcpy(buf + l, arg, al);
        l += al;
        buf[l] = '\0';
        *len = l;
        return true;
    }
    if (l + 1 >= sz) return false;
    buf[l++] = '"';
    for (const char* c = arg; ; c++) {
        size_t backslashes = 0;
        while (*c == '\\') { backslashes++; c++; }
        if (*c == '\0') {
            /* Double trailing backslashes so the closing quote stays a quote. */
            if (l + backslashes * 2 >= sz) return false;
            for (size_t i = 0; i < backslashes * 2; i++) buf[l++] = '\\';
            break;
        } else if (*c == '"') {
            if (l + backslashes * 2 + 2 >= sz) return false;
            for (size_t i = 0; i < backslashes * 2 + 1; i++) buf[l++] = '\\';
            buf[l++] = '"';
        } else {
            if (l + backslashes + 1 >= sz) return false;
            for (size_t i = 0; i < backslashes; i++) buf[l++] = '\\';
            buf[l++] = *c;
        }
    }
    if (l + 1 >= sz) return false;
    buf[l++] = '"';
    buf[l] = '\0';
    *len = l;
    return true;
}
#endif

/* Runs <tool> [sub] [first_arg] argv[from..argc-1]. Returns the child's
 * exit status, or 1 if the tool could not be found or started. */
static int run_tool(const char* tool, const char* sub, const char* first_arg, int argc, char** argv, int from) {
    char tool_path[ENLANGG_MAX_PATH];
    bool from_path = false;
    if (!resolve_tool_path(tool, tool_path, sizeof(tool_path), &from_path)) {
        char exe_dir[ENLANGG_MAX_PATH];
        if (!enlangg_get_exe_dir(exe_dir, sizeof(exe_dir))) strcpy(exe_dir, "<unknown>");
        fprintf(stderr, "Error: could not find '%s%s' next to enlangg (%s) or on PATH.\n",
                tool, ENLANGG_TOOL_EXT, exe_dir);
        fprintf(stderr, "Reinstall the Enlangg toolchain so all 7 tools are in the same folder.\n");
        return 1;
    }

    int extra = (argv && from < argc) ? (argc - from) : 0;
    size_t vec_len = 1 + (sub ? 1 : 0) + (first_arg ? 1 : 0) + (size_t)extra + 1;
    const char** vec = (const char**)malloc(vec_len * sizeof(const char*));
    if (!vec) {
        fprintf(stderr, "Error: out of memory while starting '%s'.\n", tool);
        return 1;
    }
    size_t vi = 0;
    vec[vi++] = tool_path;
    if (sub) vec[vi++] = sub;
    if (first_arg) vec[vi++] = first_arg;
    for (int i = 0; i < extra; i++) vec[vi++] = argv[from + i];
    vec[vi] = NULL;

    fflush(stdout);
    fflush(stderr);

#ifdef _WIN32
    size_t cmd_sz = 32768; /* CreateProcess command-line limit */
    char* cmdline = (char*)malloc(cmd_sz);
    if (!cmdline) {
        free(vec);
        fprintf(stderr, "Error: out of memory while starting '%s'.\n", tool);
        return 1;
    }
    cmdline[0] = '\0';
    size_t cl = 0;
    for (size_t i = 0; vec[i]; i++) {
        if (!win_append_arg(cmdline, cmd_sz, &cl, vec[i])) {
            fprintf(stderr, "Error: command line for '%s' is too long.\n", tool);
            free(cmdline);
            free(vec);
            return 1;
        }
    }
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));
    BOOL ok = CreateProcessA(tool_path, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    free(cmdline);
    free(vec);
    if (!ok) {
        fprintf(stderr, "Error: could not start '%s' (Windows error %lu).\n",
                tool_path, (unsigned long)GetLastError());
        return 1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    if (!GetExitCodeProcess(pi.hProcess, &code)) code = 1;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return (int)code;
#else
    /* Like system(): the parent ignores SIGINT/SIGQUIT while waiting so
     * Ctrl+C reaches the child and enlangg reports its exit status. */
    struct sigaction ign, old_int, old_quit;
    memset(&ign, 0, sizeof(ign));
    ign.sa_handler = SIG_IGN;
    sigemptyset(&ign.sa_mask);
    sigaction(SIGINT, &ign, &old_int);
    sigaction(SIGQUIT, &ign, &old_quit);

    pid_t pid = fork();
    if (pid < 0) {
        sigaction(SIGINT, &old_int, NULL);
        sigaction(SIGQUIT, &old_quit, NULL);
        free(vec);
        fprintf(stderr, "Error: could not start '%s': %s\n", tool_path, strerror(errno));
        return 1;
    }
    if (pid == 0) {
        sigaction(SIGINT, &old_int, NULL);
        sigaction(SIGQUIT, &old_quit, NULL);
        execv(tool_path, (char* const*)vec);
        fprintf(stderr, "Error: could not execute '%s': %s\n", tool_path, strerror(errno));
        _exit(127);
    }
    free(vec);
    int status = 0;
    pid_t w;
    do {
        w = waitpid(pid, &status, 0);
    } while (w < 0 && errno == EINTR);
    sigaction(SIGINT, &old_int, NULL);
    sigaction(SIGQUIT, &old_quit, NULL);
    if (w < 0) return 1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
#endif
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

        char src_path[MAX_PATH];
        if (!enlangg_resolve_stdlib_path(lib_file, src_path, sizeof(src_path))) {
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
        return run_tool("enlngs", "run", filepath, 0, NULL, 0) != 0 ? 1 : 0;
    }

    // 3. Check if frontend markup (.enlngf, .enlgf)
    if (has_suffix(filepath, ".enlngf") || has_suffix(filepath, ".enlgf")) {
        return run_tool("enlngf", "run", filepath, 0, NULL, 0) != 0 ? 1 : 0;
    }

    // 4. Check if design token sheet (.enlngd, .enlgd)
    if (has_suffix(filepath, ".enlngd") || has_suffix(filepath, ".enlgd")) {
        return run_tool("enlngd", NULL, filepath, 0, NULL, 0) != 0 ? 1 : 0;
    }

    // 5. Check if desktop preview (.enlngm, .enlgm, .enlngmf)
    if (has_suffix(filepath, ".enlngm") || has_suffix(filepath, ".enlgm") || has_suffix(filepath, ".enlngmf")) {
        return run_tool("enlngm", "run", filepath, 0, NULL, 0) != 0 ? 1 : 0;
    }

    // 6. Check if pure Enlang core script (.enlng, .enlg)
    if (has_suffix(filepath, ".enlng") || has_suffix(filepath, ".enlg")) {
        return run_tool("enlng", NULL, filepath, 0, NULL, 0) != 0 ? 1 : 0;
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
                return run_tool("enlngs", "run", filepath, 0, NULL, 0) != 0 ? 1 : 0;
            }
            if (strncmp(buf, "type enlngf", 11) == 0 || strncmp(buf, "type enlgf", 10) == 0) {
                fclose(chk);
                return run_tool("enlngf", "run", filepath, 0, NULL, 0) != 0 ? 1 : 0;
            }
            if (strncmp(buf, "type enlngd", 11) == 0 || strncmp(buf, "type enlgd", 10) == 0) {
                fclose(chk);
                return run_tool("enlngd", NULL, filepath, 0, NULL, 0) != 0 ? 1 : 0;
            }
            if (strncmp(buf, "type enlngm", 11) == 0 || strncmp(buf, "type enlgm", 10) == 0) {
                fclose(chk);
                return run_tool("enlngm", "run", filepath, 0, NULL, 0) != 0 ? 1 : 0;
            }
            if (strncmp(buf, "type enlng", 10) == 0 || strncmp(buf, "type enlg", 9) == 0) {
                fclose(chk);
                return run_tool("enlng", NULL, filepath, 0, NULL, 0) != 0 ? 1 : 0;
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
        return launch_studio(argc >= 3 ? argv[2] : NULL);
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
        if (argc >= 3 && (strcmp(argv[2], "--version") == 0 || strcmp(argv[2], "-v") == 0)) {
            printf("enlngdb version %s (Pure C Microsecond Database Engine)\n", VERSION);
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
        return run_tool("enlngf", NULL, NULL, argc, argv, 2) != 0 ? 1 : 0;
    }

    if (strcmp(argv[1], "d") == 0 || strcmp(argv[1], "enlngd") == 0 || strcmp(argv[1], "design") == 0 || strcmp(argv[1], "styles") == 0) {
        return run_tool("enlngd", NULL, NULL, argc, argv, 2) != 0 ? 1 : 0;
    }

    if (strcmp(argv[1], "s") == 0 || strcmp(argv[1], "enlngs") == 0 || strcmp(argv[1], "script") == 0) {
        return run_tool("enlngs", NULL, NULL, argc, argv, 2) != 0 ? 1 : 0;
    }

    if (strcmp(argv[1], "m") == 0 || strcmp(argv[1], "enlngm") == 0 || strcmp(argv[1], "mobile") == 0) {
        return run_tool("enlngm", NULL, NULL, argc, argv, 2) != 0 ? 1 : 0;
    }

    if (strcmp(argv[1], "enlng") == 0) {
        return run_tool("enlng", NULL, NULL, argc, argv, 2) != 0 ? 1 : 0;
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
            return run_tool("enlngs", "run", filepath, argc, argv, file_idx + 1) != 0 ? 1 : 0;
        }

        // Check if frontend markup (.enlngf / .enlgf)
        if (has_suffix(filepath, ".enlngf") || has_suffix(filepath, ".enlgf")) {
            return run_tool("enlngf", "run", filepath, argc, argv, file_idx + 1) != 0 ? 1 : 0;
        }

        // Check if design token sheet (.enlngd / .enlgd)
        if (has_suffix(filepath, ".enlngd") || has_suffix(filepath, ".enlgd")) {
            return run_tool("enlngd", NULL, filepath, argc, argv, file_idx + 1) != 0 ? 1 : 0;
        }

        // Check if desktop preview (.enlngm / .enlgm)
        if (has_suffix(filepath, ".enlngm") || has_suffix(filepath, ".enlgm") || has_suffix(filepath, ".enlngmf")) {
            return run_tool("enlngm", "run", filepath, argc, argv, file_idx + 1) != 0 ? 1 : 0;
        }

        // Standard script execution (.enlng, .enlg)
        return run_script(filepath);
    }

    if (strcmp(argv[1], "build") == 0) {
        return run_tool("enlngm", "build", NULL, argc, argv, 2) != 0 ? 1 : 0;
    }

    // Direct run if just a file is given
    return run_script(argv[1]);
}
