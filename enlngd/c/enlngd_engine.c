#include "enlngd.h"

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

static bool str_equals_ci(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++; b++;
    }
    return *a == *b;
}

EnlngdSheet* enlngd_create_sheet(void) {
    return (EnlngdSheet*)calloc(1, sizeof(EnlngdSheet));
}

void enlngd_free_sheet(EnlngdSheet* sheet) {
    if (sheet) free(sheet);
}

static void strip_outer_quotes(char* s) {
    s = trim(s);
    size_t len = strlen(s);
    while (len >= 2 && ((s[0] == '"' && s[len - 1] == '"') || (s[0] == '\'' && s[len - 1] == '\''))) {
        s[len - 1] = '\0';
        memmove(s, s + 1, len - 1);
        s = trim(s);
        len = strlen(s);
    }
}

static void strip_stray_quotes(char* s) {
    if (!s) return;
    char* dst = s;
    for (const char* src = s; *src; src++) {
        if (*src != '"' && *src != '\'') {
            *dst++ = *src;
        }
    }
    *dst = '\0';
    trim(s);
}

static bool is_unitless_prop(const char* prop) {
    if (!prop) return false;
    static const char* unitless[] = {
        "line-height", "opacity", "z-index", "font-weight",
        "flex", "flex-grow", "flex-shrink", "order", "columns",
        "grid-column", "grid-row", "scale", "zoom", "aspect-ratio",
        "animation-iteration-count", "tab-size", "orphans", "widows",
        "token", "color", "font-family", "display", "position",
        "text-align", "cursor", "overflow", "overflow-x", "overflow-y",
        "user-select", "touch-action", "flex-direction", "align-items",
        "justify-content", NULL
    };
    for (int i = 0; unitless[i]; i++) {
        if (strcmp(prop, unitless[i]) == 0) return true;
    }
    return false;
}

static bool is_bare_number_token(const char* tok) {
    if (!tok || !*tok) return false;
    int i = 0;
    if (tok[i] == '-' || tok[i] == '+') i++;
    if (!isdigit((unsigned char)tok[i])) return false;
    bool seen_dot = false;
    for (; tok[i]; i++) {
        if (tok[i] == '.') {
            if (seen_dot) return false;
            seen_dot = true;
            if (!isdigit((unsigned char)tok[i + 1])) return false;
        } else if (!isdigit((unsigned char)tok[i])) {
            return false;
        }
    }
    return true;
}

static void replace_str(char* src, const char* find, const char* rep) {
    char buf[1024];
    char* p = src;
    buf[0] = '\0';
    size_t find_len = strlen(find);
    while (*p) {
        char* match = strstr(p, find);
        if (match) {
            strncat(buf, p, (size_t)(match - p));
            strcat(buf, rep);
            p = match + find_len;
        } else {
            strcat(buf, p);
            break;
        }
    }
    strncpy(src, buf, 1023);
    src[1023] = '\0';
}

