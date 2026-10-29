/*
 * Enlangg Setup (enlangg-setup.exe) - pure C Win32 installer / uninstaller
 *
 *   enlangg-setup.exe                  interactive installer
 *   enlangg-setup.exe /S [options]     silent install
 *   <dir>\uninstall.exe                uninstaller (same binary)
 *
 * Options: /DIR=<path>  /NOPATH  /NOASSOC  /STUDIO  /LOG=<file>
 *
 * Per-user only: files go to %LOCALAPPDATA%\Programs\Enlangg, registry writes
 * go to HKCU. Never elevates. The toolchain payload is embedded as a resource.
 * Enlangg Studio is optional: its installer is downloaded and verified against
 * the SHA-256 published in enlangg.site/version.json before it is launched.
 */
#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0601
#define _WIN32_IE 0x0600
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wininet.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>

#include "../src/enlangg_version.h"

#ifndef CALG_SHA_256
#define CALG_SHA_256 0x0000800c
#endif
#ifndef PROV_RSA_AES
#define PROV_RSA_AES 24
#endif

#define W2(x) L##x
#define W(x) W2(x)
#define VERSION_W W(ENLANGG_VERSION)

#define MANIFEST_NAME L"install.manifest"
#define UNINSTALL_NAME L"uninstall.exe"
#define REG_APP L"Software\\Enlangg"
#define REG_UNINSTALL L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Enlangg"
#define MANIFEST_URL "https://enlangg.site/version.json"

#define WM_APP_LOG  (WM_APP + 1)
#define WM_APP_DONE (WM_APP + 2)

enum { IDC_DIR = 101, IDC_BROWSE, IDC_PATH, IDC_ASSOC, IDC_STUDIO, IDC_INSTALL, IDC_LOG, IDC_STATUS };

typedef struct {
    wchar_t dir[MAX_PATH];
    int addPath, assoc, studio, silent;
    HWND hwnd;
    FILE *logfile;
} Opts;

static Opts g;
static int g_failed, g_done;

/* ------------------------------------------------------------ logging */

static void say(const wchar_t *fmt, ...) {
    wchar_t buf[2048];
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf(buf, 2047, fmt, ap);
    va_end(ap);
    buf[2047] = 0;
    if (g.hwnd) PostMessageW(g.hwnd, WM_APP_LOG, 0, (LPARAM)_wcsdup(buf));
    if (g.logfile) {
        char u8[4096];
        int n = WideCharToMultiByte(CP_UTF8, 0, buf, -1, u8, sizeof(u8) - 2, NULL, NULL);
        if (n > 0) { u8[n - 1] = '\n'; fwrite(u8, 1, (size_t)n, g.logfile); fflush(g.logfile); }
    }
}

/* ------------------------------------------------------------ path helpers */

static int join(wchar_t *dst, const wchar_t *a, const wchar_t *b) {
    int n = _snwprintf(dst, MAX_PATH, L"%ls\\%ls", a, b);
    if (n < 0 || n >= MAX_PATH) { dst[0] = 0; return 0; }
    dst[MAX_PATH - 1] = 0;
    return 1;
}

static void mkdir_p(const wchar_t *path) {
    wchar_t tmp[MAX_PATH];
    wcsncpy(tmp, path, MAX_PATH - 1);
    tmp[MAX_PATH - 1] = 0;
    for (wchar_t *p = tmp + 3; *p; p++) {
        if (*p == L'\\') { *p = 0; CreateDirectoryW(tmp, NULL); *p = L'\\'; }
    }
    CreateDirectoryW(tmp, NULL);
}

static void strip_trailing_slash(wchar_t *p) {
    size_t n = wcslen(p);
    while (n > 3 && (p[n - 1] == L'\\' || p[n - 1] == L'/')) p[--n] = 0;
}

/* a relative path from the payload or a manifest: strict allowlist */
static int safe_rel(const char *s) {
    if (!*s || s[0] == '/' || strstr(s, "..") || strstr(s, "//")) return 0;
    for (const char *p = s; *p; p++) {
        char c = *p;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '.' || c == '_' || c == '-' || c == '/')) return 0;
    }
    return 1;
}

static int rel_to_full(wchar_t *dst, const wchar_t *dir, const char *rel) {
    wchar_t wrel[MAX_PATH];
    if (!safe_rel(rel)) return 0;
    if (MultiByteToWideChar(CP_UTF8, 0, rel, -1, wrel, MAX_PATH) <= 0) return 0;
    for (wchar_t *p = wrel; *p; p++) if (*p == L'/') *p = L'\\';
    return join(dst, dir, wrel);
}

/* an install dir must be absolute and not a drive root */
static int dir_ok(const wchar_t *d) {
    size_t n = wcslen(d);
    if (n < 4 || n > 200) return 0;
    int drive = ((d[0] >= L'A' && d[0] <= L'Z') || (d[0] >= L'a' && d[0] <= L'z')) && d[1] == L':' && d[2] == L'\\';
    int unc = d[0] == L'\\' && d[1] == L'\\';
    if (!drive && !unc) return 0;
    if (drive && n == 3) return 0;
    if (wcsstr(d, L"..")) return 0;
    return 1;
}

