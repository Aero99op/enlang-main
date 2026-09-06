"""Comprehensive test suite for Conversational English SQL, Silent Words, and Hints in enlgdb."""

import pytest
from enlgdb.compiler import compile_enlgdb_source
from enlgdb.engine import DatabaseEngine
from enlgdb.emitter import SQLEmitter


def test_silent_words_and_english_verbs_in_select():
    code = """
type enlgdb

find the username, the email from the table "users" where the rating is greater than or equal to 1200
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 1
    sql, params = sql_tuples[0]
    assert 'SELECT "username", "email" FROM "users" WHERE ("rating" >= ?);' == sql
    assert params == [1200]


def test_conversational_comparisons_and_sorting():
    code = """
type enlgdb

fetch all from "users" where rating is at least 1500 and is_active is true sorted by rating highest first limit to 10
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 1
    sql, params = sql_tuples[0]
    assert 'SELECT * FROM "users"' in sql
    assert '("rating" >= ?)' in sql
    assert 'ORDER BY "rating" DESC' in sql
    assert 'LIMIT 10' in sql
    assert params == [1500, True]


def test_top_n_and_lowest_first():
    code = """
type enlgdb

get top 5 from "users" where rating is more than 800 sorted by rating lowest first
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 1
    sql, params = sql_tuples[0]
    assert 'SELECT * FROM "users"' in sql
    assert '("rating" > ?)' in sql
    assert 'ORDER BY "rating" ASC' in sql
    assert 'LIMIT 5' in sql
    assert params == [800]


def test_query_hints():
    code = """
type enlgdb

find all from "users" where rating is at least 1000 hint index: "idx_user_rating", cache: true
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 1
    sql, params = sql_tuples[0]
    assert 'INDEXED BY "idx_user_rating"' in sql
    assert '/*+ HINT(' in sql
    assert ast.statements[0].hints["index"] == "idx_user_rating"
    assert ast.statements[0].hints["cache"] is True


def test_table_level_hints():
    code = """
type enlgdb

create table "accounts" with:
    hint engine: "in_memory"
    hint journal: "wal"
    id as integer primary key autoincrement
    account_holder as text not null
    balance as real default 0.0
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 1
    assert ast.statements[0].hints["engine"] == "in_memory"
    assert ast.statements[0].hints["journal"] == "wal"
    sql, _ = sql_tuples[0]
    assert 'CREATE TABLE IF NOT EXISTS "accounts"' in sql


def test_standalone_hint_statement():
    code = """
type enlgdb

hint timeout: 5000, strict_mode: true
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 1
    sql, _ = sql_tuples[0]
    assert '-- HINT:' in sql
    assert ast.statements[0].hints["timeout"] == 5000
    assert ast.statements[0].hints["strict_mode"] is True


def test_count_query_syntax():
    code = """
type enlgdb

count records in "users" where rating is at least 1000
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    assert len(sql_tuples) == 1
    sql, params = sql_tuples[0]
    assert 'SELECT COUNT(*) FROM "users" WHERE ("rating" >= ?);' == sql
    assert params == [1000]


def test_conversational_dml_and_end_to_end_execution():
    code = """
type enlgdb

create table "players" with:
    hint engine: "sqlite"
    id as integer primary key autoincrement
    name as text not null
    score as integer default 0
    tier as text default "Rookie"

insert record into "players" values:
    name: "Aero"
    score: 1500
    tier: "Master"

save into "players" values:
    name: "Specter"
    score: 1200
    tier: "Elite"

save into "players" values:
    name: "Shadow"
    score: 600
    tier: "Rookie"

# Update with 'to' and English equality
update "players" set score to 1800 where name is "Aero"

# Find top 2 players with score at least 1000
find top 2 from the table "players" where score is at least 1000 sorted by score highest first

# Direct English count
count rows in "players" where score is greater than 1000

# Remove low score players
remove from "players" where score is below 800
"""
    ast, sql_tuples = compile_enlgdb_source(code)
    emitter = SQLEmitter(dialect="sqlite")
    engine = DatabaseEngine(db_path=":memory:")
    reports = engine.execute_program(ast, emitter)

    assert len(reports) == 8
    assert all(r["success"] for r in reports)

    # Verify select top 2
    select_rep = reports[5]
    assert select_rep["count"] == 2
    assert select_rep["rows"][0]["name"] == "Aero"
    assert select_rep["rows"][0]["score"] == 1800
    assert select_rep["rows"][1]["name"] == "Specter"
    assert select_rep["rows"][1]["score"] == 1200

    # Verify count query
    count_rep = reports[6]
    assert count_rep["rows"][0]["COUNT(*)"] == 2

    # Verify delete removed Shadow
    check_ast, _ = compile_enlgdb_source('type enlgdb\nfind all from "players"')
    remaining_rep = engine.execute_program(check_ast, emitter)[0]
    assert remaining_rep["count"] == 2
    assert all(r["name"] != "Shadow" for r in remaining_rep["rows"])
