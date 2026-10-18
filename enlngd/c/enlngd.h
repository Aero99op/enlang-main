#ifndef ENLNGD_H
#define ENLNGD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define ENLNGD_VERSION "5.3.0"
#define MAX_TOKENS 512
#define MAX_RULES 512
#define MAX_PROPS 64
#define MAX_MEDIA_RULES 64
#define MAX_KEYFRAMES 64
#define MAX_KEYFRAME_FRAMES 16

typedef struct {
    char name[64];
    char value[256];
    char type[32]; // "color", "size", "font", "other"
} EnlngdToken;

typedef struct {
    char prop[64];
    char value[256];
} EnlngdProp;

typedef struct {
    char selector[256];
    EnlngdProp props[MAX_PROPS];
    int prop_count;
    bool is_hover;
} EnlngdRule;

typedef struct {
    char stop[32]; // "0%", "100%"
    EnlngdProp props[MAX_PROPS];
    int prop_count;
} EnlngdKeyframeFrame;

typedef struct {
    char name[64];
    EnlngdKeyframeFrame frames[MAX_KEYFRAME_FRAMES];
    int frame_count;
} EnlngdKeyframe;

typedef struct {
    char query[128]; // e.g. "(max-width: 768px)"
    EnlngdRule rules[MAX_RULES / 4];
    int rule_count;
} EnlngdMediaRule;

typedef struct {
    EnlngdToken tokens[MAX_TOKENS];
    int token_count;
    EnlngdRule rules[MAX_RULES];
    int rule_count;
    EnlngdMediaRule media_rules[MAX_MEDIA_RULES];
    int media_rule_count;
    EnlngdKeyframe keyframes[MAX_KEYFRAMES];
    int keyframe_count;
} EnlngdSheet;

/* Core API */
EnlngdSheet* enlngd_create_sheet(void);
void enlngd_free_sheet(EnlngdSheet* sheet);
bool enlngd_parse_file(EnlngdSheet* sheet, const char* filepath);
int enlngd_inspect_sheet(const EnlngdSheet* sheet);
int enlngd_compile_to_css(const EnlngdSheet* sheet, const char* outpath);

#endif /* ENLNGD_H */

