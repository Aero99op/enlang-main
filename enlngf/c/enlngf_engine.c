#include "enlngf.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

static char* trim(char* s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char* back = s + strlen(s) - 1;
    while (back > s && (isspace((unsigned char)*back) || *back == ';')) {
        *back = '\0';
        back--;
    }
    return s;
}

static bool str_starts_with_ci(const char* s, const char* prefix) {
    if (!s || !prefix) return false;
    while (*prefix) {
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) return false;
        s++; prefix++;
    }
    return true;
}

EnlngfDoc* enlngf_create_doc(void) {
    EnlngfDoc* doc = (EnlngfDoc*)calloc(1, sizeof(EnlngfDoc));
    strcpy(doc->title, "Enlangg Sovereign Web Studio");
    return doc;
}

void enlngf_free_doc(EnlngfDoc* doc) {
    if (doc) free(doc);
}

static void extract_quoted(const char* s, char* out, size_t out_sz) {
    const char* q1 = strchr(s, '"');
    if (!q1) q1 = strchr(s, '\'');
    if (q1) {
        char quote = *q1;
        const char* q2 = strchr(q1 + 1, quote);
        if (q2) {
            size_t len = (size_t)(q2 - (q1 + 1));
            if (len >= out_sz) len = out_sz - 1;
            strncpy(out, q1 + 1, len);
            out[len] = '\0';
            return;
        }
    }
    strncpy(out, s, out_sz - 1);
    out[out_sz - 1] = '\0';
}

bool enlngf_parse_file(EnlngfDoc* doc, const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return false;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char* s = trim(line);
        if (!*s || s[0] == '#') continue;

        if (str_starts_with_ci(s, "title ")) {
            extract_quoted(s + 6, doc->title, sizeof(doc->title));
            continue;
        }
        if (str_starts_with_ci(s, "page ")) {
            extract_quoted(s + 5, doc->title, sizeof(doc->title));
            continue;
        }
        if (str_starts_with_ci(s, "connect design ")) {
            char fpath[128];
            extract_quoted(s + 15, fpath, sizeof(fpath));
            strncpy(doc->design_file, trim(fpath), sizeof(doc->design_file) - 1);
            continue;
        }
        if (str_starts_with_ci(s, "connect script ")) {
            char fpath[128];
            extract_quoted(s + 15, fpath, sizeof(fpath));
            strncpy(doc->script_file, trim(fpath), sizeof(doc->script_file) - 1);
            continue;
        }

        if (doc->element_count >= MAX_ELEMENTS) continue;
        EnlngfElement* el = &doc->elements[doc->element_count];

        // Heading: heading 1 "Text" [with color "..."]
        if (str_starts_with_ci(s, "heading ")) {
            el->type = ELEM_HEADING;
            el->level = 1;
            char* p = s + 8;
            if (isdigit((unsigned char)*p)) {
                el->level = *p - '0';
                p++;
            }
            extract_quoted(p, el->text, sizeof(el->text));
            if (strstr(s, "id ")) extract_quoted(strstr(s, "id ") + 3, el->id, sizeof(el->id));
            if (strstr(s, "class ")) extract_quoted(strstr(s, "class ") + 6, el->class_name, sizeof(el->class_name));
            doc->element_count++;
            continue;
        }

        // Paragraph: paragraph "Text"
        if (str_starts_with_ci(s, "paragraph ")) {
            el->type = ELEM_PARAGRAPH;
            extract_quoted(s + 10, el->text, sizeof(el->text));
            doc->element_count++;
            continue;
        }

        // Button: button "Text" [id "..."]
        if (str_starts_with_ci(s, "button ") || str_starts_with_ci(s, "submit button ")) {
            el->type = ELEM_BUTTON;
            char* p = str_starts_with_ci(s, "submit button ") ? s + 14 : s + 7;
            extract_quoted(p, el->text, sizeof(el->text));
            if (strstr(s, "id ")) extract_quoted(strstr(s, "id ") + 3, el->id, sizeof(el->id));
            if (strstr(s, "class ")) extract_quoted(strstr(s, "class ") + 6, el->class_name, sizeof(el->class_name));
            doc->element_count++;
            continue;
        }

        // Text input: text input id "..." hint "..."
        if (str_starts_with_ci(s, "text input ") || str_starts_with_ci(s, "input ")) {
            el->type = ELEM_INPUT;
            if (strstr(s, "id ")) extract_quoted(strstr(s, "id ") + 3, el->id, sizeof(el->id));
            if (strstr(s, "hint ")) extract_quoted(strstr(s, "hint ") + 5, el->hint, sizeof(el->hint));
            doc->element_count++;
            continue;
        }
    }

    fclose(f);
    return true;
}

