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
    "type enlgs": "DOMAIN_DECL",
    "type script": "DOMAIN_DECL",
    "type js": "DOMAIN_DECL",
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

    # ── General Variable & Index Assignments ──
    "set": "ASSIGN_VAR",
    "assign": "ASSIGN_VAR",
    "change": "ASSIGN_VAR",
    "update": "ASSIGN_VAR",
    "put": "PUT_INTO",
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

    # ── Functions, Async & Generators ──
    "async to do": "ASYNC_FUNC_DEF",
    "async function": "ASYNC_FUNC_DEF",
    "async action": "ASYNC_FUNC_DEF",
    "generator to do": "GENERATOR_DEF",
    "generator function": "GENERATOR_DEF",
    "generator": "GENERATOR_DEF",
    "yield": "GENERATOR_YIELD",
    "give back yield": "GENERATOR_YIELD",
    "to do": "FUNC_DEF",
    "function": "FUNC_DEF",
    "action": "FUNC_DEF",
    "routine": "FUNC_DEF",
    "return json": "HTTP_RETURN_JSON",
    "return": "FUNC_RETURN",
    "give back": "FUNC_RETURN",
    "call": "FUNC_CALL",
    "run": "FUNC_CALL",
    "invoke": "FUNC_CALL",
    "execute": "FUNC_CALL",
    "await": "AWAIT_EXPR",

    # ── Conditionals & Pattern Matching ──
    "if": "COND_IF",
    "else if": "COND_ELIF",
    "elif": "COND_ELIF",
    "else": "COND_ELSE",
    "match": "PATTERN_MATCH",
    "case": "PATTERN_CASE",

    # ── Loops & Iterations ──
    "repeat": "LOOP_REPEAT",
    "for each": "LOOP_FOR_EACH",
    "for every": "LOOP_FOR_EACH",
    "for": "LOOP_FOR",
    "while": "LOOP_WHILE",

    # ── Functional Array Pipelines & List Operations ──
    "filter": "ARRAY_FILTER",
    "map": "ARRAY_MAP",
    "reduce": "ARRAY_REDUCE",
    "find in": "ARRAY_FIND",
    "find": "ARRAY_FIND",
    "sort": "ARRAY_SORT",
    "add": "LIST_ADD",
    "push": "LIST_ADD",
    "insert": "LIST_INSERT",
    "remove": "LIST_REMOVE",
    "delete from": "LIST_DELETE_FROM",

    # ── TypeScript Shapes & Type Contracts ──
    "shape": "SHAPE_DEF",
    "type": "SHAPE_DEF",
    "interface": "SHAPE_DEF",

    # ── Declarative UI Components (React/Vue in Enlgs) ──
    "component": "COMPONENT_DEF",
    "widget": "COMPONENT_DEF",
    "make element": "DOM_MAKE_ELEMENT",
    "create element": "DOM_MAKE_ELEMENT",
    "add element": "DOM_ADD_ELEMENT",
    "append to": "DOM_APPEND_TO",
    "append": "DOM_APPEND_TO",
    "prepend to": "DOM_PREPEND_TO",
    "prepend": "DOM_PREPEND_TO",
    "animate": "ANIMATE_TARGET",

    # ── Full-Stack Servers & Web APIs ──
    "serve http on port": "SERVER_HTTP",
    "serve http": "SERVER_HTTP",
    "serve web on port": "SERVER_HTTP",
    "serve web": "SERVER_HTTP",
    "route get": "ROUTE_GET",
    "route post": "ROUTE_POST",
    "route put": "ROUTE_PUT",
    "route delete": "ROUTE_DELETE",
    "route": "ROUTE_ANY",

    # ── Centralized State Stores (Redux/Zustand style) ──
    "store": "STORE_DEF",
    "state": "STORE_STATE",
    "dispatch": "STORE_DISPATCH",

    # ── 3D World & Canvas DSL (Three.js / WebGL) ──
    "world 3d on": "WORLD_3D",
    "world 3d": "WORLD_3D",
    "3d scene on": "WORLD_3D",
    "3d scene": "WORLD_3D",
    "on every animation frame": "ANIM_FRAME_LOOP",
    "every frame": "ANIM_FRAME_LOOP",
    "animate frame": "ANIM_FRAME_LOOP",
    "render scene with": "RENDER_SCENE",
    "render scene": "RENDER_SCENE",
    "render": "RENDER_SCENE",
    "rotate by": "ROTATE_BY",
    "rotate": "ROTATE_BY",
    "translate by": "TRANSLATE_BY",
    "translate": "TRANSLATE_BY",
    "move to": "MOVE_TARGET",
    "move by": "MOVE_TARGET",
    "move": "MOVE_TARGET",

    # ── Destructuring & Spread ──
    "extract": "EXTRACT_FROM",
    "unpack": "EXTRACT_FROM",
    "spread": "SPREAD_EXPR",

    # ── WebSockets & Realtime ──
    "connect websocket to": "WEBSOCKET_CONNECT",
    "connect websocket": "WEBSOCKET_CONNECT",
    "connect socket to": "WEBSOCKET_CONNECT",
    "connect socket": "WEBSOCKET_CONNECT",
    "when socket receives": "WEBSOCKET_RECEIVE",

    # ── Fetch / Network APIs ──
    "fetch data": "FETCH_GET",
    "fetch json": "FETCH_GET",
    "fetch": "FETCH_GET",
    "send data": "FETCH_POST",
    "send json": "FETCH_POST",
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
    "store in local": "STORAGE_SET",
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

    # ── OOP / Classes / Blueprints ──
    "blueprint": "BLUEPRINT_DEF",
    "class": "CLASS_DEF",
    "to initialize with": "CLASS_INIT",
    "to initialize": "CLASS_INIT",
    "initialize with": "CLASS_INIT",
    "initialize": "CLASS_INIT",
    "super with": "CLASS_SUPER",
    "super": "CLASS_SUPER",
    "getter": "CLASS_GETTER",
    "setter": "CLASS_SETTER",
    "extends": "CLASS_EXTENDS",
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
