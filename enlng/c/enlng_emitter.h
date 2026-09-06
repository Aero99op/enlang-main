#ifndef ENLNG_EMITTER_H
#define ENLNG_EMITTER_H

#include <stdbool.h>

/*
 * Checks if a natural English script requires the Python God Mode bridge
 * (e.g. contains 'import python module' or 'use library "numpy"').
 */
bool enlng_has_python_dependency(const char* filepath);

/*
 * Emits clean, standalone ISO C99 source code with automatic arena memory
 * management from a natural English (.enlng / .enlg) file.
 */
bool enlng_emit_c_from_file(const char* in_filepath, const char* out_c_filepath);

/*
 * Ahead-Of-Time (AOT) compiles a natural English file directly into a standalone
 * native x86_64 Windows machine code executable (.exe) using GCC -O3.
 */
bool enlng_compile_file_to_exe(const char* in_filepath, const char* out_exe_filepath);

#endif /* ENLNG_EMITTER_H */
