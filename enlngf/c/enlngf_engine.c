#include "enlngf.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#endif

static char* trim(char* s) {
    if ((unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) {
        s += 3;
    }
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

static bool str_equals_ci(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static void strip_outer_quotes(char* s) {
    s = trim(s);
    size_t len = strlen(s);
    if (len >= 2 && ((s[0] == '"' && s[len - 1] == '"') || (s[0] == '\'' && s[len - 1] == '\''))) {
        s[len - 1] = '\0';
        memmove(s, s + 1, len - 1);
        trim(s);
    }
}

static void extract_quoted(const char* s, char* out, size_t out_sz) {
    out[0] = '\0';
    if (!s) return;
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
    strip_outer_quotes(out);
}

/* Extracts a quoted string ONLY if it appears immediately at `s` (after whitespace) */
static void extract_immediate_quoted(const char* s, char* out, size_t out_sz) {
    out[0] = '\0';
    if (!s) return;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '"' || *s == '\'') {
        char quote = *s;
        const char* q2 = strchr(s + 1, quote);
        if (q2) {
            size_t len = (size_t)(q2 - (s + 1));
            if (len >= out_sz) len = out_sz - 1;
            strncpy(out, s + 1, len);
            out[len] = '\0';
        }
    }
}

static void rewrite_extension(char* path, const char* old_ext, const char* new_ext) {
    char* p = strstr(path, old_ext);
    if (p && strlen(p) == strlen(old_ext)) {
        strcpy(p, new_ext);
    }
}

static void normalize_link_asset(char* path, bool is_css) {
    strip_outer_quotes(path);
    if (is_css) {
        rewrite_extension(path, ".enlngd", ".css");
        rewrite_extension(path, ".enlgd", ".css");
    } else {
        rewrite_extension(path, ".enlngs", ".js");
        rewrite_extension(path, ".enlgs", ".js");
    }
}

EnlngfDoc* enlngf_create_doc(void) {
    EnlngfDoc* doc = (EnlngfDoc*)calloc(1, sizeof(EnlngfDoc));
    strcpy(doc->title, "Enlangg Sovereign Web Studio");
    doc->root = &doc->nodes_pool[doc->node_count++];
    strcpy(doc->root->tag, "body");
    doc->root->indent_level = -1;
    return doc;
}

void enlngf_free_doc(EnlngfDoc* doc) {
    if (doc) free(doc);
}

static EnlngfNode* alloc_node(EnlngfDoc* doc, const char* tag, int indent_level) {
    if (doc->node_count >= MAX_NODES) return NULL;
    EnlngfNode* n = &doc->nodes_pool[doc->node_count++];
    memset(n, 0, sizeof(EnlngfNode));
    strncpy(n->tag, tag, sizeof(n->tag) - 1);
    n->indent_level = indent_level;

    static const char* self_closing[] = {"input", "br", "hr", "img", "meta", "link", NULL};
    for (int i = 0; self_closing[i]; i++) {
        if (strcmp(tag, self_closing[i]) == 0) {
            n->is_self_closing = true;
            break;
        }
    }
    return n;
}

static void add_class_token(char* class_buf, size_t buf_sz, const char* cls) {
    if (!cls || !*cls) return;
    char temp[256];
    strncpy(temp, cls, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    char* token = strtok(temp, " ");
    while (token) {
        char pattern[128];
        snprintf(pattern, sizeof(pattern), " %s ", token);
        char padded[512];
        snprintf(padded, sizeof(padded), " %s ", class_buf);
        if (strstr(padded, pattern) == NULL) {
            if (class_buf[0]) {
                strncat(class_buf, " ", buf_sz - strlen(class_buf) - 1);
            }
            strncat(class_buf, token, buf_sz - strlen(class_buf) - 1);
        }
        token = strtok(NULL, " ");
    }
}

static void add_attr(EnlngfNode* node, const char* key, const char* val) {
    if (!node || !key || !*key || !val || node->attr_count >= MAX_ATTRS) return;
    for (int i = 0; i < node->attr_count; i++) {
        if (strcmp(node->attrs[i].key, key) == 0) {
            if (strcmp(key, "class") == 0) {
                add_class_token(node->attrs[i].value, sizeof(node->attrs[i].value), val);
            } else {
                strncpy(node->attrs[i].value, val, sizeof(node->attrs[i].value) - 1);
            }
            return;
        }
    }
    EnlngfAttr* a = &node->attrs[node->attr_count++];
    strncpy(a->key, key, sizeof(a->key) - 1);
    strncpy(a->value, val, sizeof(a->value) - 1);
}

static void add_child(EnlngfNode* parent, EnlngfNode* child) {
    if (!parent || !child || parent->child_count >= MAX_CHILDREN) return;
    parent->children[parent->child_count++] = child;
    child->parent = parent;
}

/* Search for keyword `kw` outside of quoted strings */
static bool find_keyword_pos(const char* s, const char* kw, const char** out_pos) {
    size_t kw_len = strlen(kw);
    bool in_quote = false;
    char quote_ch = 0;
    for (const char* p = s; *p; p++) {
        if (!in_quote && (*p == '"' || *p == '\'')) {
            in_quote = true;
            quote_ch = *p;
            continue;
        }
        if (in_quote) {
            if (*p == quote_ch) in_quote = false;
            continue;
        }
        if (str_starts_with_ci(p, kw)) {
            if (p == s || isspace((unsigned char)*(p - 1)) || *(p - 1) == '"' || *(p - 1) == '\'') {
                *out_pos = p + kw_len;
                return true;
            }
        }
    }
    return false;
}

static void extract_attribute_match(const char* s, const char* kw, char* out, size_t out_sz) {
    out[0] = '\0';
    const char* p = NULL;
    if (find_keyword_pos(s, kw, &p)) {
        while (isspace((unsigned char)*p)) p++;
        if (*p == '"' || *p == '\'') {
            extract_immediate_quoted(p, out, out_sz);
        }
    }
}

/* Extract the first quoted string in `after_cmd` that is NOT preceded by an attribute keyword */
static void extract_unclaimed_quoted_text(const char* after_cmd, char* out, size_t out_sz) {
    out[0] = '\0';
    if (!after_cmd) return;

    /* First check explicit `with text "..."` or `and text "..."` */
    extract_attribute_match(after_cmd, "with text ", out, out_sz);
    if (out[0]) return;
    extract_attribute_match(after_cmd, "and text ", out, out_sz);
    if (out[0]) return;

    static const char* attr_words[] = {
        "id", "class", "src", "alt", "href", "target", "to",
        "placeholder", "hint", "style", "title", "type", "name", "named",
        "value", "min", "max", "default", "rows", "cols",
        "allow", "allowfullscreen", "readonly", "required", "checked",
        "onclick", "click", "clicked", "action", "method", "for",
        "width", "height", "color", "background", "margin", "padding",
        "border", "border-top", "container", "from", NULL
    };

    const char* p = after_cmd;
    while (*p) {
        if (*p == '"' || *p == '\'') {
            char quote_ch = *p;
            const char* q_end = strchr(p + 1, quote_ch);
            if (!q_end) break;

            /* Inspect the word immediately preceding `p` inside `after_cmd` */
            const char* back = p;
            while (back > after_cmd && isspace((unsigned char)*(back - 1))) back--;
            const char* word_end = back;
            while (back > after_cmd && !isspace((unsigned char)*(back - 1)) && *(back - 1) != '"' && *(back - 1) != '\'') {
                back--;
            }
            char prev_word[64] = {0};
            size_t wlen = (size_t)(word_end - back);
            if (wlen >= sizeof(prev_word)) wlen = sizeof(prev_word) - 1;
            strncpy(prev_word, back, wlen);
            prev_word[wlen] = '\0';

            bool is_attr = false;
            for (int i = 0; attr_words[i]; i++) {
                if (str_equals_ci(prev_word, attr_words[i])) {
                    is_attr = true;
                    break;
                }
            }

            if (!is_attr) {
                size_t len = (size_t)(q_end - (p + 1));
                if (len >= out_sz) len = out_sz - 1;
                strncpy(out, p + 1, len);
                out[len] = '\0';
                return;
            }
            p = q_end + 1;
        } else {
            p++;
        }
    }
}

static bool match_cmd_prefix(const char* s, const char* prefix, const char** out_rest) {
    size_t len = strlen(prefix);
    if (str_starts_with_ci(s, prefix)) {
        const char* after = s + len;
        if (*after == '\0' || *after == ':' || isspace((unsigned char)*after) || *after == '"' || *after == '\'') {
            while (isspace((unsigned char)*after)) after++;
            *out_rest = after;
            return true;
        }
    }
    return false;
}

bool enlngf_parse_file(EnlngfDoc* doc, const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return false;

    strncpy(doc->source_dir, filepath, sizeof(doc->source_dir) - 1);
    doc->source_dir[sizeof(doc->source_dir) - 1] = '\0';
    {
        char* s1 = strrchr(doc->source_dir, '/');
        char* s2 = strrchr(doc->source_dir, '\\');
        char* sep = (s1 > s2) ? s1 : s2;
        if (sep) *sep = '\0';
        else doc->source_dir[0] = '\0';
    }

    char line[4096];
    EnlngfNode* stack[MAX_STACK_DEPTH];
    int stack_top = 0;
    stack[0] = doc->root;

    int line_num = 0;
    int in_embedded_block = 0; /* 0 = none, 1 = styles, 2 = script */
    int embedded_indent = 0;

    while (fgets(line, sizeof(line), f)) {
        line_num++;
        int indent = 0;
        char* p = line;
        while (*p == ' ' || *p == '\t') {
            indent += (*p == '\t') ? 4 : 1;
            p++;
        }
        char* s = trim(p);
        if (!*s || s[0] == '#') continue;

        /* Handle embedded `styles:` or `in script:` blocks */
        if (in_embedded_block != 0) {
            if (indent > embedded_indent) {
                if (in_embedded_block == 1) {
                    /* Simple embedded style line: e.g. `font-family is "Arial"` or `.box:` */
                    if (s[strlen(s) - 1] == ':') {
                        if (doc->inline_css[0] && strchr(doc->inline_css, '{') && !strrchr(doc->inline_css, '}')) {
                            strncat(doc->inline_css, " }\n", sizeof(doc->inline_css) - strlen(doc->inline_css) - 1);
                        }
                        char sel[128];
                        strncpy(sel, s, sizeof(sel) - 1);
                        sel[sizeof(sel) - 1] = '\0';
                        sel[strlen(sel) - 1] = '\0';
                        char rule_open[160];
                        snprintf(rule_open, sizeof(rule_open), "    %s { ", sel);
                        strncat(doc->inline_css, rule_open, sizeof(doc->inline_css) - strlen(doc->inline_css) - 1);
                    } else {
                        char* is_pos = strstr(s, " is ");
                        if (is_pos) {
                            char prop[64] = {0}, val[128] = {0};
                            size_t plen = (size_t)(is_pos - s);
                            if (plen >= sizeof(prop)) plen = sizeof(prop) - 1;
                            strncpy(prop, s, plen);
                            extract_quoted(is_pos + 4, val, sizeof(val));
                            char decl[256];
                            snprintf(decl, sizeof(decl), "%s: %s; ", trim(prop), val);
                            if (!strchr(doc->inline_css, '{')) {
                                strncat(doc->inline_css, "    body { ", sizeof(doc->inline_css) - strlen(doc->inline_css) - 1);
                            }
                            strncat(doc->inline_css, decl, sizeof(doc->inline_css) - strlen(doc->inline_css) - 1);
                        }
                    }
                } else if (in_embedded_block == 2) {
                    strncat(doc->inline_js, "    // ", sizeof(doc->inline_js) - strlen(doc->inline_js) - 1);
                    strncat(doc->inline_js, s, sizeof(doc->inline_js) - strlen(doc->inline_js) - 1);
                    strncat(doc->inline_js, "\n", sizeof(doc->inline_js) - strlen(doc->inline_js) - 1);
                }
                continue;
            } else {
                if (in_embedded_block == 1 && doc->inline_css[0]) {
                    strncat(doc->inline_css, "}\n", sizeof(doc->inline_css) - strlen(doc->inline_css) - 1);
                }
                in_embedded_block = 0;
            }
        }

        if (str_equals_ci(s, "styles:") || str_equals_ci(s, "style:")) {
            in_embedded_block = 1;
            embedded_indent = indent;
            continue;
        }
        if (str_equals_ci(s, "in script:") || str_equals_ci(s, "script:")) {
            in_embedded_block = 2;
            embedded_indent = indent;
            continue;
        }

        /* Skip type headers */
        if (str_starts_with_ci(s, "type enlngf") || str_starts_with_ci(s, "type enlgf")) continue;

        /* Document root & head directives */
        if (str_starts_with_ci(s, "document enlgf") || str_starts_with_ci(s, "document enlngf") ||
            str_starts_with_ci(s, "document in english") || str_starts_with_ci(s, "document:") ||
            str_starts_with_ci(s, "make document") || str_starts_with_ci(s, "start document") ||
            str_starts_with_ci(s, "end document") || str_starts_with_ci(s, "finish document")) {
            continue;
        }
        if (str_starts_with_ci(s, "head:") || str_starts_with_ci(s, "inside head:") ||
            str_starts_with_ci(s, "in head:") || str_starts_with_ci(s, "finish head") ||
            str_starts_with_ci(s, "charset ") || str_starts_with_ci(s, "viewport responsive")) {
            continue;
        }
        if (str_starts_with_ci(s, "inside body:") || str_starts_with_ci(s, "in body:") ||
            str_starts_with_ci(s, "finish body") || str_equals_ci(s, "end") || str_starts_with_ci(s, "end ")) {
            continue;
        }

        /* Style 2 block closers: `finish header`, `finish navigation`, `finish main content`, `finish section`, `finish footer`, `finish card`, etc. */
        if (str_starts_with_ci(s, "finish ")) {
            if (stack_top > 0) stack_top--;
            continue;
        }

        /* Title directives at document level */
        if ((str_starts_with_ci(s, "title ") || str_starts_with_ci(s, "page title ")) && (stack_top == 0 || indent <= 8)) {
            const char* tp = str_starts_with_ci(s, "page title ") ? s + 11 : s + 6;
            char title_val[128];
            extract_quoted(tp, title_val, sizeof(title_val));
            if (title_val[0]) strncpy(doc->title, title_val, sizeof(doc->title) - 1);
            continue;
        }
        if (str_starts_with_ci(s, "description ") && (stack_top == 0 || indent <= 8)) {
            continue;
        }
        if (str_starts_with_ci(s, "create page ") || str_starts_with_ci(s, "page ")) {
            const char* tp = str_starts_with_ci(s, "create page ") ? s + 12 : s + 5;
            char title_val[128];
            extract_quoted(tp, title_val, sizeof(title_val));
            if (title_val[0]) strncpy(doc->title, title_val, sizeof(doc->title) - 1);
            continue;
        }

        /* Connect design / stylesheet */
        if (str_starts_with_ci(s, "connect design ") || str_starts_with_ci(s, "connect style ") ||
            str_starts_with_ci(s, "use theme ") || str_starts_with_ci(s, "use style ") ||
            str_starts_with_ci(s, "stylesheet from ") || str_starts_with_ci(s, "style from ") ||
            str_starts_with_ci(s, "link stylesheet ")) {
            char fpath[128] = {0};
            if (strchr(s, '"') || strchr(s, '\'')) {
                extract_quoted(s, fpath, sizeof(fpath));
            } else {
                const char* last_sp = strrchr(s, ' ');
                if (last_sp) {
                    strncpy(fpath, last_sp + 1, sizeof(fpath) - 1);
                }
            }
            normalize_link_asset(fpath, true);
            if (fpath[0] && doc->design_file_count < 16) {
                strncpy(doc->design_files[doc->design_file_count++], fpath, 127);
            }
            continue;
        }

        /* Connect script */
        if (str_starts_with_ci(s, "connect script ") || str_starts_with_ci(s, "use script ") ||
            str_starts_with_ci(s, "script from ") || str_starts_with_ci(s, "include script from ") ||
            str_starts_with_ci(s, "link script ")) {
            char fpath[128] = {0};
            if (strchr(s, '"') || strchr(s, '\'')) {
                extract_quoted(s, fpath, sizeof(fpath));
            } else {
                const char* last_sp = strrchr(s, ' ');
                if (last_sp) {
                    strncpy(fpath, last_sp + 1, sizeof(fpath) - 1);
                }
            }
            normalize_link_asset(fpath, false);
            if (fpath[0] && doc->script_file_count < 16) {
                strncpy(doc->script_files[doc->script_file_count++], fpath, 127);
            }
            continue;
        }

        /* Pop stack until parent indent is less than current indent */
        while (stack_top > 0 && stack[stack_top]->indent_level >= indent) {
            stack_top--;
        }
        EnlngfNode* parent = stack[stack_top];

        /* Quoted text literal directly inside parent (e.g. inside `pre:` block) */
        if (s[0] == '"' || s[0] == '\'') {
            char lit[2048] = {0};
            extract_immediate_quoted(s, lit, sizeof(lit));
            if (parent) {
                strncat(parent->text, lit, sizeof(parent->text) - strlen(parent->text) - 1);
            }
            continue;
        }

        /* Raw HTML passthrough lines (`<div ...>`, `</ul>`, etc.) */
        if (s[0] == '<') {
            EnlngfNode* raw_node = alloc_node(doc, "__raw__", indent);
            if (raw_node) {
                strncpy(raw_node->text, s, sizeof(raw_node->text) - 1);
                add_child(parent, raw_node);
            }
            continue;
        }

        /* `display "..."` / `show "..."` -> emit `<div class="enlng-log">...</div>` */
        if (str_starts_with_ci(s, "display ") || str_starts_with_ci(s, "show ")) {
            const char* rest = str_starts_with_ci(s, "display ") ? s + 8 : s + 5;
            EnlngfNode* log_node = alloc_node(doc, "div", indent);
            if (log_node) {
                add_attr(log_node, "class", "enlng-log");
                extract_quoted(rest, log_node->text, sizeof(log_node->text));
                add_child(parent, log_node);
            }
            continue;
        }

        /* Parse element details */
        char tag[32] = "";
        char text[2048] = {0};
        char id[128] = {0};
        char class_name[256] = {0};
        char target_href[256] = {0};
        char src_val[256] = {0};
        char alt_val[256] = {0};
        char hint[256] = {0};
        char inline_style[256] = {0};
        char title_attr[256] = {0};
        char type_attr[64] = {0};
        char name_attr[128] = {0};
        char value_attr[256] = {0};
        char rows_attr[32] = {0};
        char min_attr[32] = {0};
        char max_attr[32] = {0};
        char allow_attr[256] = {0};
        char allowfs_attr[32] = {0};
        char onclick_attr[256] = {0};
        char action_attr[256] = {0};
        char for_attr[128] = {0};
        char readonly_attr[32] = {0};
        char required_attr[32] = {0};
        char checked_attr[32] = {0};

        bool is_container = (s[strlen(s) - 1] == ':');
        const char* after_cmd = NULL;

        /* Extract attributes safely using word boundaries outside quotes */
        extract_attribute_match(s, "with id ", id, sizeof(id));
        if (!id[0]) extract_attribute_match(s, "and id ", id, sizeof(id));
        if (!id[0]) extract_attribute_match(s, "id ", id, sizeof(id));

        extract_attribute_match(s, "with class ", class_name, sizeof(class_name));
        if (!class_name[0]) extract_attribute_match(s, "class ", class_name, sizeof(class_name));
        if (strstr(s, "and class ")) {
            char extra_cls[128];
            extract_attribute_match(s, "and class ", extra_cls, sizeof(extra_cls));
            if (extra_cls[0]) add_class_token(class_name, sizeof(class_name), extra_cls);
        }

        extract_attribute_match(s, "with target ", target_href, sizeof(target_href));
        if (!target_href[0]) extract_attribute_match(s, "and target ", target_href, sizeof(target_href));
        if (!target_href[0]) extract_attribute_match(s, "target ", target_href, sizeof(target_href));
        if (!target_href[0]) extract_attribute_match(s, "pointing to ", target_href, sizeof(target_href));
        if (!target_href[0]) extract_attribute_match(s, "with href ", target_href, sizeof(target_href));
        if (!target_href[0]) extract_attribute_match(s, "and href ", target_href, sizeof(target_href));
        if (!target_href[0]) extract_attribute_match(s, "href ", target_href, sizeof(target_href));
        if (!target_href[0]) extract_attribute_match(s, "to ", target_href, sizeof(target_href));

        extract_attribute_match(s, "with src ", src_val, sizeof(src_val));
        if (!src_val[0]) extract_attribute_match(s, "and src ", src_val, sizeof(src_val));
        if (!src_val[0]) extract_attribute_match(s, "src ", src_val, sizeof(src_val));
        if (!src_val[0]) extract_attribute_match(s, "from ", src_val, sizeof(src_val));

        extract_attribute_match(s, "with alt ", alt_val, sizeof(alt_val));
        if (!alt_val[0]) extract_attribute_match(s, "and alt ", alt_val, sizeof(alt_val));
        if (!alt_val[0]) extract_attribute_match(s, "alt ", alt_val, sizeof(alt_val));

        extract_attribute_match(s, "with placeholder ", hint, sizeof(hint));
        if (!hint[0]) extract_attribute_match(s, "and placeholder ", hint, sizeof(hint));
        if (!hint[0]) extract_attribute_match(s, "placeholder ", hint, sizeof(hint));
        if (!hint[0]) extract_attribute_match(s, "showing hint ", hint, sizeof(hint));
        if (!hint[0]) extract_attribute_match(s, "with hint ", hint, sizeof(hint));
        if (!hint[0]) extract_attribute_match(s, "and hint ", hint, sizeof(hint));
        if (!hint[0]) extract_attribute_match(s, "hint ", hint, sizeof(hint));

        extract_attribute_match(s, "with style ", inline_style, sizeof(inline_style));
        if (!inline_style[0]) extract_attribute_match(s, "and style ", inline_style, sizeof(inline_style));
        if (!inline_style[0]) extract_attribute_match(s, "style ", inline_style, sizeof(inline_style));

        extract_attribute_match(s, "with title ", title_attr, sizeof(title_attr));
        if (!title_attr[0]) extract_attribute_match(s, "and title ", title_attr, sizeof(title_attr));

        extract_attribute_match(s, "with type ", type_attr, sizeof(type_attr));
        if (!type_attr[0]) extract_attribute_match(s, "and type ", type_attr, sizeof(type_attr));
        if (!type_attr[0]) extract_attribute_match(s, "type ", type_attr, sizeof(type_attr));

        extract_attribute_match(s, "with name ", name_attr, sizeof(name_attr));
        if (!name_attr[0]) extract_attribute_match(s, "and name ", name_attr, sizeof(name_attr));
        if (!name_attr[0]) extract_attribute_match(s, "named ", name_attr, sizeof(name_attr));
        if (!name_attr[0]) extract_attribute_match(s, "name ", name_attr, sizeof(name_attr));

        extract_attribute_match(s, "with value ", value_attr, sizeof(value_attr));
        if (!value_attr[0]) extract_attribute_match(s, "and value ", value_attr, sizeof(value_attr));
        if (!value_attr[0]) extract_attribute_match(s, "value ", value_attr, sizeof(value_attr));
        if (!value_attr[0]) extract_attribute_match(s, "default ", value_attr, sizeof(value_attr));

        extract_attribute_match(s, "with rows ", rows_attr, sizeof(rows_attr));
        if (!rows_attr[0]) extract_attribute_match(s, "and rows ", rows_attr, sizeof(rows_attr));
        if (!rows_attr[0]) extract_attribute_match(s, "rows ", rows_attr, sizeof(rows_attr));

        extract_attribute_match(s, "with min ", min_attr, sizeof(min_attr));
        if (!min_attr[0]) extract_attribute_match(s, "and min ", min_attr, sizeof(min_attr));
        if (!min_attr[0]) extract_attribute_match(s, "min ", min_attr, sizeof(min_attr));

        extract_attribute_match(s, "with max ", max_attr, sizeof(max_attr));
        if (!max_attr[0]) extract_attribute_match(s, "and max ", max_attr, sizeof(max_attr));
        if (!max_attr[0]) extract_attribute_match(s, "max ", max_attr, sizeof(max_attr));

        extract_attribute_match(s, "with allow ", allow_attr, sizeof(allow_attr));
        if (!allow_attr[0]) extract_attribute_match(s, "and allow ", allow_attr, sizeof(allow_attr));
        if (!allow_attr[0]) extract_attribute_match(s, "allow ", allow_attr, sizeof(allow_attr));

        extract_attribute_match(s, "with allowfullscreen ", allowfs_attr, sizeof(allowfs_attr));
        if (!allowfs_attr[0]) extract_attribute_match(s, "and allowfullscreen ", allowfs_attr, sizeof(allowfs_attr));
        if (!allowfs_attr[0]) extract_attribute_match(s, "allowfullscreen ", allowfs_attr, sizeof(allowfs_attr));

        extract_attribute_match(s, "when clicked ", onclick_attr, sizeof(onclick_attr));
        if (!onclick_attr[0]) extract_attribute_match(s, "on click ", onclick_attr, sizeof(onclick_attr));
        if (!onclick_attr[0]) extract_attribute_match(s, "onclick ", onclick_attr, sizeof(onclick_attr));

        extract_attribute_match(s, "with action ", action_attr, sizeof(action_attr));
        if (!action_attr[0]) extract_attribute_match(s, "submitting to ", action_attr, sizeof(action_attr));
        if (!action_attr[0]) extract_attribute_match(s, "submit to ", action_attr, sizeof(action_attr));
        if (!action_attr[0]) extract_attribute_match(s, "action ", action_attr, sizeof(action_attr));

        extract_attribute_match(s, "for ", for_attr, sizeof(for_attr));

        extract_attribute_match(s, "with readonly ", readonly_attr, sizeof(readonly_attr));
        if (!readonly_attr[0]) extract_attribute_match(s, "and readonly ", readonly_attr, sizeof(readonly_attr));
        if (!readonly_attr[0]) extract_attribute_match(s, "readonly ", readonly_attr, sizeof(readonly_attr));

        extract_attribute_match(s, "with required ", required_attr, sizeof(required_attr));
        if (!required_attr[0]) extract_attribute_match(s, "and required ", required_attr, sizeof(required_attr));
        if (!required_attr[0]) extract_attribute_match(s, "required ", required_attr, sizeof(required_attr));

        extract_attribute_match(s, "with checked ", checked_attr, sizeof(checked_attr));
        if (!checked_attr[0]) extract_attribute_match(s, "and checked ", checked_attr, sizeof(checked_attr));
        if (!checked_attr[0]) extract_attribute_match(s, "checked ", checked_attr, sizeof(checked_attr));

        /* Inline CSS phrase extraction */
        if (strstr(s, "with text-align center")) {
            if (inline_style[0]) strncat(inline_style, "; ", sizeof(inline_style) - strlen(inline_style) - 1);
            strncat(inline_style, "text-align: center", sizeof(inline_style) - strlen(inline_style) - 1);
        }
        if (strstr(s, "with width 100 percent")) {
            if (inline_style[0]) strncat(inline_style, "; ", sizeof(inline_style) - strlen(inline_style) - 1);
            strncat(inline_style, "width: 100%", sizeof(inline_style) - strlen(inline_style) - 1);
        }
        {
            char val_buf[64];
            extract_attribute_match(s, "with color ", val_buf, sizeof(val_buf));
            if (!val_buf[0]) extract_attribute_match(s, "and color ", val_buf, sizeof(val_buf));
            if (!val_buf[0]) extract_attribute_match(s, "color ", val_buf, sizeof(val_buf));
            if (val_buf[0]) {
                if (inline_style[0]) strncat(inline_style, "; ", sizeof(inline_style) - strlen(inline_style) - 1);
                strncat(inline_style, "color: ", sizeof(inline_style) - strlen(inline_style) - 1);
                strncat(inline_style, val_buf, sizeof(inline_style) - strlen(inline_style) - 1);
            }
            extract_attribute_match(s, "with background ", val_buf, sizeof(val_buf));
            if (val_buf[0]) {
                if (inline_style[0]) strncat(inline_style, "; ", sizeof(inline_style) - strlen(inline_style) - 1);
                strncat(inline_style, "background: ", sizeof(inline_style) - strlen(inline_style) - 1);
                strncat(inline_style, val_buf, sizeof(inline_style) - strlen(inline_style) - 1);
            }
        }

        /* 1. Containers & Layout Primitives */
        if (match_cmd_prefix(s, "create application", &after_cmd) || match_cmd_prefix(s, "application", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "application");
            char app_title[128] = {0};
            extract_immediate_quoted(after_cmd, app_title, sizeof(app_title));
            if (app_title[0] && strcmp(doc->title, "Enlangg Sovereign Web Studio") == 0) {
                strncpy(doc->title, app_title, sizeof(doc->title) - 1);
            }
            char cont_name[64] = {0};
            extract_attribute_match(s, "container ", cont_name, sizeof(cont_name));
            if (cont_name[0]) {
                if (!id[0]) strncpy(id, cont_name, sizeof(id) - 1);
                add_class_token(class_name, sizeof(class_name), cont_name);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create header", &after_cmd) || match_cmd_prefix(s, "inside header", &after_cmd) ||
                   match_cmd_prefix(s, "header", &after_cmd)) {
            strcpy(tag, "header");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create footer", &after_cmd) || match_cmd_prefix(s, "inside footer", &after_cmd) ||
                   match_cmd_prefix(s, "footer", &after_cmd)) {
            strcpy(tag, "footer");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create main", &after_cmd) || match_cmd_prefix(s, "inside main content", &after_cmd) ||
                   match_cmd_prefix(s, "inside main", &after_cmd) || match_cmd_prefix(s, "main content", &after_cmd) ||
                   match_cmd_prefix(s, "main", &after_cmd)) {
            strcpy(tag, "main");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create action_group", &after_cmd) || match_cmd_prefix(s, "action_group", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "action-group");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create container", &after_cmd) || match_cmd_prefix(s, "inside container", &after_cmd) ||
                   match_cmd_prefix(s, "container", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "container");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create screen", &after_cmd) || match_cmd_prefix(s, "screen", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "screen");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create grid", &after_cmd) || match_cmd_prefix(s, "inside grid", &after_cmd) ||
                   match_cmd_prefix(s, "grid", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "grid");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create row", &after_cmd) || match_cmd_prefix(s, "inside row", &after_cmd) ||
                   match_cmd_prefix(s, "row", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "row");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create column", &after_cmd) || match_cmd_prefix(s, "column", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "column");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create card", &after_cmd) || match_cmd_prefix(s, "inside card", &after_cmd) ||
                   match_cmd_prefix(s, "card", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "card");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create box", &after_cmd) || match_cmd_prefix(s, "inside box", &after_cmd) ||
                   match_cmd_prefix(s, "box", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "box");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create panel", &after_cmd) || match_cmd_prefix(s, "panel", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "panel");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create controls", &after_cmd) || match_cmd_prefix(s, "controls", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "controls");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create navbar", &after_cmd) || match_cmd_prefix(s, "inside navigation", &after_cmd) ||
                   match_cmd_prefix(s, "navigation", &after_cmd) || match_cmd_prefix(s, "navbar", &after_cmd) ||
                   match_cmd_prefix(s, "add navbar", &after_cmd) || match_cmd_prefix(s, "nav", &after_cmd)) {
            strcpy(tag, "nav");
            if (str_starts_with_ci(s, "create navbar") || str_starts_with_ci(s, "navbar") || str_starts_with_ci(s, "add navbar")) {
                add_class_token(class_name, sizeof(class_name), "navbar");
            }
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create nav_menu", &after_cmd) || match_cmd_prefix(s, "nav_menu", &after_cmd)) {
            strcpy(tag, "ul");
            add_class_token(class_name, sizeof(class_name), "nav-menu");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "create section", &after_cmd) || match_cmd_prefix(s, "inside section", &after_cmd) ||
                   match_cmd_prefix(s, "section", &after_cmd)) {
            strcpy(tag, "section");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "body", &after_cmd)) {
            if (class_name[0]) add_attr(doc->root, "class", class_name);
            if (id[0]) add_attr(doc->root, "id", id);
            doc->root->indent_level = indent;
            stack[0] = doc->root;
            stack_top = 0;
            continue;
        } else if (match_cmd_prefix(s, "aside", &after_cmd) || match_cmd_prefix(s, "create aside", &after_cmd) ||
                   match_cmd_prefix(s, "sidebar", &after_cmd)) {
            strcpy(tag, "aside");
            is_container = true;
        } else if (match_cmd_prefix(s, "article", &after_cmd) || match_cmd_prefix(s, "create article", &after_cmd)) {
            strcpy(tag, "article");
            is_container = true;
        } else if (match_cmd_prefix(s, "form", &after_cmd) || match_cmd_prefix(s, "create form", &after_cmd) ||
                   match_cmd_prefix(s, "inside form", &after_cmd)) {
            strcpy(tag, "form");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
            is_container = true;
        } else if (match_cmd_prefix(s, "ul", &after_cmd) || match_cmd_prefix(s, "unordered list", &after_cmd) ||
                   match_cmd_prefix(s, "bullet list", &after_cmd) || match_cmd_prefix(s, "list", &after_cmd)) {
            strcpy(tag, "ul");
            is_container = true;
        } else if (match_cmd_prefix(s, "ol", &after_cmd) || match_cmd_prefix(s, "ordered list", &after_cmd)) {
            strcpy(tag, "ol");
            is_container = true;
        } else if (match_cmd_prefix(s, "li", &after_cmd) || match_cmd_prefix(s, "add item", &after_cmd) ||
                   match_cmd_prefix(s, "list item", &after_cmd) || match_cmd_prefix(s, "item", &after_cmd)) {
            strcpy(tag, "li");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "table", &after_cmd) || match_cmd_prefix(s, "create table", &after_cmd)) {
            strcpy(tag, "table");
            is_container = true;
        } else if (match_cmd_prefix(s, "thead", &after_cmd) || match_cmd_prefix(s, "table head", &after_cmd) ||
                   match_cmd_prefix(s, "header row", &after_cmd)) {
            strcpy(tag, "thead");
            is_container = true;
        } else if (match_cmd_prefix(s, "tbody", &after_cmd) || match_cmd_prefix(s, "table body", &after_cmd) ||
                   match_cmd_prefix(s, "body rows", &after_cmd)) {
            strcpy(tag, "tbody");
            is_container = true;
        } else if (match_cmd_prefix(s, "tr", &after_cmd) || match_cmd_prefix(s, "table row", &after_cmd)) {
            strcpy(tag, "tr");
            is_container = true;
        } else if (match_cmd_prefix(s, "th", &after_cmd) || match_cmd_prefix(s, "column heading", &after_cmd) ||
                   match_cmd_prefix(s, "table header", &after_cmd)) {
            strcpy(tag, "th");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "td", &after_cmd) || match_cmd_prefix(s, "cell", &after_cmd) ||
                   match_cmd_prefix(s, "table cell", &after_cmd)) {
            strcpy(tag, "td");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "div", &after_cmd) || match_cmd_prefix(s, "create div", &after_cmd)) {
            strcpy(tag, "div");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        }

        /* 2. Headings (h1 - h6) */
        else if (match_cmd_prefix(s, "add heading 1", &after_cmd) || match_cmd_prefix(s, "heading 1", &after_cmd) ||
                 match_cmd_prefix(s, "h1", &after_cmd)) {
            strcpy(tag, "h1");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add heading 2", &after_cmd) || match_cmd_prefix(s, "heading 2", &after_cmd) ||
                   match_cmd_prefix(s, "h2", &after_cmd)) {
            strcpy(tag, "h2");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add heading 3", &after_cmd) || match_cmd_prefix(s, "heading 3", &after_cmd) ||
                   match_cmd_prefix(s, "h3", &after_cmd)) {
            strcpy(tag, "h3");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add heading 4", &after_cmd) || match_cmd_prefix(s, "heading 4", &after_cmd) ||
                   match_cmd_prefix(s, "h4", &after_cmd)) {
            strcpy(tag, "h4");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add heading 5", &after_cmd) || match_cmd_prefix(s, "heading 5", &after_cmd) ||
                   match_cmd_prefix(s, "h5", &after_cmd)) {
            strcpy(tag, "h5");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add heading 6", &after_cmd) || match_cmd_prefix(s, "heading 6", &after_cmd) ||
                   match_cmd_prefix(s, "h6", &after_cmd)) {
            strcpy(tag, "h6");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add title", &after_cmd) || match_cmd_prefix(s, "title", &after_cmd)) {
            strcpy(tag, "h2");
            add_class_token(class_name, sizeof(class_name), "title");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add heading", &after_cmd) || match_cmd_prefix(s, "heading", &after_cmd)) {
            strcpy(tag, "h2");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        }

        /* 3. Text & Semantic Elements */
        else if (match_cmd_prefix(s, "add subtitle", &after_cmd) || match_cmd_prefix(s, "subtitle", &after_cmd)) {
            strcpy(tag, "p");
            add_class_token(class_name, sizeof(class_name), "subtitle");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add description", &after_cmd) || match_cmd_prefix(s, "description", &after_cmd)) {
            strcpy(tag, "p");
            add_class_token(class_name, sizeof(class_name), "description");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add copyright", &after_cmd) || match_cmd_prefix(s, "copyright", &after_cmd)) {
            strcpy(tag, "p");
            add_class_token(class_name, sizeof(class_name), "copyright");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add paragraph", &after_cmd) || match_cmd_prefix(s, "paragraph", &after_cmd) ||
                   match_cmd_prefix(s, "p", &after_cmd)) {
            strcpy(tag, "p");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "span", &after_cmd) || match_cmd_prefix(s, "add span", &after_cmd) ||
                   match_cmd_prefix(s, "add text", &after_cmd) || match_cmd_prefix(s, "text", &after_cmd)) {
            strcpy(tag, "span");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add tag", &after_cmd) || match_cmd_prefix(s, "tag", &after_cmd)) {
            strcpy(tag, "span");
            add_class_token(class_name, sizeof(class_name), "tag");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add icon", &after_cmd) || match_cmd_prefix(s, "icon", &after_cmd)) {
            strcpy(tag, "span");
            add_class_token(class_name, sizeof(class_name), "icon");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add contact_item", &after_cmd) || match_cmd_prefix(s, "contact_item", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "contact-item");
            char label_txt[128] = {0};
            extract_immediate_quoted(after_cmd, label_txt, sizeof(label_txt));
            if (label_txt[0] && value_attr[0]) {
                snprintf(text, sizeof(text), "%s: %s", label_txt, value_attr);
                value_attr[0] = '\0';
            } else {
                extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
            }
        } else if (match_cmd_prefix(s, "strong", &after_cmd) || match_cmd_prefix(s, "bold", &after_cmd)) {
            strcpy(tag, "strong");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "em", &after_cmd) || match_cmd_prefix(s, "italic", &after_cmd)) {
            strcpy(tag, "em");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "code", &after_cmd) || match_cmd_prefix(s, "add code", &after_cmd)) {
            strcpy(tag, "code");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "pre", &after_cmd) || match_cmd_prefix(s, "code block", &after_cmd)) {
            strcpy(tag, "pre");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add badge", &after_cmd) || match_cmd_prefix(s, "badge", &after_cmd)) {
            strcpy(tag, "span");
            add_class_token(class_name, sizeof(class_name), "badge");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add readout", &after_cmd) || match_cmd_prefix(s, "readout", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "readout");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add note", &after_cmd) || match_cmd_prefix(s, "note", &after_cmd)) {
            strcpy(tag, "p");
            add_class_token(class_name, sizeof(class_name), "note");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add value", &after_cmd) || match_cmd_prefix(s, "value", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "value");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add label", &after_cmd) || match_cmd_prefix(s, "label", &after_cmd)) {
            if (for_attr[0] || (parent && strcmp(parent->tag, "form") == 0)) {
                strcpy(tag, "label");
            } else {
                strcpy(tag, "div");
                add_class_token(class_name, sizeof(class_name), "label");
            }
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add logo", &after_cmd) || match_cmd_prefix(s, "logo", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "brand-logo");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add nav_link", &after_cmd) || match_cmd_prefix(s, "nav_link", &after_cmd) ||
                   match_cmd_prefix(s, "add link", &after_cmd) || match_cmd_prefix(s, "link", &after_cmd) ||
                   match_cmd_prefix(s, "a", &after_cmd)) {
            strcpy(tag, "a");
            if (str_starts_with_ci(s, "add nav_link") || str_starts_with_ci(s, "nav_link")) {
                add_class_token(class_name, sizeof(class_name), "nav-link");
            }
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add section_tag", &after_cmd) || match_cmd_prefix(s, "section_tag", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "section-tag");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add section_title", &after_cmd) || match_cmd_prefix(s, "section_title", &after_cmd)) {
            strcpy(tag, "h2");
            add_class_token(class_name, sizeof(class_name), "section-title");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add speech_bubble", &after_cmd) || match_cmd_prefix(s, "speech_bubble", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "speech-bubble");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add viewport", &after_cmd) || match_cmd_prefix(s, "viewport", &after_cmd)) {
            strcpy(tag, "div");
            add_class_token(class_name, sizeof(class_name), "viewport");
            char name[64] = {0};
            extract_immediate_quoted(after_cmd, name, sizeof(name));
            if (name[0]) {
                add_class_token(class_name, sizeof(class_name), name);
                if (!id[0]) strncpy(id, name, sizeof(id) - 1);
            }
        }

        /* 4. Media, Embeds, Canvas, Interactive & Form Controls */
        else if (match_cmd_prefix(s, "add image", &after_cmd) || match_cmd_prefix(s, "image", &after_cmd) ||
                 match_cmd_prefix(s, "add img", &after_cmd) || match_cmd_prefix(s, "img", &after_cmd)) {
            strcpy(tag, "img");
            if (!src_val[0]) extract_immediate_quoted(after_cmd, src_val, sizeof(src_val));
        } else if (match_cmd_prefix(s, "add iframe", &after_cmd) || match_cmd_prefix(s, "iframe", &after_cmd) ||
                   match_cmd_prefix(s, "add embed", &after_cmd) || match_cmd_prefix(s, "embed", &after_cmd)) {
            strcpy(tag, "iframe");
            if (!src_val[0]) extract_immediate_quoted(after_cmd, src_val, sizeof(src_val));
        } else if (match_cmd_prefix(s, "add video", &after_cmd) || match_cmd_prefix(s, "video", &after_cmd)) {
            strcpy(tag, "video");
            if (!src_val[0]) extract_immediate_quoted(after_cmd, src_val, sizeof(src_val));
        } else if (match_cmd_prefix(s, "add audio", &after_cmd) || match_cmd_prefix(s, "audio", &after_cmd)) {
            strcpy(tag, "audio");
            if (!src_val[0]) extract_immediate_quoted(after_cmd, src_val, sizeof(src_val));
        } else if (match_cmd_prefix(s, "add canvas", &after_cmd) || match_cmd_prefix(s, "canvas", &after_cmd)) {
            strcpy(tag, "canvas");
        } else if (match_cmd_prefix(s, "add textarea_field", &after_cmd) || match_cmd_prefix(s, "textarea_field", &after_cmd) ||
                   match_cmd_prefix(s, "add textarea", &after_cmd) || match_cmd_prefix(s, "textarea", &after_cmd) ||
                   match_cmd_prefix(s, "add text area", &after_cmd) || match_cmd_prefix(s, "text area", &after_cmd)) {
            strcpy(tag, "textarea");
            if (str_starts_with_ci(s, "add textarea_field") || str_starts_with_ci(s, "textarea_field")) {
                if (!hint[0]) extract_immediate_quoted(after_cmd, hint, sizeof(hint));
            } else {
                extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
            }
        } else if (match_cmd_prefix(s, "add select", &after_cmd) || match_cmd_prefix(s, "select", &after_cmd) ||
                   match_cmd_prefix(s, "add dropdown", &after_cmd) || match_cmd_prefix(s, "dropdown", &after_cmd)) {
            strcpy(tag, "select");
            is_container = true;
        } else if (match_cmd_prefix(s, "add option", &after_cmd) || match_cmd_prefix(s, "option", &after_cmd)) {
            strcpy(tag, "option");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add submit button", &after_cmd) || match_cmd_prefix(s, "submit button", &after_cmd)) {
            strcpy(tag, "input");
            add_class_token(class_name, sizeof(class_name), "btn");
            strcpy(type_attr, "submit");
            extract_unclaimed_quoted_text(after_cmd, value_attr, sizeof(value_attr));
        } else if (match_cmd_prefix(s, "add button", &after_cmd) || match_cmd_prefix(s, "button", &after_cmd)) {
            strcpy(tag, "button");
            extract_unclaimed_quoted_text(after_cmd, text, sizeof(text));
        } else if (match_cmd_prefix(s, "add input_field", &after_cmd) || match_cmd_prefix(s, "input_field", &after_cmd)) {
            strcpy(tag, "input");
            if (!type_attr[0]) strcpy(type_attr, "text");
            if (!hint[0]) extract_immediate_quoted(after_cmd, hint, sizeof(hint));
        } else if (match_cmd_prefix(s, "add email input", &after_cmd) || match_cmd_prefix(s, "email input", &after_cmd)) {
            strcpy(tag, "input");
            strcpy(type_attr, "email");
        } else if (match_cmd_prefix(s, "add text input", &after_cmd) || match_cmd_prefix(s, "text input", &after_cmd) ||
                   match_cmd_prefix(s, "add input", &after_cmd) || match_cmd_prefix(s, "input", &after_cmd)) {
            strcpy(tag, "input");
            if (!type_attr[0]) strcpy(type_attr, "text");
        } else if (match_cmd_prefix(s, "add slider", &after_cmd) || match_cmd_prefix(s, "slider", &after_cmd)) {
            strcpy(tag, "input");
            add_class_token(class_name, sizeof(class_name), "slider");
            strcpy(type_attr, "range");
        } else if (match_cmd_prefix(s, "br", &after_cmd) || match_cmd_prefix(s, "line break", &after_cmd)) {
            strcpy(tag, "br");
        } else if (match_cmd_prefix(s, "hr", &after_cmd) || match_cmd_prefix(s, "divider", &after_cmd) ||
                   match_cmd_prefix(s, "horizontal rule", &after_cmd)) {
            strcpy(tag, "hr");
        }

        /* Unknown element -> report line-numbered error and fail! */
        if (tag[0] == '\0') {
            fprintf(stderr, "SyntaxError: Line %d: Unknown element or statement: '%s'\n", line_num, s);
            fclose(f);
            return false;
        }

        /* Clean text if it was parsed as syntax residual */
        if (text[0] == ':' || str_starts_with_ci(text, "with ") || str_starts_with_ci(text, "class ") || str_starts_with_ci(text, "id ")) {
            text[0] = '\0';
        }

        /* Allocate node */
        EnlngfNode* node = alloc_node(doc, tag, indent);
        if (!node) {
            fprintf(stderr, "Error: Line %d: Maximum DOM node limit (%d) exceeded.\n", line_num, MAX_NODES);
            fclose(f);
            return false;
        }

        if (text[0]) strncpy(node->text, text, sizeof(node->text) - 1);
        if (id[0]) add_attr(node, "id", id);
        if (class_name[0]) add_attr(node, "class", class_name);
        if (target_href[0]) add_attr(node, "href", target_href);
        if (src_val[0]) add_attr(node, "src", src_val);
        if (alt_val[0]) add_attr(node, "alt", alt_val);
        if (hint[0]) add_attr(node, "placeholder", hint);
        if (inline_style[0]) add_attr(node, "style", inline_style);
        if (title_attr[0]) add_attr(node, "title", title_attr);
        if (type_attr[0]) add_attr(node, "type", type_attr);
        if (name_attr[0]) add_attr(node, "name", name_attr);
        if (value_attr[0]) add_attr(node, "value", value_attr);
        if (rows_attr[0]) add_attr(node, "rows", rows_attr);
        if (min_attr[0]) add_attr(node, "min", min_attr);
        if (max_attr[0]) add_attr(node, "max", max_attr);
        if (allow_attr[0]) add_attr(node, "allow", allow_attr);
        if (allowfs_attr[0]) add_attr(node, "allowfullscreen", allowfs_attr);
        if (onclick_attr[0]) add_attr(node, "onclick", onclick_attr);
        if (action_attr[0]) add_attr(node, "action", action_attr);
        if (for_attr[0]) add_attr(node, "for", for_attr);
        if (readonly_attr[0]) add_attr(node, "readonly", readonly_attr);
        if (required_attr[0]) add_attr(node, "required", required_attr);
        if (checked_attr[0]) add_attr(node, "checked", checked_attr);

        add_child(parent, node);

        if (is_container) {
            if (stack_top + 1 >= MAX_STACK_DEPTH) {
                fprintf(stderr, "Error: Line %d: Maximum nesting depth (%d) exceeded.\n", line_num, MAX_STACK_DEPTH);
                fclose(f);
                return false;
            }
            stack[++stack_top] = node;
        }
    }

    if (in_embedded_block == 1 && doc->inline_css[0]) {
        strncat(doc->inline_css, "}\n", sizeof(doc->inline_css) - strlen(doc->inline_css) - 1);
    }

    fclose(f);
    return true;
}