static void default_dir(wchar_t *out) {
    wchar_t base[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, base) == S_OK)
        _snwprintf(out, MAX_PATH, L"%ls\\Programs\\Enlangg", base);
    else
        wcscpy(out, L"C:\\Enlangg");
    out[MAX_PATH - 1] = 0;
}

static char *read_small_file(const wchar_t *path, DWORD *len) {
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return NULL;
    DWORD sz = GetFileSize(h, NULL);
    if (sz == INVALID_FILE_SIZE || sz > (1u << 20)) { CloseHandle(h); return NULL; }
    char *buf = (char *)malloc(sz + 1);
    DWORD got = 0;
    if (!buf || !ReadFile(h, buf, sz, &got, NULL)) { CloseHandle(h); free(buf); return NULL; }
    CloseHandle(h);
    buf[got] = 0;
    if (len) *len = got;
    return buf;
}

/* ------------------------------------------------------------ registry */

static void reg_set(const wchar_t *sub, const wchar_t *name, DWORD type, const void *data, DWORD size) {
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, sub, 0, NULL, 0, KEY_SET_VALUE, NULL, &k, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(k, name, 0, type, (const BYTE *)data, size);
        RegCloseKey(k);
    }
}
static void reg_sz(const wchar_t *sub, const wchar_t *name, const wchar_t *v) {
    reg_set(sub, name, REG_SZ, v, (DWORD)((wcslen(v) + 1) * sizeof(wchar_t)));
}
static void reg_dw(const wchar_t *sub, const wchar_t *name, DWORD v) {
    reg_set(sub, name, REG_DWORD, &v, sizeof(v));
}
static int reg_get_sz(const wchar_t *sub, const wchar_t *name, wchar_t *out, DWORD cch) {
    HKEY k;
    DWORD type = 0, size = cch * sizeof(wchar_t);
    int ok = 0;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, sub, 0, KEY_QUERY_VALUE, &k) == ERROR_SUCCESS) {
        if (RegQueryValueExW(k, name, NULL, &type, (BYTE *)out, &size) == ERROR_SUCCESS &&
            (type == REG_SZ || type == REG_EXPAND_SZ)) { out[cch - 1] = 0; ok = 1; }
        RegCloseKey(k);
    }
    return ok;
}

static void notify_env(void) {
    DWORD_PTR r;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 3000, &r);
}

/* PATH entries compare equal ignoring case and one trailing backslash */
static int same_entry(const wchar_t *a, const wchar_t *b) {
    wchar_t x[MAX_PATH * 2], y[MAX_PATH * 2];
    wchar_t xe[MAX_PATH * 2], ye[MAX_PATH * 2];
    wcsncpy(x, a, MAX_PATH * 2 - 1); x[MAX_PATH * 2 - 1] = 0;
    wcsncpy(y, b, MAX_PATH * 2 - 1); y[MAX_PATH * 2 - 1] = 0;
    ExpandEnvironmentStringsW(x, xe, MAX_PATH * 2);
    ExpandEnvironmentStringsW(y, ye, MAX_PATH * 2);
    size_t n = wcslen(xe); while (n > 0 && xe[n - 1] == L'\\') xe[--n] = 0;
    n = wcslen(ye); while (n > 0 && ye[n - 1] == L'\\') ye[--n] = 0;
    return _wcsicmp(xe, ye) == 0;
}

/* mode 1: put bindir first on the user PATH; mode 0: remove it */
static int user_path_edit(const wchar_t *bindir, int add) {
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &k) != ERROR_SUCCESS) return 0;
    DWORD type = REG_EXPAND_SZ, size = 0;
    wchar_t *old = NULL;
    if (RegQueryValueExW(k, L"Path", NULL, &type, NULL, &size) == ERROR_SUCCESS) {
        old = (wchar_t *)calloc(size / sizeof(wchar_t) + 2, sizeof(wchar_t));
        if (!old || RegQueryValueExW(k, L"Path", NULL, &type, (BYTE *)old, &size) != ERROR_SUCCESS) {
            free(old); RegCloseKey(k); return 0;
        }
        if (type != REG_SZ && type != REG_EXPAND_SZ) { free(old); RegCloseKey(k); return 0; }
    } else {
        type = REG_EXPAND_SZ;
        old = (wchar_t *)calloc(2, sizeof(wchar_t));
    }
    size_t cap = wcslen(old) + wcslen(bindir) + 4;
    wchar_t *res = (wchar_t *)calloc(cap, sizeof(wchar_t));
    if (add) { wcscpy(res, bindir); }
    wchar_t *copy = _wcsdup(old);
    for (wchar_t *tok = wcstok(copy, L";"); tok; tok = wcstok(NULL, L";")) {
        if (!*tok || same_entry(tok, bindir)) continue;
        if (*res) wcscat(res, L";");
        wcscat(res, tok);
    }
    free(copy);
    int ok = RegSetValueExW(k, L"Path", 0, type, (const BYTE *)res, (DWORD)((wcslen(res) + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
    free(res); free(old);
    RegCloseKey(k);
    if (ok) notify_env();
    return ok;
}

/* ------------------------------------------------------------ payload */

typedef struct { const char *path; unsigned short plen; const unsigned char *data; unsigned long size; } Entry;
static Entry entries[64];
static int nentries;

static unsigned long rd32(const unsigned char *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned long)p[3] << 24); }

