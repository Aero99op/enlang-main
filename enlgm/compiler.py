"""enlgm Compiler Interface.

Provides convenient functions to compile .enlgm source code and files directly into Flutter / Dart.
"""

import os
from .lexer import ENLGMLexer
from .parser import ENLGMParser
from .emitter import ENLGMEmitter

def compile_enlgm_source(source: str) -> str:
    """Compiles .enlgm mobile source string into clean Flutter / Dart code."""
    tokens = ENLGMLexer(source).tokenize()
    ast = ENLGMParser(tokens).parse()
    dart_code = ENLGMEmitter(ast).emit()
    return dart_code

def compile_enlgm_file(filepath: str) -> str:
    """Reads a .enlgm file and compiles it into Flutter / Dart code."""
    with open(filepath, "r", encoding="utf-8") as f:
        source = f.read()
    return compile_enlgm_source(source)

def build_enlgm_file(input_path: str, output_path: str = None) -> str:
    """Compiles a .enlgm file and writes the resulting Dart file to disk (defaults to lib/main.dart or {base}.dart)."""
    dart_code = compile_enlgm_file(input_path)
    if output_path is None:
        base, _ = os.path.splitext(input_path)
        # If in a flutter project with lib/ folder, write to lib/main.dart
        if os.path.exists("lib") and os.path.isdir("lib"):
            output_path = os.path.join("lib", "main.dart")
        else:
            output_path = f"{base}.dart"

    out_dir = os.path.dirname(output_path)
    if out_dir and not os.path.exists(out_dir):
        os.makedirs(out_dir, exist_ok=True)

    with open(output_path, "w", encoding="utf-8") as f:
        f.write(dart_code)
    print(f"[enlgm] Built Flutter / Dart: {output_path}")
    return output_path
