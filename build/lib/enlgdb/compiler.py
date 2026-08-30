"""Compiler pipeline for enlgdb."""

import os
from typing import Optional, List, Tuple, Any, Dict
from pathlib import Path
from enlgdb.lexer import Lexer
from enlgdb.parser import Parser
from enlgdb.emitter import SQLEmitter
from enlgdb.engine import DatabaseEngine
from enlgdb.ast_nodes import ProgramNode


def compile_enlgdb_source(source: str, dialect: str = "sqlite") -> Tuple[ProgramNode, List[Tuple[str, List[Any]]]]:
    """Tokenizes, parses, and emits parameterized SQL statements from enlgdb source text."""
    tokens = Lexer(source).tokenize()
    ast = Parser(tokens).parse()
    emitter = SQLEmitter(dialect=dialect)
    sql_tuples = emitter.emit(ast)
    return ast, sql_tuples


def build_enlgdb_file(input_file: str, output_path: Optional[str] = None, dialect: str = "sqlite") -> str:
    """Compiles an .enlgdb file into a .sql schema/script file."""
    with open(input_file, "r", encoding="utf-8") as f:
        source = f.read()

    ast, sql_tuples = compile_enlgdb_source(source, dialect=dialect)

    sql_statements = []
    for sql, params in sql_tuples:
        if params:
            # Inline parameters into static SQL file with safe SQL literal escaping
            escaped_sql = sql
            for p in params:
                if isinstance(p, str):
                    val_str = "'" + p.replace("'", "''") + "'"
                elif p is None:
                    val_str = "NULL"
                elif isinstance(p, bool):
                    val_str = "1" if p else "0"
                else:
                    val_str = str(p)
                escaped_sql = escaped_sql.replace("?", val_str, 1)
            sql_statements.append(escaped_sql)
        else:
            sql_statements.append(sql)

    out_content = f"-- Compiled from EnLang Database DSL ({os.path.basename(input_file)})\n-- Target Dialect: {dialect.upper()}\n\n"
    out_content += "\n\n".join(sql_statements) + "\n"

    target = output_path or f"{os.path.splitext(input_file)[0]}.sql"
    with open(target, "w", encoding="utf-8") as f:
        f.write(out_content)

    print(f"[enlgdb] Compiled SQL script: {target}")
    return target


def run_enlgdb_file(input_file: str, db_path: Optional[str] = None) -> List[Dict[str, Any]]:
    """Compiles and executes an .enlgdb file against an SQLite database."""
    with open(input_file, "r", encoding="utf-8") as f:
        source = f.read()

    # Determine database path (default to same name with .db extension, or in-memory)
    target_db = db_path or f"{os.path.splitext(input_file)[0]}.db"
    
    ast, sql_tuples = compile_enlgdb_source(source, dialect="sqlite")
    emitter = SQLEmitter(dialect="sqlite")
    engine = DatabaseEngine(db_path=target_db)
    reports = engine.execute_program(ast, emitter)
    engine.print_reports(reports)
    return reports