static int load_payload(void) {
    HRSRC r = FindResourceW(NULL, L"PAYLOAD", RT_RCDATA);
    if (!r) return 0;
    HGLOBAL h = LoadResource(NULL, r);
    const unsigned char *p = (const unsigned char *)LockResource(h), *end = p + SizeofResource(NULL, r);
    if (!p || end - p < 8 || rd32(p) != 0x504C4E45UL) return 0;
    unsigned long count = rd32(p + 4);
    p += 8;
    if (count == 0 || count > 64) return 0;
    for (unsigned long i = 0; i < count; i++) {
        if (end - p < 2) return 0;
        unsigned plen = p[0] | (p[1] << 8);
        p += 2;
        if ((unsigned long)(end - p) < plen + 4UL) return 0;
        char *name = (char *)malloc(plen + 1);
        memcpy(name, p, plen); name[plen] = 0;
        p += plen;
        unsigned long sz = rd32(p);
        p += 4;
        if ((unsigned long)(end - p) < sz) return 0;
        if (!safe_rel(name)) return 0;
        entries[nentries].path = name; entries[nentries].plen = (unsigned short)plen;
        entries[nentries].data = p; entries[nentries].size = sz;
        nentries++;
        p += sz;
    }
    return 1;
}

static int write_atomic(const wchar_t *dest, const void *data, DWORD size) {
    wchar_t tmp[MAX_PATH + 8];
    _snwprintf(tmp, MAX_PATH + 7, L"%ls.new", dest);
    tmp[MAX_PATH + 7] = 0;
    HANDLE h = CreateFileW(tmp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;
    DWORD w = 0;
    int ok = WriteFile(h, data, size, &w, NULL) && w == size;
    CloseHandle(h);
    if (ok) ok = MoveFileExW(tmp, dest, MOVEFILE_REPLACE_EXISTING);
    if (!ok) DeleteFileW(tmp);
    return ok;
}

/* ------------------------------------------------------------ checks */

static int find_gcc(wchar_t *where) {
    wchar_t buf[MAX_PATH];
    DWORD n = SearchPathW(NULL, L"gcc", L".exe", MAX_PATH, buf, NULL);
    if (n > 0 && n < MAX_PATH) { if (where) wcscpy(where, buf); return 1; }
    return 0;
}

/* ------------------------------------------------------------ Studio download */

static int json_str(const char *json, const char *key, char *out, size_t cap) {
    char pat[64];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(json, pat);
    if (!p) return 0;
    p = strchr(p + strlen(pat), ':');
    if (!p) return 0;
    p = strchr(p, '"');
    if (!p) return 0;
    p++;
    size_t n = 0;
    while (*p && *p != '"' && n + 1 < cap) out[n++] = *p++;
    out[n] = 0;
    return *p == '"' && n > 0;
}

static int host_allowed(const char *url) {
    static const char *ok[] = { "github.com", "enlangg.site", "enlangg.vercel.app" };
    if (strncmp(url, "https://", 8) != 0) return 0;
    const char *h = url + 8, *e = h;
    while (*e && *e != '/' && *e != '?' && *e != '#') e++;
    size_t n = (size_t)(e - h);
    for (size_t i = 0; i < sizeof(ok) / sizeof(ok[0]); i++)
        if (strlen(ok[i]) == n && _strnicmp(h, ok[i], n) == 0) return 1;
    return 0;
}

/* GET url; into memory (mem/cap) or into file handle. returns bytes or -1 */
static long long http_get(HINTERNET net, const char *url, char *mem, size_t cap, HANDLE file, size_t limit) {
    HINTERNET u = InternetOpenUrlA(net, url, NULL, 0,
        INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI, 0);
    if (!u) return -1;
    DWORD status = 0, sl = sizeof(status);
    if (!HttpQueryInfoA(u, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &sl, NULL) || status != 200) {
        InternetCloseHandle(u); return -1;
    }
    unsigned char chunk[32768];
    long long total = 0;
    size_t nextReport = 10u << 20;
    for (;;) {
        DWORD got = 0;
        if (!InternetReadFile(u, chunk, sizeof(chunk), &got)) { total = -1; break; }
        if (!got) break;
        if ((size_t)total + got > limit) { total = -1; break; }
        if (file) {
            DWORD w;
            if (!WriteFile(file, chunk, got, &w, NULL) || w != got) { total = -1; break; }
        } else {
            if ((size_t)total + got >= cap) { total = -1; break; }
            memcpy(mem + total, chunk, got);
        }
        total += got;
        if (file && (size_t)total >= nextReport) { say(L"  downloaded %lu MB...", (unsigned long)((size_t)total >> 20)); nextReport += 10u << 20; }
    }
    InternetCloseHandle(u);
    if (!file && total >= 0) mem[total] = 0;
    return total;
}

static int sha256_file(const wchar_t *path, char hex[65]) {
    HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return 0;
    HCRYPTPROV prov = 0; HCRYPTHASH hash = 0;
    int ok = 0;
    if (CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) &&
        CryptCreateHash(prov, CALG_SHA_256, 0, 0, &hash)) {
        unsigned char buf[65536]; DWORD got;
        ok = 1;
        while (ReadFile(f, buf, sizeof(buf), &got, NULL) && got) {
            if (!CryptHashData(hash, buf, got, 0)) { ok = 0; break; }
        }
        unsigned char d[32]; DWORD dl = 32;
        if (ok && CryptGetHashParam(hash, HP_HASHVAL, d, &dl, 0) && dl == 32) {
            for (int i = 0; i < 32; i++) sprintf(hex + i * 2, "%02x", d[i]);
            hex[64] = 0;
        } else ok = 0;
    }
    if (hash) CryptDestroyHash(hash);
    if (prov) CryptReleaseContext(prov, 0);
    CloseHandle(f);
    return ok;
}

