"""enlgm Parser.

Parses token streams from ENLGMLexer into a Flutter / Dart AST.
Enforces the mandatory 'in mobile:' architecture root, processes English hint phrases,
and constructs strongly-typed Widget Trees, Navigation graphs, State, and Network nodes.
"""

from typing import List, Optional, Dict, Any, Tuple
from .tokens import Token, ENLGMTokenType
from .ast_nodes import (
    ASTNode, MobileRootNode, FlutterImportNode, AppDefNode, ScreenDefNode,
    WidgetNode, TextWidgetNode, IconWidgetNode, ImageWidgetNode, AvatarWidgetNode,
    ButtonWidgetNode, InputWidgetNode, SpacerWidgetNode, DividerWidgetNode,
    ChipWidgetNode, ProgressWidgetNode, ControlWidgetNode, ListViewWidgetNode,
    GridViewWidgetNode, AppBarNode, BottomNavNode, BottomNavItem, DrawerNode,
    DrawerItem, NavigationNode, StateDeclNode, StateAssignNode, ToastFeedbackNode,
    NetworkCallNode, RawDartNode, WidgetBlueprintNode, BlueprintInstanceNode,
    CallFunctionNode, MethodDefNode
)

def _tokens_to_str(tokens: List[Token]) -> str:
    """Converts a token subslice into a clean expression or string representation."""
    parts = []
    for t in tokens:
        if t.type == ENLGMTokenType.STRING:
            parts.append(f'"{t.value}"')
        else:
            parts.append(t.value)
    return " ".join(parts).strip()

def _parse_props_from_tokens(tokens: List[Token]) -> Dict[str, Any]:
    """Extracts property key-values (e.g. size 28, bold, color "#fff", align center)."""
    props = {}
    i = 0
    length = len(tokens)

    while i < length:
        t = tokens[i]
        val_lower = t.value.lower()

        # Connectors can be skipped
        if t.type == ENLGMTokenType.CONNECTOR and val_lower in ("with", "and", "having", "is", "the", "a", "an"):
            i += 1
            continue

        if t.type == ENLGMTokenType.SYMBOL and t.value in (",", ":", ";"):
            i += 1
            continue

        # Standalone flags
        if val_lower in ("bold", "italic", "underline", "hidden", "centered", "spaced", "back_button"):
            props[val_lower] = True
            i += 1
            continue

        # Key-Value pairs
        if val_lower in (
            "size", "color", "font", "align", "radius", "corner-radius", "corner_radius",
            "elevation", "padding", "width", "height", "lines", "placeholder", "type",
            "max_lines", "max-lines", "overflow", "gap", "columns", "duration", "background",
            "border", "border-color", "border_color", "shadow", "flex"
        ):
            prop_key = val_lower.replace("-", "_")
            if i + 1 < length:
                next_t = tokens[i + 1]
                # Consume connector if any (e.g. 'color is "#fff"')
                if next_t.type == ENLGMTokenType.CONNECTOR and next_t.value.lower() in ("is", "as", "to", "of"):
                    if i + 2 < length:
                        props[prop_key] = tokens[i + 2].value
                        i += 3
                        continue
                props[prop_key] = next_t.value
                i += 2
                continue

        # Generic key-value fallback
        if t.type == ENLGMTokenType.IDENTIFIER and i + 1 < length and tokens[i+1].type in (ENLGMTokenType.STRING, ENLGMTokenType.NUMBER, ENLGMTokenType.IDENTIFIER):
            props[t.value.lower()] = tokens[i+1].value
            i += 2
            continue

        i += 1

    return props

