#include "enlngs.h"

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
            snprintf(buf, bufsize, "%lld", v.i_val);
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
            fprintf(stderr, "Error: Cannot reassign constant '%s'\n", name);
            return;
        }
        existing->val = val;
        return;
    }
    if (vm->var_count < MAX_VARS) {
        strncpy(vm->vars[vm->var_count].name, name, 63);
        vm->vars[vm->var_count].val = val;
        vm->vars[vm->var_count].is_const = is_const;
        vm->var_count++;
    }
}

EnlngsVal enlngs_eval_expr(EnlngsVM* vm, const char* raw_expr) {
    if (!raw_expr) return enlngs_val_null();
    char expr[512];
    strncpy(expr, raw_expr, sizeof(expr) - 1);
    expr[sizeof(expr) - 1] = '\0';
    char* s = trim(expr);

    if (strlen(s) == 0) return enlngs_val_null();

    // String literal
    if ((s[0] == '"' && s[strlen(s) - 1] == '"') || (s[0] == '\'' && s[strlen(s) - 1] == '\'')) {
        char unquoted[256] = {0};
        size_t len = strlen(s);
        if (len >= 2) {
            strncpy(unquoted, s + 1, len - 2);
            unquoted[len - 2] = '\0';
        }
        return enlngs_val_string(unquoted);
    }

    // Literals
    if (str_equals_ci(s, "true")) return enlngs_val_bool(true);
    if (str_equals_ci(s, "false")) return enlngs_val_bool(false);
    if (str_equals_ci(s, "null") || str_equals_ci(s, "none") || str_equals_ci(s, "undefined")) {
        return enlngs_val_null();
    }

    // String Concatenation or Binary Addition (+)
    char* plus = strstr(s, " + ");
    if (plus) {
        *plus = '\0';
        char* left = trim(s);
        char* right = trim(plus + 3);
        EnlngsVal v1 = enlngs_eval_expr(vm, left);
        EnlngsVal v2 = enlngs_eval_expr(vm, right);

        if (v1.type == VAL_STRING || v2.type == VAL_STRING) {
            char b1[256], b2[256], res[512];
            enlngs_val_to_str(v1, b1, sizeof(b1));
            enlngs_val_to_str(v2, b2, sizeof(b2));
            snprintf(res, sizeof(res), "%s%s", b1, b2);
            return enlngs_val_string(res);
        }
        if (v1.type == VAL_DOUBLE || v2.type == VAL_DOUBLE) {
            double d1 = (v1.type == VAL_DOUBLE) ? v1.d_val : (double)v1.i_val;
            double d2 = (v2.type == VAL_DOUBLE) ? v2.d_val : (double)v2.i_val;
            return enlngs_val_double(d1 + d2);
        }
        return enlngs_val_int(v1.i_val + v2.i_val);
    }

    // Comparisons
    const char* ops[] = {" >= ", " <= ", " == ", " != ", " > ", " < ", " is ", " equals "};
    for (int o = 0; o < 8; o++) {
        char* op_pos = strstr(s, ops[o]);
        if (op_pos) {
            *op_pos = '\0';
            char* left = trim(s);
            char* right = trim(op_pos + strlen(ops[o]));
            EnlngsVal v1 = enlngs_eval_expr(vm, left);
            EnlngsVal v2 = enlngs_eval_expr(vm, right);

            double d1 = (v1.type == VAL_DOUBLE) ? v1.d_val : (double)v1.i_val;
            double d2 = (v2.type == VAL_DOUBLE) ? v2.d_val : (double)v2.i_val;

            if (o == 0) return enlngs_val_bool(d1 >= d2);
            if (o == 1) return enlngs_val_bool(d1 <= d2);
            if (o == 2 || o == 6 || o == 7) {
                if (v1.type == VAL_STRING && v2.type == VAL_STRING) {
                    return enlngs_val_bool(strcmp(v1.s_val, v2.s_val) == 0);
                }
                return enlngs_val_bool(d1 == d2);
            }
            if (o == 3) {
                if (v1.type == VAL_STRING && v2.type == VAL_STRING) {
                    return enlngs_val_bool(strcmp(v1.s_val, v2.s_val) != 0);
                }
                return enlngs_val_bool(d1 != d2);
            }
            if (o == 4) return enlngs_val_bool(d1 > d2);
            if (o == 5) return enlngs_val_bool(d1 < d2);
        }
    }

    // Arithmetic: - * /
    char* minus = strstr(s, " - ");
    if (minus) {
        *minus = '\0';
        EnlngsVal v1 = enlngs_eval_expr(vm, trim(s));
        EnlngsVal v2 = enlngs_eval_expr(vm, trim(minus + 3));
        if (v1.type == VAL_DOUBLE || v2.type == VAL_DOUBLE) {
            double d1 = (v1.type == VAL_DOUBLE) ? v1.d_val : (double)v1.i_val;
            double d2 = (v2.type == VAL_DOUBLE) ? v2.d_val : (double)v2.i_val;
            return enlngs_val_double(d1 - d2);
        }
        return enlngs_val_int(v1.i_val - v2.i_val);
    }
    char* mul = strstr(s, " * ");
    if (mul) {
        *mul = '\0';
        EnlngsVal v1 = enlngs_eval_expr(vm, trim(s));
        EnlngsVal v2 = enlngs_eval_expr(vm, trim(mul + 3));
        return enlngs_val_int(v1.i_val * v2.i_val);
    }
    char* div = strstr(s, " / ");
    if (div) {
        *div = '\0';
        EnlngsVal v1 = enlngs_eval_expr(vm, trim(s));
        EnlngsVal v2 = enlngs_eval_expr(vm, trim(div + 3));
        if (v2.i_val == 0 && v2.d_val == 0.0) return enlngs_val_int(0);
        return enlngs_val_double((double)v1.i_val / (double)v2.i_val);
    }

    // Numeric check
    char* endptr = NULL;
    long long iv = strtoll(s, &endptr, 10);
    if (endptr && *endptr == '\0') return enlngs_val_int(iv);

    double dv = strtod(s, &endptr);
    if (endptr && *endptr == '\0') return enlngs_val_double(dv);

    // Variable lookup
    EnlngsVar* v = find_var(vm, s);
    if (v) return v->val;

    // Default string fallback
    return enlngs_val_string(s);
}

