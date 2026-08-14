"""enlgd Compiler Interface.

Provides convenient functions to compile .enlgd stylesheet source/files directly to CSS.
"""

import os
from .lexer import ENLGDLexer
from .parser import ENLGDParser
from .emitter import ENLGDEmitter

def compile_enlgd_source(source: str) -> str:
    """Compiles .enlgd stylesheet source string into standard CSS string."""
    tokens = ENLGDLexer(source).tokenize()
    ast = ENLGDParser(tokens).parse()
    css = ENLGDEmitter(ast).emit()
    return css

def compile_enlgd_file(filepath: str) -> str:
    """Reads a .enlgd file and compiles it into standard CSS."""
    with open(filepath, "r", encoding="utf-8") as f:
        source = f.read()
    return compile_enlgd_source(source)

def build_enlgd_file(input_path: str, output_path: str = None) -> str:
    """Compiles a .enlgd file and writes the resulting CSS to disk."""
    css = compile_enlgd_file(input_path)
    if output_path is None:
        base, _ = os.path.splitext(input_path)
        output_path = f"{base}.css"
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(css)
    print(f"[enlgd] Built CSS: {output_path}")
    return output_path
