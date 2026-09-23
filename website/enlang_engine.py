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
def ask(prompt=''): return _smart_input(prompt)
def _cat(*args): return ''.join(str(a) for a in args)

def sleep(sec): time.sleep(sec)
def random_number(a, b): return random.randint(a, b)
def sqrt(n): return math.sqrt(n)
def floor(n): return math.floor(n)
def ceil(n): return math.ceil(n)

def _enlng_arrange(target, indices):
    if not isinstance(indices, (list, tuple)):
        indices = [indices]
    tlen = len(target)
    res = []
    for idx in indices:
        try:
            i = int(idx)
        except Exception:
            continue
        if i < 0:
            i = tlen + i
        if 0 <= i < tlen:
            res.append(target[i])
    return "".join(res) if isinstance(target, str) else res

def _enlng_parse_pos(pos, slen, is_add):
    p = str(pos)
    if p == 'at_first': return 0
    if p == 'at_last': return slen if is_add else (slen - 1 if slen > 0 else 0)
    try:
        val = int(p)
        if val < 0:
            if is_add: return 0
            val = slen + val
            return max(0, val)
        if is_add and val >= slen: return slen
        if not is_add and val >= slen: return max(0, slen - 1)
        return val
    except: return 0

def string_add(arg1, arg2, pos):
    s1 = str(arg1); s2 = str(arg2)
    if len(s1) == 1 and len(s2) > 1: item = s1; target = s2
    elif len(s1) > 1 and len(s2) == 1: target = s1; item = s2
    elif len(s1) > len(s2): target = s1; item = s2
    elif len(s1) < len(s2):
        try: pval = int(pos)
        except: pval = 0
        if pval <= len(s1) and pval > 0: target = s1; item = s2
        else: item = s1; target = s2
    else: item = s1; target = s2
    slen = len(target)
    idx = _enlng_parse_pos(pos, slen, True)
    return target[:idx] + item + target[idx:]

def string_insert(arg1, arg2, pos): return string_add(arg1, arg2, pos)

def string_remove_at(s, pos):
    s = str(s); slen = len(s)
    if slen == 0: return s
    idx = _enlng_parse_pos(pos, slen, False)
    if 0 <= idx < slen: return s[:idx] + s[idx+1:]
    return s

def string_remove(s, pos):
    s = str(s)
    if isinstance(pos, int) or str(pos) in ('at_first', 'at_last'):
        return string_remove_at(s, pos)
    p = str(pos)
    if (p.startswith('-') and p[1:].isdigit()) or p.isdigit():
        return string_remove_at(s, pos)
    idx = s.find(p)
    if idx != -1: return s[:idx] + s[idx+len(p):]
    return s

def string_replace(arg1, arg2, pos):
    s1 = str(arg1); s2 = str(arg2)
    if len(s1) == 1 and len(s2) > 1: new_val = s1; target = s2
    elif len(s1) > 1 and len(s2) == 1: target = s1; new_val = s2
    elif len(s1) > len(s2): target = s1; new_val = s2
    elif len(s1) < len(s2):
        try: pval = int(pos)
        except: pval = 0
        if pval < len(s1) and pval > 0: target = s1; new_val = s2
        else: new_val = s1; target = s2
    else: new_val = s1; target = s2
    slen = len(target)
    if slen == 0: return target
    idx = _enlng_parse_pos(pos, slen, False)
    if 0 <= idx < slen: return target[:idx] + new_val + target[idx+1:]
    return target

