"""enlgm Abstract Syntax Tree (AST) Node Definitions.

Defines dataclasses for the Flutter / Dart Widget Tree, Navigation, State Management,
Lifecycle hooks, Network operations, and custom Widget Blueprints.
"""

from dataclasses import dataclass, field
from typing import List, Dict, Any, Optional

@dataclass
class ASTNode:
    pass

@dataclass
class MobileRootNode(ASTNode):
    imports: List['FlutterImportNode'] = field(default_factory=list)
    app: Optional['AppDefNode'] = None
    screens: List['ScreenDefNode'] = field(default_factory=list)
    blueprints: List['WidgetBlueprintNode'] = field(default_factory=list)
    raw_dart_blocks: List['RawDartNode'] = field(default_factory=list)

@dataclass
class FlutterImportNode(ASTNode):
    package_name: str
    is_flutter: bool = True
    as_alias: Optional[str] = None

@dataclass
class AppDefNode(ASTNode):
    name: str
    theme: str = "dark"           # "dark" or "light"
    primary_color: Optional[str] = None  # Hex or named color
    home_screen: str = "HomeScreen"

@dataclass
class ScreenDefNode(ASTNode):
    name: str
    is_stateful: bool = False
    title: Optional[str] = None
    state_vars: List['StateDeclNode'] = field(default_factory=list)
    lifecycle_inits: List[ASTNode] = field(default_factory=list)
    lifecycle_disposes: List[ASTNode] = field(default_factory=list)
    app_bar: Optional['AppBarNode'] = None
    body: List[ASTNode] = field(default_factory=list)
    fab: Optional['ButtonWidgetNode'] = None
    bottom_bar: Optional['BottomNavNode'] = None
    drawer: Optional['DrawerNode'] = None
    methods: List['MethodDefNode'] = field(default_factory=list)

@dataclass
class MethodDefNode(ASTNode):
    name: str
    params: List[str] = field(default_factory=list)
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class StateDeclNode(ASTNode):
    name: str
    initial_val: str
    type_hint: Optional[str] = None

@dataclass
class StateAssignNode(ASTNode):
    target: str
    op: str = "="   # "=", "+=", "-="
    value: str = ""

@dataclass
class NavigationNode(ASTNode):
    action: str           # "push", "push_data", "pop", "pop_result", "replace", "clear"
    target_screen: str = ""
    data_expr: Optional[str] = None

@dataclass
class ToastFeedbackNode(ASTNode):
    feedback_type: str    # "toast", "snackbar", "alert"
    message_expr: str = ""
    duration: Optional[int] = None
    confirm_label: Optional[str] = None
    confirm_body: List[ASTNode] = field(default_factory=list)
    cancel_label: Optional[str] = None

@dataclass
class NetworkCallNode(ASTNode):
    method: str           # "GET" or "POST"
    url_expr: str = ""
    body_expr: Optional[str] = None
    on_success_param: str = "result"
    on_success_body: List[ASTNode] = field(default_factory=list)
    on_failure_param: str = "error"
    on_failure_body: List[ASTNode] = field(default_factory=list)

@dataclass
class WidgetNode(ASTNode):
    widget_type: str      # "column", "row", "stack", "center", "scroll", "safearea", "expanded", "container", "card", "padding"
    props: Dict[str, Any] = field(default_factory=dict)
    children: List[ASTNode] = field(default_factory=list)
    on_tap_body: List[ASTNode] = field(default_factory=list)

@dataclass
class TextWidgetNode(ASTNode):
    text_expr: str
    props: Dict[str, Any] = field(default_factory=dict)

@dataclass
class IconWidgetNode(ASTNode):
    icon_name: str
    props: Dict[str, Any] = field(default_factory=dict)

@dataclass
class ImageWidgetNode(ASTNode):
    source_type: str      # "network" or "asset"
    path_or_url: str
    width: Optional[str] = None
    height: Optional[str] = None
    props: Dict[str, Any] = field(default_factory=dict)

@dataclass
class AvatarWidgetNode(ASTNode):
    source_type: str      # "network", "asset", "initials"
    value: str
    size: Optional[str] = None
    props: Dict[str, Any] = field(default_factory=dict)

@dataclass
class ButtonWidgetNode(ASTNode):
    btn_type: str         # "elevated", "text", "icon", "fab"
    label_expr: Optional[str] = None
    icon_name: Optional[str] = None
    props: Dict[str, Any] = field(default_factory=dict)
    on_tap_body: List[ASTNode] = field(default_factory=list)

@dataclass
class InputWidgetNode(ASTNode):
    id_name: str
    placeholder: str = ""
    input_type: str = "text"  # "text", "email", "password", "number"
    is_hidden: bool = False
    lines: int = 1
    props: Dict[str, Any] = field(default_factory=dict)
    on_submit_body: List[ASTNode] = field(default_factory=list)

@dataclass
class SpacerWidgetNode(ASTNode):
    is_expandable: bool = False
    height: Optional[str] = None
    width: Optional[str] = None

@dataclass
class DividerWidgetNode(ASTNode):
    color: Optional[str] = None
    thickness: Optional[str] = None

@dataclass
class ChipWidgetNode(ASTNode):
    label_expr: str
    props: Dict[str, Any] = field(default_factory=dict)

@dataclass
class ProgressWidgetNode(ASTNode):
    is_circular: bool = True
    color: Optional[str] = None
    size: Optional[str] = None

@dataclass
class ControlWidgetNode(ASTNode):
    control_type: str     # "switch", "slider", "checkbox", "dropdown"
    value_var: str
    label: Optional[str] = None
    options: List[str] = field(default_factory=list)
    min_val: float = 0.0
    max_val: float = 100.0
    on_change_body: List[ASTNode] = field(default_factory=list)

@dataclass
class ListViewWidgetNode(ASTNode):
    items_expr: str
    item_var: str = "item"
    template: List[ASTNode] = field(default_factory=list)

@dataclass
class GridViewWidgetNode(ASTNode):
    columns: int = 2
    gap: int = 12
    children: List[ASTNode] = field(default_factory=list)

@dataclass
class AppBarNode(ASTNode):
    title_expr: str = ""
    has_back_btn: bool = True
    actions: List[ASTNode] = field(default_factory=list)

@dataclass
class BottomNavItem(ASTNode):
    icon_name: str
    label: str
    on_tap_body: List[ASTNode] = field(default_factory=list)

@dataclass
class BottomNavNode(ASTNode):
    items: List[BottomNavItem] = field(default_factory=list)

@dataclass
class DrawerItem(ASTNode):
    icon_name: str
    label: str
    on_tap_body: List[ASTNode] = field(default_factory=list)

@dataclass
class DrawerNode(ASTNode):
    header_children: List[ASTNode] = field(default_factory=list)
    items: List[DrawerItem] = field(default_factory=list)

@dataclass
class WidgetBlueprintNode(ASTNode):
    name: str
    params: List[str] = field(default_factory=list)
    body: List[ASTNode] = field(default_factory=list)

@dataclass
class BlueprintInstanceNode(ASTNode):
    name: str
    named_args: Dict[str, str] = field(default_factory=dict)

@dataclass
class CallFunctionNode(ASTNode):
    fn_name: str
    args: List[str] = field(default_factory=list)

@dataclass
class RawDartNode(ASTNode):
    code: str
