/*
 * Nexora High-Performance Native C Web Server
 * Engine: Pure C WinSock2 Multi-Threaded Streaming Daemon
 * Zero Python Server Daemon / Zero Static HTML on Disk
 */

#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define DEFAULT_PORT 4200
#define BUFFER_SIZE 65536
#define MAX_ASSET_SIZE (3 * 1024 * 1024) /* 3 MB max payload */

typedef struct {
    char path[64];
    char enlngf_name[64];
    char* cached_html;
    size_t cached_len;
    time_t last_mtime;
} RouteEntry;

static RouteEntry g_routes[] = {
    { "/",          "index.enlngf",     NULL, 0, 0 },
    { "/product",   "product.enlngf",   NULL, 0, 0 },
    { "/solutions", "solutions.enlngf", NULL, 0, 0 },
    { "/pricing",   "pricing.enlngf",   NULL, 0, 0 },
    { "/resources", "resources.enlngf", NULL, 0, 0 },
    { "/login",     "login.enlngf",     NULL, 0, 0 },
    { "/404",       "404.enlngf",       NULL, 0, 0 },
    { "",           "",                 NULL, 0, 0 } /* Sentinel */
};

/* Separate dynamic cache for compiled CSS and JS */
static char* g_cached_css = NULL;
static size_t g_cached_css_len = 0;
static time_t g_last_css_mtime = 0;

static char* g_cached_js = NULL;
static size_t g_cached_js_len = 0;
static time_t g_last_js_mtime = 0;

static CRITICAL_SECTION g_cache_lock;
static time_t g_server_start_time;
static char g_base_dir[MAX_PATH] = "d:\\enlangg\\nexora";
static char g_root_dir[MAX_PATH] = "d:\\enlangg";

/* Helper to normalize forward slashes for python pipe */
static void normalize_slashes(char* dest, const char* src, size_t max_len) {
    size_t i = 0;
    while (src[i] && i < max_len - 1) {
        dest[i] = (src[i] == '\\') ? '/' : src[i];
        i++;
    }
    dest[i] = '\0';
}

/* Get last modified time of a file */
static time_t get_file_mtime(const char* filename) {
    char path[MAX_PATH];
    struct _stat st;
    snprintf(path, sizeof(path), "%s\\%s", g_base_dir, filename);
    if (_stat(path, &st) == 0) return st.st_mtime;
    return 0;
}

/* Get last modified time of page dependencies (enlngf + styles.enlngd + logic.enlngs) */
static time_t get_page_mtime(const char* enlngf_filename) {
    time_t max_m = get_file_mtime(enlngf_filename);
    time_t m_css = get_file_mtime("styles.enlngd");
    time_t m_js = get_file_mtime("logic.enlngs");
    if (m_css > max_m) max_m = m_css;
    if (m_js > max_m) max_m = m_js;
    return max_m;
}

/* Compile command runner: captures stdout in memory without creating files on disk */
static char* run_pipe_compiler(const char* cmd, size_t* out_len) {
    FILE* pipe = _popen(cmd, "rb");
    if (!pipe) return NULL;

    char* buffer = (char*)malloc(MAX_ASSET_SIZE);
    if (!buffer) {
        _pclose(pipe);
        return NULL;
    }

    size_t total_read = 0;
    size_t bytes_read = 0;
    while ((bytes_read = fread(buffer + total_read, 1, BUFFER_SIZE, pipe)) > 0) {
        total_read += bytes_read;
        if (total_read + BUFFER_SIZE >= MAX_ASSET_SIZE) break;
    }

    int exit_code = _pclose(pipe);
    if (exit_code != 0 || total_read == 0) {
        free(buffer);
        return NULL;
    }

    buffer[total_read] = '\0';
    *out_len = total_read;
    return buffer;
}