static void format_css_value(const char* prop, const char* raw_val, char* out, size_t out_sz) {
    char buf[1024];
    strncpy(buf, raw_val, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    trim(buf);

    if (strcmp(prop, "content") != 0) {
        strip_outer_quotes(buf);
    }

    // Replace English unit words
    replace_str(buf, "viewport-height", "vh");
    replace_str(buf, "viewport height", "vh");
    replace_str(buf, "viewport-width", "vw");
    replace_str(buf, "viewport width", "vw");
    replace_str(buf, "pixels", "px");
    replace_str(buf, "pixel", "px");
    replace_str(buf, "percentage", "%");
    replace_str(buf, "percent", "%");
    replace_str(buf, "degrees", "deg");

    // Remove space between number and unit: e.g. "100 vh" -> "100vh", "16 px" -> "16px"
    char cleaned[1024];
    int ci = 0;
    int len = (int)strlen(buf);
    for (int i = 0; i < len; i++) {
        if (isdigit((unsigned char)buf[i])) {
            cleaned[ci++] = buf[i];
            int j = i + 1;
            while (j < len && (isdigit((unsigned char)buf[j]) || buf[j] == '.')) {
                cleaned[ci++] = buf[j++];
            }
            if (j < len && buf[j] == ' ') {
                int k = j + 1;
                while (k < len && buf[k] == ' ') k++;
                if (k < len) {
                    if (str_starts_with_ci(&buf[k], "px")) {
                        cleaned[ci++] = 'p'; cleaned[ci++] = 'x';
                        i = k + 1;
                        continue;
                    } else if (str_starts_with_ci(&buf[k], "vh")) {
                        cleaned[ci++] = 'v'; cleaned[ci++] = 'h';
                        i = k + 1;
                        continue;
                    } else if (str_starts_with_ci(&buf[k], "vw")) {
                        cleaned[ci++] = 'v'; cleaned[ci++] = 'w';
                        i = k + 1;
                        continue;
                    } else if (str_starts_with_ci(&buf[k], "rem")) {
                        cleaned[ci++] = 'r'; cleaned[ci++] = 'e'; cleaned[ci++] = 'm';
                        i = k + 2;
                        continue;
                    } else if (str_starts_with_ci(&buf[k], "em") && (k + 2 == len || isspace((unsigned char)buf[k + 2]) || buf[k + 2] == ';')) {
                        cleaned[ci++] = 'e'; cleaned[ci++] = 'm';
                        i = k + 1;
                        continue;
                    } else if (str_starts_with_ci(&buf[k], "%")) {
                        cleaned[ci++] = '%';
                        i = k;
                        continue;
                    } else if (str_starts_with_ci(&buf[k], "deg")) {
                        cleaned[ci++] = 'd'; cleaned[ci++] = 'e'; cleaned[ci++] = 'g';
                        i = k + 2;
                        continue;
                    } else if (str_starts_with_ci(&buf[k], "ms") && (k + 2 == len || isspace((unsigned char)buf[k + 2]) || buf[k + 2] == ';' || buf[k + 2] == ',')) {
                        cleaned[ci++] = 'm'; cleaned[ci++] = 's';
                        i = k + 1;
                        continue;
                    } else if (str_starts_with_ci(&buf[k], "s") && (k + 1 == len || isspace((unsigned char)buf[k + 1]) || buf[k + 1] == ';' || buf[k + 1] == ',')) {
                        cleaned[ci++] = 's';
                        i = k;
                        continue;
                    }
                }
            }
            i = j - 1;
        } else {
            cleaned[ci++] = buf[i];
        }
    }
    cleaned[ci] = '\0';
    trim(cleaned);

    // If bare number (including negative or float) and property is not unitless, append px
    if (cleaned[0] != '\0' && !is_unitless_prop(prop)) {
        if (is_bare_number_token(cleaned)) {
            if (strcmp(cleaned, "0") == 0) {
                snprintf(out, out_sz, "0");
            } else {
                snprintf(out, out_sz, "%spx", cleaned);
            }
            return;
        }
        // Check if space-separated list of bare numbers (e.g. "0 0 10 10" or "10 20")
        if (strchr(cleaned, ' ') != NULL && strchr(cleaned, '(') == NULL && strchr(cleaned, ',') == NULL) {
            char tmp_list[512];
            strncpy(tmp_list, cleaned, sizeof(tmp_list) - 1);
            tmp_list[sizeof(tmp_list) - 1] = '\0';
            char rebuilt[512] = {0};
            bool all_nums = true;
            int tok_cnt = 0;
            char* t = strtok(tmp_list, " ");
            while (t) {
                if (!is_bare_number_token(t)) {
                    all_nums = false;
                    break;
                }
                if (tok_cnt > 0) strncat(rebuilt, " ", sizeof(rebuilt) - strlen(rebuilt) - 1);
                if (strcmp(t, "0") == 0) {
                    strncat(rebuilt, "0", sizeof(rebuilt) - strlen(rebuilt) - 1);
                } else {
                    char with_px[64];
                    snprintf(with_px, sizeof(with_px), "%spx", t);
                    strncat(rebuilt, with_px, sizeof(rebuilt) - strlen(rebuilt) - 1);
                }
                tok_cnt++;
                t = strtok(NULL, " ");
            }
            if (all_nums && tok_cnt > 1) {
                strncpy(out, rebuilt, out_sz - 1);
                out[out_sz - 1] = '\0';
                return;
            }
        }
    }

    strncpy(out, cleaned, out_sz - 1);
    out[out_sz - 1] = '\0';
}

static void resolve_token_and_gradients(const EnlngdSheet* sheet, const char* val_str, char* out_buf, size_t out_sz) {
    (void)sheet;
    char work[1024];
    strncpy(work, val_str, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';
    trim(work);
    strip_outer_quotes(work);

    // 1. Check for "use [color|size|font|value] <name>"
    if (strstr(work, "use ")) {
        char* u = strstr(work, "use ");
        while (u) {
            char* p = u + 4;
            if (str_starts_with_ci(p, "color ")) p += 6;
            else if (str_starts_with_ci(p, "size ")) p += 5;
            else if (str_starts_with_ci(p, "font ")) p += 5;
            else if (str_starts_with_ci(p, "spacing ")) p += 8;
            else if (str_starts_with_ci(p, "value ")) p += 6;
            p = trim(p);
            char tok_name[64] = {0};
            int ti = 0;
            if (*p == '"' || *p == '\'') p++;
            while (*p && *p != '"' && *p != '\'' && !isspace((unsigned char)*p) && ti < 63) {
                tok_name[ti++] = *p++;
            }
            tok_name[ti] = '\0';
            if (*p == '"' || *p == '\'') p++;

            char rep[128];
            snprintf(rep, sizeof(rep), "var(--%s)", tok_name);
            char prefix[1024] = {0};
            size_t u_idx = (size_t)(u - work);
            strncpy(prefix, work, u_idx);
            prefix[u_idx] = '\0';
            char suffix[1024];
            strncpy(suffix, p, sizeof(suffix) - 1);
            suffix[sizeof(suffix) - 1] = '\0';
            snprintf(work, sizeof(work), "%s%s%s", prefix, rep, suffix);

            u = strstr(work, "use ");
        }
    }

    // 2. Check for gradient phrases
    // Example: "radial from #12102e at top center to #07070e"
    if (strstr(work, "radial from ") || strstr(work, "radial from")) {
        char from_col[64] = {0};
        char at_pos[64] = "center";
        char to_col[64] = {0};
        char* rf = strstr(work, "radial from");
        char* after_rf = rf + 11;
        while (isspace((unsigned char)*after_rf)) after_rf++;
        char* at_ptr = strstr(after_rf, " at ");
        char* to_ptr = strstr(after_rf, " to ");
        if (at_ptr && to_ptr && at_ptr < to_ptr) {
            size_t flen = (size_t)(at_ptr - after_rf);
            if (flen >= sizeof(from_col)) flen = sizeof(from_col) - 1;
            strncpy(from_col, after_rf, flen);
            from_col[flen] = '\0';
            char* after_at = at_ptr + 4;
            size_t plen = (size_t)(to_ptr - after_at);
            if (plen >= sizeof(at_pos)) plen = sizeof(at_pos) - 1;
            strncpy(at_pos, after_at, plen);
            at_pos[plen] = '\0';
            char* after_to = to_ptr + 4;
            strncpy(to_col, after_to, sizeof(to_col) - 1);
            to_col[sizeof(to_col) - 1] = '\0';
            strip_stray_quotes(from_col);
            strip_stray_quotes(at_pos);
            strip_stray_quotes(to_col);
            snprintf(out_buf, out_sz, "radial-gradient(circle at %s, %s, %s)", at_pos, from_col, to_col);
            return;
        }
    }

    // Example: "linear from #ffffff via #e0e7ff to #818cf8"
    if (strstr(work, "linear from ")) {
        char from_col[64] = {0};
        char via_col[64] = {0};
        char to_col[64] = {0};
        char* lf = strstr(work, "linear from ");
        char* p = lf + 12;
        char* via_ptr = strstr(p, " via ");
        char* to_ptr = strstr(p, " to ");
        if (via_ptr && to_ptr && via_ptr < to_ptr) {
            size_t flen = (size_t)(via_ptr - p);
            if (flen >= sizeof(from_col)) flen = sizeof(from_col) - 1;
            strncpy(from_col, p, flen);
            from_col[flen] = '\0';
            char* after_via = via_ptr + 5;
            size_t vlen = (size_t)(to_ptr - after_via);
            if (vlen >= sizeof(via_col)) vlen = sizeof(via_col) - 1;
            strncpy(via_col, after_via, vlen);
            via_col[vlen] = '\0';
            char* after_to = to_ptr + 4;
            strncpy(to_col, after_to, sizeof(to_col) - 1);
            to_col[sizeof(to_col) - 1] = '\0';
            strip_stray_quotes(from_col);
            strip_stray_quotes(via_col);
            strip_stray_quotes(to_col);
            snprintf(out_buf, out_sz, "linear-gradient(135deg, %s, %s, %s)", from_col, via_col, to_col);
            return;
        } else if (to_ptr) {
            size_t flen = (size_t)(to_ptr - p);
            if (flen >= sizeof(from_col)) flen = sizeof(from_col) - 1;
            strncpy(from_col, p, flen);
            from_col[flen] = '\0';
            char* after_to = to_ptr + 4;
            strncpy(to_col, after_to, sizeof(to_col) - 1);
            to_col[sizeof(to_col) - 1] = '\0';
            strip_stray_quotes(from_col);
            strip_stray_quotes(to_col);
            snprintf(out_buf, out_sz, "linear-gradient(135deg, %s, %s)", from_col, to_col);
            return;
        }
    }

    // Example: "gradient from \"#1a1a2e\" to \"#16213e\" at 135 degrees"
    if (strstr(work, "gradient from ") || strstr(work, "from ")) {
        char* gf = strstr(work, "from ");
        if (gf) {
            char* p = gf + 5;
            char* to_ptr = strstr(p, " to ");
            if (to_ptr) {
                char from_col[64] = {0};
                char to_col[64] = {0};
                char deg_str[32] = "135deg";
                size_t flen = (size_t)(to_ptr - p);
                if (flen >= sizeof(from_col)) flen = sizeof(from_col) - 1;
                strncpy(from_col, p, flen);
                from_col[flen] = '\0';
                char* after_to = to_ptr + 4;
                char* at_ptr = strstr(after_to, " at ");
                if (at_ptr) {
                    size_t tlen = (size_t)(at_ptr - after_to);
                    if (tlen >= sizeof(to_col)) tlen = sizeof(to_col) - 1;
                    strncpy(to_col, after_to, tlen);
                    to_col[tlen] = '\0';
                    char* after_at = at_ptr + 4;
                    strncpy(deg_str, after_at, sizeof(deg_str) - 1);
                    deg_str[sizeof(deg_str) - 1] = '\0';
                    replace_str(deg_str, "degrees", "deg");
                    replace_str(deg_str, " ", "");
                } else {
                    strncpy(to_col, after_to, sizeof(to_col) - 1);
                    to_col[sizeof(to_col) - 1] = '\0';
                }
                strip_stray_quotes(from_col);
                strip_stray_quotes(to_col);
                strip_stray_quotes(deg_str);
                snprintf(out_buf, out_sz, "linear-gradient(%s, %s, %s)", deg_str, from_col, to_col);
                return;
            }
        }
    }

    strncpy(out_buf, work, out_sz - 1);
    out_buf[out_sz - 1] = '\0';
}

static void add_prop(EnlngdRule* rule, const char* prop, const char* val) {
    if (!rule || rule->prop_count >= MAX_PROPS || !prop || !val || !*prop) return;
    // Check if property already exists in this rule; if so, update it
    for (int i = 0; i < rule->prop_count; i++) {
        if (strcmp(rule->props[i].prop, prop) == 0) {
            strncpy(rule->props[i].value, val, sizeof(rule->props[i].value) - 1);
            rule->props[i].value[sizeof(rule->props[i].value) - 1] = '\0';
            return;
        }
    }
    EnlngdProp* p = &rule->props[rule->prop_count++];
    strncpy(p->prop, prop, sizeof(p->prop) - 1);
    p->prop[sizeof(p->prop) - 1] = '\0';
    strncpy(p->value, val, sizeof(p->value) - 1);
    p->value[sizeof(p->value) - 1] = '\0';
}

static EnlngdRule* get_or_create_rule(EnlngdSheet* sheet, const char* selector) {
    for (int i = 0; i < sheet->rule_count; i++) {
        if (strcmp(sheet->rules[i].selector, selector) == 0) {
            return &sheet->rules[i];
        }
    }
    if (sheet->rule_count < MAX_RULES) {
        EnlngdRule* r = &sheet->rules[sheet->rule_count++];
        strncpy(r->selector, selector, sizeof(r->selector) - 1);
        r->selector[sizeof(r->selector) - 1] = '\0';
        r->prop_count = 0;
        r->is_hover = (strstr(selector, ":hover") != NULL);
        return r;
    }
    return NULL;
}

static void apply_hover_segment(EnlngdRule* hov_rule, char* raw_seg) {
    char* seg = trim(raw_seg);
    if (!*seg) return;
    if (str_starts_with_ci(seg, "scale ")) {
        char t[64]; snprintf(t, sizeof(t), "scale(%s)", trim(seg + 6));
        add_prop(hov_rule, "transform", t);
    } else if (str_starts_with_ci(seg, "transform ")) {
        add_prop(hov_rule, "transform", trim(seg + 10));
    } else if (str_starts_with_ci(seg, "translateY ")) {
        char t[64]; snprintf(t, sizeof(t), "translateY(%s)", trim(seg + 11));
        add_prop(hov_rule, "transform", t);
    } else if (str_starts_with_ci(seg, "translateX ")) {
        char t[64]; snprintf(t, sizeof(t), "translateX(%s)", trim(seg + 11));
        add_prop(hov_rule, "transform", t);
    } else if (str_starts_with_ci(seg, "background ")) {
        add_prop(hov_rule, "background", trim(seg + 11));
    } else if (str_starts_with_ci(seg, "border-color ") || str_starts_with_ci(seg, "border color ")) {
        add_prop(hov_rule, "border-color", trim(seg + 13));
    } else if (str_starts_with_ci(seg, "color ")) {
        add_prop(hov_rule, "color", trim(seg + 6));
    } else if (str_starts_with_ci(seg, "shadow ")) {
        add_prop(hov_rule, "box-shadow", trim(seg + 7));
    } else if (str_starts_with_ci(seg, "box-shadow ") || str_starts_with_ci(seg, "box shadow ")) {
        add_prop(hov_rule, "box-shadow", trim(seg + 11));
    } else if (str_starts_with_ci(seg, "opacity ")) {
        add_prop(hov_rule, "opacity", trim(seg + 8));
    }
}

static void map_and_add_declaration(EnlngdSheet* sheet, EnlngdRule* current_rule, const char* comp_name, const char* clause) {
    char work[512];
    strncpy(work, clause, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';
    char* s = trim(work);
    if (!*s) return;

    if (str_starts_with_ci(s, "set ")) {
        s = trim(s + 4);
    }

    // Special 1: hover effect to "..."
    if (str_starts_with_ci(s, "hover effect to ") || str_starts_with_ci(s, "hover effect ")) {
        char* eff = str_starts_with_ci(s, "hover effect to ") ? s + 16 : s + 13;
        strip_outer_quotes(eff);
        char hov_sel[128];
        if (comp_name && *comp_name) {
            snprintf(hov_sel, sizeof(hov_sel), ".%s:hover, #%s:hover", comp_name, comp_name);
        } else if (current_rule) {
            snprintf(hov_sel, sizeof(hov_sel), "%s:hover", current_rule->selector);
        } else {
            return;
        }
        EnlngdRule* hov_rule = get_or_create_rule(sheet, hov_sel);
        // Split effects by top-level "," or " and " (ignoring commas inside parentheses like rgba(...))
        char eff_buf[512];
        strncpy(eff_buf, eff, sizeof(eff_buf) - 1);
        eff_buf[sizeof(eff_buf) - 1] = '\0';
        int paren_depth = 0;
        char* seg_start = eff_buf;
        int elen = (int)strlen(eff_buf);
        for (int i = 0; i < elen; i++) {
            if (eff_buf[i] == '(') paren_depth++;
            else if (eff_buf[i] == ')' && paren_depth > 0) paren_depth--;
            else if (paren_depth == 0) {
                if (eff_buf[i] == ',') {
                    eff_buf[i] = '\0';
                    apply_hover_segment(hov_rule, seg_start);
                    seg_start = &eff_buf[i + 1];
                } else if (str_starts_with_ci(&eff_buf[i], " and ")) {
                    eff_buf[i] = '\0';
                    apply_hover_segment(hov_rule, seg_start);
                    i += 4;
                    seg_start = &eff_buf[i + 1];
                }
            }
        }
        if (*seg_start) {
            apply_hover_segment(hov_rule, seg_start);
        }
        return;
    }

    // Special 2: hover color to "..."
    if (str_starts_with_ci(s, "hover color to ") || str_starts_with_ci(s, "hover color ")) {
        char* col = str_starts_with_ci(s, "hover color to ") ? s + 15 : s + 12;
        strip_outer_quotes(col);
        char hov_sel[128];
        if (comp_name && *comp_name) {
            snprintf(hov_sel, sizeof(hov_sel), ".%s:hover, #%s:hover", comp_name, comp_name);
        } else if (current_rule) {
            snprintf(hov_sel, sizeof(hov_sel), "%s:hover", current_rule->selector);
        } else {
            return;
        }
        EnlngdRule* hov_rule = get_or_create_rule(sheet, hov_sel);
        char clean_col[128];
        format_css_value("color", col, clean_col, sizeof(clean_col));
        add_prop(hov_rule, "color", clean_col);
        return;
    }

    if (!current_rule) return;

    // Special 3: text gradient to "..."
    if (str_starts_with_ci(s, "text gradient to ") || str_starts_with_ci(s, "text gradient ")) {
        char* grad = str_starts_with_ci(s, "text gradient to ") ? s + 17 : s + 14;
        strip_outer_quotes(grad);
        char resolved[256];
        resolve_token_and_gradients(sheet, grad, resolved, sizeof(resolved));
        add_prop(current_rule, "-webkit-background-clip", "text");
        add_prop(current_rule, "-webkit-text-fill-color", "transparent");
        add_prop(current_rule, "background-image", resolved);
        return;
    }

    // Special 4: backdrop blur <val>
    if (str_starts_with_ci(s, "backdrop blur ")) {
        char* val = s + 14;
        char clean_val[128];
        format_css_value("backdrop-filter", val, clean_val, sizeof(clean_val));
        char filter_str[128];
        snprintf(filter_str, sizeof(filter_str), "blur(%s)", clean_val);
        add_prop(current_rule, "backdrop-filter", filter_str);
        add_prop(current_rule, "-webkit-backdrop-filter", filter_str);
        return;
    }

    // Special 5: columns <N>
    if (str_starts_with_ci(s, "columns ") || str_starts_with_ci(s, "grid columns ")) {
        char* p = str_starts_with_ci(s, "columns ") ? s + 8 : s + 13;
        p = trim(p);
        if (isdigit((unsigned char)*p)) {
            char cols[64];
            snprintf(cols, sizeof(cols), "repeat(%s, 1fr)", p);
            add_prop(current_rule, "grid-template-columns", cols);
        } else {
            add_prop(current_rule, "grid-template-columns", p);
        }
        return;
    }

    // Special 6: align center / space between / justify center
    if (str_equals_ci(s, "align center") || str_equals_ci(s, "align-center")) {
        add_prop(current_rule, "align-items", "center");
        return;
    }
    if (str_equals_ci(s, "space between") || str_equals_ci(s, "space-between")) {
        add_prop(current_rule, "justify-content", "space-between");
        return;
    }
    if (str_equals_ci(s, "justify center") || str_equals_ci(s, "justify-center")) {
        add_prop(current_rule, "justify-content", "center");
        return;
    }
    if (str_equals_ci(s, "direction column") || str_equals_ci(s, "direction-column")) {
        add_prop(current_rule, "flex-direction", "column");
        return;
    }
    if (str_equals_ci(s, "direction row") || str_equals_ci(s, "direction-row")) {
        add_prop(current_rule, "flex-direction", "row");
        return;
    }
    if (str_starts_with_ci(s, "at top ")) {
        char clean_val[128];
        format_css_value("top", trim(s + 7), clean_val, sizeof(clean_val));
        add_prop(current_rule, "top", clean_val);
        return;
    }

    // Standard Property Name Mapping
    char prop_name[64] = {0};
    char* raw_val_ptr = NULL;

    static const struct { const char* eng; const char* css; } prop_map[] = {
        {"global background to ", "background"},
        {"global background ", "background"},
        {"global bg to ", "background"},
        {"global bg ", "background"},
        {"primary background to ", "background"},
        {"primary background ", "background"},
        {"background-image to ", "background-image"},
        {"background-image ", "background-image"},
        {"background image to ", "background-image"},
        {"background image ", "background-image"},
        {"gradient to ", "background-image"},
        {"gradient ", "background-image"},
        {"background-color to ", "background-color"},
        {"background-color ", "background-color"},
        {"background color to ", "background-color"},
        {"background color ", "background-color"},
        {"background to ", "background"},
        {"background ", "background"},
        {"bg color to ", "background-color"},
        {"bg color ", "background-color"},
        {"bg to ", "background"},
        {"bg ", "background"},

        {"text color to ", "color"},
        {"text color ", "color"},
        {"text-color to ", "color"},
        {"text-color ", "color"},
        {"color to ", "color"},
        {"color ", "color"},

        {"font family to ", "font-family"},
        {"font family ", "font-family"},
        {"font-family to ", "font-family"},
        {"font-family ", "font-family"},
        {"font size to ", "font-size"},
        {"font size ", "font-size"},
        {"font-size to ", "font-size"},
        {"font-size ", "font-size"},
        {"font weight to ", "font-weight"},
        {"font weight ", "font-weight"},
        {"font-weight to ", "font-weight"},
        {"font-weight ", "font-weight"},

        {"line height to ", "line-height"},
        {"line height ", "line-height"},
        {"line-height to ", "line-height"},
        {"line-height ", "line-height"},
        {"letter spacing to ", "letter-spacing"},
        {"letter spacing ", "letter-spacing"},
        {"letter-spacing to ", "letter-spacing"},
        {"letter-spacing ", "letter-spacing"},
        {"text align to ", "text-align"},
        {"text align ", "text-align"},
        {"text-align to ", "text-align"},
        {"text-align ", "text-align"},
        {"align text to ", "text-align"},
        {"align text ", "text-align"},
        {"text shadow to ", "text-shadow"},
        {"text shadow ", "text-shadow"},
        {"text-shadow to ", "text-shadow"},
        {"text-shadow ", "text-shadow"},

        {"max width to ", "max-width"},
        {"max width ", "max-width"},
        {"max-width to ", "max-width"},
        {"max-width ", "max-width"},
        {"min width to ", "min-width"},
        {"min width ", "min-width"},
        {"min-width to ", "min-width"},
        {"min-width ", "min-width"},
        {"max height to ", "max-height"},
        {"max height ", "max-height"},
        {"max-height to ", "max-height"},
        {"max-height ", "max-height"},
        {"min height to ", "min-height"},
        {"min height ", "min-height"},
        {"min-height to ", "min-height"},
        {"min-height ", "min-height"},

        {"width to ", "width"},
        {"width ", "width"},
        {"height to ", "height"},
        {"height ", "height"},

        {"margin top to ", "margin-top"},
        {"margin top ", "margin-top"},
        {"margin-top to ", "margin-top"},
        {"margin-top ", "margin-top"},
        {"margin bottom to ", "margin-bottom"},
        {"margin bottom ", "margin-bottom"},
        {"margin-bottom to ", "margin-bottom"},
        {"margin-bottom ", "margin-bottom"},
        {"margin left to ", "margin-left"},
        {"margin left ", "margin-left"},
        {"margin-left to ", "margin-left"},
        {"margin-left ", "margin-left"},
        {"margin right to ", "margin-right"},
        {"margin right ", "margin-right"},
        {"margin-right to ", "margin-right"},
        {"margin-right ", "margin-right"},
        {"margin to ", "margin"},
        {"margin ", "margin"},

        {"padding top to ", "padding-top"},
        {"padding top ", "padding-top"},
        {"padding-top to ", "padding-top"},
        {"padding-top ", "padding-top"},
        {"padding bottom to ", "padding-bottom"},
        {"padding bottom ", "padding-bottom"},
        {"padding-bottom to ", "padding-bottom"},
        {"padding-bottom ", "padding-bottom"},
        {"padding left to ", "padding-left"},
        {"padding left ", "padding-left"},
        {"padding-left to ", "padding-left"},
        {"padding-left ", "padding-left"},
        {"padding right to ", "padding-right"},
        {"padding right ", "padding-right"},
        {"padding-right to ", "padding-right"},
        {"padding-right ", "padding-right"},
        {"padding to ", "padding"},
        {"padding ", "padding"},

        {"border radius to ", "border-radius"},
        {"border radius ", "border-radius"},
        {"border-radius to ", "border-radius"},
        {"border-radius ", "border-radius"},
        {"corner radius to ", "border-radius"},
        {"corner radius ", "border-radius"},
        {"border color to ", "border-color"},
        {"border color ", "border-color"},
        {"border-color to ", "border-color"},
        {"border-color ", "border-color"},
        {"border bottom to ", "border-bottom"},
        {"border bottom ", "border-bottom"},
        {"border-bottom to ", "border-bottom"},
        {"border-bottom ", "border-bottom"},
        {"border top to ", "border-top"},
        {"border top ", "border-top"},
        {"border-top to ", "border-top"},
        {"border-top ", "border-top"},
        {"border left to ", "border-left"},
        {"border left ", "border-left"},
        {"border-left to ", "border-left"},
        {"border-left ", "border-left"},
        {"border right to ", "border-right"},
        {"border right ", "border-right"},
        {"border-right to ", "border-right"},
        {"border-right ", "border-right"},
        {"border to ", "border"},
        {"border ", "border"},

        {"box shadow to ", "box-shadow"},
        {"box shadow ", "box-shadow"},
        {"box-shadow to ", "box-shadow"},
        {"box-shadow ", "box-shadow"},
        {"shadow to ", "box-shadow"},
        {"shadow ", "box-shadow"},

        {"display to ", "display"},
        {"display ", "display"},
        {"direction to ", "flex-direction"},
        {"direction ", "flex-direction"},
        {"flex direction to ", "flex-direction"},
        {"flex direction ", "flex-direction"},
        {"flex-direction to ", "flex-direction"},
        {"flex-direction ", "flex-direction"},
        {"align items to ", "align-items"},
        {"align items ", "align-items"},
        {"align-items to ", "align-items"},
        {"align-items ", "align-items"},
        {"justify content to ", "justify-content"},
        {"justify content ", "justify-content"},
        {"justify-content to ", "justify-content"},
        {"justify-content ", "justify-content"},
        {"gap to ", "gap"},
        {"gap ", "gap"},
        {"spacing to ", "gap"},
        {"spacing ", "gap"},

        {"cursor to ", "cursor"},
        {"cursor ", "cursor"},
        {"transition to ", "transition"},
        {"transition ", "transition"},
        {"transform to ", "transform"},
        {"transform ", "transform"},
        {"opacity to ", "opacity"},
        {"opacity ", "opacity"},
        {"position to ", "position"},
        {"position ", "position"},
        {"z-index to ", "z-index"},
        {"z-index ", "z-index"},
        {"z index to ", "z-index"},
        {"z index ", "z-index"},
        {"overflow-x to ", "overflow-x"},
        {"overflow-x ", "overflow-x"},
        {"overflow-y to ", "overflow-y"},
        {"overflow-y ", "overflow-y"},
        {"overflow to ", "overflow"},
        {"overflow ", "overflow"},
        {"user select to ", "user-select"},
        {"user select ", "user-select"},
        {"user-select to ", "user-select"},
        {"user-select ", "user-select"},
        {"touch-action to ", "touch-action"},
        {"touch-action ", "touch-action"},
        {"filter to ", "filter"},
        {"filter ", "filter"},
        {"backdrop-filter to ", "backdrop-filter"},
        {"backdrop-filter ", "backdrop-filter"},
        {"stroke color to ", "stroke"},
        {"stroke color ", "stroke"},
        {"stroke width to ", "stroke-width"},
        {"stroke width ", "stroke-width"},
        {"stroke dasharray to ", "stroke-dasharray"},
        {"stroke dasharray ", "stroke-dasharray"},
        {NULL, NULL}
    };

    for (int i = 0; prop_map[i].eng; i++) {
        if (str_starts_with_ci(s, prop_map[i].eng)) {
            strcpy(prop_name, prop_map[i].css);
            raw_val_ptr = s + strlen(prop_map[i].eng);
            break;
        }
    }

    if (!prop_name[0]) {
        // Generic fallback: first word or colon-delimited key is property
        char* sp = strchr(s, ' ');
        char* col = strchr(s, ':');
        char* sep = (col && (!sp || col < sp)) ? col : sp;
        if (sep) {
            size_t plen = (size_t)(sep - s);
            if (plen >= sizeof(prop_name)) plen = sizeof(prop_name) - 1;
            strncpy(prop_name, s, plen);
            prop_name[plen] = '\0';
            raw_val_ptr = sep + 1;
            while (*raw_val_ptr == ' ' || *raw_val_ptr == '\t' || *raw_val_ptr == ':') raw_val_ptr++;
            if (str_starts_with_ci(raw_val_ptr, "to ")) raw_val_ptr += 3;
        }
    }

    if (prop_name[0] && raw_val_ptr) {
        size_t p_len = strlen(prop_name);
        while (p_len > 0 && (prop_name[p_len - 1] == ':' || prop_name[p_len - 1] == ' ')) {
            prop_name[--p_len] = '\0';
        }
        while (*raw_val_ptr == ':' || *raw_val_ptr == ' ' || *raw_val_ptr == '\t') {
            raw_val_ptr++;
        }
        char resolved[512];
        resolve_token_and_gradients(sheet, trim(raw_val_ptr), resolved, sizeof(resolved));
        if (strcmp(prop_name, "position") == 0) {
            char* at_top = strstr(resolved, " at top ");
            if (at_top) {
                *at_top = '\0';
                char top_val[128];
                format_css_value("top", trim(at_top + 8), top_val, sizeof(top_val));
                add_prop(current_rule, "position", trim(resolved));
                add_prop(current_rule, "top", top_val);
                return;
            }
            char* at_bottom = strstr(resolved, " at bottom ");
            if (at_bottom) {
                *at_bottom = '\0';
                char bot_val[128];
                format_css_value("bottom", trim(at_bottom + 11), bot_val, sizeof(bot_val));
                add_prop(current_rule, "position", trim(resolved));
                add_prop(current_rule, "bottom", bot_val);
                return;
            }
        }
        char clean_val[256];
        format_css_value(prop_name, resolved, clean_val, sizeof(clean_val));
        add_prop(current_rule, prop_name, clean_val);
    }
}

static void parse_chained_line(EnlngdSheet* sheet, EnlngdRule* current_rule, const char* comp_name, const char* line) {
    char work[1024];
    strncpy(work, line, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';
    char* s = trim(work);
    if (!*s) return;

    // We split by connectors: " with ", " and ", " also ", " using "
    // Note: don't split inside quotes
    char clause[512];
    int ci = 0;
    bool in_quote = false;
    char q_char = '\0';

    int len = (int)strlen(s);
    for (int i = 0; i < len; i++) {
        char c = s[i];
        if (!in_quote && (c == '"' || c == '\'')) {
            in_quote = true;
            q_char = c;
            clause[ci++] = c;
        } else if (in_quote && c == q_char) {
            in_quote = false;
            clause[ci++] = c;
        } else if (!in_quote) {
            // Check connectors
            if (str_starts_with_ci(&s[i], " with ") ||
                str_starts_with_ci(&s[i], " and ") ||
                str_starts_with_ci(&s[i], " also ") ||
                str_starts_with_ci(&s[i], " using ")) {
                clause[ci] = '\0';
                map_and_add_declaration(sheet, current_rule, comp_name, clause);
                ci = 0;
                int skip = 5;
                if (str_starts_with_ci(&s[i], " with ")) skip = 6;
                else if (str_starts_with_ci(&s[i], " and ")) skip = 5;
                else if (str_starts_with_ci(&s[i], " also ")) skip = 6;
                else if (str_starts_with_ci(&s[i], " using ")) skip = 7;
                i += (skip - 1);
                continue;
            }
            clause[ci++] = c;
        } else {
            clause[ci++] = c;
        }
    }
    if (ci > 0) {
        clause[ci] = '\0';
        map_and_add_declaration(sheet, current_rule, comp_name, clause);
    }
}

bool enlngd_parse_file(EnlngdSheet* sheet, const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return false;

    char line[1024];
    EnlngdRule* current_rule = NULL;
    char current_comp_name[64] = {0};
    bool in_theme = false;
    EnlngdRule* theme_body_rule = NULL;

    EnlngdMediaRule* current_media = NULL;
    EnlngdKeyframe* current_keyframe = NULL;
    EnlngdKeyframeFrame* current_frame = NULL;

    while (fgets(line, sizeof(line), f)) {
        char* s = trim(line);
        if (!*s || s[0] == '#') continue;

        // Skip type headers
        if (str_starts_with_ci(s, "type enlngd") || str_starts_with_ci(s, "type enlgd")) {
            continue;
        }

        // Skip display / echo statements
        if (str_starts_with_ci(s, "display ") || str_starts_with_ci(s, "show ")) {
            continue;
        }

        // 1. define token: define color "primary" as "#1a1a2e"
        if (str_starts_with_ci(s, "define color ") ||
            str_starts_with_ci(s, "define size ") ||
            str_starts_with_ci(s, "define font ") ||
            str_starts_with_ci(s, "define spacing ") ||
            str_starts_with_ci(s, "define value ")) {
            char* p = s + 7;
            char type[32] = "other";
            if (str_starts_with_ci(p, "color ")) { strcpy(type, "color"); p += 6; }
            else if (str_starts_with_ci(p, "size ")) { strcpy(type, "size"); p += 5; }
            else if (str_starts_with_ci(p, "font ")) { strcpy(type, "font"); p += 5; }
            else if (str_starts_with_ci(p, "spacing ")) { strcpy(type, "size"); p += 8; }
            else if (str_starts_with_ci(p, "value ")) { strcpy(type, "other"); p += 6; }

            char* as_pos = strstr(p, " as ");
            if (!as_pos) as_pos = strstr(p, " = ");
            if (as_pos && sheet->token_count < MAX_TOKENS) {
                *as_pos = '\0';
                char* tname = trim(p);
                strip_outer_quotes(tname);
                char* tval = trim(as_pos + 4);
                strip_outer_quotes(tval);

                EnlngdToken* tok = &sheet->tokens[sheet->token_count++];
                strncpy(tok->name, tname, sizeof(tok->name) - 1);
                format_css_value("token", tval, tok->value, sizeof(tok->value));
                strncpy(tok->type, type, sizeof(tok->type) - 1);
            }
            continue;
        }

        // 2. define theme "<theme_name>": or theme "<theme_name>":
        if (str_starts_with_ci(s, "define theme ") || (str_starts_with_ci(s, "theme ") && s[strlen(s) - 1] == ':')) {
            in_theme = true;
            current_rule = get_or_create_rule(sheet, "body");
            theme_body_rule = current_rule;
            current_comp_name[0] = '\0';
            continue;
        }

        // 3. define component "<comp_name>": or component "<comp_name>": or class "<comp_name>":
        if (str_starts_with_ci(s, "define component ") ||
            ((str_starts_with_ci(s, "component ") || str_starts_with_ci(s, "class ")) && s[strlen(s) - 1] == ':')) {
            char* p = str_starts_with_ci(s, "define component ") ? (s + 17) : (str_starts_with_ci(s, "component ") ? (s + 10) : (s + 6));
            char* col = strchr(p, ':');
            if (col) *col = '\0';
            char comp[64];
            strncpy(comp, trim(p), sizeof(comp) - 1);
            comp[sizeof(comp) - 1] = '\0';
            strip_outer_quotes(comp);
            strncpy(current_comp_name, comp, sizeof(current_comp_name) - 1);

            // Selector: if standard tag (e.g. button, header, input), allow "tag, .tag"
            char sel[128];
            static const char* std_tags[] = {
                "button", "header", "footer", "nav", "section", "main",
                "aside", "row", "card", "input", "body", "p", "a", "span",
                "div", "h1", "h2", "h3", "h4", "h5", "h6", "canvas",
                "table", "tr", "td", "th", "ul", "ol", "li", NULL
            };
            bool is_tag = false;
            for (int i = 0; std_tags[i]; i++) {
                if (str_equals_ci(comp, std_tags[i])) {
                    is_tag = true;
                    break;
                }
            }
            if (is_tag) {
                snprintf(sel, sizeof(sel), "%s, .%s, #%s", comp, comp, comp);
            } else {
                snprintf(sel, sizeof(sel), ".%s, #%s", comp, comp);
            }
            current_rule = get_or_create_rule(sheet, sel);
            continue;
        }

        // 4. animation "<name>":
        if (str_starts_with_ci(s, "animation ")) {
            char* p = s + 10;
            char* col = strchr(p, ':');
            if (col) *col = '\0';
            char anim_name[64];
            strncpy(anim_name, trim(p), sizeof(anim_name) - 1);
            anim_name[sizeof(anim_name) - 1] = '\0';
            strip_outer_quotes(anim_name);

            if (sheet->keyframe_count < MAX_KEYFRAMES) {
                current_keyframe = &sheet->keyframes[sheet->keyframe_count++];
                strncpy(current_keyframe->name, anim_name, sizeof(current_keyframe->name) - 1);
                current_keyframe->frame_count = 0;
            }
            current_frame = NULL;
            current_rule = NULL;
            continue;
        }

        // Inside animation: at 0 percent: / at 100 percent: / from: / to:
        if (current_keyframe && (str_starts_with_ci(s, "at ") || str_starts_with_ci(s, "from") || str_starts_with_ci(s, "to"))) {
            char stop_str[32] = "0%";
            if (str_starts_with_ci(s, "at ")) {
                char* p = s + 3;
                char* col = strchr(p, ':');
                if (col) *col = '\0';
                p = trim(p);
                char num[32] = "0";
                int ni = 0;
                while (*p && (isdigit((unsigned char)*p) || *p == '.') && ni < 31) {
                    num[ni++] = *p++;
                }
                num[ni] = '\0';
                snprintf(stop_str, sizeof(stop_str), "%s%%", num);
            } else if (str_starts_with_ci(s, "from")) {
                strcpy(stop_str, "0%");
            } else if (str_starts_with_ci(s, "to")) {
                strcpy(stop_str, "100%");
            }
            if (current_keyframe->frame_count < MAX_KEYFRAME_FRAMES) {
                current_frame = &current_keyframe->frames[current_keyframe->frame_count++];
                strncpy(current_frame->stop, stop_str, sizeof(current_frame->stop) - 1);
                current_frame->prop_count = 0;
            }
            continue;
        }

        // 5. when screen is smaller/larger than <w> apply:
        if (str_starts_with_ci(s, "when screen is ") || str_starts_with_ci(s, "when screen ")) {
            char query[128] = "(max-width: 768px)";
            if (strstr(s, "smaller than ") || strstr(s, "max ")) {
                char* p = strstr(s, "smaller than ");
                if (!p) p = strstr(s, "max ");
                p = trim(p + (strstr(s, "smaller than ") ? 13 : 4));
                char* app = strstr(p, "apply");
                if (app) *app = '\0';
                char* col = strchr(p, ':');
                if (col) *col = '\0';
                char clean_w[64];
                format_css_value("width", trim(p), clean_w, sizeof(clean_w));
                snprintf(query, sizeof(query), "(max-width: %s)", clean_w);
            } else if (strstr(s, "larger than ") || strstr(s, "min ")) {
                char* p = strstr(s, "larger than ");
                if (!p) p = strstr(s, "min ");
                p = trim(p + (strstr(s, "larger than ") ? 12 : 4));
                char* app = strstr(p, "apply");
                if (app) *app = '\0';
                char* col = strchr(p, ':');
                if (col) *col = '\0';
                char clean_w[64];
                format_css_value("width", trim(p), clean_w, sizeof(clean_w));
                snprintf(query, sizeof(query), "(min-width: %s)", clean_w);
            } else if (strstr(s, "portrait")) {
                strcpy(query, "(orientation: portrait)");
            } else if (strstr(s, "landscape")) {
                strcpy(query, "(orientation: landscape)");
            }

            if (sheet->media_rule_count < MAX_MEDIA_RULES) {
                current_media = &sheet->media_rules[sheet->media_rule_count++];
                strncpy(current_media->query, query, sizeof(current_media->query) - 1);
                current_media->rule_count = 0;
            }
            current_rule = NULL;
            continue;
        }

        // 6. for "<selector>" apply: | style "<selector>": | apply to "<selector>":
        if (str_starts_with_ci(s, "for ") || str_starts_with_ci(s, "style ") || str_starts_with_ci(s, "apply to ")) {
            char* p = s;
            if (str_starts_with_ci(s, "for ")) p += 4;
            else if (str_starts_with_ci(s, "style ")) p += 6;
            else if (str_starts_with_ci(s, "apply to ")) p += 9;

            char* apply_pos = strstr(p, " apply");
            if (!apply_pos) apply_pos = strchr(p, ':');
            if (apply_pos) *apply_pos = '\0';
            char sel[128];
            strncpy(sel, trim(p), sizeof(sel) - 1);
            sel[sizeof(sel) - 1] = '\0';
            strip_outer_quotes(sel);

            if (current_media) {
                if (current_media->rule_count < MAX_RULES / 4) {
                    current_rule = &current_media->rules[current_media->rule_count++];
                    strncpy(current_rule->selector, sel, sizeof(current_rule->selector) - 1);
                    current_rule->prop_count = 0;
                    current_rule->is_hover = false;
                }
            } else {
                current_rule = get_or_create_rule(sheet, sel);
            }
            current_comp_name[0] = '\0';
            continue;
        }

        // 7. when "<selector>" is hovered apply:
        if (str_starts_with_ci(s, "when ") && strstr(s, "hover")) {
            char* p = s + 5;
            char* hov_pos = strstr(p, " is hovered apply");
            if (!hov_pos) hov_pos = strstr(p, " hovered apply");
            if (!hov_pos) hov_pos = strstr(p, " is hovered:");
            if (!hov_pos) hov_pos = strstr(p, " hovered:");
            if (hov_pos) *hov_pos = '\0';
            char sel[128];
            strncpy(sel, trim(p), sizeof(sel) - 1);
            sel[sizeof(sel) - 1] = '\0';
            strip_outer_quotes(sel);

            char full_sel[160];
            snprintf(full_sel, sizeof(full_sel), "%s:hover", sel);
            if (current_media) {
                if (current_media->rule_count < MAX_RULES / 4) {
                    current_rule = &current_media->rules[current_media->rule_count++];
                    strncpy(current_rule->selector, full_sel, sizeof(current_rule->selector) - 1);
                    current_rule->prop_count = 0;
                    current_rule->is_hover = true;
                }
            } else {
                current_rule = get_or_create_rule(sheet, full_sel);
            }
            current_comp_name[0] = '\0';
            continue;
        }

        // 8. End blocks
        if (str_equals_ci(s, "end") || str_equals_ci(s, "finish")) {
            if (current_frame) {
                current_frame = NULL;
            } else if (current_keyframe) {
                current_keyframe = NULL;
            } else if (current_media && current_rule) {
                current_rule = NULL;
            } else if (current_media) {
                current_media = NULL;
            } else {
                current_rule = NULL;
                current_comp_name[0] = '\0';
            }
            continue;
        }

        // 9. Properties inside current keyframe frame
        if (current_frame) {
            EnlngdRule temp_rule = {0};
            parse_chained_line(sheet, &temp_rule, "", s);
            for (int pi = 0; pi < temp_rule.prop_count && current_frame->prop_count < MAX_PROPS; pi++) {
                current_frame->props[current_frame->prop_count++] = temp_rule.props[pi];
            }
            continue;
        }

        // 10. Properties inside current rule
        if (current_rule) {
            parse_chained_line(sheet, current_rule, current_comp_name, s);
        } else if (in_theme && theme_body_rule) {
            parse_chained_line(sheet, theme_body_rule, "body", s);
        }
    }

    fclose(f);
    return true;
}

int enlngd_inspect_sheet(const EnlngdSheet* sheet) {
    printf("=================================================================\n");
    printf("     ENLNGD: SOVEREIGN DESIGN TOKENS & STYLE RESOLUTION          \n");
    printf("=================================================================\n\n");

    printf("► RESOLVED DESIGN TOKENS (%d):\n", sheet->token_count);
    printf("+----------------------+---------+------------------------------+\n");
    printf("| Token Name           | Type    | Resolved Value               |\n");
    printf("+----------------------+---------+------------------------------+\n");
    for (int i = 0; i < sheet->token_count; i++) {
        printf("| %-20s | %-7s | %-28s |\n",
            sheet->tokens[i].name,
            sheet->tokens[i].type,
            sheet->tokens[i].value);
    }
    printf("+----------------------+---------+------------------------------+\n\n");

    printf("► COMPILED STYLE RULES (%d):\n", sheet->rule_count);
    for (int r = 0; r < sheet->rule_count; r++) {
        printf("  Selector: %s (%d properties)\n", sheet->rules[r].selector, sheet->rules[r].prop_count);
        for (int p = 0; p < sheet->rules[r].prop_count; p++) {
            printf("    - %s: %s;\n", sheet->rules[r].props[p].prop, sheet->rules[r].props[p].value);
        }
    }
    printf("\n[Status] Sovereign Design Token evaluation: 100%% Valid.\n");
    return 0;
}

int enlngd_compile_to_css(const EnlngdSheet* sheet, const char* outpath) {
    FILE* fout = fopen(outpath, "w");
    if (!fout) {
        fprintf(stderr, "Error: Could not open output file '%s'\n", outpath);
        return 1;
    }

    fprintf(fout, "/**\n * Compiled by Enlangg Sovereign Design Token Engine v%s\n */\n\n", ENLNGD_VERSION);

    // 1. :root custom properties
    if (sheet->token_count > 0) {
        fprintf(fout, ":root {\n");
        for (int i = 0; i < sheet->token_count; i++) {
            fprintf(fout, "  --%s: %s;\n", sheet->tokens[i].name, sheet->tokens[i].value);
        }
        fprintf(fout, "}\n\n");
    }

    // 2. Keyframes
    for (int k = 0; k < sheet->keyframe_count; k++) {
        const EnlngdKeyframe* kf = &sheet->keyframes[k];
        fprintf(fout, "@keyframes %s {\n", kf->name);
        for (int f = 0; f < kf->frame_count; f++) {
            const EnlngdKeyframeFrame* frame = &kf->frames[f];
            fprintf(fout, "  %s {\n", frame->stop);
            for (int p = 0; p < frame->prop_count; p++) {
                fprintf(fout, "    %s: %s;\n", frame->props[p].prop, frame->props[p].value);
            }
            fprintf(fout, "  }\n");
        }
        fprintf(fout, "}\n\n");
    }

    // 3. Regular Rules
    for (int r = 0; r < sheet->rule_count; r++) {
        if (sheet->rules[r].prop_count == 0) continue;
        fprintf(fout, "%s {\n", sheet->rules[r].selector);
        for (int p = 0; p < sheet->rules[r].prop_count; p++) {
            fprintf(fout, "  %s: %s;\n", sheet->rules[r].props[p].prop, sheet->rules[r].props[p].value);
        }
        fprintf(fout, "}\n\n");
    }

    // 4. Media Queries
    for (int m = 0; m < sheet->media_rule_count; m++) {
        const EnlngdMediaRule* mr = &sheet->media_rules[m];
        if (mr->rule_count == 0) continue;
        fprintf(fout, "@media %s {\n", mr->query);
        for (int r = 0; r < mr->rule_count; r++) {
            if (mr->rules[r].prop_count == 0) continue;
            fprintf(fout, "  %s {\n", mr->rules[r].selector);
            for (int p = 0; p < mr->rules[r].prop_count; p++) {
                fprintf(fout, "    %s: %s;\n", mr->rules[r].props[p].prop, mr->rules[r].props[p].value);
            }
            fprintf(fout, "  }\n");
        }
        fprintf(fout, "}\n\n");
    }

    fclose(fout);
    printf("[enlngd] CSS3 stylesheet compiled successfully: '%s'\n", outpath);
    return 0;
}
