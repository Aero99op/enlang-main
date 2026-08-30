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
    event_type: str  # "click", "submit", "change", "load", "keydown", "mouseenter", "mouseleave", "message"
    key_filter: Optional[str] = None
    is_variable: bool = False
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
    parent: Optional[str] = None
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class ClassInitNode(ASTNode):
    """Represents a class constructor (to initialize with params: ...)."""
    params: List[str] = field(default_factory=list)
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class ClassSuperNode(ASTNode):
    """Represents super() call in constructor."""
    args: List[str] = field(default_factory=list)

@dataclass
class ShapeDefNode(ASTNode):
    """Represents TypeScript-style interface/shape contract in Enlgs."""
    name: str
    fields: List[Tuple[str, str]] = field(default_factory=list)  # (field_name, field_type)

@dataclass
class ComponentDefNode(ASTNode):
    """Represents a declarative UI Component (React/Vue style in Enlgs)."""
    name: str
    props: List[str] = field(default_factory=list)
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class DOMMakeElementNode(ASTNode):
    """Represents declarative element creation: make element 'div' with class 'card': ..."""
    tag: str
    attrs: Dict[str, str] = field(default_factory=dict)
    target_var: Optional[str] = None
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class DOMAddElementNode(ASTNode):
    """Represents adding a child element inside a make element block."""
    tag: str
    attrs: Dict[str, str] = field(default_factory=dict)
    text: Optional[str] = None

@dataclass
class DOMAppendToNode(ASTNode):
    """Represents appending or prepending an element to a container."""
    child: str
    parent: str
    is_prepend: bool = False

@dataclass
class AnimateTargetNode(ASTNode):
    """Represents Web Animation API / CSS transition animation: animate 'box' over 500 ms with {...}."""
    target: str
    duration_ms: str
    properties: str

@dataclass
class FunctionalPipelineNode(ASTNode):
    """Represents functional array transformation (filter, map, reduce, find, sort)."""
    op: str  # "filter", "map", "reduce", "find", "sort"
    source_expr: str
    target_var: Optional[str] = None
    item_name: str = "item"
    acc_name: Optional[str] = None
    body_expr: str = ""
    initial_expr: Optional[str] = None

@dataclass
class HttpServerNode(ASTNode):
    """Represents a Node.js / Express-style HTTP server in Enlgs."""
    port: str
    routes: List[ASTNode] = field(default_factory=list)

@dataclass
class HttpRouteNode(ASTNode):
    """Represents an HTTP endpoint route (route get '/api': ...)."""
    method: str  # "GET", "POST", "PUT", "DELETE", "ANY"
    path: str
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class HttpReturnJsonNode(ASTNode):
    """Represents returning JSON from an HTTP route."""
    data_expr: str

@dataclass
class StoreStateNode(ASTNode):
    """Represents a state property definition inside a store."""
    name: str
    value_expr: str

@dataclass
class StoreDefNode(ASTNode):
    """Represents a centralized reactive state store (Redux/Zustand style)."""
    name: str
    states: List[Tuple[str, str]] = field(default_factory=list)  # (state_name, initial_value)
    actions: List[FunctionDefNode] = field(default_factory=list)

@dataclass
class World3DNode(ASTNode):
    """Represents a Three.js / WebGL 3D canvas world declaration."""
    canvas_target: str
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class AnimationFrameLoopNode(ASTNode):
    """Represents a 60FPS requestAnimationFrame loop block."""
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class RenderSceneNode(ASTNode):
    """Represents rendering a 3D scene: render scene with camera."""
    scene: str = "scene"
    camera: str = "camera"

@dataclass
class MoveTargetNode(ASTNode):
    """Represents setting 3D object position: move obj to (x, y, z)."""
    target: str
    coordinates: List[str] = field(default_factory=list)

@dataclass
class RotateByNode(ASTNode):
    """Represents 3D object rotation: rotate obj by x 0.01, y 0.02."""
    target: str
    rotations: Dict[str, str] = field(default_factory=dict)

@dataclass
class TranslateByNode(ASTNode):
    """Represents 3D object translation: translate obj by x 1, y 2, z 3."""
    target: str
    translations: Dict[str, str] = field(default_factory=dict)

@dataclass
class ExtractDestructureNode(ASTNode):
    """Represents destructuring assignment: extract name, age from user."""
    variables: List[str] = field(default_factory=list)
    source_expr: str = ""

@dataclass
class WebSocketConnectNode(ASTNode):
    """Represents WebSocket connection."""
    url: str
    socket_var: str = "socket"

@dataclass
class WebSocketReceiveNode(ASTNode):
    """Represents WebSocket onmessage event."""
    socket_var: str = "socket"
    data_var: str = "data"
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class GeneratorDefNode(ASTNode):
    """Represents a JavaScript generator function (function*)."""
    name: str
    params: List[str] = field(default_factory=list)
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class GeneratorYieldNode(ASTNode):
    """Represents yield value in a generator."""
    value: Optional[str] = None

@dataclass
class PreventDefaultNode(ASTNode):
    """Represents event.preventDefault()."""
    pass

@dataclass
class ListAddNode(ASTNode):
    """Represents adding/pushing an item to an array: add X to list / push X to list."""
    item: str
    target: str

@dataclass
class ListRemoveAtNode(ASTNode):
    """Represents removing an item at an index from an array: remove item at idx from list."""
    index: str
    target: str

@dataclass
class ListInsertNode(ASTNode):
    """Represents inserting an item at an index into an array: insert X at idx in list."""
    item: str
    index: str
    target: str

@dataclass
class ScriptNode(ASTNode):
    """Root AST Node representing the compiled script."""
    body: List[ASTNode] = field(default_factory=list)
