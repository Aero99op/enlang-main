"""enlgm Lexer.

Tokenizes .enlgm source code into structured tokens.
Handles domain declaration ('in mobile:'), multi-word English hints,
silent connectors, operators, strings, numbers, raw Dart escapes, and indentation tracking.
"""

from typing import List, Optional
from .tokens import (
    Token, ENLGMTokenType, MOBILE_HINT_REGISTRY, CONNECTORS
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

OPERATOR_SYMBOLS = [
    "===", "!==", "==", "!=", ">=", "<=", "+=", "-=", "*=", "/=", "&&", "||",
    ">", "<", "=", "+", "-", "*", "/", "%", "!"
]

class ENLGMLexer:
    """Tokenizes .enlgm mobile source strings into structured tokens."""

    def __init__(self, source: str):
        self.source = source
        self.tokens: List[Token] = []
        self.line_num = 1
        self.indent_stack = [0]

    def tokenize(self) -> List[Token]:
        lines = self.source.splitlines()
        sorted_hints = sorted(list(MOBILE_HINT_REGISTRY.keys()), key=len, reverse=True)

        for self.line_num, line in enumerate(lines, 1):
            line_no_comment = _strip_comment(line)
            stripped_line = line_no_comment.strip()
            if not stripped_line:
                continue

            # Support standard domain type header (e.g. 'type enlgm')
            if stripped_line.lower().startswith("type ") and stripped_line.lower().split()[1].rstrip(":") in ("enlgm", "mobile", "flutter"):
                self.tokens.append(Token(ENLGMTokenType.HINT, "TYPE_HEADER", self.line_num, 1, raw_text=stripped_line))
                continue

            # Compute indentation
            indent_length = len(line_no_comment) - len(line_no_comment.lstrip())

            if indent_length > self.indent_stack[-1]:
                self.indent_stack.append(indent_length)
                self.tokens.append(Token(ENLGMTokenType.INDENT, "", self.line_num, 1, raw_text=stripped_line))
            elif indent_length < self.indent_stack[-1]:
                while self.indent_stack and indent_length < self.indent_stack[-1]:
                    self.indent_stack.pop()
                    self.tokens.append(Token(ENLGMTokenType.DEDENT, "", self.line_num, 1, raw_text=stripped_line))

            raw_line = line_no_comment.lstrip()
            self._tokenize_line(raw_line, stripped_line, sorted_hints)
            self.tokens.append(Token(ENLGMTokenType.NEWLINE, "\n", self.line_num, len(line), raw_text=stripped_line))

        # Unwind indents at EOF
        while len(self.indent_stack) > 1:
            self.indent_stack.pop()
            self.tokens.append(Token(ENLGMTokenType.DEDENT, "", self.line_num, 1, raw_text=""))

        self.tokens.append(Token(ENLGMTokenType.EOF, "", self.line_num, 1, raw_text=""))
        try:
            self.tokens.has_type_header = self.has_type_header
        except AttributeError:
            pass
        return self.tokens

    def _tokenize_line(self, line: str, original_stripped: str, sorted_hints: List[str]):
        i = 0
        length = len(line)

        while i < length:
            # Skip whitespace
            if line[i] in (' ', '\t'):
                i += 1
                continue

            # Structural symbols
            if line[i] in (':', ',', ';', '(', ')', '{', '}', '[', ']'):
                self.tokens.append(Token(ENLGMTokenType.SYMBOL, line[i], self.line_num, i + 1, raw_text=original_stripped))
                i += 1
                continue

            # String literals
            if line[i] in ('"', "'"):
                quote = line[i]
                start_col = i + 1
                i += 1
                str_chars = []
                while i < length:
                    if line[i] == '\\' and i + 1 < length:
                        str_chars.append(line[i:i+2])
                        i += 2
                    elif line[i] == quote:
                        i += 1
                        break
                    else:
                        str_chars.append(line[i])
                        i += 1
                str_val = "".join(str_chars)
                self.tokens.append(Token(ENLGMTokenType.STRING, str_val, self.line_num, start_col, raw_text=original_stripped))
                continue

            # Multi-character or single-character operators
            matched_op = None
            for op in OPERATOR_SYMBOLS:
                if line[i:i+len(op)] == op:
                    matched_op = op
                    break
            if matched_op:
                self.tokens.append(Token(ENLGMTokenType.OPERATOR, matched_op, self.line_num, i + 1, raw_text=original_stripped))
                i += len(matched_op)
                continue

            # Numbers (ints and floats, including negative)
            if line[i].isdigit() or (line[i] == '.' and i + 1 < length and line[i+1].isdigit()):
                start_col = i + 1
                num_chars = []
                while i < length and (line[i].isdigit() or line[i] == '.'):
                    num_chars.append(line[i])
                    i += 1
                num_val = "".join(num_chars)
                self.tokens.append(Token(ENLGMTokenType.NUMBER, num_val, self.line_num, start_col, raw_text=original_stripped))
                continue

            # Hint Phrases (longest match first)
            matched_hint = None
            for hint_phrase in sorted_hints:
                h_len = len(hint_phrase)
                if line[i:i+h_len].lower() == hint_phrase:
                    end_idx = i + h_len
                    # Word boundary check
                    if end_idx >= length or line[end_idx] in (' ', '\t', ':', ',', ';', '(', ')', '{', '}', '[', ']'):
                        matched_hint = hint_phrase
                        break

            if matched_hint:
                intent_id = MOBILE_HINT_REGISTRY[matched_hint]
                self.tokens.append(Token(ENLGMTokenType.HINT, intent_id, self.line_num, i + 1, raw_text=matched_hint))
                i += len(matched_hint)
                continue

            # Identifiers and words
            start_col = i + 1
            word_chars = []
            while i < length and not (line[i] in (' ', '\t', ':', ',', ';', '(', ')', '{', '}', '[', ']') or any(line[i:i+len(op)] == op for op in OPERATOR_SYMBOLS)):
                word_chars.append(line[i])
                i += 1
            word = "".join(word_chars)

            if word.lower() in CONNECTORS:
                self.tokens.append(Token(ENLGMTokenType.CONNECTOR, word, self.line_num, start_col, raw_text=word))
            else:
                self.tokens.append(Token(ENLGMTokenType.IDENTIFIER, word, self.line_num, start_col, raw_text=word))
