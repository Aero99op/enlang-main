"""enlg Unified Human-Friendly Diagnostic Error Formatter.

Formats compiler, parser, and runtime errors across all 6 Enlang domains:
- Core (.enlng / .enlg)
- Database (.enlngdb / .enlgdb)
- Design Tokens (.enlngd / .enlgd)
- Frontend Markup (.enlngf / .enlgf)
- Reactive Scripts (.enlngs / .enlgs)
- Mobile Applications (.enlngm / .enlgm)

Provides:
1. Exact File Location (file:line:col)
2. Contextual Code Snippet with '>>>' and caret '^^^^' highlighting
3. Detailed 'What' and 'Why' cognitive explanation
4. Actionable 'Suggestions' (Did you mean...?)
"""

import os
import re
from typing import List, Optional, Tuple, Dict, Any

# Common Enlang keywords for fuzzy matching suggestions
CORE_KEYWORDS = [
    "remember", "freeze", "change", "increase", "decrease", "when", "if",
    "otherwise", "while", "repeat", "for", "each", "in", "from", "to", "by",
    "function", "define", "give", "return", "display", "show", "ask", "call",
    "attempt", "rescue", "catch", "finally", "throw", "raise", "pass", "break",
    "continue", "swap", "pair", "type", "hint", "use", "import"
]

DATABASE_KEYWORDS = [
    "create", "table", "with", "insert", "record", "records", "into", "values",
    "find", "show", "select", "fetch", "from", "in", "where", "order", "by",
    "ascending", "descending", "count", "update", "change", "delete", "remove",
    "database", "use", "open", "save", "drop", "column"
]


def _find_closest_keyword(word: str, candidates: List[str]) -> Optional[str]:
    """Finds the closest keyword by simple edit distance for typos."""
    w = word.lower()
    best_match = None
    min_dist = 999
    for cand in candidates:
        # Check simple prefix or substring or distance
        d = _levenshtein(w, cand)
        if d < min_dist and d <= 2 and len(w) >= 3:
            min_dist = d
            best_match = cand
    return best_match


def _levenshtein(s1: str, s2: str) -> int:
    if len(s1) < len(s2):
        return _levenshtein(s2, s1)
    if len(s2) == 0:
        return len(s1)
    previous_row = range(len(s2) + 1)
    for i, c1 in enumerate(s1):
        current_row = [i + 1]
        for j, c2 in enumerate(s2):
            insertions = previous_row[j + 1] + 1
            deletions = current_row[j] + 1
            substitutions = previous_row[j] + (c1 != c2)
            current_row.append(min(insertions, deletions, substitutions))
        previous_row = current_row
    return previous_row[-1]


