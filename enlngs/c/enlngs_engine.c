#include "enlngs.h"

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

static bool str_equals_ci(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++; b++;
    }
    return *a == *b;
}

static bool str_starts_with_ci(const char* s, const char* prefix) {
    if (!s || !prefix) return false;
    while (*prefix) {
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) return false;
        s++; prefix++;
    }
    return true;
}

EnlngsVal enlngs_val_null(void) {
    EnlngsVal v;
    v.type = VAL_NULL;
    return v;
}

EnlngsVal enlngs_val_bool(bool b) {
    EnlngsVal v;
    v.type = VAL_BOOL;
    v.b_val = b;
    return v;
}

EnlngsVal enlngs_val_int(long long i) {
    EnlngsVal v;
    v.type = VAL_INT;
    v.i_val = i;
    return v;
}

EnlngsVal enlngs_val_double(double d) {
    EnlngsVal v;
    v.type = VAL_DOUBLE;
    v.d_val = d;
    return v;
}

EnlngsVal enlngs_val_string(const char* s) {
    EnlngsVal v;
    v.type = VAL_STRING;
    strncpy(v.s_val, s ? s : "", sizeof(v.s_val) - 1);
    v.s_val[sizeof(v.s_val) - 1] = '\0';
    return v;
}

char* enlngs_val_to_str(EnlngsVal v, char* buf, size_t bufsize) {
    switch (v.type) {
        case VAL_NULL:
            snprintf(buf, bufsize, "null");
            break;
        case VAL_BOOL:
            snprintf(buf, bufsize, "%s", v.b_val ? "true" : "false");
            break;
        case VAL_INT:
            snprintf(buf, bufsize, "%ld", (long)v.i_val);
            break;
        case VAL_DOUBLE:
            snprintf(buf, bufsize, "%g", v.d_val);
            break;
        case VAL_STRING:
            snprintf(buf, bufsize, "%s", v.s_val);
            break;
    }
    return buf;
}

void enlngs_print_val(EnlngsVal v) {
    char buf[512];
    printf("%s", enlngs_val_to_str(v, buf, sizeof(buf)));
}

EnlngsVM* enlngs_create_vm(void) {
    EnlngsVM* vm = (EnlngsVM*)calloc(1, sizeof(EnlngsVM));
    return vm;
}

void enlngs_free_vm(EnlngsVM* vm) {
    if (vm) free(vm);
}

static EnlngsVar* find_var(EnlngsVM* vm, const char* name) {
    for (int i = 0; i < vm->var_count; i++) {
        if (strcmp(vm->vars[i].name, name) == 0) {
            return &vm->vars[i];
        }
    }
    return NULL;
}

static void set_var(EnlngsVM* vm, const char* name, EnlngsVal val, bool is_const) {
    EnlngsVar* existing = find_var(vm, name);
    if (existing) {
        if (existing->is_const) {
            fprintf(stderr, "RuntimeError: Cannot reassign constant variable '%s'\n", name);
            return;
        }
        existing->val = val;
        return;
    }
    if (vm->var_count < MAX_VARS) {
        strncpy(vm->vars[vm->var_count].name, name, 63);
        vm->vars[vm->var_count].name[63] = '\0';
        vm->vars[vm->var_count].val = val;
        vm->vars[vm->var_count].is_const = is_const;
        vm->var_count++;
    }
}

