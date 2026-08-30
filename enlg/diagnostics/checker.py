"""enlg Syntax & Error Diagnostic Checker.

Performs static analysis and grammar verification across all Enlang sub-languages:
- .enlg  (Core backend & logic)
- .enlgf (Frontend web markup)
- .enlgd (Design & stylesheets)
- .enlgs (Client-side reactive scripting)
- .enlgm (Mobile flutter application)
- .enlgdb (Natural English SQL & Database)
"""

import os
import sys
import traceback
from typing import Tuple, List, Optional

def check_file(filepath: str) -> Tuple[bool, List[str], int]:
    """Inspects a file for syntax errors without executing it.
    
    Returns:
        (is_valid: bool, errors: List[str], line_count: int)
    """
    if not os.path.exists(filepath):
        return False, [f"File not found: '{filepath}'"], 0

    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    lines = content.splitlines()
    line_count = len(lines)
    errors = []
    file_lower = filepath.lower()

    try:
        if file_lower.endswith(".enlg"):
            from enlg.lexer.lexer import Lexer
            from enlg.parser.block_parser import BlockParser
            from enlg.compiler.generator import CIRGenerator

            tokens = Lexer(content).tokenize()
            ast = BlockParser.parse(tokens)
            CIRGenerator().generate(ast)

        elif file_lower.endswith((".enlgf", ".html")):
            from enlgf.lexer import ENLGFLexer
            from enlgf.parser import ENLGFParser

            tokens = ENLGFLexer(content).tokenize()
            ENLGFParser(tokens).parse()

        elif file_lower.endswith((".enlgd", ".css")):
            from enlgd.lexer import ENLGDLexer
            from enlgd.parser import ENLGDParser

            tokens = ENLGDLexer(content).tokenize()
            ENLGDParser(tokens).parse()

        elif file_lower.endswith((".enlgs", ".js")):
            from enlgs.lexer import ENLGSLexer
            from enlgs.parser import ENLGSParser
            from enlgs.emitter import ENLGSEmitter

            tokens = ENLGSLexer(content).tokenize()
            ast = ENLGSParser(tokens).parse()
            ENLGSEmitter(ast).emit()

        elif file_lower.endswith((".enlgm", ".dart")):
            from enlgm.lexer import ENLGMLexer
            from enlgm.parser import ENLGMParser
            from enlgm.emitter import ENLGMEmitter

            tokens = ENLGMLexer(content).tokenize()
            ast = ENLGMParser(tokens).parse()
            ENLGMEmitter(ast).emit()

        elif file_lower.endswith((".enlgdb", ".sql")):
            from enlgdb.compiler import compile_enlgdb_source
            compile_enlgdb_source(content)

        elif file_lower.endswith(".py"):
            import ast
            ast.parse(content, filename=filepath)

        else:
            # Try parsing as core .enlg
            from enlg.lexer.lexer import Lexer
            from enlg.parser.block_parser import BlockParser
            tokens = Lexer(content).tokenize()
            BlockParser.parse(tokens)

    except Exception as e:
        err_msg = str(e)
        # Extract line number if available
        line_num = getattr(e, "line_num", None) or getattr(e, "lineno", None)
        if line_num and 1 <= line_num <= len(lines):
            bad_line = lines[line_num - 1].strip()
            errors.append(f"Line {line_num}: {err_msg}\n    --> {bad_line}")
        else:
            errors.append(err_msg)

    return (len(errors) == 0, errors, line_count)

def print_diagnostic_report(filepath: str) -> bool:
    """Runs check_file and prints a rich terminal diagnostic card."""
    is_valid, errors, line_count = check_file(filepath)
    filename = os.path.basename(filepath)

    print("================================================================")
    print(f"  EnLang Syntax Diagnostic: {filename}")
    print("================================================================")
    print(f"  Target File:  {filepath}")
    print(f"  Total Lines:  {line_count}")

    if is_valid:
        print(f"  Status:       [PASS] No syntax errors found!")
        print(f"  Integrity:    100% Valid EnLang Source Code")
        print("================================================================")
        return True
    else:
        print(f"  Status:       [FAIL] {len(errors)} Syntax Error(s) Detected:")
        print("----------------------------------------------------------------")
        for idx, err in enumerate(errors, 1):
            print(f"  [{idx}] {err}")
        print("================================================================")
        return False
