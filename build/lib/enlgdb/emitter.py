"""Parameterized SQL emitter for enlgdb targeting SQLite, PostgreSQL, and ANSI SQL."""

from typing import List, Tuple, Any, Dict
from enlgdb.ast_nodes import (
    ProgramNode, CreateTableNode, ColumnDefNode, InsertNode,
    CreateDatabaseNode, UseDatabaseNode, ShowDatabasesNode, ShowTablesNode, DropDatabaseNode,
    SelectNode, UpdateNode, DeleteNode, DropTableNode,
    TruncateTableNode, AlterTableNode, BinaryOpNode,
    UnaryOpNode, FunctionCallNode, IdentifierNode, LiteralNode, ASTNode
)


class SQLEmitter:
    def __init__(self, dialect: str = "sqlite"):
        self.dialect = dialect.lower()

    def emit(self, node: ProgramNode) -> List[Tuple[str, List[Any]]]:
        """Translates an enlgdb Program AST into a list of parameterized (sql_string, params_list)."""
        results: List[Tuple[str, List[Any]]] = []
        for stmt in node.statements:
            res = self.emit_statement(stmt)
            if res:
                results.append(res)
        return results

    def emit_statement(self, stmt: ASTNode) -> Tuple[str, List[Any]]:
        if isinstance(stmt, CreateDatabaseNode):
            if self.dialect == "sqlite":
                return f'ATTACH DATABASE "{stmt.db_name}.db" AS "{stmt.db_name}";', []
            return f'CREATE DATABASE "{stmt.db_name}";', []
        elif isinstance(stmt, UseDatabaseNode):
            if self.dialect == "sqlite":
                return f'-- Switching active database to: {stmt.db_name}', []
            return f'USE "{stmt.db_name}";', []
        elif isinstance(stmt, ShowDatabasesNode):
            if self.dialect == "sqlite":
                return 'PRAGMA database_list;', []
            return 'SHOW DATABASES;', []
        elif isinstance(stmt, ShowTablesNode):
            if self.dialect == "sqlite":
                return "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';", []
            return f'SHOW TABLES FROM "{stmt.db_name}";' if stmt.db_name else 'SHOW TABLES;', []
        elif isinstance(stmt, DropDatabaseNode):
            if self.dialect == "sqlite":
                return f'DETACH DATABASE "{stmt.db_name}";', []
            return f'DROP DATABASE IF EXISTS "{stmt.db_name}";', []
        elif isinstance(stmt, CreateTableNode):
            return self.emit_create_table(stmt)
        elif isinstance(stmt, InsertNode):
            return self.emit_insert(stmt)
        elif isinstance(stmt, SelectNode):
            return self.emit_select(stmt)
        elif isinstance(stmt, UpdateNode):
            return self.emit_update(stmt)
        elif isinstance(stmt, DeleteNode):
            return self.emit_delete(stmt)
        elif isinstance(stmt, DropTableNode):
            return self.emit_drop_table(stmt)
        elif isinstance(stmt, TruncateTableNode):
            return self.emit_truncate_table(stmt)
        elif isinstance(stmt, AlterTableNode):
            return self.emit_alter_table(stmt)
        else:
            raise ValueError(f"Unknown statement node type: {type(stmt)}")

    def emit_create_table(self, node: CreateTableNode) -> Tuple[str, List[Any]]:
        col_defs: List[str] = []
        for col in node.columns:
            parts = [f'"{col.name}"']
            # Type mapping
            t = col.data_type
            if self.dialect == "sqlite" and t == "BOOLEAN":
                t = "INTEGER"
            parts.append(t)

            if col.is_primary_key:
                parts.append("PRIMARY KEY")
                if col.autoincrement:
                    if self.dialect == "sqlite":
                        parts.append("AUTOINCREMENT")
                    elif self.dialect == "postgres":
                        # In Postgres serial handles it, or GENERATED ALWAYS
                        pass
            if col.not_null:
                parts.append("NOT NULL")
            if col.unique:
                parts.append("UNIQUE")
            if col.default_value is not None:
                if isinstance(col.default_value, str) and not col.default_value.upper().startswith("CURRENT_"):
                    parts.append(f"DEFAULT '{col.default_value}'")
                else:
                    parts.append(f"DEFAULT {col.default_value}")
            if col.references_table:
                ref = f'REFERENCES "{col.references_table}"'
                if col.references_column:
                    ref += f'("{col.references_column}")'
                parts.append(ref)

            col_defs.append("    " + " ".join(parts))

        if_not_exists = "IF NOT EXISTS " if node.if_not_exists else ""
        sql = f'CREATE TABLE {if_not_exists}"{node.table_name}" (\n' + ",\n".join(col_defs) + "\n);"
        return sql, []

    def emit_insert(self, node: InsertNode) -> Tuple[str, List[Any]]:
        columns = list(node.values.keys())
        params: List[Any] = []
        placeholders: List[str] = []

        for col in columns:
            val_node = node.values[col]
            sql_expr, val_params = self.emit_expression(val_node)
            placeholders.append(sql_expr)
            params.extend(val_params)

        cols_str = ", ".join([f'"{c}"' for c in columns])
        vals_str = ", ".join(placeholders)
        sql = f'INSERT INTO "{node.table_name}" ({cols_str}) VALUES ({vals_str});'
        return sql, params

    def emit_select(self, node: SelectNode) -> Tuple[str, List[Any]]:
        params: List[Any] = []
        fields_str_list: List[str] = []

        for f in node.fields:
            if f == "*":
                fields_str_list.append("*")
            elif isinstance(f, FunctionCallNode):
                args_str = ", ".join([f'"{a}"' if a != "*" else "*" for a in f.arguments])
                fields_str_list.append(f"{f.name}({args_str})")
            else:
                fields_str_list.append(f'"{f}"' if "." not in f else f)

        distinct_kw = "DISTINCT " if node.distinct else ""
        sql_parts = [f'SELECT {distinct_kw}{", ".join(fields_str_list)} FROM "{node.table_name}"']

        # Joins
        for j in node.joins:
            sql_parts.append(f'{j.join_type} JOIN "{j.table_name}" ON {j.left_col} = {j.right_col}')

        # Where
        if node.where:
            where_sql, where_params = self.emit_expression(node.where)
            sql_parts.append(f"WHERE {where_sql}")
            params.extend(where_params)

        # Order by
        if node.order_by:
            sql_parts.append(f'ORDER BY "{node.order_by.field}" {node.order_by.direction}')

        # Limit & Offset
        if node.limit is not None:
            sql_parts.append(f"LIMIT {node.limit}")
        if node.offset is not None:
            sql_parts.append(f"OFFSET {node.offset}")

        return " ".join(sql_parts) + ";", params

    def emit_update(self, node: UpdateNode) -> Tuple[str, List[Any]]:
        params: List[Any] = []
        set_parts: List[str] = []

        for col, val_expr in node.assignments.items():
            expr_sql, expr_params = self.emit_expression(val_expr)
            set_parts.append(f'"{col}" = {expr_sql}')
            params.extend(expr_params)

        sql_parts = [f'UPDATE "{node.table_name}" SET {", ".join(set_parts)}']
        if node.where:
            where_sql, where_params = self.emit_expression(node.where)
            sql_parts.append(f"WHERE {where_sql}")
            params.extend(where_params)

        return " ".join(sql_parts) + ";", params

    def emit_delete(self, node: DeleteNode) -> Tuple[str, List[Any]]:
        params: List[Any] = []
        sql_parts = [f'DELETE FROM "{node.table_name}"']
        if node.where:
            where_sql, where_params = self.emit_expression(node.where)
            sql_parts.append(f"WHERE {where_sql}")
            params.extend(where_params)

        return " ".join(sql_parts) + ";", params

    def emit_drop_table(self, node: DropTableNode) -> Tuple[str, List[Any]]:
        if_exists = "IF EXISTS " if node.if_exists else ""
        sql = f'DROP TABLE {if_exists}"{node.table_name}";'
        return sql, []

    def emit_truncate_table(self, node: TruncateTableNode) -> Tuple[str, List[Any]]:
        if self.dialect == "sqlite":
            sql = f'DELETE FROM "{node.table_name}";'
        else:
            sql = f'TRUNCATE TABLE "{node.table_name}";'
        return sql, []

    def emit_alter_table(self, node: AlterTableNode) -> Tuple[str, List[Any]]:
        if node.action == "ADD_COLUMN" and node.column_def:
            col = node.column_def
            parts = [f'"{col.name}"', col.data_type]
            if col.not_null:
                parts.append("NOT NULL")
            if col.default_value is not None:
                parts.append(f"DEFAULT '{col.default_value}'")
            return f'ALTER TABLE "{node.table_name}" ADD COLUMN {" ".join(parts)};', []
        elif node.action == "DROP_COLUMN" and node.drop_column:
            return f'ALTER TABLE "{node.table_name}" DROP COLUMN "{node.drop_column}";', []
        raise ValueError("Invalid AlterTable action")

    def emit_expression(self, node: Any) -> Tuple[str, List[Any]]:
        if isinstance(node, LiteralNode):
            if self.dialect == "postgres":
                return "$?", [node.value]
            else:
                return "?", [node.value]
        elif isinstance(node, IdentifierNode):
            return f'"{node.name}"' if "." not in node.name else node.name, []
        elif isinstance(node, BinaryOpNode):
            l_sql, l_params = self.emit_expression(node.left)
            r_sql, r_params = self.emit_expression(node.right)
            op = node.operator
            if op == "IS":
                op = "="
            return f"({l_sql} {op} {r_sql})", l_params + r_params
        elif isinstance(node, UnaryOpNode):
            expr_sql, params = self.emit_expression(node.operand)
            return f"{node.operator} ({expr_sql})", params
        elif isinstance(node, FunctionCallNode):
            args_sql = []
            params = []
            for arg in node.arguments:
                if arg == "*":
                    args_sql.append("*")
                else:
                    a_sql, a_params = self.emit_expression(arg)
                    args_sql.append(a_sql)
                    params.extend(a_params)
            return f"{node.name}({', '.join(args_sql)})", params
        elif isinstance(node, (int, float, str, bool)):
            return "?", [node]
        else:
            return str(node), []
