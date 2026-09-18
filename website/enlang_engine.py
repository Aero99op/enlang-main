"""
Enlang Sovereign In-Browser WebAssembly Compiler Engine.
Executes all 6 Enlang domains (enlng, enlngdb, enlngd, enlngf, enlgs, enlngm)
directly inside the browser's WebAssembly sandbox.
Zero backend server, zero bridge required.
"""

import sys
import os
import re
import math
import time
import random
import json
import io
import traceback
from enlg.diagnostics.error_formatter import format_human_diagnostic

# --- Standard Built-in Utilities for Universal Execution ---
def _smart_input(prompt=''):
    return ''

def _smart_display(*args, sep=' '):
    if not args:
        print()
        return
    if sep == '':
        print(''.join(str(a) for a in args))
        return
    res = []
    for i, a in enumerate(args):
        s = str(a)
        if i > 0 and res:
            prev = res[-1]
            if not prev.endswith((' ', '\t', '\n')) and not s.startswith((' ', '\t', '\n')):
                res.append(sep)
        res.append(s)
    print(''.join(res))

def _transpile_enlng_line(line: str) -> str:
    indent_len = len(line) - len(line.lstrip(' '))
    indent = line[:indent_len]
    trimmed = line.strip()
    if not trimmed or trimmed.startswith('#'):
        return line
    if trimmed.startswith('type ') or trimmed.startswith('hint '):
        return f"{indent}# {trimmed}"

    def fix_expr(expr):
        expr = re.sub(r'\bis equal to\b', '==', expr)
        expr = re.sub(r'\bis not equal to\b', '!=', expr)
        expr = re.sub(r'\bis at least\b', '>=', expr)
        expr = re.sub(r'\bis at most\b', '<=', expr)
        expr = re.sub(r'\bis greater than or equal to\b', '>=', expr)
        expr = re.sub(r'\bis less than or equal to\b', '<=', expr)
        expr = re.sub(r'\bis greater than\b', '>', expr)
        expr = re.sub(r'\bis less than\b', '<', expr)
        expr = re.sub(r'\bgreater than\b', '>', expr)
        expr = re.sub(r'\bless than\b', '<', expr)
        expr = re.sub(r'\bequal to\b', '==', expr)
        expr = re.sub(r'\bequals\b', '==', expr)
        expr = re.sub(r'\bmultiplied by\b', '*', expr)
        expr = re.sub(r'\bdivided by\b', '/', expr)
        expr = re.sub(r'\bplus\b', '+', expr)
        expr = re.sub(r'\bminus\b', '-', expr)
        expr = re.sub(r'\b(mod|modulo|modulus|modulous)\b', '%', expr)
        expr = re.sub(r'\b([a-zA-Z0-9_\[\]]+)\s+contains\s+(.*)', r'(\2 in \1)', expr)
        expr = re.sub(r'\bcall\s+([a-zA-Z0-9_]+)\s+with\s+(.*?)(?=[,\):]|$)', r'\1(\2)', expr)
        expr = re.sub(r'\bcall\s+([a-zA-Z0-9_]+)\b', r'\1()', expr)
        expr = re.sub(r'\b(?:count of|length of)\s+([a-zA-Z0-9_\[\]\"\'\(\)]+)', r'len(\1)', expr)
        expr = re.sub(r'\breverse\s+(?:of\s+)?([a-zA-Z0-9_\[\]\"\'\(\)]+)', r'(\1[::-1] if hasattr(\1, "__getitem__") else \1)', expr)
        expr = re.sub(r'\b([a-zA-Z0-9_]+)\s+at\s+([a-zA-Z0-9_\"\'\(\)]+)', r'\1[\2]', expr)
        return expr

    # 1. Reverse statement: reverse target
    m = re.match(r'^reverse\s+([a-zA-Z0-9_]+)$', trimmed, re.I)
    if m:
        return f"{indent}{m.group(1)} = {m.group(1)}[::-1]"

    # 2. Variable Declarations
    m = re.match(r'^(?:remember\s+|freeze\s+|create\s+(?:a\s+|an\s+|the\s+)?|declare\s+|let\s+|define\s+)([a-zA-Z0-9_]+)\s+(?:of|as|to|=)\s+(.*)$', trimmed, re.I)
    if m:
        return f"{indent}{m.group(1)} = {fix_expr(m.group(2))}"

    # 3. Increment / Decrement
    m = re.match(r'^([a-zA-Z0-9_\[\]\.]+)\s+(?:increases|increased)\s+by\s+(.*)$', trimmed, re.I)
    if m:
        return f"{indent}{m.group(1)} += {fix_expr(m.group(2))}"
    m = re.match(r'^([a-zA-Z0-9_\[\]\.]+)\s+(?:decreases|decreased)\s+by\s+(.*)$', trimmed, re.I)
    if m:
        return f"{indent}{m.group(1)} -= {fix_expr(m.group(2))}"

    # 4. Assignments & Mutations
    m = re.match(r'^(?:set|update|assign|change)\s+([a-zA-Z0-9_\[\]\.]+)\s+(?:to|=)\s+(.*)$', trimmed, re.I)
    if m:
        return f"{indent}{m.group(1)} = {fix_expr(m.group(2))}"

    # 5. Swap
    m = re.match(r'^swap\s+([a-zA-Z0-9_\[\]\.]+)(?:\s*(?:and|with|,)\s*|\s+)([a-zA-Z0-9_\[\]\.]+)$', trimmed, re.I)
    if m and m.group(1).lower() != 'pair':
        return f"{indent}{m.group(1)}, {m.group(2)} = {m.group(2)}, {m.group(1)}"

    # 6. Conditionals
    m = re.match(r'^(?:when|if)\s+(.*?):$', trimmed, re.I)
    if m:
        return f"{indent}if {fix_expr(m.group(1))}:"
    m = re.match(r'^(?:otherwise\s+when|otherwise\s+if|elif)\s+(.*?):$', trimmed, re.I)
    if m:
        return f"{indent}elif {fix_expr(m.group(1))}:"
    if re.match(r'^(?:otherwise|else):$', trimmed, re.I):
        return f"{indent}else:"

    # 7. Loops
    m = re.match(r'^(?:repeat\s+while|while)\s+(.*?):$', trimmed, re.I)
    if m:
        return f"{indent}while {fix_expr(m.group(1))}:"
    m = re.match(r'^for\s+([a-zA-Z0-9_]+)\s+in\s+(.*?):$', trimmed, re.I)
    if m:
        return f"{indent}for {m.group(1)} in {fix_expr(m.group(2))}:"
    m = re.match(r'^for\s+([a-zA-Z0-9_]+)\s+from\s+(.*?)\s+to\s+(.*?)(?:\s+by\s+(.*?))?:$', trimmed, re.I)
    if m:
        start_val = fix_expr(m.group(2))
        end_val = fix_expr(m.group(3))
        step_val = fix_expr(m.group(4)) if m.group(4) else "1"
        return f"{indent}for {m.group(1)} in range({start_val}, {end_val} + 1, {step_val}):"

    # 8. Function Definition
    m = re.match(r'^(?:function|define|def)\s+([a-zA-Z0-9_]+)\s+with\s+(.*?):$', trimmed, re.I)
    if m:
        return f"{indent}def {m.group(1)}({m.group(2)}):"
    m = re.match(r'^(?:function|define|def)\s+([a-zA-Z0-9_]+)\s*\((.*?)\)\s*:$', trimmed, re.I)
    if m:
        return f"{indent}def {m.group(1)}({m.group(2)}):"

    # 9. Return
    m = re.match(r'^(?:give|return)(?:\s+(.*))?$', trimmed, re.I)
    if m:
        val = fix_expr(m.group(1)) if m.group(1) else ""
        return f"{indent}return {val}"

    # 10. Display / Show
    m = re.match(r'^(?:display|show|output|print)\s+(.*)$', trimmed, re.I)
    if m:
        body = m.group(1).strip()
        sep_arg = ""
        if re.search(r'\s+(?:without\s+spaces?|with\s+no\s+spaces?|joined)\s*$', body, re.I):
            sep_arg = ", sep=''"
            body = re.sub(r'\s+(?:without\s+spaces?|with\s+no\s+spaces?|joined)\s*$', '', body, flags=re.I).strip()
        
        # Auto-insert commas between tokens and string literals
        body = re.sub(r'("[^"]*"|\'[^\']*\')\s+([a-zA-Z0-9_\(\[\{])', r'\1, \2', body)
        body = re.sub(r'([a-zA-Z0-9_\]\)\}])\s+("[^"]*"|\'[^\']*\')', r'\1, \2', body)
        body = re.sub(r'("[^"]*"|\'[^\']*\')\s+("[^"]*"|\'[^\']*\')', r'\1, \2', body)
        body = fix_expr(body)
        return f"{indent}_smart_display({body}{sep_arg})"

    return f"{indent}{fix_expr(trimmed)}"


