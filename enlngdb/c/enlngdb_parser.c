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

    // 4. create table <name> with <col1>, <col2>...
    if (str_starts_with_ci(stmt, "create table ")) {
        char tbl_name[64];
        char* with_pos = strstr(stmt, " with ");
        if (!with_pos) with_pos = strstr(stmt, " WITH ");

        if (with_pos) {
            int nlen = (int)(with_pos - (stmt + 13));
            if (nlen >= (int)sizeof(tbl_name)) nlen = (int)sizeof(tbl_name) - 1;
            strncpy(tbl_name, stmt + 13, nlen);
            tbl_name[nlen] = '\0';
            trim(tbl_name);

            char* cols_str = with_pos + 6;
            EnlngColumn cols[ENLNGDB_MAX_COLS];
            int col_count = 0;

            char cols_copy[512];
            strncpy(cols_copy, cols_str, sizeof(cols_copy) - 1);
            cols_copy[sizeof(cols_copy) - 1] = '\0';

            char* p = cols_copy;
            while (*p && col_count < ENLNGDB_MAX_COLS) {
                while (isspace((unsigned char)*p)) p++;
                if (!*p) break;
                char* item_start = p;
                while (*p && *p != ',') p++;
                if (*p == ',') {
                    *p = '\0';
                    p++;
                }
                char* col_item = trim(item_start);
                char cname[64];
                sscanf(col_item, "%63s", cname);

                memset(&cols[col_count], 0, sizeof(EnlngColumn));
                strncpy(cols[col_count].name, cname, ENLNGDB_MAX_NAME - 1);
                cols[col_count].type = ENLNG_VAL_STRING;
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

    // 6. find [all] records from / find records from <name> [where <col> <op> <val>]
    if (str_starts_with_ci(stmt, "find ") || str_starts_with_ci(stmt, "show all records from ")) {
        char tbl_name[64] = {0};
        char filter_col[64] = {0};
        EnlngOp op = OP_NONE;
        EnlngVal target_val = enlng_null();

        const char* from_pos = strstr(stmt, " from ");
        if (!from_pos) from_pos = strstr(stmt, " in ");

        if (from_pos) {
            const char* after_from = from_pos + (strstr(from_pos, " from ") ? 6 : 4);
            while (isspace((unsigned char)*after_from)) after_from++;

            char rest[512];
            strncpy(rest, after_from, sizeof(rest) - 1);
            rest[sizeof(rest) - 1] = '\0';

            char* where_pos = strstr(rest, " where ");
            if (!where_pos) where_pos = strstr(rest, " WHERE ");

            if (where_pos) {
                *where_pos = '\0';
                strncpy(tbl_name, trim(rest), sizeof(tbl_name) - 1);
                char* cond = trim(where_pos + 7);

                // Parse condition: col op value
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
            } else {
                strncpy(tbl_name, trim(rest), sizeof(tbl_name) - 1);
            }

            EnlngTable* tbl = enlngdb_get_table(db, tbl_name);
            if (!tbl) {
                if (print_output) {
                    printf("ERROR: Table '%s' does not exist in database '%s'.\n\n", tbl_name, db->name);
                }
                return false;
            }

            EnlngQueryResult* res = enlngdb_find(tbl, filter_col[0] ? filter_col : NULL, op, &target_val, 0, 0);
            enlng_free_val(&target_val);
            if (res) {
                if (print_output) enlngdb_print_result(res);
                enlngdb_free_result(res);
                return true;
            }
        }
    }

    // 7. count records from <name> [where ...]
    if (str_starts_with_ci(stmt, "count ") || str_starts_with_ci(stmt, "count records from ")) {
        const char* from_pos = strstr(stmt, " from ");
        if (!from_pos) from_pos = strstr(stmt, " in ");
        if (from_pos) {
            char tbl_name[64];
            sscanf(from_pos + 6, "%63s", tbl_name);
            EnlngTable* tbl = enlngdb_get_table(db, tbl_name);
            if (tbl) {
                int c = enlngdb_count(tbl, NULL, OP_NONE, NULL);
                if (print_output) {
                    printf("+----------+\n");
                    printf("| count(*) |\n");
                    printf("+----------+\n");
                    printf("| %-8d |\n", c);
                    printf("+----------+\n");
                    printf("1 row in set (0.01 ms)\n\n");
                }
                return true;
            }
        }
    }

    // 8. drop table <name> confirmed
    if (str_starts_with_ci(stmt, "drop table ")) {
        char tbl_name[64];
        bool confirmed = (strstr(stmt, " confirmed") != NULL || strstr(stmt, " CONFIRMED") != NULL);
        if (sscanf(stmt + 11, "%63s", tbl_name) == 1) {
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

    return true;
}

int enlngdb_execute_script(EnlngDatabase* db, const char* script_content, bool print_output) {
    if (!db || !script_content) return 0;
    char* copy = strdup(script_content);
    int executed = 0;

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
            }
        }
    }
    free(copy);
    return executed;
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

    // Auto-save updated state to binary format
    enlngdb_save(db, edb_file);

    free(buffer);
    enlngdb_free(db);
    return count > 0 ? 0 : 1;
}
