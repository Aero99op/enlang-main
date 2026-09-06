"""Sovereign Native Storage Engine for enlngdb (ZERO SQL, ZERO SQLITE, ZERO EXTERNAL DEPENDENCIES)."""

import json
import os
import re
from pathlib import Path
from contextlib import contextmanager
from typing import Dict, List, Any, Optional, Callable
from enlngdb.ast_nodes import (
    ColumnDefNode, BinaryOpNode, UnaryOpNode, IdentifierNode, LiteralNode, OrderByNode
)
from enlngdb.locking import DatabaseLock


class StorageError(Exception):
    pass


class Table:
    def __init__(self, name: str, columns: Optional[List[ColumnDefNode]] = None, hints: Optional[Dict[str, Any]] = None):
        self.name = name
        self.columns = columns or []
        self.hints = hints or {}
        self.rows: List[Dict[str, Any]] = []
        self.primary_key: Optional[str] = None
        self.indexes: Dict[str, Dict[Any, List[int]]] = {}
        self.auto_increment_counters: Dict[str, int] = {}
        self.version: int = 1

        for col in self.columns:
            if col.is_primary_key:
                self.primary_key = col.name
                self.create_index(col.name)
            if col.unique:
                self.create_index(col.name)
            if col.autoincrement:
                self.auto_increment_counters[col.name] = 1

    def create_index(self, column_name: str):
        if column_name not in self.indexes:
            self.indexes[column_name] = {}
            for idx, row in enumerate(self.rows):
                if column_name in row:
                    val = row[column_name]
                    self.indexes[column_name].setdefault(val, []).append(idx)

    def insert(self, record: Dict[str, Any]) -> Dict[str, Any]:
        new_row = dict(record)
        new_row.setdefault("_version", 1)
        self._apply_defaults_and_autoincrement(new_row)
        self._validate_constraints(new_row)
        row_idx = len(self.rows)
        self.rows.append(new_row)
        self.version += 1
        self._update_indexes(new_row, row_idx)
        return new_row

    def _apply_defaults_and_autoincrement(self, new_row: Dict[str, Any]):
        for col in self.columns:
            if col.name not in new_row or new_row[col.name] is None:
                if col.autoincrement and col.name in self.auto_increment_counters:
                    new_row[col.name] = self.auto_increment_counters[col.name]
                    self.auto_increment_counters[col.name] += 1
                elif col.default_value is not None:
                    new_row[col.name] = col.default_value

    def _validate_constraints(self, new_row: Dict[str, Any]):
        for col in self.columns:
            if (col.is_primary_key or col.unique) and col.name in new_row:
                val = new_row[col.name]
                if col.name in self.indexes and val in self.indexes[col.name]:
                    raise StorageError(f"Duplicate key error: {col.name}='{val}' already exists in table '{self.name}'.")

    def _update_indexes(self, new_row: Dict[str, Any], row_idx: int):
        for col_name, idx_map in self.indexes.items():
            if col_name in new_row:
                idx_map.setdefault(new_row[col_name], []).append(row_idx)

    def find(self,
             filter_fn: Optional[Callable[[Dict[str, Any]], bool]] = None,
             order_by: Optional[OrderByNode] = None,
             limit: Optional[int] = None,
             offset: Optional[int] = None,
             fields: Optional[List[str]] = None) -> List[Dict[str, Any]]:
        matched_rows = [r for r in self.rows if filter_fn is None or filter_fn(r)]
        if order_by and order_by.field:
            self._sort_rows(matched_rows, order_by)
        if offset is not None and offset > 0:
            matched_rows = matched_rows[offset:]
        if limit is not None and limit >= 0:
            matched_rows = matched_rows[:limit]
        return self._project_fields(matched_rows, fields)

    @staticmethod
    def _sort_rows(rows: List[Dict[str, Any]], order_by: OrderByNode):
        reverse = (order_by.direction.upper() == "DESC")
        def sort_key(r):
            val = r.get(order_by.field)
            if isinstance(val, LiteralNode):
                val = val.value
            if val is None:
                return (0, "")
            if isinstance(val, (int, float)):
                return (1, val)
            return (2, str(val))
        rows.sort(key=sort_key, reverse=reverse)

    @staticmethod
    def _project_fields(rows: List[Dict[str, Any]], fields: Optional[List[str]]) -> List[Dict[str, Any]]:
        if fields and fields != ["*"]:
            return [{f: row.get(f) for f in fields} for row in rows]
        return [dict(r) for r in rows]

    def update(self, filter_fn: Optional[Callable[[Dict[str, Any]], bool]], assignments: Dict[str, Any]) -> int:
        updated_count = 0
        for row in self.rows:
            if filter_fn is None or filter_fn(row):
                for k, v in assignments.items():
                    row[k] = v
                row["_version"] = row.get("_version", 1) + 1
                updated_count += 1

        if updated_count > 0:
            self.version += 1
            self._rebuild_indexes()
        return updated_count

    def delete(self, filter_fn: Optional[Callable[[Dict[str, Any]], bool]]) -> int:
        if filter_fn is None:
            count = len(self.rows)
            self.rows.clear()
            self.version += 1
            self._rebuild_indexes()
            return count

        initial_count = len(self.rows)
        self.rows = [r for r in self.rows if not filter_fn(r)]
        deleted_count = initial_count - len(self.rows)
        if deleted_count > 0:
            self.version += 1
            self._rebuild_indexes()
        return deleted_count

    def delete_column(self, column_name: str) -> bool:
        existed = any(c.name == column_name for c in self.columns) or any(column_name in r for r in self.rows)
        self.columns = [c for c in self.columns if c.name != column_name]
        for r in self.rows:
            r.pop(column_name, None)
        self.indexes.pop(column_name, None)
        if self.primary_key == column_name:
            self.primary_key = None
        self.auto_increment_counters.pop(column_name, None)
        if existed:
            self.version += 1
        return existed

    def count(self, filter_fn: Optional[Callable[[Dict[str, Any]], bool]] = None) -> int:
        if filter_fn is None:
            return len(self.rows)
        return sum(1 for r in self.rows if filter_fn(r))

    def _rebuild_indexes(self):
        for col_name in self.indexes:
            self.indexes[col_name] = {}
            for idx, row in enumerate(self.rows):
                if col_name in row:
                    val = row[col_name]
                    self.indexes[col_name].setdefault(val, []).append(idx)

    def to_dict(self) -> Dict[str, Any]:
        return {
            "name": self.name,
            "hints": self.hints,
            "version": getattr(self, "version", 1),
            "columns": [
                {
                    "name": col.name,
                    "data_type": col.data_type,
                    "is_primary_key": col.is_primary_key,
                    "autoincrement": col.autoincrement,
                    "unique": col.unique,
                    "default_value": col.default_value,
                    "hints": col.hints
                }
                for col in self.columns
            ],
            "rows": self.rows
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'Table':
        cols = []
        for c in data.get("columns", []):
            cols.append(ColumnDefNode(
                name=c["name"],
                data_type=c.get("data_type", "TEXT"),
                is_primary_key=c.get("is_primary_key", False),
                autoincrement=c.get("autoincrement", False),
                unique=c.get("unique", False),
                default_value=c.get("default_value"),
                hints=c.get("hints", {})
            ))
        table = cls(name=data["name"], columns=cols, hints=data.get("hints", {}))
        table.version = data.get("version", 1)
        table.rows = data.get("rows", [])
        table._rebuild_indexes()
        return table


class ExpressionEvaluator:
    """Evaluates AST conditions and expressions directly against in-memory record dictionaries."""

    @staticmethod
    def _unwrap(val: Any) -> Any:
        return val.value if isinstance(val, LiteralNode) else val

    @classmethod
    def evaluate(cls, expr: Any, row: Dict[str, Any]) -> Any:
        if expr is None:
            return True
        if isinstance(expr, LiteralNode):
            return expr.value
        if isinstance(expr, IdentifierNode):
            return cls._unwrap(row.get(expr.name))
        if isinstance(expr, BinaryOpNode):
            left = cls._unwrap(cls.evaluate(expr.left, row))
            right = cls._unwrap(cls.evaluate(expr.right, row))
            return cls._evaluate_binary(left, expr.operator.upper(), right)
        if isinstance(expr, UnaryOpNode):
            val = cls.evaluate(expr.operand, row)
            return not bool(val) if expr.operator.upper() == "NOT" else (-val if expr.operator == "-" else val)
        return expr

    @classmethod
    def _evaluate_binary(cls, left: Any, op: str, right: Any) -> Any:
        if op == "AND":
            return bool(left) and bool(right)
        if op == "OR":
            return bool(left) or bool(right)
        if op in ("=", "=="):
            return left == right
        if op == "!=":
            return left != right
        if op in (">", ">=", "<", "<="):
            return cls._evaluate_relational(left, op, right)
        if op == "LIKE":
            return cls._evaluate_like(left, right)
        if op in ("+", "-"):
            return cls._evaluate_arithmetic(left, op, right)
        raise StorageError(f"Unsupported storage operator '{op}'")

    @staticmethod
    def _evaluate_relational(left: Any, op: str, right: Any) -> bool:
        if left is None or right is None:
            return False
        l_num, r_num = float(left), float(right)
        if op == ">":
            return l_num > r_num
        if op == ">=":
            return l_num >= r_num
        if op == "<":
            return l_num < r_num
        return l_num <= r_num

    @staticmethod
    def _evaluate_like(left: Any, right: Any) -> bool:
        if left is None or right is None:
            return False
        parts = []
        for seg in str(right).split('%'):
            subparts = [re.escape(s) for s in seg.split('_')]
            parts.append('.'.join(subparts))
        pattern = "^" + ".*".join(parts) + "$"
        return bool(re.match(pattern, str(left), re.IGNORECASE))

    @staticmethod
    def _evaluate_arithmetic(left: Any, op: str, right: Any) -> Any:
        if op == "+":
            if isinstance(left, str) or isinstance(right, str):
                return str(left) + str(right)
            return (left or 0) + (right or 0)
        return (left or 0) - (right or 0)


class NativeStorageEngine:
    """Zero-SQL Sovereign Native Storage Engine for Enlangg with Thread & Process Concurrency Locking."""

    def __init__(self, db_path: Optional[str] = None):
        self.tables: Dict[str, Table] = {}
        self.db_path = db_path
        self.lock = DatabaseLock(db_path=db_path)
        if db_path and os.path.exists(db_path):
            self.load_from_disk(db_path)

    @contextmanager
    def transaction(self, timeout: Optional[float] = 10.0):
        """Atomic transaction context manager holding an exclusive cross-thread/process write lock."""
        with self.lock.write(timeout=timeout):
            yield self

    def create_table(self, table_name: str, columns: Optional[List[ColumnDefNode]] = None, hints: Optional[Dict[str, Any]] = None, if_not_exists: bool = False) -> Table:
        with self.lock.write():
            if table_name in self.tables and if_not_exists:
                return self.tables[table_name]
            table = Table(name=table_name, columns=columns, hints=hints)
            self.tables[table_name] = table
            return table

    def insert(self, table_name: str, values: Dict[str, Any], hints: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        with self.lock.write():
            if table_name not in self.tables:
                # Auto-provision table if not explicitly created
                self.tables[table_name] = Table(name=table_name, hints=hints)
            elif hints:
                self.tables[table_name].hints.update(hints)

            evaluated_values = {}
            for k, v in values.items():
                if isinstance(v, LiteralNode):
                    evaluated_values[k] = v.value
                elif isinstance(v, (IdentifierNode, BinaryOpNode, UnaryOpNode)):
                    evaluated_values[k] = ExpressionEvaluator.evaluate(v, {})
                else:
                    evaluated_values[k] = v

            return self.tables[table_name].insert(evaluated_values)

    def find(self,
             table_name: str,
             fields: Optional[List[str]] = None,
             where_ast: Optional[Any] = None,
             order_by: Optional[OrderByNode] = None,
             limit: Optional[int] = None,
             offset: Optional[int] = None,
             hints: Optional[Dict[str, Any]] = None) -> List[Dict[str, Any]]:
        with self.lock.read():
            if table_name not in self.tables:
                raise StorageError(f"Table '{table_name}' does not exist.")
            if hints:
                self.tables[table_name].hints.update(hints)

            filter_fn = None
            if where_ast is not None:
                filter_fn = lambda r: bool(ExpressionEvaluator.evaluate(where_ast, r))

            return self.tables[table_name].find(
                filter_fn=filter_fn,
                order_by=order_by,
                limit=limit,
                offset=offset,
                fields=fields
            )

    def update(self,
               table_name: str,
               assignments: Dict[str, Any],
               where_ast: Optional[Any] = None,
               hints: Optional[Dict[str, Any]] = None) -> int:
        with self.lock.write():
            if table_name not in self.tables:
                raise StorageError(f"Table '{table_name}' does not exist.")
            if hints:
                self.tables[table_name].hints.update(hints)

            filter_fn = None
            if where_ast is not None:
                filter_fn = lambda r: bool(ExpressionEvaluator.evaluate(where_ast, r))

            # Evaluate expressions in assignments if needed
            evaluated_assignments = {}
            for k, v in assignments.items():
                if isinstance(v, LiteralNode):
                    evaluated_assignments[k] = v.value
                elif isinstance(v, (IdentifierNode, BinaryOpNode, UnaryOpNode)):
                    evaluated_assignments[k] = ExpressionEvaluator.evaluate(v, {})
                else:
                    evaluated_assignments[k] = v

            return self.tables[table_name].update(filter_fn=filter_fn, assignments=evaluated_assignments)

    def delete(self,
               table_name: str,
               where_ast: Optional[Any] = None,
               hints: Optional[Dict[str, Any]] = None) -> int:
        with self.lock.write():
            if table_name not in self.tables:
                raise StorageError(f"Table '{table_name}' does not exist.")
            if hints:
                self.tables[table_name].hints.update(hints)

            filter_fn = None
            if where_ast is not None:
                filter_fn = lambda r: bool(ExpressionEvaluator.evaluate(where_ast, r))

            return self.tables[table_name].delete(filter_fn=filter_fn)

    def count(self,
              table_name: str,
              where_ast: Optional[Any] = None,
              hints: Optional[Dict[str, Any]] = None) -> int:
        with self.lock.read():
            if table_name not in self.tables:
                raise StorageError(f"Table '{table_name}' does not exist.")
            if hints:
                self.tables[table_name].hints.update(hints)

            filter_fn = None
            if where_ast is not None:
                filter_fn = lambda r: bool(ExpressionEvaluator.evaluate(where_ast, r))

            return self.tables[table_name].count(filter_fn=filter_fn)

    def delete_column(self, table_name: str, column_name: str) -> bool:
        with self.lock.write():
            if table_name not in self.tables:
                raise StorageError(f"Table '{table_name}' does not exist.")
            return self.tables[table_name].delete_column(column_name)

    def drop_table(self, table_name: str) -> bool:
        with self.lock.write():
            if table_name not in self.tables:
                raise StorageError(f"Table '{table_name}' does not exist.")
            del self.tables[table_name]
            return True

    def drop_database(self, database_name: str, resolved_path: Optional[str] = None) -> bool:
        with self.lock.write():
            target_path = resolved_path or database_name
            removed = False
            if os.path.exists(target_path):
                os.remove(target_path)
                removed = True
            if self.db_path and (self.db_path == target_path or Path(self.db_path).stem == database_name):
                self.tables.clear()
                self.db_path = None
            return removed

    def save_to_disk(self, file_path: str):
        with self.lock.write():
            payload = {
                "format": "ENLNGDB_SOVEREIGN_V1",
                "tables": {name: t.to_dict() for name, t in self.tables.items()}
            }
            temp_path = f"{file_path}.tmp"
            with open(temp_path, "w", encoding="utf-8") as f:
                json.dump(payload, f, indent=2, default=str)
            if os.path.exists(file_path):
                os.remove(file_path)
            os.rename(temp_path, file_path)

    def load_from_disk(self, file_path: str):
        with self.lock.write():
            if not os.path.exists(file_path):
                raise StorageError(f"Database file '{file_path}' not found.")
            if os.path.getsize(file_path) == 0:
                self.tables.clear()
                return
            try:
                with open(file_path, "r", encoding="utf-8") as f:
                    payload = json.load(f)
            except (UnicodeDecodeError, json.JSONDecodeError):
                try:
                    with open(file_path, "rb") as bf:
                        header = bf.read(16)
                        if b"SQLite" in header:
                            raise StorageError(f"File '{file_path}' is a legacy SQLite binary database. EnlngDB is 100% sovereign native and uses .edb format.")
                except StorageError:
                    raise
                except Exception:
                    pass
                raise StorageError(f"File '{file_path}' is not a valid sovereign EnlngDB database file.")

            tables_data = payload.get("tables", {})
            self.tables.clear()
            for name, t_dict in tables_data.items():
                self.tables[name] = Table.from_dict(t_dict)
