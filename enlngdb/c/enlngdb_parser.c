#include "enlngdb.h"
#include <ctype.h>

static char* trim(char* s) {
    while (isspace((unsigned char)*s) || *s == ';') s++;
    if (*s == 0) return s;
    char* end = s + strlen(s) - 1;
    while (end > s && (isspace((unsigned char)*end) || *end == ';')) end--;
    end[1] = '\0';
    return s;
}

static bool str_starts_with_ci(const char* str, const char* prefix) {
    while (*prefix) {
        if (tolower((unsigned char)*str) != tolower((unsigned char)*prefix)) return false;
        str++;
        prefix++;
    }
    return true;
}

static int str_ci_cmp(const char* s1, const char* s2) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;
    while (*s1 && *s2) {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0) return diff;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

static EnlngVal parse_val_token(const char* token) {
    char clean[256];
    strncpy(clean, token, sizeof(clean) - 1);
    clean[sizeof(clean) - 1] = '\0';
    char* t = trim(clean);

    // Strips enclosing quotes
    size_t len = strlen(t);
    if (len >= 2 && ((t[0] == '"' && t[len - 1] == '"') || (t[0] == '\'' && t[len - 1] == '\''))) {
        t[len - 1] = '\0';
        return enlng_str(t + 1);
    }

    if (strcmp(t, "true") == 0 || strcmp(t, "True") == 0) return enlng_bool(true);
    if (strcmp(t, "false") == 0 || strcmp(t, "False") == 0) return enlng_bool(false);
    if (strcmp(t, "null") == 0 || strcmp(t, "Null") == 0) return enlng_null();

    // Check if integer or double
    char* endptr = NULL;
    long long iv = strtoll(t, &endptr, 10);
    if (endptr && *endptr == '\0' && strlen(t) > 0) {
        return enlng_int(iv);
    }

    double dv = strtod(t, &endptr);
    if (endptr && *endptr == '\0' && strlen(t) > 0) {
        return enlng_double(dv);
    }

    return enlng_str(t);
}

static EnlngOp parse_op_phrase(const char* s, int* op_len) {
    struct {
        const char* phrase;
        EnlngOp op;
    } ops[] = {
        {"is greater than or equal to", OP_GTE},
        {"greater than or equal to",    OP_GTE},
        {"is at least",                 OP_GTE},
        {">=",                          OP_GTE},
        {"is less than or equal to",    OP_LTE},
        {"less than or equal to",       OP_LTE},
        {"is at most",                  OP_LTE},
        {"<=",                          OP_LTE},
        {"is greater than",             OP_GT},
        {"greater than",                OP_GT},
        {">",                           OP_GT},
        {"is less than",                OP_LT},
        {"less than",                   OP_LT},
        {"<",                           OP_LT},
        {"is not equal to",             OP_NEQ},
        {"not equal to",                OP_NEQ},
        {"is not",                      OP_NEQ},
        {"!=",                          OP_NEQ},
        {"is equal to",                 OP_EQ},
        {"equal to",                    OP_EQ},
        {"equals",                      OP_EQ},
        {"==",                          OP_EQ},
        {"=",                           OP_EQ},
        {"like",                        OP_LIKE},
        {"is",                          OP_EQ},
        {NULL,                          OP_NONE}
    };

    for (int i = 0; ops[i].phrase != NULL; i++) {
        size_t plen = strlen(ops[i].phrase);
        if (str_starts_with_ci(s, ops[i].phrase)) {
            // Check word boundary
            char next = s[plen];
            if (next == '\0' || isspace((unsigned char)next) || strchr("=<>!\"'", next)) {
                if (op_len) *op_len = (int)plen;
                return ops[i].op;
            }
        }
    }
    if (op_len) *op_len = 0;
    return OP_NONE;
}

