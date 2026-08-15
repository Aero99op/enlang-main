"""enlgm Token Types & English Hint Intent Registry.

Defines lexical tokens, canonical Mobile Hint Registry mappings, silent connectors,
and operators for compiling .enlgm source to Flutter / Dart.
"""

from enum import Enum, auto
from dataclasses import dataclass
from typing import Dict, Set

class ENLGMTokenType(Enum):
    HINT = auto()        # Recognized mobile intent hint phrases/words
    IDENTIFIER = auto()  # Variable names, class names, property keys
    NUMBER = auto()      # Numeric literals (24, 3.14, 100)
    STRING = auto()      # String literals ("Hello", "https://...")
    SYMBOL = auto()      # Structural symbols (:, ,, ;, (, ), {, }, [, ])
    OPERATOR = auto()    # Operators (==, !=, >, <, +, -, *, /, &&, ||, =, +=, -=)
    CONNECTOR = auto()   # Silent filler words (with, and, to, from, as, of, in, the, a, an, by, carrying, having)
    INDENT = auto()      # Indentation level increase
    DEDENT = auto()      # Indentation level decrease
    NEWLINE = auto()     # Line break
    RAW_DART = auto()    # Raw Dart block
    EOF = auto()         # End of file

@dataclass
class Token:
    type: ENLGMTokenType
    value: str
    line: int
    column: int
    raw_text: str = ""

