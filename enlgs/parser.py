"""enlgs Parser.

Parses tokens into an ENLGS AST (ScriptNode).
Implements Hint-Word Intent Discovery with automatic silent connector skipping,
symbolic operator support, and automatic raw JS fallback passthrough.
"""

from typing import List, Optional, Tuple, Dict
from .tokens import Token, TokenType, HINT_REGISTRY, CONNECTORS
from .ast_nodes import (
    ASTNode, ScriptNode, RawJSNode, VarDeclNode, VarAssignNode, OutputNode,
    FunctionDefNode, FunctionCallNode, ReturnNode, IfNode,
    LoopRepeatNode, LoopForNode, LoopWhileNode, DOMSetNode, DOMRefreshNode,
    DOMClassNode, DOMVisibilityNode, EventNode, FetchNode, TimerNode,
    BrowserActionNode, StorageNode, TryCatchNode, ClassDefNode, PreventDefaultNode
)

def _tokens_to_expr(tokens: List[Token]) -> str:
    """Converts a token subslice to a clean JavaScript expression string."""
    i = 0
    parts = []
    length = len(tokens)
    
    while i < length:
        t = tokens[i]
        
        # Check for multi-word hint tokens: 'get value', 'get text', 'get element', or 'get'
        t_val = t.value.lower()
        if t_val in ("get value", "get text", "get element", "get"):
            target_idx = i + 1
            if t_val == "get" and target_idx < length and tokens[target_idx].value.lower() in ("value", "text", "element"):
                t_val = f"get {tokens[target_idx].value.lower()}"
                target_idx += 1
            if target_idx < length and tokens[target_idx].value.lower() == "of":
                target_idx += 1
            if target_idx < length:
                target_val = tokens[target_idx].value.strip('"\'')
                if t_val == "get value":
                    parts.append(f'(document.getElementById("{target_val}") || document.querySelector("{target_val}")).value')
                elif t_val == "get text":
                    parts.append(f'(document.getElementById("{target_val}") || document.querySelector("{target_val}")).textContent')
                else:
                    parts.append(f'(document.getElementById("{target_val}") || document.querySelector("{target_val}"))')
                i = target_idx + 1
                continue
        
        if t.type == TokenType.STRING:
            escaped = t.value.replace('"', '\\"')
            parts.append(f'"{escaped}"')
        elif t.type == TokenType.CONNECTOR:
            if t.value in ("and", "or"):
                parts.append("&&" if t.value == "and" else "||")
            elif t.value in ("is", "are"):
                parts.append("===")
            else:
                pass # skip silent connector
        elif t.type == TokenType.SYMBOL and t.value == ";":
            pass # skip semicolon inside expressions
        else:
            parts.append(t.value)
        i += 1
        
    return " ".join(parts).strip()

def _parse_time_to_ms(val_str: str) -> str:
    """Converts time string (e.g. 3, 3s, 500ms, 1 second, 2 minutes) to milliseconds."""
    val = val_str.lower().strip()
    if val.endswith("ms"):
        return val[:-2].strip()
    elif val.endswith("s") and not val.endswith("ms"):
        num = val[:-1].strip()
        return str(int(float(num) * 1000)) if num.replace('.', '', 1).isdigit() else f"({num} * 1000)"
    elif val.isdigit():
        num = int(val)
        return str(num * 1000) if num <= 60 else str(num)
    return val

