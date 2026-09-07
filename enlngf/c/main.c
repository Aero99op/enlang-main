#include "enlngf.h"

int main(int argc, char* argv[]) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("enlngf - Enlangg Sovereign Frontend & Web Studio Engine v%s (Pure C Zero-Python)\n\n", ENLNGF_VERSION);
        printf("Usage:\n");
        printf("  enlngf <file.enlngf>                         Launch native Win32 Desktop GUI Window\n");
        printf("  enlngf run <file.enlngf>                     Launch native Win32 Desktop GUI Window\n");
        printf("  enlngf gui <file.enlngf>                     Launch native Win32 Desktop GUI Window\n");
        printf("  enlngf serve <file.enlngf> [--port <port>]   Serve live over embedded WinSock2 HTTP server\n");
        printf("  enlngf build <file.enlngf> [-o <out.html>]   Compile frontend markup to standalone HTML5\n");
        printf("  enlngf --version                             Display version information\n");
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("enlngf version %s (Pure C Sovereign Frontend Engine - ZERO PYTHON)\n", ENLNGF_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "serve") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: enlngf serve <file.enlngf> [--port <port>]\n");
            return 1;
        }
        const char* src = argv[2];
        int port = 3000;
        for (int i = 3; i < argc; i++) {
            if ((strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "-p") == 0) && i + 1 < argc) {
                port = atoi(argv[i + 1]);
                break;
            }
        }
        return enlngf_serve_http(src, port);
    }

    if (strcmp(argv[1], "build") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: enlngf build <file.enlngf> [-o <out.html>]\n");
            return 1;
        }
        const char* src = argv[2];
        char outname[256];
        const char* out = NULL;
        if (argc >= 5 && strcmp(argv[3], "-o") == 0) {
            out = argv[4];
        } else {
            snprintf(outname, sizeof(outname), "%s.html", src);
            out = outname;
        }

        EnlngfDoc* doc = enlngf_create_doc();
        if (!enlngf_parse_file(doc, src)) {
            fprintf(stderr, "Error: Failed to parse '%s'\n", src);
            enlngf_free_doc(doc);
            return 1;
        }
        int res = enlngf_compile_to_html(doc, out);
        enlngf_free_doc(doc);
        return res;
    }

    const char* filepath = argv[1];
    if ((strcmp(filepath, "run") == 0 || strcmp(filepath, "gui") == 0) && argc >= 3) {
        filepath = argv[2];
    }

    return enlngf_run_gui(filepath);
}
