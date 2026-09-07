#include "enlngdb.h"

int main(int argc, char* argv[]) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("EnlngDB Pure C Native Database Engine v%s\n", ENLNGDB_VERSION);
        printf("Usage:\n");
        printf("  enlngdb <script.enlngdb>       Execute natural English database script\n");
        printf("  enlngdb -e \"<query>\"           Execute inline statement\n");
        printf("  enlngdb --version              Display version information\n");
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("enlngdb version %s (Pure C Microsecond Database Engine)\n", ENLNGDB_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "-e") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing query string for -e.\n");
            return 1;
        }
        EnlngDatabase* db = enlngdb_create("default");
        int count = enlngdb_execute_script(db, argv[2], true);
        enlngdb_free(db);
        return count >= 0 ? 0 : 1;
    }

    const char* filepath = argv[1];
    return enlngdb_run_file(filepath);
}