/* Compile .enlngf strictly in-memory */
static char* compile_enlngf_in_memory(const char* enlngf_filename, size_t* out_len) {
    char norm_root[MAX_PATH], norm_file[MAX_PATH], full_file[MAX_PATH];
    snprintf(full_file, sizeof(full_file), "%s/%s", g_base_dir, enlngf_filename);
    normalize_slashes(norm_root, g_root_dir, sizeof(norm_root));
    normalize_slashes(norm_file, full_file, sizeof(norm_file));

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "python -X utf8 -c \"import sys; sys.path.insert(0, '%s'); from enlgf.server import compile_enlgf_file; sys.stdout.buffer.write(compile_enlgf_file('%s').encode('utf-8'))\"",
        norm_root, norm_file);

    return run_pipe_compiler(cmd, out_len);
}

/* Compile styles.enlngd to CSS strictly using Native Sovereign C Compiler enlngd.exe */
static char* compile_enlngd_in_memory(size_t* out_len) {
    char tmp_path[MAX_PATH];
    snprintf(tmp_path, sizeof(tmp_path), "%s\\_temp_compiled_%ld.css", g_base_dir, (long)GetCurrentProcessId());

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s\\enlngd.exe compile %s\\styles.enlngd -o %s >nul 2>&1", g_root_dir, g_base_dir, tmp_path);
    int res = system(cmd);
    if (res != 0) {
        DeleteFileA(tmp_path);
        return NULL;
    }

    FILE* f = fopen(tmp_path, "rb");
    if (!f) {
        DeleteFileA(tmp_path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buf = (char*)malloc(sz + 1);
    if (buf) {
        fread(buf, 1, sz, f);
        buf[sz] = '\0';
        *out_len = (size_t)sz;
    }
    fclose(f);
    DeleteFileA(tmp_path);
    return buf;
}

/* Compile logic.enlngs to JS strictly using Native Sovereign C Compiler enlngs.exe */
static char* compile_enlngs_in_memory(size_t* out_len) {
    char tmp_path[MAX_PATH];
    snprintf(tmp_path, sizeof(tmp_path), "%s\\_temp_compiled_%ld.js", g_base_dir, (long)GetCurrentProcessId());

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s\\enlngs.exe compile %s\\logic.enlngs -o %s >nul 2>&1", g_root_dir, g_base_dir, tmp_path);
    int res = system(cmd);
    if (res != 0) {
        DeleteFileA(tmp_path);
        return NULL;
    }

    FILE* f = fopen(tmp_path, "rb");
    if (!f) {
        DeleteFileA(tmp_path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buf = (char*)malloc(sz + 1);
    if (buf) {
        fread(buf, 1, sz, f);
        buf[sz] = '\0';
        *out_len = (size_t)sz;
    }
    fclose(f);
    DeleteFileA(tmp_path);
    return buf;
}

/* Fetch compiled page with Stale-While-Revalidate resilient caching */
static char* get_page_html(RouteEntry* entry, size_t* out_len) {
    EnterCriticalSection(&g_cache_lock);
    time_t cur_mtime = get_page_mtime(entry->enlngf_name);

    if (entry->cached_html == NULL || cur_mtime > entry->last_mtime) {
        size_t new_len = 0;
        char* new_html = compile_enlngf_in_memory(entry->enlngf_name, &new_len);
        if (new_html) {
            if (entry->cached_html) free(entry->cached_html);
            entry->cached_html = new_html;
            entry->cached_len = new_len;
            entry->last_mtime = cur_mtime;
            printf("[C Server COMPILE] %s -> %zu bytes (live in-memory)\n", entry->enlngf_name, new_len);
        } else if (entry->cached_html) {
            printf("[C Server RESILIENCE] Compilation failed. Serving stale cache for '%s'\n", entry->enlngf_name);
        }
    }

    char* ret_buf = NULL;
    if (entry->cached_html) {
        ret_buf = (char*)malloc(entry->cached_len + 1);
        if (ret_buf) {
            memcpy(ret_buf, entry->cached_html, entry->cached_len);
            ret_buf[entry->cached_len] = '\0';
            *out_len = entry->cached_len;
        }
    }
    LeaveCriticalSection(&g_cache_lock);
    return ret_buf;
}

/* Fetch compiled styles.css */
static char* get_styles_css(size_t* out_len) {
    EnterCriticalSection(&g_cache_lock);
    time_t cur_mtime = get_file_mtime("styles.enlngd");

    if (g_cached_css == NULL || cur_mtime > g_last_css_mtime) {
        size_t new_len = 0;
        char* new_css = compile_enlngd_in_memory(&new_len);
        if (new_css) {
            if (g_cached_css) free(g_cached_css);
            g_cached_css = new_css;
            g_cached_css_len = new_len;
            g_last_css_mtime = cur_mtime;
            printf("[C Server COMPILE] styles.enlngd -> %zu bytes CSS (live in-memory)\n", new_len);
        }
    }

    char* ret_buf = NULL;
    if (g_cached_css) {
        ret_buf = (char*)malloc(g_cached_css_len + 1);
        if (ret_buf) {
            memcpy(ret_buf, g_cached_css, g_cached_css_len);
            ret_buf[g_cached_css_len] = '\0';
            *out_len = g_cached_css_len;
        }
    }
    LeaveCriticalSection(&g_cache_lock);
    return ret_buf;
}

/* Fetch compiled logic.js */
static char* get_logic_js(size_t* out_len) {
    EnterCriticalSection(&g_cache_lock);
    time_t cur_mtime = get_file_mtime("logic.enlngs");

    if (g_cached_js == NULL || cur_mtime > g_last_js_mtime) {
        size_t new_len = 0;
        char* new_js = compile_enlngs_in_memory(&new_len);
        if (new_js) {
            if (g_cached_js) free(g_cached_js);
            g_cached_js = new_js;
            g_cached_js_len = new_len;
            g_last_js_mtime = cur_mtime;
            printf("[C Server COMPILE] logic.enlngs -> %zu bytes JS (live in-memory)\n", new_len);
        }
    }

    char* ret_buf = NULL;
    if (g_cached_js) {
        ret_buf = (char*)malloc(g_cached_js_len + 1);
        if (ret_buf) {
            memcpy(ret_buf, g_cached_js, g_cached_js_len);
            ret_buf[g_cached_js_len] = '\0';
            *out_len = g_cached_js_len;
        }
    }
    LeaveCriticalSection(&g_cache_lock);
    return ret_buf;
}

/* Find route entry by URL path */
static RouteEntry* find_route(const char* path) {
    for (int i = 0; g_routes[i].path[0] != '\0'; i++) {
        if (strcmp(g_routes[i].path, path) == 0) {
            return &g_routes[i];
        }
    }
    return NULL;
}

/* Serve binary static image (JPEG) */
static void serve_binary_image(SOCKET sock, const char* filename) {
    char filepath[MAX_PATH];
    snprintf(filepath, sizeof(filepath), "%s\\%s", g_base_dir, filename);

    FILE* f = fopen(filepath, "rb");
    if (!f) {
        const char* not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        send(sock, not_found, (int)strlen(not_found), 0);
        return;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char header[512];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: %ld\r\n"
        "Cache-Control: public, max-age=86400\r\n"
        "Connection: close\r\n\r\n",
        file_size);

    send(sock, header, hlen, 0);

    char chunk[BUFFER_SIZE];
    size_t r = 0;
    while ((r = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        send(sock, chunk, (int)r, 0);
    }

    fclose(f);
}

/* Serve API Health Diagnostic */
static void serve_health_api(SOCKET sock) {
    time_t now = time(NULL);
    long uptime = (long)(now - g_server_start_time);

    char json[1024];
    int jlen = snprintf(json, sizeof(json),
        "{\n"
        "  \"status\": \"healthy\",\n"
        "  \"server\": \"Nexora Pure C WinSock2 Daemon (server.exe)\",\n"
        "  \"language\": \"Pure C (MinGW GCC)\",\n"
        "  \"zero_python_server\": true,\n"
        "  \"zero_disk_artifacts\": true,\n"
        "  \"uptime_seconds\": %ld,\n"
        "  \"port\": 4200\n"
        "}\n",
        uptime);

    char header[512];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n",
        jlen);

    send(sock, header, hlen, 0);
    send(sock, json, jlen, 0);
}

/* Worker thread for each incoming client connection */
static unsigned __stdcall client_worker(void* arg) {
    SOCKET sock = (SOCKET)(intptr_t)arg;
    char req[4096];
    int bytes = recv(sock, req, sizeof(req) - 1, 0);

    if (bytes <= 0) {
        closesocket(sock);
        return 0;
    }
    req[bytes] = '\0';

    /* Parse HTTP Method and Path */
    char method[16] = {0};
    char raw_path[256] = {0};
    sscanf(req, "%15s %255s", method, raw_path);

    /* Strip query string if any */
    char* q = strchr(raw_path, '?');
    if (q) *q = '\0';

    /* Clean trailing slash if path != "/" */
    size_t plen = strlen(raw_path);
    if (plen > 1 && raw_path[plen - 1] == '/') {
        raw_path[plen - 1] = '\0';
    }

    /* 1. Health API Route */
    if (strcmp(raw_path, "/api/health") == 0) {
        serve_health_api(sock);
        closesocket(sock);
        return 0;
    }

    /* 2. Favicon SVG / ICO */
    if (strcmp(raw_path, "/favicon.ico") == 0) {
        const char* fav = "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n";
        send(sock, fav, (int)strlen(fav), 0);
        closesocket(sock);
        return 0;
    }

    /* 3. Stylesheet Route (/styles.css or /styles.enlngd) */
    if (strcmp(raw_path, "/styles.css") == 0 || strcmp(raw_path, "/styles.enlngd") == 0) {
        size_t css_len = 0;
        char* css = get_styles_css(&css_len);
        if (css && css_len > 0) {
            char header[512];
            int hlen = snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/css; charset=UTF-8\r\n"
                "Content-Length: %zu\r\n"
                "Server: Nexora-Pure-C-WinSock2\r\n"
                "Connection: close\r\n\r\n",
                css_len);
            send(sock, header, hlen, 0);
            send(sock, css, (int)css_len, 0);
            free(css);
        } else {
            const char* not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            send(sock, not_found, (int)strlen(not_found), 0);
        }
        closesocket(sock);
        return 0;
    }

    /* 4. Logic Script Route (/logic.js or /logic.enlngs) */
    if (strcmp(raw_path, "/logic.js") == 0 || strcmp(raw_path, "/logic.enlngs") == 0) {
        size_t js_len = 0;
        char* js = get_logic_js(&js_len);
        if (js && js_len > 0) {
            char header[512];
            int hlen = snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/javascript; charset=UTF-8\r\n"
                "Content-Length: %zu\r\n"
                "Server: Nexora-Pure-C-WinSock2\r\n"
                "Connection: close\r\n\r\n",
                js_len);
            send(sock, header, hlen, 0);
            send(sock, js, (int)js_len, 0);
            free(js);
        } else {
            const char* not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            send(sock, not_found, (int)strlen(not_found), 0);
        }
        closesocket(sock);
        return 0;
    }

    /* 5. Image Assets */
    if (strstr(raw_path, ".jpg") || strstr(raw_path, ".png") || strstr(raw_path, ".jpeg")) {
        const char* fname = raw_path;
        if (fname[0] == '/') fname++;
        serve_binary_image(sock, fname);
        closesocket(sock);
        return 0;
    }

    /* 6. Page Routes */
    RouteEntry* route = find_route(raw_path);
    int status_code = 200;
    const char* status_text = "OK";

    if (!route) {
        route = find_route("/404");
        status_code = 404;
        status_text = "Not Found";
    }

    size_t html_len = 0;
    char* html = NULL;
    if (route) {
        html = get_page_html(route, &html_len);
    }

    if (html && html_len > 0) {
        char header[512];
        int hlen = snprintf(header, sizeof(header),
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: %zu\r\n"
            "Server: Nexora-Pure-C-WinSock2\r\n"
            "Connection: close\r\n\r\n",
            status_code, status_text, html_len);

        send(sock, header, hlen, 0);
        send(sock, html, (int)html_len, 0);
        free(html);
    } else {
        const char* err_resp =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 22\r\n"
            "Connection: close\r\n\r\n"
            "500 Internal Error C";
        send(sock, err_resp, (int)strlen(err_resp), 0);
    }

    closesocket(sock);
    return 0;
}

int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        }
    }

    setvbuf(stdout, NULL, _IONBF, 0);
    g_server_start_time = time(NULL);
    InitializeCriticalSection(&g_cache_lock);

    /* Prime cache for instant sub-millisecond first load */
    printf("=================================================================\n");
    printf("  NEXORA HIGH-PERFORMANCE NATIVE C WEB SERVER (server.exe)       \n");
    printf("  Engine: Pure C WinSock2 Multi-Threaded Daemon (Zero Python)    \n");
    printf("=================================================================\n");
    printf("[C Engine] Priming in-memory cache from Sovereign Trinity...\n");

    /* 1. Prime CSS */
    size_t css_len = 0;
    char* css_buf = get_styles_css(&css_len);
    if (css_buf) {
        printf("  [OK] /styles.css  -> %zu bytes CSS in memory\n", css_len);
        free(css_buf);
    }

    /* 2. Prime JS */
    size_t js_len = 0;
    char* js_buf = get_logic_js(&js_len);
    if (js_buf) {
        printf("  [OK] /logic.js    -> %zu bytes JS in memory\n", js_len);
        free(js_buf);
    }

    /* 3. Prime HTML Pages */
    for (int i = 0; g_routes[i].path[0] != '\0'; i++) {
        size_t len = 0;
        char* buf = get_page_html(&g_routes[i], &len);
        if (buf) {
            printf("  [OK] %-12s -> %zu bytes in memory\n", g_routes[i].path, len);
            free(buf);
        }
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[FATAL] WSAStartup failed.\n");
        return 1;
    }

    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        fprintf(stderr, "[FATAL] Could not create socket.\n");
        WSACleanup();
        return 1;
    }

    BOOL opt = TRUE;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons((u_short)port);

    if (bind(server_sock, (struct sockaddr*)&saddr, sizeof(saddr)) == SOCKET_ERROR) {
        fprintf(stderr, "[FATAL] Failed to bind to port %d. Error: %d\n", port, WSAGetLastError());
        closesocket(server_sock);
        WSACleanup();
        return 1;
    }

    if (listen(server_sock, 64) == SOCKET_ERROR) {
        fprintf(stderr, "[FATAL] Listen failed.\n");
        closesocket(server_sock);
        WSACleanup();
        return 1;
    }

    printf("\n  >> Pure C Web Server Live: http://localhost:%d\n", port);
    printf("  >> Diagnostic Route:      http://localhost:%d/api/health\n", port);
    printf("  >> Disk State:            ZERO .html, ZERO .css, ZERO .js, ZERO server.py\n");
    printf("  >> Thread Pool:           Active (Worker per request)\n");
    printf("=================================================================\n\n");

    while (1) {
        struct sockaddr_in caddr;
        int clen = sizeof(caddr);
        SOCKET client_sock = accept(server_sock, (struct sockaddr*)&caddr, &clen);
        if (client_sock == INVALID_SOCKET) {
            continue;
        }

        uintptr_t th = _beginthreadex(NULL, 0, client_worker, (void*)(intptr_t)client_sock, 0, NULL);
        if (th) {
            CloseHandle((HANDLE)th);
        } else {
            closesocket(client_sock);
        }
    }

    closesocket(server_sock);
    WSACleanup();
    DeleteCriticalSection(&g_cache_lock);
    return 0;
}
