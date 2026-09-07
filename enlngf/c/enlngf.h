#ifndef ENLNGF_H
#define ENLNGF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <windows.h>

#define ENLNGF_VERSION "5.3.0"
#define MAX_ELEMENTS 256

typedef enum {
    ELEM_HEADING,
    ELEM_PARAGRAPH,
    ELEM_BUTTON,
    ELEM_INPUT,
    ELEM_SECTION,
    ELEM_CARD
} EnlngfElemType;

typedef struct {
    EnlngfElemType type;
    int level; // for heading (1, 2, 3)
    char text[256];
    char id[64];
    char class_name[64];
    char hint[128];
    char color[32];
    int width;
    int height;
} EnlngfElement;

typedef struct {
    char title[128];
    char design_file[128];
    char script_file[128];
    EnlngfElement elements[MAX_ELEMENTS];
    int element_count;
} EnlngfDoc;

/* Core API */
EnlngfDoc* enlngf_create_doc(void);
void enlngf_free_doc(EnlngfDoc* doc);
bool enlngf_parse_file(EnlngfDoc* doc, const char* filepath);

int enlngf_run_gui(const char* filepath);
int enlngf_serve_http(const char* filepath, int port);
int enlngf_compile_to_html(const EnlngfDoc* doc, const char* outpath);

#endif /* ENLNGF_H */
