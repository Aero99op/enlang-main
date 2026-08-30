"""Tokens definition for enlgdb (Natural English SQL & Database Language)."""

from enum import Enum, auto
from dataclasses import dataclass
from typing import Any, Optional


class TokenType(Enum):
    # Header & Domain Declaration
    TYPE = auto()
    ENLGDB = auto()

    # DDL & Schema Keywords
    CREATE = auto()
    TABLE = auto()
    TABLES = auto()
    DATABASE = auto()
    DATABASES = auto()
    USE = auto()
    SHOW = auto()
    WITH = auto()
    ALTER = auto()
    ADD = auto()
    DROP = auto()
    TRUNCATE = auto()
    COLUMN = auto()
    INDEX = auto()

    # Safety Guard (Mandatory for destructive actions)
    CONFIRM = auto()
    CONFIRMED = auto()

    # DML & DQL Keywords
    INSERT = auto()
    INTO = auto()
    VALUES = auto()
    SELECT = auto()
    ALL = auto()
    FROM = auto()
    WHERE = auto()
    ORDER = auto()
    BY = auto()
    ASCENDING = auto()
    ASC = auto()
    DESCENDING = auto()
    DESC = auto()
    LIMIT = auto()
    OFFSET = auto()
    UPDATE = auto()
    SET = auto()
    DELETE = auto()
    JOIN = auto()
    INNER = auto()
    LEFT = auto()
    RIGHT = auto()
    ON = auto()
    GROUP = auto()
    HAVING = auto()
    DISTINCT = auto()

    # Data Types
    TYPE_INTEGER = auto()
    TYPE_TEXT = auto()
    TYPE_REAL = auto()
    TYPE_BOOLEAN = auto()
    TYPE_TIMESTAMP = auto()
    TYPE_BLOB = auto()
    TYPE_JSON = auto()

    # Column Constraints
    PRIMARY = auto()
    KEY = auto()
    AUTOINCREMENT = auto()
    NOT = auto()
    NULL = auto()
    UNIQUE = auto()
    DEFAULT = auto()
    REFERENCES = auto()

    # Operators & Logical
    AND = auto()
    OR = auto()
    IS = auto()
    LIKE = auto()
    IN = auto()
    BETWEEN = auto()
    AS = auto()

    # Aggregate Functions
    COUNT = auto()
    SUM = auto()
    AVG = auto()
    MIN = auto()
    MAX = auto()

    # Symbols & Punctuation
    COLON = auto()
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
    STAR = auto()
    SLASH = auto()

    # Literals
    IDENTIFIER = auto()
    STRING_LITERAL = auto()
    NUMBER_LITERAL = auto()
    BOOLEAN_LITERAL = auto()
    NULL_LITERAL = auto()

    # Whitespace & Structural
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
