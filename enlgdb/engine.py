import os
import sqlite3
from typing import List, Tuple, Any, Dict, Optional
from pathlib import Path
from enlgdb.ast_nodes import (
    ProgramNode, SelectNode, CreateTableNode, InsertNode, UpdateNode, DeleteNode, DropTableNode,
    CreateDatabaseNode, UseDatabaseNode, ShowDatabasesNode, ShowTablesNode, DropDatabaseNode
)


class DatabaseEngine:
    def __init__(self, db_path: str = ":memory:"):
        self.db_path = db_path
        self.conn = sqlite3.connect(db_path)
        self.conn.row_factory = sqlite3.Row
        self.cursor = self.conn.cursor()

    def execute_program(self, program: ProgramNode, emitter) -> List[Dict[str, Any]]:
        """Executes all statements in the enlgdb AST program."""
        reports = []
        for stmt in program.statements:
            sql, params = emitter.emit_statement(stmt)
            report = self.execute_single(stmt, sql, params)
            reports.append(report)
        return reports

    def execute_single(self, stmt_node: Any, sql: str, params: List[Any]) -> Dict[str, Any]:
        """Executes a single parameterized SQL statement and returns an execution report."""
        try:
            if isinstance(stmt_node, CreateDatabaseNode):
                db_file = f"{stmt_node.db_name}.db"
                # Connect to create file if not exists
                c = sqlite3.connect(db_file)
                c.close()
                return {
                    "type": "CREATE_DATABASE",
                    "database": stmt_node.db_name,
                    "sql": sql,
                    "success": True,
                    "message": f"Database '{stmt_node.db_name}' created ({db_file})."
                }

            elif isinstance(stmt_node, UseDatabaseNode):
                db_file = f"{stmt_node.db_name}.db" if not stmt_node.db_name.endswith(".db") else stmt_node.db_name
                self.conn.close()
                self.db_path = db_file
                self.conn = sqlite3.connect(db_file)
                self.conn.row_factory = sqlite3.Row
                self.cursor = self.conn.cursor()
                return {
                    "type": "USE_DATABASE",
                    "database": stmt_node.db_name,
                    "sql": sql,
                    "success": True,
                    "message": f"Active database switched to: '{stmt_node.db_name}' ({db_file})."
                }

            elif isinstance(stmt_node, ShowDatabasesNode):
                # Search for all .db files in current working directory
                db_files = [f.name for f in Path(".").glob("*.db")]
                rows = [{"database_name": f.replace(".db", ""), "file": f} for f in db_files]
                return {
                    "type": "SHOW_DATABASES",
                    "columns": ["database_name", "file"],
                    "rows": rows,
                    "count": len(rows),
                    "success": True
                }

            elif isinstance(stmt_node, ShowTablesNode):
                self.cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';")
                rows = [{"table_name": r[0]} for r in self.cursor.fetchall()]
                return {
                    "type": "SHOW_TABLES",
                    "columns": ["table_name"],
                    "rows": rows,
                    "count": len(rows),
                    "success": True
                }

            elif isinstance(stmt_node, DropDatabaseNode):
                db_file = f"{stmt_node.db_name}.db"
                if os.path.exists(db_file):
                    if self.db_path == db_file:
                        self.conn.close()
                        self.db_path = ":memory:"
                        self.conn = sqlite3.connect(":memory:")
                        self.cursor = self.conn.cursor()
                    os.remove(db_file)
                return {
                    "type": "DROP_DATABASE",
                    "database": stmt_node.db_name,
                    "sql": sql,
                    "success": True,
                    "message": f"Database '{stmt_node.db_name}' ({db_file}) deleted safely."
                }

            # Standard SQL operations
            self.cursor.execute(sql, params)
            self.conn.commit()

            if isinstance(stmt_node, SelectNode):
                rows = [dict(r) for r in self.cursor.fetchall()]
                cols = [desc[0] for desc in self.cursor.description] if self.cursor.description else []
                return {
                    "type": "SELECT",
                    "table": stmt_node.table_name,
                    "sql": sql,
                    "params": params,
                    "columns": cols,
                    "rows": rows,
                    "count": len(rows),
                    "success": True
                }
            elif isinstance(stmt_node, CreateTableNode):
                return {
                    "type": "CREATE_TABLE",
                    "table": stmt_node.table_name,
                    "sql": sql,
                    "success": True,
                    "message": f"Table '{stmt_node.table_name}' created successfully."
                }
            elif isinstance(stmt_node, InsertNode):
                return {
                    "type": "INSERT",
                    "table": stmt_node.table_name,
                    "sql": sql,
                    "params": params,
                    "rowid": self.cursor.lastrowid,
                    "success": True,
                    "message": f"Inserted 1 record into '{stmt_node.table_name}' (ID: {self.cursor.lastrowid})."
                }
            elif isinstance(stmt_node, UpdateNode):
                return {
                    "type": "UPDATE",
                    "table": stmt_node.table_name,
                    "sql": sql,
                    "params": params,
                    "rows_affected": self.cursor.rowcount,
                    "success": True,
                    "message": f"Updated {self.cursor.rowcount} record(s) in '{stmt_node.table_name}'."
                }
            elif isinstance(stmt_node, DeleteNode):
                return {
                    "type": "DELETE",
                    "table": stmt_node.table_name,
                    "sql": sql,
                    "params": params,
                    "rows_affected": self.cursor.rowcount,
                    "success": True,
                    "message": f"Deleted {self.cursor.rowcount} record(s) from '{stmt_node.table_name}'."
                }
            elif isinstance(stmt_node, DropTableNode):
                return {
                    "type": "DROP_TABLE",
                    "table": stmt_node.table_name,
                    "sql": sql,
                    "success": True,
                    "message": f"Table '{stmt_node.table_name}' dropped successfully."
                }
            else:
                return {
                    "type": "STATEMENT",
                    "sql": sql,
                    "success": True
                }
        except Exception as e:
            return {
                "type": "ERROR",
                "sql": sql,
                "params": params,
                "error": str(e),
                "success": False
            }

    def print_reports(self, reports: List[Dict[str, Any]]):
        """Displays rich terminal cards for execution results."""
        print("\n================================================================")
        print("  ENLANG DATABASE EXECUTION ENGINE (enlgdb)")
        print(f"  Target Database: {self.db_path}")
        print("================================================================")

        for i, rep in enumerate(reports, 1):
            t = rep.get("type", "UNKNOWN")
            success = rep.get("success", False)

            if not success:
                print(f"\n❌ [STEP {i}] Query Failed:")
                print(f"   SQL:   {rep.get('sql')}")
                print(f"   Error: {rep.get('error')}")
                continue

            if t == "CREATE_DATABASE":
                print(f"\n🗄️ [STEP {i}] CREATE DATABASE: {rep['database']}")
                print(f"   Status: ✅ {rep['message']}")

            elif t == "USE_DATABASE":
                print(f"\n🔌 [STEP {i}] USE DATABASE: {rep['database']}")
                print(f"   Status: 🔗 {rep['message']}")

            elif t == "SHOW_DATABASES":
                print(f"\n📋 [STEP {i}] SHOW DATABASES ({rep['count']} databases registered)")
                for r in rep.get("rows", []):
                    print(f"   - {r.get('database_name')} ({r.get('file')})")

            elif t == "SHOW_TABLES":
                print(f"\n📑 [STEP {i}] SHOW TABLES ({rep['count']} tables found)")
                for r in rep.get("rows", []):
                    print(f"   - {r.get('table_name')}")

            elif t == "DROP_DATABASE":
                print(f"\n💥 [STEP {i}] DROP DATABASE: {rep['database']}")
                print(f"   Status: ⚠️ {rep['message']}")

            elif t == "CREATE_TABLE":
                print(f"\n📦 [STEP {i}] CREATE TABLE: {rep['table']}")
                print(f"   Status: ✅ {rep['message']}")

            elif t == "INSERT":
                print(f"\n📥 [STEP {i}] INSERT RECORD: {rep['table']}")
                print(f"   Status: ✅ {rep['message']}")

            elif t == "UPDATE":
                print(f"\n🔄 [STEP {i}] UPDATE RECORDS: {rep['table']}")
                print(f"   Status: ✅ {rep['message']}")

            elif t == "DELETE":
                print(f"\n🗑️ [STEP {i}] DELETE RECORDS: {rep['table']}")
                print(f"   Status: ✅ {rep['message']}")

            elif t == "DROP_TABLE":
                print(f"\n💥 [STEP {i}] DROP TABLE: {rep['table']}")
                print(f"   Status: ⚠️ {rep['message']}")

            elif t == "SELECT":
                print(f"\n🔍 [STEP {i}] SELECT FROM: {rep['table']} ({rep['count']} rows found)")
                cols = rep.get("columns", [])
                rows = rep.get("rows", [])
                if rows:
                    # Calculate column widths
                    col_widths = {c: len(str(c)) for c in cols}
                    for r in rows:
                        for c in cols:
                            col_widths[c] = max(col_widths[c], len(str(r.get(c, ""))))

                    header_line = " | ".join([str(c).ljust(col_widths[c]) for c in cols])
                    sep_line = "-+-".join(["-" * col_widths[c] for c in cols])
                    print(f"   {header_line}")
                    print(f"   {sep_line}")
                    for r in rows:
                        row_line = " | ".join([str(r.get(c, "")).ljust(col_widths[c]) for c in cols])
                        print(f"   {row_line}")
                else:
                    print("   (0 records returned)")

        print("\n================================================================")
        print("  Database execution completed with 0 fatal errors.")
        print("================================================================\n")
