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

    def _parse_element(self) -> ASTNode:
        tag_token = self._advance()
        tag, default_attrs = TAG_MAPPINGS[tag_token.value]
        
        # Handle Embedded Style Block: styles: / style:
        if tag == "style":
            return self._parse_style_block()

        # Handle Embedded Script Block: in script: / inside script: / script:
        if tag == "script" and tag_token.value in ("in script", "inside script", "script", "create script"):
            return self._parse_script_block()

        attributes = dict(default_attrs)
        styles = {}
        events = {}
        text_content = None
        children = []
        
        # Self-closing tags
        self_closing_tags = {"br", "hr", "img", "input", "meta", "link"}
        is_self_closing = tag in self_closing_tags
        
        # Inline property mappings for full .enlgd styling compatibility
        inline_prop_map = {
            "color": "color",
            "text color": "color",
            "background": "background",
            "background color": "background-color",
            "bg": "background",
            "font": "font-family",
            "font family": "font-family",
            "font-family": "font-family",
            "font-size": "font-size",
            "font size": "font-size",
            "size": "font-size",
            "text size": "font-size",
            "font-weight": "font-weight",
            "font weight": "font-weight",
            "weight": "font-weight",
            "border-radius": "border-radius",
            "border radius": "border-radius",
            "corner radius": "border-radius",
            "radius": "border-radius",
            "border": "border",
            "shadow": "box-shadow",
            "box-shadow": "box-shadow",
            "box shadow": "box-shadow",
            "padding": "padding",
            "margin": "margin",
            "width": "width",
            "height": "height",
            "max-width": "max-width",
            "max width": "max-width",
            "min-width": "min-width",
            "min width": "min-width",
            "display": "display",
            "opacity": "opacity",
            "transform": "transform",
            "cursor": "cursor",
            "transition": "transition",
            "text-align": "text-align",
            "text align": "text-align",
            "align text": "text-align",
            "align": "align-items",
            "justify": "justify-content",
            "gap": "gap",
            "spacing": "gap",
            "line-height": "line-height",
            "line height": "line-height",
        }

        # Parse inline tokens on the same line
        while not self._is_at_end() and self._peek().type not in (TokenType.NEWLINE, TokenType.EOF, TokenType.SYMBOL, TokenType.INDENT):
            t = self._peek()

            # Skip connector words (with, and, also, using)
            if t.value.lower() in ("with", "and", "also", "using"):
                self._advance()
                continue

            # Handle unquoted stylesheet/script filename identifiers on link/script elements
            if tag in ("link", "script") and t.type in (TokenType.STRING, TokenType.IDENTIFIER):
                val = t.value.strip('"\'')
                if val.endswith(".enlgd") or val.endswith(".css"):
                    attributes["href"] = val
                    self._advance()
                    continue
                elif val.endswith(".enlgs") or val.endswith(".js"):
                    attributes["src"] = val
                    self._advance()
                    continue

            # String literal text content or attribute value
            if t.type == TokenType.STRING:
                val = self._advance().value
                if tag == "link" and "href" not in attributes:
                    attributes["href"] = val
                elif text_content is None:
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

            # 3. Rich Inline CSS Properties (.enlgd compatibility)
            # Check two-word properties first
            two_word = f"{val_lower} {self._peek_next().value.lower()}" if self._peek_next() else ""
            if two_word in inline_prop_map:
                css_prop = inline_prop_map[two_word]
                self._advance() # first word
                self._advance() # second word
                style_val = self._parse_inline_style_value(css_prop)
                if style_val:
                    styles[css_prop] = style_val
                continue

            if val_lower in inline_prop_map:
                css_prop = inline_prop_map[val_lower]
                self._advance()
                style_val = self._parse_inline_style_value(css_prop)
                if style_val:
                    styles[css_prop] = style_val
                continue

            # Flex/Grid layout shortcuts
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

        # Normalize <link> attributes: if 'src' was set from "style from ...", convert to 'href'
        if tag == "link":
            if "src" in attributes and "href" not in attributes:
                attributes["href"] = attributes.pop("src")

        return ElementNode(
            tag=tag,
            attributes=attributes,
            styles=styles,
            events=events,
            text_content=text_content,
            children=children,
            is_self_closing=is_self_closing
        )

    def _parse_inline_style_value(self, css_prop: str) -> str:
        """Parses a single or multi-part inline style value."""
        val_parts = []
        unitless_props = {"line-height", "opacity", "z-index", "font-weight"}

        while not self._is_at_end() and self._peek().type not in (TokenType.NEWLINE, TokenType.EOF, TokenType.SYMBOL, TokenType.INDENT):
            t = self._peek()
            val_lower = t.value.lower()
            if val_lower in ("with", "and", "also", "using"):
                break
            if val_lower in ATTR_MAPPINGS or val_lower in EVENT_MAPPINGS:
                break
            # If next is another known property keyword, break
            if val_lower in ("color", "background", "font", "padding", "margin", "border", "shadow", "radius", "corner", "width", "height", "display", "cursor", "text", "size"):
                if val_parts: # only break if we already captured at least one value part
                    break

            tok = self._advance()
            val = tok.value.strip('"\'') if tok.type == TokenType.STRING else tok.value
            val_parts.append(val)

        res = " ".join(val_parts).strip()
        if res.isdigit() and css_prop not in unitless_props:
            res += "px"
        return res

    def _parse_style_block(self) -> ASTNode:
        """Parses an inner/embedded .enlgd style block inside .enlgf."""
        if not self._is_at_end() and self._peek().type == TokenType.SYMBOL and self._peek().value == ":":
            self._advance()

        style_tokens = []
        has_indent = not self._is_at_end() and self._peek().type == TokenType.INDENT
        if has_indent:
            self._advance()

        while not self._is_at_end():
            if self._peek().type == TokenType.DEDENT:
                break
            if self._peek().type == TokenType.END_BLOCK and self._peek().value == "style":
                self._advance()
                break
            if self._peek().value in ("end", "finish"):
                self._advance()
                if not self._is_at_end() and self._peek().value in ("style", "styles"):
                    self._advance()
                break
            style_tokens.append(self._advance())

        if has_indent and not self._is_at_end() and self._peek().type == TokenType.DEDENT:
            self._advance()

        # Reconstruct style source and compile via enlgd
        lines = []
        cur_line = []
        for t in style_tokens:
            if t.type == TokenType.NEWLINE:
                lines.append(" ".join(cur_line))
                cur_line = []
            elif t.type == TokenType.INDENT:
                cur_line.append("    ")
            elif t.type == TokenType.DEDENT:
                pass
            elif t.type == TokenType.STRING:
                cur_line.append(f'"{t.value}"')
            else:
                cur_line.append(t.value)
        if cur_line:
            lines.append(" ".join(cur_line))

        style_src = "\n".join(lines)
        try:
            from enlgd.compiler import compile_enlgd_source
            compiled_css = compile_enlgd_source(style_src)
        except Exception as e:
            compiled_css = f"/* Style compilation error: {e} */"

        return RawHTMLNode(content=f"<style>\n{compiled_css}\n</style>")

    def _parse_script_block(self) -> ASTNode:
        """Parses an inner/embedded .enlgs script block inside .enlgf."""
        if not self._is_at_end() and self._peek().type == TokenType.SYMBOL and self._peek().value == ":":
            self._advance()

        script_tokens = []
        has_indent = not self._is_at_end() and self._peek().type == TokenType.INDENT
        if has_indent:
            self._advance()

        while not self._is_at_end():
            if self._peek().type == TokenType.DEDENT:
                break
            if self._peek().type == TokenType.END_BLOCK and self._peek().value == "script":
                self._advance()
                break
            if self._peek().value in ("end", "finish"):
                self._advance()
                if not self._is_at_end() and self._peek().value in ("script", "scripts"):
                    self._advance()
                break
            script_tokens.append(self._advance())

        if has_indent and not self._is_at_end() and self._peek().type == TokenType.DEDENT:
            self._advance()

        # Reconstruct script source and compile via enlgs
        lines = []
        cur_line = []
        for t in script_tokens:
            if t.type == TokenType.NEWLINE:
                lines.append(" ".join(cur_line))
                cur_line = []
            elif t.type == TokenType.INDENT:
                cur_line.append("    ")
            elif t.type == TokenType.DEDENT:
                pass
            elif t.type == TokenType.STRING:
                cur_line.append(f'"{t.value}"')
            else:
                cur_line.append(t.value)
        if cur_line:
            lines.append(" ".join(cur_line))

        script_src = "\n".join(lines)
        try:
            from enlgs.compiler import compile_enlgs_source
            compiled_js = compile_enlgs_source(script_src)
        except Exception as e:
            compiled_js = f"/* Script compilation error: {e} */"

        return RawHTMLNode(content=f"<script>\n{compiled_js}\n</script>")

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