def analyze_error(source: str, line_num: int, col_num: int, raw_msg: str, domain: str = "enlng") -> Tuple[str, str, List[str]]:
    """Analyzes the faulty line and context to generate What, Why, and Suggestions."""
    lines = source.splitlines() if source else []
    line_idx = line_num - 1 if line_num and 1 <= line_num <= len(lines) else -1
    line_text = lines[line_idx] if line_idx >= 0 else ""
    trimmed = line_text.strip()
    low = trimmed.lower()

    what = ""
    why = ""
    suggestions: List[str] = []

    # 1. Domain: Database (.enlngdb / .enlgdb)
    if domain in ("enlngdb", "enlgdb") or "database" in raw_msg.lower() or low.startswith("type enlngdb"):
        if low.startswith("create table") and (low.endswith("with") or "with" not in low):
            what = "Table creation statement is missing column definitions."
            why = "EnlngDB requires at least one column definition following the 'with' keyword."
            suggestions.append("Define table columns, e.g.: create table users with id, name, score")
            return what, why, suggestions

        if (low.startswith("find") or low.startswith("select") or low.startswith("show")) and not any(k in low for k in [" from ", " in "]):
            what = "Missing source table in query statement."
            why = "Query statements need a target table specified via 'from <table>' or 'in <table>'."
            suggestions.append("Add the target table, e.g.: find records from users")
            suggestions.append("Or specify projected columns: find user_id of users from users")
            return what, why, suggestions

        if "does not exist" in raw_msg.lower() or "not found" in raw_msg.lower():
            what = raw_msg
            why = "The referenced table or database entity has not been created or opened in the current session."
            suggestions.append("Verify the table name spelling or create it first: create table <name> with <cols>")
            return what, why, suggestions

    # 2. Domain: Core (.enlng / .enlg)
    # Check A: Single '=' used inside conditional expression
    cond_match = re.match(r'^(?:if|when|while|repeat\s+while)\s+(.*?)(?::|$)', trimmed, re.I)
    if cond_match:
        expr = cond_match.group(1)
        if re.search(r'(?<![=<>!])=(?![=])', expr):
            what = "Invalid assignment operator '=' found in conditional expression."
            why = "In Enlang, '=' is reserved for assigning values. Conditions require a comparison operator like '==' or 'is'."
            fixed_expr = re.sub(r'(?<![=<>!])=(?![=])', '==', expr)
            suggestions.append(f"Replace '=' with '==' or 'is':\n      --> {cond_match.group(0).split()[0]} {fixed_expr}:")
            suggestions.append("To check inequality, use '!=' or 'is not'.")
            return what, why, suggestions

    # Check B: Missing trailing colon on block statements
    block_start_match = re.match(r'^(?:if|when|else\s+if|otherwise\s+if|elif|else|otherwise|while|repeat\s+while|for|function|define\s+function|def|class|blueprint|model|attempt|try|catch|rescue|finally)\b(.*?)$', trimmed, re.I)
    if block_start_match and not trimmed.endswith(":"):
        what = f"Missing colon ':' at the end of block statement."
        why = "Enlang block headers (if, when, loop, function, class) must terminate with a colon ':' to open an indented suite."
        suggestions.append(f"Add a colon ':' at the end of the line:\n      --> {trimmed}:")
        return what, why, suggestions

    # Check C: Missing 'as' or '=' in variable declaration
    decl_match = re.match(r'^(?:remember|freeze|let|declare|create)\s+([a-zA-Z0-9_]+)\s*([^:=a-zA-Z0-9_]|$)', trimmed, re.I)
    if decl_match and " as " not in low and " = " not in low and not low.endswith("="):
        var_name = decl_match.group(1)
        what = f"Incomplete variable declaration for '{var_name}'."
        why = "Variable declarations require 'as' or '=' to bind the initial value."
        suggestions.append(f"Bind a value using 'as' or '=', e.g.: remember {var_name} as 100")
        return what, why, suggestions

    # Check D: Misspelled keyword (e.g. 'shwo' -> 'show', 'rember' -> 'remember')
    tokens = trimmed.split()
    if tokens:
        first_word = re.sub(r'[^a-zA-Z0-9_]', '', tokens[0])
        cand = _find_closest_keyword(first_word, CORE_KEYWORDS)
        if cand and cand != first_word.lower():
            what = f"Unknown keyword or command '{first_word}'."
            why = f"'{first_word}' is not recognized as a valid Enlang command or statement."
            suggestions.append(f"Did you mean '{cand}'?\n      --> {cand} {' '.join(tokens[1:])}")
            return what, why, suggestions

    # Check E: Unmatched parentheses / brackets / braces
    open_parens = trimmed.count("(") - trimmed.count(")")
    open_brackets = trimmed.count("[") - trimmed.count("]")
    open_braces = trimmed.count("{") - trimmed.count("}")
    if open_parens > 0:
        what = "Unmatched opening parenthesis '('."
        why = f"Found {open_parens} unclosed '(' parenthesis in this statement."
        suggestions.append("Close all open parentheses with matching ')' before the end of the line.")
        return what, why, suggestions
    if open_brackets > 0:
        what = "Unmatched opening square bracket '['."
        why = f"Found {open_brackets} unclosed '[' bracket in list or index expression."
        suggestions.append("Ensure every list or indexing expression closes with ']'.")
        return what, why, suggestions
    if open_braces > 0:
        what = "Unmatched opening curly brace '{'."
        why = f"Found {open_braces} unclosed '{{' brace in map/dictionary expression."
        suggestions.append("Ensure every map literal closes with '}'.")
        return what, why, suggestions

    # Check F: Unclosed string literal
    quotes_single = trimmed.count("'") % 2 != 0
    quotes_double = trimmed.count('"') % 2 != 0
    if quotes_single or quotes_double:
        quote_char = "'" if quotes_single else '"'
        what = f"Unclosed string literal starting with {quote_char}."
        why = "A string was started but not terminated before the end of the line."
        suggestions.append(f"Add a closing {quote_char} at the end of the string.")
        return what, why, suggestions

    # 3. Fallback generic parsing from raw message
    what = raw_msg or "Syntax or evaluation error in statement."
    why = f"The compiler encountered an unexpected token or malformed construct during {domain} evaluation."
    if "indent" in raw_msg.lower():
        what = "Indentation error."
        why = "Enlang blocks use 4 spaces for indentation. A mismatched indent or dedent was detected."
        suggestions.append("Ensure consistent 4-space indentation across all nested blocks.")
    elif "expected" in raw_msg.lower():
        suggestions.append(f"Verify syntax requirements: {raw_msg}")
    else:
        suggestions.append("Check line syntax against standard Enlang conventions.")

    return what, why, suggestions


