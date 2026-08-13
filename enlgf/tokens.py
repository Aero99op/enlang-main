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
    RAW_HTML = auto()    # Traditional HTML passthrough tags e.g. <div>...</div>
    END_BLOCK = auto()   # Explicit block closures e.g. end body, finish section
    EOF = auto()         # End of file

@dataclass
class Token:
    type: TokenType
    value: str
    line: int
    column: int

# English element phrases to HTML tags
TAG_MAPPINGS = {
    "document enlgf": ("html", {"lang": "en"}),
    "make document in english": ("html", {"lang": "en"}),
    "create document in english": ("html", {"lang": "en"}),
    "start document in english": ("html", {"lang": "en"}),
    "document in english": ("html", {"lang": "en"}),
    "make document": ("html", {}),
    "create document": ("html", {}),
    "start document": ("html", {}),
    "document": ("html", {}),

    "inside head": ("head", {}),
    "create head": ("head", {}),
    "in head": ("head", {}),
    "head": ("head", {}),

    "inside body": ("body", {}),
    "create body": ("body", {}),
    "in body": ("body", {}),
    "body": ("body", {}),

    "add title": ("title", {}),
    "create title": ("title", {}),
    "title": ("title", {}),

    "add heading 1": ("h1", {}),
    "create heading 1": ("h1", {}),
    "make heading 1": ("h1", {}),
    "heading 1": ("h1", {}),

    "add heading 2": ("h2", {}),
    "create heading 2": ("h2", {}),
    "make heading 2": ("h2", {}),
    "heading 2": ("h2", {}),

    "add heading 3": ("h3", {}),
    "create heading 3": ("h3", {}),
    "heading 3": ("h3", {}),
    "heading 4": ("h4", {}),
    "heading 5": ("h5", {}),
    "heading 6": ("h6", {}),

    "add paragraph": ("p", {}),
    "create paragraph": ("p", {}),
    "make paragraph": ("p", {}),
    "paragraph": ("p", {}),

    "inline": ("span", {}),
    "bold": ("strong", {}),
    "italic": ("em", {}),
    "line break": ("br", {}),
    "divider": ("hr", {}),
    "quote": ("blockquote", {}),
    "code": ("code", {}),
    "preformatted": ("pre", {}),

    "inside section": ("div", {}),
    "create section": ("div", {}),
    "add section": ("div", {}),
    "in section": ("div", {}),
    "section": ("div", {}),

    "inside header": ("header", {}),
    "create header": ("header", {}),
    "in header": ("header", {}),
    "header": ("header", {}),

    "inside footer": ("footer", {}),
    "create footer": ("footer", {}),
    "in footer": ("footer", {}),
    "footer": ("footer", {}),

    "inside main": ("main", {}),
    "create main": ("main", {}),
    "in main": ("main", {}),
    "main": ("main", {}),

    "create navigation": ("nav", {}),
    "navigation": ("nav", {}),

    "article": ("article", {}),
    "sidebar": ("aside", {}),
    "region": ("section", {}),

    "create bullet list": ("ul", {}),
    "bullet list": ("ul", {}),
    "create numbered list": ("ol", {}),
    "numbered list": ("ol", {}),
    "add item": ("li", {}),
    "create item": ("li", {}),
    "item": ("li", {}),

    "add link": ("a", {}),
    "create link": ("a", {}),
    "link": ("a", {}),

    "add image": ("img", {}),
    "create image": ("img", {}),
    "image": ("img", {}),

    "video": ("video", {}),
    "audio": ("audio", {}),
    "embed": ("iframe", {}),

    "create table": ("table", {}),
    "table": ("table", {}),
    "header row": ("thead", {}),
    "body rows": ("tbody", {}),
    "row": ("tr", {}),
    "column heading": ("th", {}),
    "cell": ("td", {}),

    "inside form": ("form", {}),
    "create form": ("form", {}),
    "in form": ("form", {}),
    "form": ("form", {}),

    "create text input": ("input", {"type": "text"}),
    "add text input": ("input", {"type": "text"}),
    "text input": ("input", {"type": "text"}),
    "email input": ("input", {"type": "email"}),
    "password input": ("input", {"type": "password"}),
    "number input": ("input", {"type": "number"}),
    "checkbox": ("input", {"type": "checkbox"}),
    "radio": ("input", {"type": "radio"}),
    "file upload": ("input", {"type": "file"}),
    "submit button": ("input", {"type": "submit"}),

    "add button": ("button", {}),
    "create button": ("button", {}),
    "button": ("button", {}),

    "dropdown": ("select", {}),
    "option": ("option", {}),
    "text area": ("textarea", {}),
    "label": ("label", {}),
}

# Explicit block closure phrases for Style 2
END_MAPPINGS = {
    "make document end": "html",
    "end document": "html",
    "finish document": "html",

    "finish head": "head",
    "end head": "head",

    "finish body": "body",
    "end body": "body",

    "finish section": "div",
    "end section": "div",

    "finish header": "header",
    "end header": "header",

    "finish footer": "footer",
    "end footer": "footer",

    "finish main": "main",
    "end main": "main",

    "finish form": "form",
    "end form": "form",

    "finish list": "ul",
    "end list": "ul",

    "finish table": "table",
    "end table": "table",
}

# Attribute English mappings
ATTR_MAPPINGS = {
    "to": "href",
    "pointing to": "href",
    "from": "src",
    "hint": "placeholder",
    "showing hint": "placeholder",
    "for": "for",
    "name": "name",
    "named": "name",
    "value": "value",
    "alt": "alt",
    "describing": "alt",
    "class": "class",
    "id": "id",
    "type": "type",
    "width": "width",
    "height": "height",
}

# Event / JS Inline Actions
EVENT_MAPPINGS = {
    "on click": "onclick",
    "when clicked": "onclick",
    "on submit": "onsubmit",
    "when submitted": "onsubmit",
    "on change": "onchange",
    "when changed": "onchange",
    "on hover": "onmouseover",
    "when hovered": "onmouseover",
}
