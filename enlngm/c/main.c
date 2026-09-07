#include "enlngm.h"

int main(int argc, char* argv[]) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("enlngm - Enlangg Sovereign Mobile Native & HAL Engine v%s (Pure C Zero-Python)\n\n", ENLNGM_VERSION);
        printf("Usage:\n");
        printf("  enlngm <file.enlngm>                         Launch Native Smartphone Simulator Window\n");
        printf("  enlngm run <file.enlngm>                     Launch Native Smartphone Simulator Window\n");
        printf("  enlngm build <file.enlngm> [--target <apk|ipa>] [-o <out>] Build production package\n");
        printf("  enlngm export <file.enlngm> [-o <out.dart>]  Export to Flutter/Dart source code\n");
        printf("  enlngm --version                             Display version information\n");
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("enlngm version %s (Pure C Sovereign Mobile Native Engine - ZERO PYTHON)\n", ENLNGM_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "build") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: enlngm build <file.enlngm> [--target <apk|ipa>] [-o <out>]\n");
            return 1;
        }
        const char* src = argv[2];
        const char* target = "apk";
        const char* out = "app.apk";
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) target = argv[i + 1];
            else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) out = argv[i + 1];
        }
        return enlngm_build_package(src, target, out);
    }

    if (strcmp(argv[1], "export") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: enlngm export <file.enlngm> [-o <out.dart>]\n");
            return 1;
        }
        const char* src = argv[2];
        char outname[256];
        const char* out = NULL;
        if (argc >= 5 && strcmp(argv[3], "-o") == 0) out = argv[4];
        else {
            snprintf(outname, sizeof(outname), "%s.dart", src);
            out = outname;
        }
        EnlngmApp* app = enlngm_create_app();
        if (!enlngm_parse_file(app, src)) {
            fprintf(stderr, "Error: Could not parse '%s'\n", src);
            enlngm_free_app(app);
            return 1;
        }
        int res = enlngm_export_dart(app, out);
        enlngm_free_app(app);
        return res;
    }

    const char* filepath = argv[1];
    if (strcmp(filepath, "run") == 0 && argc >= 3) {
        filepath = argv[2];
    }

    return enlngm_run_simulator(filepath);
}
