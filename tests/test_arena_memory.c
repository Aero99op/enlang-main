#include "../enlng/c/enlng_mem.h"
#include <time.h>
#include <assert.h>

int main(void) {
    printf("==============================================================\n");
    printf("    SOVEREIGN SCOPED BUMP ARENA MEMORY BENCHMARK (C NATIVE)   \n");
    printf("==============================================================\n");

    enlng_init_memory();
    printf(" >> Arena initialized: 4MB chunk allocated.\n");

    size_t initial_offset = g_enlng_arena.current->offset;
    printf(" >> Baseline Arena Offset: %zu bytes\n", initial_offset);

    clock_t start = clock();
    const int ITERATIONS = 1000000; // 1 Million allocations!

    for (int i = 0; i < ITERATIONS; i++) {
        ENLNG_SCOPE_START();

        // 1. Dynamic string allocations inside scope
        EnlngVal a = enlng_str("Temporary string in scope ");
        EnlngVal b = enlng_int(i);
        EnlngVal s = enlng_add(a, b);

        // 2. Dynamic list creation inside scope
        EnlngList* list = enlng_list_create(8);
        enlng_list_append(list, s);
        enlng_list_append(list, enlng_int(i * 2));
        enlng_list_append(list, enlng_double(i * 1.5));

        // 3. Spoken arithmetic and comparisons
        EnlngVal sum = enlng_add(enlng_int(i), enlng_double(3.14159));
        bool ok = enlng_gt(sum, enlng_int(0));
        (void)ok;

        // AUTOMATIC 1-CYCLE RECLAMATION
        ENLNG_SCOPE_END();
    }

    clock_t end = clock();
    double total_sec = (double)(end - start) / CLOCKS_PER_SEC;
    size_t final_offset = g_enlng_arena.current->offset;

    printf(" >> Executed %d nested scope cycles in %.4f seconds\n", ITERATIONS, total_sec);
    printf(" >> Final Arena Offset: %zu bytes\n", final_offset);
    printf(" >> Leak bytes: %zu bytes (Target: 0 bytes)\n", final_offset - initial_offset);
    printf(" >> Nanoseconds per scope cycle: %.2f ns\n", (total_sec / ITERATIONS) * 1e9);

    if (final_offset == initial_offset) {
        printf("[SUCCESS] AUTOMATIC ARENA RECLAMATION PASSED: 0 LEAKS, 100%% DETERMINISTIC MEMORY!\n");
        return 0;
    } else {
        printf("[FAIL] Memory leaked: %zu bytes\n", final_offset - initial_offset);
        return 1;
    }
}