EnlngsVal enlngs_eval_expr(EnlngsVM* vm, const char* expr) {
    char buf[512];
    strncpy(buf, expr, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char* s = trim(buf);

    if (!*s) return enlngs_val_null();

    // Boolean literals
    if (str_equals_ci(s, "true")) return enlngs_val_bool(true);
    if (str_equals_ci(s, "false")) return enlngs_val_bool(false);
    if (str_equals_ci(s, "null")) return enlngs_val_null();

    // String literal
    if ((s[0] == '"' && s[strlen(s) - 1] == '"') || (s[0] == '\'' && s[strlen(s) - 1] == '\'')) {
        s[strlen(s) - 1] = '\0';
        return enlngs_val_string(s + 1);
    }

    // Binary addition / concatenation
    char* plus = strchr(s, '+');
    if (plus && plus != s && *(plus - 1) != 'e' && *(plus - 1) != 'E') {
        *plus = '\0';
        EnlngsVal left = enlngs_eval_expr(vm, trim(s));
        EnlngsVal right = enlngs_eval_expr(vm, trim(plus + 1));
        if (left.type == VAL_STRING || right.type == VAL_STRING) {
            char b1[512], b2[512], comb[1024];
            snprintf(comb, sizeof(comb), "%s%s",
                     enlngs_val_to_str(left, b1, sizeof(b1)),
                     enlngs_val_to_str(right, b2, sizeof(b2)));
            return enlngs_val_string(comb);
        }
        if (left.type == VAL_DOUBLE || right.type == VAL_DOUBLE) {
            double v1 = (left.type == VAL_DOUBLE) ? left.d_val : left.i_val;
            double v2 = (right.type == VAL_DOUBLE) ? right.d_val : right.i_val;
            return enlngs_val_double(v1 + v2);
        }
        return enlngs_val_int(left.i_val + right.i_val);
    }

    // Binary subtraction
    char* minus = strchr(s, '-');
    if (minus && minus != s && *(minus - 1) != 'e' && *(minus - 1) != 'E' && isspace((unsigned char)*(minus - 1))) {
        *minus = '\0';
        EnlngsVal left = enlngs_eval_expr(vm, trim(s));
        EnlngsVal right = enlngs_eval_expr(vm, trim(minus + 1));
        if (left.type == VAL_DOUBLE || right.type == VAL_DOUBLE) {
            double v1 = (left.type == VAL_DOUBLE) ? left.d_val : left.i_val;
            double v2 = (right.type == VAL_DOUBLE) ? right.d_val : right.i_val;
            return enlngs_val_double(v1 - v2);
        }
        return enlngs_val_int(left.i_val - right.i_val);
    }

    // Comparison: is equal to / ==
    if (strstr(s, " is equal to ") || strstr(s, " == ")) {
        char* pos = strstr(s, " is equal to ");
        int len = 13;
        if (!pos) { pos = strstr(s, " == "); len = 4; }
        *pos = '\0';
        EnlngsVal left = enlngs_eval_expr(vm, trim(s));
        EnlngsVal right = enlngs_eval_expr(vm, trim(pos + len));
        if (left.type == VAL_STRING && right.type == VAL_STRING) {
            return enlngs_val_bool(strcmp(left.s_val, right.s_val) == 0);
        }
        double v1 = (left.type == VAL_DOUBLE) ? left.d_val : left.i_val;
        double v2 = (right.type == VAL_DOUBLE) ? right.d_val : right.i_val;
        return enlngs_val_bool(v1 == v2);
    }

    // Comparison: >
    if (strstr(s, " > ") || strstr(s, " is greater than ")) {
        char* pos = strstr(s, " > ");
        int len = 3;
        if (!pos) { pos = strstr(s, " is greater than "); len = 17; }
        *pos = '\0';
        EnlngsVal left = enlngs_eval_expr(vm, trim(s));
        EnlngsVal right = enlngs_eval_expr(vm, trim(pos + len));
        double v1 = (left.type == VAL_DOUBLE) ? left.d_val : left.i_val;
        double v2 = (right.type == VAL_DOUBLE) ? right.d_val : right.i_val;
        return enlngs_val_bool(v1 > v2);
    }

    // Comparison: <
    if (strstr(s, " < ") || strstr(s, " is less than ")) {
        char* pos = strstr(s, " < ");
        int len = 3;
        if (!pos) { pos = strstr(s, " is less than "); len = 14; }
        *pos = '\0';
        EnlngsVal left = enlngs_eval_expr(vm, trim(s));
        EnlngsVal right = enlngs_eval_expr(vm, trim(pos + len));
        double v1 = (left.type == VAL_DOUBLE) ? left.d_val : left.i_val;
        double v2 = (right.type == VAL_DOUBLE) ? right.d_val : right.i_val;
        return enlngs_val_bool(v1 < v2);
    }

    // Number literal
    char* endptr;
    long long ival = strtoll(s, &endptr, 10);
    if (*endptr == '\0') {
        return enlngs_val_int(ival);
    }
    double dval = strtod(s, &endptr);
    if (*endptr == '\0') {
        return enlngs_val_double(dval);
    }

    // Variable lookup
    EnlngsVar* v = find_var(vm, s);
    if (v) return v->val;

    return enlngs_val_string(s);
}

bool enlngs_exec_line(EnlngsVM* vm, const char* line) {
    char buf[MAX_LINE_LEN];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char* s = trim(buf);

    if (!*s || s[0] == '#' || strncmp(s, "//", 2) == 0) return true;

    // 1. Output commands: show <expr> / display <expr>
    if (str_starts_with_ci(s, "show ") || str_starts_with_ci(s, "display ")) {
        char* expr = trim(s + (str_starts_with_ci(s, "show ") ? 5 : 8));
        EnlngsVal val = enlngs_eval_expr(vm, expr);
        enlngs_print_val(val);
        printf("\n");
        return true;
    }

    // 2. Variable declarations: create [a|an] <name> [as|=|of] <expr> / define ... / declare ...
    if (str_starts_with_ci(s, "create ") || str_starts_with_ci(s, "define ") ||
        str_starts_with_ci(s, "let ") || str_starts_with_ci(s, "declare ")) {
        bool is_const = str_starts_with_ci(s, "define ");
        char* p = s;
        while (*p && !isspace((unsigned char)*p)) p++;
        while (isspace((unsigned char)*p)) p++;

        if (str_starts_with_ci(p, "a ") && !str_starts_with_ci(p + 2, "as ") && !str_starts_with_ci(p + 2, "of ") && p[2] != '=') p += 2;
        else if (str_starts_with_ci(p, "an ") && !str_starts_with_ci(p + 3, "as ") && !str_starts_with_ci(p + 3, "of ") && p[3] != '=') p += 3;
        while (isspace((unsigned char)*p)) p++;

        char* as_pos = strstr(p, " as ");
        if (!as_pos) as_pos = strstr(p, " of ");
        if (!as_pos) as_pos = strstr(p, " = ");
        if (!as_pos) as_pos = strchr(p, '=');

        char varname[64] = {0};
        if (as_pos) {
            int nlen = (int)(as_pos - p);
            if (nlen >= 63) nlen = 63;
            strncpy(varname, p, nlen);
            varname[nlen] = '\0';
            trim(varname);

            char* val_str = as_pos;
            if (strncmp(val_str, " as ", 4) == 0) val_str += 4;
            else if (strncmp(val_str, " of ", 4) == 0) val_str += 4;
            else if (strncmp(val_str, " = ", 3) == 0) val_str += 3;
            else if (*val_str == '=') val_str += 1;

            EnlngsVal val = enlngs_eval_expr(vm, trim(val_str));
            set_var(vm, varname, val, is_const);
        } else {
            sscanf(p, "%63s", varname);
            set_var(vm, varname, enlngs_val_null(), is_const);
        }
        return true;
    }

    // 3. Increment / Decrement: increase <var> by <val> / decrease <var> by <val>
    if (str_starts_with_ci(s, "increase ")) {
        char varname[64] = {0};
        char* p = s + 9;
        char* by_pos = strstr(p, " by ");
        if (by_pos) {
            int nlen = (int)(by_pos - p);
            strncpy(varname, p, nlen);
            varname[nlen] = '\0';
            trim(varname);
            EnlngsVal delta = enlngs_eval_expr(vm, trim(by_pos + 4));
            EnlngsVar* v = find_var(vm, varname);
            if (v) {
                if (v->val.type == VAL_DOUBLE) v->val.d_val += (delta.type == VAL_DOUBLE ? delta.d_val : delta.i_val);
                else v->val.i_val += (delta.type == VAL_DOUBLE ? (long long)delta.d_val : delta.i_val);
            }
        }
        return true;
    }
    if (str_starts_with_ci(s, "decrease ")) {
        char varname[64] = {0};
        char* p = s + 9;
        char* by_pos = strstr(p, " by ");
        if (by_pos) {
            int nlen = (int)(by_pos - p);
            strncpy(varname, p, nlen);
            varname[nlen] = '\0';
            trim(varname);
            EnlngsVal delta = enlngs_eval_expr(vm, trim(by_pos + 4));
            EnlngsVar* v = find_var(vm, varname);
            if (v) {
                if (v->val.type == VAL_DOUBLE) v->val.d_val -= (delta.type == VAL_DOUBLE ? delta.d_val : delta.i_val);
                else v->val.i_val -= (delta.type == VAL_DOUBLE ? (long long)delta.d_val : delta.i_val);
            }
        }
        return true;
    }

    // 4. Assignment: set <var> to <val> / <var> = <val>
    if (str_starts_with_ci(s, "set ")) {
        char* p = s + 4;
        char* to_pos = strstr(p, " to ");
        if (!to_pos) to_pos = strstr(p, " = ");
        if (to_pos) {
            char varname[64] = {0};
            int nlen = (int)(to_pos - p);
            strncpy(varname, p, nlen);
            varname[nlen] = '\0';
            trim(varname);
            char* val_str = to_pos + (strncmp(to_pos, " to ", 4) == 0 ? 4 : 3);
            EnlngsVal val = enlngs_eval_expr(vm, trim(val_str));
            set_var(vm, varname, val, false);
            return true;
        }
    }

    char* eq = strchr(s, '=');
    if (eq && eq != s && *(eq - 1) != '!' && *(eq - 1) != '>' && *(eq - 1) != '<' && *(eq + 1) != '=') {
        char varname[64] = {0};
        int nlen = (int)(eq - s);
        strncpy(varname, s, nlen);
        varname[nlen] = '\0';
        trim(varname);
        EnlngsVal val = enlngs_eval_expr(vm, trim(eq + 1));
        set_var(vm, varname, val, false);
        return true;
    }

    // 5. Function Call: call <func>(<args>)
    if (str_starts_with_ci(s, "call ")) {
        char* fname = trim(s + 5);
        for (int i = 0; i < vm->func_count; i++) {
            if (strcmp(vm->funcs[i].name, fname) == 0) {
                for (int b = 0; b < vm->funcs[i].body_count; b++) {
                    enlngs_exec_line(vm, vm->funcs[i].body[b]);
                }
                return true;
            }
        }
    }

    // 6. Return
    if (str_starts_with_ci(s, "return ")) {
        vm->return_val = enlngs_eval_expr(vm, trim(s + 7));
        vm->has_returned = true;
        return true;
    }

    return true;
}

bool enlngs_exec_script(EnlngsVM* vm, const char* script) {
    if (!script) return false;
    char buffer[MAX_LINE_LEN];
    const char* p = script;

    while (*p) {
        int idx = 0;
        while (*p && *p != '\n' && idx < MAX_LINE_LEN - 1) {
            buffer[idx++] = *p++;
        }
        buffer[idx] = '\0';
        if (*p == '\n') p++;

        char* line = trim(buffer);
        if (!*line || line[0] == '#' || strncmp(line, "//", 2) == 0) continue;

        // Loop construct: repeat <N> times:
        if (str_starts_with_ci(line, "repeat ")) {
            char* times_pos = strstr(line, " times");
            long long count = 0;
            if (times_pos) {
                *times_pos = '\0';
                count = enlngs_eval_expr(vm, line + 7).i_val;
            }
            char body_lines[64][MAX_LINE_LEN];
            int bcount = 0;
            while (*p && bcount < 64) {
                idx = 0;
                while (*p && *p != '\n' && idx < MAX_LINE_LEN - 1) {
                    buffer[idx++] = *p++;
                }
                buffer[idx] = '\0';
                if (*p == '\n') p++;
                char* bline = trim(buffer);
                if (str_equals_ci(bline, "end") || str_equals_ci(bline, "end repeat")) break;
                if (*bline && bline[0] != '#') {
                    strncpy(body_lines[bcount++], bline, MAX_LINE_LEN - 1);
                }
            }
            for (long long c = 0; c < count; c++) {
                for (int b = 0; b < bcount; b++) {
                    enlngs_exec_line(vm, body_lines[b]);
                }
            }
            continue;
        }

        // Functions: function / to do
        if (str_starts_with_ci(line, "function ") || str_starts_with_ci(line, "to do ")) {
            if (vm->func_count < MAX_FUNCS) {
                EnlngsFunc* fn = &vm->funcs[vm->func_count++];
                char* name_start = line + (str_starts_with_ci(line, "function ") ? 9 : 6);
                char* with_pos = strstr(name_start, " with ");
                char* colon = strchr(name_start, ':');
                if (colon) *colon = '\0';

                if (with_pos) {
                    *with_pos = '\0';
                    strncpy(fn->name, trim(name_start), 63);
                    char* param_str = trim(with_pos + 6);
                    sscanf(param_str, "%63s", fn->params[0]);
                    fn->param_count = 1;
                } else {
                    strncpy(fn->name, trim(name_start), 63);
                }

                while (*p && fn->body_count < MAX_BODY_LINES) {
                    idx = 0;
                    while (*p && *p != '\n' && idx < MAX_LINE_LEN - 1) {
                        buffer[idx++] = *p++;
                    }
                    buffer[idx] = '\0';
                    if (*p == '\n') p++;
                    char* bline = trim(buffer);
                    if (str_equals_ci(bline, "end") || str_equals_ci(bline, "end to do")) break;
                    if (*bline && bline[0] != '#') {
                        strncpy(fn->body[fn->body_count++], bline, MAX_LINE_LEN - 1);
                    }
                }
            }
            continue;
        }

        // Conditionals: if <cond>:
        if (str_starts_with_ci(line, "if ")) {
            char* cond_str = line + 3;
            char* colon = strchr(cond_str, ':');
            if (colon) *colon = '\0';
            EnlngsVal cond = enlngs_eval_expr(vm, trim(cond_str));
            bool is_true = (cond.type == VAL_BOOL && cond.b_val) ||
                           (cond.type == VAL_INT && cond.i_val != 0);

            char true_body[64][MAX_LINE_LEN];
            int true_count = 0;
            char false_body[64][MAX_LINE_LEN];
            int false_count = 0;
            bool in_else = false;

            while (*p) {
                idx = 0;
                while (*p && *p != '\n' && idx < MAX_LINE_LEN - 1) {
                    buffer[idx++] = *p++;
                }
                buffer[idx] = '\0';
                if (*p == '\n') p++;
                char* bline = trim(buffer);
                if (str_equals_ci(bline, "end") || str_equals_ci(bline, "end if")) break;
                if (str_starts_with_ci(bline, "else")) {
                    in_else = true;
                    continue;
                }
                if (*bline && bline[0] != '#') {
                    if (!in_else && true_count < 64) {
                        strncpy(true_body[true_count++], bline, MAX_LINE_LEN - 1);
                    } else if (in_else && false_count < 64) {
                        strncpy(false_body[false_count++], bline, MAX_LINE_LEN - 1);
                    }
                }
            }

            if (is_true) {
                for (int b = 0; b < true_count; b++) enlngs_exec_line(vm, true_body[b]);
            } else {
                for (int b = 0; b < false_count; b++) enlngs_exec_line(vm, false_body[b]);
            }
            continue;
        }

        enlngs_exec_line(vm, line);
    }
    return true;
}

int enlngs_run_file(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "Error: Could not open script file '%s'\n", filepath);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = (char*)malloc(sz + 1);
    if (!content) {
        fclose(f);
        return 1;
    }
    fread(content, 1, sz, f);
    content[sz] = '\0';
    fclose(f);

    EnlngsVM* vm = enlngs_create_vm();
    enlngs_exec_script(vm, content);
    enlngs_free_vm(vm);
    free(content);
    return 0;
}

