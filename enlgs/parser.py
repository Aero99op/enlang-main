"""enlgs Parser.

Parses tokens into an ENLGS AST (ScriptNode).
Implements Hint-Word Intent Discovery with automatic silent connector skipping,
symbolic operator support, universal JavaScript universe constructs, and automatic raw JS fallback passthrough.
"""

from typing import List, Optional, Tuple, Dict
from .tokens import Token, TokenType, HINT_REGISTRY, CONNECTORS
from .ast_nodes import (
    ASTNode, ScriptNode, RawJSNode, VarDeclNode, VarAssignNode, OutputNode,
    FunctionDefNode, FunctionCallNode, ReturnNode, IfNode,
    LoopRepeatNode, LoopForNode, LoopWhileNode, DOMSetNode, DOMRefreshNode,
    DOMClassNode, DOMVisibilityNode, EventNode, FetchNode, TimerNode,
    BrowserActionNode, StorageNode, TryCatchNode, ClassDefNode, ClassInitNode,
    ClassSuperNode, ShapeDefNode, ComponentDefNode, DOMMakeElementNode,
    DOMAddElementNode, DOMAppendToNode, AnimateTargetNode, FunctionalPipelineNode,
    HttpServerNode, HttpRouteNode, HttpReturnJsonNode, StoreDefNode, StoreStateNode,
    World3DNode, AnimationFrameLoopNode, RotateByNode, TranslateByNode,
    ExtractDestructureNode, WebSocketConnectNode, WebSocketReceiveNode,
    GeneratorDefNode, GeneratorYieldNode, PreventDefaultNode
)

def _tokens_to_expr(tokens: List[Token]) -> str:
    """Converts a token subslice to a clean JavaScript expression string."""
    i = 0
    parts = []
    length = len(tokens)
    
    while i < length:
        t = tokens[i]
        t_val = t.value.lower()
        
        # Check for multi-word hint tokens: 'get value', 'get text', 'get element', or 'get'
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

        # Check for functional array pipelines: filter X where Y
        if t_val == "filter" and i + 3 < length:
            where_idx = -1
            for w in range(i + 1, length):
                if tokens[w].value.lower() == "where":
                    where_idx = w
                    break
            if where_idx != -1:
                source_tokens = tokens[i+1:where_idx]
                predicate_tokens = tokens[where_idx+1:]
                source_expr = _tokens_to_expr(source_tokens)
                predicate_expr = _tokens_to_expr(predicate_tokens)
                parts.append(f"{source_expr}.filter(item => {predicate_expr})")
                break

        # Check for map X using item: Y
        if t_val == "map" and i + 2 < length:
            using_idx = -1
            for u in range(i + 1, length):
                if tokens[u].value.lower() == "using":
                    using_idx = u
                    break
            if using_idx != -1:
                source_tokens = tokens[i+1:using_idx]
                transform_tokens = tokens[using_idx+1:]
            else:
                source_tokens = [tokens[i+1]]
                transform_tokens = tokens[i+2:]

            source_expr = _tokens_to_expr(source_tokens)
            param = "item"
            expr_toks = transform_tokens
            if len(transform_tokens) >= 2 and transform_tokens[0].type == TokenType.IDENTIFIER and transform_tokens[1].value == ":":
                param = transform_tokens[0].value
                expr_toks = transform_tokens[2:]
            transform_expr = _tokens_to_expr(expr_toks)
            parts.append(f"{source_expr}.map({param} => {transform_expr})")
            break

        # Check for find in X where Y
        if (t_val == "find in" or t_val == "find") and i + 2 < length:
            where_idx = -1
            for w in range(i + 1, length):
                if tokens[w].value.lower() == "where":
                    where_idx = w
                    break
            if where_idx != -1:
                source_tokens = tokens[i+1:where_idx]
                predicate_tokens = tokens[where_idx+1:]
                source_expr = _tokens_to_expr(source_tokens)
                predicate_expr = _tokens_to_expr(predicate_tokens)
                parts.append(f"{source_expr}.find(item => {predicate_expr})")
                break
        
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
            pass
        else:
            parts.append(t.value)
        i += 1
        
    return " ".join(parts).strip()

