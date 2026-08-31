"""Non-destructive Sample Programs Benchmark Runner.

Executes all sample programs across all domains in the repo to verify complete operational integrity.
"""

import sys
import os
import subprocess

repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, repo_root)

sample_enlg_files = [
    "atm_management.enlg",
    "palindrome.enlg",
    "linked_list.enlg",
    "middle_linked_list.enlg",
    "sample_loop.enlg",
    "test_advanced_operators.enlg",
]

print("=" * 70)
print("  EXECUTING ALL SAMPLE ENLG SCRIPTS IN WORKSPACE")
print("=" * 70)

from enlg.lexer.lexer import Lexer
from enlg.parser.block_parser import BlockParser
from enlg.compiler.generator import CIRGenerator
from enlg.runtime.vm import VirtualMachine

passed = 0
total = 0

for sample in sample_enlg_files:
    file_path = os.path.join(repo_root, sample)
    if not os.path.exists(file_path):
        continue
    total += 1
    with open(file_path, "r", encoding="utf-8") as f:
        src = f.read()
    try:
        tokens = Lexer(src).tokenize()
        ast = BlockParser.parse(tokens)
        cir = CIRGenerator().generate(ast)
        vm = VirtualMachine()
        vm.execute(cir)
        print(f"  [SUCCESS] {sample}")
        passed += 1
    except Exception as e:
        print(f"  [FAILURE] {sample} -> {e}")

print(f"\nSample Programs Pass Rate: {passed}/{total} ({passed/total*100:.1f}%)")
print("=" * 70)