static void emit_node_html(const EnlngfNode* node, int depth, FILE* fout) {
    if (!node) return;
    char indent[128];
    int pad = depth * 2;
    if (pad > 120) pad = 120;
    for (int i = 0; i < pad; i++) indent[i] = ' ';
    indent[pad] = '\0';

    if (strcmp(node->tag, "__raw__") == 0) {
        fprintf(fout, "%s%s\n", indent, node->text);
        return;
    }

    fprintf(fout, "%s<%s", indent, node->tag);
    for (int i = 0; i < node->attr_count; i++) {
        fprintf(fout, " %s=\"%s\"", node->attrs[i].key, node->attrs[i].value);
    }

    if (node->is_self_closing) {
        fprintf(fout, ">\n");
        return;
    }

    if (node->child_count == 0) {
        fprintf(fout, ">%s</%s>\n", node->text, node->tag);
        return;
    }

    fprintf(fout, ">\n");
    if (node->text[0] != '\0') {
        fprintf(fout, "%s  %s\n", indent, node->text);
    }
    for (int c = 0; c < node->child_count; c++) {
        emit_node_html(node->children[c], depth + 1, fout);
    }
    fprintf(fout, "%s</%s>\n", indent, node->tag);
}

static bool file_exists_local(const char* path) {
    if (!path || !*path) return false;
    FILE* f = fopen(path, "rb");
    if (f) { fclose(f); return true; }
    return false;
}

