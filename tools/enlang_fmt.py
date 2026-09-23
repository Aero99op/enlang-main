#!/usr/bin/env python3
"""
tools/enlang_fmt.py - The Sovereign Enlang Formatter (enlangg fmt)

Enforces:
  1. Universal 4-space indentation hierarchy.
  2. Removal of redundant enclosing parentheses in conversational constructs.
  3. Consistent operator spacing (arithmetic, comparison, assignment, collections).
  4. Preservation of string literals, comments, and line layout.
  5. In-place formatting (--write / -w) and CI lint mode (--check).
"""

import sys
import os
import re
import difflib
import argparse

# Extensions recognized by the Sovereign Formatter
ENLANG_EXTENSIONS = (
    '.enlng', '.enlg', '.enlngs', '.enlgs', 
    '.enlngf', '.enlgf', '.enlngd', '.enlgd', 
    '.enlngdb', '.enlgdb', '.enlngm', '.enlgm'
)

def _mask_strings_and_comments(line):
    """
    Masks string literals and comments to protect them from spacing/paren rules.
    Returns: (masked_line, token_map)
    """
    tokens = {}
    counter = 0

    def store_token(val, kind):
        nonlocal counter
        key = f"__ENL_TOK_{counter}_{kind}__"
        tokens[key] = val
        counter += 1
        return key

    result = []
    i = 0
    n = len(line)
    in_str = False
    str_char = None
    str_start = 0

    while i < n:
        c = line[i]
        
        # Check for escape in string
        if in_str:
            if c == '\\' and i + 1 < n:
                i += 2
                continue
            if c == str_char:
                in_str = False
                token_val = line[str_start:i+1]
                key = store_token(token_val, "STR")
                result.append(key)
                i += 1
                continue
            i += 1
            continue

        # Check for string start
        if c in ('"', "'"):
            # Check triple quotes
            if line[i:i+3] == c * 3:
                # Triple quote
                tq = c * 3
                end_idx = line.find(tq, i + 3)
                if end_idx != -1:
                    token_val = line[i:end_idx+3]
                    key = store_token(token_val, "TSTR")
                    result.append(key)
                    i = end_idx + 3
                    continue
            in_str = True
            str_char = c
            str_start = i
            i += 1
            continue

        # Check for comment start
        if c == '#':
            comment_val = line[i:]
            key = store_token(comment_val, "COMMENT")
            result.append(key)
            break

        result.append(c)
        i += 1

    if in_str:
        # Unclosed string literal, restore
        token_val = line[str_start:]
        key = store_token(token_val, "STR")
        result.append(key)

    return "".join(result), tokens

def _unmask(text, tokens):
    """Restores masked strings and comments."""
    for key in reversed(list(tokens.keys())):
        text = text.replace(key, tokens[key])
    return text

