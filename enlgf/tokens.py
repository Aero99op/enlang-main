"""enlgf Token Types & Token Definitions.

Defines lexical token categories and English-to-HTML mapping dictionaries.
"""

from enum import Enum, auto
from dataclasses import dataclass

class TokenType(Enum):
    KEYWORD = auto()     # HTML tag/structure keywords (heading, paragraph, section, etc.)
    STRING = auto()      # "Literal text inside quotes"
    NUMBER = auto()      # Numbers for style attributes or heading levels (1, 20px, 100)
    IDENTIFIER = auto()  # Class names, IDs, variable names
    SYMBOL = auto()      # Symbols like :, ,, =, etc.
    INDENT = auto()      # Block indentation
    DEDENT = auto()      # Block dedent
    NEWLINE = auto()     # Newline character
    EOF = auto()         # End of file

@dataclass
class Token:
    type: TokenType
    value: str
    line: int
    column: int

# English element phrases to HTML tags
TAG_MAPPINGS = {
    "document in english": ("html", {"lang": "en"}),
    "document": ("html", {}),
    "head": ("head", {}),
    "body": ("body", {}),
    "title": ("title", {}),
    "heading 1": ("h1", {}),
    "heading 2": ("h2", {}),
    "heading 3": ("h3", {}),
    "heading 4": ("h4", {}),
    "heading 5": ("h5", {}),
    "heading 6": ("h6", {}),
    "paragraph": ("p", {}),
    "inline": ("span", {}),
    "bold": ("strong", {}),
    "italic": ("em", {}),
    "line break": ("br", {}),
    "divider": ("hr", {}),
    "quote": ("blockquote", {}),
    "code": ("code", {}),
    "preformatted": ("pre", {}),
    "section": ("div", {}),
    "header": ("header", {}),
    "footer": ("footer", {}),
    "main": ("main", {}),
    "navigation": ("nav", {}),
    "article": ("article", {}),
    "sidebar": ("aside", {}),
    "region": ("section", {}),
    "bullet list": ("ul", {}),
    "numbered list": ("ol", {}),
    "item": ("li", {}),
    "link": ("a", {}),
    "image": ("img", {}),
    "video": ("video", {}),
    "audio": ("audio", {}),
    "embed": ("iframe", {}),
    "table": ("table", {}),
    "header row": ("thead", {}),
    "body rows": ("tbody", {}),
    "row": ("tr", {}),
    "column heading": ("th", {}),
    "cell": ("td", {}),
    "form": ("form", {}),
    "text input": ("input", {"type": "text"}),
    "email input": ("input", {"type": "email"}),
    "password input": ("input", {"type": "password"}),
    "number input": ("input", {"type": "number"}),
    "checkbox": ("input", {"type": "checkbox"}),
    "radio": ("input", {"type": "radio"}),
    "file upload": ("input", {"type": "file"}),
    "submit button": ("input", {"type": "submit"}),
    "button": ("button", {}),
    "dropdown": ("select", {}),
    "option": ("option", {}),
    "text area": ("textarea", {}),
    "label": ("label", {}),
}

# Attribute English mappings
ATTR_MAPPINGS = {
    "to": "href",
    "from": "src",
    "hint": "placeholder",
    "for": "for",
    "name": "name",
    "value": "value",
    "alt": "alt",
    "class": "class",
    "id": "id",
    "type": "type",
    "width": "width",
    "height": "height",
}

# Event / JS Inline Actions
EVENT_MAPPINGS = {
    "on click": "onclick",
    "on submit": "onsubmit",
    "on change": "onchange",
    "on hover": "onmouseover",
}
