import sys
from enlg.lexer.lexer import Lexer
from enlg.parser.block_parser import BlockParser
from enlg.compiler.generator import CIRGenerator
from enlg.runtime.vm import VirtualMachine

source = 'function add with x, y:\n    return x\ncall add with "passed", 2'
ast = BlockParser.parse(Lexer(source).tokenize())
print("AST:", ast)

gen = CIRGenerator()
cir = gen.generate(ast)
print("CIR:")
for instr in cir.instructions:
    print("  ", instr)
    
vm = VirtualMachine()
vm.execute(cir)
print("Stack:", vm.stack)