def _strip_redundant_parens(line):
    """
    Strips redundant outer parentheses in statements like:
      when (x > 10): -> when x > 10:
      show (x)       -> show x
      give (val)     -> give val
      remember x as (100) -> remember x as 100
      (use "math")   -> use "math"
    """
    trimmed = line.strip()

    # Case 0: Entire statement wrapped in parens: (use "math") or (remember x as 10)
    if trimmed.startswith('(') and trimmed.endswith(')'):
        # Check if parentheses are matching outer
        depth = 0
        outer_match = True
        for idx, char in enumerate(trimmed):
            if char == '(': depth += 1
            elif char == ')':
                depth -= 1
                if depth == 0 and idx < len(trimmed) - 1:
                    outer_match = False
                    break
        if outer_match and depth == 0:
            inner = trimmed[1:-1].strip()
            # If inner contains valid statement starter, unwrap
            if any(inner.startswith(kw) for kw in ('use ', 'import ', 'remember ', 'freeze ', 'show ', 'display ', 'print ', 'add ', 'remove ')):
                line = line.replace(trimmed, inner)
                trimmed = inner

    # Case 1: Control flow headers: when (cond):, if (cond):, while (cond):, repeat while (cond):, repeat until (cond):, otherwise when (cond):
    ctrl_patterns = [
        r'^(when|if|while|repeat\s+while|repeat\s+until|until|otherwise\s+when|else\s+if|elif)\s+\((.+)\)\s*:$',
        r'^(for\s+each\s+[a-zA-Z0-9_]+\s+in)\s+\((.+)\)\s*:$',
    ]
    for pat in ctrl_patterns:
        m = re.match(pat, trimmed, re.I)
        if m:
            prefix = m.group(1)
            inner = m.group(2).strip()
            # Verify inner doesn't have unbalanced parens
            depth = 0
            balanced = True
            for ch in inner:
                if ch == '(': depth += 1
                elif ch == ')':
                    depth -= 1
                    if depth < 0: balanced = False; break
            if balanced and depth == 0:
                replaced = f"{prefix} {inner}:"
                line = line.replace(trimmed, replaced)
                trimmed = replaced
                break

    # Case 2: Output and return statements: show (expr), display (expr), print (expr), give (expr), return (expr)
    out_patterns = [
        r'^(show|display|output|print|give|return)\s+\((.+)\)$',
    ]
    for pat in out_patterns:
        m = re.match(pat, trimmed, re.I)
        if m:
            prefix = m.group(1)
            inner = m.group(2).strip()
            depth = 0
            balanced = True
            for ch in inner:
                if ch == '(': depth += 1
                elif ch == ')':
                    depth -= 1
                    if depth < 0: balanced = False; break
            if balanced and depth == 0 and not (',' in inner and prefix in ('show', 'display', 'print')):
                # Unwrap if not a comma-separated tuple
                replaced = f"{prefix} {inner}"
                line = line.replace(trimmed, replaced)
                trimmed = replaced
                break

    # Case 3: Declarations: remember x as (expr), freeze x as (expr)
    decl_pattern = r'^(remember|freeze)\s+([a-zA-Z0-9_]+)\s+as\s+\((.+)\)$'
    m = re.match(decl_pattern, trimmed, re.I)
    if m:
        kw, var, inner = m.group(1), m.group(2), m.group(3).strip()
        depth = 0
        balanced = True
        for ch in inner:
            if ch == '(': depth += 1
            elif ch == ')':
                depth -= 1
                if depth < 0: balanced = False; break
        if balanced and depth == 0:
            replaced = f"{kw} {var} as {inner}"
            line = line.replace(trimmed, replaced)

    return line

def _normalize_operator_spacing(line):
    """
    Standardizes spacing around operators and punctuation in masked lines.
    """
    # 1. Normalize colons at line ends (e.g. 'when x > 0 :' -> 'when x > 0:')
    line = re.sub(r'\s+:$', ':', line)

    # 2. Normalize commas: ensure space after comma unless followed by space or end
    line = re.sub(r',([^\s\),\]\}])', r', \1', line)

    # 3. Normalize colons in dicts or type annotations: 'key:val' -> 'key: val'
    # avoid double colons ::
    line = re.sub(r'(?<!:):([^\s:])', r': \1', line)

    # 4. Normalize comparison operators: ==, !=, <=, >=, <, >
    # Protect arrow ->
    line = re.sub(r'(?<![<>=!])([=!]=|<=|>=)(?![=])', r' \1 ', line)
    # Single < or > (not << or >> or -> or <-)
    line = re.sub(r'(?<![-<>=!])([<>])(?![=<>])', r' \1 ', line)

    # 5. Normalize assignment operators: =, +=, -=, *=, /=
    # Make sure not to double-pad ==, <=, >=, !=
    def fix_assign(m):
        op = m.group(1)
        return f" {op} "
    line = re.sub(r'(?<![<>=!+\-*/%])([+\-*/%]?=)(?![=])', fix_assign, line)

    # 6. Normalize arithmetic operators (+, -, *, /) when acting as binary
    # We must be careful not to space unary negative (e.g. -1, at -1, [-1])
    # Binary plus:
    line = re.sub(r'(?<=[a-zA-Z0-9_\)\]\}])\+(?=[a-zA-Z0-9_\(\[\{])', ' + ', line)
    # Binary multiply:
    line = re.sub(r'(?<=[a-zA-Z0-9_\)\]\}])\*(?=[a-zA-Z0-9_\(\[\{])', ' * ', line)
    # Binary divide:
    line = re.sub(r'(?<=[a-zA-Z0-9_\)\]\}])\/(?=[a-zA-Z0-9_\(\[\{])', ' / ', line)
    # Binary minus (ensure preceded by operand):
    line = re.sub(r'(?<=[a-zA-Z0-9_\)\]\}])-(?=[a-zA-Z0-9_\(\[\{])', ' - ', line)

    # 7. Collapse redundant multiple spaces (outside of leading indent)
    leading = len(line) - len(line.lstrip(' '))
    indent_prefix = line[:leading]
    rest = line[leading:]
    rest = re.sub(r'[ \t]{2,}', ' ', rest)

    # Clean up accidental double spaces around parens or brackets
    rest = re.sub(r'\(\s+', '(', rest)
    rest = re.sub(r'\s+\)', ')', rest)
    rest = re.sub(r'\[\s+', '[', rest)
    rest = re.sub(r'\s+\]', ']', rest)
    rest = re.sub(r'\{\s+', '{', rest)
    rest = re.sub(r'\s+\}', '}', rest)

    return indent_prefix + rest

