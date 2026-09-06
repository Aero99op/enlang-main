"""Lexer for enlngdb (Sovereign Native Storage Engine - ZERO SQL)."""

from typing import List
from enlngdb.tokens import Token, TokenType


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
        # Domain Header & Output
        "type": TokenType.TYPE,
        "enlngdb": TokenType.ENLNGDB,
        "enlgdb": TokenType.ENLGDB,
        "display": TokenType.DISPLAY,

        # DDL & Database Lifecycle
        "create": TokenType.CREATE,
        "table": TokenType.TABLE,
        "tables": TokenType.TABLES,
        "database": TokenType.DATABASE,
        "databases": TokenType.DATABASES,
        "use": TokenType.USE,
        "open": TokenType.OPEN,
        "connect": TokenType.CONNECT,
        "save": TokenType.SAVE,
        "put": TokenType.PUT,
        "with": TokenType.WITH,
        "drop": TokenType.DROP,
        "index": TokenType.INDEX,

        # Safety Guard
        "confirm": TokenType.CONFIRM,
        "confirmed": TokenType.CONFIRMED,

        # DML & Queries
        "insert": TokenType.INSERT,
        "into": TokenType.INTO,
        "values": TokenType.VALUES,
        "select": TokenType.SELECT,
        "find": TokenType.FIND,
        "fetch": TokenType.FETCH,
        "get": TokenType.GET,
        "show": TokenType.SHOW,
        "update": TokenType.UPDATE,
        "change": TokenType.CHANGE,
        "set": TokenType.SET,
        "delete": TokenType.DELETE,
        "remove": TokenType.REMOVE,
        "count": TokenType.COUNT,

        # Prepositions & Modifiers
        "all": TokenType.ALL,
        "of": TokenType.OF,
        "from": TokenType.FROM,
        "where": TokenType.WHERE,
        "in": TokenType.IN,
        "order": TokenType.ORDER,
        "sorted": TokenType.SORTED,
        "by": TokenType.BY,
        "ascending": TokenType.ASCENDING,
        "asc": TokenType.ASC,
        "descending": TokenType.DESCENDING,
        "desc": TokenType.DESC,
        "highest": TokenType.HIGHEST,
        "lowest": TokenType.LOWEST,
        "first": TokenType.FIRST,
        "limit": TokenType.LIMIT,
        "top": TokenType.TOP,
        "offset": TokenType.OFFSET,
        "if": TokenType.IF,
        "exists": TokenType.EXISTS,

        # Natural Comparators
        "is": TokenType.IS,
        "not": TokenType.NOT,
        "greater": TokenType.GREATER,
        "less": TokenType.LESS,
        "than": TokenType.THAN,
        "at": TokenType.AT,
        "least": TokenType.LEAST,
        "most": TokenType.MOST,
        "above": TokenType.ABOVE,
        "below": TokenType.BELOW,
        "more": TokenType.MORE,
        "under": TokenType.UNDER,
        "equal": TokenType.EQUAL,
        "equals": TokenType.EQUALS,
        "to": TokenType.TO,
        "like": TokenType.LIKE,
        "and": TokenType.AND,
        "or": TokenType.OR,
        "as": TokenType.AS,

        # Directives
        "hint": TokenType.HINT,

        # Silent Words & Noise Nouns
        "the": TokenType.THE,
        "a": TokenType.A,
        "an": TokenType.AN,
        "please": TokenType.PLEASE,
        "kindly": TokenType.KINDLY,
        "record": TokenType.RECORD,
        "records": TokenType.RECORDS,
        "row": TokenType.ROW,
        "rows": TokenType.ROWS,
        "entry": TokenType.ENTRY,
        "entries": TokenType.ENTRIES,
        "data": TokenType.DATA,
        "item": TokenType.ITEM,
        "items": TokenType.ITEMS,

        # Types
        "integer": TokenType.TYPE_INTEGER,
        "int": TokenType.TYPE_INTEGER,
        "text": TokenType.TYPE_TEXT,
        "string": TokenType.TYPE_TEXT,
        "real": TokenType.TYPE_REAL,
        "float": TokenType.TYPE_REAL,
        "double": TokenType.TYPE_REAL,
        "boolean": TokenType.TYPE_BOOLEAN,
        "bool": TokenType.TYPE_BOOLEAN,
        "json": TokenType.TYPE_JSON,

        # Constraints
        "primary": TokenType.PRIMARY,
        "key": TokenType.KEY,
        "autoincrement": TokenType.AUTOINCREMENT,
        "null": TokenType.NULL,
        "unique": TokenType.UNIQUE,
        "default": TokenType.DEFAULT,

        # Literals
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
                leading_spaces = 0
                while self.pos < self.length and self.peek() in (' ', '\t'):
                    if self.peek() == ' ':
                        leading_spaces += 1
                    elif self.peek() == '\t':
                        leading_spaces += 4
                    self.advance()

                # Ignore blank lines
                if self.pos < self.length and self.peek() in ('\r', '\n'):
                    if self.peek() == '\r':
                        self.advance()
                    if self.peek() == '\n':
                        self.advance()
                    continue

                # Comments: # or --
                if self.pos < self.length and (self.peek() == '#' or (self.peek() == '-' and self.peek(1) == '-')):
                    while self.pos < self.length and self.peek() not in ('\r', '\n'):
                        self.advance()
                    if self.peek() == '\r':
                        self.advance()
                    if self.peek() == '\n':
                        self.advance()
                    continue

                current_indent = self.indent_stack[-1]
                if leading_spaces > current_indent:
                    self.indent_stack.append(leading_spaces)
                    self.tokens.append(Token(TokenType.INDENT, leading_spaces, self.line, self.col))
                elif leading_spaces < current_indent:
                    while self.indent_stack and self.indent_stack[-1] > leading_spaces:
                        self.indent_stack.pop()
                        self.tokens.append(Token(TokenType.DEDENT, leading_spaces, self.line, self.col))

                if self.pos >= self.length:
                    break

                at_line_start = False

            ch = self.peek()
            if not ch:
                break

            if ch in (' ', '\t'):
                self.advance()
                continue

            if ch == '\r':
                self.advance()
                if self.peek() == '\n':
                    self.advance()
                self.tokens.append(Token(TokenType.NEWLINE, '\n', self.line - 1, self.col))
                at_line_start = True
                continue

            if ch == '\n':
                self.advance()
                self.tokens.append(Token(TokenType.NEWLINE, '\n', self.line - 1, self.col))
                at_line_start = True
                continue

            if ch == '#' or (ch == '-' and self.peek(1) == '-'):
                while self.pos < self.length and self.peek() not in ('\r', '\n'):
                    self.advance()
                continue

            # Strings
            if ch in ('"', "'"):
                quote_char = ch
                start_line = self.line
                start_col = self.col
                self.advance()
                str_val = []
                while self.pos < self.length and self.peek() != quote_char:
                    if self.peek() == '\\':
                        self.advance()
                        esc = self.advance()
                        if esc == 'n': str_val.append('\n')
                        elif esc == 't': str_val.append('\t')
                        elif esc == 'r': str_val.append('\r')
                        elif esc == '"': str_val.append('"')
                        elif esc == "'": str_val.append("'")
                        elif esc == '\\': str_val.append('\\')
                        else: str_val.append(esc)
                    else:
                        str_val.append(self.advance())
                if self.pos >= self.length:
                    raise LexerError(f"Unterminated string literal starting with {quote_char}", start_line, start_col)
                self.advance()  # closing quote
                self.tokens.append(Token(TokenType.STRING_LITERAL, "".join(str_val), start_line, start_col))
                continue

            # Numbers
            if ch.isdigit() or (ch == '.' and self.peek(1).isdigit()):
                start_col = self.col
                num_str = []
                is_float = False
                while self.pos < self.length and (self.peek().isdigit() or self.peek() == '.'):
                    if self.peek() == '.':
                        if is_float:
                            break
                        is_float = True
                    num_str.append(self.advance())
                val = float("".join(num_str)) if is_float else int("".join(num_str))
                self.tokens.append(Token(TokenType.NUMBER_LITERAL, val, self.line, start_col))
                continue

            # Identifiers and Keywords
            if ch.isalpha() or ch == '_':
                start_col = self.col
                word = []
                while self.pos < self.length and (self.peek().isalnum() or self.peek() == '_'):
                    word.append(self.advance())
                word_str = "".join(word)
                lower_word = word_str.lower()

                if lower_word in self.KEYWORDS:
                    tok_type = self.KEYWORDS[lower_word]
                    val = True if lower_word == "true" else False if lower_word == "false" else None if lower_word == "none" else lower_word
                    self.tokens.append(Token(tok_type, val, self.line, start_col))
                else:
                    self.tokens.append(Token(TokenType.IDENTIFIER, word_str, self.line, start_col))
                continue

            # Symbols & Punctuation
            start_col = self.col
            if ch == ':':
                self.advance()
                self.tokens.append(Token(TokenType.COLON, ':', self.line, start_col))
            elif ch == ';':
                self.advance()
                self.tokens.append(Token(TokenType.SEMICOLON, ';', self.line, start_col))
            elif ch == ',':
                self.advance()
                self.tokens.append(Token(TokenType.COMMA, ',', self.line, start_col))
            elif ch == '.':
                self.advance()
                self.tokens.append(Token(TokenType.DOT, '.', self.line, start_col))
            elif ch == '(':
                self.advance()
                self.tokens.append(Token(TokenType.LPAREN, '(', self.line, start_col))
            elif ch == ')':
                self.advance()
                self.tokens.append(Token(TokenType.RPAREN, ')', self.line, start_col))
            elif ch == '=':
                self.advance()
                if self.peek() == '=':
                    self.advance()
                self.tokens.append(Token(TokenType.EQUALS, '=', self.line, start_col))
            elif ch == '!' and self.peek(1) == '=':
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.NOT_EQUALS, '!=', self.line, start_col))
            elif ch == '>' and self.peek(1) == '=':
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.GTE, '>=', self.line, start_col))
            elif ch == '>':
                self.advance()
                self.tokens.append(Token(TokenType.GT, '>', self.line, start_col))
            elif ch == '<' and self.peek(1) == '=':
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.LTE, '<=', self.line, start_col))
            elif ch == '<' and self.peek(1) == '>':
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.NOT_EQUALS, '!=', self.line, start_col))
            elif ch == '<':
                self.advance()
                self.tokens.append(Token(TokenType.LT, '<', self.line, start_col))
            elif ch == '+':
                self.advance()
                self.tokens.append(Token(TokenType.PLUS, '+', self.line, start_col))
            elif ch == '-':
                self.advance()
                self.tokens.append(Token(TokenType.MINUS, '-', self.line, start_col))
            else:
                self.advance()
                raise LexerError(f"Unexpected character '{ch}'", self.line, start_col)

        while len(self.indent_stack) > 1:
            self.indent_stack.pop()
            self.tokens.append(Token(TokenType.DEDENT, 0, self.line, self.col))

        self.tokens.append(Token(TokenType.EOF, "", self.line, self.col))
        return self.tokens
