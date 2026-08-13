"""Unit tests for the enlg Block Parser and Control Flow."""

import unittest
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from enlg.lexer.lexer import Lexer
from enlg.parser.block_parser import BlockParser
from enlg.ast.nodes import BlockNode, IfNode, WhileNode, VariableDeclNode
from enlg.diagnostics.diagnostics import SyntaxError

class TestBlockParser(unittest.TestCase):

    def _parse(self, source: str):
        tokens = Lexer(source).tokenize()
        return BlockParser.parse(tokens)

    def test_flat_statements(self):
        source = 'declare x = 20\ndisplay "hello"'
        ast = self._parse(source)
        self.assertIsInstance(ast, BlockNode)
        self.assertEqual(len(ast.statements), 2)
        self.assertEqual(ast.statements[0].identifier, "x")

    def test_if_block(self):
        source = 'if flag:\n    declare y = 50\n    display "yes"'
        ast = self._parse(source)
        self.assertEqual(len(ast.statements), 1)
        if_node = ast.statements[0]
        self.assertIsInstance(if_node, IfNode)
        self.assertEqual(if_node.condition.name, "flag")
        
        # Verify body block
        self.assertEqual(len(if_node.body.statements), 2)
        self.assertIsInstance(if_node.body.statements[0], VariableDeclNode)

    def test_while_block(self):
        source = 'while true:\n    declare count = 1'
        ast = self._parse(source)
        while_node = ast.statements[0]
        self.assertIsInstance(while_node, WhileNode)
        self.assertEqual(while_node.condition.value, True)

    def test_missing_indentation(self):
        source = 'if flag:\ndeclare y = 50'
        with self.assertRaises(SyntaxError) as ctx:
            self._parse(source)
        self.assertIn("Expected an indented block", str(ctx.exception))

    def test_attempt_rescue_blocks(self):
        source = 'attempt:\n    throw "Boom"\nrescue e:\n    display e'
        ast = self._parse(source)
        self.assertEqual(len(ast.statements), 2)
        
        attempt_node = ast.statements[0]
        rescue_node = ast.statements[1]
        
        self.assertEqual(type(attempt_node).__name__, "AttemptNode")
        self.assertEqual(type(attempt_node.body.statements[0]).__name__, "RaiseNode")
        self.assertEqual(attempt_node.body.statements[0].expression.value, "Boom")
        
        self.assertEqual(type(rescue_node).__name__, "RescueNode")
        self.assertEqual(rescue_node.error_name, "e")

    def test_import_statement(self):
        source = 'import math\ninclude os'
        ast = self._parse(source)
        self.assertEqual(len(ast.statements), 2)
        self.assertEqual(type(ast.statements[0]).__name__, "ImportNode")
        self.assertEqual(ast.statements[0].module, "math")
        self.assertEqual(ast.statements[1].module, "os")

if __name__ == '__main__':
    unittest.main()