def string_set_at(s, pos, new_char): return string_replace(new_char, s, pos)

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
    # 0a. Library imports (from library / from module / from ... import ...)
    m_from = re.match(r'^from\s+(?:library\s+|module\s+)?["\']?([a-zA-Z0-9_]+)["\']?\s+import\s+(.*)$', trimmed, re.I)
    if m_from:
        return f"{indent}from {m_from.group(1)} import {m_from.group(2).strip()}"

    # 0b. Library imports (use library / use python module / import library / load library ...)
    m_lib = re.match(r'^(?:use\s+python\s+library|use\s+python\s+module|use\s+library|use\s+module|import\s+python\s+module|import\s+library|import\s+module|load\s+library|load\s+module)\s+["\']?([a-zA-Z0-9_]+)["\']?(?:\s+as\s+([a-zA-Z0-9_]+))?$', trimmed, re.I)
    if m_lib:
        mod, alias = m_lib.group(1), m_lib.group(2)
        if alias: return f"{indent}import {mod} as {alias}"
        return f"{indent}import {mod}"

    # 0c. Direct import statement (import <mod> [as <alias>])
    m_imp = re.match(r'^import\s+([a-zA-Z0-9_]+)(?:\s+as\s+([a-zA-Z0-9_]+))?$', trimmed, re.I)
    if m_imp:
        mod, alias = m_imp.group(1), m_imp.group(2)
        if alias: return f"{indent}import {mod} as {alias}"
        return f"{indent}import {mod}"

    # 0d. Local module file inclusion: use "file.enlng" / import "file.enlng"
    m_use = re.match(r'^(?:use|import)\s+["\']([^"\']+)["\']$', trimmed, re.I)
    if m_use:
        imported_file = m_use.group(1)
        for search_dir in ['.', 'stdlib', 'lib', 'website']:
            candidate = os.path.join(search_dir, imported_file)
            if os.path.exists(candidate):
                try:
                    with open(candidate, 'r', encoding='utf-8') as fh:
                        imported_src = fh.read()
                    sub_lines = [_transpile_enlng_line(l) for l in imported_src.splitlines()]
                    return '\n'.join(sub_lines)
                except Exception:
                    pass
        if re.match(r'^[a-zA-Z0-9_]+$', imported_file):
            return f"{indent}import {imported_file}"
        return f"{indent}# {trimmed}"

    def fix_expr(expr):
        str_literals = []
        def _mask_str(m):
            str_literals.append(m.group(0))
            return f"__STR_LITERAL_{len(str_literals) - 1}__"
        expr = re.sub(r'("[^"]*"|\'[^\']*\')', _mask_str, expr)

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
        expr = re.sub(r'\b(mod|modulo|modulus|modulous|modoulous)\b', '%', expr)
        # Action Word Predicates & Comparisons
        expr = re.sub(r'\b([a-zA-Z0-9_\[\]\.]+)\s+starts\s+with\s+(.*?)(?=[,\):]|$)', r'(\1.startswith(\2) if hasattr(\1, "startswith") else False)', expr)
        expr = re.sub(r'\b([a-zA-Z0-9_\[\]\.]+)\s+ends\s+with\s+(.*?)(?=[,\):]|$)', r'(\1.endswith(\2) if hasattr(\1, "endswith") else False)', expr)
        expr = re.sub(r'\b([a-zA-Z0-9_\[\]\.]+)\s+is\s+between\s+(.*?)\s+and\s+([a-zA-Z0-9_\[\]\.\(\)]+)', r'(\2 <= \1 <= \3)', expr)
        expr = re.sub(r'\b([a-zA-Z0-9_\[\]\.]+)\s+is\s+even\b', r'(\1 % 2 == 0)', expr)
        expr = re.sub(r'\b([a-zA-Z0-9_\[\]\.]+)\s+is\s+odd\b', r'(\1 % 2 != 0)', expr)
        expr = re.sub(r'\b([a-zA-Z0-9_\[\]\.]+)\s+is\s+not\s+empty\b', r'(len(\1) > 0 if hasattr(\1, "__len__") else bool(\1))', expr)
        expr = re.sub(r'\b([a-zA-Z0-9_\[\]\.]+)\s+is\s+empty\b', r'(len(\1) == 0 if hasattr(\1, "__len__") else not \1)', expr)

        # Action Aggregations
        expr = re.sub(r'\bsum\s+of\s+([a-zA-Z0-9_\[\]\(\)]+)', r'sum(\1)', expr)
        expr = re.sub(r'\b(?:average|avg)\s+of\s+([a-zA-Z0-9_\[\]\(\)]+)', r'(sum(\1)/len(\1) if len(\1)>0 else 0)', expr)
        expr = re.sub(r'\b(?:highest|max)\s+of\s+([a-zA-Z0-9_\[\]\(\)]+)', r'max(\1)', expr)
        expr = re.sub(r'\b(?:lowest|min)\s+of\s+([a-zA-Z0-9_\[\]\(\)]+)', r'min(\1)', expr)
        expr = re.sub(r'\buppercase\s+of\s+([a-zA-Z0-9_\[\]\(\)]+)', r'(\1.upper() if hasattr(\1, "upper") else \1)', expr)
        expr = re.sub(r'\blowercase\s+of\s+([a-zA-Z0-9_\[\]\(\)]+)', r'(\1.lower() if hasattr(\1, "lower") else \1)', expr)
        expr = re.sub(r'\btrim\s+spaces\s+from\s+([a-zA-Z0-9_\[\]\(\)]+)', r'(\1.strip() if hasattr(\1, "strip") else \1)', expr)
        expr = re.sub(r'\btrim\s+([a-zA-Z0-9_\[\]\(\)]+)', r'(\1.strip() if hasattr(\1, "strip") else \1)', expr)

        # Action Words: replace, split, join, find
        expr = re.sub(r'\breplace\s+(.*?)\s+with\s+(.*?)\s+in\s+([a-zA-Z0-9_\[\]\(\)\.]+)', r'(\3.replace(\1, \2) if hasattr(\3, "replace") else \3)', expr)
        expr = re.sub(r'\bsplit\s+(.*?)\s+(?:by|on|with)\s+([a-zA-Z0-9_\[\]\(\)\"\'\.]+)', r'(\1.split(\2) if hasattr(\1, "split") else [])', expr)
        expr = re.sub(r'\bjoin\s+(.*?)\s+(?:with|by)\s+([a-zA-Z0-9_\[\]\(\)\"\'\.]+)', r'(\2.join([str(x) for x in \1]) if hasattr(\2, "join") and hasattr(\1, "__iter__") else str(\1))', expr)
        expr = re.sub(r'\bfind\s+(.*?)\s+in\s+([a-zA-Z0-9_\[\]\(\)\.]+)', r'(\2.find(\1) if hasattr(\2, "find") else (\2.index(\1) if \1 in \2 else -1))', expr)

        expr = re.sub(r'\b([a-zA-Z0-9_\[\]]+)\s+contains\s+(.*)', r'(\2 in \1)', expr)
        # Flexible Calling Action Words & Prepositions (use/call/run/invoke/execute/apply)
        expr = re.sub(r'\b(?:call|use|run|invoke|execute|apply)\s+([a-zA-Z0-9_]+)\s+with\s+(.*?)\s+(?:from|using|on|in)\s+([a-zA-Z0-9_\[\]\.]+)\b', r'\3.\1(\2)', expr)
        expr = re.sub(r'\b(?:call|use|run|invoke|execute|apply)\s+([a-zA-Z0-9_]+)\s+(?:from|using|on|in)\s+([a-zA-Z0-9_\[\]\.]+)\s+with\s+(.*?)(?=[,\):]|\s+(?:and|or)\b|$)', r'\2.\1(\3)', expr)
        expr = re.sub(r'\b(?:call|use|run|invoke|execute|apply)\s+([a-zA-Z0-9_]+)\s+(?:from|using|on|in)\s+([a-zA-Z0-9_\[\]\.]+)\b', r'\2.\1()', expr)
        expr = re.sub(r'\b(?:call|use|run|invoke|execute|apply)\s+([a-zA-Z0-9_\[\]\.]+)\.([a-zA-Z0-9_]+)\s+with\s+(.*?)(?=[,\):]|\s+(?:and|or)\b|$)', r'\1.\2(\3)', expr)
        expr = re.sub(r'\b(?:call|use|run|invoke|execute|apply)\s+([a-zA-Z0-9_\[\]\.]+)\.([a-zA-Z0-9_]+)\b', r'\1.\2()', expr)
        expr = re.sub(r'\b(?:tell|ask)\s+([a-zA-Z0-9_\[\]\.]+)\s+to\s+([a-zA-Z0-9_]+)\s+with\s+(.*?)(?=[,\):]|\s+(?:and|or)\b|$)', r'\1.\2(\3)', expr)
        expr = re.sub(r'\b(?:tell|ask)\s+([a-zA-Z0-9_\[\]\.]+)\s+to\s+([a-zA-Z0-9_]+)\b', r'\1.\2()', expr)
        expr = re.sub(r'\b(?:call|run|invoke|execute)\s+([a-zA-Z0-9_]+)\s+with\s+(.*?)(?=[,\):]|\s+(?:and|or)\b|$)', r'\1(\2)', expr)
        expr = re.sub(r'\b(?:call|run|invoke|execute)\s+([a-zA-Z0-9_]+)\b', r'\1()', expr)
        expr = re.sub(r'\bcount\s+(?:of\s+)?(.*?)\s+in\s+([a-zA-Z0-9_\[\]\(\)]+)', r'(\2.count(\1) if hasattr(\2, "count") else 0)', expr)
        expr = re.sub(r'\b(?:count of|length of)\s+(\[.*?\]|\{.*?\}|[a-zA-Z0-9_\"\'\(\)\.]+)', r'len(\1)', expr)
        expr = re.sub(r'\bfirst\s+of\s+(\[.*?\]|\{.*?\}|[a-zA-Z0-9_\"\'\(\)\.]+)', r'(\1[0])', expr)
        expr = re.sub(r'\blast\s+of\s+(\[.*?\]|\{.*?\}|[a-zA-Z0-9_\"\'\(\)\.]+)', r'(\1[-1])', expr)
        expr = re.sub(r'\breverse\s+(?:of\s+)?([a-zA-Z0-9_\[\]\"\'\(\)]+)', r'(\1[::-1] if hasattr(\1, "__getitem__") else \1)', expr)
        expr = re.sub(r'\b([a-zA-Z0-9_]+)\s+at_first\b', r'\1[0]', expr)
        expr = re.sub(r'\b([a-zA-Z0-9_]+)\s+at_last\b', r'\1[-1]', expr)
        expr = re.sub(r'\b([a-zA-Z0-9_]+)\s+at\s+([^\s,\)]+)', r'\1[\2]', expr)
        expr = re.sub(r"\b([a-zA-Z0-9_]+)'s\s+([a-zA-Z0-9_]+)\b", r"(\1.\2 if hasattr(\1, '\2') else \1['\2'])", expr)
        expr = re.sub(r"\b(?!type\b|out\b|end\b|count\b|length\b|reverse\b|uppercase\b|lowercase\b|sum\b|average\b|highest\b|lowest\b|max\b|min\b|first\b|last\b)([a-zA-Z0-9_]+)\s+of\s+(\{.*?\}|\[.*?\]|[a-zA-Z0-9_\.]+)", r"(\2.\1 if hasattr(\2, '\1') else \2['\1'])", expr)

        for i, s in enumerate(str_literals):
            expr = expr.replace(f"__STR_LITERAL_{i}__", s)
        return expr


    # 1. Loop controls: break / continue / pass
    if re.match(r'^(?:break|stop\s+loop|exit\s+loop)$', trimmed, re.I): return f"{indent}break"
    if re.match(r'^(?:continue|skip\s+iteration|next\s+iteration)$', trimmed, re.I): return f"{indent}continue"
    if re.match(r'^(?:pass|do\s+nothing)$', trimmed, re.I): return f"{indent}pass"

    # 2. Return statements: return / give back / give
    m = re.match(r'^(?:return|give\s+back|give)(?:\s+(.*))?$', trimmed, re.I)
    if m:
        val = m.group(1)
        return f"{indent}return {fix_expr(val)}" if val else f"{indent}return"

    # 3. Exception handling: try / catch / finally / raise
    if re.match(r'^(?:try|attempt):$', trimmed, re.I): return f"{indent}try:"
    m = re.match(r'^(?:catch|rescue|except)(?:\\s+as\\s+|\\s+)([a-zA-Z0-9_]+):$', trimmed, re.I)
    if m: return f"{indent}except Exception as {m.group(1)}:"
    if re.match(r'^(?:catch|rescue|except):$', trimmed, re.I): return f"{indent}except Exception:"
    if re.match(r'^finally:$', trimmed, re.I): return f"{indent}finally:"
    m = re.match(r'^(?:raise|throw)\s+(.*)$', trimmed, re.I)
    if m: return f"{indent}raise Exception({fix_expr(m.group(1))})"

    # 4. List mutations: add / append / push / remove / delete
    m = re.match(r'^(?:add|append|push)\s+(.*?)\s+to\s+([a-zA-Z0-9_\[\]\.]+)$', trimmed, re.I)
    if m: return f"{indent}{m.group(2)}.append({fix_expr(m.group(1))})"
    m = re.match(r'^(?:remove|delete)\s+(.*?)\s+from\s+([a-zA-Z0-9_\[\]\.]+)$', trimmed, re.I)
    if m: return f"{indent}{m.group(2)}.remove({fix_expr(m.group(1))})"

    # 4b. String mutations: string_replace / string_add / string_insert / string_remove
    m = re.match(r'^string_replace\s+(.*?)\s+(?:in|into|from|by|to|with|at)\s+([a-zA-Z0-9_]+)(?:\s+(?:at|in|by|to))?\s+(.*)$', trimmed, re.I)
    if m:
        new_val, var_name, pos = m.group(1), m.group(2), m.group(3).strip()
        if pos in ('at_first', 'at_last'): pos = f"'{pos}'"
        else: pos = fix_expr(pos)
        return f"{indent}{var_name} = string_replace({fix_expr(new_val)}, {var_name}, {pos})"

    m = re.match(r'^(?:string_add|string_insert)\s+(.*?)\s+(?:in|into|from|by|to|with|at)\s+([a-zA-Z0-9_]+)(?:\s+(?:at|in|by|to))?\s+(.*)$', trimmed, re.I)
    if m:
        item, var_name, pos = m.group(1), m.group(2), m.group(3).strip()
        if pos in ('at_first', 'at_last'): pos = f"'{pos}'"
        else: pos = fix_expr(pos)
        return f"{indent}{var_name} = string_add({fix_expr(item)}, {var_name}, {pos})"

    m = re.match(r'^string_remove\s+(?:from\s+|in\s+)?([a-zA-Z0-9_]+)(?:\s+(?:at|in|by|to))?\s+(.*)$', trimmed, re.I)
    if m:
        var_name, pos = m.group(1), m.group(2).strip()
        if pos in ('at_first', 'at_last'): pos = f"'{pos}'"
        else: pos = fix_expr(pos)
        return f"{indent}{var_name} = string_remove({var_name}, {pos})"


    # 5. OOP: Class & Methods (class / blueprint / model)
    m = re.match(r'^(?:define\s+class|class|blueprint|model)\s+([a-zA-Z0-9_]+)(?:\\s+(?:inherits\s+from|extends)\s+([a-zA-Z0-9_]+))?:$', trimmed, re.I)
    if m:
        cname, base = m.group(1), m.group(2)
        return f"{indent}class {cname}({base}):" if base else f"{indent}class {cname}:"
    m = re.match(r'^(?:(public|private|protected|hidden|secret)\s+)?(?:method|define\s+method)\s+([a-zA-Z0-9_]+)(?:\s+with\s+(.*?))?:$', trimmed, re.I)
    if m:
        vis, mname, params = m.group(1), m.group(2), m.group(3) or ''
        if vis and vis.lower() in ('private', 'secret', 'hidden', 'protected'): mname = f'_{mname}'
        return f"{indent}def {mname}(self, {params}):" if params else f"{indent}def {mname}(self):"

    # 5b. Flexible Method Invocation Statements
    m = re.match(r'^(?:call|use|run|invoke|execute|apply)\s+([a-zA-Z0-9_]+)\s+with\s+(.*?)\s+(?:from|using|on|in)\s+([a-zA-Z0-9_\[\]\.]+)$', trimmed, re.I)
    if m:
        meth, args, obj = m.group(1), m.group(2), m.group(3)
        return f"{indent}{obj}.{meth}({fix_expr(args)})"

    m = re.match(r'^(?:call|use|run|invoke|execute|apply)\s+([a-zA-Z0-9_]+)\s+(?:from|using|on|in)\s+([a-zA-Z0-9_\[\]\.]+)(?:\s+with\s+(.*))?$', trimmed, re.I)
    if m:
        meth, obj, args = m.group(1), m.group(2), m.group(3)
        return f"{indent}{obj}.{meth}({fix_expr(args)})" if args else f"{indent}{obj}.{meth}()"

    m = re.match(r'^(?:call|use|run|invoke|execute|apply)\s+([a-zA-Z0-9_\[\]\.]+)\.([a-zA-Z0-9_]+)(?:\s+with\s+(.*))?$', trimmed, re.I)
    if m:
        obj, meth, args = m.group(1), m.group(2), m.group(3)
        return f"{indent}{obj}.{meth}({fix_expr(args)})" if args else f"{indent}{obj}.{meth}()"

    m = re.match(r'^(?:tell|ask)\s+([a-zA-Z0-9_\.]+)\s+to\s+([a-zA-Z0-9_]+)(?:\s+with\s+(.*))?$', trimmed, re.I)
    if m:
        obj, meth, args = m.group(1), m.group(2), m.group(3)
        return f"{indent}{obj}.{meth}({fix_expr(args)})" if args else f"{indent}{obj}.{meth}()"

    # 6. Reverse statement: reverse target
    m = re.match(r'^reverse\s+([a-zA-Z0-9_]+)$', trimmed, re.I)
    if m:
        return f"{indent}{m.group(1)} = {m.group(1)}[::-1]"

    # 6b. Arrange statement: arrange target by/with/in indices
    m = re.match(r'^arrange\s+([a-zA-Z0-9_]+)(?:\s+(?:by|with|to|in|at|of|as))?\s+(.*)$', trimmed, re.I)
    if m:
        _tgt, _idx = m.group(1), m.group(2).strip()
        if _idx.startswith('[') and _idx.endswith(']'):
            return f"{indent}{_tgt} = _enlng_arrange({_tgt}, {_idx})"
        elif ',' in _idx:
            return f"{indent}{_tgt} = _enlng_arrange({_tgt}, [{_idx}])"
        else:
            return f"{indent}{_tgt} = _enlng_arrange({_tgt}, {_idx})"

    # 7. Variable Declarations
    m = re.match(r'^(?:remember\s+|freeze\s+|create\s+(?:a\s+|an\s+|the\s+)?|declare\s+|initialize\s+(?:the\s+)?|let\s+|define\s+)([a-zA-Z0-9_]+)\s+(?:of|as|to|=)\s+(.*)$', trimmed, re.I)
    if m:
        return f"{indent}{m.group(1)} = {fix_expr(m.group(2))}"

    # 8. Increment / Decrement / Variable Assignments & Mutations
    m_of_inc = re.match(r'^([a-zA-Z0-9_]+)\s+of\s+([a-zA-Z0-9_]+)\s+(?:increases|increased)\s+by\s+(.*)$', trimmed, re.I)
    if m_of_inc:
        return f"{indent}{m_of_inc.group(2)}['{m_of_inc.group(1)}'] += {fix_expr(m_of_inc.group(3))}"
    m_of_dec = re.match(r'^([a-zA-Z0-9_]+)\s+of\s+([a-zA-Z0-9_]+)\s+(?:decreases|decreased)\s+by\s+(.*)$', trimmed, re.I)
    if m_of_dec:
        return f"{indent}{m_of_dec.group(2)}['{m_of_dec.group(1)}'] -= {fix_expr(m_of_dec.group(3))}"
    m_of_set = re.match(r'^([a-zA-Z0-9_]+)\s+of\s+([a-zA-Z0-9_]+)\s*=\s*(.*)$', trimmed, re.I)
    if m_of_set:
        return f"{indent}{m_of_set.group(2)}['{m_of_set.group(1)}'] = {fix_expr(m_of_set.group(3))}"

    m = re.match(r'^([a-zA-Z0-9_\[\]\.]+)\s+(?:increases|increased)\s+by\s+(.*)$', trimmed, re.I)
    if m:
        return f"{indent}{m.group(1)} += {fix_expr(m.group(2))}"
    m = re.match(r'^([a-zA-Z0-9_\[\]\.]+)\s+(?:decreases|decreased)\s+by\s+(.*)$', trimmed, re.I)
    if m:
        return f"{indent}{m.group(1)} -= {fix_expr(m.group(2))}"
    m = re.match(r'^(?:increase)\s+([a-zA-Z0-9_\[\]\"\.]+)\s+by\s+(.*)$', trimmed, re.I)
    if m: return f"{indent}{m.group(1)} += {fix_expr(m.group(2))}"
    m = re.match(r'^(?:decrease)\s+([a-zA-Z0-9_\[\]\"\.]+)\s+by\s+(.*)$', trimmed, re.I)
    if m: return f"{indent}{m.group(1)} -= {fix_expr(m.group(2))}"
    m = re.match(r'^(?:multiply)\s+([a-zA-Z0-9_\[\]\"\.]+)\s+by\s+(.*)$', trimmed, re.I)
    if m: return f"{indent}{m.group(1)} *= {fix_expr(m.group(2))}"
    m = re.match(r'^(?:divide)\s+([a-zA-Z0-9_\[\]\"\.]+)\s+by\s+(.*)$', trimmed, re.I)
    if m: return f"{indent}{m.group(1)} /= {fix_expr(m.group(2))}"
    m = re.match(r'^(?:set|update|assign|change)\s+([a-zA-Z0-9_\[\]\.]+)\s+(?:to|=)\s+(.*)$', trimmed, re.I)
    if m:
        return f"{indent}{m.group(1)} = {fix_expr(m.group(2))}"

    # 9. Swap
    m = re.match(r'^swap\s+([a-zA-Z0-9_\[\]\.]+)(?:\s*(?:and|with|,)\s*|\s+)([a-zA-Z0-9_\[\]\.]+)$', trimmed, re.I)
    if m and m.group(1).lower() != 'pair':
        return f"{indent}{m.group(1)}, {m.group(2)} = {m.group(2)}, {m.group(1)}"

    # 10. Conditionals
    if trimmed.endswith(':'):
        m = re.match(r'^(?:when|if)\s+(.*):$', trimmed, re.I)
        if m:
            return f"{indent}if {fix_expr(m.group(1))}:"
        m = re.match(r'^(?:otherwise\s+when|otherwise\s+if|elif)\s+(.*):$', trimmed, re.I)
        if m:
            return f"{indent}elif {fix_expr(m.group(1))}:"
        if re.match(r'^(?:otherwise|else):$', trimmed, re.I):
            return f"{indent}else:"
    else:
        colon_idx = -1
        in_q = False
        q_c = ''
        for idx_c, ch in enumerate(trimmed):
            if not in_q and ch in ('"', "'"):
                in_q = True
                q_c = ch
            elif in_q and ch == q_c:
                in_q = False
            elif not in_q and ch == ':':
                colon_idx = idx_c
                break
        if colon_idx != -1:
            head = trimmed[:colon_idx].strip()
            rest = trimmed[colon_idx+1:].strip()
            m = re.match(r'^(?:when|if)\s+(.*)$', head, re.I)
            if m:
                return f"{indent}if {fix_expr(m.group(1))}:\n{indent}    {_transpile_enlng_line(rest)}"
            m = re.match(r'^(?:otherwise\s+when|otherwise\s+if|elif)\s+(.*)$', head, re.I)
            if m:
                return f"{indent}elif {fix_expr(m.group(1))}:\n{indent}    {_transpile_enlng_line(rest)}"
            if re.match(r'^(?:otherwise|else)$', head, re.I):
                return f"{indent}else:\n{indent}    {_transpile_enlng_line(rest)}"

    # 11. Loops
    m = re.match(r'^repeat\s+(.*?)\s+times:$', trimmed, re.I)
    if m:
        return f"{indent}for _ in range({fix_expr(m.group(1))}):"
    m = re.match(r'^(?:repeat\s+until|until)\s+(.*?):$', trimmed, re.I)
    if m:
        return f"{indent}while not ({fix_expr(m.group(1))}):"
    m = re.match(r'^(?:repeat\s+while|while)\s+(.*?):$', trimmed, re.I)
    if m:
        return f"{indent}while {fix_expr(m.group(1))}:"
    m = re.match(r'^for\s+([a-zA-Z0-9_]+)\s+(?:from|in)\s+(.*?)\s+to\s+(.*?)(?:\s+by\s+(.*?))?:$', trimmed, re.I)
    if m:
        start_val = fix_expr(m.group(2))
        end_val = fix_expr(m.group(3))
        step_val = fix_expr(m.group(4)) if m.group(4) else "1"
        return f"{indent}for {m.group(1)} in range({start_val}, ({end_val}) + 1, {step_val}):"
    m = re.match(r'^for\s+(?:each|every|all)\s+([a-zA-Z0-9_]+)\s+in\s+(.*?):$', trimmed, re.I)
    if m:
        return f"{indent}for {m.group(1)} in {fix_expr(m.group(2))}:"
    m = re.match(r'^for\s+([a-zA-Z0-9_]+)\s+in\s+(.*?):$', trimmed, re.I)
    if m:
        return f"{indent}for {m.group(1)} in {fix_expr(m.group(2))}:"

    # 12. Function Definition
    m = re.match(r'^(?:define\s+function|function|routine|procedure|def|action)\s+([a-zA-Z0-9_]+)\s+with\s+(.*?):$', trimmed, re.I)
    if m:
        return f"{indent}def {m.group(1)}({m.group(2)}):"
    m = re.match(r'^(?:define\s+function|function|routine|procedure|def|action)\s+([a-zA-Z0-9_]+)\s*\((.*?)\)\s*:$', trimmed, re.I)
    if m:
        return f"{indent}def {m.group(1)}({m.group(2)}):"
    m = re.match(r'^(?:define\s+function|function|routine|procedure|def|action)\s+([a-zA-Z0-9_]+):$', trimmed, re.I)
    if m:
        return f"{indent}def {m.group(1)}():"

    # 13. Display / Show
    m = re.match(r'^(?:display|show|output|print)\s+(.*)$', trimmed, re.I)
    if m:
        body = m.group(1).strip()
        sep_arg = ""
        if re.search(r'\s+(?:without\s+spaces?|with\s+no\s+spaces?|joined)\s*$', body, re.I):
            sep_arg = ", sep=''"
            body = re.sub(r'\s+(?:without\s+spaces?|with\s+no\s+spaces?|joined)\s*$', '', body, flags=re.I).strip()
        elif re.search(r'\s+with\s+separator\s+(\".*?\"|\'.*?\')\s*$', body, re.I):
            _ms2 = re.search(r'\s+with\s+separator\s+(\".*?\"|\'.*?\')\s*$', body, re.I)
            sep_arg = f", sep={_ms2.group(1)}"
            body = body[:_ms2.start()].strip()
        
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
        res = _transpile_enlng_line(line)
        if '\n' in res:
            py_lines.extend(res.splitlines())
        else:
            py_lines.append(res)
    py_code = '\n'.join(py_lines)

    exec_globals = {
        '_smart_display': _smart_display,
        '_smart_input': _smart_input,
        '_cat': _cat,
        '_enlng_arrange': _enlng_arrange,
        '_enlng_parse_pos': _enlng_parse_pos,
        'string_add': string_add,
        'string_insert': string_insert,
        'string_remove_at': string_remove_at,
        'string_remove': string_remove,
        'string_replace': string_replace,
        'string_set_at': string_set_at,
        'true': True, 'false': False, 'null': None, 'undefined': None,
        'pi': math.pi, 'e': math.e,
        'sqrt': math.sqrt, 'floor': math.floor, 'ceil': math.ceil,
        'sleep': sleep, 'random_number': random_number,
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
