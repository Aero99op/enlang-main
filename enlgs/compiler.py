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
    try:
        return compile_enlgs_source(source)
    except Exception as e:
        import sys
        from enlg.diagnostics.error_formatter import format_human_diagnostic
        token = getattr(e, "token", None)
        line_num = getattr(token, "line", None) or getattr(e, "line", None)
        col_num = getattr(token, "column", None) or getattr(e, "col", None) or getattr(e, "column", None)
        card = format_human_diagnostic(
            source=source,
            line=line_num,
            col=col_num,
            file_path=filepath,
            domain="enlngs",
            error_type=e.__class__.__name__,
            raw_error=str(e)
        )
        print(card, file=sys.stderr)
        sys.exit(1)

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
