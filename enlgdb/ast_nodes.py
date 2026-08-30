"""AST Node definitions for enlgdb."""

from dataclasses import dataclass, field
from typing import List, Optional, Any, Dict


class ASTNode:
    """Base class for all AST nodes."""
    pass


@dataclass
class DomainHeaderNode(ASTNode):
    domain: str = "enlgdb"


@dataclass
class CreateDatabaseNode(ASTNode):
    db_name: str


@dataclass
class UseDatabaseNode(ASTNode):
    db_name: str


@dataclass
class ShowDatabasesNode(ASTNode):
    pass


@dataclass
class ShowTablesNode(ASTNode):
    db_name: Optional[str] = None


@dataclass
class DropDatabaseNode(ASTNode):
    db_name: str
    confirmation_token: str


@dataclass
class ColumnDefNode(ASTNode):
    name: str
    data_type: str
    is_primary_key: bool = False
    autoincrement: bool = False
    not_null: bool = False
    unique: bool = False
    default_value: Optional[Any] = None
    references_table: Optional[str] = None
    references_column: Optional[str] = None


@dataclass
class CreateTableNode(ASTNode):
    table_name: str
    columns: List[ColumnDefNode] = field(default_factory=list)
    if_not_exists: bool = True


@dataclass
class InsertNode(ASTNode):
    table_name: str
    values: Dict[str, Any] = field(default_factory=dict)


@dataclass
class BinaryOpNode(ASTNode):
    left: Any
    operator: str
    right: Any


@dataclass
class UnaryOpNode(ASTNode):
    operator: str
    operand: Any


@dataclass
class FunctionCallNode(ASTNode):
    name: str
    arguments: List[Any] = field(default_factory=list)


@dataclass
class IdentifierNode(ASTNode):
    name: str


@dataclass
class LiteralNode(ASTNode):
    value: Any
    literal_type: str  # "string", "number", "boolean", "null"


@dataclass
class OrderByNode(ASTNode):
    field: str
    direction: str = "ASC"  # "ASC" or "DESC"


@dataclass
class JoinNode(ASTNode):
    join_type: str  # "INNER", "LEFT", "RIGHT"
    table_name: str
    left_col: str
    right_col: str


@dataclass
class SelectNode(ASTNode):
    fields: List[Any]
    table_name: str
    joins: List[JoinNode] = field(default_factory=list)
    where: Optional[Any] = None
    order_by: Optional[OrderByNode] = None
    limit: Optional[int] = None
    offset: Optional[int] = None
    distinct: bool = False


@dataclass
class UpdateNode(ASTNode):
    table_name: str
    assignments: Dict[str, Any] = field(default_factory=dict)
    where: Optional[Any] = None


@dataclass
class DeleteNode(ASTNode):
    table_name: str
    is_all: bool = False
    where: Optional[Any] = None
    confirmation_token: Optional[str] = None


@dataclass
class DropTableNode(ASTNode):
    table_name: str
    confirmation_token: str
    if_exists: bool = True


@dataclass
class TruncateTableNode(ASTNode):
    table_name: str
    confirmation_token: str


@dataclass
class AlterTableNode(ASTNode):
    table_name: str
    action: str  # "ADD_COLUMN", "DROP_COLUMN"
    column_def: Optional[ColumnDefNode] = None
    drop_column: Optional[str] = None
    confirmation_token: Optional[str] = None


@dataclass
class ProgramNode(ASTNode):
    header: Optional[DomainHeaderNode]
    statements: List[ASTNode] = field(default_factory=list)
