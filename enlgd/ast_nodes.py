"""enlgd Abstract Syntax Tree (AST) Nodes.

Defines the node structures representing stylesheets, rules, declarations, media queries, variables, and animations.
"""

from dataclasses import dataclass, field
from typing import List, Optional

class ASTNode:
    """Base class for all .enlgd AST nodes."""
    pass

@dataclass
class DeclarationNode(ASTNode):
    """Represents a single CSS property declaration."""
    property_name: str
    value: str

@dataclass
class VariableNode(ASTNode):
    """Represents a CSS Custom Property (:root variable)."""
    var_type: str  # color, size, etc.
    name: str      # e.g. "primary" -> will emit --primary
    value: str     # e.g. "#1e3c72" or "16px"

@dataclass
class RuleNode(ASTNode):
    """Represents a CSS rule set with a selector and declarations."""
    selector: str
    declarations: List[DeclarationNode] = field(default_factory=list)

@dataclass
class MediaRuleNode(ASTNode):
    """Represents an @media query block containing nested rules."""
    query: str  # e.g. "(max-width: 768px)" or "(orientation: portrait)"
    rules: List[RuleNode] = field(default_factory=list)

@dataclass
class KeyframeFrameNode(ASTNode):
    """Represents a single keyframe step (e.g. 0%, 100%, from, to)."""
    stop: str  # e.g. "0%", "100%"
    declarations: List[DeclarationNode] = field(default_factory=list)

@dataclass
class KeyframeNode(ASTNode):
    """Represents an @keyframes animation definition."""
    name: str
    frames: List[KeyframeFrameNode] = field(default_factory=list)

@dataclass
class StylesheetNode(ASTNode):
    """Represents the entire compiled .enlgd stylesheet."""
    variables: List[VariableNode] = field(default_factory=list)
    rules: List[RuleNode] = field(default_factory=list)
    media_rules: List[MediaRuleNode] = field(default_factory=list)
    keyframes: List[KeyframeNode] = field(default_factory=list)
