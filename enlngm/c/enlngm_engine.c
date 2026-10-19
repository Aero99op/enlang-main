#include "enlngm.h"

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

EnlngmApp* enlngm_create_app(void) {
    EnlngmApp* app = (EnlngmApp*)calloc(1, sizeof(EnlngmApp));
    strcpy(app->app_name, "Enlangg Mobile App");
    strcpy(app->primary_color, "#00f2fe");
    strcpy(app->appbar_title, "Dashboard");
    app->is_dark = true;
    return app;
}

void enlngm_free_app(EnlngmApp* app) {
    if (app) free(app);
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

static void write_escaped_c_str(FILE* f, const char* s) {
    fputc('"', f);
    for (const char* p = s; *p; p++) {
        if (*p == '"' || *p == '\\') {
            fputc('\\', f);
            fputc(*p, f);
        } else if (*p == '\n') {
            fputs("\\n", f);
        } else if (*p == '\r') {
            /* skip */
        } else {
            fputc(*p, f);
        }
    }
    fputc('"', f);
}

bool enlngm_parse_file(EnlngmApp* app, const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return false;

    char line[512];
    EnlngmWidget* current_widget = NULL;

    while (fgets(line, sizeof(line), f)) {
        char* s = trim(line);
        if (!*s || s[0] == '#') continue;

        if (str_starts_with_ci(s, "app ") || str_starts_with_ci(s, "create mobile application ")) {
            const char* p = str_starts_with_ci(s, "create mobile application ") ? s + 26 : s + 4;
            extract_quoted(p, app->app_name, sizeof(app->app_name));
            continue;
        }
        if (str_starts_with_ci(s, "accent color ") || str_starts_with_ci(s, "primary color ")) {
            extract_quoted(s + 13, app->primary_color, sizeof(app->primary_color));
            continue;
        }
        if (str_starts_with_ci(s, "theme dark")) {
            app->is_dark = true;
            continue;
        }
        if (str_starts_with_ci(s, "title ") || str_starts_with_ci(s, "add title ")) {
            const char* p = str_starts_with_ci(s, "add title ") ? s + 10 : s + 6;
            extract_quoted(p, app->appbar_title, sizeof(app->appbar_title));
            continue;
        }
        if (str_starts_with_ci(s, "screen ") || str_starts_with_ci(s, "stateful screen ") || str_starts_with_ci(s, "create screen ")) {
            char* p = s + 7;
            if (str_starts_with_ci(s, "stateful screen ")) p = s + 16;
            else if (str_starts_with_ci(s, "create screen ")) p = s + 14;
            char tmp[64];
            extract_quoted(p, tmp, sizeof(tmp));
            char* col = strchr(tmp, ':');
            if (col) *col = '\0';
            if (!app->screen_title[0]) {
                strncpy(app->screen_title, trim(tmp), sizeof(app->screen_title) - 1);
            }
            continue;
        }

        /* Button tap / action handlers */
        if ((str_starts_with_ci(s, "show toast ") || str_starts_with_ci(s, "show snackbar ")) && current_widget) {
            const char* p = str_starts_with_ci(s, "show snackbar ") ? s + 14 : s + 11;
            extract_quoted(p, current_widget->action_toast, sizeof(current_widget->action_toast));
            continue;
        }
        if (str_starts_with_ci(s, "go to ") && current_widget) {
            char dest[64];
            extract_quoted(s + 6, dest, sizeof(dest));
            snprintf(current_widget->action_toast, sizeof(current_widget->action_toast), "Navigating to %s...", trim(dest));
            continue;
        }
        if (str_starts_with_ci(s, "go back") && current_widget) {
            if (!current_widget->action_toast[0]) {
                strcpy(current_widget->action_toast, "Navigating Back...");
            }
            continue;
        }

        if (app->widget_count >= MAX_MOBILE_WIDGETS) continue;
        EnlngmWidget* w = &app->widgets[app->widget_count];

        /* Text / heading / label / badge / chip widgets */
        if (str_starts_with_ci(s, "text ") || str_starts_with_ci(s, "add heading ") ||
            str_starts_with_ci(s, "add description ") || str_starts_with_ci(s, "add label ") ||
            str_starts_with_ci(s, "add value ") || str_starts_with_ci(s, "add subvalue ") ||
            str_starts_with_ci(s, "add status ") || str_starts_with_ci(s, "add badge ") ||
            str_starts_with_ci(s, "chip ") || str_starts_with_ci(s, "create section_title ")) {
            w->type = WIDGET_TEXT;
            w->font_size = 15;
            if (str_starts_with_ci(s, "add heading ") || str_starts_with_ci(s, "create section_title ")) {
                w->font_size = 18;
                w->bold = true;
            } else if (str_starts_with_ci(s, "add value ")) {
                w->font_size = 17;
                w->bold = true;
            }
            extract_quoted(s, w->text, sizeof(w->text));
            char* sz_pos = strstr(s, "size ");
            if (sz_pos) w->font_size = atoi(sz_pos + 5);
            if (strstr(s, "bold")) w->bold = true;
            app->widget_count++;
            current_widget = w;
            continue;
        }

        /* Button / icon button / floating button / action_button / tile / tab */
        if (str_starts_with_ci(s, "button ") || str_starts_with_ci(s, "icon button ") ||
            str_starts_with_ci(s, "floating button ") || str_starts_with_ci(s, "add action_button ") ||
            str_starts_with_ci(s, "add tab ")) {
            w->type = str_starts_with_ci(s, "icon button ") ? WIDGET_ICON_BTN : WIDGET_BUTTON;
            extract_quoted(s, w->text, sizeof(w->text));
            snprintf(w->action_toast, sizeof(w->action_toast), "%s activated", w->text);
            app->widget_count++;
            current_widget = w;
            continue;
        }

        /* Input field */
        if (str_starts_with_ci(s, "input ")) {
            w->type = WIDGET_CARD;
            char* ph = strstr(s, "placeholder ");
            if (ph) extract_quoted(ph + 12, w->text, sizeof(w->text));
            else extract_quoted(s + 6, w->text, sizeof(w->text));
            app->widget_count++;
            current_widget = w;
            continue;
        }

        /* Spacer: spacer height 16 */
        if (str_starts_with_ci(s, "spacer")) {
            w->type = WIDGET_SPACER;
            w->height = 16;
            char* h_pos = strstr(s, "height ");
            if (h_pos) w->height = atoi(h_pos + 7);
            app->widget_count++;
            current_widget = w;
            continue;
        }

        /* Divider: divider */
        if (str_starts_with_ci(s, "divider")) {
            w->type = WIDGET_DIVIDER;
            app->widget_count++;
            current_widget = w;
            continue;
        }
    }

    fclose(f);
    return true;
}

int enlngm_export_c(const EnlngmApp* app, const char* outpath) {
    FILE* f = fopen(outpath, "w");
    if (!f) return 1;

    fprintf(f, "/*\n");
    fprintf(f, " * Standalone Win32 Desktop Preview Application\n");
    fprintf(f, " * Generated by Enlangg Sovereign Preview Compiler\n");
    fprintf(f, " * Compile on Windows: gcc \"%s\" -o desktop_preview.exe -lgdi32 -luser32\n", outpath);
    fprintf(f, " */\n\n");
    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <stdlib.h>\n");
    fprintf(f, "#include <string.h>\n");
    fprintf(f, "#include <stdbool.h>\n");
    fprintf(f, "#ifdef _WIN32\n");
    fprintf(f, "#include <windows.h>\n");
    fprintf(f, "#endif\n\n");

    fprintf(f, "typedef enum {\n");
    fprintf(f, "    EXP_WIDGET_TEXT = 0,\n");
    fprintf(f, "    EXP_WIDGET_BUTTON = 1,\n");
    fprintf(f, "    EXP_WIDGET_ICON_BTN = 2,\n");
    fprintf(f, "    EXP_WIDGET_SPACER = 3,\n");
    fprintf(f, "    EXP_WIDGET_DIVIDER = 4,\n");
    fprintf(f, "    EXP_WIDGET_INPUT = 5\n");
    fprintf(f, "} ExportWidgetType;\n\n");

    fprintf(f, "typedef struct {\n");
    fprintf(f, "    ExportWidgetType type;\n");
    fprintf(f, "    const char* text;\n");
    fprintf(f, "    int font_size;\n");
    fprintf(f, "    bool bold;\n");
    fprintf(f, "    int height;\n");
    fprintf(f, "    const char* action_toast;\n");
    fprintf(f, "} ExportWidget;\n\n");

    fprintf(f, "static const char* APP_NAME = ");
    write_escaped_c_str(f, app->app_name);
    fprintf(f, ";\n");
    fprintf(f, "static const char* APPBAR_TITLE = ");
    write_escaped_c_str(f, app->appbar_title);
    fprintf(f, ";\n");
    fprintf(f, "static const char* SCREEN_TITLE = ");
    write_escaped_c_str(f, app->screen_title[0] ? app->screen_title : "MainScreen");
    fprintf(f, ";\n");
    fprintf(f, "static const char* PRIMARY_COLOR = ");
    write_escaped_c_str(f, app->primary_color);
    fprintf(f, ";\n\n");

    fprintf(f, "static const ExportWidget G_WIDGETS[] = {\n");
    for (int i = 0; i < app->widget_count; i++) {
        const EnlngmWidget* w = &app->widgets[i];
        fprintf(f, "    { %d, ", (int)w->type);
        write_escaped_c_str(f, w->text);
        fprintf(f, ", %d, %s, %d, ", w->font_size > 0 ? w->font_size : 15, w->bold ? "true" : "false", w->height);
        write_escaped_c_str(f, w->action_toast);
        fprintf(f, " }%s\n", (i + 1 < app->widget_count) ? "," : "");
    }
    if (app->widget_count == 0) {
        fprintf(f, "    { 0, \"\", 15, false, 0, \"\" }\n");
    }
    fprintf(f, "};\n");
    fprintf(f, "static const int G_WIDGET_COUNT = %d;\n\n", app->widget_count);

    fprintf(f, "#ifdef _WIN32\n");
    fprintf(f, "static LRESULT CALLBACK ExportMobileWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {\n");
    fprintf(f, "    switch (msg) {\n");
    fprintf(f, "        case WM_COMMAND: {\n");
    fprintf(f, "            int id = LOWORD(wParam);\n");
    fprintf(f, "            if (id >= 200 && id < 200 + G_WIDGET_COUNT) {\n");
    fprintf(f, "                int idx = id - 200;\n");
    fprintf(f, "                const char* toast = (G_WIDGETS[idx].action_toast && G_WIDGETS[idx].action_toast[0])\n");
    fprintf(f, "                    ? G_WIDGETS[idx].action_toast : \"Button Tapped!\";\n");
    fprintf(f, "                MessageBoxA(hwnd, toast, \"Enlangg Preview Toast\", MB_OK | MB_ICONINFORMATION);\n");
    fprintf(f, "            }\n");
    fprintf(f, "            break;\n");
    fprintf(f, "        }\n");
    fprintf(f, "        case WM_PAINT: {\n");
    fprintf(f, "            PAINTSTRUCT ps;\n");
    fprintf(f, "            HDC hdc = BeginPaint(hwnd, &ps);\n");
    fprintf(f, "            SetBkMode(hdc, TRANSPARENT);\n");
    fprintf(f, "            SetTextColor(hdc, RGB(255, 255, 255));\n");
    fprintf(f, "            TextOutA(hdc, 24, 10, \"9:41\", 4);\n");
    fprintf(f, "            TextOutA(hdc, 330, 10, \"5G 100%%\", 7);\n");
    fprintf(f, "            RECT appbar_rc = { 0, 32, 390, 86 };\n");
    fprintf(f, "            HBRUSH hAppbarBr = CreateSolidBrush(RGB(30, 41, 59));\n");
    fprintf(f, "            FillRect(hdc, &appbar_rc, hAppbarBr);\n");
    fprintf(f, "            DeleteObject(hAppbarBr);\n");
    fprintf(f, "            SetTextColor(hdc, RGB(56, 189, 248));\n");
    fprintf(f, "            HFONT hTitleFont = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, \"Segoe UI\");\n");
    fprintf(f, "            SelectObject(hdc, hTitleFont);\n");
    fprintf(f, "            TextOutA(hdc, 20, 46, APPBAR_TITLE, (int)strlen(APPBAR_TITLE));\n");
    fprintf(f, "            DeleteObject(hTitleFont);\n");
    fprintf(f, "            EndPaint(hwnd, &ps);\n");
    fprintf(f, "            break;\n");
    fprintf(f, "        }\n");
    fprintf(f, "        case WM_DESTROY:\n");
    fprintf(f, "            PostQuitMessage(0);\n");
    fprintf(f, "            break;\n");
    fprintf(f, "        default:\n");
    fprintf(f, "            return DefWindowProc(hwnd, msg, wParam, lParam);\n");
    fprintf(f, "    }\n");
    fprintf(f, "    return 0;\n");
    fprintf(f, "}\n");
    fprintf(f, "#endif\n\n");

    fprintf(f, "int main(int argc, char* argv[]) {\n");
    fprintf(f, "    bool headless = false;\n");
    fprintf(f, "    for (int a = 1; a < argc; a++) {\n");
    fprintf(f, "        if (strcmp(argv[a], \"--headless\") == 0) headless = true;\n");
    fprintf(f, "    }\n");
    fprintf(f, "    if (headless) {\n");
    fprintf(f, "        printf(\"[App] %%s | [Screen] %%s | [AppBar] %%s | [Color] %%s\\n\", APP_NAME, SCREEN_TITLE, APPBAR_TITLE, PRIMARY_COLOR);\n");
    fprintf(f, "        for (int i = 0; i < G_WIDGET_COUNT; i++) {\n");
    fprintf(f, "            printf(\"  Widget[%%d] type=%%d text='%%s' action='%%s'\\n\", i, G_WIDGETS[i].type, G_WIDGETS[i].text, G_WIDGETS[i].action_toast);\n");
    fprintf(f, "        }\n");
    fprintf(f, "        return 0;\n");
    fprintf(f, "    }\n");
    fprintf(f, "#ifdef _WIN32\n");
    fprintf(f, "    HINSTANCE hInstance = GetModuleHandle(NULL);\n");
    fprintf(f, "    WNDCLASSA wc = {0};\n");
    fprintf(f, "    wc.lpfnWndProc = ExportMobileWndProc;\n");
    fprintf(f, "    wc.hInstance = hInstance;\n");
    fprintf(f, "    wc.lpszClassName = \"Enlangg Desktop Preview (390x844)\";\n");
    fprintf(f, "    wc.hCursor = LoadCursor(NULL, IDC_ARROW);\n");
    fprintf(f, "    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(15, 23, 42));\n");
    fprintf(f, "    RegisterClassA(&wc);\n");
    fprintf(f, "    HWND hwnd = CreateWindowExA(WS_EX_TOPMOST, wc.lpszClassName, \"Enlangg Desktop Preview (390x844)\",\n");
    fprintf(f, "        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,\n");
    fprintf(f, "        CW_USEDEFAULT, CW_USEDEFAULT, 406, 820, NULL, NULL, hInstance, NULL);\n");
    fprintf(f, "    if (!hwnd) return 1;\n");
    fprintf(f, "    int y = 106;\n");
    fprintf(f, "    for (int i = 0; i < G_WIDGET_COUNT; i++) {\n");
    fprintf(f, "        const ExportWidget* w = &G_WIDGETS[i];\n");
    fprintf(f, "        if (w->type == EXP_WIDGET_TEXT) {\n");
    fprintf(f, "            HWND hTxt = CreateWindowExA(0, \"STATIC\", w->text, WS_VISIBLE | WS_CHILD, 24, y, 342, 28, hwnd, NULL, hInstance, NULL);\n");
    fprintf(f, "            HFONT hf = CreateFontA(w->font_size, 0, 0, 0, w->bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, \"Segoe UI\");\n");
    fprintf(f, "            SendMessage(hTxt, WM_SETFONT, (WPARAM)hf, TRUE);\n");
    fprintf(f, "            y += 34;\n");
    fprintf(f, "        } else if (w->type == EXP_WIDGET_BUTTON || w->type == EXP_WIDGET_ICON_BTN) {\n");
    fprintf(f, "            HWND hBtn = CreateWindowExA(0, \"BUTTON\", w->text, WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 24, y, 342, 44, hwnd, (HMENU)(INT_PTR)(200 + i), hInstance, NULL);\n");
    fprintf(f, "            HFONT hf = CreateFontA(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, \"Segoe UI\");\n");
    fprintf(f, "            SendMessage(hBtn, WM_SETFONT, (WPARAM)hf, TRUE);\n");
    fprintf(f, "            y += 54;\n");
    fprintf(f, "        } else if (w->type == EXP_WIDGET_INPUT) {\n");
    fprintf(f, "            CreateWindowExA(WS_EX_CLIENTEDGE, \"EDIT\", w->text, WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 24, y, 342, 32, hwnd, NULL, hInstance, NULL);\n");
    fprintf(f, "            y += 42;\n");
    fprintf(f, "        } else if (w->type == EXP_WIDGET_SPACER) {\n");
    fprintf(f, "            y += w->height;\n");
    fprintf(f, "        } else if (w->type == EXP_WIDGET_DIVIDER) {\n");
    fprintf(f, "            CreateWindowExA(0, \"STATIC\", \"\", WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ, 24, y, 342, 2, hwnd, NULL, hInstance, NULL);\n");
    fprintf(f, "            y += 12;\n");
    fprintf(f, "        }\n");
    fprintf(f, "    }\n");
    fprintf(f, "    MSG msg = {0};\n");
    fprintf(f, "    while (GetMessage(&msg, NULL, 0, 0)) {\n");
    fprintf(f, "        TranslateMessage(&msg);\n");
    fprintf(f, "        DispatchMessage(&msg);\n");
    fprintf(f, "    }\n");
    fprintf(f, "    return 0;\n");
    fprintf(f, "#else\n");
    fprintf(f, "    printf(\"[App] %%s | [Screen] %%s | [AppBar] %%s\\n\", APP_NAME, SCREEN_TITLE, APPBAR_TITLE);\n");
    fprintf(f, "    for (int i = 0; i < G_WIDGET_COUNT; i++) {\n");
    fprintf(f, "        printf(\"  Widget[%%d] type=%%d text='%%s' action='%%s'\\n\", i, G_WIDGETS[i].type, G_WIDGETS[i].text, G_WIDGETS[i].action_toast);\n");
    fprintf(f, "    }\n");
    fprintf(f, "    return 0;\n");
    fprintf(f, "#endif\n");
    fprintf(f, "}\n");

    fclose(f);
    printf("[enlngm] Win32 desktop preview C source exported successfully: '%s'\n", outpath);
    return 0;
}

int enlngm_build_package(const char* filepath, const char* target, const char* outpath) {
    (void)filepath;
    (void)target;
    (void)outpath;
    fprintf(stderr, "Error: APK packaging not yet implemented.\n");
    return 1;
}

#ifdef _WIN32
/* Win32 Native Desktop Preview Window (390x844) */
static EnlngmApp* g_sim_app = NULL;

static LRESULT CALLBACK MobileSimWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id >= 200 && id < 200 + g_sim_app->widget_count) {
                int widx = id - 200;
                const char* toast = g_sim_app->widgets[widx].action_toast[0] ?
                    g_sim_app->widgets[widx].action_toast : "Button Tapped!";
                MessageBoxA(hwnd, toast, "Enlangg Preview Toast", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // 1. Draw Phone Status Bar (Top notch & time)
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            TextOutA(hdc, 24, 10, "9:41", 4);
            TextOutA(hdc, 330, 10, "5G 100%", 7);

            // 2. Draw Top AppBar Background
            RECT appbar_rc = { 0, 32, 390, 86 };
            HBRUSH hAppbarBr = CreateSolidBrush(RGB(30, 41, 59));
            FillRect(hdc, &appbar_rc, hAppbarBr);
            DeleteObject(hAppbarBr);

            // AppBar Title
            SetTextColor(hdc, RGB(56, 189, 248));
            HFONT hTitleFont = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Segoe UI");
            SelectObject(hdc, hTitleFont);
            TextOutA(hdc, 20, 46, g_sim_app->appbar_title, (int)strlen(g_sim_app->appbar_title));
            DeleteObject(hTitleFont);

            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int enlngm_run_simulator(const char* filepath) {
    EnlngmApp* app = enlngm_create_app();
    if (!enlngm_parse_file(app, filepath)) {
        fprintf(stderr, "Error: Could not parse '%s'\n", filepath);
        enlngm_free_app(app);
        return 1;
    }
    g_sim_app = app;

    HINSTANCE hInstance = GetModuleHandle(NULL);
    const char CLASS_NAME[] = "Enlangg Desktop Preview (390x844)";

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = MobileSimWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(15, 23, 42)); // Modern phone AMOLED black

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST, CLASS_NAME, "Enlangg Desktop Preview (390x844)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 406, 820,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        fprintf(stderr, "Error: Failed to create desktop preview window.\n");
        enlngm_free_app(app);
        return 1;
    }

    // Spawn controls inside phone frame
    int y = 106;
    HFONT hFontBtn = CreateFontA(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Segoe UI");
    HFONT hFontText = CreateFontA(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Segoe UI");

    for (int i = 0; i < app->widget_count; i++) {
        EnlngmWidget* w = &app->widgets[i];
        if (w->type == WIDGET_TEXT) {
            HWND hText = CreateWindowExA(0, "STATIC", w->text, WS_VISIBLE | WS_CHILD, 24, y, 342, 28, hwnd, NULL, hInstance, NULL);
            SendMessage(hText, WM_SETFONT, (WPARAM)hFontText, TRUE);
            y += 34;
        } else if (w->type == WIDGET_BUTTON || w->type == WIDGET_ICON_BTN) {
            HWND hBtn = CreateWindowExA(0, "BUTTON", w->text, WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 24, y, 342, 44, hwnd, (HMENU)(INT_PTR)(200 + i), hInstance, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
            y += 54;
        } else if (w->type == WIDGET_SPACER) {
            y += w->height;
        } else if (w->type == WIDGET_DIVIDER) {
            CreateWindowExA(0, "STATIC", "", WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ, 24, y, 342, 2, hwnd, NULL, hInstance, NULL);
            y += 12;
        }
    }

    printf("=================================================================\n");
    printf("   ENLNGM: SOVEREIGN DESKTOP PREVIEW LIVE (390x844 VIEWPORT)     \n");
    printf("=================================================================\n");
    printf(" >> Viewport: Win32 Desktop Preview Port 390x844\n");
    printf(" >> App: '%s' | Screen: '%s'\n", app->app_name, app->screen_title);
    printf(" >> Engine: Pure C Sovereign Runtime (Zero Heavy Runtimes / Zero Android Studio)\n");
    printf(" >> Close preview window to exit.\n\n");

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    enlngm_free_app(app);
    return 0;
}
#else
int enlngm_run_simulator(const char* filepath) {
    EnlngmApp* app = enlngm_create_app();
    if (!enlngm_parse_file(app, filepath)) {
        fprintf(stderr, "Error: Could not parse '%s'\n", filepath);
        enlngm_free_app(app);
        return 1;
    }
    printf("=================================================================\n");
    printf("     ENLNGM: SOVEREIGN DESKTOP PREVIEW (CLI VIEWPORT)            \n");
    printf("=================================================================\n");
    printf(" >> App: '%s' | Screen: '%s'\n", app->app_name, app->screen_title);
    printf(" >> Widgets: %d total widgets defined.\n", app->widget_count);
    enlngm_free_app(app);
    return 0;
}
#endif
