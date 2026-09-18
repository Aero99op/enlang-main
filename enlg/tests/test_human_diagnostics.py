"""Comprehensive test suite for Human-Friendly Compiler Diagnostics across all Enlang domains."""

import unittest
import sys
import os

# Add root to sys.path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from enlg.diagnostics.error_formatter import format_human_diagnostic, analyze_error
from enlg.diagnostics.diagnostics import EnlgError, SyntaxError, UnknownHintError
from enlg.diagnostics.checker import check_file


class TestHumanDiagnostics(unittest.TestCase):

    def test_core_assignment_in_condition(self):
        source = 'remember score as 100\nwhen score = 90:\n    show "Grade A"\n'
        card = format_human_diagnostic(
            source=source,
            line=2,
            col=12,
            file_path="sample.enlng",
            domain="enlng",
            error_type="SyntaxError"
        )
        self.assertIn("Location: sample.enlng:2:", card)
        self.assertIn("when score = 90:", card)
        self.assertIn("^", card)
        self.assertIn("Invalid assignment operator '='", card)
        self.assertIn("when score == 90:", card)

    def test_core_missing_colon(self):
        source = 'when score is greater than 50\n    show "Pass"\n'
        card = format_human_diagnostic(
            source=source,
            line=1,
            col=29,
            file_path="logic.enlng",
            domain="enlng",
            error_type="SyntaxError"
        )
        self.assertIn("Missing colon ':'", card)
        self.assertIn("--> when score is greater than 50:", card)

    def test_core_incomplete_variable_declaration(self):
        source = 'remember count\ncount increases by 1\n'
        card = format_human_diagnostic(
            source=source,
            line=1,
            col=9,
            file_path="vars.enlng",
            domain="enlng",
            error_type="SyntaxError"
        )
        self.assertIn("Incomplete variable declaration for 'count'", card)
        self.assertIn("remember count as 100", card)

    def test_core_keyword_typo_suggestion(self):
        source = 'shwo "Welcome to Enlang"\n'
        card = format_human_diagnostic(
            source=source,
            line=1,
            col=1,
            file_path="hello.enlng",
            domain="enlng",
            error_type="SyntaxError"
        )
        self.assertIn("Did you mean 'show'?", card)

    def test_core_unclosed_parenthesis(self):
        source = 'remember res as (10 * (20 + 3)\n'
        card = format_human_diagnostic(
            source=source,
            line=1,
            col=31,
            file_path="calc.enlng",
            domain="enlng",
            error_type="SyntaxError"
        )
        self.assertIn("Unmatched opening parenthesis '('", card)

    def test_database_table_missing_columns(self):
        source = 'type enlngdb\ncreate table users with\n'
        card = format_human_diagnostic(
            source=source,
            line=2,
            col=1,
            file_path="schema.enlngdb",
            domain="enlngdb",
            error_type="ParserError"
        )
        self.assertIn("missing column definitions", card)
        self.assertIn("create table users with id, name", card)

    def test_database_query_missing_table(self):
        source = 'type enlngdb\nfind all\n'
        card = format_human_diagnostic(
            source=source,
            line=2,
            col=1,
            file_path="query.enlngdb",
            domain="enlngdb",
            error_type="ParserError"
        )
        self.assertIn("Missing source table in query", card)
        self.assertIn("find records from users", card)

    def test_enlg_error_integration_and_backward_compatibility(self):
        # 1. Backward compatibility for __str__()
        legacy_err = EnlgError("E1001", details="Unexpected '@'")
        self.assertTrue(str(legacy_err).startswith("[E1001]"))
        self.assertIn("Unexpected '@'", str(legacy_err))

        # 2. Rich human diagnostic via to_human_diagnostic()
        rich_err = EnlgError(
            "E1003",
            line=5,
            column=10,
            file_path="test.enlng",
            source_code="remember x as 10\nremember y as 20\nremember z as 30\n\nwhen x = 10:\n    show x\n",
            what="Invalid syntax in expression",
            why="Enlang conditions cannot use assignment",
            suggestions=["Use '==' instead of '='"],
            domain="enlng"
        )
        card = rich_err.to_human_diagnostic()
        self.assertIn("Location: test.enlng:5:", card)
        self.assertIn("when x = 10:", card)
        self.assertIn("Invalid syntax in expression", card)
        self.assertIn("Use '==' instead of '='", card)


if __name__ == '__main__':
    unittest.main()
