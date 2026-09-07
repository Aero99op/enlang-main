#include "enlngd.h"

int main(int argc, char* argv[]) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("enlngd - Enlangg Sovereign Design Tokens & Style Engine v%s (Pure C Zero-Python)\n\n", ENLNGD_VERSION);
        printf("Usage:\n");
        printf("  enlngd <file.enlngd>                         Inspect and resolve design tokens & styles\n");
        printf("  enlngd inspect <file.enlngd>                 Display detailed design token table\n");
        printf("  enlngd compile <file.enlngd> [-o <out.css>]  Export design tokens to CSS3 (Web Target)\n");
        printf("  enlngd --version                             Display version information\n");
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("enlngd version %s (Pure C Sovereign Design Tokens Engine - ZERO PYTHON)\n", ENLNGD_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "compile") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: enlngd compile <file.enlngd> [-o <out.css>]\n");
            return 1;
        }
        const char* src = argv[2];
        char outname[256];
        const char* out = NULL;
        if (argc >= 5 && strcmp(argv[3], "-o") == 0) {
            out = argv[4];
        } else {
            snprintf(outname, sizeof(outname), "%s.css", src);
            out = outname;
        }

        EnlngdSheet* sheet = enlngd_create_sheet();
        if (!enlngd_parse_file(sheet, src)) {
            fprintf(stderr, "Error: Failed to parse '%s'\n", src);
            enlngd_free_sheet(sheet);
            return 1;
        }
        int res = enlngd_compile_to_css(sheet, out);
        enlngd_free_sheet(sheet);
        return res;
    }

    const char* filepath = argv[1];
    if (strcmp(filepath, "inspect") == 0 && argc >= 3) {
        filepath = argv[2];
    }

    EnlngdSheet* sheet = enlngd_create_sheet();
    if (!enlngd_parse_file(sheet, filepath)) {
        fprintf(stderr, "Error: Failed to open or parse '%s'\n", filepath);
        enlngd_free_sheet(sheet);
        return 1;
    }
    enlngd_inspect_sheet(sheet);
    enlngd_free_sheet(sheet);
    return 0;
}
