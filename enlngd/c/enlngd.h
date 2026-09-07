#ifndef ENLNGD_H
#define ENLNGD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define ENLNGD_VERSION "5.3.0"
#define MAX_TOKENS 128
#define MAX_RULES 128
#define MAX_PROPS 32

typedef struct {
    char name[64];
    char value[128];
    char type[32]; // "color", "size", "font", "other"
} EnlngdToken;

typedef struct {
    char prop[64];
    char value[128];
} EnlngdProp;

typedef struct {
    char selector[128];
    EnlngdProp props[MAX_PROPS];
    int prop_count;
    bool is_hover;
} EnlngdRule;

typedef struct {
    EnlngdToken tokens[MAX_TOKENS];
    int token_count;
    EnlngdRule rules[MAX_RULES];
    int rule_count;
} EnlngdSheet;

/* Core API */
EnlngdSheet* enlngd_create_sheet(void);
void enlngd_free_sheet(EnlngdSheet* sheet);
bool enlngd_parse_file(EnlngdSheet* sheet, const char* filepath);
int enlngd_inspect_sheet(const EnlngdSheet* sheet);
int enlngd_compile_to_css(const EnlngdSheet* sheet, const char* outpath);

#endif /* ENLNGD_H */