static void transform_js_expr(const char* in, char* out, size_t out_sz) {
    if (!in || !out || out_sz == 0) return;
    char buf[16384];
    strncpy(buf, in, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* len_call = strstr(buf, "call length with ");
    if (len_call) {
        char target[256];
        strncpy(target, len_call + 17, sizeof(target) - 1);
        target[sizeof(target) - 1] = '\0';
        char* t = trim(target);
        snprintf(buf, sizeof(buf), "(%s).length", t);
    }

    char* b_open = strchr(buf, '[');
    char* to_pos = b_open ? strstr(b_open, " to ") : NULL;
    char* b_close = to_pos ? strchr(to_pos, ']') : NULL;
    // Only treat as slice if preceded by an identifier char, and no quotes/commas inside [ ... to ... ]
    if (b_open && to_pos && b_close && b_open > buf &&
        (isalnum((unsigned char)*(b_open - 1)) || *(b_open - 1) == '_' || *(b_open - 1) == ')')) {
        bool has_quote_or_comma = false;
        for (const char* p = b_open; p <= b_close; p++) {
            if (*p == '"' || *p == '\'' || *p == ',') {
                has_quote_or_comma = true;
                break;
            }
        }
        if (!has_quote_or_comma) {
            *b_open = '\0';
            *to_pos = '\0';
            *b_close = '\0';
            char* base = trim(buf);
            char* start_e = trim(b_open + 1);
            char* end_e = trim(to_pos + 4);
            snprintf(out, out_sz, "%s.slice(%s, %s)", base, start_e, end_e);
            return;
        }
    }

    char res[16384] = {0};
    const char* src = buf;
    while (*src) {
        if (str_starts_with_ci(src, "get value of ")) {
            src += 13;
            while (*src && isspace((unsigned char)*src)) src++;
            char target[128] = {0};
            int ti = 0;
            if (*src == '"' || *src == '\'') {
                char q = *src++;
                while (*src && *src != q && ti < 127) target[ti++] = *src++;
                if (*src == q) src++;
            } else {
                while (*src && !isspace((unsigned char)*src) && *src != ')' && *src != ',' && ti < 127) {
                    target[ti++] = *src++;
                }
            }
            char elem_code[256];
            snprintf(elem_code, sizeof(elem_code), "_enlEl(\"%s\").value", target);
            strncat(res, elem_code, sizeof(res) - strlen(res) - 1);
        } else if (str_starts_with_ci(src, " and ")) {
            strncat(res, " && ", sizeof(res) - strlen(res) - 1);
            src += 5;
        } else if (str_starts_with_ci(src, " or ")) {
            strncat(res, " || ", sizeof(res) - strlen(res) - 1);
            src += 4;
        } else if (str_starts_with_ci(src, "get element ")) {
            src += 12;
            while (*src && isspace((unsigned char)*src)) src++;
            char target[128] = {0};
            int ti = 0;
            if (*src == '"' || *src == '\'') {
                char q = *src++;
                while (*src && *src != q && ti < 127) target[ti++] = *src++;
                if (*src == q) src++;
            } else {
                while (*src && !isspace((unsigned char)*src) && *src != ')' && *src != ',' && ti < 127) {
                    target[ti++] = *src++;
                }
            }
            char elem_code[256];
            snprintf(elem_code, sizeof(elem_code), "_enlEl(\"%s\")", target);
            strncat(res, elem_code, sizeof(res) - strlen(res) - 1);
        } else if (str_starts_with_ci(src, " is equal to ")) {
            strncat(res, " === ", sizeof(res) - strlen(res) - 1);
            src += 13;
        } else if (str_starts_with_ci(src, " is not equal to ")) {
            strncat(res, " !== ", sizeof(res) - strlen(res) - 1);
            src += 17;
        } else if (str_starts_with_ci(src, " is less than ")) {
            strncat(res, " < ", sizeof(res) - strlen(res) - 1);
            src += 14;
        } else if (str_starts_with_ci(src, " is greater than ")) {
            strncat(res, " > ", sizeof(res) - strlen(res) - 1);
            src += 17;
        } else if (str_starts_with_ci(src, " is at most ")) {
            strncat(res, " <= ", sizeof(res) - strlen(res) - 1);
            src += 12;
        } else if (str_starts_with_ci(src, " is at least ")) {
            strncat(res, " >= ", sizeof(res) - strlen(res) - 1);
            src += 13;
        } else if (str_starts_with_ci(src, " is true")) {
            strncat(res, " === true", sizeof(res) - strlen(res) - 1);
            src += 8;
        } else if (str_starts_with_ci(src, " is false")) {
            strncat(res, " === false", sizeof(res) - strlen(res) - 1);
            src += 9;
        } else if (str_starts_with_ci(src, "rand()")) {
            strncat(res, "Math.random()", sizeof(res) - strlen(res) - 1);
            src += 6;
        } else if (str_starts_with_ci(src, "floor(")) {
            strncat(res, "Math.floor(", sizeof(res) - strlen(res) - 1);
            src += 6;
        } else {
            size_t rlen = strlen(res);
            if (rlen + 1 < sizeof(res)) {
                res[rlen] = *src;
                res[rlen + 1] = '\0';
            }
            src++;
        }
    }
    strncpy(out, trim(res), out_sz - 1);
    out[out_sz - 1] = '\0';
}

static void extract_target_id(const char* s, char* out_id, size_t out_sz) {
    out_id[0] = '\0';
    const char* q1 = strchr(s, '"');
    if (!q1) q1 = strchr(s, '\'');
    if (q1) {
        char quote = *q1;
        const char* q2 = strchr(q1 + 1, quote);
        if (q2) {
            size_t len = (size_t)(q2 - q1 - 1);
            if (len >= out_sz) len = out_sz - 1;
            strncpy(out_id, q1 + 1, len);
            out_id[len] = '\0';
            return;
        }
    }
    // Fallback: unquoted target
    const char* p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    size_t i = 0;
    while (*p && !isspace((unsigned char)*p) && *p != ':' && i + 1 < out_sz) {
        out_id[i++] = *p++;
    }
    out_id[i] = '\0';
}

static char* find_last_token(char* s, const char* token) {
    char* last = NULL;
    char* p = s;
    int depth = 0;
    bool in_quote = false;
    char q = 0;
    size_t tlen = strlen(token);

    while (*p) {
        if (!in_quote && (*p == '"' || *p == '\'')) {
            in_quote = true; q = *p;
        } else if (in_quote && *p == q) {
            in_quote = false;
        } else if (!in_quote) {
            if (*p == '{' || *p == '[' || *p == '(') depth++;
            else if (*p == '}' || *p == ']' || *p == ')') depth--;
            else if (depth == 0 && strncmp(p, token, tlen) == 0) {
                last = p;
            }
        }
        p++;
    }
    return last;
}

enum BlockType {
    BLOCK_BRACE = 1,
    BLOCK_EVENT = 2,
    BLOCK_TIMEOUT = 3,
    BLOCK_INTERVAL = 4,
    BLOCK_FETCH = 5,
    BLOCK_IF = 6
};

static void emit_close_block(FILE* fout, int type, int data) {
    switch (type) {
        case BLOCK_EVENT:
        case BLOCK_FETCH:
            fprintf(fout, "});\n");
            break;
        case BLOCK_TIMEOUT:
        case BLOCK_INTERVAL:
            fprintf(fout, "}, %d);\n", data > 0 ? data : 1000);
            break;
        case BLOCK_BRACE:
        case BLOCK_IF:
        default:
            fprintf(fout, "}\n");
            break;
    }
}

int enlngs_compile_to_js(const char* filepath, const char* outpath) {
    FILE* fin = fopen(filepath, "r");
    if (!fin) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filepath);
        return 1;
    }
    char outname[256];
    if (!outpath) {
        snprintf(outname, sizeof(outname), "%s.js", filepath);
        outpath = outname;
    }
    FILE* fout = fopen(outpath, "w");
    if (!fout) {
        fclose(fin);
        return 1;
    }

    fprintf(fout, "/**\n * Compiled by Enlangg Sovereign Script Engine v%s\n */\n", ENLNGS_VERSION);
    fprintf(fout, "const _enlNoopEl = { style: {}, classList: { add(){}, remove(){}, toggle(){}, contains(){ return false; } }, addEventListener(){}, removeEventListener(){}, setAttribute(){}, getAttribute(){ return null; }, appendChild(){}, remove(){}, scrollIntoView(){}, focus(){}, blur(){}, value: \"\", textContent: \"\", innerHTML: \"\" };\n");
    fprintf(fout, "function _enlEl(s) {\n");
    fprintf(fout, "  if (typeof document === \"undefined\") return _enlNoopEl;\n");
    fprintf(fout, "  let el = document.getElementById(s);\n");
    fprintf(fout, "  if (!el) { try { el = document.querySelector(s); } catch (_) {} }\n");
    fprintf(fout, "  if (!el) { console.warn(\"[enlngs] Element not found:\", s); return _enlNoopEl; }\n");
    fprintf(fout, "  return el;\n");
    fprintf(fout, "}\n\n");

    int indent_stack[128];
    int type_stack[128];
    int data_stack[128];
    int stack_top = 0;
    indent_stack[0] = 0;
    type_stack[0] = BLOCK_BRACE;
    data_stack[0] = 0;

    static char raw_line[65536];
    static char line_buf[65536];
    int line_num = 0;

    while (fgets(raw_line, sizeof(raw_line), fin)) {
        line_num++;

        int cur_indent = 0;
        char* p = raw_line;
        while (*p == ' ' || *p == '\t') {
            cur_indent += (*p == '\t') ? 4 : 1;
            p++;
        }

        strncpy(line_buf, p, sizeof(line_buf) - 1);
        line_buf[sizeof(line_buf) - 1] = '\0';
        char* s = trim(line_buf);

        if (!*s || s[0] == '#' || strncmp(s, "//", 2) == 0) continue;

        // Strip domain type headers and block wrappers
        if (str_starts_with_ci(s, "type enlngs") || str_starts_with_ci(s, "type enlgs") ||
            str_starts_with_ci(s, "in script:") || str_equals_ci(s, "in script")) {
            continue;
        }

        // Single-line inline JS if statement check: e.g. "if (window.__NEXORA_INITIALIZED__) return;" or "if (e) e.stopPropagation();"
        if (str_starts_with_ci(s, "if (") && s[strlen(s) - 1] != '{' && s[strlen(s) - 1] != ':') {
            if (s[strlen(s) - 1] == ';') {
                fprintf(fout, "%s\n", s);
            } else {
                fprintf(fout, "%s;\n", s);
            }
            continue;
        }

        bool is_else_line = str_starts_with_ci(s, "else:") || str_equals_ci(s, "else") ||
                            str_starts_with_ci(s, "elif ") || str_starts_with_ci(s, "else if ") ||
                            str_starts_with_ci(s, "catch") || str_starts_with_ci(s, "except");

        if (is_else_line) {
            // Close any deeper nested blocks inside the if block
            while (stack_top > 0 && indent_stack[stack_top] > cur_indent + 4) {
                emit_close_block(fout, type_stack[stack_top], data_stack[stack_top]);
                stack_top--;
            }
            if (str_starts_with_ci(s, "elif ") || str_starts_with_ci(s, "else if ")) {
                char* cond = s + (str_starts_with_ci(s, "elif ") ? 5 : 8);
                char* col = strchr(cond, ':');
                if (col) *col = '\0';
                char tcond[512];
                transform_js_expr(trim(cond), tcond, sizeof(tcond));
                fprintf(fout, "} else if (%s) {\n", tcond);
                if (stack_top == 0 || indent_stack[stack_top] < cur_indent + 4) {
                    indent_stack[++stack_top] = cur_indent + 4;
                }
                type_stack[stack_top] = BLOCK_IF;
                continue;
            } else if (str_starts_with_ci(s, "catch") || str_starts_with_ci(s, "except")) {
                char* col = strchr(s, ':');
                if (col) *col = '\0';
                char* p = s + (str_starts_with_ci(s, "catch") ? 5 : 6);
                p = trim(p);
                if (*p) {
                    fprintf(fout, "} catch (%s) {\n", p);
                } else {
                    fprintf(fout, "} catch (err) {\n");
                }
                if (stack_top == 0 || indent_stack[stack_top] < cur_indent + 4) {
                    indent_stack[++stack_top] = cur_indent + 4;
                }
                type_stack[stack_top] = BLOCK_BRACE;
                continue;
            } else {
                fprintf(fout, "} else {\n");
                if (stack_top == 0 || indent_stack[stack_top] < cur_indent + 4) {
                    indent_stack[++stack_top] = cur_indent + 4;
                }
                type_stack[stack_top] = BLOCK_IF;
                continue;
            }
        }

        // Try block
        if (str_equals_ci(s, "try:") || str_equals_ci(s, "try")) {
            fprintf(fout, "try {\n");
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_BRACE;
            data_stack[stack_top] = 0;
            continue;
        }

        // Closing bracket in source code: already provides the close!
        if (s[0] == '}' || strcmp(s, "])") == 0 || strcmp(s, "]);") == 0) {
            // Dedent down to current indent without printing extra closing braces
            while (stack_top > 0 && cur_indent + 4 < indent_stack[stack_top]) {
                stack_top--;
            }
            if (stack_top > 0) stack_top--;
            fprintf(fout, "%s\n", s);
            continue;
        }

        // Standard dedent handling
        while (stack_top > 0 && cur_indent < indent_stack[stack_top]) {
            emit_close_block(fout, type_stack[stack_top], data_stack[stack_top]);
            stack_top--;
        }

        // 1. Explicit Block End
        if (str_equals_ci(s, "end") || str_starts_with_ci(s, "end repeat") ||
            str_starts_with_ci(s, "end while") || str_starts_with_ci(s, "end if") ||
            str_starts_with_ci(s, "end script") || str_starts_with_ci(s, "end for") ||
            str_starts_with_ci(s, "end to do")) {
            if (stack_top > 0) {
                emit_close_block(fout, type_stack[stack_top], data_stack[stack_top]);
                stack_top--;
            }
            continue;
        }

        // 2. Event Listeners: on click / when clicked / on submit / when submitted / moved / resized / input
        if (str_starts_with_ci(s, "when page loads") || str_starts_with_ci(s, "when window loads") || str_starts_with_ci(s, "when document loads")) {
            fprintf(fout, "window.addEventListener(\"DOMContentLoaded\", () => {\n");
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_EVENT;
            data_stack[stack_top] = 0;
            continue;
        }

        if (str_starts_with_ci(s, "when window presses") || str_starts_with_ci(s, "when \"window\" presses")) {
            fprintf(fout, "window.addEventListener(\"keydown\", (event) => {\n");
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_EVENT;
            data_stack[stack_top] = 0;
            continue;
        }

        if (str_starts_with_ci(s, "when window releases") || str_starts_with_ci(s, "when \"window\" releases")) {
            fprintf(fout, "window.addEventListener(\"keyup\", (event) => {\n");
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_EVENT;
            data_stack[stack_top] = 0;
            continue;
        }

        if (str_starts_with_ci(s, "when ") && strstr(s, " is moved")) {
            char target_id[128];
            extract_target_id(s + 5, target_id, sizeof(target_id));
            if (strcmp(target_id, "window") == 0) {
                fprintf(fout, "window.addEventListener(\"mousemove\", (event) => {\n");
            } else {
                fprintf(fout, "_enlEl(\"%s\").addEventListener(\"mousemove\", (event) => {\n", target_id);
            }
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_EVENT;
            data_stack[stack_top] = 0;
            continue;
        }

        if (str_starts_with_ci(s, "when ") && strstr(s, " is resized")) {
            char target_id[128];
            extract_target_id(s + 5, target_id, sizeof(target_id));
            if (strcmp(target_id, "window") == 0) {
                fprintf(fout, "window.addEventListener(\"resize\", (event) => {\n");
            } else {
                fprintf(fout, "_enlEl(\"%s\").addEventListener(\"resize\", (event) => {\n", target_id);
            }
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_EVENT;
            data_stack[stack_top] = 0;
            continue;
        }

        if (str_starts_with_ci(s, "on click of ") || (str_starts_with_ci(s, "when ") && strstr(s, " is clicked"))) {
            char target_id[128];
            extract_target_id(s, target_id, sizeof(target_id));
            if (!target_id[0]) {
                fprintf(stderr, "SyntaxError: Line %d: Missing event target in '%s'\n", line_num, s);
                fclose(fin); fclose(fout); remove(outpath); return 1;
            }
            if (strcmp(target_id, "body") == 0) {
                fprintf(fout, "document.body.addEventListener(\"click\", (event) => {\n");
            } else if (strcmp(target_id, "window") == 0) {
                fprintf(fout, "window.addEventListener(\"click\", (event) => {\n");
            } else {
                fprintf(fout, "_enlEl(\"%s\").addEventListener(\"click\", (event) => {\n", target_id);
            }
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_EVENT;
            data_stack[stack_top] = 0;
            continue;
        }

        if (str_starts_with_ci(s, "on submit of ") || (str_starts_with_ci(s, "when ") && strstr(s, " is submitted"))) {
            char target_id[128];
            extract_target_id(s, target_id, sizeof(target_id));
            fprintf(fout, "_enlEl(\"%s\").addEventListener(\"submit\", (event) => {\n", target_id);
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_EVENT;
            data_stack[stack_top] = 0;
            continue;
        }

        if (str_starts_with_ci(s, "on input of ") || (str_starts_with_ci(s, "when ") && strstr(s, " is input"))) {
            char target_id[128];
            extract_target_id(s, target_id, sizeof(target_id));
            fprintf(fout, "_enlEl(\"%s\").addEventListener(\"input\", (event) => {\n", target_id);
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_EVENT;
            data_stack[stack_top] = 0;
            continue;
        }

        // 3. Timers: after <N> seconds: / every <N> seconds:
        if (str_starts_with_ci(s, "after ")) {
            char* dur_str = s + 6;
            char* col = strchr(dur_str, ':');
            if (col) *col = '\0';
            dur_str = trim(dur_str);
            int ms = 1000;
            if (strstr(dur_str, "second") || strstr(dur_str, "sec") || strstr(dur_str, "s")) {
                int sec = atoi(dur_str);
                ms = (sec > 0 ? sec : 1) * 1000;
            } else {
                ms = atoi(dur_str);
                if (ms <= 0) ms = 1000;
            }
            fprintf(fout, "setTimeout(() => {\n");
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_TIMEOUT;
            data_stack[stack_top] = ms;
            continue;
        }

        if (str_starts_with_ci(s, "every ")) {
            char* dur_str = s + 6;
            char* col = strchr(dur_str, ':');
            if (col) *col = '\0';
            dur_str = trim(dur_str);
            int ms = 1000;
            if (strstr(dur_str, "second") || strstr(dur_str, "sec") || strstr(dur_str, "s")) {
                int sec = atoi(dur_str);
                ms = (sec > 0 ? sec : 1) * 1000;
            } else {
                ms = atoi(dur_str);
                if (ms <= 0) ms = 1000;
            }
            fprintf(fout, "setInterval(() => {\n");
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_INTERVAL;
            data_stack[stack_top] = ms;
            continue;
        }

        // 4. Async Fetch: fetch data from "<url>":
        if (str_starts_with_ci(s, "fetch data from ") || str_starts_with_ci(s, "fetch from ")) {
            char url[256];
            extract_target_id(s, url, sizeof(url));
            fprintf(fout, "fetch(\"%s\").then(res => res.json()).then(data => {\n", url);
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_FETCH;
            data_stack[stack_top] = 0;
            continue;
        }

        // 5. Function Definitions: function / to do / async function / async to do
        if (str_starts_with_ci(s, "async function ") || str_starts_with_ci(s, "async to do ")) {
            char* fn_start = s + (str_starts_with_ci(s, "async function ") ? 15 : 12);
            char* col = strchr(fn_start, ':');
            if (col) *col = '\0';
            char* with_pos = strstr(fn_start, " with ");
            if (with_pos) {
                *with_pos = '\0';
                char* fname = trim(fn_start);
                char* fargs = trim(with_pos + 6);
                fprintf(fout, "async function %s(%s) {\n", fname, fargs);
            } else {
                char* fname = trim(fn_start);
                fprintf(fout, "async function %s() {\n", fname);
            }
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_BRACE;
            data_stack[stack_top] = 0;
            continue;
        }

        if (str_starts_with_ci(s, "function ") && (strchr(s, '(') != NULL || strchr(s, '{') != NULL)) {
            if (s[strlen(s) - 1] != '{') {
                fprintf(fout, "%s {\n", s);
            } else {
                fprintf(fout, "%s\n", s);
            }
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_BRACE;
            data_stack[stack_top] = 0;
            continue;
        }

        if (str_starts_with_ci(s, "function ") || str_starts_with_ci(s, "to do ")) {
            char* fn_start = s + (str_starts_with_ci(s, "function ") ? 9 : 6);
            char* col = strchr(fn_start, ':');
            if (col) *col = '\0';
            char* with_pos = strstr(fn_start, " with ");
            if (with_pos) {
                *with_pos = '\0';
                char* fname = trim(fn_start);
                char* fargs = trim(with_pos + 6);
                fprintf(fout, "function %s(%s) {\n", fname, fargs);
            } else {
                char* fname = trim(fn_start);
                fprintf(fout, "function %s() {\n", fname);
            }
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_BRACE;
            data_stack[stack_top] = 0;
            continue;
        }

        // 6. Loops: repeat / while / for
        if (str_starts_with_ci(s, "repeat ")) {
            char* p_times = s + 7;
            char* col = strchr(p_times, ':');
            if (col) *col = '\0';
            char* times_pos = strstr(p_times, " times");
            if (times_pos) *times_pos = '\0';
            fprintf(fout, "for (let _i%d = 0; _i%d < %s; _i%d++) {\n", stack_top, stack_top, trim(p_times), stack_top);
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_BRACE;
            data_stack[stack_top] = 0;
            continue;
        }

        if (str_starts_with_ci(s, "while ")) {
            char* cond = s + 6;
            char* col = strchr(cond, ':');
            if (col) *col = '\0';
            char tcond[512];
            transform_js_expr(trim(cond), tcond, sizeof(tcond));
            fprintf(fout, "while (%s) {\n", tcond);
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_BRACE;
            data_stack[stack_top] = 0;
            continue;
        }

        if (str_starts_with_ci(s, "for ") && strstr(s, " in ")) {
            char* p_in = strstr(s + 4, " in ");
            *p_in = '\0';
            char* var_name = trim(s + 4);
            if (str_starts_with_ci(var_name, "each ")) var_name = trim(var_name + 5);
            char* expr = trim(p_in + 4);
            char* col = strchr(expr, ':');
            if (col) *col = '\0';
            fprintf(fout, "for (const %s of %s) {\n", var_name, expr);
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_BRACE;
            data_stack[stack_top] = 0;
            continue;
        }

        // 7. Conditionals: if <cond>: or raw JS if (...) {
        if (str_starts_with_ci(s, "if ")) {
            if (s[strlen(s) - 1] == '{') {
                fprintf(fout, "%s\n", s);
                indent_stack[++stack_top] = cur_indent + 4;
                type_stack[stack_top] = BLOCK_BRACE;
                data_stack[stack_top] = 0;
                continue;
            }
            char* cond = s + 3;
            char* col = strchr(cond, ':');
            if (col) *col = '\0';
            char tcond[4096];
            transform_js_expr(trim(cond), tcond, sizeof(tcond));
            fprintf(fout, "if (%s) {\n", tcond);
            indent_stack[++stack_top] = cur_indent + 4;
            type_stack[stack_top] = BLOCK_IF;
            data_stack[stack_top] = 0;
            continue;
        }

        // 8. Variable Declarations: create / define / declare / let / const
        if (str_starts_with_ci(s, "create ") || str_starts_with_ci(s, "let ") ||
            str_starts_with_ci(s, "define ") || str_starts_with_ci(s, "const ") ||
            str_starts_with_ci(s, "declare ")) {
            bool is_const = str_starts_with_ci(s, "define ") || str_starts_with_ci(s, "const ");
            char* vp = s;
            while (*vp && !isspace((unsigned char)*vp)) vp++;
            while (isspace((unsigned char)*vp)) vp++;

            if (str_starts_with_ci(vp, "a ") && !str_starts_with_ci(vp + 2, "as ") && !str_starts_with_ci(vp + 2, "of ") && vp[2] != '=') vp += 2;
            else if (str_starts_with_ci(vp, "an ") && !str_starts_with_ci(vp + 3, "as ") && !str_starts_with_ci(vp + 3, "of ") && vp[3] != '=') vp += 3;
            while (isspace((unsigned char)*vp)) vp++;

            char* as_pos = strstr(vp, " as ");
            if (!as_pos) as_pos = strstr(vp, " of ");
            if (!as_pos) as_pos = strstr(vp, " = ");
            if (!as_pos) as_pos = strchr(vp, '=');

            if (as_pos) {
                int nlen = (int)(as_pos - vp);
                char vname[64] = {0};
                if (nlen >= 63) nlen = 63;
                strncpy(vname, vp, nlen);
                vname[nlen] = '\0';
                trim(vname);

                char* val_str = as_pos;
                if (strncmp(val_str, " as ", 4) == 0) val_str += 4;
                else if (strncmp(val_str, " of ", 4) == 0) val_str += 4;
                else if (strncmp(val_str, " = ", 3) == 0) val_str += 3;
                else if (*val_str == '=') val_str += 1;

                val_str = trim(val_str);

                if (str_starts_with_ci(val_str, "get value of ")) {
                    char target_id[128];
                    extract_target_id(val_str, target_id, sizeof(target_id));
                    fprintf(fout, "%s %s = _enlEl(\"%s\").value;\n",
                            is_const ? "const" : "let", vname, target_id);
                } else if (str_starts_with_ci(val_str, "get element ")) {
                    char target_id[128];
                    extract_target_id(val_str + 12, target_id, sizeof(target_id));
                    fprintf(fout, "%s %s = _enlEl(\"%s\");\n",
                            is_const ? "const" : "let", vname, target_id);
                } else {
                    char tval[16384];
                    transform_js_expr(val_str, tval, sizeof(tval));
                    fprintf(fout, "%s %s = %s;\n", is_const ? "const" : "let", vname, tval);
                }
            } else {
                fprintf(fout, "%s %s;\n", is_const ? "const" : "let", trim(vp));
            }
            continue;
        }

        // 9. In-place arithmetic: increase / decrease
        if (str_starts_with_ci(s, "increase ")) {
            char* p_by = strstr(s + 9, " by ");
            if (p_by) {
                *p_by = '\0';
                char* vname = trim(s + 9);
                char* delta = trim(p_by + 4);
                char tval[256];
                transform_js_expr(delta, tval, sizeof(tval));
                char* of_pos = strstr(vname, " of ");
                if (of_pos) {
                    *of_pos = '\0';
                    char* prop = trim(vname);
                    char* target = trim(of_pos + 4);
                    char ttarget[256];
                    transform_js_expr(target, ttarget, sizeof(ttarget));
                    fprintf(fout, "(%s).%s += %s;\n", ttarget, prop, tval);
                } else {
                    fprintf(fout, "%s += %s;\n", vname, tval);
                }
                continue;
            }
        }

        if (str_starts_with_ci(s, "decrease ")) {
            char* p_by = strstr(s + 9, " by ");
            if (p_by) {
                *p_by = '\0';
                char* vname = trim(s + 9);
                char* delta = trim(p_by + 4);
                char tval[256];
                transform_js_expr(delta, tval, sizeof(tval));
                char* of_pos = strstr(vname, " of ");
                if (of_pos) {
                    *of_pos = '\0';
                    char* prop = trim(vname);
                    char* target = trim(of_pos + 4);
                    char ttarget[256];
                    transform_js_expr(target, ttarget, sizeof(ttarget));
                    fprintf(fout, "(%s).%s -= %s;\n", ttarget, prop, tval);
                } else {
                    fprintf(fout, "%s -= %s;\n", vname, tval);
                }
                continue;
            }
        }

        // 9b. Put item into target: put <item> into <target>
        if (str_starts_with_ci(s, "put ")) {
            char* p_into = find_last_token(s + 4, " into ");
            if (p_into) {
                *p_into = '\0';
                char* item = trim(s + 4);
                char* target = trim(p_into + 6);
                char ttarget[512], titem[4096];
                transform_js_expr(target, ttarget, sizeof(ttarget));
                transform_js_expr(item, titem, sizeof(titem));
                fprintf(fout, "%s = %s;\n", ttarget, titem);
                continue;
            }
        }

        // 9b. Assign property of object to value: assign <prop> of <target> to <val>
        if (str_starts_with_ci(s, "assign ")) {
            char* p_of = strstr(s + 7, " of ");
            char* p_to = p_of ? strstr(p_of + 4, " to ") : NULL;
            if (p_of && p_to) {
                *p_of = '\0';
                *p_to = '\0';
                char* prop = trim(s + 7);
                char* target = trim(p_of + 4);
                char* val = trim(p_to + 4);
                char ttarget[512], tval[512];
                transform_js_expr(target, ttarget, sizeof(ttarget));
                transform_js_expr(val, tval, sizeof(tval));
                fprintf(fout, "(%s).%s = %s;\n", ttarget, prop, tval);
                continue;
            }
        }

        // 9c. Add item to collection: add <item> to <collection> (when not "add class")
        if (str_starts_with_ci(s, "add ") && !str_starts_with_ci(s, "add class ")) {
            char* to_pos = find_last_token(s + 4, " to ");
            if (to_pos) {
                *to_pos = '\0';
                char* item = trim(s + 4);
                char* coll = trim(to_pos + 4);
                char tcoll[256], titem[4096];
                transform_js_expr(coll, tcoll, sizeof(tcoll));
                transform_js_expr(item, titem, sizeof(titem));
                fprintf(fout, "(%s).push(%s);\n", tcoll, titem);
                continue;
            }
        }

        // 10. DOM Property updates
        if (str_starts_with_ci(s, "set ") && strstr(s, " of ") && strstr(s, " to ")) {
            char* p_of = strstr(s + 4, " of ");
            char* p_to = strstr(p_of + 4, " to ");
            if (p_of && p_to) {
                *p_of = '\0';
                *p_to = '\0';
                char* prop = trim(s + 4);
                if (str_starts_with_ci(prop, "style ")) prop = trim(prop + 6);
                char target_id[128];
                extract_target_id(p_of + 4, target_id, sizeof(target_id));
                char* val_str = trim(p_to + 4);
                char tval[16384];
                transform_js_expr(val_str, tval, sizeof(tval));

                if (strcmp(prop, "text") == 0) {
                    fprintf(fout, "_enlEl(\"%s\").textContent = %s;\n", target_id, tval);
                } else if (strcmp(prop, "value") == 0) {
                    fprintf(fout, "_enlEl(\"%s\").value = %s;\n", target_id, tval);
                } else if (strcmp(prop, "html") == 0) {
                    fprintf(fout, "_enlEl(\"%s\").innerHTML = %s;\n", target_id, tval);
                } else if (strcmp(prop, "color") == 0) {
                    fprintf(fout, "_enlEl(\"%s\").style.color = %s;\n", target_id, tval);
                } else if (strcmp(prop, "background") == 0) {
                    fprintf(fout, "_enlEl(\"%s\").style.background = %s;\n", target_id, tval);
                } else {
                    fprintf(fout, "_enlEl(\"%s\").style.%s = %s;\n", target_id, prop, tval);
                }
                continue;
            }
        }

        if (str_starts_with_ci(s, "toggle class ")) {
            char cls[64], target_id[128];
            extract_target_id(s + 13, cls, sizeof(cls));
            char* on_pos = strstr(s + 13, " on ");
            if (!on_pos) on_pos = strstr(s + 13, " in ");
            if (on_pos) {
                extract_target_id(on_pos + 4, target_id, sizeof(target_id));
                if (strcmp(target_id, "body") == 0) {
                    fprintf(fout, "document.body.classList.toggle(\"%s\");\n", cls);
                } else {
                    fprintf(fout, "_enlEl(\"%s\").classList.toggle(\"%s\");\n", target_id, cls);
                }
                continue;
            }
        }

        if (str_starts_with_ci(s, "set ")) {
            char* eq_pos = strstr(s + 4, " = ");
            if (!eq_pos) eq_pos = strchr(s + 4, '=');
            char* to_pos = strstr(s + 4, " to ");
            char* as_pos = strstr(s + 4, " as ");
            char* split_pos = eq_pos ? eq_pos : (to_pos ? to_pos : as_pos);
            int skip = eq_pos ? (*eq_pos == ' ' ? 3 : 1) : 4;

            if (split_pos) {
                int nlen = (int)(split_pos - (s + 4));
                char vname[128] = {0};
                if (nlen >= 127) nlen = 127;
                strncpy(vname, s + 4, nlen);
                vname[nlen] = '\0';
                trim(vname);
                char* val_str = trim(split_pos + skip);
                char tval[16384];
                transform_js_expr(val_str, tval, sizeof(tval));
                fprintf(fout, "%s = %s;\n", vname, tval);
                continue;
            }
        }

        if (str_starts_with_ci(s, "update ") && strstr(s, " with text ")) {
            char* p_with = strstr(s, " with text ");
            *p_with = '\0';
            char target_id[128];
            extract_target_id(s + 7, target_id, sizeof(target_id));
            char* txt_val = trim(p_with + 11);
            char tval[16384];
            transform_js_expr(txt_val, tval, sizeof(tval));
            fprintf(fout, "_enlEl(\"%s\").textContent = %s;\n", target_id, tval);
            continue;
        }

        if (str_starts_with_ci(s, "refresh ") && strstr(s, " with ")) {
            char* p_with = strstr(s, " with ");
            *p_with = '\0';
            char target_id[128];
            extract_target_id(s + 8, target_id, sizeof(target_id));
            char* txt_val = trim(p_with + 6);
            char tval[16384];
            transform_js_expr(txt_val, tval, sizeof(tval));
            fprintf(fout, "_enlEl(\"%s\").textContent = %s;\n", target_id, tval);
            continue;
        }

        // 11. DOM Class / Visibility
        if (str_starts_with_ci(s, "add class ")) {
            char cls[64], target_id[128];
            extract_target_id(s + 10, cls, sizeof(cls));
            char* to_pos = strstr(s + 10, " to ");
            if (to_pos) {
                extract_target_id(to_pos + 4, target_id, sizeof(target_id));
                if (strcmp(target_id, "body") == 0) {
                    fprintf(fout, "document.body.classList.add(\"%s\");\n", cls);
                } else {
                    fprintf(fout, "_enlEl(\"%s\").classList.add(\"%s\");\n", target_id, cls);
                }
                continue;
            }
        }

        if (str_starts_with_ci(s, "remove class ")) {
            char cls[64], target_id[128];
            extract_target_id(s + 13, cls, sizeof(cls));
            char* to_pos = strstr(s + 13, " to ");
            if (!to_pos) to_pos = strstr(s + 13, " from ");
            if (to_pos) {
                extract_target_id(to_pos + (strncmp(to_pos, " to ", 4) == 0 ? 4 : 6), target_id, sizeof(target_id));
                if (strcmp(target_id, "body") == 0) {
                    fprintf(fout, "document.body.classList.remove(\"%s\");\n", cls);
                } else {
                    fprintf(fout, "_enlEl(\"%s\").classList.remove(\"%s\");\n", target_id, cls);
                }
                continue;
            }
        }

        if (str_starts_with_ci(s, "hide element ")) {
            char target_id[128];
            extract_target_id(s + 13, target_id, sizeof(target_id));
            fprintf(fout, "_enlEl(\"%s\").style.display = 'none';\n", target_id);
            continue;
        }

        if (str_starts_with_ci(s, "show element ")) {
            char target_id[128];
            extract_target_id(s + 13, target_id, sizeof(target_id));
            fprintf(fout, "_enlEl(\"%s\").style.display = 'block';\n", target_id);
            continue;
        }

        if (str_equals_ci(s, "prevent default")) {
            fprintf(fout, "if (typeof event !== 'undefined' && event.preventDefault) event.preventDefault();\n");
            continue;
        }

        // 12. Notifications, Alerts, Scroll & Copy
        if (str_starts_with_ci(s, "trigger toast ")) {
            char tval[16384];
            transform_js_expr(trim(s + 14), tval, sizeof(tval));
            fprintf(fout, "console.log(\"[Toast] \" + (%s));\n", tval);
            continue;
        }

        if (str_starts_with_ci(s, "trigger modal ")) {
            char tval[16384];
            transform_js_expr(trim(s + 14), tval, sizeof(tval));
            fprintf(fout, "alert(%s);\n", tval);
            continue;
        }

        if (str_starts_with_ci(s, "trigger scroll to ") || str_starts_with_ci(s, "scroll to ")) {
            char target_id[128];
            extract_target_id(s + (str_starts_with_ci(s, "scroll to ") ? 10 : 18), target_id, sizeof(target_id));
            fprintf(fout, "_enlEl(\"%s\").scrollIntoView({ behavior: 'smooth' });\n", target_id);
            continue;
        }

        if (str_starts_with_ci(s, "copy ")) {
            char tval[16384];
            transform_js_expr(trim(s + 5), tval, sizeof(tval));
            fprintf(fout, "if (typeof navigator !== 'undefined' && navigator.clipboard) navigator.clipboard.writeText(%s);\n", tval);
            continue;
        }

        if (str_starts_with_ci(s, "alert ")) {
            char tval[16384];
            transform_js_expr(trim(s + 6), tval, sizeof(tval));
            fprintf(fout, "alert(%s);\n", tval);
            continue;
        }

        if (str_starts_with_ci(s, "show ") || str_starts_with_ci(s, "display ") || str_starts_with_ci(s, "log ")) {
            int skip_len = str_starts_with_ci(s, "display ") ? 8 : (str_starts_with_ci(s, "show ") ? 5 : 4);
            char* expr = trim(s + skip_len);
            char tval[16384];
            transform_js_expr(expr, tval, sizeof(tval));
            fprintf(fout, "console.log(%s);\n", tval);
            continue;
        }

        if (str_starts_with_ci(s, "store ") && strstr(s, " as ")) {
            char key[128];
            extract_target_id(s + 6, key, sizeof(key));
            char* as_pos = strstr(s + 6, " as ");
            char* val_str = trim(as_pos + 4);
            fprintf(fout, "localStorage.setItem(\"%s\", %s);\n", key, val_str);
            continue;
        }

        // 13. Calls and Returns
        if (str_starts_with_ci(s, "call ")) {
            char* call_body = s + 5;
            char* with_pos = strstr(call_body, " with ");
            if (with_pos) {
                *with_pos = '\0';
                char* fname = trim(call_body);
                char* fargs = trim(with_pos + 6);
                fprintf(fout, "%s(%s);\n", fname, fargs);
            } else {
                fprintf(fout, "%s();\n", trim(call_body));
            }
            continue;
        }

        if (str_starts_with_ci(s, "return")) {
            char* ret_val = trim(s + 6);
            if (*ret_val) {
                char tval[16384];
                transform_js_expr(ret_val, tval, sizeof(tval));
                fprintf(fout, "return %s;\n", tval);
            } else {
                fprintf(fout, "return;\n");
            }
            continue;
        }

        // 14. Direct JavaScript Passthrough Statements
        // Matches valid JS: variable declarations, standard globals, assignment expressions, method invocations
        bool has_assignment = (strchr(s, '=') != NULL && !strstr(s, "==") && !strstr(s, "<=") && !strstr(s, ">=") && !strstr(s, "!="));
        bool has_invocation = (strchr(s, '(') != NULL && strchr(s, ')') != NULL);
        bool is_js_ident = isalpha((unsigned char)s[0]) || s[0] == '_' || s[0] == '$';
        bool is_bracket_line = (s[0] == '{' || s[0] == '}' || s[0] == '[' || s[0] == ']' || s[strlen(s)-1] == ',' || s[0] == '.');

        if (str_starts_with_ci(s, "const ") || str_starts_with_ci(s, "let ") ||
            str_starts_with_ci(s, "var ") || str_starts_with_ci(s, "function ") ||
            str_starts_with_ci(s, "console.") || str_starts_with_ci(s, "window.") ||
            str_starts_with_ci(s, "document.") || str_starts_with_ci(s, "Math.") ||
            str_starts_with_ci(s, "localStorage.") || str_starts_with_ci(s, "sessionStorage.") ||
            str_starts_with_ci(s, "requestAnimationFrame(") || str_starts_with_ci(s, "if (") ||
            str_starts_with_ci(s, "for (") || str_starts_with_ci(s, "while (") ||
            (is_js_ident && (has_assignment || has_invocation)) ||
            is_bracket_line ||
            s[strlen(s) - 1] == ';' || s[strlen(s) - 1] == '}' || s[strlen(s) - 1] == '{') {
            
            char term = s[strlen(s) - 1];
            if (term == ';' || term == '{' || term == '}' || term == ',' || term == ':') {
                fprintf(fout, "%s\n", s);
            } else {
                fprintf(fout, "%s;\n", s);
            }
            continue;
        }

        // 15. UNRECOGNIZED LINE -> ERROR! Do NOT emit to JS
        fprintf(stderr, "SyntaxError: Line %d: Unrecognized statement '%s'\n", line_num, s);
        fclose(fin);
        fclose(fout);
        remove(outpath);
        return 1;
    }

    // Close any unclosed blocks
    while (stack_top > 0) {
        emit_close_block(fout, type_stack[stack_top], data_stack[stack_top]);
        stack_top--;
    }

    fclose(fin);
    fclose(fout);
    printf("[enlngs] Sovereign Reactive Script compiled successfully: '%s'\n", outpath);
    return 0;
}
