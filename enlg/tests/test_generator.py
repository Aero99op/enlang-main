"""Unit tests for the enlg CIR Generator."""

import unittest
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from enlg.lexer.lexer import Lexer
from enlg.parser.block_parser import BlockParser
from enlg.compiler.generator import CIRGenerator
from enlg.compiler.cir import CIROpcode

class TestCIRGenerator(unittest.TestCase):

    def _compile_to_cir(self, source: str):
        tokens = Lexer(source).tokenize()
        ast = BlockParser.parse(tokens)
        gen = CIRGenerator()
        return gen.generate(ast)

    def test_basic_cir_generation(self):
        source = 'declare name = "enlg"\ndisplay name'
        cir_block = self._compile_to_cir(source)
        insts = cir_block.instructions
        
        self.assertEqual(len(insts), 4)
        
        # 1. LOAD_CONST "enlg"
        self.assertEqual(insts[0].opcode, CIROpcode.LOAD_CONST)
        self.assertEqual(insts[0].args[0], "enlg")
        
        # 2. STORE_VAR "name"
        self.assertEqual(insts[1].opcode, CIROpcode.STORE_VAR)
        self.assertEqual(insts[1].args[0], "name")
        
        # 3. LOAD_VAR "name"
        self.assertEqual(insts[2].opcode, CIROpcode.LOAD_VAR)
        self.assertEqual(insts[2].args[0], "name")
        
        # 4. PRINT
        self.assertEqual(insts[3].opcode, CIROpcode.PRINT)
        self.assertEqual(len(insts[3].args), 0)

    def test_if_statement_cir(self):
        source = 'if flag:\n    display 1'
        cir_block = self._compile_to_cir(source)
        insts = cir_block.instructions
        
        self.assertEqual(len(insts), 4)
        # 0: LOAD_VAR "flag"
        self.assertEqual(insts[0].opcode, CIROpcode.LOAD_VAR)
        self.assertEqual(insts[0].args[0], "flag")
        # 1: JUMP_IF_FALSE 4
        self.assertEqual(insts[1].opcode, CIROpcode.JUMP_IF_FALSE)
        self.assertEqual(insts[1].args[0], 4)
        # 2: LOAD_CONST 1
        self.assertEqual(insts[2].opcode, CIROpcode.LOAD_CONST)
        # 3: PRINT
        self.assertEqual(insts[3].opcode, CIROpcode.PRINT)

    def test_while_statement_cir(self):
        source = 'while flag:\n    display 1'
        cir_block = self._compile_to_cir(source)
        insts = cir_block.instructions
        
        self.assertEqual(len(insts), 5)
        # 0: LOAD_VAR "flag"
        self.assertEqual(insts[0].opcode, CIROpcode.LOAD_VAR)
        # 1: JUMP_IF_FALSE 5
        self.assertEqual(insts[1].opcode, CIROpcode.JUMP_IF_FALSE)
        self.assertEqual(insts[1].args[0], 5)
        # 2: LOAD_CONST 1
        self.assertEqual(insts[2].opcode, CIROpcode.LOAD_CONST)
        # 3: PRINT
        self.assertEqual(insts[3].opcode, CIROpcode.PRINT)
        # 4: JUMP 0
        self.assertEqual(insts[4].opcode, CIROpcode.JUMP)
        self.assertEqual(insts[4].args[0], 0)

    def test_function_def_cir(self):
        source = 'function add with x, y:\n    return x'
        cir_block = self._compile_to_cir(source)
        insts = cir_block.instructions
        
        self.assertEqual(len(insts), 2)
        # 0: MAKE_FUNCTION
        self.assertEqual(insts[0].opcode, CIROpcode.MAKE_FUNCTION)
        self.assertEqual(insts[0].args[0], ["x", "y"]) # parameters
        # Test isolated block
        isolated_block = insts[0].args[1]
        self.assertEqual(isolated_block.instructions[0].opcode, CIROpcode.LOAD_VAR)
        self.assertEqual(isolated_block.instructions[0].args[0], "x")
        self.assertEqual(isolated_block.instructions[1].opcode, CIROpcode.RETURN)
        self.assertEqual(isolated_block.instructions[1].args[0], True) # has_value
        
        # 1: STORE_VAR "add"
        self.assertEqual(insts[1].opcode, CIROpcode.STORE_VAR)
        self.assertEqual(insts[1].args[0], "add")

    def test_function_call_cir(self):
        source = 'call calculate with 10, 20'
        cir_block = self._compile_to_cir(source)
        insts = cir_block.instructions
        
        self.assertEqual(len(insts), 4)
        # 0: LOAD_VAR "calculate"
        self.assertEqual(insts[0].opcode, CIROpcode.LOAD_VAR)
        self.assertEqual(insts[0].args[0], "calculate")
        
        # 1, 2: LOAD_CONST args
        self.assertEqual(insts[1].opcode, CIROpcode.LOAD_CONST)
        self.assertEqual(insts[1].args[0], "10")
        self.assertEqual(insts[2].opcode, CIROpcode.LOAD_CONST)
        self.assertEqual(insts[2].args[0], "20")
        
        # 3: CALL
        self.assertEqual(insts[3].opcode, CIROpcode.CALL)
        self.assertEqual(insts[3].args[0], 2) # argc

    def test_class_def_cir(self):
        source = 'class Car:\n    declare wheels = 4'
        cir_block = self._compile_to_cir(source)
        insts = cir_block.instructions
        
        self.assertEqual(len(insts), 2)
        # 0: MAKE_CLASS
        self.assertEqual(insts[0].opcode, CIROpcode.MAKE_CLASS)
        self.assertEqual(insts[0].args[0], "Car")
        
        # Test isolated block
        isolated_block = insts[0].args[2]
        self.assertEqual(isolated_block.instructions[0].opcode, CIROpcode.LOAD_CONST)
        self.assertEqual(isolated_block.instructions[0].args[0], "4")
        self.assertEqual(isolated_block.instructions[1].opcode, CIROpcode.STORE_VAR)
        self.assertEqual(isolated_block.instructions[1].args[0], "wheels")
        
        # 1: STORE_VAR "Car"
        self.assertEqual(insts[1].opcode, CIROpcode.STORE_VAR)
        self.assertEqual(insts[1].args[0], "Car")

    def test_instantiate_cir(self):
        source = 'new Car with "Red", 2'
        cir_block = self._compile_to_cir(source)
        insts = cir_block.instructions
        
        # 0: LOAD_VAR "Car"
        self.assertEqual(insts[0].opcode, CIROpcode.LOAD_VAR)
        self.assertEqual(insts[0].args[0], "Car")
        
        # 1, 2: LOAD_CONST args
        self.assertEqual(insts[1].opcode, CIROpcode.LOAD_CONST)
        self.assertEqual(insts[1].args[0], "Red")
        self.assertEqual(insts[2].opcode, CIROpcode.LOAD_CONST)
        self.assertEqual(insts[2].args[0], "2")
        
        # 3: INSTANTIATE
        self.assertEqual(insts[3].opcode, CIROpcode.INSTANTIATE)
        self.assertEqual(insts[3].args[0], 2) # argc

    def test_attempt_rescue_cir(self):
        source = 'attempt:\n    raise "error"\nrescue e:\n    declare recovered = e'
        cir_block = self._compile_to_cir(source)
        insts = cir_block.instructions
        
        self.assertEqual(insts[0].opcode, CIROpcode.SETUP_ATTEMPT)
        
        # 1: LOAD_CONST "error"
        # 2: RAISE
        self.assertEqual(insts[2].opcode, CIROpcode.RAISE)
        
        # 3: END_ATTEMPT
        self.assertEqual(insts[3].opcode, CIROpcode.END_ATTEMPT)
        
        # 4: JUMP
        self.assertEqual(insts[4].opcode, CIROpcode.JUMP)
        
        # SETUP_ATTEMPT target should be the start of rescue block (index 5)
        self.assertEqual(insts[0].args[0], 5)
        
        # 5: STORE_VAR "e"
        self.assertEqual(insts[5].opcode, CIROpcode.STORE_VAR)
        self.assertEqual(insts[5].args[0], "e")
        
        # JUMP target should bypass rescue block
        self.assertEqual(insts[4].args[0], len(insts))

    def test_await_cir(self):
        source = 'await fetch'
        cir_block = self._compile_to_cir(source)
        insts = cir_block.instructions
        
        self.assertEqual(len(insts), 2)
        # 0: LOAD_VAR "fetch"
        self.assertEqual(insts[0].opcode, CIROpcode.LOAD_VAR)
        self.assertEqual(insts[0].args[0], "fetch")
        
        # 1: AWAIT
        self.assertEqual(insts[1].opcode, CIROpcode.AWAIT)

    def test_import_cir(self):
        source = 'import math'
        cir_block = self._compile_to_cir(source)
        insts = cir_block.instructions
        
        self.assertEqual(len(insts), 1)
        self.assertEqual(insts[0].opcode, CIROpcode.IMPORT_MODULE)
        self.assertEqual(insts[0].args[0], "math")

if __name__ == '__main__':
    unittest.main()
