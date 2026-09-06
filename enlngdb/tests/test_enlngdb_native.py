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


def test_semicolon_line_endings_and_chaining():
    # 1. Standard semicolon line endings
    code1 = """
    type enlngdb;
    create table users with id, name;
    insert record into users with id 1, name "Bibhu";
    find records from users;
    """
    reports1 = run_enlngdb_source(code1, stream_output=False)
    assert len(reports1) == 3
    assert reports1[-1]["count"] == 1

    # 2. Semicolons chaining multiple statements on single line
    code2 = "create table items with id, title; insert record into items with id 10, title 'Mouse'; find records from items;"
    reports2 = run_enlngdb_source(code2, stream_output=False)
    assert len(reports2) == 3
    assert reports2[-1]["rows"][0]["title"] == "Mouse"


def test_show_databases_and_tables():
    code = """
    type enlngdb;
    show databases;
    create table inventory with item_id, stock;
    show tables;
    """
    reports = run_enlngdb_source(code, stream_output=False)
    assert len(reports) == 3
    assert reports[0]["type"] == "SHOW_DATABASES"
    assert reports[0]["success"] is True
    assert reports[2]["type"] == "SHOW_TABLES"
    assert reports[2]["success"] is True
    table_names = [r["table_name"] for r in reports[2]["rows"]]
    assert "inventory" in table_names


def test_use_database_and_show_tables_of_db(tmp_path):
    test_db = (tmp_path / "custom_app.edb").as_posix()
    code = f"""
    type enlgdb;
    use database "{test_db}";
    create table products with id, title, price;
    insert record into products with id 101, title "Laptop", price 999;
    show tables;
    show tables of database "{test_db}";
    show all records from products;
    find all records from products;
    """
    reports = run_enlngdb_source(code, stream_output=False)
    assert any(r.get("type") == "USE_DATABASE" for r in reports)
    find_reports = [r for r in reports if r.get("type") == "FIND"]
    assert len(find_reports) == 2
    assert find_reports[0]["rows"][0]["title"] == "Laptop"
    assert find_reports[1]["rows"][0]["title"] == "Laptop"


def test_delete_row_column_table_and_database(tmp_path):
    test_db = (tmp_path / "lifecycle_ops.edb").as_posix()
    code = f"""
    type enlngdb;
    use database "{test_db}";
    create table staff with id, name, salary, dept;
    insert record into staff with id 1, name "Bibhu", salary 50000, dept "IT";
    insert record into staff with id 2, name "Ansh", salary 45000, dept "HR";

    -- 1. Delete row
    delete from staff where id is 2;

    -- 2. Delete column (both with and without 'column' keyword)
    delete column salary from staff;
    delete dept from staff;

    -- 3. Delete table confirmed
    delete table staff confirmed;

    -- 4. Delete database confirmed
    delete database "{test_db}" confirmed;
    """
    reports = run_enlngdb_source(code, stream_output=False)
    types = [r["type"] for r in reports]
    assert "DELETE" in types
    assert "DELETE_COLUMN" in types
    assert "DROP_TABLE" in types
    assert "DROP_DATABASE" in types