/* ========================================================================= */
/* Pure C SHA-256 Implementation (FIPS 180-2 compliant, zero dependencies)  */
/* ========================================================================= */
#include <stdint.h>

#ifdef _WIN32
#define enlngf_strncasecmp _strnicmp
#else
#define enlngf_strncasecmp strncasecmp
#endif

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];
} EnlngfSha256;

#define ENLNGF_ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define ENLNGF_CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define ENLNGF_MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define ENLNGF_EP0(x) (ENLNGF_ROTR(x, 2) ^ ENLNGF_ROTR(x, 13) ^ ENLNGF_ROTR(x, 22))
#define ENLNGF_EP1(x) (ENLNGF_ROTR(x, 6) ^ ENLNGF_ROTR(x, 11) ^ ENLNGF_ROTR(x, 25))
#define ENLNGF_SIG0(x) (ENLNGF_ROTR(x, 7) ^ ENLNGF_ROTR(x, 18) ^ ((x) >> 3))
#define ENLNGF_SIG1(x) (ENLNGF_ROTR(x, 17) ^ ENLNGF_ROTR(x, 19) ^ ((x) >> 10))

static const uint32_t ENLNGF_K256[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
    0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
    0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
    0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
    0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
    0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
    0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
    0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
    0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
    0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
    0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
    0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
    0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

static void enlngf_sha256_transform(EnlngfSha256* ctx, const uint8_t data[64]) {
    uint32_t a, b, c, d, e, f, g, h, m[64];
    for (int i = 0, j = 0; i < 16; ++i, j += 4) {
        m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j + 1] << 16) |
               ((uint32_t)data[j + 2] << 8) | ((uint32_t)data[j + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        m[i] = ENLNGF_SIG1(m[i - 2]) + m[i - 7] + ENLNGF_SIG0(m[i - 15]) + m[i - 16];
    }
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];
    for (int i = 0; i < 64; ++i) {
        uint32_t t1 = h + ENLNGF_EP1(e) + ENLNGF_CH(e, f, g) + ENLNGF_K256[i] + m[i];
        uint32_t t2 = ENLNGF_EP0(a) + ENLNGF_MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void enlngf_sha256_init(EnlngfSha256* ctx) {
    ctx->state[0] = 0x6a09e667UL;
    ctx->state[1] = 0xbb67ae85UL;
    ctx->state[2] = 0x3c6ef372UL;
    ctx->state[3] = 0xa54ff53aUL;
    ctx->state[4] = 0x510e527fUL;
    ctx->state[5] = 0x9b05688cUL;
    ctx->state[6] = 0x1f83d9abUL;
    ctx->state[7] = 0x5be0cd19UL;
    ctx->count = 0;
}

static void enlngf_sha256_update(EnlngfSha256* ctx, const uint8_t* data, size_t len) {
    size_t i = 0;
    size_t index = (size_t)((ctx->count >> 3) & 0x3f);
    ctx->count += (uint64_t)len << 3;
    size_t part_len = 64 - index;
    if (len >= part_len) {
        memcpy(&ctx->buffer[index], data, part_len);
        enlngf_sha256_transform(ctx, ctx->buffer);
        for (i = part_len; i + 63 < len; i += 64) {
            enlngf_sha256_transform(ctx, &data[i]);
        }
        index = 0;
    }
    memcpy(&ctx->buffer[index], &data[i], len - i);
}

static void enlngf_sha256_final(EnlngfSha256* ctx, uint8_t hash[32]) {
    uint8_t bits[8];
    for (int i = 0; i < 8; ++i) {
        bits[i] = (uint8_t)((ctx->count >> ((7 - i) * 8)) & 0xff);
    }
    size_t index = (size_t)((ctx->count >> 3) & 0x3f);
    size_t pad_len = (index < 56) ? (56 - index) : (120 - index);
    static const uint8_t PADDING[64] = { 0x80 };
    enlngf_sha256_update(ctx, PADDING, pad_len);
    enlngf_sha256_update(ctx, bits, 8);
    for (int i = 0; i < 8; ++i) {
        hash[i * 4]     = (uint8_t)((ctx->state[i] >> 24) & 0xff);
        hash[i * 4 + 1] = (uint8_t)((ctx->state[i] >> 16) & 0xff);
        hash[i * 4 + 2] = (uint8_t)((ctx->state[i] >> 8) & 0xff);
        hash[i * 4 + 3] = (uint8_t)(ctx->state[i] & 0xff);
    }
}

static void enlngf_sha256_hex(const uint8_t* data, size_t len, char hex_out[65]) {
    EnlngfSha256 ctx;
    enlngf_sha256_init(&ctx);
    enlngf_sha256_update(&ctx, data, len);
    uint8_t h[32];
    enlngf_sha256_final(&ctx, h);
    for (int i = 0; i < 32; ++i) {
        sprintf(hex_out + (i * 2), "%02x", h[i]);
    }
    hex_out[64] = '\0';
}

static bool is_valid_enlngf_generated_asset(const char* path) {
    if (!path || !*path) return false;
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) {
        fclose(f);
        return false;
    }
    char* buf = (char*)malloc(sz + 1);
    if (!buf) {
        fclose(f);
        return false;
    }
    size_t rd = fread(buf, 1, sz, f);
    fclose(f);
    buf[rd] = '\0';

    const char MARKER_PREFIX[] = "/* enlngf-generated: ";
    size_t prefix_len = strlen(MARKER_PREFIX);
    if (rd < prefix_len + 64 + 3) {
        free(buf);
        return false;
    }
    if (strncmp(buf, MARKER_PREFIX, prefix_len) != 0) {
        free(buf);
        return false;
    }
    const char* expected_hash = buf + prefix_len;
    for (int i = 0; i < 64; ++i) {
        char ch = expected_hash[i];
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))) {
            free(buf);
            return false;
        }
    }
    if (strncmp(expected_hash + 64, " */", 3) != 0) {
        free(buf);
        return false;
    }

    const char* newline = strchr(expected_hash + 67, '\n');
    const char* rest = newline ? newline + 1 : "";
    size_t rest_len = newline ? (rd - (size_t)(rest - buf)) : 0;

    char computed_hash[65];
    enlngf_sha256_hex((const uint8_t*)rest, rest_len, computed_hash);

    bool match = (enlngf_strncasecmp(expected_hash, computed_hash, 64) == 0);
    free(buf);
    return match;
}