int enlngf_compile_to_html(const EnlngfDoc* doc, const char* outpath) {
    FILE* fout = fopen(outpath, "w");
    if (!fout) {
        fprintf(stderr, "Error: Could not open output file '%s'\n", outpath);
        return 1;
    }

    fprintf(fout, "<!DOCTYPE html>\n");
    fprintf(fout, "<html lang=\"en\">\n");
    fprintf(fout, "<head>\n");
    fprintf(fout, "  <meta charset=\"UTF-8\">\n");
    fprintf(fout, "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    fprintf(fout, "  <title>%s</title>\n", doc->title);

    if (doc->design_file[0]) {
        fprintf(fout, "  <link rel=\"stylesheet\" href=\"%s\">\n", doc->design_file);
    }
    fprintf(fout, "  <style>\n");
    fprintf(fout, "    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #0f172a; color: #f8fafc; margin: 0; padding: 40px; }\n");
    fprintf(fout, "    .container { max-width: 800px; margin: 0 auto; background: #1e293b; padding: 32px; border-radius: 12px; border: 1px solid #334155; }\n");
    fprintf(fout, "    h1, h2, h3 { color: #38bdf8; }\n");
    fprintf(fout, "    button { background: #0284c7; color: white; border: none; padding: 10px 20px; border-radius: 6px; font-weight: 600; cursor: pointer; }\n");
    fprintf(fout, "    button:hover { background: #0369a1; }\n");
    fprintf(fout, "    input { background: #0f172a; border: 1px solid #334155; color: white; padding: 8px 12px; border-radius: 6px; width: 100%%; box-sizing: border-box; margin-bottom: 12px; }\n");
    fprintf(fout, "  </style>\n");
    fprintf(fout, "</head>\n");
    fprintf(fout, "<body>\n");
    fprintf(fout, "  <div class=\"container\">\n");

    for (int i = 0; i < doc->element_count; i++) {
        const EnlngfElement* el = &doc->elements[i];
        if (el->type == ELEM_HEADING) {
            fprintf(fout, "    <h%d", el->level);
            if (el->id[0]) fprintf(fout, " id=\"%s\"", el->id);
            if (el->class_name[0]) fprintf(fout, " class=\"%s\"", el->class_name);
            fprintf(fout, ">%s</h%d>\n", el->text, el->level);
        } else if (el->type == ELEM_PARAGRAPH) {
            fprintf(fout, "    <p>%s</p>\n", el->text);
        } else if (el->type == ELEM_BUTTON) {
            fprintf(fout, "    <button");
            if (el->id[0]) fprintf(fout, " id=\"%s\"", el->id);
            if (el->class_name[0]) fprintf(fout, " class=\"%s\"", el->class_name);
            fprintf(fout, ">%s</button>\n", el->text);
        } else if (el->type == ELEM_INPUT) {
            fprintf(fout, "    <input type=\"text\"");
            if (el->id[0]) fprintf(fout, " id=\"%s\"", el->id);
            fprintf(fout, " placeholder=\"%s\">\n", el->hint);
        }
    }

    fprintf(fout, "  </div>\n");
    if (doc->script_file[0]) {
        fprintf(fout, "  <script src=\"%s\"></script>\n", doc->script_file);
    }
    fprintf(fout, "</body>\n");
    fprintf(fout, "</html>\n");

    fclose(fout);
    printf("[enlngf] Semantic HTML5 compiled successfully: '%s'\n", outpath);
    return 0;
}

/* Win32 Native Desktop GUI Implementation */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                MessageBoxA(hwnd, "Native Sovereign UI Button Clicked!", "Enlangg Sovereign GUI", MB_OK | MB_ICONINFORMATION);
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int enlngf_run_gui(const char* filepath) {
    EnlngfDoc* doc = enlngf_create_doc();
    if (!enlngf_parse_file(doc, filepath)) {
        fprintf(stderr, "Error: Could not parse '%s'\n", filepath);
        enlngf_free_doc(doc);
        return 1;
    }

    HINSTANCE hInstance = GetModuleHandle(NULL);
    const char CLASS_NAME[] = "EnlngfSovereignWindowClass";

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(15, 23, 42)); // Modern dark theme

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0, CLASS_NAME, doc->title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 840, 650,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        fprintf(stderr, "Error: Could not create native Win32 window.\n");
        enlngf_free_doc(doc);
        return 1;
    }

    // Spawn controls for elements
    int y = 30;
    HFONT hFontHeading = CreateFontA(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Segoe UI");
    HFONT hFontText = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Segoe UI");

    for (int i = 0; i < doc->element_count; i++) {
        EnlngfElement* el = &doc->elements[i];
        if (el->type == ELEM_HEADING) {
            HWND hText = CreateWindowExA(0, "STATIC", el->text, WS_VISIBLE | WS_CHILD, 40, y, 740, 36, hwnd, NULL, hInstance, NULL);
            SendMessage(hText, WM_SETFONT, (WPARAM)hFontHeading, TRUE);
            y += 48;
        } else if (el->type == ELEM_PARAGRAPH) {
            HWND hText = CreateWindowExA(0, "STATIC", el->text, WS_VISIBLE | WS_CHILD, 40, y, 740, 24, hwnd, NULL, hInstance, NULL);
            SendMessage(hText, WM_SETFONT, (WPARAM)hFontText, TRUE);
            y += 34;
        } else if (el->type == ELEM_INPUT) {
            CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", el->hint, WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 40, y, 400, 32, hwnd, NULL, hInstance, NULL);
            y += 46;
        } else if (el->type == ELEM_BUTTON) {
            HWND hBtn = CreateWindowExA(0, "BUTTON", el->text, WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 40, y, 180, 38, hwnd, (HMENU)(INT_PTR)(100 + i), hInstance, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFontText, TRUE);
            y += 50;
        }
    }

    printf("=================================================================\n");
    printf("     ENLNGF: SOVEREIGN NATIVE DESKTOP GUI RUNNING                \n");
    printf("=================================================================\n");
    printf(" >> Window: '%s'\n", doc->title);
    printf(" >> Mode: Pure C Native Win32 Engine (Zero Chrome / Zero WebBrowser)\n");
    printf(" >> Close window to exit.\n\n");

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    enlngf_free_doc(doc);
    return 0;
}

