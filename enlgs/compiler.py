"""enlgs Compiler Interface.

Provides convenient functions to compile .enlgs source code and files directly into JavaScript.
"""

import os
from .lexer import ENLGSLexer
from .parser import ENLGSParser
from .emitter import ENLGSEmitter

def compile_enlgs_source(source: str) -> str:
    """Compiles .enlgs script source string into standard JavaScript."""
    tokens = ENLGSLexer(source).tokenize()
    ast = ENLGSParser(tokens).parse()
    js = ENLGSEmitter(ast).emit()
    return js

def compile_enlgs_file(filepath: str) -> str:
    """Reads a .enlgs file and compiles it into JavaScript."""
    with open(filepath, "r", encoding="utf-8") as f:
        source = f.read()
    return compile_enlgs_source(source)

def build_enlgs_file(input_path: str, output_path: str = None) -> str:
    """Compiles a .enlgs file and writes the resulting .js file to disk."""
    js = compile_enlgs_file(input_path)
    if output_path is None:
        base, _ = os.path.splitext(input_path)
        output_path = f"{base}.js"
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(js)
    print(f"[enlgs] Built JavaScript: {output_path}")
    return output_path
