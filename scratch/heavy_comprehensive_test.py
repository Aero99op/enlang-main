"""Master Heavy Test Suite for Enlang Ecosystem.

Runs extensive, non-destructive validation across all 6 domains and runtime subsystems.
"""

import sys
import os
import traceback

# Ensure root workspace is in sys.path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

# Core Enlang imports
from enlg.lexer.lexer import Lexer
from enlg.parser.block_parser import BlockParser
from enlg.compiler.generator import CIRGenerator
from enlg.runtime.vm import VirtualMachine

# Domain imports
from enlgdb.lexer import Lexer as DBLexer
from enlgdb.parser import Parser as DBParser
from enlgdb.emitter import SQLEmitter
from enlgdb.engine import DatabaseEngine

from enlgf.lexer import ENLGFLexer
from enlgf.parser import ENLEGFPParser
from enlgf.emitter import ENLGFEmitter

from enlgd.lexer import ENLGDLexer
from enlgd.parser import ENLGDParser
from enlgd.emitter import ENLGDEmitter

from enlgs.lexer import ENLGSLexer
from enlgs.parser import ENLGSParser
from enlgs.emitter import ENLGSEmitter

from enlgm.lexer import ENLGMLexer
from enlgm.parser import ENLGMParser
from enlgm.emitter import ENLGMEmitter


results = []

def run_test(category, name, fn):
    try:
        fn()
        results.append((category, name, "PASSED", None))
        print(f"  [PASS] {category} :: {name}")
    except Exception as e:
        err_msg = traceback.format_exc()
        results.append((category, name, "FAILED", err_msg))
        print(f"  [FAIL] {category} :: {name} -> {e}")

def exec_enlg(code):
    tokens = Lexer(code).tokenize()
    ast = BlockParser.parse(tokens)
    cir = CIRGenerator().generate(ast)
    vm = VirtualMachine()
    vm.execute(cir)
    return vm

print("=" * 70)
print("  ENLANG MASTER HEAVY TEST SUITE")
print("=" * 70)

# ─── 1. CORE ENLANG: ARITHMETIC & PRECEDENCE ────────────────────────────────
print("\n--- 1. Testing Core Arithmetic & Pratt Precedence ---")

def test_arithmetic_basic():
    vm = exec_enlg("declare res = 10 + 20 * 3\nset res = res")
    assert vm.environment.get("res") == 70

def test_arithmetic_parens():
    vm = exec_enlg("declare res = (10 + 20) * 3\nset res = res")
    assert vm.environment.get("res") == 90

def test_arithmetic_complex_chain():
    vm = exec_enlg("declare res = 100 - 50 + 25 * 2 // 5 + (2 ** 3)\nset res = res")
    # 100 - 50 = 50; 25 * 2 // 5 = 10; 2 ** 3 = 8; 50 + 10 + 8 = 68
    assert vm.environment.get("res") == 68

def test_arithmetic_modulo():
    vm = exec_enlg("declare is_odd = 29 % 2 == 1\nset is_odd = is_odd")
    assert vm.environment.get("is_odd") is True

run_test("Core Math", "Basic Precedence (* before +)", test_arithmetic_basic)
run_test("Core Math", "Parenthesized Grouping", test_arithmetic_parens)
run_test("Core Math", "Complex Operator Chaining & Power", test_arithmetic_complex_chain)
run_test("Core Math", "Modulo and Equality", test_arithmetic_modulo)

# ─── 2. CORE ENLANG: CONTROL FLOW & IF-ELSE ─────────────────────────────────
print("\n--- 2. Testing Control Flow & Conditionals ---")

def test_if_else_true_branch():
    code = """
declare age = 20
declare status = "Unknown"
if age >= 18:
    set status = "Adult"
else:
    set status = "Minor"
"""
    vm = exec_enlg(code)
    assert vm.environment.get("status") == "Adult"

def test_if_else_false_branch():
    code = """
declare age = 15
declare status = "Unknown"
if age >= 18:
    set status = "Adult"
else:
    set status = "Minor"
"""
    vm = exec_enlg(code)
    assert vm.environment.get("status") == "Minor"

def test_while_loop_accumulator():
    code = """
declare total = 0
declare count = 1
while count <= 10:
    set total = total + count
    set count = count + 1
"""
    vm = exec_enlg(code)
    assert vm.environment.get("total") == 55
    assert vm.environment.get("count") == 11

run_test("Control Flow", "If-Else True Branch Execution", test_if_else_true_branch)
run_test("Control Flow", "If-Else False Branch Execution", test_if_else_false_branch)
run_test("Control Flow", "While Loop Accumulator (Sum 1..10)", test_while_loop_accumulator)

