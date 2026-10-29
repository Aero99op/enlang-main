/*
 * pack.c - build-time tool that bundles the staged toolchain into payload.bin
 *
 * Usage: pack <out.bin> <base-dir> <relative/path> [<relative/path> ...]
 *
 * Format (little endian):
 *   u32 magic 'ENLP', u32 count
 *   count x { u16 pathlen, path bytes (forward slashes), u32 size, data }
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put16(FILE *f, unsigned v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); }
static void put32(FILE *f, unsigned long v) {
    fputc((int)(v & 255), f); fputc((int)((v >> 8) & 255), f);
    fputc((int)((v >> 16) & 255), f); fputc((int)((v >> 24) & 255), f);
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: pack out.bin base rel...\n"); return 2; }
    FILE *out = fopen(argv[1], "wb");
    if (!out) { perror(argv[1]); return 1; }
    put32(out, 0x504C4E45UL);
    put32(out, (unsigned long)(argc - 3));
    for (int i = 3; i < argc; i++) {
        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", argv[2], argv[i]);
        FILE *in = fopen(full, "rb");
        if (!in) { perror(full); return 1; }
        fseek(in, 0, SEEK_END);
        long n = ftell(in);
        fseek(in, 0, SEEK_SET);
        unsigned char *buf = (unsigned char *)malloc(n > 0 ? (size_t)n : 1);
        if (!buf || fread(buf, 1, (size_t)n, in) != (size_t)n) { fprintf(stderr, "read failed: %s\n", full); return 1; }
        fclose(in);
        put16(out, (unsigned)strlen(argv[i]));
        fwrite(argv[i], 1, strlen(argv[i]), out);
        put32(out, (unsigned long)n);
        fwrite(buf, 1, (size_t)n, out);
        free(buf);
    }
    fclose(out);
    return 0;
}