static void install_studio(void) {
    say(L"Studio: reading %hs", MANIFEST_URL);
    HINTERNET net = InternetOpenA("EnlanggSetup/" ENLANGG_VERSION, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!net) { say(L"Studio skipped: could not initialise networking."); return; }
    static char json[65536];
    if (http_get(net, MANIFEST_URL, json, sizeof(json), NULL, sizeof(json)) < 0) {
        say(L"Studio skipped: could not reach enlangg.site (you can install Studio later from the website).");
        InternetCloseHandle(net); return;
    }
    char url[512], sha[128];
    if (!json_str(json, "downloadUrl", url, sizeof(url)) || !json_str(json, "sha256", sha, sizeof(sha)) || strlen(sha) != 64) {
        say(L"Studio skipped: update manifest is incomplete.");
        InternetCloseHandle(net); return;
    }
    for (int i = 0; i < 64; i++) {
        char c = sha[i];
        if (c >= 'A' && c <= 'F') sha[i] = (char)(c + 32);
        else if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) { say(L"Studio skipped: bad checksum in manifest."); InternetCloseHandle(net); return; }
    }
    if (!host_allowed(url)) { say(L"Studio skipped: download host is not on the allowlist."); InternetCloseHandle(net); return; }

    wchar_t tmpdir[MAX_PATH], tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpdir);
    _snwprintf(tmp, MAX_PATH, L"%lsEnlanggStudio-Setup-%lu.exe", tmpdir, (unsigned long)GetTickCount());
    tmp[MAX_PATH - 1] = 0;
    HANDLE f = CreateFileW(tmp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) { say(L"Studio skipped: cannot write to the temp folder."); InternetCloseHandle(net); return; }
    say(L"Studio: downloading installer...");
    long long n = http_get(net, url, NULL, 0, f, 400u << 20);
    CloseHandle(f);
    InternetCloseHandle(net);
    if (n <= 0) { DeleteFileW(tmp); say(L"Studio skipped: download failed."); return; }
    char got[65];
    if (!sha256_file(tmp, got) || strcmp(got, sha) != 0) {
        DeleteFileW(tmp);
        say(L"Studio NOT installed: checksum mismatch. The downloaded file was deleted.");
        g_failed = 1;
        return;
    }
    say(L"Studio: checksum verified (%hs). Launching its installer...", sha);
    if ((INT_PTR)ShellExecuteW(NULL, L"open", tmp, NULL, NULL, SW_SHOWNORMAL) <= 32)
        say(L"Studio: could not start the installer. It was saved to %ls", tmp);
}

/* ------------------------------------------------------------ install */

static int studio_exe(wchar_t *out) {
    wchar_t base[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, base) != S_OK) return 0;
    _snwprintf(out, MAX_PATH, L"%ls\\Programs\\Enlangg Studio\\Enlangg Studio.exe", base);
    out[MAX_PATH - 1] = 0;
    return GetFileAttributesW(out) != INVALID_FILE_ATTRIBUTES;
}

