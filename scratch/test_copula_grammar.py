"""Comprehensive Test Suite for Universal Connectors, 'is' Copula Bridge, and String Boundaries."""

import sys
import os

repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, repo_root)

from enlg.lexer.lexer import Lexer
from enlg.parser.block_parser import BlockParser
from enlg.compiler.generator import CIRGenerator
from enlg.runtime.vm import VirtualMachine

def run_code(code: str) -> VirtualMachine:
    tokens = Lexer(code).tokenize()
    ast = BlockParser.parse(tokens)
    cir = CIRGenerator().generate(ast)
    vm = VirtualMachine()
    vm.execute(cir)
    return vm

print("=" * 75)
print("  TESTING UNIVERSAL CONNECTORS, 'IS' COPULA, & STRING DATA BOUNDARIES")
print("=" * 75)

# 1. Test 'is greater than'
t1 = """type enlg
create a score of 78
if score is greater than 50:
    declare status = "PASSED"
else:
    declare status = "FAILED"
"""
vm1 = run_code(t1)
assert vm1.environment.get("status") == "PASSED"
print("  [PASS] 1. 'score is greater than 50' -> PASSED")

# 2. Test 'is at least'
t2 = """type enlg
create score 50
if score is at least 50:
    declare result = "QUALIFIED"
else:
    declare result = "DISQUALIFIED"
"""
vm2 = run_code(t2)
assert vm2.environment.get("result") == "QUALIFIED"
print("  [PASS] 2. 'score is at least 50' -> QUALIFIED")

# 3. Test 'is equal to'
t3 = """type enlg
create score 100
if score is equal to 100:
    declare rank = "TOP"
else:
    declare rank = "NORMAL"
"""
vm3 = run_code(t3)
assert vm3.environment.get("rank") == "TOP"
print("  [PASS] 3. 'score is equal to 100' -> TOP")

# 4. Test 'is not equal to' / 'differs from'
t4 = """type enlg
create score 25
if score is not equal to 0:
    declare non_zero = true
else:
    declare non_zero = false
"""
vm4 = run_code(t4)
assert vm4.environment.get("non_zero") == True
print("  [PASS] 4. 'score is not equal to 0' -> True")

# 5. Test Active Verbs ('exceeds', 'reaches', 'equals')
t5 = """type enlg
create points 80
if points exceeds 75:
    declare flag1 = true

if points reaches 80:
    declare flag2 = true

if points equals 80:
    declare flag3 = true
"""
vm5 = run_code(t5)
assert vm5.environment.get("flag1") == True
assert vm5.environment.get("flag2") == True
assert vm5.environment.get("flag3") == True
print("  [PASS] 5. Active verbs 'exceeds', 'reaches', 'equals' -> All True")

# 6. Test Quoted Strings containing keywords (Data Boundary Law)
t6 = """type enlg
declare payload = "the watch is good and Aero is a boy"
declare query = "create table users with id is null"
display the message payload
"""
vm6 = run_code(t6)
assert vm6.environment.get("payload") == "the watch is good and Aero is a boy"
assert vm6.environment.get("query") == "create table users with id is null"
print("  [PASS] 6. Quoted strings containing 'is', 'a', 'the', 'table' preserved 100%!")

# 7. Test Silenced Connectors in Variable Declarations & Outputs
t7 = """type enlg
create a count as 10
initialize the total to 0
while count is greater than 0:
    set total = total + count
    set count = count - 1

display the message "Total sum is: " + total
"""
vm7 = run_code(t7)
assert vm7.environment.get("total") == 55
print("  [PASS] 7. Full natural sentence loops ('create a count as 10', 'while count is greater than 0') -> sum = 55")

print("=" * 75)
print("  ALL COPULA, CONNECTOR, AND STRING BOUNDARY TESTS PASSED WITH 100% SUCCESS!")
print("=" * 75)
