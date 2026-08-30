"""enlg Lexical Analyzer.

Scans raw source code strings and emits structured Tokens.
Enforces indentation constraints and lexical boundaries.
Raises LexicalError for malformed token streams.
"""

import re
from typing import List
from .tokens import Token, TokenType
from enlg.diagnostics.diagnostics import LexicalError

# Token Regex patterns
REGEX_PATTERNS = [
    (TokenType.NUMBER, r'\d+(?:\.\d+)?'),
    (TokenType.STRING, r'"(?:\\.|[^"\\])*"'),
    (TokenType.IDENTIFIER, r'[a-zA-Z_][a-zA-Z0-9_\.]*'),
    (TokenType.SYMBOL, r'\*\*=|//=|<<=|>>=|\+=|-=|\*=|/=|%=|&=|\|=|\^=|==|!=|<=|>=|&&|\|\||\*\*|//|<<|>>|<|>|\+|-|\*|/|%|&|\||\^|~|!|=|,|:|\(|\)|\[|\]|\{|\}'),
]

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
            elif c == '#':
                return line[:i]
            elif c == '-' and i + 1 < len(line) and line[i+1] == '-':
                return line[:i]
        i += 1
    return line

class Lexer:
    """Tokenizes enlg source code."""
    
    def __init__(self, source: str):
        self.source = source
        self.tokens: List[Token] = []
        self.line_num = 1
        self.col_num = 1
        self.indent_stack = [0]
    
    def tokenize(self) -> List[Token]:
        lines = self.source.splitlines()
        for self.line_num, line in enumerate(lines, 1):
            line_no_comment = _strip_comment(line)
            if not line_no_comment.strip():
                continue
            
            # Handle Indentation
            indent_length = len(line_no_comment) - len(line_no_comment.lstrip())
            if indent_length > self.indent_stack[-1]:
                self.indent_stack.append(indent_length)
                self.tokens.append(Token(TokenType.INDENT, "", self.line_num, 1))
            elif indent_length < self.indent_stack[-1]:
                while self.indent_stack and indent_length < self.indent_stack[-1]:
                    self.indent_stack.pop()
                    self.tokens.append(Token(TokenType.DEDENT, "", self.line_num, 1))
                if indent_length != self.indent_stack[-1]:
                    raise LexicalError("E1002", f"Inconsistent dedent level on line {self.line_num}.")
            
            self.col_num = indent_length + 1
            raw_line = line_no_comment.lstrip()
            
            while raw_line:
                raw_line = raw_line.lstrip(' \t')
                if not raw_line:
                    break
                
                matched = False
                for token_type, pattern in REGEX_PATTERNS:
                    match = re.match(pattern, raw_line)
                    if match:
                        val = match.group(0)
                        if token_type == TokenType.STRING:
                            val = val[1:-1] # Strip quotes
                        self.tokens.append(Token(token_type, val, self.line_num, self.col_num))
                        raw_line = raw_line[len(match.group(0)):]
                        self.col_num += len(match.group(0))
                        matched = True
                        break
                
                if not matched:
                    # Capture unrecognized char
                    bad_char = raw_line[0]
                    raise LexicalError("E1001", f"Found '{bad_char}' at L{self.line_num}:C{self.col_num}")

            self.tokens.append(Token(TokenType.NEWLINE, "\n", self.line_num, self.col_num))
            
        # Unwind remaining indents at EOF
        while len(self.indent_stack) > 1:
            self.indent_stack.pop()
            self.tokens.append(Token(TokenType.DEDENT, "", self.line_num, self.col_num))
            
        self.tokens.append(Token(TokenType.EOF, "", self.line_num, self.col_num))
        return self.tokens
