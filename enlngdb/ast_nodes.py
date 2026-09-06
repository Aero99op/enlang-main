"""AST Node definitions for enlngdb (Sovereign Native Storage Engine - ZERO SQL)."""

from dataclasses import dataclass, field
from typing import List, Optional, Any, Dict


class ASTNode:
    """Base class for all enlngdb AST nodes."""
    pass


@dataclass
class DomainHeaderNode(ASTNode):
    domain: str = "enlngdb"


@dataclass
class DisplayNode(ASTNode):
    message: Any


@dataclass
class UseDatabaseNode(ASTNode):
    db_path: str


@dataclass
class OpenDatabaseNode(ASTNode):
    db_path: str


@dataclass
class SaveDatabaseNode(ASTNode):
    db_path: str


@dataclass
class ShowDatabasesNode(ASTNode):
    pass


@dataclass
class ShowTablesNode(ASTNode):
    database_name: Optional[str] = None


@dataclass
class HintNode(ASTNode):
    hints: Dict[str, Any] = field(default_factory=dict)


@dataclass
class ColumnDefNode(ASTNode):
    name: str
    data_type: str = "TEXT"
    is_primary_key: bool = False
    autoincrement: bool = False
    not_null: bool = False
    unique: bool = False
    default_value: Optional[Any] = None
    hints: Dict[str, Any] = field(default_factory=dict)


@dataclass
class CreateTableNode(ASTNode):
    table_name: str
    columns: List[ColumnDefNode] = field(default_factory=list)
    if_not_exists: bool = True
    hints: Dict[str, Any] = field(default_factory=dict)


@dataclass
class InsertRecordNode(ASTNode):
    table_name: str
    values: Dict[str, Any] = field(default_factory=dict)
    hints: Dict[str, Any] = field(default_factory=dict)


@dataclass
class OrderByNode(ASTNode):
    field: str
    direction: str = "ASC"  # "ASC" or "DESC"


@dataclass
class FindRecordsNode(ASTNode):
    table_name: str
    fields: List[str] = field(default_factory=lambda: ["*"])
    where: Optional[Any] = None
    order_by: Optional[OrderByNode] = None
    limit: Optional[int] = None
    offset: Optional[int] = None
    hints: Dict[str, Any] = field(default_factory=dict)


@dataclass
class UpdateRecordsNode(ASTNode):
    table_name: str
    assignments: Dict[str, Any] = field(default_factory=dict)
    where: Optional[Any] = None
    hints: Dict[str, Any] = field(default_factory=dict)


@dataclass
class DeleteRecordsNode(ASTNode):
    table_name: str
    where: Optional[Any] = None
    is_all: bool = False
    confirmation_token: Optional[str] = None
    hints: Dict[str, Any] = field(default_factory=dict)


@dataclass
class CountRecordsNode(ASTNode):
    table_name: str
    where: Optional[Any] = None
    hints: Dict[str, Any] = field(default_factory=dict)


@dataclass
class BinaryOpNode(ASTNode):
    left: Any
    operator: str  # "=", "!=", ">", ">=", "<", "<=", "LIKE", "IN", "AND", "OR"
    right: Any


@dataclass
class UnaryOpNode(ASTNode):
    operator: str
    operand: Any


@dataclass
class IdentifierNode(ASTNode):
    name: str


@dataclass
class LiteralNode(ASTNode):
    value: Any
    literal_type: str  # "string", "number", "boolean", "null"


@dataclass
class ProgramNode(ASTNode):
    header: Optional[DomainHeaderNode]
    statements: List[ASTNode] = field(default_factory=list)
