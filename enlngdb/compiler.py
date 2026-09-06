"""Compiler and pipeline executor for enlngdb (Sovereign Native Storage Engine - ZERO SQL)."""

import os
from typing import Optional, List, Dict, Any, Tuple
from enlngdb.lexer import Lexer
from enlngdb.parser import Parser
from enlngdb.ast_nodes import ProgramNode
from enlngdb.engine import NativeExecutionEngine
from enlngdb.storage import NativeStorageEngine


def compile_enlngdb_source(source: str) -> ProgramNode:
    """Tokenizes and parses conversational English database statements into an AST."""
    tokens = Lexer(source).tokenize()
    ast = Parser(tokens).parse()
    return ast


def run_enlngdb_source(source: str,
                       db_path: Optional[str] = None,
                       engine: Optional[NativeExecutionEngine] = None,
                       stream_output: bool = True) -> List[Dict[str, Any]]:
    """Compiles and directly executes enlngdb source code against the sovereign native storage engine."""
    ast = compile_enlngdb_source(source)
    if engine is None:
        engine = NativeExecutionEngine(db_path=db_path, stream_output=stream_output)
    reports = engine.execute_program(ast)
    if engine.db_path:
        engine.storage.save_to_disk(engine.db_path)
    return reports


def run_enlngdb_file(input_file: str,
                     db_path: Optional[str] = None,
                     stream_output: bool = True) -> List[Dict[str, Any]]:
    """Loads and executes an .enlngdb script file."""
    with open(input_file, "r", encoding="utf-8") as f:
        source = f.read()

    target_db = db_path
    engine = NativeExecutionEngine(db_path=target_db, stream_output=stream_output)
    reports = run_enlngdb_source(source, db_path=target_db, engine=engine, stream_output=stream_output)
    return reports