# ─── 3. CORE ENLANG: FUNCTIONS & RECURSION ──────────────────────────────────
print("\n--- 3. Testing Functions & Algorithms ---")

def test_function_simple():
    code = """
function add with x, y:
    return x + y

declare result = call add with 15, 25
"""
    vm = exec_enlg(code)
    assert vm.environment.get("result") == 40

def test_recursive_factorial():
    code = """
function factorial with n:
    if n <= 1:
        return 1
    return n * call factorial with (n - 1)

declare f6 = call factorial with 6
"""
    vm = exec_enlg(code)
    assert vm.environment.get("f6") == 720

def test_prime_check_algorithm():
    code = """
function isPrime with n:
    if n <= 1:
        return false
    declare i = 2
    while i * i <= n:
        if n % i == 0:
            return false
        set i = i + 1
    return true

declare p29 = call isPrime with 29
declare p30 = call isPrime with 30
"""
    vm = exec_enlg(code)
    assert vm.environment.get("p29") is True
    assert vm.environment.get("p30") is False

def test_binary_search():
    code = """
function binarySearch with arr, target:
    declare low = 0
    declare high = call length with arr - 1
    while low <= high:
        declare mid = (low + high) // 2
        if arr[mid] == target:
            return mid
        if arr[mid] < target:
            set low = mid + 1
        else:
            set high = mid - 1
    return -1

declare dataset = [10, 20, 30, 40, 50, 60, 70, 80, 90]
declare found_idx = call binarySearch with dataset, 70
declare missing_idx = call binarySearch with dataset, 99
"""
    vm = exec_enlg(code)
    assert vm.environment.get("found_idx") == 6
    assert vm.environment.get("missing_idx") == -1

run_test("Functions", "Multi-argument Function Call", test_function_simple)
run_test("Functions", "Recursive Factorial (n=6 -> 720)", test_recursive_factorial)
run_test("Algorithms", "Prime Check (29=True, 30=False)", test_prime_check_algorithm)
run_test("Algorithms", "Binary Search (Found & Missing)", test_binary_search)

# ─── 4. CORE ENLANG: DATA STRUCTURES & BUILTINS ──────────────────────────────
print("\n--- 4. Testing Collections & String Coercion ---")

def test_list_and_indexing():
    code = """
declare items = [100, 200, 300, 400]
declare second = items[1]
declare last = items[3]
"""
    vm = exec_enlg(code)
    assert vm.environment.get("second") == 200
    assert vm.environment.get("last") == 400

def test_map_and_lookup():
    code = """
declare config = {"host": "localhost", "port": 8080, "debug": true}
declare host_val = config["host"]
declare port_val = config["port"]
"""
    vm = exec_enlg(code)
    assert vm.environment.get("host_val") == "localhost"
    assert vm.environment.get("port_val") == 8080

def test_string_number_concatenation():
    code = """
declare score = 98
declare msg = "High score: " + score + " points!"
"""
    vm = exec_enlg(code)
    assert vm.environment.get("msg") == "High score: 98 points!"

run_test("Collections", "List indexing", test_list_and_indexing)
run_test("Collections", "Map key lookup", test_map_and_lookup)
run_test("VM Runtime", "Seamless String + Int Concatenation", test_string_number_concatenation)

# ─── 5. DOMAIN: .ENLGDB (NATURAL DATABASE DSL) ──────────────────────────────
print("\n--- 5. Testing enlgdb (Natural Database DSL) ---")

def test_enlgdb_table_and_crud():
    code = """
type enlgdb

create table "players" with:
    id as integer primary key autoincrement
    username as text not null unique
    rating as integer default 1000
    is_pro as boolean default false

insert into "players" values:
    username: "Striker"
    rating: 1450
    is_pro: true

insert into "players" values:
    username: "Rookie"
    rating: 950
    is_pro: false

select all from "players" where rating >= 1000 order by rating descending
"""
    tokens = DBLexer(code).tokenize()
    ast = DBParser(tokens).parse()
    emitter = SQLEmitter(dialect="sqlite")
    sql_tuples = emitter.emit(ast)
    assert len(sql_tuples) == 4

    # Run in in-memory SQLite engine
    engine = DatabaseEngine(":memory:")
    results_list = engine.execute_script(sql_tuples)
    rows = results_list[-1]
    assert len(rows) == 1
    assert rows[0]["username"] == "Striker"
    assert rows[0]["rating"] == 1450

