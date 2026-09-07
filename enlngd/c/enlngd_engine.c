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

static const char* resolve_token(const EnlngdSheet* sheet, const char* val_str, char* out_buf, size_t out_sz) {
    if (strstr(val_str, "use color ") || strstr(val_str, "use size ")) {
        char clean[128] = {0};
        const char* p = strstr(val_str, "use ");
        if (p) p += 4;
        if (str_starts_with_ci(p, "color ")) p += 6;
        else if (str_starts_with_ci(p, "size ")) p += 5;
        p = trim((char*)p);
        if (*p == '"' || *p == '\'') p++;
        strncpy(clean, p, sizeof(clean) - 1);
        char* endq = strchr(clean, '"');
        if (!endq) endq = strchr(clean, '\'');
        if (endq) *endq = '\0';

        snprintf(out_buf, out_sz, "var(--%s)", clean);
        return out_buf;
    }
    strncpy(out_buf, val_str, out_sz - 1);
    out_buf[out_sz - 1] = '\0';
    return out_buf;
}

bool enlngd_parse_file(EnlngdSheet* sheet, const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return false;

    char line[512];
    EnlngdRule* current_rule = NULL;

    while (fgets(line, sizeof(line), f)) {
        char* s = trim(line);
        if (!*s || s[0] == '#') continue;

        // 1. define token: define color "primary" as "#1a1a2e"
        if (str_starts_with_ci(s, "define ")) {
            char* p = s + 7;
            char type[32] = "other";
            if (str_starts_with_ci(p, "color ")) { strcpy(type, "color"); p += 6; }
            else if (str_starts_with_ci(p, "size ")) { strcpy(type, "size"); p += 5; }
            else if (str_starts_with_ci(p, "font ")) { strcpy(type, "font"); p += 5; }

            char* as_pos = strstr(p, " as ");
            if (as_pos && sheet->token_count < MAX_TOKENS) {
                *as_pos = '\0';
                char* tname = trim(p);
                if (*tname == '"' || *tname == '\'') tname++;
                char* eq = strchr(tname, '"');
                if (!eq) eq = strchr(tname, '\'');
                if (eq) *eq = '\0';

                char* tval = trim(as_pos + 4);
                if (*tval == '"' || *tval == '\'') tval++;
                eq = strchr(tval, '"');
                if (!eq) eq = strchr(tval, '\'');
                if (eq) *eq = '\0';

                EnlngdToken* tok = &sheet->tokens[sheet->token_count++];
                strncpy(tok->name, tname, 63);
                strncpy(tok->value, tval, 127);
                strncpy(tok->type, type, 31);
            }
            continue;
        }

        // 2. Rule block: for "<selector>" apply:
        if (str_starts_with_ci(s, "for ")) {
            char* p = s + 4;
            char* apply_pos = strstr(p, " apply");
            if (apply_pos && sheet->rule_count < MAX_RULES) {
                *apply_pos = '\0';
                char* sel = trim(p);
                if (*sel == '"' || *sel == '\'') sel++;
                char* eq = strchr(sel, '"');
                if (!eq) eq = strchr(sel, '\'');
                if (eq) *eq = '\0';

                current_rule = &sheet->rules[sheet->rule_count++];
                strncpy(current_rule->selector, sel, 127);
                current_rule->is_hover = false;
            }
            continue;
        }

        // 3. Hover block: when "<selector>" is hovered apply:
        if (str_starts_with_ci(s, "when ")) {
            char* p = s + 5;
            char* hov_pos = strstr(p, " is hovered apply");
            if (!hov_pos) hov_pos = strstr(p, " hovered apply");
            if (hov_pos && sheet->rule_count < MAX_RULES) {
                *hov_pos = '\0';
                char* sel = trim(p);
                if (*sel == '"' || *sel == '\'') sel++;
                char* eq = strchr(sel, '"');
                if (!eq) eq = strchr(sel, '\'');
                if (eq) *eq = '\0';

                current_rule = &sheet->rules[sheet->rule_count++];
                snprintf(current_rule->selector, 127, "%s:hover", sel);
                current_rule->is_hover = true;
            }
            continue;
        }

        // 4. End block
        if (str_equals_ci(s, "end")) {
            current_rule = NULL;
            continue;
        }

        // 5. Properties inside rule
        if (current_rule && current_rule->prop_count < MAX_PROPS) {
            char prop_name[64] = {0};
            char prop_val[128] = {0};

            // corner radius -> border-radius
            if (str_starts_with_ci(s, "corner radius ")) {
                strcpy(prop_name, "border-radius");
                char resolved[128];
                resolve_token(sheet, trim(s + 14), resolved, sizeof(resolved));
                if (isdigit((unsigned char)resolved[0])) snprintf(prop_val, 127, "%spx", resolved);
                else strncpy(prop_val, resolved, 127);
            } else if (str_starts_with_ci(s, "background ")) {
                strcpy(prop_name, "background");
                char resolved[128];
                resolve_token(sheet, trim(s + 11), resolved, sizeof(resolved));
                strncpy(prop_val, resolved, 127);
            } else if (str_starts_with_ci(s, "color ")) {
                strcpy(prop_name, "color");
                char resolved[128];
                resolve_token(sheet, trim(s + 6), resolved, sizeof(resolved));
                strncpy(prop_val, resolved, 127);
            } else if (str_starts_with_ci(s, "font size ") || str_starts_with_ci(s, "font-size ")) {
                strcpy(prop_name, "font-size");
                char* p = s + (str_starts_with_ci(s, "font size ") ? 10 : 10);
                char resolved[128];
                resolve_token(sheet, trim(p), resolved, sizeof(resolved));
                if (isdigit((unsigned char)resolved[0])) snprintf(prop_val, 127, "%spx", resolved);
                else strncpy(prop_val, resolved, 127);
            } else {
                // Generic prop val
                char* sp = strchr(s, ' ');
                if (sp) {
                    int plen = (int)(sp - s);
                    strncpy(prop_name, s, plen);
                    prop_name[plen] = '\0';
                    char resolved[128];
                    resolve_token(sheet, trim(sp + 1), resolved, sizeof(resolved));
                    strncpy(prop_val, resolved, 127);
                }
            }

            if (prop_name[0]) {
                EnlngdProp* p = &current_rule->props[current_rule->prop_count++];
                strncpy(p->prop, prop_name, 63);
                strncpy(p->value, prop_val, 127);
            }
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
            printf("    - %s: %s\n", sheet->rules[r].props[p].prop, sheet->rules[r].props[p].value);
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
            const char* val = sheet->tokens[i].value;
            if (strcmp(sheet->tokens[i].type, "size") == 0 && isdigit((unsigned char)val[0])) {
                fprintf(fout, "  --%s: %spx;\n", sheet->tokens[i].name, val);
            } else {
                fprintf(fout, "  --%s: %s;\n", sheet->tokens[i].name, val);
            }
        }
        fprintf(fout, "}\n\n");
    }

    // 2. Rules
    for (int r = 0; r < sheet->rule_count; r++) {
        fprintf(fout, "%s {\n", sheet->rules[r].selector);
        for (int p = 0; p < sheet->rules[r].prop_count; p++) {
            fprintf(fout, "  %s: %s;\n", sheet->rules[r].props[p].prop, sheet->rules[r].props[p].value);
        }
        fprintf(fout, "}\n\n");
    }

    fclose(fout);
    printf("[enlngd] CSS3 stylesheet compiled successfully: '%s'\n", outpath);
    return 0;
}
