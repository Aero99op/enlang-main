"""enlgs JavaScript Emitter.

Compiles an ENLGS AST (ScriptNode) into standard, clean, modern JavaScript.
Handles automatic async/await for network requests, DOM bindings, and event wrappers.
"""

from typing import List
from .ast_nodes import (
    ASTNode, ScriptNode, RawJSNode, VarDeclNode, VarAssignNode, OutputNode,
    FunctionDefNode, FunctionCallNode, ReturnNode, IfNode,
    LoopRepeatNode, LoopForNode, LoopWhileNode, DOMSetNode, DOMRefreshNode,
    DOMClassNode, DOMVisibilityNode, EventNode, FetchNode, TimerNode,
    BrowserActionNode, StorageNode, TryCatchNode, ClassDefNode, PreventDefaultNode
)

def _dom_el(target: str) -> str:
    """Helper to generate reliable element getter: checks id first, then selector."""
    target_clean = target.strip('"\'')
    if target_clean in ("window", "document", "body"):
        return f"document.{target_clean}" if target_clean == "body" else target_clean
    if target_clean.startswith(".") or target_clean.startswith("#") or " " in target_clean:
        return f'document.querySelector("{target_clean}")'
    return f'(document.getElementById("{target_clean}") || document.querySelector("{target_clean}"))'

