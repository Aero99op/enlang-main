#ifndef ENLNGF_H
#define ENLNGF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define ENLNGF_VERSION "5.3.0"
#define MAX_CHILDREN 512
#define MAX_ATTRS 24
#define MAX_NODES 4096
#define MAX_STACK_DEPTH 512

typedef struct {
    char key[32];
    char value[256];
} EnlngfAttr;

typedef struct EnlngfNode {
    char tag[32];
    char text[2048];
    EnlngfAttr attrs[MAX_ATTRS];
    int attr_count;
    bool is_self_closing;
    int indent_level;
    struct EnlngfNode* children[MAX_CHILDREN];
    int child_count;
    struct EnlngfNode* parent;
} EnlngfNode;

typedef struct {
    char title[128];
    char source_dir[512];
    char design_files[16][128];
    int design_file_count;
    char script_files[16][128];
    int script_file_count;
    char inline_css[8192];
    char inline_js[8192];
    EnlngfNode nodes_pool[MAX_NODES];
    int node_count;
    EnlngfNode* root;
} EnlngfDoc;

/* Core API */
EnlngfDoc* enlngf_create_doc(void);
void enlngf_free_doc(EnlngfDoc* doc);
bool enlngf_parse_file(EnlngfDoc* doc, const char* filepath);

int enlngf_run_gui(const char* filepath);
int enlngf_serve_http(const char* filepath, int port);
int enlngf_compile_to_html(const EnlngfDoc* doc, const char* outpath);

#endif /* ENLNGF_H */