bool enlngdb_execute_statement(EnlngDatabase* db, const char* statement, bool print_output) {
    if (!db || !statement) return false;
    char line[1024];
    strncpy(line, statement, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';

    char* stmt = trim(line);
    // Remove trailing semicolon
    size_t slen = strlen(stmt);
    while (slen > 0 && stmt[slen - 1] == ';') {
        stmt[--slen] = '\0';
    }
    stmt = trim(stmt);
    if (strlen(stmt) == 0) return true;

    // 0. type enlngdb / type enlgdb
    if (str_starts_with_ci(stmt, "type enlngdb") || str_starts_with_ci(stmt, "type enlgdb")) {
        return true;
    }

    // display / print
    if (str_starts_with_ci(stmt, "display ") || str_starts_with_ci(stmt, "print ")) {
        const char* p = stmt + (str_starts_with_ci(stmt, "display ") ? 8 : 6);
        while (isspace((unsigned char)*p)) p++;
        char clean_msg[1024];
        strncpy(clean_msg, p, sizeof(clean_msg) - 1);
        clean_msg[sizeof(clean_msg) - 1] = '\0';
        char* msg = trim(clean_msg);
        size_t mlen = strlen(msg);
        if (mlen >= 2 && ((msg[0] == '"' && msg[mlen - 1] == '"') || (msg[0] == '\'' && msg[mlen - 1] == '\''))) {
            msg[mlen - 1] = '\0';
            msg++;
        }
        if (print_output) {
            printf("%s\n", msg);
        }
        return true;
    }

    // hint
    if (str_starts_with_ci(stmt, "hint ") || str_starts_with_ci(stmt, "hint:")) {
        return true;
    }

    // save database to "<filename>"
    if (str_starts_with_ci(stmt, "save database to ") || str_starts_with_ci(stmt, "save database ")) {
        const char* p = stmt + (str_starts_with_ci(stmt, "save database to ") ? 17 : 14);
        while (isspace((unsigned char)*p)) p++;
        char path_buf[256];
        strncpy(path_buf, p, sizeof(path_buf) - 1);
        path_buf[sizeof(path_buf) - 1] = '\0';
        char* path = trim(path_buf);
        size_t plen = strlen(path);
        if (plen >= 2 && ((path[0] == '"' && path[plen - 1] == '"') || (path[0] == '\'' && path[plen - 1] == '\''))) {
            path[plen - 1] = '\0';
            path++;
        }
        bool ok = enlngdb_save(db, path);
        if (ok && print_output) {
            printf("Database saved to '%s'. (0.02 ms)\n\n", path);
        }
        return ok;
    }

    // open database to "<filename>" / load database from "<filename>"
    if (str_starts_with_ci(stmt, "open database to ") || str_starts_with_ci(stmt, "open database ") || str_starts_with_ci(stmt, "load database from ")) {
        const char* p = stmt;
        if (str_starts_with_ci(p, "open database to ")) p += 17;
        else if (str_starts_with_ci(p, "open database ")) p += 14;
        else if (str_starts_with_ci(p, "load database from ")) p += 19;
        while (isspace((unsigned char)*p)) p++;
        char path_buf[256];
        strncpy(path_buf, p, sizeof(path_buf) - 1);
        path_buf[sizeof(path_buf) - 1] = '\0';
        char* path = trim(path_buf);
        size_t plen = strlen(path);
        if (plen >= 2 && ((path[0] == '"' && path[plen - 1] == '"') || (path[0] == '\'' && path[plen - 1] == '\''))) {
            path[plen - 1] = '\0';
            path++;
        }
        bool ok = enlngdb_load(db, path);
        if (ok && print_output) {
            printf("Database loaded from '%s'. (0.02 ms)\n\n", path);
        }
        return ok;
    }

    // 1. show tables
    if (str_starts_with_ci(stmt, "show tables")) {
        if (print_output) enlngdb_print_tables(db);
        return true;
    }

    // 2. show databases
    if (str_starts_with_ci(stmt, "show databases")) {
        if (print_output) {
            printf("+---------------------------+\n");
            printf("| Database                  |\n");
            printf("+---------------------------+\n");
            printf("| %-25s |\n", db->name);
            printf("+---------------------------+\n");
            printf("1 row(s) in set (0.01 ms)\n\n");
        }
        return true;
    }

    // 3. use database <name> / use <name>
    if (str_starts_with_ci(stmt, "use database ")) {
        char dbname[64];
        if (sscanf(stmt + 13, "%63[^; \t\r\n]", dbname) == 1) {
            char* sc = strchr(dbname, ';');
            if (sc) *sc = '\0';
            trim(dbname);
            strncpy(db->name, dbname, sizeof(db->name) - 1);
            char edb_file[256];
            snprintf(edb_file, sizeof(edb_file), "%s.edb", dbname);
            FILE* testf = fopen(edb_file, "rb");
            if (!testf) {
                snprintf(edb_file, sizeof(edb_file), "tests/%s.edb", dbname);
                testf = fopen(edb_file, "rb");
            }
            if (testf) {
                fclose(testf);
                enlngdb_load(db, edb_file);
            }
            if (print_output) {
                printf("Database changed to '%s'. (0.01 ms)\n\n", db->name);
            }
            return true;
        }
    } else if (str_starts_with_ci(stmt, "use ")) {
        char dbname[64];
        if (sscanf(stmt + 4, "%63[^; \t\r\n]", dbname) == 1) {
            char* sc = strchr(dbname, ';');
            if (sc) *sc = '\0';
            trim(dbname);
            strncpy(db->name, dbname, sizeof(db->name) - 1);
            char edb_file[256];
            snprintf(edb_file, sizeof(edb_file), "%s.edb", dbname);
            FILE* testf = fopen(edb_file, "rb");
            if (!testf) {
                snprintf(edb_file, sizeof(edb_file), "tests/%s.edb", dbname);
                testf = fopen(edb_file, "rb");
            }
            if (testf) {
                fclose(testf);
                enlngdb_load(db, edb_file);
            }
            if (print_output) {
                printf("Database changed to '%s'. (0.01 ms)\n\n", db->name);
            }
            return true;
        }
    }

    // 4. create table [if not exists] <name> with <col1>, <col2>...
    if (str_starts_with_ci(stmt, "create table ")) {
        const char* p = stmt + 13;
        while (isspace((unsigned char)*p)) p++;
        bool if_not_exists = false;
        if (str_starts_with_ci(p, "if not exists ")) {
            p += 14;
            if_not_exists = true;
        } else if (str_starts_with_ci(p, "if not exist ")) {
            p += 13;
            if_not_exists = true;
        }
        while (isspace((unsigned char)*p)) p++;

        char tbl_name[64];
        const char* with_pos = strstr(p, " with ");
        if (!with_pos) with_pos = strstr(p, " WITH ");

        if (with_pos) {
            int nlen = (int)(with_pos - p);
            if (nlen >= (int)sizeof(tbl_name)) nlen = (int)sizeof(tbl_name) - 1;
            strncpy(tbl_name, p, nlen);
            tbl_name[nlen] = '\0';
            trim(tbl_name);

            if (if_not_exists && enlngdb_get_table(db, tbl_name) != NULL) {
                if (print_output) {
                    printf("Query OK, 0 rows affected. Table '%s' already exists. (0.01 ms)\n\n", tbl_name);
                }
                return true;
            }

            char* cols_str = (char*)(with_pos + 6);
            EnlngColumn cols[ENLNGDB_MAX_COLS];
            int col_count = 0;

            char cols_copy[512];
            strncpy(cols_copy, cols_str, sizeof(cols_copy) - 1);
            cols_copy[sizeof(cols_copy) - 1] = '\0';

            char* cp = cols_copy;
            while (*cp && col_count < ENLNGDB_MAX_COLS) {
                while (isspace((unsigned char)*cp)) cp++;
                if (!*cp) break;
                char* item_start = cp;
                while (*cp && *cp != ',') cp++;
                if (*cp == ',') {
                    *cp = '\0';
                    cp++;
                }
                char* col_item = trim(item_start);
                char cname[64];
                sscanf(col_item, "%63s", cname);

                memset(&cols[col_count], 0, sizeof(EnlngColumn));
                strncpy(cols[col_count].name, cname, ENLNGDB_MAX_NAME - 1);
                cols[col_count].type = ENLNG_VAL_STRING;
                if (strstr(col_item, "primary key") != NULL ||
                    str_ci_cmp(cname, "id") == 0 ||
                    (strlen(cname) > 3 && str_ci_cmp(cname + strlen(cname) - 3, "_id") == 0)) {
                    cols[col_count].is_primary_key = true;
                }
                col_count++;
            }

            EnlngTable* tbl = enlngdb_create_table(db, tbl_name, cols, col_count);
            if (tbl && print_output) {
                printf("Query OK, 0 rows affected. Table '%s' created with %d columns. (0.05 ms)\n\n", tbl_name, col_count);
            }
            return tbl != NULL;
        }
    }

    // 5. insert into <name> with / insert record into <name> with
    if (str_starts_with_ci(stmt, "insert into ") || str_starts_with_ci(stmt, "insert record into ")) {
        const char* start = str_starts_with_ci(stmt, "insert record into ") ? (stmt + 19) : (stmt + 12);
        char tbl_name[64];
        const char* with_pos = strstr(start, " with ");
        if (!with_pos) with_pos = strstr(start, " WITH ");

        if (with_pos) {
            int nlen = (int)(with_pos - start);
            if (nlen >= (int)sizeof(tbl_name)) nlen = (int)sizeof(tbl_name) - 1;
            strncpy(tbl_name, start, nlen);
            tbl_name[nlen] = '\0';
            trim(tbl_name);

            EnlngTable* tbl = enlngdb_get_table(db, tbl_name);
            if (!tbl) {
                // Auto create table
                tbl = enlngdb_create_table(db, tbl_name, NULL, 0);
            }

            const char* pairs_str = with_pos + 6;
            char pairs_copy[512];
            strncpy(pairs_copy, pairs_str, sizeof(pairs_copy) - 1);
            pairs_copy[sizeof(pairs_copy) - 1] = '\0';

            EnlngVal cells[ENLNGDB_MAX_COLS];
            int cell_cnt = 0;

            // Split pairs by comma outside quotes
            char* p = pairs_copy;
            while (*p && cell_cnt < ENLNGDB_MAX_COLS) {
                while (isspace((unsigned char)*p)) p++;
                if (!*p) break;

                char* item_start = p;
                bool in_q = false;
                char q_char = 0;

                while (*p) {
                    if (!in_q && (*p == '"' || *p == '\'')) {
                        in_q = true;
                        q_char = *p;
                    } else if (in_q && *p == q_char) {
                        in_q = false;
                    } else if (!in_q && *p == ',') {
                        break;
                    }
                    p++;
                }

                char saved = *p;
                *p = '\0';

                char* pair = trim(item_start);
                char col_k[64];
                char* val_part = NULL;

                // Match key value: key val OR key: val OR key = val
                char* sep = strpbrk(pair, " :=");
                if (sep) {
                    int klen = (int)(sep - pair);
                    if (klen >= (int)sizeof(col_k)) klen = (int)sizeof(col_k) - 1;
                    strncpy(col_k, pair, klen);
                    col_k[klen] = '\0';
                    while (*sep == ' ' || *sep == ':' || *sep == '=') sep++;
                    val_part = sep;

                    // If column does not exist on table, add it
                    int col_idx = -1;
                    for (int c = 0; c < tbl->col_count; c++) {
                        if (strcmp(tbl->columns[c].name, col_k) == 0) {
                            col_idx = c;
                            break;
                        }
                    }
                    if (col_idx < 0 && tbl->col_count < ENLNGDB_MAX_COLS) {
                        col_idx = tbl->col_count++;
                        strncpy(tbl->columns[col_idx].name, col_k, ENLNGDB_MAX_NAME - 1);
                    }

                    cells[cell_cnt++] = parse_val_token(val_part);
                }

                if (saved == ',') p++;
            }

            bool ok = enlngdb_insert_row(tbl, cells, cell_cnt);
            for (int i = 0; i < cell_cnt; i++) enlng_free_val(&cells[i]);

            if (ok && print_output) {
                printf("Query OK, 1 row affected (0.04 ms)\n\n");
            }
            return ok;
        }
    }

    // 6. find [all] records from / find records from <name> [where ...] [order by ...]
    if (str_starts_with_ci(stmt, "find ") || str_starts_with_ci(stmt, "show all records from ") || str_starts_with_ci(stmt, "select ")) {
        char tbl_name[64] = {0};
        char order_col[64] = {0};
        bool is_desc = false;

        const char* from_pos = strstr(stmt, " from ");
        int offset = 6;
        if (!from_pos) {
            from_pos = strstr(stmt, " in ");
            offset = 4;
        }

        if (from_pos) {
            const char* after_from = from_pos + offset;
            while (isspace((unsigned char)*after_from)) after_from++;

            char rest[512];
            strncpy(rest, after_from, sizeof(rest) - 1);
            rest[sizeof(rest) - 1] = '\0';

            char* where_pos = strstr(rest, " where ");
            if (!where_pos) where_pos = strstr(rest, " WHERE ");
            char* order_pos = strstr(rest, " order by ");
            if (!order_pos) order_pos = strstr(rest, " ORDER BY ");
            if (!order_pos) order_pos = strstr(rest, " sorted by ");
            if (!order_pos) order_pos = strstr(rest, " SORTED BY ");

            if (order_pos) {
                const char* ord_arg = order_pos + (strstr(order_pos, " sorted by ") ? 11 : 10);
                while (isspace((unsigned char)*ord_arg)) ord_arg++;
                sscanf(ord_arg, "%63s", order_col);
                if (strstr(ord_arg, "desc") != NULL || strstr(ord_arg, "DESC") != NULL || strstr(ord_arg, "highest") != NULL) {
                    is_desc = true;
                }
                if (!where_pos || order_pos > where_pos) {
                    *order_pos = '\0';
                }
            }

            typedef struct {
                char col[64];
                EnlngOp op;
                EnlngVal val;
            } FilterCond;
            FilterCond conds[4];
            int cond_count = 0;

            if (where_pos) {
                *where_pos = '\0';
                strncpy(tbl_name, trim(rest), sizeof(tbl_name) - 1);
                char* cond_str = trim(where_pos + 7);

                // Split conditions by " and " / " AND "
                char* pcond = cond_str;
                while (*pcond && cond_count < 4) {
                    char* and_pos = strstr(pcond, " and ");
                    if (!and_pos) and_pos = strstr(pcond, " AND ");
                    if (and_pos) {
                        *and_pos = '\0';
                    }
                    char* single_cond = trim(pcond);
                    char cname[64];
                    if (sscanf(single_cond, "%63s", cname) == 1) {
                        char* dot = strchr(cname, '.');
                        if (dot) strncpy(conds[cond_count].col, dot + 1, sizeof(conds[cond_count].col) - 1);
                        else strncpy(conds[cond_count].col, cname, sizeof(conds[cond_count].col) - 1);

                        char* op_start = single_cond + strlen(cname);
                        while (isspace((unsigned char)*op_start)) op_start++;

                        if (str_starts_with_ci(op_start, "of ")) {
                            op_start += 3;
                            while (isspace((unsigned char)*op_start)) op_start++;
                            if (str_starts_with_ci(op_start, "the ")) {
                                op_start += 4;
                                while (isspace((unsigned char)*op_start)) op_start++;
                            }
                            while (*op_start && !isspace((unsigned char)*op_start)) op_start++;
                            while (isspace((unsigned char)*op_start)) op_start++;
                        }

                        int op_len = 0;
                        conds[cond_count].op = parse_op_phrase(op_start, &op_len);
                        if (conds[cond_count].op != OP_NONE) {
                            char* val_str = trim(op_start + op_len);
                            conds[cond_count].val = parse_val_token(val_str);
                            cond_count++;
                        }
                    }

                    if (and_pos) {
                        pcond = and_pos + 5;
                    } else {
                        break;
                    }
                }
            } else {
                strncpy(tbl_name, trim(rest), sizeof(tbl_name) - 1);
            }

            EnlngTable* tbl = enlngdb_get_table(db, tbl_name);
            if (!tbl) {
                if (print_output) {
                    printf("ERROR: Table '%s' does not exist in database '%s'.\n\n", tbl_name, db->name);
                }
                for (int i = 0; i < cond_count; i++) enlng_free_val(&conds[i].val);
                return false;
            }

            EnlngQueryResult* res = NULL;
            if (cond_count <= 1) {
                res = enlngdb_find(tbl, cond_count == 1 ? conds[0].col : NULL, cond_count == 1 ? conds[0].op : OP_NONE, cond_count == 1 ? &conds[0].val : NULL, 0, 0);
            } else {
                res = (EnlngQueryResult*)calloc(1, sizeof(EnlngQueryResult));
                res->col_count = tbl->col_count;
                for (int c = 0; c < tbl->col_count; c++) res->columns[c] = tbl->columns[c];
                size_t cap = 16;
                res->rows = (EnlngRow*)malloc(cap * sizeof(EnlngRow));
                res->row_count = 0;

                int col_indices[4];
                for (int i = 0; i < cond_count; i++) {
                    col_indices[i] = -1;
                    for (int c = 0; c < tbl->col_count; c++) {
                        if (strcmp(tbl->columns[c].name, conds[i].col) == 0) {
                            col_indices[i] = c;
                            break;
                        }
                    }
                }

                for (size_t r = 0; r < tbl->row_count; r++) {
                    EnlngRow* row = &tbl->rows[r];
                    bool all_match = true;
                    for (int i = 0; i < cond_count; i++) {
                        if (col_indices[i] >= 0 && col_indices[i] < row->cell_count) {
                            if (!enlng_eval_condition(&row->cells[col_indices[i]], conds[i].op, &conds[i].val)) {
                                all_match = false;
                                break;
                            }
                        } else {
                            all_match = false;
                            break;
                        }
                    }

                    if (all_match) {
                        if (res->row_count >= cap) {
                            cap *= 2;
                            res->rows = (EnlngRow*)realloc(res->rows, cap * sizeof(EnlngRow));
                        }
                        EnlngRow* dest_row = &res->rows[res->row_count++];
                        dest_row->cell_count = row->cell_count;
                        dest_row->cells = (EnlngVal*)malloc(row->cell_count * sizeof(EnlngVal));
                        for (int c = 0; c < row->cell_count; c++) {
                            dest_row->cells[c] = enlng_copy_val(&row->cells[c]);
                        }
                    }
                }
            }

            for (int i = 0; i < cond_count; i++) enlng_free_val(&conds[i].val);

            if (res) {
                if (order_col[0] != '\0') {
                    int ord_idx = -1;
                    for (int c = 0; c < res->col_count; c++) {
                        if (str_ci_cmp(res->columns[c].name, order_col) == 0) {
                            ord_idx = c;
                            break;
                        }
                    }
                    if (ord_idx >= 0 && res->row_count > 1) {
                        for (size_t i = 0; i < res->row_count - 1; i++) {
                            for (size_t j = i + 1; j < res->row_count; j++) {
                                int cmp = enlng_compare_vals(&res->rows[i].cells[ord_idx], &res->rows[j].cells[ord_idx]);
                                if ((is_desc && cmp < 0) || (!is_desc && cmp > 0)) {
                                    EnlngRow tmp = res->rows[i];
                                    res->rows[i] = res->rows[j];
                                    res->rows[j] = tmp;
                                }
                            }
                        }
                    }
                }

                if (print_output) enlngdb_print_result(res);
                enlngdb_free_result(res);
                return true;
            }
        }
    }

    // 7. count records from / count records in <name> [where ...]
    if (str_starts_with_ci(stmt, "count ") || str_starts_with_ci(stmt, "count records from ") || str_starts_with_ci(stmt, "count records in ") || str_starts_with_ci(stmt, "count all from ") || str_starts_with_ci(stmt, "count all in ")) {
        const char* from_pos = strstr(stmt, " from ");
        int offset = 6;
        if (!from_pos) {
            from_pos = strstr(stmt, " in ");
            offset = 4;
        }
        if (from_pos) {
            const char* p = from_pos + offset;
            while (isspace((unsigned char)*p)) p++;

            char rest[512];
            strncpy(rest, p, sizeof(rest) - 1);
            rest[sizeof(rest) - 1] = '\0';

            char tbl_name[64] = {0};
            char filter_col[64] = {0};
            EnlngOp op = OP_NONE;
            EnlngVal target_val = enlng_null();

            char* where_pos = strstr(rest, " where ");
            if (!where_pos) where_pos = strstr(rest, " WHERE ");

            if (where_pos) {
                *where_pos = '\0';
                strncpy(tbl_name, trim(rest), sizeof(tbl_name) - 1);
                char* cond = trim(where_pos + 7);
                char cname[64];
                if (sscanf(cond, "%63s", cname) == 1) {
                    char* dot = strchr(cname, '.');
                    if (dot) strncpy(filter_col, dot + 1, sizeof(filter_col) - 1);
                    else strncpy(filter_col, cname, sizeof(filter_col) - 1);

                    char* op_start = cond + strlen(cname);
                    while (isspace((unsigned char)*op_start)) op_start++;
                    int op_len = 0;
                    op = parse_op_phrase(op_start, &op_len);
                    if (op != OP_NONE) {
                        char* val_str = trim(op_start + op_len);
                        target_val = parse_val_token(val_str);
                    }
                }
            } else {
                strncpy(tbl_name, trim(rest), sizeof(tbl_name) - 1);
            }

            EnlngTable* tbl = enlngdb_get_table(db, tbl_name);
            if (tbl) {
                int c = enlngdb_count(tbl, filter_col[0] ? filter_col : NULL, op, &target_val);
                enlng_free_val(&target_val);
                if (print_output) {
                    printf("+----------+\n");
                    printf("| count(*) |\n");
                    printf("+----------+\n");
                    printf("| %-8d |\n", c);
                    printf("+----------+\n");
                    printf("1 row in set (0.01 ms)\n\n");
                }
                return true;
            } else {
                if (print_output) printf("ERROR: Table '%s' does not exist in database '%s'.\n\n", tbl_name, db->name);
                return false;
            }
        }
    }

    // 8. delete / remove records from <table> [where ...]
    if (str_starts_with_ci(stmt, "delete records from ") || str_starts_with_ci(stmt, "delete from ") || str_starts_with_ci(stmt, "delete all from ") || str_starts_with_ci(stmt, "delete all records from ") ||
        str_starts_with_ci(stmt, "remove records from ") || str_starts_with_ci(stmt, "remove from ") || str_starts_with_ci(stmt, "remove all from ") || str_starts_with_ci(stmt, "remove all records from ")) {
        const char* p = stmt;
        if (str_starts_with_ci(p, "delete all records from ")) p += 24;
        else if (str_starts_with_ci(p, "delete all from ")) p += 16;
        else if (str_starts_with_ci(p, "delete records from ")) p += 20;
        else if (str_starts_with_ci(p, "delete from ")) p += 12;
        else if (str_starts_with_ci(p, "remove all records from ")) p += 24;
        else if (str_starts_with_ci(p, "remove all from ")) p += 16;
        else if (str_starts_with_ci(p, "remove records from ")) p += 20;
        else if (str_starts_with_ci(p, "remove from ")) p += 12;
        while (isspace((unsigned char)*p)) p++;

        char rest[512];
        strncpy(rest, p, sizeof(rest) - 1);
        rest[sizeof(rest) - 1] = '\0';

        bool confirmed = (strstr(rest, " confirmed") != NULL || strstr(rest, " CONFIRMED") != NULL);
        char* conf_pos = strstr(rest, " confirmed");
        if (!conf_pos) conf_pos = strstr(rest, " CONFIRMED");
        if (conf_pos) *conf_pos = '\0';

        char tbl_name[64] = {0};
        char filter_col[64] = {0};
        EnlngOp op = OP_NONE;
        EnlngVal target_val = enlng_null();

        char* where_pos = strstr(rest, " where ");
        if (!where_pos) where_pos = strstr(rest, " WHERE ");

        if (where_pos) {
            *where_pos = '\0';
            strncpy(tbl_name, trim(rest), sizeof(tbl_name) - 1);
            char* cond = trim(where_pos + 7);
            char cname[64];
            if (sscanf(cond, "%63s", cname) == 1) {
                char* dot = strchr(cname, '.');
                if (dot) strncpy(filter_col, dot + 1, sizeof(filter_col) - 1);
                else strncpy(filter_col, cname, sizeof(filter_col) - 1);

                char* op_start = cond + strlen(cname);
                while (isspace((unsigned char)*op_start)) op_start++;
                int op_len = 0;
                op = parse_op_phrase(op_start, &op_len);
                if (op != OP_NONE) {
                    char* val_str = trim(op_start + op_len);
                    target_val = parse_val_token(val_str);
                }
            }
        } else {
            strncpy(tbl_name, trim(rest), sizeof(tbl_name) - 1);
        }

        EnlngTable* tbl = enlngdb_get_table(db, tbl_name);
        if (!tbl) {
            if (print_output) printf("ERROR: Table '%s' does not exist in database '%s'.\n\n", tbl_name, db->name);
            return false;
        }

        if (!filter_col[0] && !confirmed) {
            enlng_free_val(&target_val);
            if (print_output) {
                printf("REFUSED: 'delete from %s' with no 'where' would remove every row.\n", tbl_name);
                printf("         Nothing was deleted. To go ahead, add the word 'confirmed':\n");
                printf("         delete all from %s confirmed;\n\n", tbl_name);
            }
            return false;
        }

        int del_cnt = enlngdb_delete(tbl, filter_col[0] ? filter_col : NULL, op, &target_val, filter_col[0] ? true : confirmed);
        enlng_free_val(&target_val);
        if (print_output) {
            printf("Query OK, %d row(s) deleted. (0.02 ms)\n\n", del_cnt);
        }
        return true;
    }

    // 8. drop table <name> confirmed
    if (str_starts_with_ci(stmt, "drop table ")) {
        char tbl_name[64];
        bool confirmed = (strstr(stmt, " confirmed") != NULL || strstr(stmt, " CONFIRMED") != NULL);
        if (sscanf(stmt + 11, "%63s", tbl_name) == 1) {
            if (!confirmed && enlngdb_get_table(db, tbl_name)) {
                if (print_output) {
                    printf("REFUSED: dropping table '%s' deletes the table and all its rows.\n", tbl_name);
                    printf("         Nothing was changed. To go ahead, add the word 'confirmed':\n");
                    printf("         drop table %s confirmed;\n\n", tbl_name);
                }
                return false;
            }
            bool ok = enlngdb_drop_table(db, tbl_name, confirmed);
            if (ok && print_output) {
                printf("Query OK, table '%s' dropped successfully. (0.02 ms)\n\n", tbl_name);
            }
            return ok;
        }
    }

    // 9. update / change / in <table_name> update/change/set
    bool is_in_stmt = str_starts_with_ci(stmt, "in ");
    bool is_upd_stmt = str_starts_with_ci(stmt, "update ") || str_starts_with_ci(stmt, "change ") || str_starts_with_ci(stmt, "modify ");

    if (is_in_stmt || is_upd_stmt) {
        char tbl_name[64] = {0};
        const char* after_action = NULL;

        if (is_in_stmt) {
            const char* p = stmt + 3;
            while (isspace((unsigned char)*p)) p++;
            if (str_starts_with_ci(p, "table ")) p += 6;
            if (str_starts_with_ci(p, "the ")) p += 4;
            while (isspace((unsigned char)*p)) p++;

            char tname[64] = {0};
            if (sscanf(p, "%63s", tname) == 1) {
                strncpy(tbl_name, tname, sizeof(tbl_name) - 1);
                const char* post_tbl = p + strlen(tname);
                while (isspace((unsigned char)*post_tbl)) post_tbl++;

                if (str_starts_with_ci(post_tbl, "update ")) {
                    post_tbl += 7;
                } else if (str_starts_with_ci(post_tbl, "change ")) {
                    post_tbl += 7;
                } else if (str_starts_with_ci(post_tbl, "set ")) {
                    post_tbl += 4;
                } else if (str_starts_with_ci(post_tbl, "modify ")) {
                    post_tbl += 7;
                } else {
                    // Not an update statement starting with 'in'
                    return true;
                }

                while (isspace((unsigned char)*post_tbl)) post_tbl++;
                if (str_starts_with_ci(post_tbl, "set ")) {
                    post_tbl += 4;
                    while (isspace((unsigned char)*post_tbl)) post_tbl++;
                }
                after_action = post_tbl;
            }
        } else {
            const char* p = stmt;
            if (str_starts_with_ci(p, "update ")) p += 7;
            else if (str_starts_with_ci(p, "change ")) p += 7;
            else if (str_starts_with_ci(p, "modify ")) p += 7;
            while (isspace((unsigned char)*p)) p++;

            if (str_starts_with_ci(p, "records in ")) p += 11;
            else if (str_starts_with_ci(p, "rows in ")) p += 8;
            else if (str_starts_with_ci(p, "in ")) p += 3;
            if (str_starts_with_ci(p, "the ")) p += 4;
            if (str_starts_with_ci(p, "table ")) p += 6;
            while (isspace((unsigned char)*p)) p++;

            const char* set_pos = strstr(p, " set ");
            if (!set_pos) set_pos = strstr(p, " SET ");

            if (set_pos) {
                int tlen = (int)(set_pos - p);
                if (tlen >= (int)sizeof(tbl_name)) tlen = (int)sizeof(tbl_name) - 1;
                strncpy(tbl_name, p, tlen);
                tbl_name[tlen] = '\0';
                trim(tbl_name);

                after_action = set_pos + 5;
                while (isspace((unsigned char)*after_action)) after_action++;
            } else {
                char tname[64] = {0};
                if (sscanf(p, "%63s", tname) == 1) {
                    strncpy(tbl_name, tname, sizeof(tbl_name) - 1);
                    const char* post_tbl = p + strlen(tname);
                    while (isspace((unsigned char)*post_tbl)) post_tbl++;
                    if (str_starts_with_ci(post_tbl, "set ")) {
                        post_tbl += 4;
                        while (isspace((unsigned char)*post_tbl)) post_tbl++;
                    }
                    after_action = post_tbl;
                }
            }
        }

        if (tbl_name[0] && after_action && *after_action) {
            EnlngTable* tbl = enlngdb_get_table(db, tbl_name);
            if (!tbl) {
                if (print_output) printf("ERROR: Table '%s' does not exist in database '%s'.\n\n", tbl_name, db->name);
                return false;
            }

            char rest[512];
            strncpy(rest, after_action, sizeof(rest) - 1);
            rest[sizeof(rest) - 1] = '\0';

            char filter_col[64] = {0};
            EnlngOp op = OP_NONE;
            EnlngVal target_val = enlng_null();

            char* where_pos = strstr(rest, " where ");
            if (!where_pos) where_pos = strstr(rest, " WHERE ");

            if (where_pos) {
                *where_pos = '\0';
                char* cond = trim(where_pos + 7);
                char cname[64];
                if (sscanf(cond, "%63s", cname) == 1) {
                    strncpy(filter_col, cname, sizeof(filter_col) - 1);
                    char* op_start = cond + strlen(cname);
                    while (isspace((unsigned char)*op_start)) op_start++;
                    int op_len = 0;
                    op = parse_op_phrase(op_start, &op_len);
                    if (op != OP_NONE) {
                        char* val_str = trim(op_start + op_len);
                        target_val = parse_val_token(val_str);
                    }
                }
            }

            int total_updated = 0;
            char* aptr = rest;
            while (*aptr) {
                while (isspace((unsigned char)*aptr) || *aptr == ',') aptr++;
                if (!*aptr) break;

                char* assign_end = strchr(aptr, ',');
                if (assign_end) *assign_end = '\0';

                char* sep = strstr(aptr, " = ");
                if (!sep) sep = strstr(aptr, " to ");
                if (!sep) sep = strstr(aptr, " is ");
                if (!sep) sep = strstr(aptr, " : ");
                if (!sep) sep = strchr(aptr, '=');

                if (sep) {
                    char col_name[64] = {0};
                    int clen = (int)(sep - aptr);
                    if (clen >= (int)sizeof(col_name)) clen = (int)sizeof(col_name) - 1;
                    strncpy(col_name, aptr, clen);
                    col_name[clen] = '\0';
                    trim(col_name);

                    if (*sep == '=') sep++;
                    else if (strncmp(sep, " = ", 3) == 0) sep += 3;
                    else if (strncmp(sep, " to ", 4) == 0) sep += 4;
                    else if (strncmp(sep, " is ", 4) == 0) sep += 4;
                    else if (strncmp(sep, " : ", 3) == 0) sep += 3;

                    char* val_str = trim(sep);
                    EnlngVal set_val = parse_val_token(val_str);

                    int count = enlngdb_update(tbl, filter_col[0] ? filter_col : NULL, op, &target_val, col_name, &set_val);
                    enlng_free_val(&set_val);
                    if (count > total_updated) total_updated = count;
                }

                if (assign_end) aptr = assign_end + 1;
                else break;
            }

            enlng_free_val(&target_val);

            if (print_output) {
                printf("Query OK, %d row(s) updated. (0.03 ms)\n\n", total_updated);
            }
            return true;
        }
    }

    if (print_output) {
        fprintf(stderr, "\n-- EnlangDB SyntaxError ------------------------------------------------\n");
        fprintf(stderr, "Location: <statement>:1:1\n\n");
        fprintf(stderr, "    1 | %s\n", stmt);
        fprintf(stderr, "      | ^\n\n");
        if (str_starts_with_ci(stmt, "create table")) {
            fprintf(stderr, "What: Incomplete 'create table' statement.\n");
            fprintf(stderr, "Why:  EnlngDB tables require column definitions after the 'with' keyword.\n\n");
            fprintf(stderr, "Suggestions:\n");
            fprintf(stderr, "  * Specify table columns, e.g.: create table users with id, name, email\n");
        } else if (str_starts_with_ci(stmt, "find") || str_starts_with_ci(stmt, "select")) {
            fprintf(stderr, "What: Malformed query statement.\n");
            fprintf(stderr, "Why:  EnlngDB queries require a target table specified via 'from <table>' or 'in <table>'.\n\n");
            fprintf(stderr, "Suggestions:\n");
            fprintf(stderr, "  * Add the source table, e.g.: find records from users\n");
            fprintf(stderr, "  * Or project columns: find user_id of users from users\n");
        } else if (str_starts_with_ci(stmt, "insert")) {
            fprintf(stderr, "What: Malformed 'insert' statement.\n");
            fprintf(stderr, "Why:  Records require target table and values specified via 'with' or 'values'.\n\n");
            fprintf(stderr, "Suggestions:\n");
            fprintf(stderr, "  * Insert key-value pairs: insert into users with id 1, name \"Aero\"\n");
        } else {
            fprintf(stderr, "What: Unrecognized or malformed database statement: '%s'\n", stmt);
            fprintf(stderr, "Why:  Statement does not match any valid conversational EnlngDB grammar.\n\n");
            fprintf(stderr, "Suggestions:\n");
            fprintf(stderr, "  * Supported commands: create table, insert into, find records from, update, delete\n");
            fprintf(stderr, "  * Run 'enlngdb --help' to view command reference.\n");
        }
        fprintf(stderr, "------------------------------------------------------------------------\n\n");
    }
    return false;
}

int enlngdb_execute_script(EnlngDatabase* db, const char* script_content, bool print_output) {
    if (!db || !script_content) return 0;
    char* copy = strdup(script_content);
    int executed = 0;
    int failed = 0;

    char* ptr = copy;
    while (*ptr) {
        while (*ptr && (isspace((unsigned char)*ptr) || *ptr == ';' || *ptr == '\n' || *ptr == '\r')) ptr++;
        if (!*ptr) break;

        // Skip comments starting with '#'
        if (*ptr == '#') {
            while (*ptr && *ptr != '\n' && *ptr != '\r') ptr++;
            continue;
        }

        char* stmt_start = ptr;
        bool in_q = false;
        char q_char = 0;

        while (*ptr) {
            if (!in_q && (*ptr == '"' || *ptr == '\'')) {
                in_q = true;
                q_char = *ptr;
            } else if (in_q && *ptr == q_char) {
                in_q = false;
            } else if (!in_q && (*ptr == ';' || *ptr == '\n' || *ptr == '\r')) {
                break;
            }
            ptr++;
        }

        if (*ptr) {
            *ptr = '\0';
            ptr++;
        }

        char* trimmed = trim(stmt_start);
        if (*trimmed && *trimmed != '#') {
            if (enlngdb_execute_statement(db, trimmed, print_output)) {
                executed++;
            } else {
                failed++;
            }
        }
    }
    free(copy);
    return failed > 0 ? -failed : executed;
}

int enlngdb_run_file(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "[ENLNGDB ERROR] Could not open file: %s\n", filepath);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer = (char*)malloc(sz + 1);
    if (!buffer) {
        fclose(f);
        return 1;
    }

    size_t actual = fread(buffer, 1, sz, f);
    buffer[actual] = '\0';
    fclose(f);

    // Derive database name from filename
    char dbname[64];
    const char* base = strrchr(filepath, '/');
    if (!base) base = strrchr(filepath, '\\');
    base = base ? base + 1 : filepath;
    strncpy(dbname, base, sizeof(dbname) - 1);
    char* dot = strrchr(dbname, '.');
    if (dot) *dot = '\0';

    EnlngDatabase* db = enlngdb_create(dbname);

    // Try auto-loading existing binary database file
    char edb_file[256];
    snprintf(edb_file, sizeof(edb_file), "%s.edb", dbname);
    enlngdb_load(db, edb_file);

    int count = enlngdb_execute_script(db, buffer, true);

    // Auto-save updated state to binary format only if no failures occurred
    if (count >= 0) {
        enlngdb_save(db, edb_file);
    }

    free(buffer);
    enlngdb_free(db);
    return count >= 0 ? 0 : 1;
}
