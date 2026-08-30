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

def _format_css_value(prop: str, raw_val: str) -> str:
    """Auto-appends px for bare numbers on length/size properties."""
    val = raw_val.strip()
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

            # 1. CSS Variables: define [type] "name" as [value]
            if t.type == TokenType.KEYWORD and t.value == "define":
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
            line_decls = self._parse_declaration_line()
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
        """Parses a single declaration or multiple declarations linked by connectors ('and', 'with', 'also')."""
        decls: List[DeclarationNode] = []

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
                prop_name = PROPERTY_MAPPINGS.get(prop_tok.value.lower(), prop_tok.value)

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

                    # Handle 'gradient from "A" to "B" at 135 degrees'
                    if tok.value == "gradient":
                        self._advance()
                        grad_val = self._parse_gradient_value()
                        val_parts.append(grad_val)
                        continue

                    # Handle 'percent', 'vh', 'vw', 'degrees' units following numbers
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

                    # Regular value token
                    self._advance()
                    v = tok.value.strip('"\'') if tok.type == TokenType.STRING else tok.value
                    val_parts.append(v)

                raw_val = " ".join(val_parts)
                final_val = _format_css_value(prop_name, raw_val)
                decls.append(DeclarationNode(property_name=prop_name, value=final_val))
            else:
                self._advance()

        return decls

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
