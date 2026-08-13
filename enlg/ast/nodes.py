"""enlg Abstract Syntax Tree (AST) Nodes.

Defines the strictly-typed structures that represent parsed code.
"""

from dataclasses import dataclass
from typing import Optional

class ASTNode:
    """Base class for all AST nodes."""
    pass

class StatementNode(ASTNode):
    """Base class for statement nodes."""
    pass

class ExpressionNode(ASTNode):
    """Base class for expression nodes."""
    pass

@dataclass
class BinaryOpNode(ExpressionNode):
    """Represents a binary operation (arithmetic, logical, or comparison)."""
    left: ExpressionNode
    op: str
    right: ExpressionNode

@dataclass
class UnaryOpNode(ExpressionNode):
    """Represents a unary operation (e.g. not, -, ~, !)."""
    op: str
    operand: ExpressionNode

@dataclass
class LiteralNode(ExpressionNode):
    """Represents string, number, or boolean literals."""
    value: str
    type_name: str # e.g. 'number', 'string'

@dataclass
class IdentifierNode(ExpressionNode):
    """Represents a variable reference."""
    name: str

@dataclass
class VariableDeclNode(StatementNode):
    """Represents a variable declaration (e.g. declare x = 20)."""
    identifier: str
    value: Optional[ExpressionNode]

@dataclass
class AssignmentNode(StatementNode):
    """Represents a variable assignment (e.g. set x = 30)."""
    identifier: str
    value: ExpressionNode

@dataclass
class OutputNode(StatementNode):
    """Represents a print/display statement."""
    expression: ExpressionNode

@dataclass
class BooleanNode(ExpressionNode):
    """Represents true or false."""
    value: bool

@dataclass
class NullNode(ExpressionNode):
    """Represents a null/none value."""
    pass

@dataclass
class ListNode(ExpressionNode):
    """Represents an array/list of expressions."""
    elements: list[ExpressionNode]

@dataclass
class MapNode(ExpressionNode):
    """Represents a key-value dictionary structure."""
    pairs: dict[str, ExpressionNode]

@dataclass
class BlockNode(ASTNode):
    """Represents a suite of indented statements."""
    statements: list[StatementNode]

@dataclass
class IfNode(StatementNode):
    """Represents an if/elif/else structure."""
    condition: ExpressionNode
    body: BlockNode
    else_body: Optional[BlockNode] = None

@dataclass
class WhileNode(StatementNode):
    """Represents a while loop."""
    condition: ExpressionNode
    body: BlockNode

@dataclass
class ForNode(StatementNode):
    """Represents a for-each loop."""
    iterator: str
    iterable: ExpressionNode
    body: BlockNode

@dataclass
class FunctionDefNode(StatementNode):
    """Represents a custom function definition."""
    name: str
    parameters: list[str]
    body: BlockNode
    is_async: bool = False

@dataclass
class FunctionCallNode(ExpressionNode):
    """Represents a function invocation."""
    name: str
    arguments: list[ExpressionNode]

@dataclass
class ReturnNode(StatementNode):
    """Represents a return statement within a function."""
    expression: Optional[ExpressionNode] = None

@dataclass
class AttemptNode(StatementNode):
    """Represents a try block."""
    body: BlockNode

@dataclass
class RescueNode(StatementNode):
    """Represents a catch/except block."""
    error_name: str
    body: BlockNode

@dataclass
class RaiseNode(StatementNode):
    """Represents throwing an exception."""
    expression: ExpressionNode

@dataclass
class ImportNode(StatementNode):
    """Represents importing a module."""
    module: str
    aliases: list[str]

@dataclass
class ClassDefNode(StatementNode):
    """Represents a custom class blueprint definition."""
    name: str
    base_classes: list[str]
    body: BlockNode

@dataclass
class InstantiateNode(ExpressionNode):
    """Represents the creation of a new object from a class."""
    class_name: str
    arguments: list[ExpressionNode]

@dataclass
class AwaitNode(ExpressionNode):
    """Represents awaiting an async operation."""
    expression: ExpressionNode

@dataclass
class PythonInteropNode(ExpressionNode):
    """Represents a native python function invocation."""
    target: str
    arguments: list[ExpressionNode]

@dataclass
class DomainOpNode(StatementNode):
    """Represents a specialized domain operation (AI, Cybersec, Cloud).
    
    store_result: if set, VM will STORE_VAR result into this variable name.
    
    - store_result + store_back=False → target is NEW variable (LOAD_CONST var name, then STORE_VAR)
      Used by: load, preprocess, restore
    - store_result + store_back=True  → target already EXISTS (LOAD_VAR then STORE_VAR back)
      Used by: train (model is updated in place and result stored back)
    - store_result = "a:b"            → AI_SPLIT tuple unpack into two vars
    """
    op: str
    target: ExpressionNode
    arguments: list[ExpressionNode]
    store_result: Optional[str] = None
    store_back: bool = False
    from_module: Optional[str] = None

