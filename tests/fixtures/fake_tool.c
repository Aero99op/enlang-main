/*
 * Test fixture: an impostor toolchain binary.
 * Used by tests/test_enlangg_dispatch.ps1 to prove that enlangg runs
 * the tool next to itself and does not pick up a same-named program
 * from PATH or the current directory. Prints a marker and exits 42.
 */
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    const char* name = (argc > 0 && argv[0]) ? argv[0] : "?";
    const char* s1 = strrchr(name, '/');
    const char* s2 = strrchr(name, '\\');
    if (s2 && (!s1 || s2 > s1)) s1 = s2;
    if (s1) name = s1 + 1;
    printf("FAKE-TOOL-FROM-PATH %s\n", name);
    return 42;
}
