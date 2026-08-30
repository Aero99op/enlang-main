"""Comprehensive test suite for enlgdb (Natural English SQL & Database DSL)."""

import pytest
from enlgdb.lexer import Lexer, LexerError
from enlgdb.parser import Parser, ParserError
from enlgdb.emitter import SQLEmitter
from enlgdb.engine import DatabaseEngine
from enlgdb.compiler import compile_enlgdb_source


def test_type_header_and_table_creation():
    code = """
type enlgdb

create table "users" with:
    id as integer primary key autoincrement
    username as text not null unique
    email as text not null
    rating as real default 1000.0
    is_active as boolean default true
"""
    tokens = Lexer(code).tokenize()
    ast = Parser(tokens).parse()
    assert ast.header is not None
    assert ast.header.domain == "enlgdb"
    assert len(ast.statements) == 1

    emitter = SQLEmitter(dialect="sqlite")
    sql_tuples = emitter.emit(ast)
    assert len(sql_tuples) == 1
    sql, params = sql_tuples[0]
    assert "CREATE TABLE IF NOT EXISTS" in sql
    assert '"users"' in sql
    assert '"id" INTEGER PRIMARY KEY AUTOINCREMENT' in sql
    assert '"username" TEXT NOT NULL UNIQUE' in sql
    assert '"email" TEXT NOT NULL' in sql
    assert '"rating" REAL DEFAULT 1000.0' in sql


def test_insert_and_select():
    code = """
type enlgdb

insert into "users" values:
    username: "ShadowNinja"
    email: "ninja@esports.gg"
    rating: 1520.5

select all from "users" where rating >= 1200 and is_active is true order by rating descending limit 20
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 2

    # Insert check
    ins_sql, ins_params = sql_tuples[0]
    assert 'INSERT INTO "users"' in ins_sql
    assert "ShadowNinja" in ins_params
    assert "ninja@esports.gg" in ins_params
    assert 1520.5 in ins_params

    # Select check
    sel_sql, sel_params = sql_tuples[1]
    assert 'SELECT * FROM "users"' in sel_sql
    assert "ORDER BY \"rating\" DESC" in sel_sql
    assert "LIMIT 20" in sel_sql
    assert 1200 in sel_params


def test_safety_guard_drop_table_without_confirm_fails():
    code = """
type enlgdb

drop table "users"
"""
    tokens = Lexer(code).tokenize()
    with pytest.raises(ParserError) as excinfo:
        Parser(tokens).parse()
    assert "permanently blocked without confirmation" in str(excinfo.value)
    assert "💡 Hint: To execute safely, append 'confirmed'" in str(excinfo.value)


def test_safety_guard_drop_table_with_confirm_succeeds():
    code = """
type enlgdb

drop table "users" confirm "DROP_USERS_TABLE"
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 1
    sql, params = sql_tuples[0]
    assert 'DROP TABLE IF EXISTS "users";' == sql


def test_safety_guard_drop_table_from_schema_confirmed():
    code = """
type enlgdb

drop table temp_logs from table1 confirmed
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 1
    sql, params = sql_tuples[0]
    assert 'DROP TABLE IF EXISTS "table1.temp_logs";' == sql


def test_safety_guard_drop_table_confirmed():
    code = """
type enlgdb

drop table temp_logs confirmed
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 1
    sql, params = sql_tuples[0]
    assert 'DROP TABLE IF EXISTS "temp_logs";' == sql


def test_safety_guard_drop_column_confirmed():
    code = """
type enlgdb

drop column email from users confirmed
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 1
    sql, params = sql_tuples[0]
    assert 'ALTER TABLE "users" DROP COLUMN "email";' == sql


def test_safety_guard_unconstrained_delete_fails():
    code = """
type enlgdb

delete from "users"
"""
    tokens = Lexer(code).tokenize()
    with pytest.raises(ParserError) as excinfo:
        Parser(tokens).parse()
    assert "Unconstrained delete on table 'users' is blocked" in str(excinfo.value)
    assert "💡 Hint: To purge all rows safely" in str(excinfo.value)


def test_safety_guard_delete_all_with_confirm_succeeds():
    code = """
type enlgdb

delete all from "users" confirm "PURGE_ALL_USERS"
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 1
    sql, params = sql_tuples[0]
    assert 'DELETE FROM "users";' == sql


def test_delete_with_where_clause():
    code = """
type enlgdb

delete from "users" where id = 5
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 1
    sql, params = sql_tuples[0]
    assert 'DELETE FROM "users" WHERE ("id" = ?);' == sql
    assert params == [5]


def test_engine_execution_end_to_end():
    code = """
type enlgdb

create table "players" with:
    id as integer primary key autoincrement
    name as text not null
    score as integer default 0

insert into "players" values:
    name: "Aero"
    score: 950

insert into "players" values:
    name: "Specter"
    score: 820

update "players" set score = 1000 where name is "Aero"

select all from "players" order by score descending
"""
    ast, _ = compile_enlgdb_source(code)
    emitter = SQLEmitter(dialect="sqlite")
    engine = DatabaseEngine(db_path=":memory:")
    reports = engine.execute_program(ast, emitter)

    assert len(reports) == 5
    assert all(r["success"] for r in reports)

    # Select check
    select_rep = reports[4]
    assert select_rep["count"] == 2
    assert select_rep["rows"][0]["name"] == "Aero"
    assert select_rep["rows"][0]["score"] == 1000
    assert select_rep["rows"][1]["name"] == "Specter"
    assert select_rep["rows"][1]["score"] == 820


def test_database_lifecycle():
    code = """
type enlgdb

create database esports_hub
use database esports_hub
show databases
create table "teams" with:
    id as integer primary key autoincrement
    name as text not null
show tables
drop database esports_hub confirmed
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(ast.statements) == 6
    emitter = SQLEmitter(dialect="sqlite")
    engine = DatabaseEngine(db_path=":memory:")
    reports = engine.execute_program(ast, emitter)
    assert len(reports) == 6
    assert all(r["success"] for r in reports)