def _parse_time_to_ms(val_str: str) -> str:
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

        self._skip_newlines()
        if not self._is_at_end() and self._peek().type == TokenType.HINT:
            intent = HINT_REGISTRY.get(self._peek().value.lower())
            if intent == "DOMAIN_DECL":
                self._advance()
                if not self._is_at_end() and self._peek().type == TokenType.SYMBOL and self._peek().value == ":":
                    self._advance()
                root.body = self._parse_block()
                return root

        root.body = self._parse_statements_until_end()
        return root

    def _parse_block(self) -> List[ASTNode]:
        self._skip_newlines()
        if self._is_at_end():
            return []

        if self._peek().type == TokenType.INDENT:
            self._advance()
            statements = []
            while not self._is_at_end():
                self._skip_newlines()
                if self._is_at_end():
                    break
                if self._peek().type == TokenType.DEDENT:
                    self._advance()
                    break
                stmt = self._parse_statement()
                if stmt:
                    statements.append(stmt)
            return statements
        else:
            stmt = self._parse_statement()
            return [stmt] if stmt else []

    def _parse_statements_until_end(self) -> List[ASTNode]:
        statements = []
        while not self._is_at_end():
            self._skip_newlines()
            if self._is_at_end():
                break
            if self._peek().type == TokenType.DEDENT:
                self._advance()
                continue
            stmt = self._parse_statement()
            if stmt:
                statements.append(stmt)
        return statements

    def _parse_statement(self) -> Optional[ASTNode]:
        self._skip_newlines()
        if self._is_at_end():
            return None

        line_tokens = self._collect_line_tokens()
        if not line_tokens:
            return None

        first_tok = line_tokens[0]

        # 1. Check for recognized Hint Word intent
        if first_tok.type == TokenType.HINT:
            intent = HINT_REGISTRY.get(first_tok.value.lower())
            if intent:
                return self._build_node_from_intent(intent, line_tokens[1:], line_tokens)

        # 2. Check for Destructuring assignment
        if first_tok.value.lower() in ("extract", "unpack"):
            return self._build_node_from_intent("EXTRACT_FROM", line_tokens[1:], line_tokens)

        # 3. Check for Assignment
        if len(line_tokens) >= 3 and line_tokens[1].type == TokenType.OPERATOR and line_tokens[1].value in ("=", "+=", "-=", "*=", "/="):
            name = line_tokens[0].value
            op = line_tokens[1].value
            val = _tokens_to_expr(line_tokens[2:])
            return VarAssignNode(name=name, op=op, value=val)

        # 4. Check for Function Call: fn(arg1, arg2)
        if len(line_tokens) >= 2 and line_tokens[0].type == TokenType.IDENTIFIER and line_tokens[1].type == TokenType.SYMBOL and line_tokens[1].value == "(":
            return RawJSNode(code=_tokens_to_expr(line_tokens) + ";")

        # 5. Raw JavaScript Passthrough fallback
        return RawJSNode(code=first_tok.raw_text)

    def _build_node_from_intent(self, intent: str, tokens: List[Token], line_tokens: List[Token]) -> ASTNode:
        # ── Variables & Constants ──
        if intent == "DECLARE_VAR":
            if not tokens:
                return RawJSNode(code=line_tokens[0].raw_text)
            name = tokens[0].value
            val_tokens = tokens[1:]
            if val_tokens and val_tokens[0].value.lower() in ("as", "="):
                val_tokens = val_tokens[1:]
            val = _tokens_to_expr(val_tokens) if val_tokens else None
            return VarDeclNode(kind="let", name=name, value=val)

        elif intent == "DECLARE_CONST":
            if not tokens:
                return RawJSNode(code=line_tokens[0].raw_text)
            name = tokens[0].value
            val_tokens = tokens[1:]
            if val_tokens and val_tokens[0].value.lower() in ("as", "="):
                val_tokens = val_tokens[1:]
            val = _tokens_to_expr(val_tokens) if val_tokens else None
            return VarDeclNode(kind="const", name=name, value=val)

        elif intent == "ASSIGN_VAR":
            if not tokens:
                return RawJSNode(code=line_tokens[0].raw_text)
            name = tokens[0].value
            val_tokens = tokens[1:]
            if val_tokens and val_tokens[0].value.lower() in ("as", "="):
                val_tokens = val_tokens[1:]
            val = _tokens_to_expr(val_tokens) if val_tokens else "null"
            return VarAssignNode(name=name, op="=", value=val)

        elif intent == "COMPOUND_ADD":
            tokens_no_conn = [t for t in tokens if t.type != TokenType.CONNECTOR]
            name = tokens_no_conn[0].value if tokens_no_conn else "x"
            val = _tokens_to_expr(tokens_no_conn[1:]) if len(tokens_no_conn) > 1 else "1"
            return VarAssignNode(name=name, op="+=", value=val)

        elif intent == "COMPOUND_SUB":
            tokens_no_conn = [t for t in tokens if t.type != TokenType.CONNECTOR]
            name = tokens_no_conn[0].value if tokens_no_conn else "x"
            val = _tokens_to_expr(tokens_no_conn[1:]) if len(tokens_no_conn) > 1 else "1"
            return VarAssignNode(name=name, op="-=", value=val)

        elif intent == "COMPOUND_MUL":
            tokens_no_conn = [t for t in tokens if t.type != TokenType.CONNECTOR]
            name = tokens_no_conn[0].value if tokens_no_conn else "x"
            val = _tokens_to_expr(tokens_no_conn[1:]) if len(tokens_no_conn) > 1 else "1"
            return VarAssignNode(name=name, op="*=", value=val)

        elif intent == "COMPOUND_DIV":
            tokens_no_conn = [t for t in tokens if t.type != TokenType.CONNECTOR]
            name = tokens_no_conn[0].value if tokens_no_conn else "x"
            val = _tokens_to_expr(tokens_no_conn[1:]) if len(tokens_no_conn) > 1 else "1"
            return VarAssignNode(name=name, op="/=", value=val)

        # ── Destructuring Assignment ──
        elif intent == "EXTRACT_FROM":
            clean_toks = [t for t in tokens if t.value != ","]
            from_idx = -1
            for idx, tok in enumerate(clean_toks):
                if tok.value.lower() in ("from", "in", "of"):
                    from_idx = idx
                    break
            if from_idx != -1:
                var_names = [t.value for t in clean_toks[:from_idx] if t.type == TokenType.IDENTIFIER]
                source_expr = _tokens_to_expr(clean_toks[from_idx+1:])
                return ExtractDestructureNode(variables=var_names, source_expr=source_expr)
            return RawJSNode(code=line_tokens[0].raw_text)

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

        # ── DOM Setters ──
        elif intent in ("DOM_SET_TEXT", "DOM_SET_HTML", "DOM_SET_VALUE", "DOM_SET_COLOR",
                        "DOM_SET_BG", "DOM_SET_WIDTH", "DOM_SET_HEIGHT", "DOM_SET_STYLE"):
            prop_type = intent.replace("DOM_SET_", "").lower()
            if prop_type == "bg":
                prop_type = "background"

            clean_tokens = [t for t in tokens if t.value.lower() not in ("of", "to", "as", "the", "a", "an")]
            target = "app"
            style_prop = None
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
            clean_tokens = [t for t in tokens if t.value.lower() not in ("with", "to", "from", "of", "as", "the", "a", "an")]
            target = clean_tokens[0].value.strip('"\'') if clean_tokens else "app"
            val = _tokens_to_expr(clean_tokens[1:]) if len(clean_tokens) > 1 else "''"
            return DOMRefreshNode(target=target, value=val)

        elif intent in ("CLASS_ADD", "CLASS_REMOVE", "CLASS_TOGGLE"):
            action = intent.split("_")[1].lower()
            clean_tokens = [t for t in tokens if t.value.lower() not in ("class", "to", "from", "on", "in", "of", "the", "a", "an")]
            cls_name = clean_tokens[0].value.strip('"\'') if clean_tokens else "active"
            target = clean_tokens[1].value.strip('"\'') if len(clean_tokens) > 1 else "body"
            return DOMClassNode(action=action, class_name=cls_name, target=target)

        elif intent in ("DOM_SHOW", "DOM_HIDE"):
            action = "show" if intent == "DOM_SHOW" else "hide"
            clean_tokens = [t for t in tokens if t.value.lower() not in ("element", "the", "a", "an")]
            target = clean_tokens[0].value.strip('"\'') if clean_tokens else "body"
            return DOMVisibilityNode(action=action, target=target)

        # ── Declarative UI Elements ──
        elif intent == "DOM_MAKE_ELEMENT":
            clean_tokens = [t for t in tokens if t.value.lower() not in ("element", "with", "the", "a", "an")]
            tag = clean_tokens[0].value.strip('"\'') if clean_tokens else "div"
            attrs = {}
            for idx, tok in enumerate(clean_tokens):
                t_low = tok.value.lower()
                if t_low in ("class", "id", "style", "type", "href", "src") and idx + 1 < len(clean_tokens):
                    attrs[t_low] = clean_tokens[idx + 1].value.strip('"\'')
            body = self._parse_block()
            return DOMMakeElementNode(tag=tag, attrs=attrs, body=body)

        elif intent == "DOM_ADD_ELEMENT":
            clean_tokens = [t for t in tokens if t.value.lower() not in ("element", "with", "the", "a", "an")]
            tag = clean_tokens[0].value.strip('"\'') if clean_tokens else "div"
            attrs = {}
            text_val = None
            for idx, tok in enumerate(clean_tokens):
                t_low = tok.value.lower()
                if t_low == "text" and idx + 1 < len(clean_tokens):
                    text_val = clean_tokens[idx + 1].value.strip('"\'')
                elif t_low in ("class", "id", "style", "type", "href", "src") and idx + 1 < len(clean_tokens):
                    attrs[t_low] = clean_tokens[idx + 1].value.strip('"\'')
            return DOMAddElementNode(tag=tag, attrs=attrs, text=text_val)

        elif intent in ("DOM_APPEND_TO", "DOM_PREPEND_TO"):
            is_prepend = (intent == "DOM_PREPEND_TO")
            clean_tokens = [t for t in tokens if t.value.lower() not in ("to", "the", "a", "an")]
            child = clean_tokens[0].value.strip('"\'') if clean_tokens else "el"
            parent = clean_tokens[1].value.strip('"\'') if len(clean_tokens) > 1 else "document.body"
            return DOMAppendToNode(child=child, parent=parent, is_prepend=is_prepend)

        elif intent == "ANIMATE_TARGET":
            target = tokens[0].value.strip('"\'') if tokens else "window"
            duration = "500"
            raw_str = _tokens_to_expr(tokens[1:])
            return AnimateTargetNode(target=target, duration_ms=duration, properties=raw_str)

        # ── TypeScript Shapes ──
        elif intent == "SHAPE_DEF":
            shape_name = tokens[0].value if tokens else "EntityShape"
            fields = []
            block_stmts = self._parse_block()
            for stmt in block_stmts:
                if isinstance(stmt, RawJSNode):
                    parts = stmt.code.split(" is ")
                    if len(parts) == 2:
                        fields.append((parts[0].strip(), parts[1].strip()))
            return ShapeDefNode(name=shape_name, fields=fields)

        # ── Declarative UI Components ──
        elif intent == "COMPONENT_DEF":
            comp_name = tokens[0].value if tokens else "MyComponent"
            props = []
            PARAM_CONNECTORS = {"with", "and", "to", "from", "as", "of", "in", "the", "a", "an", "default"}
            for t in tokens[1:]:
                if t.type in (TokenType.IDENTIFIER, TokenType.HINT, TokenType.CONNECTOR) and t.value.lower() not in PARAM_CONNECTORS and t.value not in (":", ","):
                    props.append(t.value)
            body = self._parse_block()
            return ComponentDefNode(name=comp_name, props=props, body=body)

        # ── Events ──
        elif intent == "EVENT_BIND":
            target = "window"
            event_type = "click"
            key_filter = None
            is_variable = False

            if tokens:
                target = tokens[0].value.strip('"\'')
                is_variable = (tokens[0].type == TokenType.IDENTIFIER and tokens[0].value.lower() not in ("window", "document", "body"))
                ev_phrase = " ".join([t.value.lower() for t in tokens[1:]])
                if "click" in ev_phrase:
                    event_type = "click"
                elif "submit" in ev_phrase:
                    event_type = "submit"
                elif "change" in ev_phrase:
                    event_type = "change"
                elif "load" in ev_phrase:
                    event_type = "load"
                elif "receives" in ev_phrase or "message" in ev_phrase:
                    event_type = "message"
                elif "mousemove" in ev_phrase or "mouse moved" in ev_phrase or "moved" in ev_phrase:
                    event_type = "mousemove"
                elif "resize" in ev_phrase or "resized" in ev_phrase:
                    event_type = "resize"
                elif "scroll" in ev_phrase:
                    event_type = "scroll"
                elif "mouse-enter" in ev_phrase or "mouseenter" in ev_phrase:
                    event_type = "mouseenter"
                elif "mouse-left" in ev_phrase or "mouseleave" in ev_phrase:
                    event_type = "mouseleave"
                elif "press" in ev_phrase or "key" in ev_phrase:
                    event_type = "keydown"
                    if len(tokens) >= 3:
                        key_filter = tokens[-1].value.strip('"\'')

            body = self._parse_block()
            return EventNode(target=target, event_type=event_type, key_filter=key_filter, is_variable=is_variable, body=body)

        elif intent == "PREVENT_DEFAULT":
            return PreventDefaultNode()

        # ── Functions & Async ──
        elif intent == "FUNC_DEF":
            fn_name = tokens[0].value if tokens else "actionFn"
            params = []
            PARAM_CONNECTORS = {"with", "and", "to", "from", "as", "of", "in", "the", "a", "an", "default"}
            for t in tokens[1:]:
                if t.type in (TokenType.IDENTIFIER, TokenType.HINT, TokenType.CONNECTOR) and t.value.lower() not in PARAM_CONNECTORS and t.value not in (":", ","):
                    params.append(t.value)
            body = self._parse_block()
            return FunctionDefNode(name=fn_name, params=params, body=body, is_async=False)

        elif intent == "ASYNC_FUNC_DEF":
            fn_name = tokens[0].value if tokens else "asyncActionFn"
            params = []
            PARAM_CONNECTORS = {"with", "and", "to", "from", "as", "of", "in", "the", "a", "an", "default"}
            for t in tokens[1:]:
                if t.type in (TokenType.IDENTIFIER, TokenType.HINT, TokenType.CONNECTOR) and t.value.lower() not in PARAM_CONNECTORS and t.value not in (":", ","):
                    params.append(t.value)
            body = self._parse_block()
            return FunctionDefNode(name=fn_name, params=params, body=body, is_async=True)

        elif intent == "GENERATOR_DEF":
            fn_name = tokens[0].value if tokens else "generatorFn"
            params = []
            for t in tokens[1:]:
                if t.type == TokenType.IDENTIFIER and t.value.lower() not in CONNECTORS and t.value not in (":", ","):
                    params.append(t.value)
            body = self._parse_block()
            return GeneratorDefNode(name=fn_name, params=params, body=body)

        elif intent == "GENERATOR_YIELD":
            val = _tokens_to_expr(tokens) if tokens else None
            return GeneratorYieldNode(value=val)

        elif intent == "FUNC_RETURN":
            val = _tokens_to_expr(tokens) if tokens else None
            return ReturnNode(value=val)

        elif intent == "HTTP_RETURN_JSON":
            data_expr = _tokens_to_expr(tokens) if tokens else "{}"
            return HttpReturnJsonNode(data_expr=data_expr)

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
                    self._collect_line_tokens()
                    else_body = self._parse_block()
                    break
                else:
                    break

            return IfNode(condition=cond, body=body, elif_branches=elif_branches, else_body=else_body)

        # ── Loops ──
        elif intent == "LOOP_REPEAT":
            count = tokens[0].value if tokens else "1"
            body = self._parse_block()
            return LoopRepeatNode(count=count, body=body)

        elif intent in ("LOOP_FOR", "LOOP_FOR_EACH"):
            item_name = "item"
            iterable = "[]"
            clean_toks = [t for t in tokens if t.value not in ("each", "every")]
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

        # ── Full-Stack Servers ──
        elif intent == "SERVER_HTTP":
            port = tokens[0].value if tokens else "8080"
            routes = self._parse_block()
            return HttpServerNode(port=port, routes=routes)

        elif intent in ("ROUTE_GET", "ROUTE_POST", "ROUTE_PUT", "ROUTE_DELETE", "ROUTE_ANY"):
            method = intent.replace("ROUTE_", "")
            path = tokens[0].value.strip('"\'') if tokens else "/"
            body = self._parse_block()
            return HttpRouteNode(method=method, path=path, body=body)

        # ── Centralized State Stores ──
        elif intent in ("STORE_DEF", "STORAGE_SET"):
            # Disambiguate LocalStorage vs Reactive State Store
            if any(t.value.lower() in ("in", "as") or t.type == TokenType.STRING for t in tokens):
                clean_tokens = [t for t in tokens if t.value.lower() not in ("stored", "storage", "as", "the", "a", "an", "in", "local")]
                key = clean_tokens[0].value.strip('"\'') if clean_tokens else "key"
                val = _tokens_to_expr(clean_tokens[1:]) if len(clean_tokens) > 1 else None
                return StorageNode(action="set", key=key, value=val)
            else:
                store_name = tokens[0].value if tokens else "AppStore"
                states = []
                actions = []
                body = self._parse_block()
                for stmt in body:
                    if isinstance(stmt, FunctionDefNode):
                        actions.append(stmt)
                    elif isinstance(stmt, RawJSNode):
                        parts = stmt.code.split(" as ")
                        if len(parts) == 2:
                            states.append((parts[0].replace("state", "").strip(), parts[1].strip()))
                return StoreDefNode(name=store_name, states=states, actions=actions)

        # ── 3D World DSL ──
        elif intent == "WORLD_3D":
            canvas_target = tokens[0].value.strip('"\'') if tokens else "webgl-canvas"
            body = self._parse_block()
            return World3DNode(canvas_target=canvas_target, body=body)

        elif intent == "ANIM_FRAME_LOOP":
            body = self._parse_block()
            return AnimationFrameLoopNode(body=body)

        elif intent == "ROTATE_BY":
            target = tokens[0].value if tokens else "obj"
            rot_map = {}
            for idx, t in enumerate(tokens):
                if t.value.lower() in ("x", "y", "z") and idx + 1 < len(tokens):
                    rot_map[t.value.lower()] = tokens[idx + 1].value
            return RotateByNode(target=target, rotations=rot_map)

        elif intent == "TRANSLATE_BY":
            target = tokens[0].value if tokens else "obj"
            trans_map = {}
            for idx, t in enumerate(tokens):
                if t.value.lower() in ("x", "y", "z") and idx + 1 < len(tokens):
                    trans_map[t.value.lower()] = tokens[idx + 1].value
            return TranslateByNode(target=target, translations=trans_map)

        # ── WebSockets ──
        elif intent == "WEBSOCKET_CONNECT":
            clean_tokens = [t for t in tokens if t.value.lower() not in ("to", "the", "a", "an")]
            url = clean_tokens[0].value.strip('"\'') if clean_tokens else ""
            socket_var = clean_tokens[1].value if len(clean_tokens) > 1 else "socket"
            return WebSocketConnectNode(url=url, socket_var=socket_var)

        elif intent == "WEBSOCKET_RECEIVE":
            socket_var = tokens[0].value if tokens else "socket"
            body = self._parse_block()
            return WebSocketReceiveNode(socket_var=socket_var, data_var="data", body=body)

        # ── Timers ──
        elif intent == "TIMER_ONCE":
            raw_duration = _tokens_to_expr(tokens)
            ms = _parse_time_to_ms(raw_duration)
            body = self._parse_block()
            return TimerNode(timer_type="after", duration_ms=ms, body=body)

        elif intent == "TIMER_REPEAT":
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
        elif intent in ("STORAGE_GET", "STORAGE_REMOVE", "STORAGE_CLEAR"):
            action_map = {
                "STORAGE_GET": "get",
                "STORAGE_REMOVE": "remove",
                "STORAGE_CLEAR": "clear"
            }
            clean_tokens = [t for t in tokens if t.value.lower() not in ("stored", "storage", "as", "the", "a", "an", "in", "local")]
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

        # ── OOP / Classes / Blueprints ──
        elif intent in ("BLUEPRINT_DEF", "CLASS_DEF"):
            cls_name = tokens[0].value.strip('"\'') if tokens else "MyClass"
            parent = None
            if len(tokens) >= 3 and tokens[1].value.lower() in ("extends", "inherits"):
                parent = tokens[2].value.strip('"\'')
            body = self._parse_block()
            return ClassDefNode(name=cls_name, parent=parent, body=body)

        elif intent == "CLASS_INIT":
            params = []
            for t in tokens:
                if t.type == TokenType.IDENTIFIER and t.value.lower() not in CONNECTORS and t.value not in (":", ","):
                    params.append(t.value)
            body = self._parse_block()
            return ClassInitNode(params=params, body=body)

        elif intent == "CLASS_SUPER":
            args = []
            for t in tokens:
                if t.type == TokenType.IDENTIFIER and t.value.lower() not in CONNECTORS and t.value != ",":
                    args.append(t.value)
            return ClassSuperNode(args=args)

        # Fallback
        return RawJSNode(code=line_tokens[0].raw_text)

    def _collect_line_tokens(self) -> List[Token]:
        line_tokens: List[Token] = []
        while not self._is_at_end():
            t = self._peek()
            if t.type in (TokenType.NEWLINE, TokenType.EOF, TokenType.DEDENT):
                break
            if t.type == TokenType.SYMBOL and t.value == ":":
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
