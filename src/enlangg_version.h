/*
 * =====================================================================
 *   ENLANGG SOVEREIGN TOOLCHAIN - UNIFIED VERSION & HOME RESOLUTION
 *   src/enlangg_version.h - Single Source of Truth for Ecosystem Version
 *   and Executable-Relative Directory Discovery (../lib, ../include)
 * =====================================================================
 */

#ifndef ENLANGG_VERSION_H
#define ENLANGG_VERSION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
  #define ENLANGG_PATH_SEP '\\'
#else
  #include <unistd.h>
  #include <limits.h>
  #ifdef __APPLE__
    #include <mach-o/dyld.h>
  #endif
  #define ENLANGG_PATH_SEP '/'
#endif

/* Unified Toolchain Version (Independent of Enlangg Studio IDE Version) */
#define ENLANGG_VERSION "5.4.1"

#define ENLNG_VERSION   ENLANGG_VERSION
#define ENLNGDB_VERSION ENLANGG_VERSION
#define ENLNGF_VERSION  ENLANGG_VERSION
#define ENLNGD_VERSION  ENLANGG_VERSION
#define ENLNGS_VERSION  ENLANGG_VERSION
#define ENLNGM_VERSION  ENLANGG_VERSION

#ifndef ENLANGG_MAX_PATH
  #define ENLANGG_MAX_PATH 4096
#endif

