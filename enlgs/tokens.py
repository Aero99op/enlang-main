"""enlgs Token Types & Hint Intent Registry.

Defines lexical tokens, canonical Hint Registry mappings, silent connectors, and operator dictionaries.
"""

from enum import Enum, auto
from dataclasses import dataclass
from typing import Dict, Set

class TokenType(Enum):
    HINT = auto()        # Recognized intent hint words (create, when, fetch, show, etc.)
    IDENTIFIER = auto()  # General identifiers / variables / function names
    NUMBER = auto()      # Numeric literals (10, 3.14, 100)
    STRING = auto()      # String literals ("hello", "button-id")
    SYMBOL = auto()      # Structural symbols (:, ,, ;, (, ), {, }, [, ])
    OPERATOR = auto()    # Math / comparison / logic operators (==, !=, >, <, +, -, *, /, &&, ||, =, +=, etc.)
    CONNECTOR = auto()   # Silent filler words (to, from, with, as, of, in, the, a, an, etc.)
    INDENT = auto()      # Block indent
    DEDENT = auto()      # Block dedent
    NEWLINE = auto()     # Line break
    RAW_LINE = auto()    # Raw JS fallback line
    EOF = auto()         # End of file

@dataclass
class Token:
    type: TokenType
    value: str
    line: int
    column: int
    raw_text: str = ""

# Hint-Word Registry: Maps natural English hints & phrases to canonical Intent IDs
HINT_REGISTRY: Dict[str, str] = {
    # ── Domain Declarations ──
    "in script": "DOMAIN_DECL",
    "script enlgs": "DOMAIN_DECL",
    "start script": "DOMAIN_DECL",
    "enlgs script": "DOMAIN_DECL",

    # ── Variables ──
    "create": "DECLARE_VAR",
    "declare": "DECLARE_VAR",
    "let": "DECLARE_VAR",
    "initialize": "DECLARE_VAR",
    "define": "DECLARE_CONST",
    "const": "DECLARE_CONST",
    
    # ── DOM Setters (Checked before generic 'set') ──
    "set text": "DOM_SET_TEXT",
    "set html": "DOM_SET_HTML",
    "set value": "DOM_SET_VALUE",
    "set color": "DOM_SET_COLOR",
    "set background": "DOM_SET_BG",
    "set width": "DOM_SET_WIDTH",
    "set height": "DOM_SET_HEIGHT",
    "set style": "DOM_SET_STYLE",
    "refresh": "DOM_REFRESH",

    # ── General Variable Assignments ──
    "set": "ASSIGN_VAR",
    "assign": "ASSIGN_VAR",
    "change": "ASSIGN_VAR",
    "update": "ASSIGN_VAR",
    "increase": "COMPOUND_ADD",
    "decrease": "COMPOUND_SUB",
    "multiply": "COMPOUND_MUL",
    "divide": "COMPOUND_DIV",

    # ── Output / Console / Alerts ──
    "show element": "DOM_SHOW",
    "show": "CONSOLE_LOG",
    "log": "CONSOLE_LOG",
    "print": "CONSOLE_LOG",
    "display": "CONSOLE_LOG",
    "warn": "CONSOLE_WARN",
    "error": "CONSOLE_ERROR",
    "alert": "BROWSER_ALERT",

    # ── DOM Access & Manipulation ──
    "get element": "DOM_GET",
    "get text": "DOM_GET_TEXT",
    "get value": "DOM_GET_VALUE",
    "get": "DOM_GET",
    "find all": "DOM_QUERY_ALL",
    "find": "DOM_QUERY",
    "hide element": "DOM_HIDE",
    "hide": "DOM_HIDE",
    "add class": "CLASS_ADD",
    "remove class": "CLASS_REMOVE",
    "toggle class": "CLASS_TOGGLE",

    # ── Events ──
    "when": "EVENT_BIND",
    "on": "EVENT_BIND",
    "prevent default": "PREVENT_DEFAULT",

    # ── Functions ──
    "to do": "FUNC_DEF",
    "function": "FUNC_DEF",
    "action": "FUNC_DEF",
    "routine": "FUNC_DEF",
    "return": "FUNC_RETURN",
    "give back": "FUNC_RETURN",
    "call": "FUNC_CALL",
    "run": "FUNC_CALL",
    "invoke": "FUNC_CALL",
    "execute": "FUNC_CALL",

    # ── Conditionals ──
    "if": "COND_IF",
    "else if": "COND_ELIF",
    "elif": "COND_ELIF",
    "else": "COND_ELSE",

    # ── Loops ──
    "repeat": "LOOP_REPEAT",
    "for each": "LOOP_FOR_EACH",
    "for": "LOOP_FOR",
    "while": "LOOP_WHILE",

    # ── Fetch / Network APIs ──
    "fetch data": "FETCH_GET",
    "fetch": "FETCH_GET",
    "send data": "FETCH_POST",
    "send": "FETCH_POST",
    "post": "FETCH_POST",

    # ── Timers ──
    "after": "TIMER_ONCE",
    "every": "TIMER_REPEAT",
    "stop timer": "TIMER_STOP",

    # ── Browser Actions ──
    "redirect": "BROWSER_REDIRECT",
    "reload page": "BROWSER_RELOAD",
    "reload": "BROWSER_RELOAD",
    "go back": "BROWSER_BACK",
    "go forward": "BROWSER_FORWARD",
    "scroll to": "BROWSER_SCROLL",
    "copy": "CLIPBOARD_COPY",
    "open": "WINDOW_OPEN",

    # ── Local Storage ──
    "store": "STORAGE_SET",
    "retrieve": "STORAGE_GET",
    "remove stored": "STORAGE_REMOVE",
    "clear storage": "STORAGE_CLEAR",

    # ── Error Handling ──
    "try": "TRY_BLOCK",
    "attempt": "TRY_BLOCK",
    "catch": "CATCH_BLOCK",
    "rescue": "CATCH_BLOCK",
    "throw": "THROW_ERROR",
    "raise": "THROW_ERROR",

    # ── OOP / Classes ──
    "class": "CLASS_DEF",
    "blueprint": "CLASS_DEF",
    "new": "CLASS_NEW",

    # ── JSON ──
    "parse json": "JSON_PARSE",
    "convert to json": "JSON_STRINGIFY",
}

# Silent connector words (optional filler words that can be dropped)
CONNECTORS: Set[str] = {
    "to", "from", "with", "as", "of", "in", "into", "the", "a", "an",
    "at", "on", "by", "and", "using", "is", "are", "element", "it", "its",
    "times", "seconds", "second", "minutes", "minute", "ms", "milliseconds",
    "pixels", "px", "body", "data", "result", "response", "value", "text"
}

# English comparison phrases -> JavaScript operators
COMPARISON_PHRASES: Dict[str, str] = {
    "is greater than or equal to": ">=",
    "greater than or equal to": ">=",
    "is less than or equal to": "<=",
    "less than or equal to": "<=",
    "is greater than": ">",
    "greater than": ">",
    "is less than": "<",
    "less than": "<",
    "is strictly equal to": "===",
    "strictly equal to": "===",
    "is equal to": "===",
    "is not equal to": "!==",
    "not equal to": "!==",
    "is not": "!==",
    "equals": "===",
    "is": "===",
}
