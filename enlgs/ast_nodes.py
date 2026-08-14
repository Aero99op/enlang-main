"""enlgs Abstract Syntax Tree (AST) Nodes.

Defines the strongly-typed nodes representing all JavaScript behavior constructs.
"""

from dataclasses import dataclass, field
from typing import List, Optional, Tuple, Dict, Any

class ASTNode:
    """Base class for all .enlgs AST nodes."""
    pass

@dataclass
class RawJSNode(ASTNode):
    """Represents a raw JavaScript passthrough line or block."""
    code: str

@dataclass
class VarDeclNode(ASTNode):
    """Represents a variable declaration (let / const)."""
    kind: str  # "let" or "const"
    name: str
    value: Optional[str] = None

@dataclass
class VarAssignNode(ASTNode):
    """Represents a variable assignment or compound mutation (x = 5, x += 1)."""
    name: str
    op: str  # "=", "+=", "-=", "*=", "/="
    value: str

@dataclass
class OutputNode(ASTNode):
    """Represents console/browser output (log, warn, error, alert)."""
    method: str  # "log", "warn", "error", "alert"
    args: List[str] = field(default_factory=list)

@dataclass
class FunctionDefNode(ASTNode):
    """Represents a JavaScript function definition."""
    name: str
    params: List[str] = field(default_factory=list)
    body: List[ASTNode] = field(default_factory=list)
    is_async: bool = False

@dataclass
class FunctionCallNode(ASTNode):
    """Represents a function call statement."""
    name: str
    args: List[str] = field(default_factory=list)

@dataclass
class ReturnNode(ASTNode):
    """Represents a return statement."""
    value: Optional[str] = None

@dataclass
class IfNode(ASTNode):
    """Represents an if/else if/else conditional structure."""
    condition: str
    body: List[ASTNode] = field(default_factory=list)
    elif_branches: List[Tuple[str, List[ASTNode]]] = field(default_factory=list)
    else_body: Optional[List[ASTNode]] = None

@dataclass
class LoopRepeatNode(ASTNode):
    """Represents a numeric loop: repeat N times: ..."""
    count: str
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class LoopForNode(ASTNode):
    """Represents an iterable for loop: for each item in list: ..."""
    item_name: str
    iterable: str
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class LoopWhileNode(ASTNode):
    """Represents a while loop: while condition: ..."""
    condition: str
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class DOMSetNode(ASTNode):
    """Represents DOM property assignment (text, html, value, style)."""
    target: str
    prop_type: str  # "text", "html", "value", "color", "background", "width", "height", "style"
    value: str
    style_prop: Optional[str] = None

@dataclass
class DOMRefreshNode(ASTNode):
    """Represents refresh 'id' with value (textContent update)."""
    target: str
    value: str

@dataclass
class DOMClassNode(ASTNode):
    """Represents classList manipulation (add, remove, toggle)."""
    action: str  # "add", "remove", "toggle"
    class_name: str
    target: str

@dataclass
class DOMVisibilityNode(ASTNode):
    """Represents showing/hiding a DOM element."""
    action: str  # "show" (display = block/default) or "hide" (display = none)
    target: str

@dataclass
class EventNode(ASTNode):
    """Represents an event listener binding (when 'btn' clicked: ...)."""
    target: str
    event_type: str  # "click", "submit", "change", "load", "keydown", "mouseenter", "mouseleave"
    key_filter: Optional[str] = None
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class FetchNode(ASTNode):
    """Represents an async network fetch or send operation."""
    method: str  # "GET" or "POST"
    url: str
    body_expr: Optional[str] = None
    response_var: str = "data"
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class TimerNode(ASTNode):
    """Represents a setTimeout (after) or setInterval (every) timer."""
    timer_type: str  # "after" (setTimeout) or "every" (setInterval)
    duration_ms: str
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class BrowserActionNode(ASTNode):
    """Represents browser navigation, clipboard, scrolling, or reload."""
    action: str  # "redirect", "reload", "back", "forward", "scroll", "copy", "open"
    arg: Optional[str] = None

@dataclass
class StorageNode(ASTNode):
    """Represents localStorage operations."""
    action: str  # "set", "get", "remove", "clear"
    key: Optional[str] = None
    value: Optional[str] = None

@dataclass
class TryCatchNode(ASTNode):
    """Represents a try/catch error handling block."""
    try_body: List[ASTNode] = field(default_factory=list)
    error_var: str = "error"
    catch_body: List[ASTNode] = field(default_factory=list)

@dataclass
class ClassDefNode(ASTNode):
    """Represents a JavaScript ES6 class definition."""
    name: str
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class PreventDefaultNode(ASTNode):
    """Represents event.preventDefault()."""
    pass

@dataclass
class ScriptNode(ASTNode):
    """Root AST Node representing the compiled script."""
    body: List[ASTNode] = field(default_factory=list)
