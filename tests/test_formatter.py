"""
tests/test_formatter.py - Unit tests for Sovereign Enlang Formatter (enlangg fmt)
"""

import unittest
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'tools')))
from enlang_fmt import format_source

class TestEnlangFormatter(unittest.TestCase):

    def test_strip_redundant_parens_in_when(self):
        src = "when (score >= 90):\n    show \"Grade A\"\n"
        expected = "when score >= 90:\n    show \"Grade A\"\n"
        self.assertEqual(format_source(src), expected)

    def test_strip_redundant_parens_in_show(self):
        src = "show (x)\n"
        expected = "show x\n"
        self.assertEqual(format_source(src), expected)

    def test_strip_redundant_parens_in_use(self):
        src = "(use \"lib_math.enlng\")\n"
        expected = "use \"lib_math.enlng\"\n"
        self.assertEqual(format_source(src), expected)

    def test_reindent_2_to_4_spaces(self):
        src = "function test with a, b:\n  when a > b:\n    give a\n  otherwise:\n    give b\n"
        expected = "function test with a, b:\n    when a > b:\n        give a\n    otherwise:\n        give b\n"
        self.assertEqual(format_source(src), expected)

    def test_operator_spacing(self):
        src = "remember x as 10+20\ny=x*2\n"
        formatted = format_source(src)
        self.assertIn("10 + 20", formatted)
        self.assertIn("y = x * 2", formatted)

    def test_preserve_string_contents(self):
        src = "message = \"hello,world=1+2\"\nshow message\n"
        formatted = format_source(src)
        self.assertIn("\"hello,world=1+2\"", formatted)

    def test_preserve_comments(self):
        src = "# This is a comment\nfreeze PI as 3.14159\n"
        formatted = format_source(src)
        self.assertEqual(formatted, src)

    def test_strip_redundant_parens_in_while(self):
        src = "repeat while (count > 0):\n    count decreases by 1\n"
        expected = "repeat while count > 0:\n    count decreases by 1\n"
        self.assertEqual(format_source(src), expected)

    def test_strip_redundant_parens_in_give(self):
        src = "function add with a, b:\n    give (a + b)\n"
        expected = "function add with a, b:\n    give a + b\n"
        self.assertEqual(format_source(src), expected)

    def test_nested_loops_and_conditionals(self):
        src = """for i from 1 to 5:
  when i == 3:
    show "three"
  otherwise:
    for j from 1 to 2:
      show j
"""
        expected = """for i from 1 to 5:
    when i == 3:
        show "three"
    otherwise:
        for j from 1 to 2:
            show j
"""
        self.assertEqual(format_source(src), expected)

    def test_cli_check_and_write(self):
        from enlang_fmt import format_file
        test_file = "scratch_test_fmt.enlng"
        try:
            with open(test_file, "w", encoding="utf-8") as f:
                f.write("when (x > 0):\n  show (x)\n")
            is_same, _ = format_file(test_file, check=True)
            self.assertFalse(is_same)
            format_file(test_file, write=True)
            with open(test_file, "r", encoding="utf-8") as f:
                content = f.read()
            self.assertEqual(content, "when x > 0:\n    show x\n")
            is_same, _ = format_file(test_file, check=True)
            self.assertTrue(is_same)
        finally:
            if os.path.exists(test_file):
                os.remove(test_file)

if __name__ == '__main__':
    unittest.main()
