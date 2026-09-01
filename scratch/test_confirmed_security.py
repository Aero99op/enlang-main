import pytest
from enlgdb.lexer import Lexer
from enlgdb.parser import Parser, ParserError

def test_drop_table_without_confirmed_blocked():
    code = """type enlngdb
drop table users
"""
    tokens = Lexer(code).tokenize()
    with pytest.raises(ParserError) as exc_info:
        Parser(tokens).parse()
    assert "permanently blocked without confirmation" in str(exc_info.value)
    assert "drop table users confirmed" in str(exc_info.value)

def test_drop_table_with_confirmed_allowed():
    code = """type enlngdb
drop table users confirmed
"""
    tokens = Lexer(code).tokenize()
    prog = Parser(tokens).parse()
    assert len(prog.statements) == 1
    assert prog.statements[0].table_name == "users"
    assert prog.statements[0].confirmation_token == "CONFIRMED"

def test_delete_all_without_confirmed_blocked():
    code = """type enlngdb
delete from users
"""
    tokens = Lexer(code).tokenize()
    with pytest.raises(ParserError) as exc_info:
        Parser(tokens).parse()
    assert "Destructive operation" in str(exc_info.value)

def test_delete_all_with_confirmed_allowed():
    code = """type enlngdb
delete all from users confirmed
"""
    tokens = Lexer(code).tokenize()
    prog = Parser(tokens).parse()
    assert len(prog.statements) == 1
    assert prog.statements[0].is_all is True
    assert prog.statements[0].confirmation_token == "CONFIRMED"

if __name__ == "__main__":
    test_drop_table_without_confirmed_blocked()
    test_drop_table_with_confirmed_allowed()
    test_delete_all_without_confirmed_blocked()
    test_delete_all_with_confirmed_allowed()
    print("ALL DESTRUCTIVE CONFIRMED SECURITY TESTS PASSED!")
