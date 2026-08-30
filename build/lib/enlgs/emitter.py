"""enlgs JavaScript Emitter.

Compiles an ENLGS AST (ScriptNode) into standard, clean, modern JavaScript.
Handles OOP classes, async/await, generators, DOM tree construction, component rendering,
functional array pipelines, WebSockets, Three.js 3D helpers, and full Node.js HTTP servers.
"""

from typing import List
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
    RenderSceneNode, MoveTargetNode,
    ExtractDestructureNode, WebSocketConnectNode, WebSocketReceiveNode,
    GeneratorDefNode, GeneratorYieldNode, PreventDefaultNode,
    ListAddNode, ListRemoveAtNode, ListInsertNode
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

        elif isinstance(node, ExtractDestructureNode):
            vars_str = ", ".join(node.variables)
            return f"{pad}const {{ {vars_str} }} = {node.source_expr};"

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

        # ── Functions & Async ──
        elif isinstance(node, FunctionDefNode):
            async_prefix = "async " if node.is_async else ""
            params_str = ", ".join(node.params)
            lines = [f"{pad}{async_prefix}function {node.name}({params_str}) {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(pad + "}")
            return "\n".join(lines)

        elif isinstance(node, GeneratorDefNode):
            params_str = ", ".join(node.params)
            lines = [f"{pad}function* {node.name}({params_str}) {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(pad + "}")
            return "\n".join(lines)

        elif isinstance(node, GeneratorYieldNode):
            val_str = f" {node.value}" if node.value is not None else ""
            return f"{pad}yield{val_str};"

        elif isinstance(node, FunctionCallNode):
            args_str = ", ".join(node.args)
            return f"{pad}{node.name}({args_str});"

        elif isinstance(node, ReturnNode):
            val_str = f" {node.value}" if node.value is not None else ""
            return f"{pad}return{val_str};"

        elif isinstance(node, HttpReturnJsonNode):
            return pad + "res.writeHead(200, { 'Content-Type': 'application/json' });\n" + pad + f"res.end(JSON.stringify({node.data_expr}));"

        # ── Conditionals ──
        elif isinstance(node, IfNode):
            lines = [f"{pad}if ({node.condition}) {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(pad + "}")

            for elif_cond, elif_body in node.elif_branches:
                lines.append(f"{pad}else if ({elif_cond}) {{")
                for child in elif_body:
                    c = self._emit_node(child, indent + 1)
                    if c:
                        lines.append(c)
                lines.append(pad + "}")

            if node.else_body:
                lines.append(pad + "else {")
                for child in node.else_body:
                    c = self._emit_node(child, indent + 1)
                    if c:
                        lines.append(c)
                lines.append(pad + "}")

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
            lines.append(pad + "}")
            return "\n".join(lines)

        elif isinstance(node, LoopForNode):
            lines = [f"{pad}for (const {node.item_name} of {node.iterable}) {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(pad + "}")
            return "\n".join(lines)

        elif isinstance(node, LoopWhileNode):
            lines = [f"{pad}while ({node.condition}) {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(pad + "}")
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
            display_val = "'block'" if node.action == "show" else "'none'"
            return f"{pad}{el}.style.display = {display_val};"

        # ── Declarative UI Elements ──
        elif isinstance(node, DOMMakeElementNode):
            lines = [pad + "(function() {", f"{pad}  const el = document.createElement('{node.tag}');"]
            for k, v in node.attrs.items():
                if k == "class":
                    lines.append(f"{pad}  el.className = '{v}';")
                elif k == "id":
                    lines.append(f"{pad}  el.id = '{v}';")
                elif k == "style":
                    lines.append(f"{pad}  el.setAttribute('style', '{v}');")
                else:
                    lines.append(f"{pad}  el.setAttribute('{k}', '{v}');")
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(pad + "  return el;")
            lines.append(pad + "})();")
            return "\n".join(lines)

        elif isinstance(node, DOMAddElementNode):
            lines = [pad + "(function(parentEl) {", f"{pad}  const child = document.createElement('{node.tag}');"]
            if node.text:
                lines.append(f"{pad}  child.textContent = {node.text};")
            for k, v in node.attrs.items():
                if k == "class":
                    lines.append(f"{pad}  child.className = '{v}';")
                else:
                    lines.append(f"{pad}  child.setAttribute('{k}', '{v}');")
            lines.append(pad + "  (parentEl || document.body).appendChild(child);")
            lines.append(pad + "})(typeof el !== 'undefined' ? el : null);")
            return "\n".join(lines)

        elif isinstance(node, DOMAppendToNode):
            parent_el = _dom_el(node.parent)
            method = "prepend" if node.is_prepend else "appendChild"
            return f"{pad}{parent_el}.{method}({node.child});"

        elif isinstance(node, AnimateTargetNode):
            el = _dom_el(node.target)
            return pad + f"{el}.animate({node.properties}, {{ duration: {node.duration_ms}, fill: 'forwards' }});"

        # ── List / Array Operations ──
        elif isinstance(node, ListAddNode):
            return f"{pad}{node.target}.push({node.item});"

        elif isinstance(node, ListRemoveAtNode):
            return f"{pad}{node.target}.splice({node.index}, 1);"

        elif isinstance(node, ListInsertNode):
            return f"{pad}{node.target}.splice({node.index}, 0, {node.item});"

        # ── TypeScript Shapes ──
        elif isinstance(node, ShapeDefNode):
            lines = [pad + "/**", pad + f" * @typedef {{Object}} {node.name}"]
            for fname, ftype in node.fields:
                lines.append(pad + f" * @property {{{ftype}}} {fname}")
            lines.append(pad + " */")
            return "\n".join(lines)

        # ── Declarative UI Components ──
        elif isinstance(node, ComponentDefNode):
            props_str = ", ".join(node.props)
            lines = [
                pad + "function " + node.name + "({ " + props_str + " } = {}) {",
                f"{pad}  const componentRoot = document.createElement('div');",
                f"{pad}  componentRoot.className = '{node.name.lower()}-component';"
            ]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(pad + "  return componentRoot;")
            lines.append(pad + "}")
            return "\n".join(lines)

        # ── Events ──
        elif isinstance(node, EventNode):
            el = node.target if node.is_variable else _dom_el(node.target)
            ev_name = node.event_type
            if ev_name == "load" and node.target == "window":
                el = "window"
                ev_name = "DOMContentLoaded"

            if el in ("window", "document", "document.body"):
                lines = [f"{pad}{el}.addEventListener('{ev_name}', function(event) {{"]
                if node.key_filter:
                    lines.append(f"{pad}  if (event.key !== '{node.key_filter}') return;")
                for child in node.body:
                    c = self._emit_node(child, indent + 1)
                    if c:
                        lines.append(c)
                lines.append(pad + "});")
            else:
                lines = [
                    f"{pad}(function() {{",
                    f"{pad}  const targetEl = {el};",
                    f"{pad}  if (targetEl != null) {{",
                    f"{pad}    targetEl.addEventListener('{ev_name}', function(event) {{"
                ]
                if node.key_filter:
                    lines.append(f"{pad}      if (event.key !== '{node.key_filter}') return;")
                for child in node.body:
                    c = self._emit_node(child, indent + 3)
                    if c:
                        lines.append(c)
                lines.append(pad + "    });")
                lines.append(pad + "  }")
                lines.append(pad + "})();")
            return "\n".join(lines)

        # ── Fetch / Network ──
        elif isinstance(node, FetchNode):
            lines = [pad + "(async function() {", pad + "  try {"]
            if node.method == "POST":
                body_opt = f", body: JSON.stringify({node.body_expr})" if node.body_expr else ""
                lines.append(pad + f"    const response = await fetch('{node.url}', {{ method: 'POST', headers: {{ 'Content-Type': 'application/json' }}{body_opt} }});")
            else:
                lines.append(pad + f"    const response = await fetch('{node.url}');")

            lines.append(pad + f"    const {node.response_var} = await response.json();")
            for child in node.body:
                c = self._emit_node(child, indent + 2)
                if c:
                    lines.append(c)
            lines.append(pad + "  } catch (err) {")
            lines.append(pad + "    console.error('Fetch error:', err);")
            lines.append(pad + "  }")
            lines.append(pad + "})();")
            return "\n".join(lines)

        # ── Full-Stack Server ──
        elif isinstance(node, HttpServerNode):
            lines = [
                pad + "const http = require('http');",
                pad + "const server = http.createServer((req, res) => {",
                pad + "  const url = req.url.split('?')[0];",
                pad + "  const method = req.method;"
            ]
            for r in node.routes:
                lines.append(self._emit_node(r, indent + 1))
            lines.append(pad + "  res.writeHead(404, { 'Content-Type': 'text/plain' });")
            lines.append(pad + "  res.end('Not Found');")
            lines.append(pad + "});")
            lines.append(pad + f"server.listen({node.port}, () => console.log('Enlang HTTP Server running on port {node.port}'));")
            return "\n".join(lines)

        elif isinstance(node, HttpRouteNode):
            cond = f"method === '{node.method}' && url === '{node.path}'" if node.method != "ANY" else f"url === '{node.path}'"
            lines = [f"{pad}if ({cond}) {{"]
            for child in node.body:
                lines.append(self._emit_node(child, indent + 1))
            lines.append(pad + "  return;")
            lines.append(pad + "}")
            return "\n".join(lines)

        # ── Centralized State Store ──
        elif isinstance(node, StoreDefNode):
            lines = [
                pad + f"const {node.name} = (function() {{",
                pad + "  let state = {"
            ]
            for sname, sval in node.states:
                lines.append(f"{pad}    {sname}: {sval},")
            lines.append(pad + "  };")
            lines.append(pad + "  const listeners = [];")
            lines.append(pad + "  return {")
            lines.append(pad + "    getState: () => ({ ...state }),")
            lines.append(pad + "    subscribe: (fn) => listeners.push(fn),")
            for act in node.actions:
                params_str = ", ".join(act.params)
                lines.append(f"{pad}    {act.name}: function({params_str}) {{")
                for child in act.body:
                    lines.append(self._emit_node(child, indent + 3))
                lines.append(pad + "      listeners.forEach(fn => fn(state));")
                lines.append(pad + "    },")
            lines.append(pad + "  };")
            lines.append(pad + "})();")
            return "\n".join(lines)

        # ── 3D World & Canvas ──
        elif isinstance(node, World3DNode):
            lines = [
                pad + "(function() {",
                pad + "  if (typeof THREE === 'undefined') return;",
                pad + f"  const canvas = document.getElementById('{node.canvas_target}') || document.querySelector('{node.canvas_target}');",
                pad + "  if (!canvas) return;"
            ]
            for child in node.body:
                lines.append(self._emit_node(child, indent + 1))
            lines.append(pad + "})();")
            return "\n".join(lines)

        elif isinstance(node, AnimationFrameLoopNode):
            lines = [
                pad + "function _enlgs_animLoop() {",
                pad + "  requestAnimationFrame(_enlgs_animLoop);"
            ]
            for child in node.body:
                lines.append(self._emit_node(child, indent + 1))
            lines.append(pad + "}")
            lines.append(pad + "_enlgs_animLoop();")
            return "\n".join(lines)

        elif isinstance(node, RenderSceneNode):
            return pad + f"if (typeof renderer !== 'undefined' && renderer.render) renderer.render({node.scene}, {node.camera});"

        elif isinstance(node, MoveTargetNode):
            coords_str = ", ".join(node.coordinates)
            return pad + f"{node.target}.position.set({coords_str});"

        elif isinstance(node, RotateByNode):
            stmts = []
            for axis, val in node.rotations.items():
                stmts.append(f"{node.target}.rotation.{axis} += {val};")
            return f"{pad}" + f"\n{pad}".join(stmts)

        elif isinstance(node, TranslateByNode):
            stmts = []
            for axis, val in node.translations.items():
                stmts.append(f"{node.target}.position.{axis} += {val};")
            return f"{pad}" + f"\n{pad}".join(stmts)

        # ── WebSockets ──
        elif isinstance(node, WebSocketConnectNode):
            return f"{pad}const {node.socket_var} = new WebSocket('{node.url}');"

        elif isinstance(node, WebSocketReceiveNode):
            lines = [
                f"{pad}{node.socket_var}.addEventListener('message', function(event) {{",
                f"{pad}  let {node.data_var} = event.data;",
                pad + "  try { " + node.data_var + " = JSON.parse(event.data); } catch(e) {}"
            ]
            for child in node.body:
                lines.append(self._emit_node(child, indent + 1))
            lines.append(pad + "});")
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
                return pad + f"{target_el}.scrollIntoView({{ behavior: 'smooth' }});"
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
            lines = [pad + "try {"]
            for child in node.try_body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(pad + f"}} catch ({node.error_var}) {{")
            for child in node.catch_body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(pad + "}")
            return "\n".join(lines)

        # ── OOP / Classes / Blueprints ──
        elif isinstance(node, ClassDefNode):
            heritage = f" extends {node.parent}" if node.parent else ""
            lines = [f"{pad}class {node.name}{heritage} {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(pad + "}")
            return "\n".join(lines)

        elif isinstance(node, ClassInitNode):
            params_str = ", ".join(node.params)
            lines = [f"{pad}constructor({params_str}) {{"]
            for child in node.body:
                c = self._emit_node(child, indent + 1)
                if c:
                    lines.append(c)
            lines.append(pad + "}")
            return "\n".join(lines)

        elif isinstance(node, ClassSuperNode):
            args_str = ", ".join(node.args)
            return f"{pad}super({args_str});"

        return ""
