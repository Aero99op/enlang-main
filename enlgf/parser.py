"""enlgf Parser.

Transforms a stream of tokens into a structured ENLGF AST (DocumentNode, ElementNode, TextNode).
Supports nested indentation blocks, attributes, inline CSS styling, and JS inline actions.
"""

from typing import List, Tuple, Optional
from .tokens import Token, TokenType, TAG_MAPPINGS, EVENT_MAPPINGS, ATTR_MAPPINGS
from .ast_nodes import DocumentNode, ElementNode, TextNode, RawHTMLNode, ASTNode

class ENLEGFPParser:
    """Parses token stream into an ENLGF AST."""
    
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.pos = 0

    def parse(self) -> DocumentNode:
        doc = DocumentNode()
        
        while not self._is_at_end():
            node = self._parse_statement()
            if node:
                if isinstance(node, ElementNode) and node.tag == "head":
                    doc.head_children.extend(node.children)
                elif isinstance(node, ElementNode) and node.tag == "body":
                    doc.body_children.extend(node.children)
                elif isinstance(node, ElementNode) and node.tag == "html":
                    doc.attributes.update(node.attributes)
                    for child in node.children:
                        if isinstance(child, ElementNode) and child.tag == "head":
                            doc.head_children.extend(child.children)
                        elif isinstance(child, ElementNode) and child.tag == "body":
                            doc.body_children.extend(child.children)
                        else:
                            doc.children.append(child)
                else:
                    doc.children.append(node)
                    
        return doc

    def _parse_statement(self) -> Optional[ASTNode]:
        self._skip_newlines()
        if self._is_at_end() or self._peek().type in (TokenType.EOF, TokenType.DEDENT):
            return None

        t = self._peek()

        # Handle Explicit Block End Tokens (e.g. end body, finish section)
        if t.type == TokenType.END_BLOCK:
            return None

        # Handle Traditional Raw HTML Passthrough Tags (<...>)
        if t.type == TokenType.RAW_HTML:
            tok = self._advance()
            return RawHTMLNode(content=tok.value)

        # Handle Document Root
        if t.value in ("document enlgf", "document in english", "document", "make document in english", "create document in english", "start document in english", "make document", "create document", "start document"):
            self._advance()
            attrs = {"lang": "en"} if ("english" in t.value or "enlgf" in t.value) else {}
            children = self._parse_block()
            if not self._is_at_end() and self._peek().type == TokenType.END_BLOCK:
                self._advance()
            return ElementNode(tag="html", attributes=attrs, children=children)

        # Handle Tag Elements
        if t.value in TAG_MAPPINGS:
            return self._parse_element()

        # Handle Raw String Text
        if t.type == TokenType.STRING:
            token = self._advance()
            return TextNode(text=token.value)

        # Skip unknown tokens
        self._advance()
        return None

    def _parse_element(self) -> ElementNode:
        tag_token = self._advance()
        tag, default_attrs = TAG_MAPPINGS[tag_token.value]
        
        attributes = dict(default_attrs)
        styles = {}
        events = {}
        text_content = None
        children = []
        
        # Self-closing tags
        self_closing_tags = {"br", "hr", "img", "input", "meta", "link"}
        is_self_closing = tag in self_closing_tags
        
        # Parse inline tokens on the same line
        while not self._is_at_end() and self._peek().type not in (TokenType.NEWLINE, TokenType.EOF, TokenType.SYMBOL, TokenType.INDENT):
            t = self._peek()

            # String literal text content or attribute value
            if t.type == TokenType.STRING:
                val = self._advance().value
                if text_content is None:
                    text_content = val
                continue

            # Number token (e.g. heading levels, sizes)
            if t.type == TokenType.NUMBER:
                val = self._advance().value
                if tag.startswith("h") and len(tag) == 2:
                    tag = f"h{val}"
                elif text_content is None:
                    text_content = val
                continue

            val_lower = t.value.lower()

            # 1. Event Mappings (on click, on submit, etc.)
            if val_lower in EVENT_MAPPINGS:
                event_name = EVENT_MAPPINGS[val_lower]
                self._advance()
                if not self._is_at_end() and self._peek().type == TokenType.STRING:
                    events[event_name] = self._advance().value
                continue

            # 2. Attribute Mappings (to "url", from "src", hint "text", etc.)
            if val_lower in ATTR_MAPPINGS:
                attr_name = ATTR_MAPPINGS[val_lower]
                self._advance()
                if not self._is_at_end() and self._peek().type in (TokenType.STRING, TokenType.IDENTIFIER, TokenType.NUMBER):
                    attributes[attr_name] = self._advance().value
                continue

            # Special Attribute Shortcuts
            if val_lower == "new" and self._peek_next() and self._peek_next().value.lower() == "tab":
                self._advance()
                self._advance()
                attributes["target"] = "_blank"
                continue

            # 3. Inline CSS Style Shortcuts (color "red", size 20, margin 10, padding 5, width 100, height 50)
            if val_lower in ("color", "background", "margin", "padding", "width", "height", "size"):
                style_key = "font-size" if val_lower == "size" else val_lower
                self._advance()
                if not self._is_at_end() and self._peek().type in (TokenType.STRING, TokenType.NUMBER, TokenType.IDENTIFIER):
                    style_val = self._advance().value
                    if style_val.isdigit():
                        style_val += "px"
                    styles[style_key] = style_val
                continue

            if val_lower == "flex" and self._peek_next() and self._peek_next().value.lower() == "layout":
                self._advance()
                self._advance()
                styles["display"] = "flex"
                continue

            if val_lower == "grid" and self._peek_next() and self._peek_next().value.lower() == "layout":
                self._advance()
                self._advance()
                styles["display"] = "grid"
                continue

            # Fallback identifier
            self._advance()

        # Check for colon ':' indicating nested block
        if not self._is_at_end() and self._peek().type == TokenType.SYMBOL and self._peek().value == ":":
            self._advance()
            children = self._parse_block()

        return ElementNode(
            tag=tag,
            attributes=attributes,
            styles=styles,
            events=events,
            text_content=text_content,
            children=children,
            is_self_closing=is_self_closing
        )

    def _parse_block(self) -> List[ASTNode]:
        children = []
        self._skip_newlines()
        
        # Support both indented blocks and explicit delimiter blocks
        has_indent = not self._is_at_end() and self._peek().type == TokenType.INDENT
        if has_indent:
            self._advance() # Consume INDENT
            
        while not self._is_at_end():
            self._skip_newlines()
            if self._is_at_end() or self._peek().type in (TokenType.DEDENT, TokenType.END_BLOCK):
                break
            node = self._parse_statement()
            if node:
                children.append(node)
            self._skip_newlines()
                
        if not self._is_at_end() and self._peek().type == TokenType.DEDENT:
            self._advance() # Consume DEDENT
        elif not self._is_at_end() and self._peek().type == TokenType.END_BLOCK:
            self._advance() # Consume END_BLOCK

        return children

    def _skip_newlines(self):
        while not self._is_at_end() and self._peek().type == TokenType.NEWLINE:
            self._advance()

    def _peek(self) -> Token:
        return self.tokens[self.pos]

    def _peek_next(self) -> Optional[Token]:
        if self.pos + 1 < len(self.tokens):
            return self.tokens[self.pos + 1]
        return None

    def _advance(self) -> Token:
        token = self.tokens[self.pos]
        if self.pos < len(self.tokens) - 1:
            self.pos += 1
        return token

    def _is_at_end(self) -> bool:
        return self.pos >= len(self.tokens) or self.tokens[self.pos].type == TokenType.EOF