def format_source(source_text: str) -> str:
    """
    Formats complete Enlang source code into canonical sovereign layout.
    """
    raw_lines = source_text.splitlines()
    if not raw_lines:
        return ""

    formatted_lines = []
    indent_stack = [0]  # Tracks original indentation column to logical depth
    current_depth = 0

    DEDENT_KEYWORDS = (
        'otherwise:', 'otherwise', 'else:', 'else', 
        'otherwise when', 'elif', 'else if'
    )

    for line in raw_lines:
        # Check blank line
        if not line.strip():
            formatted_lines.append("")
            continue

        # Extract leading indentation
        orig_indent = len(line) - len(line.lstrip(' \t'))
        content = line.strip()

        # Mask strings and comments
        masked, tokens = _mask_strings_and_comments(content)

        # Redundant parenthesis removal
        masked = _strip_redundant_parens(masked)

        # Operator spacing
        masked = _normalize_operator_spacing(masked)

        # Restore strings & comments
        cleaned_content = _unmask(masked, tokens)

        # Determine indentation depth
        is_dedent = False
        lower_content = cleaned_content.lower()
        for dkw in DEDENT_KEYWORDS:
            if lower_content == dkw or lower_content.startswith(dkw + ' '):
                is_dedent = True
                break

        # Compute stack depth based on original indent
        if orig_indent > indent_stack[-1]:
            indent_stack.append(orig_indent)
            current_depth = len(indent_stack) - 1
        else:
            while len(indent_stack) > 1 and orig_indent < indent_stack[-1]:
                indent_stack.pop()
            current_depth = len(indent_stack) - 1

        line_depth = current_depth

        formatted_indent = "    " * line_depth
        formatted_lines.append(f"{formatted_indent}{cleaned_content}")

        # If this statement opens a block (ends with ':'), the next line is indented
        if cleaned_content.endswith(':'):
            # If the current line didn't already trigger a stack push
            # next line will be at least current_depth + 1
            pass

    # Clean up trailing blank lines (keep exactly 1 trailing newline)
    while formatted_lines and formatted_lines[-1] == "":
        formatted_lines.pop()

    # Collapse more than 2 consecutive blank lines into 1
    collapsed = []
    blank_count = 0
    for l in formatted_lines:
        if l == "":
            blank_count += 1
            if blank_count <= 1:
                collapsed.append(l)
        else:
            blank_count = 0
            collapsed.append(l)

    return "\n".join(collapsed) + "\n"