static bool stamp_enlngf_generated_marker(const char* path) {
    if (!path || !*path) return false;
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(sz + 1);
    if (!buf) {
        fclose(f);
        return false;
    }
    size_t rd = fread(buf, 1, sz, f);
    fclose(f);
    buf[rd] = '\0';

    const char MARKER_PREFIX[] = "/* enlngf-generated: ";
    size_t prefix_len = strlen(MARKER_PREFIX);
    const char* content = buf;
    size_t content_len = rd;
    if (rd >= prefix_len + 67 && strncmp(buf, MARKER_PREFIX, prefix_len) == 0) {
        const char* end_marker = strstr(buf, "*/");
        if (end_marker) {
            const char* nl = strchr(end_marker, '\n');
            if (nl) {
                content = nl + 1;
                content_len = rd - (size_t)(content - buf);
            }
        }
    }

    char hash[65];
    enlngf_sha256_hex((const uint8_t*)content, content_len, hash);

    f = fopen(path, "wb");
    if (!f) {
        free(buf);
        return false;
    }
    fprintf(f, "/* enlngf-generated: %s */\n", hash);
    if (content_len > 0) {
        fwrite(content, 1, content_len, f);
    }
    fclose(f);
    free(buf);
    return true;
}

static bool auto_compile_companion_assets(const EnlngfDoc* doc, const char* src_dir, const char* out_dir) {
    char bin_dir[512] = {0};
#ifdef _WIN32
    char exe_path[512] = {0};
    if (GetModuleFileNameA(NULL, exe_path, sizeof(exe_path)) > 0) {
        strncpy(bin_dir, exe_path, sizeof(bin_dir) - 1);
        char* slash = strrchr(bin_dir, '\\');
        if (!slash) slash = strrchr(bin_dir, '/');
        if (slash) *slash = '\0';
        else bin_dir[0] = '\0';
    }
#endif

    /* 1. Auto-compile linked stylesheets if companion .enlngd/.enlgd exists */
    for (int i = 0; i < doc->design_file_count; i++) {
        char out_css[512];
        if (out_dir && out_dir[0]) snprintf(out_css, sizeof(out_css), "%s/%s", out_dir, doc->design_files[i]);
        else snprintf(out_css, sizeof(out_css), "%s", doc->design_files[i]);

        char stem_name[256];
        strncpy(stem_name, doc->design_files[i], sizeof(stem_name) - 1);
        stem_name[sizeof(stem_name) - 1] = '\0';
        char* dot = strrchr(stem_name, '.');
        if (dot) *dot = '\0';

        char c1[540] = {0}, c2[540] = {0}, c3[540] = {0}, c4[540] = {0}, c5[540] = {0}, c6[540] = {0};
        if (src_dir && src_dir[0]) {
            snprintf(c1, sizeof(c1), "%s/%s.enlngd", src_dir, stem_name);
            snprintf(c2, sizeof(c2), "%s/%s.enlgd", src_dir, stem_name);
        }
        if (out_dir && out_dir[0]) {
            snprintf(c3, sizeof(c3), "%s/%s.enlngd", out_dir, stem_name);
            snprintf(c4, sizeof(c4), "%s/%s.enlgd", out_dir, stem_name);
        }
        snprintf(c5, sizeof(c5), "%s.enlngd", stem_name);
        snprintf(c6, sizeof(c6), "%s.enlgd", stem_name);

        const char* src_d = NULL;
        if (file_exists_local(c1)) src_d = c1;
        else if (file_exists_local(c2)) src_d = c2;
        else if (file_exists_local(c3)) src_d = c3;
        else if (file_exists_local(c4)) src_d = c4;
        else if (file_exists_local(c5)) src_d = c5;
        else if (file_exists_local(c6)) src_d = c6;

        if (src_d) {
            if (file_exists_local(out_css)) {
                if (!is_valid_enlngf_generated_asset(out_css)) {
                    fprintf(stderr, "Error: Companion file '%s' already exists and was not generated by Enlangg. Refusing to overwrite.\n", out_css);
                    return false;
                }
            }
            char cmd[1536];
#ifdef _WIN32
            if (bin_dir[0]) snprintf(cmd, sizeof(cmd), "\"\"%s\\enlngd.exe\" compile \"%s\" -o \"%s\" >NUL 2>&1\"", bin_dir, src_d, out_css);
            else snprintf(cmd, sizeof(cmd), "enlngd compile \"%s\" -o \"%s\" >NUL 2>&1", src_d, out_css);
#else
            snprintf(cmd, sizeof(cmd), "enlngd compile \"%s\" -o \"%s\" >/dev/null 2>&1", src_d, out_css);
#endif
            int ret = system(cmd);
            (void)ret;
            stamp_enlngf_generated_marker(out_css);
        }
    }

    /* 2. Auto-compile linked scripts if companion .enlngs/.enlgs exists */
    for (int i = 0; i < doc->script_file_count; i++) {
        char out_js[512];
        if (out_dir && out_dir[0]) snprintf(out_js, sizeof(out_js), "%s/%s", out_dir, doc->script_files[i]);
        else snprintf(out_js, sizeof(out_js), "%s", doc->script_files[i]);

        char stem_name[256];
        strncpy(stem_name, doc->script_files[i], sizeof(stem_name) - 1);
        stem_name[sizeof(stem_name) - 1] = '\0';
        char* dot = strrchr(stem_name, '.');
        if (dot) *dot = '\0';

        char c1[540] = {0}, c2[540] = {0}, c3[540] = {0}, c4[540] = {0}, c5[540] = {0}, c6[540] = {0};
        if (src_dir && src_dir[0]) {
            snprintf(c1, sizeof(c1), "%s/%s.enlngs", src_dir, stem_name);
            snprintf(c2, sizeof(c2), "%s/%s.enlgs", src_dir, stem_name);
        }
        if (out_dir && out_dir[0]) {
            snprintf(c3, sizeof(c3), "%s/%s.enlngs", out_dir, stem_name);
            snprintf(c4, sizeof(c4), "%s/%s.enlgs", out_dir, stem_name);
        }
        snprintf(c5, sizeof(c5), "%s.enlngs", stem_name);
        snprintf(c6, sizeof(c6), "%s.enlgs", stem_name);

        const char* src_s = NULL;
        if (file_exists_local(c1)) src_s = c1;
        else if (file_exists_local(c2)) src_s = c2;
        else if (file_exists_local(c3)) src_s = c3;
        else if (file_exists_local(c4)) src_s = c4;
        else if (file_exists_local(c5)) src_s = c5;
        else if (file_exists_local(c6)) src_s = c6;

        if (src_s) {
            if (file_exists_local(out_js)) {
                if (!is_valid_enlngf_generated_asset(out_js)) {
                    fprintf(stderr, "Error: Companion file '%s' already exists and was not generated by Enlangg. Refusing to overwrite.\n", out_js);
                    return false;
                }
            }
            char cmd[1536];
#ifdef _WIN32
            if (bin_dir[0]) snprintf(cmd, sizeof(cmd), "\"\"%s\\enlngs.exe\" compile \"%s\" -o \"%s\" >NUL 2>&1\"", bin_dir, src_s, out_js);
            else snprintf(cmd, sizeof(cmd), "enlngs compile \"%s\" -o \"%s\" >NUL 2>&1", src_s, out_js);
#else
            snprintf(cmd, sizeof(cmd), "enlngs compile \"%s\" -o \"%s\" >/dev/null 2>&1", src_s, out_js);
#endif
            int ret = system(cmd);
            (void)ret;
            stamp_enlngf_generated_marker(out_js);
        }
    }
    return true;
}

