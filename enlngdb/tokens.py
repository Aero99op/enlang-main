"""Tokens definition for enlngdb (Sovereign Native Storage Engine - ZERO SQL)."""

from enum import Enum, auto
from dataclasses import dataclass
from typing import Any


class TokenType(Enum):
    # Header & Domain Declaration
    TYPE = auto()
    ENLNGDB = auto()
    ENLGDB = auto()

    # Native Output & Scripting
    DISPLAY = auto()

    # DDL & Storage Schema
    CREATE = auto()
    TABLE = auto()
    TABLES = auto()
    COLUMN = auto()
    COLUMNS = auto()
    DATABASE = auto()
    DATABASES = auto()
    USE = auto()
    OPEN = auto()
    CONNECT = auto()
    SAVE = auto()
    PUT = auto()
    WITH = auto()
    DROP = auto()
    INDEX = auto()

    # Safety Guard
    CONFIRM = auto()
    CONFIRMED = auto()

    # DML & Query Verbs
    INSERT = auto()
    INTO = auto()
    VALUES = auto()
    SELECT = auto()
    FIND = auto()
    FETCH = auto()
    GET = auto()
    SHOW = auto()
    UPDATE = auto()
    CHANGE = auto()
    SET = auto()
    DELETE = auto()
    REMOVE = auto()
    COUNT = auto()

    # Clausal Prepositions & Modifiers
    ALL = auto()
    OF = auto()
    FROM = auto()
    WHERE = auto()
    IN = auto()
    ORDER = auto()
    SORTED = auto()
    BY = auto()
    ASCENDING = auto()
    ASC = auto()
    DESCENDING = auto()
    DESC = auto()
    HIGHEST = auto()
    LOWEST = auto()
    FIRST = auto()
    LIMIT = auto()
    TOP = auto()
    OFFSET = auto()
    IF = auto()
    EXISTS = auto()

    # Natural Comparators & Operators
    IS = auto()
    NOT = auto()
    GREATER = auto()
    LESS = auto()
    THAN = auto()
    AT = auto()
    LEAST = auto()
    MOST = auto()
    ABOVE = auto()
    BELOW = auto()
    MORE = auto()
    UNDER = auto()
    EQUAL = auto()
    TO = auto()
    LIKE = auto()
    AND = auto()
    OR = auto()
    AS = auto()

    # Hints & Directives
    HINT = auto()

    # Silent Words & Noise Nouns
    THE = auto()
    A = auto()
    AN = auto()
    PLEASE = auto()
    KINDLY = auto()
    RECORD = auto()
    RECORDS = auto()
    ROW = auto()
    ROWS = auto()
    ENTRY = auto()
    ENTRIES = auto()
    DATA = auto()
    ITEM = auto()
    ITEMS = auto()

    # Data Types
    TYPE_INTEGER = auto()
    TYPE_TEXT = auto()
    TYPE_REAL = auto()
    TYPE_BOOLEAN = auto()
    TYPE_JSON = auto()

    # Constraints
    PRIMARY = auto()
    KEY = auto()
    AUTOINCREMENT = auto()
    NULL = auto()
    UNIQUE = auto()
    DEFAULT = auto()

    # Symbols & Punctuation
    COLON = auto()
    SEMICOLON = auto()
    COMMA = auto()
    DOT = auto()
    LPAREN = auto()
    RPAREN = auto()
    EQUALS = auto()
    NOT_EQUALS = auto()
    GT = auto()
    GTE = auto()
    LT = auto()
    LTE = auto()
    PLUS = auto()
    MINUS = auto()

    # Literals
    IDENTIFIER = auto()
    STRING_LITERAL = auto()
    NUMBER_LITERAL = auto()
    BOOLEAN_LITERAL = auto()
    NULL_LITERAL = auto()

    # Structural
    NEWLINE = auto()
    INDENT = auto()
    DEDENT = auto()
    EOF = auto()


@dataclass
class Token:
    type: TokenType
    value: Any
    line: int
    column: int

    def __repr__(self) -> str:
        return f"Token({self.type.name}, {self.value!r}, L{self.line}:C{self.column})"