def format_file(filepath: str, write: bool = False, check: bool = False, show_diff: bool = False) -> tuple:
    """
    Processes a single file.
    Returns: (is_formatted, diff_or_content)
    """
    try:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            original = f.read()
    except Exception as e:
        return (False, f"Error reading '{filepath}': {e}")

    formatted = format_source(original)
    is_same = (original == formatted)

    if check:
        return (is_same, "" if is_same else filepath)

    if write:
        if not is_same:
            with open(filepath, 'w', encoding='utf-8', newline='\n') as f:
                f.write(formatted)
        return (True, filepath)

    if show_diff:
        diff = "".join(difflib.unified_diff(
            original.splitlines(keepends=True),
            formatted.splitlines(keepends=True),
            fromfile=f"a/{os.path.basename(filepath)}",
            tofile=f"b/{os.path.basename(filepath)}"
        ))
        return (is_same, diff)

    return (is_same, formatted)

def collect_files(target_path: str):
    """Gathers all Enlang files recursively or single target."""
    if os.path.isfile(target_path):
        return [target_path]
    files = []
    for root, _, filenames in os.walk(target_path):
        for fn in filenames:
            if fn.endswith(ENLANG_EXTENSIONS):
                files.append(os.path.join(root, fn))
    return sorted(files)

def main():
    parser = argparse.ArgumentParser(
        prog="enlangg fmt",
        description="The Sovereign Universal Enlang Formatter"
    )
    parser.add_argument("path", nargs="?", default=".", help="File or directory to format (default: .)")
    parser.add_argument("-w", "--write", action="store_true", help="Format and overwrite files in-place")
    parser.add_argument("-c", "--check", action="store_true", help="Check if files are formatted without modifying")
    parser.add_argument("-d", "--diff", action="store_true", help="Display unified diff of format changes")

    args = parser.parse_args()

    files = collect_files(args.path)
    if not files:
        if os.path.exists(args.path):
            print(f"[ENLANGG FMT] No Enlang files found in '{args.path}'.")
            sys.exit(0)
        else:
            print(f"[ENLANGG FMT ERROR] Path not found: '{args.path}'", file=sys.stderr)
            sys.exit(1)

    # If single file and no write/check/diff specified, output directly to stdout
    if len(files) == 1 and not args.write and not args.check and not args.diff and os.path.isfile(args.path):
        _, formatted = format_file(files[0], write=False, check=False)
        sys.stdout.write(formatted)
        sys.exit(0)

    unformatted_count = 0
    formatted_count = 0

    print("==============================================================")
    print("       ENLANGG SOVEREIGN SOURCE CODE FORMATTER                ")
    print("==============================================================")

    for fpath in files:
        if args.check:
            is_same, _ = format_file(fpath, check=True)
            if not is_same:
                print(f"  [NEED FMT] {fpath}")
                unformatted_count += 1
            else:
                formatted_count += 1
        elif args.write:
            is_same, _ = format_file(fpath, check=True)
            if not is_same:
                format_file(fpath, write=True)
                print(f"  [REFORMATTED] {fpath}")
                unformatted_count += 1
            else:
                formatted_count += 1
        elif args.diff:
            is_same, diff = format_file(fpath, show_diff=True)
            if not is_same:
                print(f"\n--- Diff for {fpath} ---")
                print(diff)
                unformatted_count += 1
            else:
                formatted_count += 1

    print("--------------------------------------------------------------")
    if args.check:
        if unformatted_count > 0:
            print(f"[FAIL] {unformatted_count} file(s) need formatting. Run 'enlangg fmt -w' to format.")
            sys.exit(1)
        else:
            print(f"[SUCCESS] All {formatted_count} file(s) already in canonical format.")
            sys.exit(0)
    elif args.write:
        print(f"[SUCCESS] Processed {len(files)} file(s) ({unformatted_count} reformatted, {formatted_count} already clean).")
        sys.exit(0)
    elif args.diff:
        if unformatted_count > 0:
            sys.exit(1)
        sys.exit(0)

if __name__ == "__main__":
    main()
