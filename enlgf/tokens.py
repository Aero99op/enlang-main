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
    "table row": ("tr", {}),
    "table_row": ("tr", {}),
    "column heading": ("th", {}),
    "cell": ("td", {}),

    # ── Sovereign UI Primitives & Containers ──
    "create application": ("div", {"class": "application"}),
    "add application": ("div", {"class": "application"}),
    "application": ("div", {"class": "application"}),

    "create navbar": ("nav", {"class": "navbar"}),
    "add navbar": ("nav", {"class": "navbar"}),
    "navbar": ("nav", {"class": "navbar"}),

    "create nav_menu": ("ul", {"class": "nav-menu"}),
    "add nav_menu": ("ul", {"class": "nav-menu"}),
    "nav_menu": ("ul", {"class": "nav-menu"}),
    "create nav_links": ("ul", {"class": "nav-menu"}),
    "add nav_links": ("ul", {"class": "nav-menu"}),
    "nav_links": ("ul", {"class": "nav-menu"}),

    "add nav_link": ("a", {"class": "nav-link"}),
    "create nav_link": ("a", {"class": "nav-link"}),
    "nav_link": ("a", {"class": "nav-link"}),

    "add logo": ("div", {"class": "brand-logo"}),
    "create logo": ("div", {"class": "brand-logo"}),
    "logo": ("div", {"class": "brand-logo"}),

    "create action_group": ("div", {"class": "action-group"}),
    "add action_group": ("div", {"class": "action-group"}),
    "action_group": ("div", {"class": "action-group"}),

    "create container": ("div", {"class": "container"}),
    "add container": ("div", {"class": "container"}),
    "container": ("div", {"class": "container"}),

    "create screen": ("div", {"class": "screen"}),
    "add screen": ("div", {"class": "screen"}),
    "screen": ("div", {"class": "screen"}),

    "add readout": ("div", {"class": "readout"}),
    "create readout": ("div", {"class": "readout"}),
    "readout": ("div", {"class": "readout"}),

    "add badge": ("span", {"class": "badge"}),
    "create badge": ("span", {"class": "badge"}),
    "badge": ("span", {"class": "badge"}),

    "add heading": ("h2", {}),
    "create heading": ("h2", {}),
    "heading": ("h2", {}),

    "add subtitle": ("p", {"class": "subtitle"}),
    "create subtitle": ("p", {"class": "subtitle"}),
    "subtitle": ("p", {"class": "subtitle"}),

    "create grid": ("div", {"class": "grid"}),
    "add grid": ("div", {"class": "grid"}),
    "grid": ("div", {"class": "grid"}),

    "create card": ("div", {"class": "card"}),
    "add card": ("div", {"class": "card"}),
    "card": ("div", {"class": "card"}),

    "create panel": ("div", {"class": "panel"}),
    "add panel": ("div", {"class": "panel"}),
    "panel": ("div", {"class": "panel"}),

    "create controls": ("div", {"class": "controls"}),
    "add controls": ("div", {"class": "controls"}),
    "controls": ("div", {"class": "controls"}),

    "create row": ("div", {"class": "row"}),
    "add row": ("div", {"class": "row"}),
    "row": ("div", {"class": "row"}),

    "add value": ("div", {"class": "value"}),
    "create value": ("div", {"class": "value"}),
    "value": ("div", {"class": "value"}),

    "add note": ("p", {"class": "note"}),
    "create note": ("p", {"class": "note"}),
    "note": ("p", {"class": "note"}),

    "add section_tag": ("div", {"class": "section-tag"}),
    "create section_tag": ("div", {"class": "section-tag"}),
    "section_tag": ("div", {"class": "section-tag"}),

    "add section_title": ("h2", {"class": "section-title"}),
    "create section_title": ("h2", {"class": "section-title"}),
    "section_title": ("h2", {"class": "section-title"}),

    "add speech_bubble": ("div", {"class": "speech-bubble"}),
    "create speech_bubble": ("div", {"class": "speech-bubble"}),
    "speech_bubble": ("div", {"class": "speech-bubble"}),

    "add viewport": ("div", {"class": "viewport"}),
    "create viewport": ("div", {"class": "viewport"}),
    "viewport": ("div", {"class": "viewport"}),

    "add slider": ("input", {"type": "range", "class": "slider"}),
    "create slider": ("input", {"type": "range", "class": "slider"}),
    "slider": ("input", {"type": "range", "class": "slider"}),

    "add canvas": ("canvas", {}),
    "create canvas": ("canvas", {}),

    "display": ("div", {"class": "display-log"}),
    "show": ("div", {"class": "display-log"}),

    "use theme": ("link", {"rel": "stylesheet"}),
    "use style": ("link", {"rel": "stylesheet"}),
    "use stylesheet": ("link", {"rel": "stylesheet"}),
    "use script": ("script", {}),
    "use": ("link", {"rel": "stylesheet"}),

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

    "stylesheet from": ("link", {"rel": "stylesheet"}),
    "style from": ("link", {"rel": "stylesheet"}),
    "link style from": ("link", {"rel": "stylesheet"}),
    "include style from": ("link", {"rel": "stylesheet"}),

    "connect design": ("link", {"rel": "stylesheet"}),
    "connect style": ("link", {"rel": "stylesheet"}),
    "connect stylesheet": ("link", {"rel": "stylesheet"}),
    "connect styles": ("link", {"rel": "stylesheet"}),
    "link design": ("link", {"rel": "stylesheet"}),
    "link style": ("link", {"rel": "stylesheet"}),

    "inside styles": ("style", {}),
    "inside style": ("style", {}),
    "create styles": ("style", {}),
    "create style": ("style", {}),
    "styles": ("style", {}),
    "style": ("style", {}),

    # --- Standard HTML5 Tags ---
    "div": ("div", {}),
    "span": ("span", {}),
    "p": ("p", {}),
    "a": ("a", {}),
    "nav": ("nav", {}),
    "section": ("section", {}),
    "header": ("header", {}),
    "footer": ("footer", {}),
    "main": ("main", {}),
    "aside": ("aside", {}),
    "article": ("article", {}),
    "h1": ("h1", {}),
    "h2": ("h2", {}),
    "h3": ("h3", {}),
    "h4": ("h4", {}),
    "h5": ("h5", {}),
    "h6": ("h6", {}),
    "canvas": ("canvas", {}),
    "button": ("button", {}),
    "img": ("img", {}),
    "ul": ("ul", {}),
    "ol": ("ol", {}),
    "li": ("li", {}),
    "table": ("table", {}),
    "tr": ("tr", {}),
    "td": ("td", {}),
    "th": ("th", {}),
    "thead": ("thead", {}),
    "tbody": ("tbody", {}),
    "form": ("form", {}),
    "input": ("input", {}),
    "textarea": ("textarea", {}),
    "select": ("select", {}),
    "option": ("option", {}),
    "label": ("label", {}),
    "br": ("br", {}),
    "hr": ("hr", {}),
    "iframe": ("iframe", {}),
    "video": ("video", {}),
    "audio": ("audio", {}),
    "source": ("source", {}),
    "pre": ("pre", {}),
    "code": ("code", {}),
    "strong": ("strong", {}),
    "em": ("em", {}),
    "b": ("b", {}),
    "i": ("i", {}),

    "connect script": ("script", {}),
    "link script": ("script", {}),
    "script from": ("script", {}),
    "include script from": ("script", {}),
    "in script": ("script", {}),
    "inside script": ("script", {}),
    "create script": ("script", {}),
    "script": ("script", {}),
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

    "finish styles": "style",
    "end styles": "style",
    "finish style": "style",
    "end style": "style",

    "finish script": "script",
    "end script": "script",

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
    "container": "id",
    "default": "value",
    "text": "text",
    "href": "href",
    "src": "src",
    "target": "target",
    "style": "style",
    "rel": "rel",
    "charset": "charset",
    "name": "name",
    "content": "content",
    "title": "title",
    "action": "action",
    "method": "method",
    "placeholder": "placeholder",
    "value": "value",
    "min": "min",
    "max": "max",
    "step": "step",
    "required": "required",
    "disabled": "disabled",
    "readonly": "readonly",
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
