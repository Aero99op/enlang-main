"""Execution Engine for enlngdb (Sovereign Native Storage Engine - ZERO SQL).

Directly executes enlngdb AST statements against NativeStorageEngine without any SQL translation,
external database drivers, or intermediate representations.
"""

import os
from typing import List, Dict, Any, Optional
from enlngdb.ast_nodes import (
    ProgramNode, DomainHeaderNode, DisplayNode, OpenDatabaseNode, SaveDatabaseNode,
    HintNode, CreateTableNode, InsertRecordNode, FindRecordsNode, UpdateRecordsNode,
    DeleteRecordsNode, CountRecordsNode, ASTNode, LiteralNode
)
from enlngdb.storage import NativeStorageEngine, StorageError


class NativeExecutionEngine:
    """Zero-SQL Sovereign Execution Engine for EnlngDB."""

    def __init__(self, db_path: Optional[str] = None, storage: Optional[NativeStorageEngine] = None, stream_output: bool = True):
        self.db_path = db_path
        self.storage = storage or NativeStorageEngine(db_path=db_path)
        self.stream_output = stream_output
        self.active_hints: Dict[str, Any] = {}

    def execute_program(self, program: ProgramNode) -> List[Dict[str, Any]]:
        """Executes all statements in an enlngdb AST program sequentially."""
        reports: List[Dict[str, Any]] = []
        for stmt in program.statements:
            report = self.execute_statement(stmt)
            reports.append(report)
        return reports

    def execute_statement(self, stmt: ASTNode) -> Dict[str, Any]:
        """Executes a single AST statement directly on the native in-memory/persisted storage engine."""
        try:
            if isinstance(stmt, DomainHeaderNode):
                return {
                    "type": "DOMAIN_HEADER",
                    "domain": stmt.domain,
                    "success": True,
                    "message": f"Domain context initialized: {stmt.domain}"
                }

            elif isinstance(stmt, DisplayNode):
                raw_msg = stmt.message.value if isinstance(stmt.message, LiteralNode) else stmt.message
                msg = str(raw_msg)
                if self.stream_output:
                    print(msg)
                return {
                    "type": "DISPLAY",
                    "message": msg,
                    "success": True
                }

            elif isinstance(stmt, OpenDatabaseNode):
                self.storage.load_from_disk(stmt.db_path)
                self.db_path = stmt.db_path
                msg = f"Opened sovereign database file '{stmt.db_path}'."
                if self.stream_output:
                    print(f"[enlngdb] {msg}")
                return {
                    "type": "OPEN_DATABASE",
                    "path": stmt.db_path,
                    "success": True,
                    "message": msg
                }

            elif isinstance(stmt, SaveDatabaseNode):
                self.storage.save_to_disk(stmt.db_path)
                msg = f"Persisted sovereign database to '{stmt.db_path}'."
                if self.stream_output:
                    print(f"[enlngdb] {msg}")
                return {
                    "type": "SAVE_DATABASE",
                    "path": stmt.db_path,
                    "success": True,
                    "message": msg
                }

            elif isinstance(stmt, HintNode):
                self.active_hints.update(stmt.hints)
                msg = f"Engine hints updated: {stmt.hints}"
                return {
                    "type": "HINT",
                    "hints": stmt.hints,
                    "success": True,
                    "message": msg
                }

            elif isinstance(stmt, CreateTableNode):
                self.storage.create_table(
                    table_name=stmt.table_name,
                    columns=stmt.columns,
                    hints=stmt.hints,
                    if_not_exists=stmt.if_not_exists
                )
                col_names = [c.name for c in stmt.columns]
                msg = f"Table '{stmt.table_name}' initialized with columns: {col_names}"
                return {
                    "type": "CREATE_TABLE",
                    "table": stmt.table_name,
                    "columns": col_names,
                    "if_not_exists": stmt.if_not_exists,
                    "success": True,
                    "message": msg
                }

            elif isinstance(stmt, InsertRecordNode):
                merged_hints = {**self.active_hints, **stmt.hints}
                inserted = self.storage.insert(
                    table_name=stmt.table_name,
                    values=stmt.values,
                    hints=merged_hints
                )
                msg = f"Inserted record into '{stmt.table_name}': {inserted}"
                return {
                    "type": "INSERT",
                    "table": stmt.table_name,
                    "record": inserted,
                    "success": True,
                    "message": msg
                }

            elif isinstance(stmt, FindRecordsNode):
                merged_hints = {**self.active_hints, **stmt.hints}
                rows = self.storage.find(
                    table_name=stmt.table_name,
                    fields=stmt.fields,
                    where_ast=stmt.where,
                    order_by=stmt.order_by,
                    limit=stmt.limit,
                    offset=stmt.offset,
                    hints=merged_hints
                )

                if self.stream_output:
                    table_output = self.format_table(rows, stmt.fields)
                    print(table_output)

                return {
                    "type": "FIND",
                    "table": stmt.table_name,
                    "fields": stmt.fields,
                    "rows": rows,
                    "count": len(rows),
                    "success": True,
                    "message": f"Found {len(rows)} matching record(s) in '{stmt.table_name}'."
                }

            elif isinstance(stmt, UpdateRecordsNode):
                merged_hints = {**self.active_hints, **stmt.hints}
                updated_count = self.storage.update(
                    table_name=stmt.table_name,
                    assignments=stmt.assignments,
                    where_ast=stmt.where,
                    hints=merged_hints
                )
                msg = f"Updated {updated_count} record(s) in table '{stmt.table_name}'."
                return {
                    "type": "UPDATE",
                    "table": stmt.table_name,
                    "assignments": stmt.assignments,
                    "rows_affected": updated_count,
                    "success": True,
                    "message": msg
                }

            elif isinstance(stmt, DeleteRecordsNode):
                merged_hints = {**self.active_hints, **stmt.hints}
                deleted_count = self.storage.delete(
                    table_name=stmt.table_name,
                    where_ast=stmt.where,
                    hints=merged_hints
                )
                msg = f"Deleted {deleted_count} record(s) from table '{stmt.table_name}'."
                return {
                    "type": "DELETE",
                    "table": stmt.table_name,
                    "rows_affected": deleted_count,
                    "success": True,
                    "message": msg
                }

            elif isinstance(stmt, CountRecordsNode):
                merged_hints = {**self.active_hints, **stmt.hints}
                total = self.storage.count(
                    table_name=stmt.table_name,
                    where_ast=stmt.where,
                    hints=merged_hints
                )
                msg = f"Count in '{stmt.table_name}': {total}"
                if self.stream_output:
                    print(f"[enlngdb] {msg}")
                return {
                    "type": "COUNT",
                    "table": stmt.table_name,
                    "count": total,
                    "success": True,
                    "message": msg
                }

            else:
                return {
                    "type": "UNKNOWN_STATEMENT",
                    "statement": str(stmt),
                    "success": False,
                    "error": f"Unsupported statement type {type(stmt).__name__}"
                }

        except Exception as e:
            err_msg = f"Storage execution error: {str(e)}"
            if self.stream_output:
                print(f"[enlngdb ERROR] {err_msg}")
            return {
                "type": "ERROR",
                "statement": type(stmt).__name__,
                "error": str(e),
                "success": False
            }

    def format_table(self, rows: List[Dict[str, Any]], fields: Optional[List[str]] = None) -> str:
        """Renders tabular data in clean, aligned box-drawing format."""
        if not rows:
            return "  (No records found matching criteria)\n"

        # Determine column list
        if fields and fields != ["*"]:
            cols = fields
        else:
            cols = list(rows[0].keys())

        # Calculate max width per column
        col_widths: Dict[str, int] = {c: len(str(c)) for c in cols}
        for r in rows:
            for c in cols:
                val_str = str(r.get(c, ""))
                col_widths[c] = max(col_widths[c], len(val_str))

        # Build table border lines
        border = "+" + "+".join(["-" * (col_widths[c] + 2) for c in cols]) + "+"
        header = "| " + " | ".join([str(c).ljust(col_widths[c]) for c in cols]) + " |"

        lines = [border, header, border]
        for r in rows:
            row_line = "| " + " | ".join([str(r.get(c, "")).ljust(col_widths[c]) for c in cols]) + " |"
            lines.append(row_line)
        lines.append(border)
        lines.append(f"  ({len(rows)} record{'s' if len(rows) != 1 else ''} returned)\n")

        return "\n".join(lines)

    def print_reports(self, reports: List[Dict[str, Any]]):
        """Displays execution summary report."""
        success_count = sum(1 for r in reports if r.get("success", False))
        fail_count = len(reports) - success_count
        print("\n" + "=" * 64)
        print("  ENLNGDB SOVEREIGN NATIVE EXECUTION SUMMARY")
        print(f"  Total Statements: {len(reports)} | Passed: {success_count} | Failed: {fail_count}")
        print("=" * 64 + "\n")


# Alias for backward compatibility with enlgdb
DatabaseEngine = NativeExecutionEngine