class ENLGSParser:
    """Builds a ScriptNode AST from tokens."""

    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.pos = 0

    def parse(self) -> ScriptNode:
        root = ScriptNode()

        # Check for top-level domain declaration: 'in script:' / 'script enlgs:'
        self._skip_newlines()
        if not self._is_at_end() and self._peek().type == TokenType.HINT:
            intent = HINT_REGISTRY.get(self._peek().value.lower())
            if intent == "DOMAIN_DECL":
                self._advance() # consume declaration
                if not self._is_at_end() and self._peek().type == TokenType.SYMBOL and self._peek().value == ":":
                    self._advance()
                root.body = self._parse_block()
                return root

        # Flat script parsing
        root.body = self._parse_statements_until_end()
        return root

    def _parse_block(self) -> List[ASTNode]:
        """Parses an indented or delimiter-closed block of statements."""
        statements: List[ASTNode] = []
        self._skip_newlines()

        has_indent = not self._is_at_end() and self._peek().type == TokenType.INDENT
        if has_indent:
            self._advance()

        while not self._is_at_end():
            self._skip_newlines()
            if self._is_at_end() or self._peek().type == TokenType.DEDENT:
                break
            if self._peek().value.lower() in ("end", "finish", "end script", "finish script", "done"):
                self._advance()
                if not self._is_at_end() and self._peek().value.lower() in ("script", "function", "if", "loop", "when"):
                    self._advance()
                break

            stmt = self._parse_statement()
            if stmt:
                statements.append(stmt)
            self._skip_newlines()

        if has_indent and not self._is_at_end() and self._peek().type == TokenType.DEDENT:
            self._advance()

        return statements

    def _parse_statements_until_end(self) -> List[ASTNode]:
        statements: List[ASTNode] = []
        while not self._is_at_end():
            self._skip_newlines()
            if self._is_at_end():
                break
            stmt = self._parse_statement()
            if stmt:
                statements.append(stmt)
            self._skip_newlines()
        return statements

    def _parse_statement(self) -> Optional[ASTNode]:
        self._skip_newlines()
        if self._is_at_end():
            return None

        # Collect tokens on current line
        line_tokens = self._collect_line_tokens()
        if not line_tokens:
            return None

        first_tok = line_tokens[0]
        raw_line_text = first_tok.raw_text

        # 1. Check if first token is a recognized HINT
        intent = None
        if first_tok.type == TokenType.HINT:
            intent = HINT_REGISTRY.get(first_tok.value.lower())
        elif first_tok.value.lower() in HINT_REGISTRY:
            intent = HINT_REGISTRY[first_tok.value.lower()]

        if intent:
            return self._parse_intent_statement(intent, line_tokens)

        # 2. Check for implicit symbolic assignment: x = 10, x += 1, count++
        if len(line_tokens) >= 3 and line_tokens[1].type == TokenType.OPERATOR and line_tokens[1].value in ("=", "+=", "-=", "*=", "/="):
            name = line_tokens[0].value
            op = line_tokens[1].value
            val = _tokens_to_expr(line_tokens[2:])
            return VarAssignNode(name=name, op=op, value=val)

        # 3. Fallback: Auto Raw JavaScript Passthrough
        return RawJSNode(code=raw_line_text)

    def _parse_intent_statement(self, intent: str, line_tokens: List[Token]) -> Optional[ASTNode]:
        tokens = line_tokens[1:] # strip hint token

        # ── Variables ──
        if intent == "DECLARE_VAR":
            # create [name] as [val] / let [name] = [val]
            tokens_no_conn = [t for t in tokens if t.type != TokenType.CONNECTOR or t.value in ("=",)]
            if not tokens_no_conn:
                return RawJSNode(code=line_tokens[0].raw_text)
            name = tokens_no_conn[0].value
            val = _tokens_to_expr(tokens_no_conn[1:]) if len(tokens_no_conn) > 1 else None
            if val and val.startswith("="):
                val = val[1:].strip()
            return VarDeclNode(kind="let", name=name, value=val)

        elif intent == "DECLARE_CONST":
            # define [name] as [val] / const [name] = [val]
            tokens_no_conn = [t for t in tokens if t.type != TokenType.CONNECTOR or t.value in ("=",)]
            if not tokens_no_conn:
                return RawJSNode(code=line_tokens[0].raw_text)
            name = tokens_no_conn[0].value
            val = _tokens_to_expr(tokens_no_conn[1:]) if len(tokens_no_conn) > 1 else None
            if val and val.startswith("="):
                val = val[1:].strip()
            return VarDeclNode(kind="const", name=name, value=val)

        elif intent == "ASSIGN_VAR":
            # set [name] as [val] / set [name] = [val]
            tokens_no_conn = [t for t in tokens if t.type != TokenType.CONNECTOR or t.value in ("=",)]
            if not tokens_no_conn:
                return RawJSNode(code=line_tokens[0].raw_text)
            name = tokens_no_conn[0].value
            val = _tokens_to_expr(tokens_no_conn[1:]) if len(tokens_no_conn) > 1 else "null"
            if val.startswith("="):
                val = val[1:].strip()
            return VarAssignNode(name=name, op="=", value=val)

        elif intent == "COMPOUND_ADD":
            # increase [name] by [N]
            tokens_no_conn = [t for t in tokens if t.type != TokenType.CONNECTOR]
            name = tokens_no_conn[0].value if tokens_no_conn else "x"
            val = _tokens_to_expr(tokens_no_conn[1:]) if len(tokens_no_conn) > 1 else "1"
            return VarAssignNode(name=name, op="+=", value=val)

        elif intent == "COMPOUND_SUB":
            # decrease [name] by [N]
            tokens_no_conn = [t for t in tokens if t.type != TokenType.CONNECTOR]
            name = tokens_no_conn[0].value if tokens_no_conn else "x"
            val = _tokens_to_expr(tokens_no_conn[1:]) if len(tokens_no_conn) > 1 else "1"
            return VarAssignNode(name=name, op="-=", value=val)

        elif intent == "COMPOUND_MUL":
            # multiply [name] by [N]
            tokens_no_conn = [t for t in tokens if t.type != TokenType.CONNECTOR]
            name = tokens_no_conn[0].value if tokens_no_conn else "x"
            val = _tokens_to_expr(tokens_no_conn[1:]) if len(tokens_no_conn) > 1 else "1"
            return VarAssignNode(name=name, op="*=", value=val)

        elif intent == "COMPOUND_DIV":
            # divide [name] by [N]
            tokens_no_conn = [t for t in tokens if t.type != TokenType.CONNECTOR]
            name = tokens_no_conn[0].value if tokens_no_conn else "x"
            val = _tokens_to_expr(tokens_no_conn[1:]) if len(tokens_no_conn) > 1 else "1"
            return VarAssignNode(name=name, op="/=", value=val)

        # ── Output ──
        elif intent in ("CONSOLE_LOG", "CONSOLE_WARN", "CONSOLE_ERROR", "BROWSER_ALERT"):
            method_map = {
                "CONSOLE_LOG": "log",
                "CONSOLE_WARN": "warn",
                "CONSOLE_ERROR": "error",
                "BROWSER_ALERT": "alert"
            }
            expr = _tokens_to_expr(tokens)
            return OutputNode(method=method_map[intent], args=[expr] if expr else [])

        # ── DOM Setters & Manipulations ──
        elif intent in ("DOM_SET_TEXT", "DOM_SET_HTML", "DOM_SET_VALUE", "DOM_SET_COLOR", "DOM_SET_BG", "DOM_SET_WIDTH", "DOM_SET_HEIGHT", "DOM_SET_STYLE"):
            prop_map = {
                "DOM_SET_TEXT": "text",
                "DOM_SET_HTML": "html",
                "DOM_SET_VALUE": "value",
                "DOM_SET_COLOR": "color",
                "DOM_SET_BG": "background",
                "DOM_SET_WIDTH": "width",
                "DOM_SET_HEIGHT": "height",
                "DOM_SET_STYLE": "style",
            }
            prop_type = prop_map[intent]
            style_prop = None
            clean_tokens = [t for t in tokens if t.value.lower() not in ("of", "to", "from", "with", "in", "as", "the", "a", "an")]
            
            target = "body"
            val_tokens = []
            if clean_tokens:
                if prop_type == "style" and len(clean_tokens) >= 2:
                    style_prop = clean_tokens[0].value.strip('"\'')
                    target = clean_tokens[1].value.strip('"\'')
                    val_tokens = clean_tokens[2:]
                else:
                    target = clean_tokens[0].value.strip('"\'')
                    val_tokens = clean_tokens[1:]

            val = _tokens_to_expr(val_tokens) if val_tokens else "''"
            return DOMSetNode(target=target, prop_type=prop_type, value=val, style_prop=style_prop)

        elif intent == "DOM_REFRESH":
            # refresh "id" with value
            clean_tokens = [t for t in tokens if t.value.lower() not in ("with", "to", "from", "of", "as", "the", "a", "an")]
            target = clean_tokens[0].value.strip('"\'') if clean_tokens else "app"
            val = _tokens_to_expr(clean_tokens[1:]) if len(clean_tokens) > 1 else "''"
            return DOMRefreshNode(target=target, value=val)

        elif intent in ("CLASS_ADD", "CLASS_REMOVE", "CLASS_TOGGLE"):
            action = intent.split("_")[1].lower() # add, remove, toggle
            # add class "active" to "nav"
            clean_tokens = [t for t in tokens if t.value.lower() not in ("class", "to", "from", "on", "in", "of", "the", "a", "an")]
            cls_name = clean_tokens[0].value.strip('"\'') if clean_tokens else "active"
            target = clean_tokens[1].value.strip('"\'') if len(clean_tokens) > 1 else "body"
            return DOMClassNode(action=action, class_name=cls_name, target=target)

        elif intent in ("DOM_SHOW", "DOM_HIDE"):
            action = "show" if intent == "DOM_SHOW" else "hide"
            clean_tokens = [t for t in tokens if t.value.lower() not in ("element", "the", "a", "an")]
            target = clean_tokens[0].value.strip('"\'') if clean_tokens else "body"
            return DOMVisibilityNode(action=action, target=target)

        # ── Events ──
        elif intent == "EVENT_BIND":
            # when "btn" is clicked: / when "form" submitted: / when window loaded:
            tokens_clean = [t for t in tokens if t.type != TokenType.CONNECTOR or t.value in ("is",)]
            target = "window"
            event_type = "click"
            key_filter = None

            if tokens:
                target = tokens[0].value.strip('"\'')
                # Parse event phrase
                ev_phrase = " ".join([t.value.lower() for t in tokens[1:]])
                if "click" in ev_phrase:
                    event_type = "click"
                elif "submit" in ev_phrase:
                    event_type = "submit"
                elif "change" in ev_phrase:
                    event_type = "change"
                elif "load" in ev_phrase:
                    event_type = "load"
                elif "mouse-enter" in ev_phrase or "mouseenter" in ev_phrase:
                    event_type = "mouseenter"
                elif "mouse-left" in ev_phrase or "mouseleave" in ev_phrase:
                    event_type = "mouseleave"
                elif "press" in ev_phrase or "key" in ev_phrase:
                    event_type = "keydown"
                    if len(tokens) >= 3:
                        key_filter = tokens[-1].value.strip('"\'')

            body = self._parse_block()
            return EventNode(target=target, event_type=event_type, key_filter=key_filter, body=body)

        elif intent == "PREVENT_DEFAULT":
            return PreventDefaultNode()

        # ── Functions ──
        elif intent == "FUNC_DEF":
            # to do greet with name: / function add a b:
            fn_name = tokens[0].value if tokens else "actionFn"
            params = []
            for t in tokens[1:]:
                if t.type == TokenType.IDENTIFIER and t.value.lower() not in CONNECTORS and t.value != ":":
                    params.append(t.value)
            body = self._parse_block()
            return FunctionDefNode(name=fn_name, params=params, body=body)

        elif intent == "FUNC_RETURN":
            val = _tokens_to_expr(tokens) if tokens else None
            return ReturnNode(value=val)

        elif intent == "FUNC_CALL":
            fn_name = tokens[0].value if tokens else "fn"
            args = []
            if len(tokens) > 1:
                args = [_tokens_to_expr(tokens[1:])]
            return FunctionCallNode(name=fn_name, args=args)

        # ── Conditionals ──
        elif intent == "COND_IF":
            cond = _tokens_to_expr(tokens)
            body = self._parse_block()
            elif_branches = []
            else_body = None

            # Check for subsequent else if / else blocks
            while not self._is_at_end():
                self._skip_newlines()
                if self._is_at_end():
                    break
                t = self._peek()
                if t.value.lower() in ("else if", "elif"):
                    line_toks = self._collect_line_tokens()
                    elif_cond = _tokens_to_expr(line_toks[1:])
                    elif_body = self._parse_block()
                    elif_branches.append((elif_cond, elif_body))
                elif t.value.lower() == "else":
                    self._collect_line_tokens() # consume 'else' line
                    else_body = self._parse_block()
                    break
                else:
                    break

            return IfNode(condition=cond, body=body, elif_branches=elif_branches, else_body=else_body)

        # ── Loops ──
        elif intent == "LOOP_REPEAT":
            # repeat 5 times:
            count = tokens[0].value if tokens else "1"
            body = self._parse_block()
            return LoopRepeatNode(count=count, body=body)

        elif intent in ("LOOP_FOR", "LOOP_FOR_EACH"):
            # for each item in items:
            item_name = "item"
            iterable = "[]"
            clean_toks = [t for t in tokens if t.value != "each"]
            if clean_toks:
                item_name = clean_toks[0].value
                clean_after_in = [t for t in clean_toks[1:] if t.value != "in"]
                if clean_after_in:
                    iterable = _tokens_to_expr(clean_after_in)
            body = self._parse_block()
            return LoopForNode(item_name=item_name, iterable=iterable, body=body)

        elif intent == "LOOP_WHILE":
            cond = _tokens_to_expr(tokens)
            body = self._parse_block()
            return LoopWhileNode(condition=cond, body=body)

        # ── Fetch / Network ──
        elif intent == "FETCH_GET":
            # fetch data from "url":
            clean_tokens = [t for t in tokens if t.value.lower() not in ("data", "from", "to", "the", "a", "an")]
            url = clean_tokens[0].value.strip('"\'') if clean_tokens else ""
            body = self._parse_block()
            return FetchNode(method="GET", url=url, response_var="data", body=body)

        elif intent == "FETCH_POST":
            # send data to "/api" with body user:
            clean_tokens = [t for t in tokens if t.value.lower() not in ("data", "to", "from", "with", "the", "a", "an")]
            url = ""
            body_expr = None
            if clean_tokens:
                url = clean_tokens[0].value.strip('"\'')
                clean_body_tokens = [t for t in clean_tokens[1:] if t.value.lower() != "body"]
                if clean_body_tokens:
                    body_expr = _tokens_to_expr(clean_body_tokens)
            body = self._parse_block()
            return FetchNode(method="POST", url=url, body_expr=body_expr, response_var="data", body=body)

        # ── Timers ──
        elif intent == "TIMER_ONCE":
            # after 3 seconds:
            raw_duration = _tokens_to_expr(tokens)
            ms = _parse_time_to_ms(raw_duration)
            body = self._parse_block()
            return TimerNode(timer_type="after", duration_ms=ms, body=body)

        elif intent == "TIMER_REPEAT":
            # every 1 second:
            raw_duration = _tokens_to_expr(tokens)
            ms = _parse_time_to_ms(raw_duration)
            body = self._parse_block()
            return TimerNode(timer_type="every", duration_ms=ms, body=body)

        # ── Browser Actions ──
        elif intent in ("BROWSER_REDIRECT", "BROWSER_RELOAD", "BROWSER_BACK", "BROWSER_FORWARD", "BROWSER_SCROLL", "CLIPBOARD_COPY", "WINDOW_OPEN"):
            action_map = {
                "BROWSER_REDIRECT": "redirect",
                "BROWSER_RELOAD": "reload",
                "BROWSER_BACK": "back",
                "BROWSER_FORWARD": "forward",
                "BROWSER_SCROLL": "scroll",
                "CLIPBOARD_COPY": "copy",
                "WINDOW_OPEN": "open"
            }
            arg = _tokens_to_expr(tokens) if tokens else None
            return BrowserActionNode(action=action_map[intent], arg=arg)

        # ── Storage ──
        elif intent in ("STORAGE_SET", "STORAGE_GET", "STORAGE_REMOVE", "STORAGE_CLEAR"):
            action_map = {
                "STORAGE_SET": "set",
                "STORAGE_GET": "get",
                "STORAGE_REMOVE": "remove",
                "STORAGE_CLEAR": "clear"
            }
            clean_tokens = [t for t in tokens if t.value.lower() not in ("stored", "storage", "as", "the", "a", "an")]
            key = clean_tokens[0].value.strip('"\'') if clean_tokens else "key"
            val = _tokens_to_expr(clean_tokens[1:]) if len(clean_tokens) > 1 else None
            return StorageNode(action=action_map[intent], key=key, value=val)

        # ── Error Handling ──
        elif intent == "TRY_BLOCK":
            try_body = self._parse_block()
            error_var = "error"
            catch_body = []
            self._skip_newlines()
            if not self._is_at_end() and self._peek().value.lower() in ("catch", "rescue"):
                catch_line = self._collect_line_tokens()
                if len(catch_line) > 1:
                    error_var = catch_line[1].value
                catch_body = self._parse_block()
            return TryCatchNode(try_body=try_body, error_var=error_var, catch_body=catch_body)

        # ── Classes ──
        elif intent == "CLASS_DEF":
            cls_name = tokens[0].value.strip('"\'') if tokens else "MyClass"
            body = self._parse_block()
            return ClassDefNode(name=cls_name, body=body)

        # Fallback
        return RawJSNode(code=line_tokens[0].raw_text)

    def _collect_line_tokens(self) -> List[Token]:
        line_tokens: List[Token] = []
        while not self._is_at_end():
            t = self._peek()
            if t.type in (TokenType.NEWLINE, TokenType.EOF, TokenType.DEDENT):
                break
            if t.type == TokenType.SYMBOL and t.value == ":":
                # Check if this colon ends the statement header for a block
                next_idx = self.pos + 1
                if next_idx >= len(self.tokens) or self.tokens[next_idx].type in (TokenType.NEWLINE, TokenType.EOF, TokenType.INDENT):
                    self._advance()
                    break
            line_tokens.append(self._advance())
        return line_tokens

    def _skip_newlines(self):
        while not self._is_at_end() and self._peek().type == TokenType.NEWLINE:
            self._advance()

    def _peek(self) -> Token:
        return self.tokens[self.pos]

    def _advance(self) -> Token:
        tok = self.tokens[self.pos]
        if self.pos < len(self.tokens) - 1:
            self.pos += 1
        return tok

    def _is_at_end(self) -> bool:
        return self.pos >= len(self.tokens) or self.tokens[self.pos].type == TokenType.EOF
