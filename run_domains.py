import traceback
from enlg.cli import run_source

print("--- Test 1 ---")
try:
    source = 'set score to 10\ndeclare check = score is 10'
    vm = run_source(source)
    print("Test 1 check:", vm.environment.get("check"))
except Exception as e:
    traceback.print_exc()

print("--- Test 2 ---")
try:
    source = 'set score to 1\nwhile score is not 4:\n    set score to score + 1'
    vm = run_source(source)
    print("Test 2 score:", vm.environment.get("score"))
except Exception as e:
    traceback.print_exc()