def _run_universal_enlng(code: str):
    py_lines = []
    for line in code.split('\n'):
        py_lines.append(_transpile_enlng_line(line))
    py_code = '\n'.join(py_lines)

    exec_globals = {
        '_smart_display': _smart_display,
        '_smart_input': _smart_input,
        'true': True, 'false': False, 'null': None,
        'pi': math.pi, 'e': math.e,
        'math': math, 'random': random, 'time': time
    }
    exec(py_code, exec_globals)


def execute_enlang_wasm(filename: str, code: str, domain: str = "enlng") -> str:
    """Entrypoint called from JavaScript WebAssembly."""
    stdout_buf = io.StringIO()
    stderr_buf = io.StringIO()
    old_stdout, old_stderr = sys.stdout, sys.stderr
    sys.stdout, sys.stderr = stdout_buf, stderr_buf

    ext = os.path.splitext(filename)[1].lower().lstrip('.') or domain.lower()
    t0 = time.perf_counter()

    try:
        # 1. Database Domain (.enlngdb, .enlgdb)
        if ext in ('enlngdb', 'enlgdb') or 'type enlngdb' in code.lower() or 'type enlgdb' in code.lower():
            from enlngdb.compiler import run_enlngdb_source
            run_enlngdb_source(code, db_path=':memory:', stream_output=True)

        # 2. Design Token Domain (.enlngd, .enlgd)
        elif ext in ('enlngd', 'enlgd') or 'type enlngd' in code.lower() or 'type enlgd' in code.lower():
            from enlgd.compiler import compile_enlgd_source
            css = compile_enlgd_source(code)
            print(css)

        # 3. Frontend Markup Domain (.enlngf, .enlgf)
        elif ext in ('enlngf', 'enlgf') or 'type enlngf' in code.lower() or 'type enlgf' in code.lower():
            from enlgf.server import compile_enlgf_source
            html = compile_enlgf_source(code)
            print(html)

        # 4. Reactive Script Domain (.enlngs, .enlgs)
        elif ext in ('enlngs', 'enlgs') or 'type enlngs' in code.lower() or 'type enlgs' in code.lower():
            from enlgs.compiler import compile_enlgs_source
            js_code = compile_enlgs_source(code)
            print(js_code)

        # 5. Mobile Domain (.enlngm, .enlgm)
        elif ext in ('enlngm', 'enlgm') or 'type enlngm' in code.lower() or 'type enlgm' in code.lower():
            from enlgm.compiler import compile_enlgm_source
            dart = compile_enlgm_source(code)
            print(dart)

        # 6. Core Enlang Domain (.enlng, .enlg)
        else:
            _run_universal_enlng(code)

        elapsed = (time.perf_counter() - t0) * 1000
        out = stdout_buf.getvalue()
        err = stderr_buf.getvalue()
        res = {
            "success": True,
            "exitCode": 0,
            "output": out + (f"\n{err}" if err else ""),
            "stdout": out,
            "stderr": err,
            "timeMs": round(elapsed, 2),
            "executor": "Enlang Real Compiler (In-Browser WebAssembly)"
        }
        return json.dumps(res)

    except Exception as e:
        elapsed = (time.perf_counter() - t0) * 1000
        line = getattr(e, "line_num", None) or getattr(e, "lineno", None) or getattr(e, "line", None)
        col = getattr(e, "col_num", None) or getattr(e, "column", None) or getattr(e, "col", None)
        card = format_human_diagnostic(
            source=code,
            line=line,
            col=col,
            file_path=filename,
            domain=ext or "enlng",
            error_type=e.__class__.__name__,
            raw_error=str(e)
        )
        res = {
            "success": False,
            "exitCode": 1,
            "output": card,
            "stdout": stdout_buf.getvalue(),
            "stderr": card,
            "timeMs": round(elapsed, 2),
            "executor": "Enlang Real Compiler (In-Browser WebAssembly)",
            "error": str(e)
        }
        return json.dumps(res)
    finally:
        sys.stdout, sys.stderr = old_stdout, old_stderr