DOMAIN_NAMES = {
    "enlng": "Enlang",
    "enlg": "Enlang",
    "enlngdb": "EnlangDB",
    "enlgdb": "EnlangDB",
    "enlngd": "Enlang Design",
    "enlgd": "Enlang Design",
    "enlngf": "Enlang UI",
    "enlgf": "Enlang UI",
    "enlngs": "Enlang Script",
    "enlgs": "Enlang Script",
    "enlngm": "Enlang Mobile",
    "enlgm": "Enlang Mobile",
}


def _can_encode_unicode() -> bool:
    try:
        import sys
        if hasattr(sys.stderr, 'reconfigure'):
            sys.stderr.reconfigure(encoding='utf-8', errors='replace')
        if hasattr(sys.stdout, 'reconfigure'):
            sys.stdout.reconfigure(encoding='utf-8', errors='replace')
        encoding = sys.stderr.encoding or sys.stdout.encoding or "utf-8"
        "── •".encode(encoding)
        return True
    except Exception:
        return False


def format_human_diagnostic(
    source: str = "",
    line: Optional[int] = None,
    col: Optional[int] = None,
    file_path: Optional[str] = None,
    domain: str = "enlng",
    error_type: str = "SyntaxError",
    what: Optional[str] = None,
    why: Optional[str] = None,
    suggestions: Optional[List[str]] = None,
    raw_error: Optional[str] = None
) -> str:
    """Renders a clean, modern human-friendly diagnostic message."""
    use_unicode = _can_encode_unicode()
    bar = "─" if use_unicode else "-"
    bullet = "•" if use_unicode else "*"

    clean_dom = domain.lower().replace(".", "").strip()
    domain_name = DOMAIN_NAMES.get(clean_dom, f"Enlang {clean_dom.capitalize()}")
    
    clean_type = error_type.replace("Error", "") + "Error" if not error_type.endswith("Error") else error_type
    
    box_width = 72
    title = f"{domain_name} {clean_type}"
    top_line = f"{bar * 2} {title} "
    top_line += bar * max(4, box_width - len(top_line))

    lines_out: List[str] = ["", top_line]

    # 1. Location line
    file_display = file_path or "source"
    line_display = line if line is not None and line > 0 else 1
    col_display = col if col is not None and col > 0 else 1

    auto_what, auto_why, auto_suggs = analyze_error(
        source, line_display, col_display, raw_error or what or "Syntax error", domain
    )
    
    # Check if faulty line has '=' in condition to align caret
    source_lines = source.splitlines() if source else []
    if source_lines and 1 <= line_display <= len(source_lines):
        faulty_line = source_lines[line_display - 1]
        cond_match = re.match(r'^\s*(?:if|when|while|repeat\s+while)\s+(.*?)(?::|$)', faulty_line, re.I)
        if cond_match and re.search(r'(?<![=<>!])=(?![=])', cond_match.group(1)):
            eq_pos = faulty_line.find("=")
            if eq_pos >= 0:
                col_display = eq_pos + 1

    lines_out.append(f"Location: {file_display}:{line_display}:{col_display}")
    lines_out.append("")

    # 2. Code Snippet Preview with Gutter & Caret
    if source_lines and line_display <= len(source_lines):
        start_idx = max(0, line_display - 2)
        end_idx = min(len(source_lines), line_display + 1)

        for idx in range(start_idx, end_idx):
            cur_line_num = idx + 1
            line_str = source_lines[idx]
            lines_out.append(f"  {cur_line_num:3d} | {line_str}")
            if cur_line_num == line_display:
                pointer_col = max(1, col_display)
                pointer_indent = " " * (pointer_col - 1)
                lines_out.append(f"      | {pointer_indent}^")
        lines_out.append("")
    elif source_lines and line_display > len(source_lines):
        last_line_num = len(source_lines)
        lines_out.append(f"  {last_line_num:3d} | {source_lines[-1]}")
        lines_out.append(f"      | {' ' * len(source_lines[-1])}^")
        lines_out.append("")

    # 3. What and Why
    final_what = what or auto_what
    final_why = why or auto_why
    final_suggestions = suggestions if suggestions is not None else auto_suggs

    lines_out.append(f"What: {final_what}")
    lines_out.append(f"Why:  {final_why}")
    lines_out.append("")

    # 4. Suggestions
    lines_out.append("Suggestions:")
    if final_suggestions:
        for s in final_suggestions:
            lines_out.append(f"  {bullet} {s}")
    else:
        lines_out.append(f"  {bullet} Check syntax against standard Enlang conventions.")

    lines_out.append(bar * box_width)
    lines_out.append("")
    return "\n".join(lines_out)
