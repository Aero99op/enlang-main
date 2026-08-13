"""enlgf Abstract Syntax Tree (AST) Nodes.

Represents structural elements, text content, inline styles, and JS behaviors.
"""

from dataclasses import dataclass, field
from typing import List, Dict, Optional

class ASTNode:
    """Base class for all .enlgf AST nodes."""
    pass

@dataclass
class TextNode(ASTNode):
    """Represents raw text inside an HTML element."""
    text: str

@dataclass
class ElementNode(ASTNode):
    """Represents an HTML tag element."""
    tag: str
    attributes: Dict[str, str] = field(default_factory=dict)
    styles: Dict[str, str] = field(default_factory=dict)
    events: Dict[str, str] = field(default_factory=dict) # e.g. {"onclick": "alert('hi')"}
    text_content: Optional[str] = None
    children: List[ASTNode] = field(default_factory=list)
    is_self_closing: bool = False

@dataclass
class DocumentNode(ASTNode):
    """Represents the root HTML document."""
    attributes: Dict[str, str] = field(default_factory=dict)
    head_children: List[ASTNode] = field(default_factory=list)
    body_children: List[ASTNode] = field(default_factory=list)
    children: List[ASTNode] = field(default_factory=list) # Fallback root children
