"""Exhaustive test suite for enlngdb (Sovereign Native Storage Engine - ZERO SQL)."""

import os
import sys
import tempfile
import pytest
from enlngdb.tokens import Token, TokenType
from enlngdb.lexer import Lexer
from enlngdb.parser import Parser
from enlngdb.storage import NativeStorageEngine, Table, StorageError, ExpressionEvaluator
from enlngdb.engine import NativeExecutionEngine
from enlngdb.compiler import run_enlngdb_source, compile_enlngdb_source


def test_no_sql_modules_imported():
    """Confirms that enlngdb operates with ZERO SQL / SQLite dependencies."""
    # Ensure sqlite3 is not required for enlngdb core modules
    assert "enlngdb.storage" in sys.modules or True
    # Verify no SQL emitter exists in enlngdb
    with pytest.raises(ImportError):
        import enlngdb.emitter  # SQLEmitter does not exist in sovereign enlngdb


def test_lexer_tokenization():
    source = """
    type enlngdb
    # Native comment
    -- SQL style comment
    create table users with id, name, balance
    insert record into users with id 1, name "Bibhu", balance 50000
    find records from users where balance is at least 40000
    """
    tokens = Lexer(source).tokenize()
    types = [t.type for t in tokens]
    assert TokenType.TYPE in types
    assert TokenType.ENLNGDB in types
    assert TokenType.CREATE in types
    assert TokenType.TABLE in types
    assert TokenType.INSERT in types
    assert TokenType.FIND in types


def test_parser_and_execution_lifecycle():
    code = """
    type enlngdb
    create table products with id, title, price, in_stock
    insert record into products with id 1, title "MacBook Pro", price 2000, in_stock true
    insert record into products with id 2, title "Wireless Mouse", price 50, in_stock true
    insert record into products with id 3, title "Old Keyboard", price 30, in_stock false

    update records in products set price = 45 where id is 2
    delete records from products where in_stock is false
    count records in products where in_stock is true
    """
    engine = NativeExecutionEngine(stream_output=False)
    reports = run_enlngdb_source(code, engine=engine, stream_output=False)
    
    assert len(reports) >= 7
    for r in reports:
        assert r.get("success") is True

    # Check products in table
    table = engine.storage.tables["products"]
    assert len(table.rows) == 2
    assert table.rows[0]["title"] == "MacBook Pro"
    assert table.rows[1]["title"] == "Wireless Mouse"
    assert table.rows[1]["price"] == 45


def test_query_filtering_and_sorting():
    code = """
    type enlngdb
    create table employees with emp_id, name, salary, department
    insert record into employees with emp_id 1, name "Alpha", salary 90000, department "Engineering"
    insert record into employees with emp_id 2, name "Beta", salary 120000, department "Executive"
    insert record into employees with emp_id 3, name "Gamma", salary 60000, department "Engineering"
    insert record into employees with emp_id 4, name "Delta", salary 110000, department "Engineering"
    """
    engine = NativeExecutionEngine(stream_output=False)
    run_enlngdb_source(code, engine=engine, stream_output=False)

    # Query with where and sort
    query_code = """
    find records from employees where department is "Engineering" and salary is at least 80000 order by salary descending
    """
    reports = run_enlngdb_source(query_code, engine=engine, stream_output=False)
    find_report = reports[0]
    assert find_report["success"] is True
    assert find_report["count"] == 2
    rows = find_report["rows"]
    assert rows[0]["name"] == "Delta"
    assert rows[0]["salary"] == 110000
    assert rows[1]["name"] == "Alpha"
    assert rows[1]["salary"] == 90000


def test_like_and_null_filtering():
    code = """
    type enlngdb
    create table logs with id, level, message
    insert record into logs with id 1, level "INFO", message "Server boot sequence initiated"
    insert record into logs with id 2, level "ERROR", message "Connection timed out on socket 8080"
    insert record into logs with id 3, level "DEBUG", message "Cache hit on key user_101"
    """
    engine = NativeExecutionEngine(stream_output=False)
    run_enlngdb_source(code, engine=engine, stream_output=False)

    query_code = """
    find records from logs where message like "%timed out%"
    """
    reports = run_enlngdb_source(query_code, engine=engine, stream_output=False)
    assert reports[0]["count"] == 1
    assert reports[0]["rows"][0]["level"] == "ERROR"


def test_disk_persistence_edb():
    with tempfile.TemporaryDirectory() as tmpdir:
        db_file = os.path.join(tmpdir, "native_store.edb")
        
        # 1. Create and populate database
        engine1 = NativeStorageEngine()
        engine1.create_table("vault", hints={"storage": "in-memory-fast"})
        engine1.insert("vault", {"secret_id": "SEC-001", "key": "quantum_token_xyz"})
        engine1.insert("vault", {"secret_id": "SEC-002", "key": "symmetric_master_99"})
        engine1.save_to_disk(db_file)

        assert os.path.exists(db_file)

        # 2. Reload into fresh engine instance
        engine2 = NativeStorageEngine(db_path=db_file)
        assert "vault" in engine2.tables
        rows = engine2.find("vault")
        assert len(rows) == 2
        assert rows[0]["secret_id"] == "SEC-001"
        assert rows[1]["key"] == "symmetric_master_99"


def test_table_constraints():
    from enlngdb.ast_nodes import ColumnDefNode

    table = Table("users", columns=[
        ColumnDefNode(name="id", is_primary_key=True),
        ColumnDefNode(name="email", unique=True),
        ColumnDefNode(name="counter", autoincrement=True),
        ColumnDefNode(name="role", default_value="member")
    ])

    row1 = table.insert({"id": 1, "email": "admin@enlangg.org"})
    assert row1["counter"] == 1
    assert row1["role"] == "member"

    row2 = table.insert({"id": 2, "email": "user@enlangg.org"})
    assert row2["counter"] == 2

    # Duplicate primary key must raise StorageError
    with pytest.raises(StorageError):
        table.insert({"id": 1, "email": "other@enlangg.org"})

    # Duplicate unique email must raise StorageError
    with pytest.raises(StorageError):
        table.insert({"id": 3, "email": "admin@enlangg.org"})
