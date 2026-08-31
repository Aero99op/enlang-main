"""enlgs Lexer.

Tokenizes .enlgs source code into structured tokens.
Handles domain declaration ('in script:'), multi-word hints, comparison phrases,
silent connectors, operators, strings, numbers, and indentation tracking.
"""

from typing import List, Optional
from .tokens import (
    Token, TokenType, HINT_REGISTRY, CONNECTORS, COMPARISON_PHRASES
)

def _strip_comment(line: str) -> str:
    in_quote = False
    quote_char = None
    i = 0
    while i < len(line):
        c = line[i]
        if in_quote:
            if c == quote_char:
                in_quote = False
            elif c == '\\':
                i += 1
        else:
            if c in ('"', "'"):
                in_quote = True
                quote_char = c
            elif c == '#' or (c == '/' and i + 1 < len(line) and line[i+1] == '/'):
                return line[:i]
        i += 1
    return line

# Multi-character symbolic operators
OPERATOR_SYMBOLS = [
    "===", "!==", "==", "!=", ">=", "<=", "+=", "-=", "*=", "/=", "&&", "||", "**",
    ">", "<", "=", "+", "-", "*", "/", "%", "!"
]

class ENLGSLexer:
    """Tokenizes .enlgs source strings."""

    def __init__(self, source: str):
        self.source = source
        self.tokens: List[Token] = []
        self.line_num = 1
        self.indent_stack = [0]

    def tokenize(self) -> List[Token]:
        lines = self.source.splitlines()

        # Sort hint and comparison phrases by length (longest match first)
        sorted_hints = sorted(list(HINT_REGISTRY.keys()), key=len, reverse=True)
        sorted_comparisons = sorted(list(COMPARISON_PHRASES.keys()), key=len, reverse=True)

        for self.line_num, line in enumerate(lines, 1):
            line_no_comment = _strip_comment(line)
            stripped_line = line_no_comment.strip()
            if not stripped_line:
                continue

            # Support and skip standard domain type header (e.g. 'type enlgs')
            if stripped_line.lower().startswith("type ") and stripped_line.lower().split()[1].rstrip(":") in ("enlgs", "script", "js"):
                continue

            # Compute indentation
            indent_length = len(line_no_comment) - len(line_no_comment.lstrip())

            if indent_length > self.indent_stack[-1]:
                self.indent_stack.append(indent_length)
                self.tokens.append(Token(TokenType.INDENT, "", self.line_num, 1, raw_text=stripped_line))
            elif indent_length < self.indent_stack[-1]:
                while self.indent_stack and indent_length < self.indent_stack[-1]:
                    self.indent_stack.pop()
                    self.tokens.append(Token(TokenType.DEDENT, "", self.line_num, 1, raw_text=stripped_line))

            raw_line = line_no_comment.lstrip()
            self._tokenize_line(raw_line, stripped_line, sorted_hints, sorted_comparisons)
            self.tokens.append(Token(TokenType.NEWLINE, "\n", self.line_num, len(line), raw_text=stripped_line))

        # Unwind remaining indents at EOF
        while len(self.indent_stack) > 1:
            self.indent_stack.pop()
            self.tokens.append(Token(TokenType.DEDENT, "", self.line_num, 1))

        self.tokens.append(Token(TokenType.EOF, "", self.line_num, 1))
        return self.tokens

    def _tokenize_line(self, line: str, original_stripped: str, sorted_hints: List[str], sorted_comparisons: List[str]):
        i = 0
        length = len(line)

        while i < length:
            # Skip whitespace
            if line[i] in (' ', '\t'):
                i += 1
                continue

            # Structural symbols
            if line[i] in (':', ',', ';', '(', ')', '{', '}', '[', ']'):
                self.tokens.append(Token(TokenType.SYMBOL, line[i], self.line_num, i + 1, raw_text=original_stripped))
                i += 1
                continue

            # String literals
            if line[i] in ('"', "'", '`'):
                quote_char = line[i]
                start = i
                i += 1
                val = ""
                while i < length and line[i] != quote_char:
                    if line[i] == '\\' and i + 1 < length:
                        i += 1
                        val += line[i]
                    else:
                        val += line[i]
                    i += 1
                if i < length and line[i] == quote_char:
                    i += 1
                self.tokens.append(Token(TokenType.STRING, val, self.line_num, start + 1, raw_text=original_stripped))
                continue

            remainder = line[i:].lower()

            # 1. Check Multi-Word / Single-Word Hints from HINT_REGISTRY
            matched_hint = None
            for hint in sorted_hints:
                if remainder.startswith(hint):
                    h_len = len(hint)
                    if h_len == len(remainder) or not remainder[h_len].isalnum() and remainder[h_len] not in ('_', '-'):
                        matched_hint = hint
                        self.tokens.append(Token(TokenType.HINT, hint, self.line_num, i + 1, raw_text=original_stripped))
                        i += h_len
                        break
            if matched_hint:
                continue

            # 2. Check Comparison Phrases (is greater than, is equal to, etc.)
            matched_comp = None
            for comp in sorted_comparisons:
                if remainder.startswith(comp):
                    c_len = len(comp)
                    if c_len == len(remainder) or not remainder[c_len].isalnum() and remainder[c_len] not in ('_', '-'):
                        matched_comp = comp
                        op_symbol = COMPARISON_PHRASES[comp]
                        self.tokens.append(Token(TokenType.OPERATOR, op_symbol, self.line_num, i + 1, raw_text=original_stripped))
                        i += c_len
                        break
            if matched_comp:
                continue

            # 3. Check Multi-character & Single-character Operators
            matched_op = None
            for op in OPERATOR_SYMBOLS:
                if line[i:].startswith(op):
                    self.tokens.append(Token(TokenType.OPERATOR, op, self.line_num, i + 1, raw_text=original_stripped))
                    i += len(op)
                    matched_op = True
                    break
            if matched_op:
                continue

            # 4. Numbers (with optional units: 10, 3.14, 100ms, 5s)
            if line[i].isdigit():
                start = i
                while i < length and (line[i].isdigit() or line[i] == '.'):
                    i += 1
                # Check for unit suffix
                unit_start = i
                while i < length and (line[i].isalpha() or line[i] == '%'):
                    i += 1
                num_val = line[start:i]
                self.tokens.append(Token(TokenType.NUMBER, num_val, self.line_num, start + 1, raw_text=original_stripped))
                continue

            # 5. Identifiers, Connectors, or raw keywords
            if line[i].isalpha() or line[i] in ('_', '$', '.'):
                start = i
                while i < length and (line[i].isalnum() or line[i] in ('_', '$', '.')):
                    i += 1
                val = line[start:i]
                val_lower = val.lower()

                if val_lower in CONNECTORS:
                    self.tokens.append(Token(TokenType.CONNECTOR, val_lower, self.line_num, start + 1, raw_text=original_stripped))
                else:
                    self.tokens.append(Token(TokenType.IDENTIFIER, val, self.line_num, start + 1, raw_text=original_stripped))
                continue

            # Fallback single character
            self.tokens.append(Token(TokenType.SYMBOL, line[i], self.line_num, i + 1, raw_text=original_stripped))
            i += 1
