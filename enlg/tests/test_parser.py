"""Unit tests for the enlg Slot Parser and AST generation."""

import unittest
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from enlg.lexer.lexer import Lexer
from enlg.parser.discovery import IntentDiscoveryEngine
from enlg.parser.slot_parser import SlotParser
from enlg.ast.nodes import VariableDeclNode, OutputNode, LiteralNode, IdentifierNode
from enlg.diagnostics.diagnostics import SyntaxError

class TestParser(unittest.TestCase):

    def _parse(self, source: str):
        tokens = Lexer(source).tokenize()
        stmt_tokens = [t for t in tokens if t.type.name not in ("NEWLINE", "EOF")]
        locked = IntentDiscoveryEngine.process_statement(stmt_tokens)
        return SlotParser.parse(locked)

    def test_flexible_declaration(self):
        # Variant A
        ast1 = self._parse("declare x to 20")
        self.assertIsInstance(ast1, VariableDeclNode)
        self.assertEqual(ast1.identifier, "x")
        self.assertEqual(ast1.value.value, "20")

        # Variant B (Reverse order)
        ast2 = self._parse("declare 20 to x")
        self.assertIsInstance(ast2, VariableDeclNode)
        self.assertEqual(ast2.identifier, "x")
        self.assertEqual(ast2.value.value, "20")

        # Variant C (Equals syntax)
        ast3 = self._parse("declare x = 20")
        self.assertEqual(ast3.identifier, "x")
        self.assertEqual(ast3.value.value, "20")

    def test_missing_identifier(self):
        with self.assertRaises(SyntaxError) as ctx:
            self._parse("declare 20")
        self.assertIn("Missing identifier", str(ctx.exception))

    def test_output_display(self):
        ast = self._parse('display "hello world"')
        self.assertIsInstance(ast, OutputNode)
        self.assertIsInstance(ast.expression, LiteralNode)
        self.assertEqual(ast.expression.value, "hello world")

    def test_missing_output_value(self):
        with self.assertRaises(SyntaxError):
            self._parse("display")

    def test_boolean_and_null(self):
        ast_t = self._parse("declare flag = true")
        self.assertEqual(ast_t.value.value, True)

        ast_f = self._parse("declare flag = false")
        self.assertEqual(ast_f.value.value, False)

        ast_n = self._parse("declare none_val = null")
        self.assertEqual(type(ast_n.value).__name__, "NullNode")

    def test_list_parsing(self):
        ast = self._parse("declare arr = [1, 2, 3]")
        self.assertEqual(type(ast.value).__name__, "ListNode")
        self.assertEqual(len(ast.value.elements), 3)
        self.assertEqual(ast.value.elements[0].value, "1")

    def test_map_parsing(self):
        ast = self._parse('declare obj = {"key": "value"}')
        self.assertEqual(type(ast.value).__name__, "MapNode")
        self.assertEqual(ast.value.pairs["key"].value, "value")

    def test_unmatched_scopes(self):
        with self.assertRaises(SyntaxError) as ctx:
            self._parse("declare arr = [1, 2")
        self.assertIn("Unmatched", str(ctx.exception))

class TestFunctionParser(unittest.TestCase):

    def _parse_block(self, source: str):
        from enlg.parser.block_parser import BlockParser
        tokens = Lexer(source).tokenize()
        return BlockParser.parse(tokens)

    def test_function_def(self):
        source = 'function add with x, y:\n    return x'
        ast = self._parse_block(source)
        func_node = ast.statements[0]
        self.assertEqual(type(func_node).__name__, "FunctionDefNode")
        self.assertEqual(func_node.name, "add")
        self.assertEqual(func_node.parameters, ["x", "y"])
        
        # Test Return Node inside
        return_node = func_node.body.statements[0]
        self.assertEqual(type(return_node).__name__, "ReturnNode")
        self.assertEqual(return_node.expression.name, "x")

    def test_function_call(self):
        source = 'call calculate with 10, 20'
        ast = self._parse_block(source)
        call_node = ast.statements[0]
        self.assertEqual(type(call_node).__name__, "FunctionCallNode")
        self.assertEqual(call_node.name, "calculate")
        self.assertEqual(len(call_node.arguments), 2)
        self.assertEqual(call_node.arguments[0].value, "10")

class TestOOPParser(unittest.TestCase):
    
    def _parse_block(self, source: str):
        from enlg.parser.block_parser import BlockParser
        tokens = Lexer(source).tokenize()
        return BlockParser.parse(tokens)

    def test_class_def(self):
        source = 'class Car inherits Vehicle, Machine:\n    declare wheels = 4'
        ast = self._parse_block(source)
        class_node = ast.statements[0]
        self.assertEqual(type(class_node).__name__, "ClassDefNode")
        self.assertEqual(class_node.name, "Car")
        self.assertEqual(class_node.base_classes, ["Vehicle", "Machine"])
        
        # Test body
        decl_node = class_node.body.statements[0]
        self.assertEqual(type(decl_node).__name__, "VariableDeclNode")
        self.assertEqual(decl_node.identifier, "wheels")

    def test_instantiation(self):
        # We test it inside a statement (e.g. standalone or assigned)
        source = 'instantiate Car with "Red", 2024'
        ast = self._parse_block(source)
        inst_node = ast.statements[0]
        self.assertEqual(type(inst_node).__name__, "InstantiateNode")
        self.assertEqual(inst_node.class_name, "Car")
        self.assertEqual(len(inst_node.arguments), 2)
        self.assertEqual(inst_node.arguments[0].value, "Red")

class TestAsyncInteropParser(unittest.TestCase):
    
    def _parse_block(self, source: str):
        from enlg.parser.block_parser import BlockParser
        tokens = Lexer(source).tokenize()
        return BlockParser.parse(tokens)

    def test_async_function(self):
        source = 'async function fetch_data:\n    return 42'
        ast = self._parse_block(source)
        func_node = ast.statements[0]
        self.assertEqual(type(func_node).__name__, "FunctionDefNode")
        self.assertEqual(func_node.name, "fetch_data")
        self.assertTrue(func_node.is_async)

    def test_await_expression(self):
        source = 'await fetch_data'
        ast = self._parse_block(source)
        await_node = ast.statements[0]
        self.assertEqual(type(await_node).__name__, "AwaitNode")
        self.assertEqual(await_node.expression.name, "fetch_data")

    def test_interop_expression(self):
        source = 'interop json.dumps with my_map'
        ast = self._parse_block(source)
        interop_node = ast.statements[0]
        self.assertEqual(type(interop_node).__name__, "PythonInteropNode")
        self.assertEqual(interop_node.target, "json.dumps")
        self.assertEqual(len(interop_node.arguments), 1)
        self.assertEqual(interop_node.arguments[0].name, "my_map")

if __name__ == '__main__':
    unittest.main()