static void register_assoc(const wchar_t *dir) {
    wchar_t prev[256] = L"", cur[256], studio[MAX_PATH], cmd[MAX_PATH * 2];
    if (reg_get_sz(L"Software\\Classes\\.enlng", NULL, cur, 256) && wcscmp(cur, L"Enlangg.Source") != 0)
        reg_sz(REG_APP, L"PrevAssoc", cur);
    (void)prev;
    reg_sz(L"Software\\Classes\\.enlng", NULL, L"Enlangg.Source");
    reg_sz(L"Software\\Classes\\Enlangg.Source", NULL, L"Enlangg source file");
    _snwprintf(cmd, MAX_PATH * 2, L"\"%ls\\bin\\enlangg.exe\",0", dir);
    reg_sz(L"Software\\Classes\\Enlangg.Source\\DefaultIcon", NULL, cmd);
    reg_sz(L"Software\\Classes\\Enlangg.Source\\shell", NULL, L"open");
    /* double-click opens the file for editing; it never runs it */
    if (studio_exe(studio)) _snwprintf(cmd, MAX_PATH * 2, L"\"%ls\" \"%%1\"", studio);
    else wcscpy(cmd, L"notepad.exe \"%1\"");
    reg_sz(L"Software\\Classes\\Enlangg.Source\\shell\\open\\command", NULL, cmd);
    reg_sz(L"Software\\Classes\\Enlangg.Source\\shell\\run", NULL, L"Run with Enlangg");
    _snwprintf(cmd, MAX_PATH * 2, L"cmd.exe /k \"\"%ls\\bin\\enlangg.exe\" run \"%%1\"\"", dir);
    reg_sz(L"Software\\Classes\\Enlangg.Source\\shell\\run\\command", NULL, cmd);
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

static void unregister_assoc(void) {
    wchar_t cur[256], prev[256];
    if (reg_get_sz(L"Software\\Classes\\.enlng", NULL, cur, 256) && wcscmp(cur, L"Enlangg.Source") == 0) {
        if (reg_get_sz(REG_APP, L"PrevAssoc", prev, 256)) reg_sz(L"Software\\Classes\\.enlng", NULL, prev);
        else SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\.enlng");
    }
    SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\Enlangg.Source");
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

static int do_install(void) {
    wchar_t dir[MAX_PATH], bindir[MAX_PATH], full[MAX_PATH], mf[MAX_PATH], self[MAX_PATH], gccpath[MAX_PATH];
    wcscpy(dir, g.dir);
    strip_trailing_slash(dir);
    if (!dir_ok(dir)) { say(L"ERROR: '%ls' is not a valid install folder (use a full path such as C:\\Users\\you\\Enlangg).", dir); return 0; }
    if (!load_payload()) { say(L"ERROR: installer payload is missing or damaged. Re-download enlangg-setup.exe."); return 0; }

    say(L"Installing Enlangg %ls to %ls", VERSION_W, dir);
    mkdir_p(dir);
    if (GetFileAttributesW(dir) == INVALID_FILE_ATTRIBUTES) { say(L"ERROR: cannot create %ls", dir); return 0; }

    /* remember the previous install's files so stale ones can be removed */
    join(mf, dir, MANIFEST_NAME);
    char *oldman = read_small_file(mf, NULL);

    char *newman = (char *)malloc(8192);
    size_t nm = 0;
    for (int i = 0; i < nentries; i++) {
        if (!rel_to_full(full, dir, entries[i].path)) { say(L"ERROR: bad path in payload"); return 0; }
        wchar_t parent[MAX_PATH];
        wcscpy(parent, full);
        wchar_t *sl = wcsrchr(parent, L'\\');
        if (sl) { *sl = 0; mkdir_p(parent); }
        if (!write_atomic(full, entries[i].data, entries[i].size)) {
            say(L"ERROR: could not write %ls (is an Enlangg tool still running? close it and retry)", full);
            return 0;
        }
        nm += (size_t)sprintf(newman + nm, "%s\n", entries[i].path);
    }

    /* the uninstaller is this same exe */
    GetModuleFileNameW(NULL, self, MAX_PATH);
    join(full, dir, UNINSTALL_NAME);
    if (_wcsicmp(self, full) != 0 && !CopyFileW(self, full, FALSE)) say(L"warning: could not create uninstall.exe");
    nm += (size_t)sprintf(newman + nm, "uninstall.exe\n");
    if (!write_atomic(mf, newman, (DWORD)nm)) { say(L"ERROR: could not write the install manifest"); return 0; }

    if (oldman) {
        for (char *line = strtok(oldman, "\r\n"); line; line = strtok(NULL, "\r\n")) {
            int keep = 0;
            for (int i = 0; i < nentries; i++) if (strcmp(entries[i].path, line) == 0) keep = 1;
            if (!keep && strcmp(line, "uninstall.exe") != 0 && rel_to_full(full, dir, line)) DeleteFileW(full);
        }
        free(oldman);
    }
    free(newman);
    say(L"Copied %d files.", nentries);

    reg_sz(REG_APP, L"InstallDir", dir);
    reg_sz(REG_APP, L"Version", VERSION_W);
    join(bindir, dir, L"bin");
    if (g.addPath) {
        if (user_path_edit(bindir, 1)) { say(L"Added %ls to your user PATH. Open a NEW terminal to use 'enlangg'.", bindir); reg_dw(REG_APP, L"PathAdded", 1); }
        else say(L"warning: could not update PATH. Add %ls manually.", bindir);
    }
    if (g.assoc) { register_assoc(dir); say(L".enlng files: double-click opens them for editing; right-click > Run with Enlangg runs them."); reg_dw(REG_APP, L"AssocAdded", 1); }

    join(full, dir, UNINSTALL_NAME);
    wchar_t unstr[MAX_PATH + 32];
    _snwprintf(unstr, MAX_PATH + 31, L"\"%ls\" /uninstall", full);
    reg_sz(REG_UNINSTALL, L"DisplayName", L"Enlangg");
    reg_sz(REG_UNINSTALL, L"DisplayVersion", VERSION_W);
    reg_sz(REG_UNINSTALL, L"Publisher", L"Enlangg");
    reg_sz(REG_UNINSTALL, L"InstallLocation", dir);
    reg_sz(REG_UNINSTALL, L"UninstallString", unstr);
    reg_dw(REG_UNINSTALL, L"NoModify", 1);
    reg_dw(REG_UNINSTALL, L"NoRepair", 1);
    reg_dw(REG_UNINSTALL, L"EstimatedSize", 3072);

    if (find_gcc(gccpath)) say(L"gcc found: %ls", gccpath);
    else say(L"NOTE: gcc was not found on your PATH. 'enlng' compiles programs with gcc, so install a gcc (for example MinGW-w64 or MSYS2) and make sure 'gcc' works in a new terminal.");

    wchar_t other[MAX_PATH];
    if (SearchPathW(NULL, L"enlangg", L".exe", MAX_PATH, other, NULL)) {
        wchar_t mine[MAX_PATH];
        join(mine, bindir, L"enlangg.exe");
        if (_wcsicmp(other, mine) != 0) say(g.addPath ? L"NOTE: another enlangg.exe is on your PATH (%ls). The new install is placed first, but remove old copies to avoid confusion." : L"NOTE: another enlangg.exe is on your PATH (%ls) and may run instead of this install.", other);
    }

    if (g.studio) install_studio();
    return 1;
}

/* ------------------------------------------------------------ uninstall */

static int do_uninstall_run(const wchar_t *dirArg) {
    wchar_t dir[MAX_PATH], mf[MAX_PATH], full[MAX_PATH], reg[MAX_PATH];
    wcscpy(dir, dirArg);
    strip_trailing_slash(dir);
    join(mf, dir, MANIFEST_NAME);
    char *man = read_small_file(mf, NULL);
    if (!dir_ok(dir) || !man || !reg_get_sz(REG_APP, L"InstallDir", reg, MAX_PATH) || _wcsicmp(reg, dir) != 0) {
        if (!g.silent) MessageBoxW(NULL, L"This does not look like a valid Enlangg install. Nothing was removed.", L"Enlangg Uninstall", MB_ICONERROR);
        free(man);
        return 1;
    }
    if (!g.silent && MessageBoxW(NULL, L"Remove Enlangg from this computer?\n\nYour own .enlng programs are not touched.", L"Enlangg Uninstall", MB_YESNO | MB_ICONQUESTION) != IDYES) {
        free(man); return 0;
    }
    int leftovers = 0;
    wchar_t dirs[64][MAX_PATH];
    int nd = 0;
    for (char *line = strtok(man, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        if (!rel_to_full(full, dir, line)) continue;
        if (!DeleteFileW(full) && GetLastError() != ERROR_FILE_NOT_FOUND) leftovers++;
        wchar_t *sl = wcsrchr(full, L'\\');
        if (sl && nd < 64) {
            *sl = 0;
            int seen = 0;
            for (int i = 0; i < nd; i++) if (_wcsicmp(dirs[i], full) == 0) seen = 1;
            if (!seen) wcscpy(dirs[nd++], full);
        }
    }
    free(man);
    DeleteFileW(mf);
    for (int i = 0; i < nd; i++)     /* deepest first; non-empty dirs are left alone */
        for (int j = i + 1; j < nd; j++)
            if (wcslen(dirs[j]) > wcslen(dirs[i])) { wchar_t t[MAX_PATH]; wcscpy(t, dirs[i]); wcscpy(dirs[i], dirs[j]); wcscpy(dirs[j], t); }
    for (int i = 0; i < nd; i++) if (_wcsicmp(dirs[i], dir) != 0) RemoveDirectoryW(dirs[i]);
    RemoveDirectoryW(dir);

    join(full, dir, L"bin");
    user_path_edit(full, 0);
    unregister_assoc();
    SHDeleteKeyW(HKEY_CURRENT_USER, REG_UNINSTALL);
    SHDeleteKeyW(HKEY_CURRENT_USER, REG_APP);

    /* this process is a temp copy of uninstall.exe: delete it after we exit */
    wchar_t self[MAX_PATH], cmd[MAX_PATH * 2];
    GetModuleFileNameW(NULL, self, MAX_PATH);
    _snwprintf(cmd, MAX_PATH * 2, L"cmd.exe /c \"ping -n 3 127.0.0.1 >nul & del /f /q \"%ls\"\"", self);
    STARTUPINFOW si = { sizeof(si) }; PROCESS_INFORMATION pi;
    if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) { CloseHandle(pi.hProcess); CloseHandle(pi.hThread); }

    if (!g.silent) {
        if (leftovers) MessageBoxW(NULL, L"Enlangg was removed, but some files were in use and remain. Delete the folder manually after closing Enlangg tools.", L"Enlangg Uninstall", MB_ICONWARNING);
        else MessageBoxW(NULL, L"Enlangg was removed.", L"Enlangg Uninstall", MB_ICONINFORMATION);
    }
    return 0;
}

static int do_uninstall_start(void) {
    wchar_t self[MAX_PATH], dir[MAX_PATH], tmpd[MAX_PATH], tmp[MAX_PATH], cmd[MAX_PATH * 3];
    GetModuleFileNameW(NULL, self, MAX_PATH);
    wcscpy(dir, self);
    wchar_t *sl = wcsrchr(dir, L'\\');
    if (!sl) return 1;
    *sl = 0;
    GetTempPathW(MAX_PATH, tmpd);
    _snwprintf(tmp, MAX_PATH, L"%lsenlangg-uninst-%lu.exe", tmpd, (unsigned long)GetCurrentProcessId());
    tmp[MAX_PATH - 1] = 0;
    if (!CopyFileW(self, tmp, FALSE)) return 1;
    _snwprintf(cmd, MAX_PATH * 3, L"\"%ls\" /uninstall-run \"%ls\"%ls", tmp, dir, g.silent ? L" /S" : L"");
    STARTUPINFOW si = { sizeof(si) }; PROCESS_INFORMATION pi;
    if (!CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) return 1;
    if (g.silent) WaitForSingleObject(pi.hProcess, 60000);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    return 0;
}

/* ------------------------------------------------------------ GUI */

static HFONT g_font;
static int dpi_scale(int v) { HDC dc = GetDC(NULL); int d = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(NULL, dc); return MulDiv(v, d, 96); }
#define S(v) dpi_scale(v)

static HWND mk(HWND parent, const wchar_t *cls, const wchar_t *text, DWORD style, int x, int y, int w, int h, int id) {
    HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, S(x), S(y), S(w), S(h), parent, (HMENU)(INT_PTR)id, NULL, NULL);
    SendMessageW(c, WM_SETFONT, (WPARAM)g_font, TRUE);
    return c;
}

static DWORD WINAPI worker(LPVOID p) {
    (void)p;
    int ok = do_install() && !g_failed;
    PostMessageW(g.hwnd, WM_APP_DONE, ok, 0);
    return 0;
}

static void append_log(HWND h, const wchar_t *t) {
    HWND log = GetDlgItem(h, IDC_LOG);
    int len = GetWindowTextLengthW(log);
    SendMessageW(log, EM_SETSEL, len, len);
    SendMessageW(log, EM_REPLACESEL, FALSE, (LPARAM)t);
    SendMessageW(log, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
}

static LRESULT CALLBACK wndproc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        wchar_t gccpath[MAX_PATH], title[128];
        g.hwnd = h;
        _snwprintf(title, 127, L"Enlangg %ls", VERSION_W);
        HWND t = mk(h, L"STATIC", title, 0, 20, 14, 400, 26, 0);
        HFONT big = CreateFontW(-S(20), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        SendMessageW(t, WM_SETFONT, (WPARAM)big, TRUE);
        mk(h, L"STATIC", L"Installs the enlng compiler and its tools for your user account only. No administrator rights needed.", 0, 20, 44, 500, 34, 0);
        mk(h, L"STATIC", L"Install folder", 0, 20, 86, 200, 18, 0);
        mk(h, L"EDIT", g.dir, WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 20, 106, 400, 24, IDC_DIR);
        mk(h, L"BUTTON", L"Browse...", WS_TABSTOP, 430, 105, 90, 26, IDC_BROWSE);
        HWND c1 = mk(h, L"BUTTON", L"Add Enlangg to my PATH", BS_AUTOCHECKBOX | WS_TABSTOP, 20, 142, 500, 22, IDC_PATH);
        HWND c2 = mk(h, L"BUTTON", L"Associate .enlng files (double-click edits, right-click > Run)", BS_AUTOCHECKBOX | WS_TABSTOP, 20, 166, 500, 22, IDC_ASSOC);
        HWND c3 = mk(h, L"BUTTON", L"Also download Enlangg Studio (the IDE, checksum-verified)", BS_AUTOCHECKBOX | WS_TABSTOP, 20, 190, 500, 22, IDC_STUDIO);
        SendMessageW(c1, BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(c2, BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(c3, BM_SETCHECK, BST_CHECKED, 0);
        mk(h, L"STATIC", find_gcc(gccpath) ? L"gcc: found on PATH." : L"gcc: NOT found. Enlangg compiles with gcc, so install MinGW-w64 or MSYS2 gcc separately (Setup will remind you).", 0, 20, 220, 500, 34, IDC_STATUS);
        mk(h, L"EDIT", L"", WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 20, 262, 500, 140, IDC_LOG);
        mk(h, L"BUTTON", L"Install", BS_DEFPUSHBUTTON | WS_TABSTOP, 420, 414, 100, 30, IDC_INSTALL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDC_BROWSE) {
            BROWSEINFOW bi = { 0 };
            bi.hwndOwner = h; bi.lpszTitle = L"Choose the install folder"; bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS;
            LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
            if (pidl) {
                wchar_t p[MAX_PATH], q[MAX_PATH];
                if (SHGetPathFromIDListW(pidl, p)) {
                    strip_trailing_slash(p);
                    /* always install into a dedicated subfolder, never into a shared folder */
                    _snwprintf(q, MAX_PATH, L"%ls\\Enlangg", p); q[MAX_PATH - 1] = 0;
                    SetDlgItemTextW(h, IDC_DIR, q);
                }
                CoTaskMemFree(pidl);
            }
        } else if (LOWORD(w) == IDC_INSTALL) {
            if (g_done) { DestroyWindow(h); return 0; }
            GetDlgItemTextW(h, IDC_DIR, g.dir, MAX_PATH);
            g.addPath = IsDlgButtonChecked(h, IDC_PATH) == BST_CHECKED;
            g.assoc = IsDlgButtonChecked(h, IDC_ASSOC) == BST_CHECKED;
            g.studio = IsDlgButtonChecked(h, IDC_STUDIO) == BST_CHECKED;
            for (int id = IDC_DIR; id <= IDC_STUDIO; id++) EnableWindow(GetDlgItem(h, id), FALSE);
            EnableWindow(GetDlgItem(h, IDC_INSTALL), FALSE);
            SetDlgItemTextW(h, IDC_INSTALL, L"Installing...");
            HANDLE th = CreateThread(NULL, 0, worker, NULL, 0, NULL);
            if (th) CloseHandle(th);
        }
        return 0;
    case WM_APP_LOG: { wchar_t *t = (wchar_t *)l; append_log(h, t); free(t); return 0; }
    case WM_APP_DONE:
        append_log(h, w ? L"\r\nDone. You can close this window." : L"\r\nSetup did not complete. See the messages above.");
        g_done = 1;
        SetDlgItemTextW(h, IDC_INSTALL, L"Close");
        EnableWindow(GetDlgItem(h, IDC_INSTALL), TRUE);
        return 0;
    case WM_CLOSE: DestroyWindow(h); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static int run_gui(HINSTANCE inst) {
    INITCOMMONCONTROLSEX ic = { sizeof(ic), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&ic);
    CoInitialize(NULL);
    g_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = wndproc; wc.hInstance = inst; wc.lpszClassName = L"EnlanggSetup";
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    RegisterClassW(&wc);
    RECT r = { 0, 0, S(540), S(460) };
    AdjustWindowRect(&r, WS_CAPTION | WS_SYSMENU, FALSE);
    HWND w = CreateWindowExW(0, L"EnlanggSetup", L"Enlangg Setup", WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, inst, NULL);
    if (!w) return 1;
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(w, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    }
    return 0;
}

/* ------------------------------------------------------------ entry */

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show) {
    (void)prev; (void)cmdline; (void)show;
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    int uninstall = 0, uninstallRun = 0;
    wchar_t runDir[MAX_PATH] = L"";
    default_dir(g.dir);
    g.addPath = 1; g.assoc = 1; g.studio = 0;
    for (int i = 1; i < argc; i++) {
        const wchar_t *a = argv[i];
        if (!_wcsicmp(a, L"/S")) g.silent = 1;
        else if (!_wcsicmp(a, L"/uninstall")) uninstall = 1;
        else if (!_wcsicmp(a, L"/uninstall-run") && i + 1 < argc) { uninstallRun = 1; wcsncpy(runDir, argv[++i], MAX_PATH - 1); }
        else if (!_wcsicmp(a, L"/NOPATH")) g.addPath = 0;
        else if (!_wcsicmp(a, L"/NOASSOC")) g.assoc = 0;
        else if (!_wcsicmp(a, L"/STUDIO")) g.studio = 1;
        else if (!_wcsnicmp(a, L"/DIR=", 5)) { wcsncpy(g.dir, a + 5, MAX_PATH - 1); g.dir[MAX_PATH - 1] = 0; }
        else if (!_wcsnicmp(a, L"/LOG=", 5)) g.logfile = _wfopen(a + 5, L"wb");
    }
    if (uninstallRun) return do_uninstall_run(runDir);
    if (uninstall) return do_uninstall_start();
    if (g.silent) {
        CoInitialize(NULL);
        int ok = do_install() && !g_failed;
        if (g.logfile) fclose(g.logfile);
        return ok ? 0 : 1;
    }
    return run_gui(inst);
}
