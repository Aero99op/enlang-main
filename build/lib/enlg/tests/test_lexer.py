"""Unit tests for enlg Lexer."""

import unittest
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from enlg.lexer.lexer import Lexer
from enlg.lexer.tokens import TokenType
from enlg.diagnostics.diagnostics import LexicalError

class TestLexer(unittest.TestCase):

    def test_basic_tokenization(self):
        source = 'declare x = 20\ndisplay "hello"'
        lexer = Lexer(source)
        tokens = lexer.tokenize()
        
        types = [t.type for t in tokens]
        self.assertEqual(types[0], TokenType.IDENTIFIER) # declare
        self.assertEqual(types[1], TokenType.IDENTIFIER) # x
        self.assertEqual(types[2], TokenType.SYMBOL)     # =
        self.assertEqual(types[3], TokenType.NUMBER)     # 20
        self.assertEqual(types[4], TokenType.NEWLINE)
        self.assertEqual(types[5], TokenType.IDENTIFIER) # display
        self.assertEqual(types[6], TokenType.STRING)     # "hello"

    def test_indentation_tracking(self):
        source = 'loop 3 times:\n    print "hi"\nprint "done"'
        lexer = Lexer(source)
        tokens = lexer.tokenize()
        
        indents = [t for t in tokens if t.type == TokenType.INDENT]
        dedents = [t for t in tokens if t.type == TokenType.DEDENT]
        
        self.assertEqual(len(indents), 1)
        self.assertEqual(len(dedents), 1)

    def test_lexical_error(self):
        source = 'declare $x = 20'
        lexer = Lexer(source)
        with self.assertRaises(LexicalError) as ctx:
            lexer.tokenize()
        self.assertIn("Found '$'", str(ctx.exception))

if __name__ == '__main__':
    unittest.main()