class ENLGMParser:
    """Parses .enlgm token stream into a MobileRootNode AST."""

    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.pos = 0
        self.length = len(tokens)
        self.root = MobileRootNode()

    def _peek(self, offset: int = 0) -> Optional[Token]:
        idx = self.pos + offset
        return self.tokens[idx] if idx < self.length else None

    def _match(self, token_type: ENLGMTokenType, value: Optional[str] = None) -> bool:
        tok = self._peek()
        if not tok or tok.type != token_type:
            return False
        if value is not None and tok.value.lower() != value.lower():
            return False
        return True

    def _consume(self) -> Optional[Token]:
        if self.pos < self.length:
            tok = self.tokens[self.pos]
            self.pos += 1
            return tok
        return None

    def _consume_newlines(self):
        while self._match(ENLGMTokenType.NEWLINE):
            self._consume()

    def parse(self) -> MobileRootNode:
        self._consume_newlines()

        has_type_header = False
        first_tok = self._peek()
        if first_tok and first_tok.type == ENLGMTokenType.HINT and first_tok.value == "TYPE_HEADER":
            has_type_header = True
            self._consume()
            self._consume_newlines()

        first_tok = self._peek()
        if first_tok and first_tok.type == ENLGMTokenType.HINT and first_tok.value == "MOBILE_DOMAIN":
            self._consume() # Consume 'in mobile'
            if self._match(ENLGMTokenType.SYMBOL, ":"):
                self._consume()
            self._consume_newlines()

            if self._match(ENLGMTokenType.INDENT):
                self._consume()
                while not (self._match(ENLGMTokenType.DEDENT) or self._match(ENLGMTokenType.EOF)):
                    self._consume_newlines()
                    if self._match(ENLGMTokenType.DEDENT) or self._match(ENLGMTokenType.EOF):
                        break
                    stmt = self._parse_top_level_statement()
                    if stmt:
                        self._add_top_level_node(stmt)
                if self._match(ENLGMTokenType.DEDENT):
                    self._consume()
            else:
                while not self._match(ENLGMTokenType.EOF):
                    self._consume_newlines()
                    if self._match(ENLGMTokenType.EOF):
                        break
                    stmt = self._parse_top_level_statement()
                    if stmt:
                        self._add_top_level_node(stmt)
        elif has_type_header:
            while not self._match(ENLGMTokenType.EOF):
                self._consume_newlines()
                if self._match(ENLGMTokenType.EOF):
                    break
                stmt = self._parse_top_level_statement()
                if stmt:
                    self._add_top_level_node(stmt)
        else:
            raise SyntaxError("Enlang Mobile files must begin with 'type enlgm' or 'in mobile:' as the first statement.")

        return self.root

    def _add_top_level_node(self, node: ASTNode):
        if isinstance(node, FlutterImportNode):
            self.root.imports.append(node)
        elif isinstance(node, AppDefNode):
            self.root.app = node
        elif isinstance(node, ScreenDefNode):
            self.root.screens.append(node)
        elif isinstance(node, WidgetBlueprintNode):
            self.root.blueprints.append(node)
        elif isinstance(node, RawDartNode):
            self.root.raw_dart_blocks.append(node)

    def _parse_top_level_statement(self) -> Optional[ASTNode]:
        line_tokens = self._gather_line_tokens()
        if not line_tokens:
            return None

        first = line_tokens[0]

        if first.type == ENLGMTokenType.HINT:
            intent = first.value

            if intent == "FLUTTER_IMPORT":
                pkg_name = line_tokens[1].value if len(line_tokens) > 1 else "material"
                return FlutterImportNode(package_name=pkg_name, is_flutter=True)

            elif intent == "PACKAGE_IMPORT":
                pkg_name = line_tokens[1].value if len(line_tokens) > 1 else "http"
                alias = line_tokens[3].value if len(line_tokens) > 3 and line_tokens[2].value.lower() == "as" else None
                return FlutterImportNode(package_name=pkg_name, is_flutter=False, as_alias=alias)

            elif intent == "APP_DEF":
                app_name = line_tokens[1].value if len(line_tokens) > 1 else "App"
                app_node = AppDefNode(name=app_name)
                # Parse app block
                if self._has_child_block():
                    body_stmts = self._parse_block()
                    for s in body_stmts:
                        if isinstance(s, dict):
                            if "theme" in s:
                                app_node.theme = s["theme"]
                            if "color" in s or "accent_color" in s:
                                app_node.primary_color = s.get("color") or s.get("accent_color")
                            if "home" in s or "home_screen" in s:
                                app_node.home_screen = s.get("home") or s.get("home_screen")
                return app_node

            elif intent in ("SCREEN_DEF", "STATEFUL_SCREEN_DEF"):
                is_stateful = (intent == "STATEFUL_SCREEN_DEF")
                screen_name = line_tokens[1].value if len(line_tokens) > 1 else "Screen"
                screen = ScreenDefNode(name=screen_name, is_stateful=is_stateful)

                if self._has_child_block():
                    body_nodes = self._parse_block()
                    for node in body_nodes:
                        if isinstance(node, StateDeclNode):
                            screen.state_vars.append(node)
                            screen.is_stateful = True
                        elif isinstance(node, AppBarNode):
                            screen.app_bar = node
                        elif isinstance(node, BottomNavNode):
                            screen.bottom_bar = node
                        elif isinstance(node, DrawerNode):
                            screen.drawer = node
                        elif isinstance(node, ButtonWidgetNode) and node.btn_type == "fab":
                            screen.fab = node
                        elif isinstance(node, MethodDefNode):
                            screen.methods.append(node)
                        elif isinstance(node, dict) and "title" in node:
                            screen.title = node["title"]
                        elif isinstance(node, dict) and "body" in node:
                            screen.body.extend(node["body"])
                        else:
                            screen.body.append(node)

                return screen

            elif intent == "WIDGET_BLUEPRINT":
                bp_name = line_tokens[1].value if len(line_tokens) > 1 else "CustomWidget"
                params = []
                # Extract 'needs p1, p2, p3' if present on the line
                for t in line_tokens[2:]:
                    if t.type == ENLGMTokenType.IDENTIFIER and t.value.lower() not in ("needs", "with", "and", "params", ","):
                        params.append(t.value)
                bp_body = []
                if self._has_child_block():
                    raw_body = self._parse_block()
                    for item in raw_body:
                        if isinstance(item, dict) and "needs" in item:
                            params.extend(item["needs"])
                        else:
                            bp_body.append(item)
                return WidgetBlueprintNode(name=bp_name, params=params, body=bp_body)

            elif intent == "RAW_DART":
                code_lines = self._gather_raw_block_text()
                return RawDartNode(code="\n".join(code_lines))

        return None

    def _has_child_block(self) -> bool:
        self._consume_newlines()
        return self._match(ENLGMTokenType.INDENT)

    def _gather_raw_block_text(self) -> List[str]:
        lines = []
        if self._match(ENLGMTokenType.INDENT):
            self._consume()
            while not (self._match(ENLGMTokenType.DEDENT) or self._match(ENLGMTokenType.EOF)):
                tok = self._consume()
                if tok:
                    if tok.type == ENLGMTokenType.NEWLINE:
                        lines.append(tok.raw_text)
                    elif not lines:
                        lines.append(tok.raw_text)
            if self._match(ENLGMTokenType.DEDENT):
                self._consume()
        return lines

    def _parse_block(self) -> List[Any]:
        nodes = []
        if not self._match(ENLGMTokenType.INDENT):
            return nodes
        self._consume() # Consume INDENT

        while not (self._match(ENLGMTokenType.DEDENT) or self._match(ENLGMTokenType.EOF)):
            self._consume_newlines()
            if self._match(ENLGMTokenType.DEDENT) or self._match(ENLGMTokenType.EOF):
                break
            stmt = self._parse_statement()
            if stmt:
                nodes.append(stmt)
            self._consume_newlines()

        if self._match(ENLGMTokenType.DEDENT):
            self._consume()

        return nodes

    def _gather_line_tokens(self) -> List[Token]:
        tokens = []
        while not (self._match(ENLGMTokenType.NEWLINE) or self._match(ENLGMTokenType.EOF) or self._match(ENLGMTokenType.INDENT) or self._match(ENLGMTokenType.DEDENT)):
            tokens.append(self._consume())
        if self._match(ENLGMTokenType.NEWLINE):
            self._consume()
        return tokens

    def _parse_statement(self) -> Optional[Any]:
        line_tokens = self._gather_line_tokens()
        if not line_tokens:
            return None

        first = line_tokens[0]

        # Check for simple screen property blocks (e.g. 'title "Home"', 'body:', 'state:')
        if first.type == ENLGMTokenType.IDENTIFIER:
            val_lower = first.value.lower()
            if val_lower == "title" and len(line_tokens) > 1:
                return {"title": line_tokens[1].value}
            elif val_lower == "theme" and len(line_tokens) > 1:
                return {"theme": line_tokens[1].value}
            elif val_lower in ("color", "accent_color", "accent") and len(line_tokens) > 1:
                color_val = line_tokens[1].value if line_tokens[1].value != "color" else (line_tokens[2].value if len(line_tokens) > 2 else "#00f2fe")
                return {"color": color_val}
            elif val_lower in ("home", "home_screen") and len(line_tokens) > 1:
                target = [t.value for t in line_tokens[1:] if t.value.lower() not in ("screen", "to", "at", "is", "as") and t.type in (ENLGMTokenType.IDENTIFIER, ENLGMTokenType.STRING, ENLGMTokenType.HINT)]
                # If a hint was matched like 'HomeScreen' where 'Home' wasn't recognized as hint but followed by screen
                candidate = target[-1] if target else "HomeScreen"
                return {"home": candidate}
            elif val_lower == "needs":
                param_names = [t.value for t in line_tokens[1:] if t.type == ENLGMTokenType.IDENTIFIER and t.value.lower() not in ("with", "and", "params", ",")]
                return {"needs": param_names}
            elif val_lower == "body":
                body_children = self._parse_block() if self._has_child_block() else []
                return {"body": body_children}
            elif val_lower == "actions":
                actions_list = self._parse_block() if self._has_child_block() else []
                return {"actions": actions_list}
            elif val_lower == "header":
                header_list = self._parse_block() if self._has_child_block() else []
                return {"header": header_list}

        if first.type == ENLGMTokenType.HINT:
            intent = first.value
            return self._build_node_from_intent(intent, line_tokens[1:], line_tokens)

        # Check for method definition or function call
        if first.type == ENLGMTokenType.IDENTIFIER and len(line_tokens) > 1 and line_tokens[1].value == "(":
            # Function call
            fn_name = first.value
            args = [t.value for t in line_tokens[2:] if t.value not in (")", ",")]
            return CallFunctionNode(fn_name=fn_name, args=args)

        return None

    def _build_node_from_intent(self, intent: str, args: List[Token], full_line: List[Token]) -> Any:
        props = _parse_props_from_tokens(args)

        # ── State Declarations & Assignments ──
        if intent == "STATE_DECLARE":
            # create x as 0  or  create name as "Prayas"
            var_name = args[0].value if args else "val"
            val_tokens = [t for t in args[1:] if t.value.lower() not in ("as", "is", "=")]
            val_expr = _tokens_to_str(val_tokens) if val_tokens else "0"
            return StateDeclNode(name=var_name, initial_val=val_expr)

        elif intent == "STATE_BLOCK":
            # state: block containing declarations
            state_nodes = self._parse_block() if self._has_child_block() else []
            return state_nodes

        elif intent == "STATE_SET":
            # set count to 10
            clean = [t for t in args if t.value.lower() not in ("to", "=")]
            target = clean[0].value if clean else "x"
            val_expr = _tokens_to_str(clean[1:]) if len(clean) > 1 else "0"
            return StateAssignNode(target=target, op="=", value=val_expr)

        elif intent in ("STATE_INC", "STATE_ADD"):
            # increase count by 1  or  add 1 to count
            clean = [t for t in args if t.value.lower() not in ("by", "to")]
            if len(clean) >= 2:
                # Disambiguate 'increase x by 1' vs 'add 1 to x'
                if clean[0].value.isdigit():
                    val = clean[0].value
                    target = clean[1].value
                else:
                    target = clean[0].value
                    val = clean[1].value
            else:
                target = clean[0].value if clean else "x"
                val = "1"
            return StateAssignNode(target=target, op="+=", value=val)

        elif intent in ("STATE_DEC", "STATE_SUB"):
            clean = [t for t in args if t.value.lower() not in ("by", "from")]
            if len(clean) >= 2:
                if clean[0].value.isdigit():
                    val = clean[0].value
                    target = clean[1].value
                else:
                    target = clean[0].value
                    val = clean[1].value
            else:
                target = clean[0].value if clean else "x"
                val = "1"
            return StateAssignNode(target=target, op="-=", value=val)

        # ── Navigation Actions (Flutter Navigator) ──
        elif intent in ("NAV_PUSH", "NAV_CLEAR", "NAV_REPLACE", "NAV_POP", "NAV_POP_RESULT"):
            is_clear = (intent == "NAV_CLEAR") or any(t.value.lower() in ("clear", "clear_all") for t in args)
            clean = [t for t in args if t.value.lower() not in ("to", "screen", "with", "carrying", "data", "result", "and", "all", "clear")]
            action_map = {
                "NAV_PUSH": "clear" if is_clear else "push",
                "NAV_CLEAR": "clear",
                "NAV_REPLACE": "replace",
                "NAV_POP": "pop",
                "NAV_POP_RESULT": "pop_result"
            }
            action = action_map[intent]
            target_screen = clean[0].value if clean else ""
            data_expr = _tokens_to_str(clean[1:]) if len(clean) > 1 else None
            return NavigationNode(action=action, target_screen=target_screen, data_expr=data_expr)

        # ── Feedback / Alerts ──
        elif intent in ("SHOW_TOAST", "SHOW_SNACKBAR"):
            msg_tokens = [t for t in args if t.type == ENLGMTokenType.STRING or t.type == ENLGMTokenType.IDENTIFIER]
            msg_expr = _tokens_to_str(msg_tokens) if msg_tokens else '""'
            fb_type = "toast" if intent == "SHOW_TOAST" else "snackbar"
            return ToastFeedbackNode(feedback_type=fb_type, message_expr=msg_expr)

        elif intent == "SHOW_ALERT":
            msg_tokens = [t for t in args if t.type == ENLGMTokenType.STRING]
            title_expr = msg_tokens[0].value if msg_tokens else "Alert"
            alert_node = ToastFeedbackNode(feedback_type="alert", message_expr=f'"{title_expr}"')
            if self._has_child_block():
                alert_body = self._parse_block()
                for item in alert_body:
                    if isinstance(item, dict):
                        if "message" in item:
                            alert_node.message_expr = item["message"]
                        if "confirm" in item:
                            alert_node.confirm_label = item["confirm"]
                            alert_node.confirm_body = item.get("body", [])
                        if "cancel" in item:
                            alert_node.cancel_label = item["cancel"]
            return alert_node

        # ── Layout Widgets ──
        elif intent in (
            "WIDGET_COLUMN", "WIDGET_COLUMN_CENTER", "WIDGET_COLUMN_SPACED",
            "WIDGET_ROW", "WIDGET_ROW_CENTER", "WIDGET_ROW_SPACED",
            "WIDGET_STACK", "WIDGET_CENTER", "WIDGET_SCROLL", "WIDGET_SAFEAREA",
            "WIDGET_EXPANDED", "WIDGET_EXPANDED_FLEX", "WIDGET_FLEXIBLE", "WIDGET_FLEXIBLE_FLEX",
            "WIDGET_CONTAINER", "WIDGET_CARD", "WIDGET_PADDING", "WIDGET_PADDING_ALL", "WIDGET_PADDING_SYMMETRIC"
        ):
            w_type_map = {
                "WIDGET_COLUMN": "column",
                "WIDGET_COLUMN_CENTER": "column_center",
                "WIDGET_COLUMN_SPACED": "column_spaced",
                "WIDGET_ROW": "row",
                "WIDGET_ROW_CENTER": "row_center",
                "WIDGET_ROW_SPACED": "row_spaced",
                "WIDGET_STACK": "stack",
                "WIDGET_CENTER": "center",
                "WIDGET_SCROLL": "scroll",
                "WIDGET_SAFEAREA": "safearea",
                "WIDGET_EXPANDED": "expanded",
                "WIDGET_EXPANDED_FLEX": "expanded",
                "WIDGET_FLEXIBLE": "flexible",
                "WIDGET_FLEXIBLE_FLEX": "flexible",
                "WIDGET_CONTAINER": "container",
                "WIDGET_CARD": "card",
                "WIDGET_PADDING": "padding",
                "WIDGET_PADDING_ALL": "padding_all",
                "WIDGET_PADDING_SYMMETRIC": "padding_symmetric"
            }
            w_type = w_type_map[intent]
            children = []
            on_tap_body = []

            if self._has_child_block():
                child_nodes = self._parse_block()
                for c in child_nodes:
                    if isinstance(c, tuple) and c[0] == "ON_TAP":
                        on_tap_body.extend(c[1])
                    else:
                        children.append(c)

            return WidgetNode(widget_type=w_type, props=props, children=children, on_tap_body=on_tap_body)

        # ── Text Widget ──
        elif intent == "WIDGET_TEXT":
            text_tokens = []
            prop_start = -1
            for idx, t in enumerate(args):
                if t.value.lower() in ("size", "color", "bold", "italic", "align", "font", "max_lines", "overflow"):
                    prop_start = idx
                    break
                text_tokens.append(t)
            
            text_expr = _tokens_to_str(text_tokens) if text_tokens else '""'
            
            # If multi-line property block exists
            if self._has_child_block():
                block_props = self._parse_block()
                for bp in block_props:
                    if isinstance(bp, dict):
                        props.update(bp)

            return TextWidgetNode(text_expr=text_expr, props=props)

        # ── Buttons ──
        elif intent in ("WIDGET_BUTTON", "WIDGET_TEXT_BUTTON", "WIDGET_ICON_BUTTON", "WIDGET_FAB"):
            btn_type = "elevated"
            if intent == "WIDGET_TEXT_BUTTON":
                btn_type = "text"
            elif intent == "WIDGET_ICON_BUTTON":
                btn_type = "icon"
            elif intent == "WIDGET_FAB":
                btn_type = "fab"

            label_expr = None
            icon_name = None
            if args:
                if btn_type in ("icon", "fab"):
                    icon_name = args[0].value.strip('"\'')
                elif args[0].type == ENLGMTokenType.STRING:
                    label_expr = f'"{args[0].value}"'
                else:
                    label_expr = args[0].value

            on_tap_body = []
            if self._has_child_block():
                child_nodes = self._parse_block()
                for c in child_nodes:
                    if isinstance(c, tuple) and c[0] == "ON_TAP":
                        on_tap_body.extend(c[1])
                    elif isinstance(c, NavigationNode) or isinstance(c, ToastFeedbackNode) or isinstance(c, StateAssignNode):
                        on_tap_body.append(c)

            return ButtonWidgetNode(btn_type=btn_type, label_expr=label_expr, icon_name=icon_name, props=props, on_tap_body=on_tap_body)

        # ── Input / TextField ──
        elif intent == "WIDGET_TEXTFIELD":
            id_name = args[0].value.strip('"\'') if args else "inputField"
            placeholder = props.get("placeholder", "")
            input_type = props.get("type", "text")
            is_hidden = "hidden" in props or input_type == "password"
            lines = int(props.get("lines", 1))

            on_submit_body = []
            if self._has_child_block():
                child_nodes = self._parse_block()
                for c in child_nodes:
                    if isinstance(c, tuple) and c[0] == "ON_SUBMIT":
                        on_submit_body.extend(c[1])

            return InputWidgetNode(
                id_name=id_name,
                placeholder=placeholder,
                input_type=input_type,
                is_hidden=is_hidden,
                lines=lines,
                props=props,
                on_submit_body=on_submit_body
            )

        # ── Image & Avatar ──
        elif intent in ("WIDGET_IMAGE_NETWORK", "WIDGET_IMAGE_ASSET"):
            src_type = "asset" if "ASSET" in intent else "network"
            path_or_url = args[0].value.strip('"\'') if args else ""
            return ImageWidgetNode(source_type=src_type, path_or_url=path_or_url, props=props)

        elif intent in ("WIDGET_AVATAR_NETWORK", "WIDGET_AVATAR_ASSET", "WIDGET_AVATAR_INITIALS"):
            src_type = "network"
            if "ASSET" in intent:
                src_type = "asset"
            elif "INITIALS" in intent:
                src_type = "initials"
            val = args[0].value.strip('"\'') if args else ""
            size = props.get("size")
            return AvatarWidgetNode(source_type=src_type, value=val, size=size, props=props)

        # ── Icon ──
        elif intent == "WIDGET_ICON":
            icon_name = args[0].value.strip('"\'') if args else "star"
            return IconWidgetNode(icon_name=icon_name, props=props)

        # ── Spacer & Divider ──
        elif intent in ("WIDGET_SPACER", "WIDGET_SIZED_BOX_H", "WIDGET_SIZED_BOX_W"):
            if intent == "WIDGET_SIZED_BOX_H":
                h = args[0].value if args else "16"
                return SpacerWidgetNode(is_expandable=False, height=h)
            elif intent == "WIDGET_SIZED_BOX_W":
                w = args[0].value if args else "16"
                return SpacerWidgetNode(is_expandable=False, width=w)
            else:
                # If number follows 'spacer 20' -> height 20
                if args and args[0].type == ENLGMTokenType.NUMBER:
                    return SpacerWidgetNode(is_expandable=False, height=args[0].value)
                return SpacerWidgetNode(is_expandable=True)

        elif intent == "WIDGET_DIVIDER":
            return DividerWidgetNode(color=props.get("color"))

        # ── Chip & Progress ──
        elif intent == "WIDGET_CHIP":
            label_val = args[0].value if args else "Tag"
            return ChipWidgetNode(label_expr=f'"{label_val}"', props=props)

        elif intent in ("WIDGET_PROGRESS_LINEAR", "WIDGET_PROGRESS_CIRCULAR"):
            is_circ = ("CIRCULAR" in intent)
            return ProgressWidgetNode(is_circular=is_circ, color=props.get("color"))

        # ── Lists & Grids ──
        elif intent == "WIDGET_LIST":
            items_expr = args[0].value if args else "items"
            template = self._parse_block() if self._has_child_block() else []
            return ListViewWidgetNode(items_expr=items_expr, template=template)

        elif intent == "WIDGET_GRID":
            cols = int(props.get("columns", 2))
            gap = int(props.get("gap", 12))
            children = self._parse_block() if self._has_child_block() else []
            return GridViewWidgetNode(columns=cols, gap=gap, children=children)

        # ── App Bar ──
        elif intent == "WIDGET_APPBAR":
            title_expr = props.get("title", '""')
            actions = []
            if self._has_child_block():
                body_items = self._parse_block()
                for bi in body_items:
                    if isinstance(bi, dict) and "title" in bi:
                        title_expr = f'"{bi["title"]}"'
                    elif isinstance(bi, dict) and "actions" in bi:
                        actions.extend(bi["actions"])
                    else:
                        actions.append(bi)
            return AppBarNode(title_expr=title_expr, actions=actions)

        # ── Bottom Navigation ──
        elif intent == "WIDGET_BOTTOM_NAV":
            nav_items = []
            if self._has_child_block():
                raw_items = self._parse_block()
                for ri in raw_items:
                    if isinstance(ri, BottomNavItem):
                        nav_items.append(ri)
            return BottomNavNode(items=nav_items)

        # ── Drawer ──
        elif intent == "WIDGET_DRAWER":
            header_children = []
            drawer_items = []
            if self._has_child_block():
                raw_items = self._parse_block()
                for ri in raw_items:
                    if isinstance(ri, dict) and "header" in ri:
                        header_children.extend(ri["header"])
                    elif isinstance(ri, DrawerItem):
                        drawer_items.append(ri)
            return DrawerNode(header_children=header_children, items=drawer_items)

        # ── Events (when tapped, when submitted, on change) ──
        elif intent == "EVENT_TAP":
            tap_body = self._parse_block() if self._has_child_block() else []
            return ("ON_TAP", tap_body)

        elif intent == "EVENT_SUBMITTED":
            submit_body = self._parse_block() if self._has_child_block() else []
            return ("ON_SUBMIT", submit_body)

        elif intent == "EVENT_CHANGED":
            change_body = self._parse_block() if self._has_child_block() else []
            return ("ON_CHANGE", change_body)

        # ── Network Calls ──
        elif intent in ("FETCH_GET", "FETCH_POST"):
            method = "POST" if "POST" in intent else "GET"
            clean_tokens = [t for t in args if t.value.lower() not in ("from", "to", "with", "body")]
            url_expr = clean_tokens[0].value if clean_tokens else '""'
            body_expr = clean_tokens[1].value if len(clean_tokens) > 1 else None

            net_node = NetworkCallNode(method=method, url_expr=f'"{url_expr}"', body_expr=body_expr)
            if self._has_child_block():
                callbacks = self._parse_block()
                for cb in callbacks:
                    if isinstance(cb, tuple):
                        if cb[0] == "NET_SUCCESS":
                            net_node.on_success_param = cb[1]
                            net_node.on_success_body = cb[2]
                        elif cb[0] == "NET_FAILURE":
                            net_node.on_failure_param = cb[1]
                            net_node.on_failure_body = cb[2]
            return net_node

        elif intent == "NET_SUCCESS":
            clean_p = [t.value for t in args if t.value.lower() not in ("with", "and", "as", ":", ",")]
            param = clean_p[0] if clean_p else "result"
            body = self._parse_block() if self._has_child_block() else []
            return ("NET_SUCCESS", param, body)

        elif intent == "NET_FAILURE":
            clean_p = [t.value for t in args if t.value.lower() not in ("with", "and", "as", ":", ",")]
            param = clean_p[0] if clean_p else "error"
            body = self._parse_block() if self._has_child_block() else []
            return ("NET_FAILURE", param, body)

        # ── Raw Dart Escape ──
        elif intent == "RAW_DART":
            code_lines = self._gather_raw_block_text()
            return RawDartNode(code="\n".join(code_lines))

        return None
