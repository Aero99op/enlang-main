"""enlgd Parser.

Parses tokens into an ENLGD Stylesheet AST (StylesheetNode).
Supports:
- Variable definitions: define [color|size|value] "name" as [value]
- Rule blocks: for [selector] apply: ... end, style [selector]: ... end
- Pseudo states: when [selector] is [hovered|focused|active|...] apply: ... end
- Media queries: when screen is [smaller|larger] than [N] apply: ... end
- Connector words: multi-declaration chaining (and, with, also)
- Units & Shorthands: auto-px, percentages, viewport units (vh/vw), degrees
- Gradients: background gradient from [A] to [B] at [deg]
- Keyframe animations: animation [name]: at [N] percent: ... end
"""

from typing import List, Optional, Tuple
from .tokens import Token, TokenType, PROPERTY_MAPPINGS, STATE_MAPPINGS, CONNECTORS
from .ast_nodes import (
    StylesheetNode, RuleNode, DeclarationNode,
    MediaRuleNode, VariableNode, KeyframeNode, KeyframeFrameNode, ASTNode
)

# Properties that are typically unitless numbers
UNITLESS_PROPERTIES = {
    "line-height", "opacity", "z-index", "font-weight", "flex",
    "flex-grow", "flex-shrink", "order"
}

import re

def _format_css_value(prop: str, raw_val: str) -> str:
    """Cleans up CSS values: strips unwanted quotes, normalizes units, auto-px for numbers."""
    val = raw_val.strip()
    if not val:
        return val

    # Strip outer quotes if not content
    if prop != "content":
        if (val.startswith('"') and val.endswith('"')) or (val.startswith("'") and val.endswith("'")):
            val = val[1:-1].strip()

    # Normalization of english unit phrases
    val = val.replace("viewport-height", "vh").replace("viewport height", "vh")
    val = val.replace("viewport-width", "vw").replace("viewport width", "vw")
    val = val.replace("pixels", "px").replace("pixel", "px")
    val = val.replace("percent", "%").replace("percentage", "%")
    val = val.replace("degrees", "deg")

    # Fix space between number and unit (e.g. "100 vh" -> "100vh", "960 px" -> "960px", "100 %" -> "100%")
    val = re.sub(r'(\d+(?:\.\d+)?)\s*(px|vh|vw|%|deg|s|ms)\b', r'\1\2', val)

    # Unitless check
    if val.isdigit() and prop not in UNITLESS_PROPERTIES:
        return f"{val}px"
    return val