/* Pure C WinSock2 HTTP Web Studio Server */
int enlngf_serve_http(const char* filepath, int port) {
    EnlngfDoc* doc = enlngf_create_doc();
    if (!enlngf_parse_file(doc, filepath)) {
        fprintf(stderr, "Error: Could not parse '%s'\n", filepath);
        enlngf_free_doc(doc);
        return 1;
    }

    char html_temp[256] = "enlngf_preview.html";
    enlngf_compile_to_html(doc, html_temp);

    FILE* f = fopen(html_temp, "rb");
    if (!f) return 1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* html_body = (char*)malloc(sz + 1);
    fread(html_body, 1, sz, f);
    html_body[sz] = '\0';
    fclose(f);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((u_short)port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        fprintf(stderr, "Error: Port %d already in use.\n", port);
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    listen(server_fd, 10);

    printf("=================================================================\n");
    printf("     ENLNGF: SOVEREIGN WEB STUDIO HTTP SERVER LIVE               \n");
    printf("=================================================================\n");
    printf(" >> Local Studio: http://localhost:%d\n", port);
    printf(" >> Engine: Pure C WinSock2 Daemon (Zero Python / Zero Node.js)\n");
    printf(" >> Serving Document: '%s'\n", doc->title);
    printf(" >> Press Ctrl+C to terminate.\n\n");

    while (1) {
        SOCKET client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == INVALID_SOCKET) break;

        char req_buf[2048];
        recv(client_fd, req_buf, sizeof(req_buf) - 1, 0);

        char resp_hdr[512];
        snprintf(resp_hdr, sizeof(resp_hdr),
            "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", sz);

        send(client_fd, resp_hdr, (int)strlen(resp_hdr), 0);
        send(client_fd, html_body, (int)sz, 0);
        closesocket(client_fd);
    }

    closesocket(server_fd);
    WSACleanup();
    free(html_body);
    enlngf_free_doc(doc);
    return 0;
}