static inline bool enlangg_file_exists(const char *path) {
    if (!path || !*path) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

static inline void enlangg_strip_trailing_sep(char *path) {
    if (!path) return;
    size_t len = strlen(path);
    while (len > 1 && (path[len - 1] == '/' || path[len - 1] == '\\')) {
#ifdef _WIN32
        if (len == 3 && path[1] == ':') break;
#endif
        path[--len] = '\0';
    }
}

/*
 * Resolves the canonical directory containing the currently running executable,
 * following symlinks (e.g., /usr/local/bin/enlng -> /usr/local/enlangg/bin).
 */
static inline bool enlangg_get_exe_dir(char *out_dir, size_t max_len) {
    if (!out_dir || max_len == 0) return false;
    out_dir[0] = '\0';

#ifdef _WIN32
    DWORD len = GetModuleFileNameA(NULL, out_dir, (DWORD)max_len);
    if (len == 0 || len >= (DWORD)max_len) return false;
    char *last_sep = strrchr(out_dir, '\\');
    char *alt_sep = strrchr(out_dir, '/');
    if (alt_sep && (!last_sep || alt_sep > last_sep)) last_sep = alt_sep;
    if (!last_sep) return false;
    *last_sep = '\0';
    enlangg_strip_trailing_sep(out_dir);
    return true;
#elif defined(__APPLE__)
    char raw_path[ENLANGG_MAX_PATH];
    uint32_t sz = (uint32_t)sizeof(raw_path);
    if (_NSGetExecutablePath(raw_path, &sz) != 0) return false;
    char resolved[ENLANGG_MAX_PATH];
    const char *use_path = raw_path;
    if (realpath(raw_path, resolved) != NULL) {
        use_path = resolved;
    }
    strncpy(out_dir, use_path, max_len - 1);
    out_dir[max_len - 1] = '\0';
    char *last_sep = strrchr(out_dir, '/');
    if (!last_sep) return false;
    *last_sep = '\0';
    enlangg_strip_trailing_sep(out_dir);
    return true;
#else
    char link_buf[ENLANGG_MAX_PATH];
    ssize_t len = readlink("/proc/self/exe", link_buf, sizeof(link_buf) - 1);
    if (len < 0) {
        len = readlink("/proc/curproc/file", link_buf, sizeof(link_buf) - 1);
    }
    if (len < 0) {
        len = readlink("/proc/self/path/a.out", link_buf, sizeof(link_buf) - 1);
    }
    if (len > 0) {
        link_buf[len] = '\0';
        char resolved[ENLANGG_MAX_PATH];
        const char *use_path = link_buf;
        if (realpath(link_buf, resolved) != NULL) {
            use_path = resolved;
        }
        strncpy(out_dir, use_path, max_len - 1);
        out_dir[max_len - 1] = '\0';
        char *last_sep = strrchr(out_dir, '/');
        if (!last_sep) return false;
        *last_sep = '\0';
        enlangg_strip_trailing_sep(out_dir);
        return true;
    }
#ifdef ENLANGG_DEFAULT_HOME
    snprintf(out_dir, max_len, "%s/bin", ENLANGG_DEFAULT_HOME);
    return true;
#else
    return false;
#endif
#endif
}

/*
 * Determines the toolchain home directory:
 * 1. If ENLANGG_HOME is set in the environment, uses it as an override.
 * 2. Otherwise derives it from the executable directory: if the executable resides
 *    in "<prefix>/bin", home is "<prefix>" (so "../lib" and "../include" are under it);
 *    otherwise home is the executable directory itself.
 */
static inline bool enlangg_get_home_dir(char *out_home, size_t max_len) {
    if (!out_home || max_len == 0) return false;
    out_home[0] = '\0';

    const char *env_home = getenv("ENLANGG_HOME");
    if (env_home && env_home[0] != '\0') {
        strncpy(out_home, env_home, max_len - 1);
        out_home[max_len - 1] = '\0';
        enlangg_strip_trailing_sep(out_home);
        return true;
    }

    char exe_dir[ENLANGG_MAX_PATH];
    if (!enlangg_get_exe_dir(exe_dir, sizeof(exe_dir))) {
        return false;
    }

    size_t len = strlen(exe_dir);
    if (len > 4) {
        const char *tail = exe_dir + (len - 4);
        bool is_bin_sep = (tail[0] == '/' || tail[0] == '\\');
        bool is_bin_name = (tail[1] == 'b' || tail[1] == 'B') &&
                           (tail[2] == 'i' || tail[2] == 'I') &&
                           (tail[3] == 'n' || tail[3] == 'N');
        if (is_bin_sep && is_bin_name) {
            strncpy(out_home, exe_dir, max_len - 1);
            out_home[max_len - 1] = '\0';
            if (len - 4 < max_len) {
                out_home[len - 4] = '\0';
            }
            enlangg_strip_trailing_sep(out_home);
            return true;
        }
    }

    strncpy(out_home, exe_dir, max_len - 1);
    out_home[max_len - 1] = '\0';
    return true;
}

/*
 * Resolves a module or standard library file path:
 * 1. Direct path relative to current working directory (or absolute path)
 * 2. Local project .enlang_modules/<filepath>
 * 3. Executable-relative ../lib/<filepath> and <exe_dir>/<filepath>
 * 4. ENLANGG_HOME override ($ENLANGG_HOME/lib/<filepath> and $ENLANGG_HOME/<filepath>)
 */
static inline bool enlangg_resolve_stdlib_path(const char *filepath, char *out_path, size_t max_len) {
    if (!filepath || !*filepath || !out_path || max_len == 0) return false;

    /* 1. Direct path (CWD-relative or absolute) */
    if (enlangg_file_exists(filepath)) {
        snprintf(out_path, max_len, "%s", filepath);
        return true;
    }

    /* 2. Local project .enlang_modules directory */
    snprintf(out_path, max_len, ".enlang_modules%c%s", ENLANGG_PATH_SEP, filepath);
    if (enlangg_file_exists(out_path)) {
        return true;
    }

    /* 3. ENLANGG_HOME override if set */
    const char *env_home = getenv("ENLANGG_HOME");
    if (env_home && env_home[0] != '\0') {
        char clean_env[ENLANGG_MAX_PATH];
        strncpy(clean_env, env_home, sizeof(clean_env) - 1);
        clean_env[sizeof(clean_env) - 1] = '\0';
        enlangg_strip_trailing_sep(clean_env);

        snprintf(out_path, max_len, "%s%clib%c%s", clean_env, ENLANGG_PATH_SEP, ENLANGG_PATH_SEP, filepath);
        if (enlangg_file_exists(out_path)) return true;

        snprintf(out_path, max_len, "%s%c%s", clean_env, ENLANGG_PATH_SEP, filepath);
        if (enlangg_file_exists(out_path)) return true;
    }

    /* 4. Executable-relative discovery (../lib and exe_dir) */
    char exe_dir[ENLANGG_MAX_PATH];
    if (enlangg_get_exe_dir(exe_dir, sizeof(exe_dir))) {
        /* Check <exe_dir>/../lib/<filepath> */
        char parent_dir[ENLANGG_MAX_PATH];
        strncpy(parent_dir, exe_dir, sizeof(parent_dir) - 1);
        parent_dir[sizeof(parent_dir) - 1] = '\0';
        char *last_sep = strrchr(parent_dir, '\\');
        char *alt_sep = strrchr(parent_dir, '/');
        if (alt_sep && (!last_sep || alt_sep > last_sep)) last_sep = alt_sep;
        if (last_sep) {
            *last_sep = '\0';
            snprintf(out_path, max_len, "%s%clib%c%s", parent_dir, ENLANGG_PATH_SEP, ENLANGG_PATH_SEP, filepath);
            if (enlangg_file_exists(out_path)) return true;

            snprintf(out_path, max_len, "%s%c%s", parent_dir, ENLANGG_PATH_SEP, filepath);
            if (enlangg_file_exists(out_path)) return true;
        }

        /* Check <exe_dir>/lib/<filepath> and <exe_dir>/<filepath> */
        snprintf(out_path, max_len, "%s%clib%c%s", exe_dir, ENLANGG_PATH_SEP, ENLANGG_PATH_SEP, filepath);
        if (enlangg_file_exists(out_path)) return true;

        snprintf(out_path, max_len, "%s%c%s", exe_dir, ENLANGG_PATH_SEP, filepath);
        if (enlangg_file_exists(out_path)) return true;
    }

    out_path[0] = '\0';
    return false;
}

#endif /* ENLANGG_VERSION_H */
