"""Lexer for enlgdb (Natural English SQL & Database Language)."""

import re
from typing import List
from enlgdb.tokens import Token, TokenType


class LexerError(Exception):
    def __init__(self, message: str, line: int, column: int, hint: str = ""):
        self.message = message
        self.line = line
        self.column = column
        self.hint = hint
        hint_text = f"\n  💡 Hint: {hint}" if hint else ""
        super().__init__(f"Lexer error at L{line}:C{column}: {message}{hint_text}")


class Lexer:
    KEYWORDS = {
        # Domain Header
        "type": TokenType.TYPE,
        "enlgdb": TokenType.ENLGDB,
        "enlngdb": TokenType.ENLGDB,

        # DDL
        "create": TokenType.CREATE,
        "table": TokenType.TABLE,
        "tables": TokenType.TABLES,
        "database": TokenType.DATABASE,
        "databases": TokenType.DATABASES,
        "use": TokenType.USE,
        "show": TokenType.SHOW,
        "with": TokenType.WITH,
        "alter": TokenType.ALTER,
        "add": TokenType.ADD,
        "drop": TokenType.DROP,
        "truncate": TokenType.TRUNCATE,
        "column": TokenType.COLUMN,
        "index": TokenType.INDEX,

        # Safety Guard
        "confirm": TokenType.CONFIRM,
        "confirmed": TokenType.CONFIRMED,

        # DML / DQL
        "insert": TokenType.INSERT,
        "into": TokenType.INTO,
        "values": TokenType.VALUES,
        "select": TokenType.SELECT,
        "all": TokenType.ALL,
        "from": TokenType.FROM,
        "where": TokenType.WHERE,
        "order": TokenType.ORDER,
        "by": TokenType.BY,
        "ascending": TokenType.ASCENDING,
        "asc": TokenType.ASC,
        "descending": TokenType.DESCENDING,
        "desc": TokenType.DESC,
        "limit": TokenType.LIMIT,
        "offset": TokenType.OFFSET,
        "update": TokenType.UPDATE,
        "set": TokenType.SET,
        "delete": TokenType.DELETE,
        "distinct": TokenType.DISTINCT,

        # Joins
        "join": TokenType.JOIN,
        "inner": TokenType.INNER,
        "left": TokenType.LEFT,
        "right": TokenType.RIGHT,
        "on": TokenType.ON,

        # Types
        "integer": TokenType.TYPE_INTEGER,
        "int": TokenType.TYPE_INTEGER,
        "text": TokenType.TYPE_TEXT,
        "string": TokenType.TYPE_TEXT,
        "varchar": TokenType.TYPE_TEXT,
        "real": TokenType.TYPE_REAL,
        "float": TokenType.TYPE_REAL,
        "double": TokenType.TYPE_REAL,
        "boolean": TokenType.TYPE_BOOLEAN,
        "bool": TokenType.TYPE_BOOLEAN,
        "timestamp": TokenType.TYPE_TIMESTAMP,
        "datetime": TokenType.TYPE_TIMESTAMP,
        "blob": TokenType.TYPE_BLOB,
        "json": TokenType.TYPE_JSON,

        # Constraints
        "primary": TokenType.PRIMARY,
        "key": TokenType.KEY,
        "autoincrement": TokenType.AUTOINCREMENT,
        "not": TokenType.NOT,
        "null": TokenType.NULL,
        "unique": TokenType.UNIQUE,
        "default": TokenType.DEFAULT,
        "references": TokenType.REFERENCES,

        # Operators & Logic
        "and": TokenType.AND,
        "or": TokenType.OR,
        "is": TokenType.IS,
        "like": TokenType.LIKE,
        "in": TokenType.IN,
        "between": TokenType.BETWEEN,
        "as": TokenType.AS,

        # Aggregate functions
        "count": TokenType.COUNT,
        "sum": TokenType.SUM,
        "avg": TokenType.AVG,
        "min": TokenType.MIN,
        "max": TokenType.MAX,

        # Boolean & Null literals
        "true": TokenType.BOOLEAN_LITERAL,
        "false": TokenType.BOOLEAN_LITERAL,
        "none": TokenType.NULL_LITERAL,
    }

    def __init__(self, source: str):
        self.source = source
        self.length = len(source)
        self.pos = 0
        self.line = 1
        self.col = 1
        self.indent_stack = [0]
        self.tokens: List[Token] = []

    def peek(self, offset: int = 0) -> str:
        idx = self.pos + offset
        if idx < self.length:
            return self.source[idx]
        return ""

    def advance(self) -> str:
        if self.pos < self.length:
            ch = self.source[self.pos]
            self.pos += 1
            if ch == '\n':
                self.line += 1
                self.col = 1
            else:
                self.col += 1
            return ch
        return ""

    def tokenize(self) -> List[Token]:
        at_line_start = True

        while self.pos < self.length:
            if at_line_start:
                # Count leading spaces for indentation
                leading_spaces = 0
                while self.pos < self.length and self.peek() in (' ', '\t'):
                    if self.peek() == ' ':
                        leading_spaces += 1
                    elif self.peek() == '\t':
                        leading_spaces += 4
                    self.advance()

                # Ignore blank lines or comment-only lines
                if self.pos < self.length and self.peek() in ('\r', '\n'):
                    if self.peek() == '\r':
                        self.advance()
                    if self.peek() == '\n':
                        self.advance()
                    continue

                if self.pos < self.length and (self.peek() == '#' or (self.peek() == '-' and self.peek(1) == '-')):
                    # Comment line - skip to newline
                    while self.pos < self.length and self.peek() not in ('\r', '\n'):
                        self.advance()
                    if self.peek() == '\r':
                        self.advance()
                    if self.peek() == '\n':
                        self.advance()
                    continue

                # Process indentation
                current_indent = self.indent_stack[-1]
                if leading_spaces > current_indent:
                    self.indent_stack.append(leading_spaces)
                    self.tokens.append(Token(TokenType.INDENT, leading_spaces, self.line, self.col))
                elif leading_spaces < current_indent:
                    while self.indent_stack and self.indent_stack[-1] > leading_spaces:
                        self.indent_stack.pop()
                        self.tokens.append(Token(TokenType.DEDENT, leading_spaces, self.line, self.col))
                    if self.indent_stack and self.indent_stack[-1] != leading_spaces:
                        raise LexerError("Inconsistent indentation level.", self.line, self.col,
                                         "Ensure your code blocks are aligned using 4 spaces.")

                at_line_start = False

            ch = self.peek()

            # Skip inline whitespace
            if ch in (' ', '\t'):
                self.advance()
                continue

            # Comments (-- or #)
            if ch == '#' or (ch == '-' and self.peek(1) == '-'):
                while self.pos < self.length and self.peek() not in ('\r', '\n'):
                    self.advance()
                continue

            # Newlines
            if ch in ('\r', '\n'):
                if ch == '\r':
                    self.advance()
                if self.peek() == '\n':
                    self.advance()
                self.tokens.append(Token(TokenType.NEWLINE, "\n", self.line, self.col))
                at_line_start = True
                continue

            # Strings ("..." or '...')
            if ch in ('"', "'"):
                quote_char = self.advance()
                start_line, start_col = self.line, self.col
                str_val = []
                while self.pos < self.length and self.peek() != quote_char:
                    if self.peek() == '\\':
                        self.advance()
                        esc = self.advance()
                        if esc == 'n':
                            str_val.append('\n')
                        elif esc == 't':
                            str_val.append('\t')
                        elif esc == quote_char:
                            str_val.append(quote_char)
                        elif esc == '\\':
                            str_val.append('\\')
                        else:
                            str_val.append(esc)
                    else:
                        str_val.append(self.advance())

                if self.pos >= self.length or self.peek() != quote_char:
                    raise LexerError(f"Unterminated string literal starting with {quote_char}", start_line, start_col,
                                     f"Close the string with a matching {quote_char} quote.")
                self.advance()  # Consume closing quote
                self.tokens.append(Token(TokenType.STRING_LITERAL, "".join(str_val), start_line, start_col))
                continue

            # Numbers
            if ch.isdigit() or (ch == '.' and self.peek(1).isdigit()):
                start_line, start_col = self.line, self.col
                num_str = []
                is_float = False
                while self.pos < self.length and (self.peek().isdigit() or self.peek() == '.'):
                    if self.peek() == '.':
                        if is_float:
                            break
                        is_float = True
                    num_str.append(self.advance())
                val = float("".join(num_str)) if is_float else int("".join(num_str))
                self.tokens.append(Token(TokenType.NUMBER_LITERAL, val, start_line, start_col))
                continue

            # Symbols & Two-character operators
            start_line, start_col = self.line, self.col
            if ch == '!' and self.peek(1) == '=':
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.NOT_EQUALS, "!=", start_line, start_col))
                continue
            if ch == '>' and self.peek(1) == '=':
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.GTE, ">=", start_line, start_col))
                continue
            if ch == '<' and self.peek(1) == '=':
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.LTE, "<=", start_line, start_col))
                continue

            # Single-character punctuation
            single_tokens = {
                ':': TokenType.COLON,
                ',': TokenType.COMMA,
                '.': TokenType.DOT,
                '(': TokenType.LPAREN,
                ')': TokenType.RPAREN,
                '=': TokenType.EQUALS,
                '>': TokenType.GT,
                '<': TokenType.LT,
                '+': TokenType.PLUS,
                '-': TokenType.MINUS,
                '*': TokenType.STAR,
                '/': TokenType.SLASH,
            }
            if ch in single_tokens:
                self.advance()
                self.tokens.append(Token(single_tokens[ch], ch, start_line, start_col))
                continue

            # Identifiers and Keywords
            if ch.isalpha() or ch == '_':
                start_line, start_col = self.line, self.col
                ident = []
                while self.pos < self.length and (self.peek().isalnum() or self.peek() == '_'):
                    ident.append(self.advance())
                word = "".join(ident)
                word_lower = word.lower()

                if word_lower in self.KEYWORDS:
                    tok_type = self.KEYWORDS[word_lower]
                    if tok_type == TokenType.BOOLEAN_LITERAL:
                        self.tokens.append(Token(tok_type, word_lower == "true", start_line, start_col))
                    elif tok_type == TokenType.NULL_LITERAL:
                        self.tokens.append(Token(tok_type, None, start_line, start_col))
                    else:
                        self.tokens.append(Token(tok_type, word, start_line, start_col))
                else:
                    self.tokens.append(Token(TokenType.IDENTIFIER, word, start_line, start_col))
                continue

            # Unknown character
            raise LexerError(f"Unexpected character '{ch}'", self.line, self.col,
                             "Remove or replace this invalid character.")

        # Emit remaining DEDENT tokens
        while len(self.indent_stack) > 1:
            self.indent_stack.pop()
            self.tokens.append(Token(TokenType.DEDENT, 0, self.line, self.col))

        self.tokens.append(Token(TokenType.EOF, "", self.line, self.col))
        return self.tokens