bool enlngs_exec_line(EnlngsVM* vm, const char* raw_line) {
    if (!raw_line) return true;
    char line[512];
    strncpy(line, raw_line, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
    char* s = trim(line);

    if (strlen(s) == 0 || s[0] == '#' || strncmp(s, "//", 2) == 0) return true;
    if (str_starts_with_ci(s, "type enlngs") || str_starts_with_ci(s, "type enlgs") || str_starts_with_ci(s, "in script:")) return true;

    // 1. Output: show / display / log / alert / print
    const char* out_keywords[] = {"show ", "display ", "log ", "print ", "alert "};
    for (int k = 0; k < 5; k++) {
        if (str_starts_with_ci(s, out_keywords[k])) {
            char* expr = trim(s + strlen(out_keywords[k]));
            EnlngsVal res = enlngs_eval_expr(vm, expr);
            enlngs_print_val(res);
            printf("\n");
            return true;
        }
    }

    // 2. Variable declarations: create / define / let / const
    if (str_starts_with_ci(s, "create ") || str_starts_with_ci(s, "let ") ||
        str_starts_with_ci(s, "define ") || str_starts_with_ci(s, "const ")) {
        bool is_const = str_starts_with_ci(s, "define ") || str_starts_with_ci(s, "const ");
        char* p = s;
        while (*p && !isspace((unsigned char)*p)) p++;
        while (isspace((unsigned char)*p)) p++;

        char varname[64] = {0};
        char* as_pos = strstr(p, " as ");
        if (!as_pos) as_pos = strstr(p, " = ");
        if (!as_pos) as_pos = strchr(p, '=');

        if (as_pos) {
            int nlen = (int)(as_pos - p);
            if (nlen >= 63) nlen = 63;
            strncpy(varname, p, nlen);
            varname[nlen] = '\0';
            trim(varname);

            char* val_str = as_pos;
            if (strncmp(val_str, " as ", 4) == 0) val_str += 4;
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
            // Collect loop body until end / dedent
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

        // Procedure: to do <name> with <params>:
        if (str_starts_with_ci(line, "to do ")) {
            if (vm->func_count < MAX_FUNCS) {
                EnlngsFunc* fn = &vm->funcs[vm->func_count++];
                char* name_start = line + 6;
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

    fprintf(fout, "/**\n * Compiled by Enlangg Sovereign Script Engine v%s\n */\n\n", ENLNGS_VERSION);

    char line[512];
    while (fgets(line, sizeof(line), fin)) {
        char* s = trim(line);
        if (!*s || s[0] == '#') continue;
        if (str_starts_with_ci(s, "create ") || str_starts_with_ci(s, "let ")) {
            char* as_pos = strstr(s, " as ");
            if (as_pos) {
                *as_pos = '\0';
                fprintf(fout, "let %s = %s;\n", trim(s + 7), trim(as_pos + 4));
            } else {
                fprintf(fout, "let %s;\n", trim(s + 7));
            }
        } else if (str_starts_with_ci(s, "define ") || str_starts_with_ci(s, "const ")) {
            char* as_pos = strstr(s, " as ");
            if (as_pos) {
                *as_pos = '\0';
                fprintf(fout, "const %s = %s;\n", trim(s + 7), trim(as_pos + 4));
            }
        } else if (str_starts_with_ci(s, "show ") || str_starts_with_ci(s, "display ")) {
            fprintf(fout, "console.log(%s);\n", trim(s + 5));
        } else if (str_starts_with_ci(s, "repeat ")) {
            char* times_pos = strstr(s, " times");
            if (times_pos) *times_pos = '\0';
            fprintf(fout, "for (let _i = 0; _i < %s; _i++) {\n", trim(s + 7));
        } else if (str_equals_ci(s, "end")) {
            fprintf(fout, "}\n");
        } else {
            fprintf(fout, "%s;\n", s);
        }
    }

    fclose(fin);
    fclose(fout);
    printf("[enlngs] Clean JavaScript bundle generated: '%s'\n", outpath);
    return 0;
}