class ENLGDParser:
    """Builds a StylesheetNode AST from tokens."""

    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.pos = 0

    def parse(self) -> StylesheetNode:
        stylesheet = StylesheetNode()

        while not self._is_at_end():
            self._skip_newlines()
            if self._is_at_end():
                break

            t = self._peek()

            # 1. CSS Variables or Themes/Components
            if t.type == TokenType.KEYWORD and t.value == "define":
                next_tok = self._peek_offset(1)
                if next_tok and next_tok.value == "theme":
                    self._parse_theme_block(stylesheet)
                    continue
                elif next_tok and next_tok.value == "component":
                    comp_rules = self._parse_component_block()
                    if comp_rules:
                        stylesheet.rules.extend(comp_rules)
                    continue
                else:
                    var_node = self._parse_variable()
                    if var_node:
                        stylesheet.variables.append(var_node)
                    continue

            # 2. Keyframes: animation "name": ... end
            if t.type == TokenType.KEYWORD and t.value == "animation":
                kf_node = self._parse_keyframe()
                if kf_node:
                    stylesheet.keyframes.append(kf_node)
                continue

            # 3. Media Queries: when screen is ... apply:
            if t.type == TokenType.KEYWORD and t.value == "when" and self._peek_offset(1) and self._peek_offset(1).value == "screen":
                media_node = self._parse_media_query()
                if media_node:
                    stylesheet.media_rules.append(media_node)
                continue

            # 4. Pseudo-state Rules: when [selector] is [state] apply:
            if t.type == TokenType.KEYWORD and t.value == "when":
                rule_node = self._parse_when_rule()
                if rule_node:
                    stylesheet.rules.append(rule_node)
                continue

            # 5. Standard Rule Blocks: for [selector] apply: | style [selector]: | apply to [selector]:
            if (t.type == TokenType.KEYWORD and t.value in ("for", "style", "apply")) or t.type in (TokenType.STRING, TokenType.IDENTIFIER):
                rule_node = self._parse_rule()
                if rule_node:
                    stylesheet.rules.append(rule_node)
                continue

            # Skip unmatched token
            self._advance()

        return stylesheet

    # --- Parser Handlers ---

    def _parse_variable(self) -> Optional[VariableNode]:
        # define [type] "name" as [value]
        self._advance() # consume 'define'
        
        var_type = "value"
        if not self._is_at_end() and self._peek().value in ("color", "size", "spacing", "font", "value"):
            var_type = self._advance().value

        name = ""
        if not self._is_at_end() and self._peek().type in (TokenType.STRING, TokenType.IDENTIFIER):
            name = self._advance().value.strip('"\'')

        # consume 'as' or '=' if present
        if not self._is_at_end() and self._peek().value in ("as", "="):
            self._advance()

        # collect value tokens until end of line
        val_parts = []
        while not self._is_at_end() and self._peek().type not in (TokenType.NEWLINE, TokenType.EOF, TokenType.DEDENT):
            val_tok = self._advance()
            val_parts.append(val_tok.value.strip('"\'') if val_tok.type == TokenType.STRING else val_tok.value)

        val_str = " ".join(val_parts)
        if var_type == "size" and val_str.isdigit():
            val_str = f"{val_str}px"

        return VariableNode(var_type=var_type, name=name, value=val_str)

    def _parse_rule(self) -> Optional[RuleNode]:
        # for [selector] apply: | apply to [selector]: | style [selector]:
        t = self._peek()
        if t.type == TokenType.KEYWORD:
            kw = self._advance().value
            if kw == "apply" and not self._is_at_end() and self._peek().value == "to":
                self._advance() # consume 'to'

        # Extract Selector
        selector_parts = []
        while not self._is_at_end() and self._peek().type not in (TokenType.SYMBOL, TokenType.NEWLINE, TokenType.INDENT):
            tok = self._peek()
            if tok.value == "apply":
                self._advance()
                break
            selector_parts.append(tok.value.strip('"\'') if tok.type == TokenType.STRING else tok.value)
            self._advance()

        # Consume ':' if present
        if not self._is_at_end() and self._peek().type == TokenType.SYMBOL and self._peek().value == ":":
            self._advance()

        selector = " ".join(selector_parts).strip()
        declarations = self._parse_declarations_block()

        return RuleNode(selector=selector, declarations=declarations)

    def _parse_when_rule(self) -> Optional[RuleNode]:
        # when [selector] is [state] apply:
        self._advance() # consume 'when'
        
        selector_parts = []
        while not self._is_at_end() and self._peek().value not in ("is", "apply", ":"):
            tok = self._advance()
            selector_parts.append(tok.value.strip('"\'') if tok.type == TokenType.STRING else tok.value)

        # consume 'is'
        if not self._is_at_end() and self._peek().value == "is":
            self._advance()

        # extract state (e.g. hovered -> :hover)
        state = ":hover"
        if not self._is_at_end() and self._peek().type not in (TokenType.KEYWORD, TokenType.SYMBOL, TokenType.NEWLINE):
            state_tok = self._advance()
            raw_state = state_tok.value
            if raw_state.startswith(":"):
                state = raw_state
            else:
                state = STATE_MAPPINGS.get(raw_state.lower(), f":{raw_state}")

        # consume 'apply' or ':'
        if not self._is_at_end() and self._peek().value == "apply":
            self._advance()
        if not self._is_at_end() and self._peek().type == TokenType.SYMBOL and self._peek().value == ":":
            self._advance()

        base_sel = " ".join(selector_parts).strip()
        full_selector = f"{base_sel}{state}"
        declarations = self._parse_declarations_block()

        return RuleNode(selector=full_selector, declarations=declarations)

    def _parse_media_query(self) -> Optional[MediaRuleNode]:
        # when screen is smaller than 768 apply:
        # when screen is larger than 1200 apply:
        # when screen is portrait apply:
        self._advance() # consume 'when'
        if not self._is_at_end() and self._peek().value == "screen":
            self._advance() # consume 'screen'
        if not self._is_at_end() and self._peek().value == "is":
            self._advance() # consume 'is'

        query = "(max-width: 768px)"
        if not self._is_at_end():
            cond = self._advance().value.lower()
            if cond in ("smaller", "max"):
                if not self._is_at_end() and self._peek().value == "than":
                    self._advance() # consume 'than'
                val = self._advance().value if not self._is_at_end() else "768"
                val_formatted = f"{val}px" if val.isdigit() else val
                query = f"(max-width: {val_formatted})"
            elif cond in ("larger", "min"):
                if not self._is_at_end() and self._peek().value == "than":
                    self._advance() # consume 'than'
                val = self._advance().value if not self._is_at_end() else "1200"
                val_formatted = f"{val}px" if val.isdigit() else val
                query = f"(min-width: {val_formatted})"
            elif cond == "portrait":
                query = "(orientation: portrait)"
            elif cond == "landscape":
                query = "(orientation: landscape)"

        # consume 'apply' or ':'
        if not self._is_at_end() and self._peek().value == "apply":
            self._advance()
        if not self._is_at_end() and self._peek().type == TokenType.SYMBOL and self._peek().value == ":":
            self._advance()

        # Parse nested rules inside media query
        rules = []
        self._skip_newlines()
        if not self._is_at_end() and self._peek().type == TokenType.INDENT:
            self._advance()

        while not self._is_at_end():
            self._skip_newlines()
            if self._is_at_end():
                break
            if self._peek().value in ("end", "finish"):
                self._advance()
                break
            if self._peek().type == TokenType.DEDENT:
                self._advance()
                self._skip_newlines()
                if self._is_at_end() or self._peek().value in ("end", "finish"):
                    if not self._is_at_end() and self._peek().value in ("end", "finish"):
                        self._advance()
                    break
                if self._peek().type == TokenType.DEDENT:
                    self._advance()
                    break
                if self._peek().value not in ("for", "style", "apply", "when") and self._peek().type not in (TokenType.STRING, TokenType.IDENTIFIER):
                    break

            t = self._peek()
            if t.value == "when":
                rule = self._parse_when_rule()
                if rule:
                    rules.append(rule)
            elif (t.type == TokenType.KEYWORD and t.value in ("for", "style", "apply")) or t.type in (TokenType.STRING, TokenType.IDENTIFIER):
                rule = self._parse_rule()
                if rule:
                    rules.append(rule)
            else:
                self._advance()
            self._skip_newlines()

        return MediaRuleNode(query=query, rules=rules)

    def _parse_keyframe(self) -> Optional[KeyframeNode]:
        # animation "name":
        self._advance() # consume 'animation'
        name = "custom-anim"
        if not self._is_at_end() and self._peek().type in (TokenType.STRING, TokenType.IDENTIFIER):
            name = self._advance().value.strip('"\'')
        if not self._is_at_end() and self._peek().type == TokenType.SYMBOL and self._peek().value == ":":
            self._advance()

        frames = []
        self._skip_newlines()
        if not self._is_at_end() and self._peek().type == TokenType.INDENT:
            self._advance()

        while not self._is_at_end():
            self._skip_newlines()
            if self._is_at_end():
                break
            if self._peek().value in ("end", "finish"):
                self._advance()
                break
            if self._peek().type == TokenType.DEDENT:
                self._advance()
                self._skip_newlines()
                if self._is_at_end() or self._peek().value in ("end", "finish"):
                    if not self._is_at_end() and self._peek().value in ("end", "finish"):
                        self._advance()
                    break
                if self._peek().type == TokenType.DEDENT:
                    self._advance()
                    break
                if self._peek().value not in ("at", "from", "to") and self._peek().type != TokenType.NUMBER:
                    break

            # at N percent: / from: / to:
            stop = "0%"
            if not self._is_at_end() and self._peek().value == "at":
                self._advance() # consume 'at'
                num = self._advance().value if not self._is_at_end() else "0"
                if not self._is_at_end() and self._peek().value in ("percent", "%"):
                    self._advance()
                stop = f"{num}%" if not num.endswith("%") else num
            elif not self._is_at_end() and self._peek().value in ("from", "to"):
                stop = self._advance().value
            elif not self._is_at_end() and self._peek().type == TokenType.NUMBER:
                num = self._advance().value
                if not self._is_at_end() and self._peek().value in ("percent", "%"):
                    self._advance()
                stop = f"{num}%" if not num.endswith("%") else num

            if not self._is_at_end() and self._peek().type == TokenType.SYMBOL and self._peek().value == ":":
                self._advance()

            decls = self._parse_declarations_block()
            frames.append(KeyframeFrameNode(stop=stop, declarations=decls))
            self._skip_newlines()

        return KeyframeNode(name=name, frames=frames)

    def _parse_theme_block(self, stylesheet: StylesheetNode):
        self._advance() # consume 'define'
        self._advance() # consume 'theme'
        theme_name = ""
        if not self._is_at_end() and self._peek().type in (TokenType.STRING, TokenType.IDENTIFIER):
            theme_name = self._advance().value.strip('"\'')
        if not self._is_at_end() and self._peek().type == TokenType.SYMBOL and self._peek().value == ":":
            self._advance()

        self._skip_newlines()
        has_indent = not self._is_at_end() and self._peek().type == TokenType.INDENT
        if has_indent:
            self._advance()

        body_decls = []

        while not self._is_at_end():
            self._skip_newlines()
            if self._is_at_end():
                break
            if self._peek().type == TokenType.DEDENT:
                self._advance()
                break
            if self._peek().value in ("end", "finish"):
                self._advance()
                break

            if self._peek().type == TokenType.KEYWORD and self._peek().value == "define":
                if self._peek_offset(1) and self._peek_offset(1).value == "component":
                    comp_rules = self._parse_component_block()
                    if comp_rules:
                        stylesheet.rules.extend(comp_rules)
                    continue
                else:
                    var_node = self._parse_variable()
                    if var_node:
                        stylesheet.variables.append(var_node)
                    continue

            # Parse global rule lines inside theme
            line_decls, hover_rules = self._parse_declaration_line_with_hover("body")
            body_decls.extend(line_decls)
            if hover_rules:
                stylesheet.rules.extend(hover_rules)
            self._skip_newlines()

        if body_decls:
            stylesheet.rules.insert(0, RuleNode(selector="body", declarations=body_decls))

    def _parse_component_block(self) -> List[RuleNode]:
        self._advance() # consume 'define'
        self._advance() # consume 'component'
        comp_name = "component"
        if not self._is_at_end() and self._peek().type in (TokenType.STRING, TokenType.IDENTIFIER):
            comp_name = self._advance().value.strip('"\'')
        if not self._is_at_end() and self._peek().type == TokenType.SYMBOL and self._peek().value == ":":
            self._advance()

        std_tags = {"button", "header", "footer", "nav", "section", "main", "aside", "row", "card", "input", "body", "p", "a", "span", "div", "h1", "h2", "h3", "h4", "h5", "h6", "canvas", "table", "tr", "td", "th", "ul", "ol", "li"}
        if comp_name.lower() in std_tags:
            selector = f"{comp_name}, .{comp_name}, #{comp_name}"
        else:
            selector = f".{comp_name}, #{comp_name}"

        self._skip_newlines()
        has_indent = not self._is_at_end() and self._peek().type == TokenType.INDENT
        if has_indent:
            self._advance()

        decls = []
        hover_rules = []

        while not self._is_at_end():
            self._skip_newlines()
            if self._is_at_end():
                break
            if self._peek().type == TokenType.DEDENT:
                self._advance()
                break
            if self._peek().value in ("end", "finish"):
                self._advance()
                break

            line_decls, extra_rules = self._parse_declaration_line_with_hover(comp_name)
            decls.extend(line_decls)
            hover_rules.extend(extra_rules)
            self._skip_newlines()

        rules = []
        if decls:
            rules.append(RuleNode(selector=selector, declarations=decls))
        rules.extend(hover_rules)
        return rules

    def _parse_declarations_block(self) -> List[DeclarationNode]:
        """Parses declarations inside a rule, handling connectors, gradients, variables."""
        declarations: List[DeclarationNode] = []
        self._skip_newlines()

        has_indent = not self._is_at_end() and self._peek().type == TokenType.INDENT
        if has_indent:
            self._advance()

        while not self._is_at_end():
            self._skip_newlines()
            if self._is_at_end() or self._peek().type == TokenType.DEDENT:
                break
            if self._peek().value in ("end", "finish"):
                self._advance()
                break

            # Parse line with possible connector chaining
            line_decls, _ = self._parse_declaration_line_with_hover("element")
            declarations.extend(line_decls)
            self._skip_newlines()

        if has_indent and not self._is_at_end() and self._peek().type == TokenType.DEDENT:
            self._advance()

        # Consume trailing 'end' or 'finish' for this block if present
        self._skip_newlines()
        if not self._is_at_end() and self._peek().value in ("end", "finish"):
            self._advance()

        return declarations

    def _parse_declaration_line(self) -> List[DeclarationNode]:
        """Parses a single declaration line."""
        decls, _ = self._parse_declaration_line_with_hover("element")
        return decls

    def _parse_declaration_line_with_hover(self, base_comp: str) -> Tuple[List[DeclarationNode], List[RuleNode]]:
        """Parses a declaration line, extracting declarations and any companion pseudo-state rules."""
        decls: List[DeclarationNode] = []
        hover_rules: List[RuleNode] = []

        # Consume leading 'set' if present
        if not self._is_at_end() and self._peek().type == TokenType.KEYWORD and self._peek().value == "set":
            self._advance()

        while not self._is_at_end() and self._peek().type not in (TokenType.NEWLINE, TokenType.EOF, TokenType.DEDENT):
            t = self._peek()

            # End block keyword
            if t.value in ("end", "finish"):
                break

            # Skip connector if at start of chain segment
            if t.type == TokenType.CONNECTOR:
                self._advance()
                continue

            # Must be a property or identifier
            if t.type in (TokenType.PROPERTY, TokenType.IDENTIFIER, TokenType.KEYWORD):
                prop_tok = self._advance()
                raw_prop_name = prop_tok.value.lower()
                prop_name = PROPERTY_MAPPINGS.get(raw_prop_name, prop_tok.value)

                # Check if next token is 'to' or ':' or '='
                if not self._is_at_end() and self._peek().value in ("to", ":", "="):
                    self._advance()

                # Special: Hover Color
                if prop_name == "hover-color":
                    col_tok = self._advance() if not self._is_at_end() else Token(TokenType.STRING, "#fff", 0, 0)
                    col_val = col_tok.value.strip('"\'')
                    hover_rules.append(RuleNode(selector=f".{base_comp}:hover, #{base_comp}:hover", declarations=[DeclarationNode(property_name="color", value=col_val)]))
                    continue

                # Special: Hover Effect
                if prop_name == "hover-effect":
                    eff_tok = self._advance() if not self._is_at_end() else Token(TokenType.STRING, "", 0, 0)
                    eff_str = eff_tok.value.strip('"\'')
                    hover_decls = []
                    segments = re.split(r'\s+and\s+|,', eff_str)
                    for seg in segments:
                        seg = seg.strip()
                        if not seg:
                            continue
                        if seg.startswith("scale "):
                            hover_decls.append(DeclarationNode("transform", f"scale({seg[6:].strip()})"))
                        elif seg.startswith("transform "):
                            hover_decls.append(DeclarationNode("transform", seg[10:].strip()))
                        elif seg.startswith("background "):
                            hover_decls.append(DeclarationNode("background", seg[11:].strip()))
                        elif seg.startswith("border-color ") or seg.startswith("border color "):
                            hover_decls.append(DeclarationNode("border-color", seg.split(None, 1)[1].strip()))
                        elif seg.startswith("color "):
                            hover_decls.append(DeclarationNode("color", seg[6:].strip()))
                        else:
                            parts = seg.split(None, 1)
                            if len(parts) == 2:
                                hover_decls.append(DeclarationNode(parts[0], parts[1]))
                    if hover_decls:
                        hover_rules.append(RuleNode(selector=f".{base_comp}:hover, #{base_comp}:hover", declarations=hover_decls))
                    continue

                # Parse value for this property
                val_parts = []
                while not self._is_at_end() and self._peek().type not in (TokenType.NEWLINE, TokenType.EOF, TokenType.DEDENT, TokenType.CONNECTOR):
                    tok = self._peek()

                    # Handle 'use [color|size|value] "name"' -> var(--name)
                    if tok.value == "use":
                        self._advance()
                        if not self._is_at_end() and self._peek().value in ("color", "size", "spacing", "font", "value"):
                            self._advance()
                        var_name = self._advance().value.strip('"\'') if not self._is_at_end() else "var"
                        val_parts.append(f"var(--{var_name})")
                        continue

                    # Handle 'gradient ...'
                    if tok.value == "gradient":
                        self._advance()
                        if not self._is_at_end() and self._peek().type == TokenType.STRING:
                            grad_str = self._advance().value.strip('"\'')
                            if "radial from" in grad_str:
                                m = re.search(r'radial\s+from\s+([^\s]+)\s+at\s+([^\s]+(?:\s+[^\s]+)?)\s+to\s+([^\s]+)', grad_str)
                                if m:
                                    val_parts.append(f"radial-gradient(circle at {m.group(2)}, {m.group(1)}, {m.group(3)})")
                                else:
                                    val_parts.append(f"radial-gradient({grad_str})")
                            elif "linear from" in grad_str:
                                val_parts.append(self._parse_linear_str(grad_str))
                            else:
                                val_parts.append(grad_str)
                        else:
                            grad_val = self._parse_gradient_value()
                            val_parts.append(grad_val)
                        continue

                    # Units
                    if tok.value in ("percent", "%"):
                        self._advance()
                        if val_parts and val_parts[-1].isdigit():
                            val_parts[-1] = f"{val_parts[-1]}%"
                        continue

                    if tok.value in ("viewport-height", "vh"):
                        self._advance()
                        if val_parts and val_parts[-1].isdigit():
                            val_parts[-1] = f"{val_parts[-1]}vh"
                        continue

                    if tok.value in ("viewport-width", "vw"):
                        self._advance()
                        if val_parts and val_parts[-1].isdigit():
                            val_parts[-1] = f"{val_parts[-1]}vw"
                        continue

                    if tok.value in ("degrees", "deg"):
                        self._advance()
                        if val_parts and val_parts[-1].isdigit():
                            val_parts[-1] = f"{val_parts[-1]}deg"
                        continue

                    if tok.value in ("pixels", "pixel"):
                        self._advance()
                        if val_parts and val_parts[-1].isdigit():
                            val_parts[-1] = f"{val_parts[-1]}px"
                        continue

                    # Sticky at top 0
                    if prop_name == "position" and tok.value == "at":
                        self._advance() # consume 'at'
                        if not self._is_at_end() and self._peek().value in ("top", "bottom", "left", "right"):
                            edge = self._advance().value
                            num = self._advance().value if not self._is_at_end() else "0"
                            num_formatted = f"{num}px" if num.isdigit() else num
                            decls.append(DeclarationNode(property_name=edge, value=num_formatted))
                        continue

                    # Regular value token
                    self._advance()
                    v = tok.value.strip('"\'') if tok.type == TokenType.STRING else tok.value
                    val_parts.append(v)

                # Shorthand deductions when val_parts is empty or repeated property name
                if not val_parts or val_parts == [prop_name]:
                    if prop_name == "align-items":
                        val_parts = ["center"]
                    elif prop_name == "justify-content":
                        val_parts = ["space-between"]

                # Text Gradient
                if prop_name == "text-gradient":
                    grad_raw = " ".join(val_parts)
                    grad_css = self._parse_linear_str(grad_raw)
                    decls.append(DeclarationNode(property_name="background", value=grad_css))
                    decls.append(DeclarationNode(property_name="-webkit-background-clip", value="text"))
                    decls.append(DeclarationNode(property_name="-webkit-text-fill-color", value="transparent"))
                    continue

                # Backdrop filter
                if prop_name == "backdrop-filter":
                    raw_val = " ".join(val_parts)
                    raw_val = raw_val.replace("blur", "").strip()
                    val = _format_css_value("backdrop-filter", raw_val)
                    decls.append(DeclarationNode(property_name="backdrop-filter", value=f"blur({val})"))
                    decls.append(DeclarationNode(property_name="-webkit-backdrop-filter", value=f"blur({val})"))
                    continue

                # Grid columns
                if prop_name == "grid-template-columns":
                    raw_val = " ".join(val_parts)
                    if raw_val.isdigit():
                        raw_val = f"repeat({raw_val}, minmax(0, 1fr))"
                    decls.append(DeclarationNode(property_name="grid-template-columns", value=raw_val))
                    continue

                raw_val = " ".join(val_parts)
                final_val = _format_css_value(prop_name, raw_val)
                decls.append(DeclarationNode(property_name=prop_name, value=final_val))
            else:
                self._advance()

        return decls, hover_rules

    def _parse_linear_str(self, grad_raw: str) -> str:
        grad_raw = grad_raw.strip('"\'')
        if "via" in grad_raw:
            m = re.search(r'linear\s+from\s+([^\s]+)\s+via\s+([^\s]+)\s+to\s+([^\s]+)', grad_raw)
            if m:
                return f"linear-gradient(to right, {m.group(1)}, {m.group(2)}, {m.group(3)})"
        elif "from" in grad_raw and "to" in grad_raw:
            m = re.search(r'linear\s+from\s+([^\s]+)\s+to\s+([^\s]+)', grad_raw)
            if m:
                return f"linear-gradient(to right, {m.group(1)}, {m.group(2)})"
        return f"linear-gradient({grad_raw})"

    def _parse_gradient_value(self) -> str:
        # from "#1e3c72" to "#2a5298" at 135 degrees
        color1 = "#1e3c72"
        color2 = "#2a5298"
        deg = "135deg"

        while not self._is_at_end() and self._peek().type not in (TokenType.NEWLINE, TokenType.EOF, TokenType.DEDENT, TokenType.CONNECTOR):
            tok = self._advance()
            if tok.value == "from" and not self._is_at_end():
                color1 = self._advance().value.strip('"\'')
            elif tok.value == "to" and not self._is_at_end():
                color2 = self._advance().value.strip('"\'')
            elif tok.value in ("at", "degrees", "deg") and not self._is_at_end():
                num = self._advance().value
                if not self._is_at_end() and self._peek().value in ("degrees", "deg"):
                    self._advance()
                deg = f"{num}deg" if not num.endswith("deg") else num

        return f"linear-gradient({deg}, {color1}, {color2})"

    def _skip_newlines(self):
        while not self._is_at_end() and self._peek().type == TokenType.NEWLINE:
            self._advance()

    def _peek(self) -> Token:
        return self.tokens[self.pos]

    def _peek_offset(self, offset: int) -> Optional[Token]:
        if self.pos + offset < len(self.tokens):
            return self.tokens[self.pos + offset]
        return None

    def _advance(self) -> Token:
        tok = self.tokens[self.pos]
        if self.pos < len(self.tokens) - 1:
            self.pos += 1
        return tok

    def _is_at_end(self) -> bool:
        return self.pos >= len(self.tokens) or self.tokens[self.pos].type == TokenType.EOF
