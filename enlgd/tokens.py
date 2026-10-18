"""enlgd Token Types & Definitions.

Defines tokens, CSS property mappings, connector words, pseudo-states, and unit identifiers.
"""

from enum import Enum, auto
from dataclasses import dataclass
from typing import Dict, List

class TokenType(Enum):
    KEYWORD = auto()     # for, apply, when, define, as, end, finish, etc.
    PROPERTY = auto()    # CSS Property names/aliases (color, background, font-size, etc.)
    CONNECTOR = auto()   # Filler/connector words (and, with, also, using)
    STRING = auto()      # Quoted strings (".card", "white", "#333")
    NUMBER = auto()      # Numeric literals (16, 0.3, 100)
    IDENTIFIER = auto()  # General identifiers / value tokens (flex, center, bold, none)
    SYMBOL = auto()      # Symbols (:, ,, ;)
    INDENT = auto()      # Indentation
    DEDENT = auto()      # Dedent
    NEWLINE = auto()     # Line break
    EOF = auto()         # End of file

@dataclass
class Token:
    type: TokenType
    value: str
    line: int
    column: int

# CSS Property English Aliases -> Canonical CSS property name
PROPERTY_MAPPINGS: Dict[str, str] = {
    # Backgrounds
    "global background": "background",
    "global bg": "background",
    "background-color": "background-color",
    "background color": "background-color",
    "bg color": "background-color",
    "background": "background",
    "bg": "background",
    
    # Typography & Text
    "global font-family": "font-family",
    "global font family": "font-family",
    "global font": "font-family",
    "text-color": "color",
    "text color": "color",
    "color": "color",
    "font-size": "font-size",
    "font size": "font-size",
    "text size": "font-size",
    "font-family": "font-family",
    "font family": "font-family",
    "font": "font-family",
    "typeface": "font-family",
    "font-weight": "font-weight",
    "font weight": "font-weight",
    "weight": "font-weight",
    "line-height": "line-height",
    "line height": "line-height",
    "letter-spacing": "letter-spacing",
    "letter spacing": "letter-spacing",
    "text-align": "text-align",
    "text align": "text-align",
    "align text": "text-align",
    "text-decoration": "text-decoration",
    "text decoration": "text-decoration",
    "text-transform": "text-transform",
    "text transform": "text-transform",
    "text gradient": "text-gradient",
    "text-shadow": "text-shadow",
    "text shadow": "text-shadow",
    
    # Box Model & Spacing
    "margin-top": "margin-top",
    "margin top": "margin-top",
    "margin-bottom": "margin-bottom",
    "margin bottom": "margin-bottom",
    "margin-left": "margin-left",
    "margin left": "margin-left",
    "margin-right": "margin-right",
    "margin right": "margin-right",
    "margin": "margin",
    
    "padding-top": "padding-top",
    "padding top": "padding-top",
    "padding-bottom": "padding-bottom",
    "padding bottom": "padding-bottom",
    "padding-left": "padding-left",
    "padding left": "padding-left",
    "padding-right": "padding-right",
    "padding right": "padding-right",
    "padding": "padding",
    
    # Sizing
    "max-width": "max-width",
    "maximum width": "max-width",
    "max width": "max-width",
    "min-width": "min-width",
    "minimum width": "min-width",
    "min width": "min-width",
    "max-height": "max-height",
    "maximum height": "max-height",
    "max height": "max-height",
    "min-height": "min-height",
    "minimum height": "min-height",
    "min height": "min-height",
    "width": "width",
    "height": "height",
    
    # Borders & Corners
    "border-radius": "border-radius",
    "border radius": "border-radius",
    "corner radius": "border-radius",
    "radius": "border-radius",
    "border-color": "border-color",
    "border color": "border-color",
    "border-width": "border-width",
    "border width": "border-width",
    "border-style": "border-style",
    "border style": "border-style",
    "border-bottom": "border-bottom",
    "border bottom": "border-bottom",
    "border-top": "border-top",
    "border top": "border-top",
    "border-left": "border-left",
    "border left": "border-left",
    "border-right": "border-right",
    "border right": "border-right",
    "border": "border",
    "box-shadow": "box-shadow",
    "box shadow": "box-shadow",
    "shadow": "box-shadow",
    "outline": "outline",
    "stroke color": "stroke",
    "stroke-color": "stroke",
    "stroke width": "stroke-width",
    "stroke-width": "stroke-width",
    "stroke dasharray": "stroke-dasharray",
    "stroke-dasharray": "stroke-dasharray",
    "stroke": "stroke",
    
    # Layout & Flexbox/Grid
    "display": "display",
    "flex-direction": "flex-direction",
    "flex direction": "flex-direction",
    "direction": "flex-direction",
    "flex-wrap": "flex-wrap",
    "flex wrap": "flex-wrap",
    "align-items": "align-items",
    "align items": "align-items",
    "align center": "align-items",
    "align": "align-items",
    "justify-content": "justify-content",
    "justify content": "justify-content",
    "justify center": "justify-content",
    "justify": "justify-content",
    "space between": "justify-content",
    "space around": "justify-content",
    "space evenly": "justify-content",
    "columns": "grid-template-columns",
    "grid columns": "grid-template-columns",
    "gap": "gap",
    "spacing": "gap",
    
    # Positioning & Visuals
    "opacity": "opacity",
    "transform": "transform",
    "transition": "transition",
    "cursor": "cursor",
    "z-index": "z-index",
    "z index": "z-index",
    "layer": "z-index",
    "overflow-x": "overflow-x",
    "overflow x": "overflow-x",
    "overflow-y": "overflow-y",
    "overflow y": "overflow-y",
    "overflow": "overflow",
    "position": "position",
    "top": "top",
    "right": "right",
    "bottom": "bottom",
    "left": "left",
    "filter": "filter",
    "backdrop blur": "backdrop-filter",
    "backdrop-blur": "backdrop-filter",
    "backdrop-filter": "backdrop-filter",
    "user select": "user-select",
    "user-select": "user-select",
    "hover effect": "hover-effect",
    "hover color": "hover-color",
}

# Pseudo-states / pseudo-classes
STATE_MAPPINGS: Dict[str, str] = {
    "hovered": ":hover",
    "hover": ":hover",
    "focused": ":focus",
    "focus": ":focus",
    "active": ":active",
    "pressed": ":active",
    "visited": ":visited",
    "disabled": ":disabled",
    "first-child": ":first-child",
    "first child": ":first-child",
    "last-child": ":last-child",
    "last child": ":last-child",
    "checked": ":checked",
    "before": "::before",
    "after": "::after",
}

# Connectors that can link declarations or parameters seamlessly
CONNECTORS = {"and", "with", "also", "using"}

# Keywords recognized by the lexer
KEYWORDS = {
    "for", "apply", "style", "when", "is", "define", "as", "use",
    "end", "finish", "screen", "smaller", "larger", "than",
    "portrait", "landscape", "animation", "at", "degrees", "deg",
    "percent", "viewport-height", "vh", "viewport-width", "vw",
    "gradient", "from", "to", "color", "size", "duration", "easing",
    "becomes", "theme", "component", "global", "set", "pixels", "pixel",
    "via", "radial", "linear", "backdrop", "blur", "repeat", "sticky",
    "columns", "effect"
}