# Master English Hint Registry for .enlgm (sorted longest-phrase first)
MOBILE_HINT_REGISTRY: Dict[str, str] = {
    # ── Architecture Identifier (Mandatory First Line) ──
    "in mobile": "MOBILE_DOMAIN",
    "mobile app": "MOBILE_DOMAIN",
    "enlgm mobile": "MOBILE_DOMAIN",

    # ── App & Screen Structure ──
    "stateful screen": "STATEFUL_SCREEN_DEF",
    "screen": "SCREEN_DEF",
    "blueprint": "WIDGET_BLUEPRINT",
    "app": "APP_DEF",
    "scaffold": "WIDGET_SCAFFOLD",

    # ── Imports & Packages ──
    "use flutter": "FLUTTER_IMPORT",
    "use package": "PACKAGE_IMPORT",
    "include package": "PACKAGE_IMPORT",
    "use": "FLUTTER_IMPORT",

    # ── Layout Widgets ──
    "column centered": "WIDGET_COLUMN_CENTER",
    "column spaced": "WIDGET_COLUMN_SPACED",
    "column": "WIDGET_COLUMN",
    "row centered": "WIDGET_ROW_CENTER",
    "row spaced": "WIDGET_ROW_SPACED",
    "row": "WIDGET_ROW",
    "stack": "WIDGET_STACK",
    "center": "WIDGET_CENTER",
    "scroll": "WIDGET_SCROLL",
    "safe area": "WIDGET_SAFEAREA",
    "expanded flex": "WIDGET_EXPANDED_FLEX",
    "expanded": "WIDGET_EXPANDED",
    "flexible flex": "WIDGET_FLEXIBLE_FLEX",
    "flexible": "WIDGET_FLEXIBLE",
    "container": "WIDGET_CONTAINER",
    "card": "WIDGET_CARD",
    "padding all": "WIDGET_PADDING_ALL",
    "padding horizontal": "WIDGET_PADDING_SYMMETRIC",
    "padding vertical": "WIDGET_PADDING_SYMMETRIC",
    "padding": "WIDGET_PADDING",
    "list of": "WIDGET_LIST",
    "grid with columns": "WIDGET_GRID",
    "grid columns": "WIDGET_GRID",
    "grid": "WIDGET_GRID",

    # ── Basic UI Widgets ──
    "image from asset": "WIDGET_IMAGE_ASSET",
    "image from": "WIDGET_IMAGE_NETWORK",
    "image asset": "WIDGET_IMAGE_ASSET",
    "image": "WIDGET_IMAGE_NETWORK",
    "avatar from asset": "WIDGET_AVATAR_ASSET",
    "avatar from": "WIDGET_AVATAR_NETWORK",
    "avatar initials": "WIDGET_AVATAR_INITIALS",
    "avatar": "WIDGET_AVATAR_NETWORK",
    "floating action button": "WIDGET_FAB",
    "floating button": "WIDGET_FAB",
    "fab": "WIDGET_FAB",
    "text button": "WIDGET_TEXT_BUTTON",
    "icon button": "WIDGET_ICON_BUTTON",
    "button": "WIDGET_BUTTON",
    "text": "WIDGET_TEXT",
    "icon": "WIDGET_ICON",
    "input": "WIDGET_TEXTFIELD",
    "blank space": "WIDGET_SPACER",
    "space height": "WIDGET_SIZED_BOX_H",
    "space width": "WIDGET_SIZED_BOX_W",
    "spacer height": "WIDGET_SIZED_BOX_H",
    "spacer width": "WIDGET_SIZED_BOX_W",
    "spacer": "WIDGET_SPACER",
    "divider": "WIDGET_DIVIDER",
    "chip": "WIDGET_CHIP",
    "badge": "WIDGET_BADGE",
    "loading bar": "WIDGET_PROGRESS_LINEAR",
    "loading spinner": "WIDGET_PROGRESS_CIRCULAR",
    "progress bar": "WIDGET_PROGRESS_LINEAR",
    "circular progress": "WIDGET_PROGRESS_CIRCULAR",
    "toggle": "WIDGET_SWITCH",
    "switch": "WIDGET_SWITCH",
    "slider": "WIDGET_SLIDER",
    "checkbox": "WIDGET_CHECKBOX",
    "dropdown": "WIDGET_DROPDOWN",
    "hero": "WIDGET_HERO",

    # ── Navigation Bars ──
    "app bar": "WIDGET_APPBAR",
    "bottom navigation": "WIDGET_BOTTOM_NAV",
    "bottom bar": "WIDGET_BOTTOM_NAV",
    "side drawer": "WIDGET_DRAWER",
    "drawer": "WIDGET_DRAWER",
    "tab bar": "WIDGET_TABBAR",

    # ── Navigation Actions (Flutter Navigator Translation) ──
    "go to ... and clear all": "NAV_CLEAR",
    "clear push": "NAV_CLEAR",
    "go to": "NAV_PUSH",
    "open": "NAV_PUSH",
    "push": "NAV_PUSH",
    "go back with result": "NAV_POP_RESULT",
    "go back": "NAV_POP",
    "pop with": "NAV_POP_RESULT",
    "pop": "NAV_POP",
    "switch to": "NAV_REPLACE",
    "replace screen with": "NAV_REPLACE",
    "replace": "NAV_REPLACE",

    # ── State & Variables ──
    "create": "STATE_DECLARE",
    "state": "STATE_BLOCK",
    "set": "STATE_SET",
    "increase": "STATE_INC",
    "decrease": "STATE_DEC",
    "add": "STATE_ADD",
    "subtract": "STATE_SUB",

    # ── Events & Lifecycle ──
    "when long pressed": "EVENT_LONG_PRESS",
    "when tapped": "EVENT_TAP",
    "when clicked": "EVENT_TAP",
    "when value changes": "EVENT_CHANGED",
    "when submitted": "EVENT_SUBMITTED",
    "when screen loads": "LIFECYCLE_INIT",
    "when screen closes": "LIFECYCLE_DISPOSE",
    "on tap": "EVENT_TAP",
    "on click": "EVENT_TAP",
    "on change": "EVENT_CHANGED",
    "on submit": "EVENT_SUBMITTED",
    "on load": "LIFECYCLE_INIT",
    "on dispose": "LIFECYCLE_DISPOSE",

    # ── Feedback / Alerts ──
    "show snackbar": "SHOW_SNACKBAR",
    "show toast": "SHOW_TOAST",
    "show alert": "SHOW_ALERT",
    "show dialog": "SHOW_ALERT",
    "snack": "SHOW_SNACKBAR",
    "toast": "SHOW_TOAST",
    "alert": "SHOW_ALERT",

    # ── Network Calls ──
    "load from": "FETCH_GET",
    "fetch from": "FETCH_GET",
    "fetch": "FETCH_GET",
    "send to": "FETCH_POST",
    "post": "FETCH_POST",
    "on success with result": "NET_SUCCESS",
    "on success": "NET_SUCCESS",
    "on failure with error": "NET_FAILURE",
    "on fail": "NET_FAILURE",
    "on error": "NET_FAILURE",

    # ── Raw Dart Escape ──
    "write dart": "RAW_DART",
    "dart": "RAW_DART",
}

# Silent filler words that can be freely added or omitted for natural English flow
CONNECTORS: Set[str] = {
    "with", "and", "to", "from", "as", "of", "in", "the", "a", "an",
    "at", "for", "on", "by", "carrying", "having", "is", "screen", "item"
}
