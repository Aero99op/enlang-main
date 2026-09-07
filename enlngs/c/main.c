#include "enlngs.h"

int main(int argc, char* argv[]) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("enlngs - Enlangg Sovereign Reactive Script Engine v%s (Pure C Zero-Python)\n\n", ENLNGS_VERSION);
        printf("Usage:\n");
        printf("  enlngs <file.enlngs>                         Execute natively on Sovereign Script Engine\n");
        printf("  enlngs run <file.enlngs>                     Execute natively on Sovereign Script Engine\n");
        printf("  enlngs compile <file.enlngs> [-o <out.js>]   Export reactive script to JavaScript (Web Target)\n");
        printf("  enlngs -e \"<script>\"                         Execute inline script statement\n");
        printf("  enlngs --version                             Display version information\n");
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("enlngs version %s (Pure C Sovereign Reactive Script Engine - ZERO PYTHON)\n", ENLNGS_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "-e") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing code string for -e.\n");
            return 1;
        }
        EnlngsVM* vm = enlngs_create_vm();
        enlngs_exec_script(vm, argv[2]);
        enlngs_free_vm(vm);
        return 0;
    }

    if (strcmp(argv[1], "compile") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: enlngs compile <file.enlngs> [-o <out.js>]\n");
            return 1;
        }
        const char* src = argv[2];
        const char* out = NULL;
        if (argc >= 5 && strcmp(argv[3], "-o") == 0) {
            out = argv[4];
        }
        return enlngs_compile_to_js(src, out);
    }

    const char* filepath = argv[1];
    if (strcmp(filepath, "run") == 0 && argc >= 3) {
        filepath = argv[2];
    }

    return enlngs_run_file(filepath);
}
