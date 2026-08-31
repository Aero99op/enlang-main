"""enlgd Lexer.

Tokenizes .enlgd stylesheet source code into structured Tokens.
Handles string literals, multi-word property phrases, connector words, indentation blocks, and numeric values with units.
"""

from typing import List, Optional
from .tokens import Token, TokenType, PROPERTY_MAPPINGS, STATE_MAPPINGS, CONNECTORS, KEYWORDS

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

class ENLGDLexer:
    """Tokenizes .enlgd source code."""

    def __init__(self, source: str):
        self.source = source
        self.tokens: List[Token] = []
        self.line_num = 1
        self.indent_stack = [0]

    def tokenize(self) -> List[Token]:
        lines = self.source.splitlines()

        for self.line_num, line in enumerate(lines, 1):
            line_no_comment = _strip_comment(line)
            if not line_no_comment.strip():
                continue

            raw_line = line_no_comment.lstrip()
            # Support and skip standard domain type header (e.g. 'type enlgd')
            if raw_line.lower().startswith("type ") and raw_line.lower().split()[1].rstrip(":") in ("enlgd", "design", "css", "styles", "style"):
                continue

            # Compute indentation
            indent_length = len(line_no_comment) - len(line_no_comment.lstrip())

            if indent_length > self.indent_stack[-1]:
                self.indent_stack.append(indent_length)
                self.tokens.append(Token(TokenType.INDENT, "", self.line_num, 1))
            elif indent_length < self.indent_stack[-1]:
                while self.indent_stack and indent_length < self.indent_stack[-1]:
                    self.indent_stack.pop()
                    self.tokens.append(Token(TokenType.DEDENT, "", self.line_num, 1))

            raw_line = line_no_comment.lstrip()
            self._tokenize_line(raw_line)
            self.tokens.append(Token(TokenType.NEWLINE, "\n", self.line_num, len(line)))

        # Unwind remaining indents at EOF
        while len(self.indent_stack) > 1:
            self.indent_stack.pop()
            self.tokens.append(Token(TokenType.DEDENT, "", self.line_num, 1))

        self.tokens.append(Token(TokenType.EOF, "", self.line_num, 1))
        return self.tokens

    def _tokenize_line(self, line: str):
        i = 0
        length = len(line)

        # Pre-sorted property phrases by length (longest match first)
        sorted_prop_phrases = sorted(list(PROPERTY_MAPPINGS.keys()), key=len, reverse=True)
        sorted_state_phrases = sorted(list(STATE_MAPPINGS.keys()), key=len, reverse=True)

        while i < length:
            # Skip whitespace
            if line[i] in (' ', '\t'):
                i += 1
                continue

            # Symbols
            if line[i] in (':', ',', ';'):
                self.tokens.append(Token(TokenType.SYMBOL, line[i], self.line_num, i + 1))
                i += 1
                continue

            # Quoted String literals
            if line[i] in ('"', "'"):
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
                self.tokens.append(Token(TokenType.STRING, val, self.line_num, start + 1))
                continue

            # Hex colors (e.g. #1e3c72, #fff)
            if line[i] == '#':
                start = i
                i += 1
                while i < length and (line[i].isalnum() or line[i] in ('_', '-')):
                    i += 1
                self.tokens.append(Token(TokenType.STRING, line[start:i], self.line_num, start + 1))
                continue

            remainder = line[i:].lower()

            # Check Property mappings (multi-word like 'font size', 'border radius')
            matched_prop = None
            for phrase in sorted_prop_phrases:
                if remainder.startswith(phrase):
                    phrase_len = len(phrase)
                    if phrase_len == len(remainder) or not remainder[phrase_len].isalnum() and remainder[phrase_len] not in ('-', '_'):
                        matched_prop = phrase
                        canonical_name = PROPERTY_MAPPINGS[phrase]
                        self.tokens.append(Token(TokenType.PROPERTY, canonical_name, self.line_num, i + 1))
                        i += phrase_len
                        break
            if matched_prop:
                continue

            # Check State mappings (hovered, first child, etc.)
            matched_state = None
            for phrase in sorted_state_phrases:
                if remainder.startswith(phrase):
                    phrase_len = len(phrase)
                    if phrase_len == len(remainder) or not remainder[phrase_len].isalnum() and remainder[phrase_len] not in ('-', '_'):
                        matched_state = phrase
                        canonical_state = STATE_MAPPINGS[phrase]
                        self.tokens.append(Token(TokenType.IDENTIFIER, canonical_state, self.line_num, i + 1))
                        i += phrase_len
                        break
            if matched_state:
                continue

            # Numbers (including decimals, units like px, em, rem, s, ms, %, vh, vw, deg)
            if line[i].isdigit() or (line[i] in ('-', '+', '.') and i + 1 < length and (line[i+1].isdigit() or line[i+1] == '.')):
                start = i
                if line[i] in ('-', '+'):
                    i += 1
                while i < length and (line[i].isdigit() or line[i] == '.'):
                    i += 1
                
                # Check for unit suffix immediately following number (e.g. 16px, 0.3s, 100vh, 100%, 135deg)
                unit_start = i
                while i < length and (line[i].isalpha() or line[i] in ('%', '-')):
                    i += 1
                
                raw_val = line[start:i]
                self.tokens.append(Token(TokenType.NUMBER, raw_val, self.line_num, start + 1))
                continue

            # Identifiers / Keywords / Connectors / CSS Values
            if line[i].isalpha() or line[i] in ('_', '-', '@', '.'):
                start = i
                # Read word or CSS function like rgba(...)
                in_parens = 0
                while i < length:
                    c = line[i]
                    if c == '(':
                        in_parens += 1
                    elif c == ')':
                        if in_parens > 0:
                            in_parens -= 1
                    elif in_parens == 0 and (c in (' ', '\t', ':', ',', ';', '"', "'")):
                        break
                    i += 1

                val = line[start:i]
                val_lower = val.lower()

                if val_lower in CONNECTORS:
                    self.tokens.append(Token(TokenType.CONNECTOR, val_lower, self.line_num, start + 1))
                elif val_lower in KEYWORDS:
                    self.tokens.append(Token(TokenType.KEYWORD, val_lower, self.line_num, start + 1))
                else:
                    self.tokens.append(Token(TokenType.IDENTIFIER, val, self.line_num, start + 1))
                continue

            # Fallback single symbol
            self.tokens.append(Token(TokenType.SYMBOL, line[i], self.line_num, i + 1))
            i += 1