int enlngf_compile_to_html(const EnlngfDoc* doc, const char* outpath) {
    char out_dir[512] = {0};
    strncpy(out_dir, outpath, sizeof(out_dir) - 1);
    {
        char* s1 = strrchr(out_dir, '/');
        char* s2 = strrchr(out_dir, '\\');
        char* sep = (s1 > s2) ? s1 : s2;
        if (sep) *sep = '\0';
        else out_dir[0] = '\0';
    }
    if (!auto_compile_companion_assets(doc, doc->source_dir, out_dir)) {
        return 1;
    }

    FILE* fout = fopen(outpath, "w");
    if (!fout) {
        fprintf(stderr, "Error: Could not open output file '%s'\n", outpath);
        return 1;
    }

    fprintf(fout, "<!DOCTYPE html>\n");
    fprintf(fout, "<!-- Compiled by Enlangg Sovereign Web Engine -->\n");
    fprintf(fout, "<html lang=\"en\">\n");
    fprintf(fout, "<head>\n");
    fprintf(fout, "  <meta charset=\"UTF-8\">\n");
    fprintf(fout, "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    fprintf(fout, "  <title>%s</title>\n", doc->title);

    for (int i = 0; i < doc->design_file_count; i++) {
        fprintf(fout, "  <link rel=\"stylesheet\" href=\"%s\">\n", doc->design_files[i]);
    }

    fprintf(fout, "  <style>\n");
    fprintf(fout, "    *, *::before, *::after { box-sizing: border-box; }\n");
    fprintf(fout, "    body { font-family: -apple-system, BlinkMacSystemFont, 'Inter', 'Segoe UI', Roboto, sans-serif; margin: 0; padding: 0; }\n");
    if (doc->inline_css[0]) {
        fprintf(fout, "%s", doc->inline_css);
    }
    fprintf(fout, "  </style>\n");
    fprintf(fout, "</head>\n");
    fprintf(fout, "<body");
    if (doc->root) {
        for (int i = 0; i < doc->root->attr_count; i++) {
            fprintf(fout, " %s=\"%s\"", doc->root->attrs[i].key, doc->root->attrs[i].value);
        }
    }
    fprintf(fout, ">\n");

    if (doc->root) {
        for (int c = 0; c < doc->root->child_count; c++) {
            emit_node_html(doc->root->children[c], 1, fout);
        }
    }

    if (doc->inline_js[0]) {
        fprintf(fout, "  <script>\n%s  </script>\n", doc->inline_js);
    }

    for (int i = 0; i < doc->script_file_count; i++) {
        fprintf(fout, "  <script src=\"%s\"></script>\n", doc->script_files[i]);
    }

    fprintf(fout, "</body>\n");
    fprintf(fout, "</html>\n");

    fclose(fout);
    printf("[enlngf] Semantic HTML5 compiled successfully: '%s'\n", outpath);
    return 0;
}

#ifdef _WIN32
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

static void spawn_gui_controls(HWND hwnd, const EnlngfNode* node, int* y, HFONT hHeading, HFONT hText, int depth) {
    if (!node) return;
    (void)depth;
    HINSTANCE hInstance = GetModuleHandle(NULL);

    if (strcmp(node->tag, "h1") == 0 || strcmp(node->tag, "h2") == 0 || strcmp(node->tag, "h3") == 0 ||
        strcmp(node->tag, "h4") == 0 || strcmp(node->tag, "h5") == 0 || strcmp(node->tag, "h6") == 0) {
        HWND h = CreateWindowExA(0, "STATIC", node->text, WS_VISIBLE | WS_CHILD, 40, *y, 740, 32, hwnd, NULL, hInstance, NULL);
        SendMessage(h, WM_SETFONT, (WPARAM)hHeading, TRUE);
        *y += 42;
    } else if (strcmp(node->tag, "p") == 0 || strcmp(node->tag, "span") == 0 || strcmp(node->tag, "li") == 0) {
        if (node->text[0]) {
            HWND h = CreateWindowExA(0, "STATIC", node->text, WS_VISIBLE | WS_CHILD, 40, *y, 740, 24, hwnd, NULL, hInstance, NULL);
            SendMessage(h, WM_SETFONT, (WPARAM)hText, TRUE);
            *y += 30;
        }
    } else if (strcmp(node->tag, "button") == 0) {
        HWND h = CreateWindowExA(0, "BUTTON", node->text[0] ? node->text : "Button", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 40, *y, 180, 36, hwnd, (HMENU)101, hInstance, NULL);
        SendMessage(h, WM_SETFONT, (WPARAM)hText, TRUE);
        *y += 46;
    } else if (strcmp(node->tag, "input") == 0 || strcmp(node->tag, "textarea") == 0) {
        const char* ph = "";
        for (int i = 0; i < node->attr_count; i++) {
            if (strcmp(node->attrs[i].key, "placeholder") == 0) ph = node->attrs[i].value;
        }
        CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", ph, WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 40, *y, 350, 30, hwnd, NULL, hInstance, NULL);
        *y += 40;
    }

    for (int i = 0; i < node->child_count; i++) {
        spawn_gui_controls(hwnd, node->children[i], y, hHeading, hText, depth + 1);
    }
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
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(15, 23, 42));

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0, CLASS_NAME, doc->title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 860, 680,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        fprintf(stderr, "Error: Could not create native Win32 window.\n");
        enlngf_free_doc(doc);
        return 1;
    }

    int y = 30;
    HFONT hFontHeading = CreateFontA(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Segoe UI");
    HFONT hFontText = CreateFontA(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Segoe UI");

    if (doc->root) {
        for (int i = 0; i < doc->root->child_count; i++) {
            spawn_gui_controls(hwnd, doc->root->children[i], &y, hFontHeading, hFontText, 0);
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

static const char* get_mime_type(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (str_equals_ci(ext, ".html") || str_equals_ci(ext, ".htm")) return "text/html; charset=UTF-8";
    if (str_equals_ci(ext, ".css")) return "text/css; charset=UTF-8";
    if (str_equals_ci(ext, ".js") || str_equals_ci(ext, ".mjs")) return "application/javascript; charset=UTF-8";
    if (str_equals_ci(ext, ".json")) return "application/json; charset=UTF-8";
    if (str_equals_ci(ext, ".svg")) return "image/svg+xml";
    if (str_equals_ci(ext, ".png")) return "image/png";
    if (str_equals_ci(ext, ".jpg") || str_equals_ci(ext, ".jpeg")) return "image/jpeg";
    if (str_equals_ci(ext, ".gif")) return "image/gif";
    if (str_equals_ci(ext, ".ico")) return "image/x-icon";
    return "application/octet-stream";
}

/* Pure C WinSock2 HTTP Web Studio Server */
int enlngf_serve_http(const char* filepath, int port) {
    EnlngfDoc* doc = enlngf_create_doc();
    if (!enlngf_parse_file(doc, filepath)) {
        fprintf(stderr, "Error: Could not parse '%s'\n", filepath);
        enlngf_free_doc(doc);
        return 1;
    }

    char base_dir[512] = {0};
    strncpy(base_dir, filepath, sizeof(base_dir) - 1);
    char* last_slash = strrchr(base_dir, '/');
    char* last_bslash = strrchr(base_dir, '\\');
    char* sep = (last_slash > last_bslash) ? last_slash : last_bslash;
    if (sep) *sep = '\0';
    else base_dir[0] = '\0';

    /* Resolve canonical served folder path */
    char abs_serve_dir[1024] = {0};
    const char* raw_dir = base_dir[0] ? base_dir : ".";
    if (!_fullpath(abs_serve_dir, raw_dir, sizeof(abs_serve_dir))) {
        strncpy(abs_serve_dir, raw_dir, sizeof(abs_serve_dir) - 1);
    }
    size_t ad_len = strlen(abs_serve_dir);
    while (ad_len > 3 && (abs_serve_dir[ad_len - 1] == '\\' || abs_serve_dir[ad_len - 1] == '/')) {
        abs_serve_dir[--ad_len] = '\0';
    }

    char html_temp[512];
    if (base_dir[0]) snprintf(html_temp, sizeof(html_temp), "%s/.enlngf_serve_preview.html", base_dir);
    else snprintf(html_temp, sizeof(html_temp), ".enlngf_serve_preview.html");

    if (enlngf_compile_to_html(doc, html_temp) != 0) {
        enlngf_free_doc(doc);
        return 1;
    }

    FILE* f = fopen(html_temp, "rb");
    if (!f) {
        enlngf_free_doc(doc);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* html_body = (char*)malloc(sz + 1);
    fread(html_body, 1, sz, f);
    html_body[sz] = '\0';
    fclose(f);
    remove(html_temp);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons((u_short)port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        fprintf(stderr, "Error: Port %d already in use on 127.0.0.1.\n", port);
        closesocket(server_fd);
        WSACleanup();
        free(html_body);
        enlngf_free_doc(doc);
        return 1;
    }

    listen(server_fd, 10);

    printf("=================================================================\n");
    printf("     ENLNGF: SOVEREIGN WEB STUDIO HTTP SERVER LIVE               \n");
    printf("=================================================================\n");
    printf(" >> Local Studio: http://127.0.0.1:%d\n", port);
    printf(" >> Engine: Pure C WinSock2 Daemon (Zero Python / Zero Node.js)\n");
    printf(" >> Serving Document: '%s'\n", doc->title);
    printf(" >> Press Ctrl+C to terminate.\n\n");
    fflush(stdout);

    while (1) {
        SOCKET client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == INVALID_SOCKET) break;

        char req_buf[2048] = {0};
        int rlen = recv(client_fd, req_buf, sizeof(req_buf) - 1, 0);
        if (rlen <= 0) {
            closesocket(client_fd);
            continue;
        }
        req_buf[rlen] = '\0';

        /* Parse HTTP request path */
        char method[16] = {0};
        char req_path[512] = {0};
        sscanf(req_buf, "%15s %511s", method, req_path);

        if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0) {
            const char* body405 = "405 Method Not Allowed\n";
            char resp405[512];
            snprintf(resp405, sizeof(resp405),
                "HTTP/1.1 405 Method Not Allowed\r\n"
                "Allow: GET, HEAD\r\n"
                "Content-Type: text/plain; charset=UTF-8\r\n"
                "Content-Length: 22\r\n"
                "Connection: close\r\n\r\n"
                "%s", body405);
            send(client_fd, resp405, (int)strlen(resp405), 0);
            closesocket(client_fd);
            continue;
        }

        bool is_head = (strcmp(method, "HEAD") == 0);

        char* qmark = strchr(req_path, '?');
        if (qmark) *qmark = '\0';
        char* hmark = strchr(req_path, '#');
        if (hmark) *hmark = '\0';

        const char* rel_chk = (req_path[0] == '/') ? req_path + 1 : req_path;
        bool bad_path = (req_path[0] != '/') ||
                        (strstr(req_path, "..") != NULL) ||
                        (strchr(req_path, ':') != NULL) ||
                        (strchr(req_path, '\\') != NULL) ||
                        (strchr(req_path, '%') != NULL) ||
                        (req_path[1] == '/') ||
                        (isalpha((unsigned char)rel_chk[0]) && (rel_chk[1] == ':' || rel_chk[1] == '|'));

        if (bad_path) {
            const char* body403 = "403 Forbidden";
            char hdr[256];
            snprintf(hdr, sizeof(hdr),
                "HTTP/1.1 403 Forbidden\r\nContent-Type: text/plain; charset=UTF-8\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
                (int)strlen(body403));
            send(client_fd, hdr, (int)strlen(hdr), 0);
            if (!is_head) {
                send(client_fd, body403, (int)strlen(body403), 0);
            }
            closesocket(client_fd);
            continue;
        }

        if (strcmp(req_path, "/") == 0 || strcmp(req_path, "/index.html") == 0) {
            char resp_hdr[512];
            snprintf(resp_hdr, sizeof(resp_hdr),
                "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", sz);
            send(client_fd, resp_hdr, (int)strlen(resp_hdr), 0);
            if (!is_head) {
                send(client_fd, html_body, (int)sz, 0);
            }
            closesocket(client_fd);
            continue;
        }

        /* Resolve requested file inside abs_serve_dir and require prefix match */
        const char* rel = req_path + 1;
        char candidate_path[1024];
        snprintf(candidate_path, sizeof(candidate_path), "%s\\%s", abs_serve_dir, rel);

        char resolved_path[1024] = {0};
        if (!_fullpath(resolved_path, candidate_path, sizeof(resolved_path))) {
            resolved_path[0] = '\0';
        }

        size_t root_len = strlen(abs_serve_dir);
        bool inside_serve_dir = false;
        if (resolved_path[0] && _strnicmp(resolved_path, abs_serve_dir, root_len) == 0) {
            char next_ch = resolved_path[root_len];
            if (next_ch == '\\' || next_ch == '/') {
                inside_serve_dir = true;
            }
        }

        if (!inside_serve_dir) {
            const char* body403 = "403 Forbidden";
            char hdr[256];
            snprintf(hdr, sizeof(hdr),
                "HTTP/1.1 403 Forbidden\r\nContent-Type: text/plain; charset=UTF-8\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
                (int)strlen(body403));
            send(client_fd, hdr, (int)strlen(hdr), 0);
            if (!is_head) {
                send(client_fd, body403, (int)strlen(body403), 0);
            }
            closesocket(client_fd);
            continue;
        }

        DWORD fattr = GetFileAttributesA(resolved_path);
        FILE* fa = NULL;
        if (fattr != INVALID_FILE_ATTRIBUTES && !(fattr & FILE_ATTRIBUTE_DIRECTORY)) {
            fa = fopen(resolved_path, "rb");
        }

        if (fa) {
            fseek(fa, 0, SEEK_END);
            long asz = ftell(fa);
            fseek(fa, 0, SEEK_SET);
            char* abuf = NULL;
            if (!is_head && asz > 0) {
                abuf = (char*)malloc(asz + 1);
                if (abuf) {
                    fread(abuf, 1, asz, fa);
                }
            }
            const char* mime = get_mime_type(rel);
            char resp_hdr[512];
            snprintf(resp_hdr, sizeof(resp_hdr),
                "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n",
                mime, asz);
            send(client_fd, resp_hdr, (int)strlen(resp_hdr), 0);
            if (!is_head && abuf) {
                send(client_fd, abuf, (int)asz, 0);
                free(abuf);
            }
            fclose(fa);
        } else {
            const char* body404 = "404 Not Found";
            char hdr[256];
            snprintf(hdr, sizeof(hdr),
                "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain; charset=UTF-8\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
                (int)strlen(body404));
            send(client_fd, hdr, (int)strlen(hdr), 0);
            if (!is_head) {
                send(client_fd, body404, (int)strlen(body404), 0);
            }
        }
        closesocket(client_fd);
    }

    closesocket(server_fd);
    WSACleanup();
    free(html_body);
    enlngf_free_doc(doc);
    return 0;
}
#else
int enlngf_run_gui(const char* filepath) {
    EnlngfDoc* doc = enlngf_create_doc();
    if (!enlngf_parse_file(doc, filepath)) {
        fprintf(stderr, "Error: Could not parse '%s'\n", filepath);
        enlngf_free_doc(doc);
        return 1;
    }
    char html_temp[256] = "enlngf_preview.html";
    enlngf_compile_to_html(doc, html_temp);
    printf("enlngf GUI: Compiled standalone HTML to '%s' (Win32 window is Windows-only).\n", html_temp);
    enlngf_free_doc(doc);
    return 0;
}

int enlngf_serve_http(const char* filepath, int port) {
    EnlngfDoc* doc = enlngf_create_doc();
    if (!enlngf_parse_file(doc, filepath)) {
        fprintf(stderr, "Error: Could not parse '%s'\n", filepath);
        enlngf_free_doc(doc);
        return 1;
    }
    char html_temp[256] = "enlngf_preview.html";
    enlngf_compile_to_html(doc, html_temp);
    printf("enlngf serve: Generated '%s' for port %d.\n", html_temp, port);
    enlngf_free_doc(doc);
    return 0;
}
#endif