class ENLGSEmitter:
    """Emits clean JavaScript code from an AST."""

    def __init__(self, script: ScriptNode):
        self.script = script
        self._loop_counter = 0

    def emit(self) -> str:
        lines: List[str] = []
        for node in self.script.body:
            code = self._emit_node(node, indent=0)
            if code:
                lines.append(code)
        return "\n\n".join(lines)

    def _emit_node(self, node: ASTNode, indent: int = 0) -> str:
        pad = "  " * indent

        # ── Raw JS Passthrough ──
        if isinstance(node, RawJSNode):
            return f"{pad}{node.code}"

        # ── Variables ──
        elif isinstance(node, VarDeclNode):
            val = f" = {node.value}" if node.value is not None else ""
            return f"{pad}{node.kind} {node.name}{val};"

        elif isinstance(node, VarAssignNode):
            return f"{pad}{node.name} {node.op} {node.value};"

        # ── Output ──
        elif isinstance(node, OutputNode):
            args_str = ", ".join(node.args)
            if node.method == "alert":
                return f"{pad}alert({args_str});"
            elif node.method == "warn":
                return f"{pad}console.warn({args_str});"
            elif node.method == "error":
                return f"{pad}console.error({args_str});"
            return f"{pad}console.log({args_str});"

        # ── Prevent Default ──
        elif isinstance(node, PreventDefaultNode):
            return f"{pad}if (typeof event !== 'undefined' && event.preventDefault) event.preventDefault();"

        # ── Functions ──
        elif isinstance(node, FunctionDefNode):
            async_prefix = "async " if node.is_async else ""
            params_str = ", ".join(node.params)
            lines = [f"{pad}{async_prefix}function {node.name}({params_str}) {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(f"{pad}}}")
            return "\n".join(lines)

        elif isinstance(node, FunctionCallNode):
            args_str = ", ".join(node.args)
            return f"{pad}{node.name}({args_str});"

        elif isinstance(node, ReturnNode):
            val_str = f" {node.value}" if node.value is not None else ""
            return f"{pad}return{val_str};"

        # ── Conditionals ──
        elif isinstance(node, IfNode):
            lines = [f"{pad}if ({node.condition}) {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(f"{pad}}}")

            for elif_cond, elif_body in node.elif_branches:
                lines.append(f"{pad}else if ({elif_cond}) {{")
                for child in elif_body:
                    c = self._emit_node(child, indent + 1)
                    if c:
                        lines.append(c)
                lines.append(f"{pad}}}")

            if node.else_body is not None:
                lines.append(f"{pad}else {{")
                for child in node.else_body:
                    c = self._emit_node(child, indent + 1)
                    if c:
                        lines.append(c)
                lines.append(f"{pad}}}")

            return "\n".join(lines)

        # ── Loops ──
        elif isinstance(node, LoopRepeatNode):
            self._loop_counter += 1
            idx_var = f"_i{self._loop_counter}"
            lines = [f"{pad}for (let {idx_var} = 0; {idx_var} < {node.count}; {idx_var}++) {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(f"{pad}}}")
            return "\n".join(lines)

        elif isinstance(node, LoopForNode):
            lines = [f"{pad}for (const {node.item_name} of {node.iterable}) {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(f"{pad}}}")
            return "\n".join(lines)

        elif isinstance(node, LoopWhileNode):
            lines = [f"{pad}while ({node.condition}) {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(f"{pad}}}")
            return "\n".join(lines)

        # ── DOM Setters ──
        elif isinstance(node, DOMSetNode):
            el = _dom_el(node.target)
            if node.prop_type == "text":
                return f"{pad}{el}.textContent = {node.value};"
            elif node.prop_type == "html":
                return f"{pad}{el}.innerHTML = {node.value};"
            elif node.prop_type == "value":
                return f"{pad}{el}.value = {node.value};"
            elif node.prop_type == "color":
                return f"{pad}{el}.style.color = {node.value};"
            elif node.prop_type == "background":
                return f"{pad}{el}.style.background = {node.value};"
            elif node.prop_type == "width":
                val = f"{node.value}px" if node.value.isdigit() else node.value
                return f"{pad}{el}.style.width = {val};"
            elif node.prop_type == "height":
                val = f"{node.value}px" if node.value.isdigit() else node.value
                return f"{pad}{el}.style.height = {val};"
            elif node.prop_type == "style" and node.style_prop:
                return f"{pad}{el}.style['{node.style_prop}'] = {node.value};"
            return f"{pad}{el}.textContent = {node.value};"

        elif isinstance(node, DOMRefreshNode):
            el = _dom_el(node.target)
            return f"{pad}{el}.textContent = {node.value};"

        elif isinstance(node, DOMClassNode):
            el = _dom_el(node.target)
            return f"{pad}{el}.classList.{node.action}('{node.class_name}');"

        elif isinstance(node, DOMVisibilityNode):
            el = _dom_el(node.target)
            display_val = "''" if node.action == "show" else "'none'"
            return f"{pad}{el}.style.display = {display_val};"

        # ── Events ──
        elif isinstance(node, EventNode):
            el = _dom_el(node.target)
            ev_name = node.event_type
            if ev_name == "load" and node.target == "window":
                el = "window"
                ev_name = "DOMContentLoaded"

            lines = [f"{pad}{el}.addEventListener('{ev_name}', function(event) {{"]
            if node.key_filter:
                lines.append(f"{pad}  if (event.key !== '{node.key_filter}') return;")
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(f"{pad}}});")
            return "\n".join(lines)

        # ── Fetch / Network ──
        elif isinstance(node, FetchNode):
            lines = [f"{pad}(async function() {{", f"{pad}  try {{"]
            if node.method == "POST":
                body_opt = f", body: JSON.stringify({node.body_expr})" if node.body_expr else ""
                lines.append(f"{pad}    const response = await fetch('{node.url}', {{ method: 'POST', headers: {{ 'Content-Type': 'application/json' }}{body_opt} }});")
            else:
                lines.append(f"{pad}    const response = await fetch('{node.url}');")

            lines.append(f"{pad}    const {node.response_var} = await response.json();")
            for child in node.body:
                c = self._emit_node(child, indent + 2)
                if c:
                    lines.append(c)
            lines.append(f"{pad}  }} catch (err) {{")
            lines.append(f"{pad}    console.error('Fetch error:', err);")
            lines.append(f"{pad}  }}")
            lines.append(f"{pad}}})();")
            return "\n".join(lines)

        # ── Timers ──
        elif isinstance(node, TimerNode):
            fn_name = "setTimeout" if node.timer_type == "after" else "setInterval"
            lines = [f"{pad}{fn_name}(function() {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(pad + "}, " + str(node.duration_ms) + ");")
            return "\n".join(lines)

        # ── Browser Actions ──
        elif isinstance(node, BrowserActionNode):
            if node.action == "redirect":
                return f"{pad}window.location.href = {node.arg};"
            elif node.action == "reload":
                return f"{pad}window.location.reload();"
            elif node.action == "back":
                return f"{pad}window.history.back();"
            elif node.action == "forward":
                return f"{pad}window.history.forward();"
            elif node.action == "scroll":
                target_el = _dom_el(node.arg) if node.arg else "window"
                return f"{pad}{target_el}.scrollIntoView({{ behavior: 'smooth' }});"
            elif node.action == "copy":
                return f"{pad}navigator.clipboard.writeText({node.arg});"
            elif node.action == "open":
                return f"{pad}window.open({node.arg}, '_blank');"
            return ""

        # ── Storage ──
        elif isinstance(node, StorageNode):
            if node.action == "set":
                return f"{pad}localStorage.setItem('{node.key}', {node.value});"
            elif node.action == "get":
                return f"{pad}localStorage.getItem('{node.key}');"
            elif node.action == "remove":
                return f"{pad}localStorage.removeItem('{node.key}');"
            elif node.action == "clear":
                return f"{pad}localStorage.clear();"
            return ""

        # ── Error Handling ──
        elif isinstance(node, TryCatchNode):
            lines = [f"{pad}try {{"]
            for child in node.try_body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(f"{pad}}} catch ({node.error_var}) {{")
            for child in node.catch_body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(f"{pad}}}")
            return "\n".join(lines)

        # ── Classes ──
        elif isinstance(node, ClassDefNode):
            lines = [f"{pad}class {node.name} {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(f"{pad}}}")
            return "\n".join(lines)

        return ""
