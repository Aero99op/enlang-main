#ifndef ENLNGM_H
#define ENLNGM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <windows.h>

#define ENLNGM_VERSION "5.3.0"
#define MAX_MOBILE_WIDGETS 128

typedef enum {
    WIDGET_TEXT,
    WIDGET_BUTTON,
    WIDGET_ICON_BTN,
    WIDGET_SPACER,
    WIDGET_DIVIDER,
    WIDGET_CARD
} EnlngmWidgetType;

typedef struct {
    EnlngmWidgetType type;
    char text[128];
    int font_size;
    bool bold;
    char color[32];
    char action_toast[128];
    int height;
} EnlngmWidget;

typedef struct {
    char app_name[64];
    char primary_color[32];
    bool is_dark;
    char screen_title[64];
    char appbar_title[64];
    EnlngmWidget widgets[MAX_MOBILE_WIDGETS];
    int widget_count;
} EnlngmApp;

/* Core API */
EnlngmApp* enlngm_create_app(void);
void enlngm_free_app(EnlngmApp* app);
bool enlngm_parse_file(EnlngmApp* app, const char* filepath);

int enlngm_run_simulator(const char* filepath);
int enlngm_build_package(const char* filepath, const char* target, const char* outpath);
int enlngm_export_dart(const EnlngmApp* app, const char* outpath);

#endif /* ENLNGM_H */