run_test("enlgdb", "Table Creation, Multiple Inserts, Filtered Select on SQLite", test_enlgdb_table_and_crud)

# ─── 6. DOMAIN: .ENLGF (FRONTEND HTML DSL) ──────────────────────────────────
print("\n--- 6. Testing enlgf (Frontend HTML DSL) ---")

def test_enlgf_full_page():
    code = """page "Esports Hub":
    header class "hero-section":
        h1 "Live Tournaments"
        p "Track real-time brackets and leaderboard."
    main class "content-grid":
        section class "match-card":
            h2 "Grand Finals"
            p "Team Alpha vs Team Omega"
            button "Watch Live" id "btn-watch"
"""
    tokens = ENLGFLexer(code).tokenize()
    ast = ENLEGFPParser(tokens).parse()
    html = ENLGFEmitter(ast).emit()
    assert "<!DOCTYPE html>" in html
    assert "<title>Esports Hub</title>" in html
    assert '<header class="hero-section">' in html
    assert '<button id="btn-watch">Watch Live</button>' in html

run_test("enlgf", "Full HTML5 Page Compilation", test_enlgf_full_page)

# ─── 7. DOMAIN: .ENLGD (DESIGN & CSS DSL) ───────────────────────────────────
print("\n--- 7. Testing enlgd (Design CSS DSL) ---")

def test_enlgd_stylesheet():
    code = """define color "primary" as "#6366f1"
define color "dark-bg" as "#090d16"

for "body" apply:
    background: "#090d16"
    color: "#f8fafc"
    font-family: "Outfit, sans-serif"
end

when ".btn-action" is hovered apply:
    transform: "translateY(-2px)"
    opacity: 0.95
end

at media "(max-width: 768px)" apply:
    for ".content-grid" apply:
        display: "block"
    end
end
"""
    tokens = ENLGDLexer(code).tokenize()
    ast = ENLGDParser(tokens).parse()
    css = ENLGDEmitter(ast).emit()
    assert "--primary: #6366f1;" in css
    assert "body {" in css
    assert ".btn-action:hover {" in css
    assert "@media (max-width: 768px) {" in css

run_test("enlgd", "CSS Custom Props, Rules, Hover States, and Media Queries", test_enlgd_stylesheet)

# ─── 8. DOMAIN: .ENLGS (REACTIVE SCRIPTING DSL) ─────────────────────────────
print("\n--- 8. Testing enlgs (Reactive Scripting DSL) ---")

def test_enlgs_reactivity():
    code = """in script:
state count = 0

when button "increment" is clicked:
    set count to count + 1
    set text of "#counter-display" to "Current: " + count
"""
    tokens = ENLGSLexer(code).tokenize()
    ast = ENLGSParser(tokens).parse()
    js = ENLGSEmitter(ast).emit()
    assert "let count = 0;" in js
    assert "addEventListener" in js

run_test("enlgs", "State declaration, Event Listener & DOM text update", test_enlgs_reactivity)

# ─── 9. DOMAIN: .ENLGM (MOBILE FLUTTER DSL) ─────────────────────────────────
print("\n--- 9. Testing enlgm (Mobile Flutter DSL) ---")

def test_enlgm_mobile_screen():
    code = """in mobile:
screen "HomeScreen":
    appbar "Esports Tournament":
        show back button: false
    body:
        list vertical:
            card "Match #1":
                text "Alpha vs Beta"
            card "Match #2":
                text "Gamma vs Delta"
"""
    tokens = ENLGMLexer(code).tokenize()
    ast = ENLGMParser(tokens).parse()
    dart = ENLGMEmitter(ast).emit()
    assert "class HomeScreen extends StatelessWidget" in dart
    assert "AppBar(" in dart
    assert "ListView(" in dart
    assert "Card(" in dart

run_test("enlgm", "Flutter Widget Tree & Screen Emission", test_enlgm_mobile_screen)

# ─── SUMMARY AUDIT ──────────────────────────────────────────────────────────
print("\n" + "=" * 70)
total = len(results)
passed = sum(1 for _, _, s, _ in results if s == "PASSED")
failed = total - passed
print(f"  AUDIT SUMMARY: {passed}/{total} PASSED ({failed} failed)")
print("=" * 70)

if failed > 0:
    print("\nFAILED DETAILS:")
    for cat, name, status, err in results:
        if status == "FAILED":
            print(f"\n--- {cat} :: {name} ---")
            print(err)
    sys.exit(1)
else:
    print("\n ALL SUBSYSTEMS, DOMAINS, ENGINES & COMPILERS 100% HEALTHY!")
    sys.exit(0)
