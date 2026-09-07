/*
 * enlngd - Enlangg Design Tokens & Stylesheet Engine
 * Tier: Design Tokens, Responsive Themes, and Production CSS Generator (.enlngd)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define VERSION "5.0.0"
#define TOOL_NAME "enlngd"
#define MODULE_NAME "enlgd"
#define DESCRIPTION "Enlangg Design Tokens & Stylesheet Engine"

int main(int argc, char* argv[]) {
    if (argc >= 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        printf("%s version %s (%s)\n", TOOL_NAME, VERSION, DESCRIPTION);
        return 0;
    }
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("%s - %s v%s\n\n", TOOL_NAME, DESCRIPTION, VERSION);
        printf("Usage:\n");
        printf("  %s <file.enlngd> [-o <out.css>]        Compile design tokens to CSS\n", TOOL_NAME);
        printf("  %s compile <file.enlngd> [-o <out.css>] Compile design tokens to CSS\n", TOOL_NAME);
        printf("  %s --version                           Display version information\n", TOOL_NAME);
        return 0;
    }

    char cmd[4096] = "python -m " MODULE_NAME;
    for (int i = 1; i < argc; i++) {
        strcat(cmd, " \"");
        strcat(cmd, argv[i]);
        strcat(cmd, "\"");
    }
    return system(cmd);
}
