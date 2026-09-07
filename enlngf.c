/*
 * enlngf - Enlangg Frontend & Web Studio Engine
 * Tier: Frontend Markup, Component Trees, and Interactive Web Studio (.enlngf)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define VERSION "5.0.0"
#define TOOL_NAME "enlngf"
#define MODULE_NAME "enlgf"
#define DESCRIPTION "Enlangg Frontend & Web Studio Engine"

int main(int argc, char* argv[]) {
    if (argc >= 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        printf("%s version %s (%s)\n", TOOL_NAME, VERSION, DESCRIPTION);
        return 0;
    }
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("%s - %s v%s\n\n", TOOL_NAME, DESCRIPTION, VERSION);
        printf("Usage:\n");
        printf("  %s <file.enlngf> [--port <port>]       Serve live over HTTP with hot reload\n", TOOL_NAME);
        printf("  %s run <file.enlngf> [--port <port>]   Launch interactive Web Studio\n", TOOL_NAME);
        printf("  %s build <file.enlngf> [-o <out.html>] Compile frontend markup to standalone HTML\n", TOOL_NAME);
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
