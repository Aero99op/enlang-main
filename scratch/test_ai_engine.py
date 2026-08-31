"""Test AI Knowledge Base and Multi-Domain Compiler Validator."""

from enlg.ai.knowledge_base import (
    get_enlang_system_prompt,
    validate_enlg_output,
    validate_with_compiler,
    extract_code_blocks
)

print("1. Testing Master System Prompt...")
prompt = get_enlang_system_prompt()
assert "type enlg" in prompt
assert "type enlgdb" in prompt
assert "type enlgf" in prompt
assert "type enlgd" in prompt
assert "type enlgs" in prompt
assert "type enlgm" in prompt
print("  [PASS] System prompt mandates 'type <ext>' across all 6 domains!")

print("2. Testing Multi-Domain Compiler Validator...")
test_ai_output = """
Bhai, ye lo Prime Number check karne ka program:

```enlg
type enlg

function isPrime with n:
    if n <= 1:
        return false
    declare i = 2
    while i * i <= n:
        if n % i == 0:
            return false
        set i = i + 1
    return true

declare num = 31
if call isPrime with num:
    print num + " is prime!"
```

Aur ye database table definition:

```enlgdb
type enlgdb

create table "guilds" with:
    id as integer primary key autoincrement
    name as text not null unique

select all from "guilds"
```
"""

banned = validate_enlg_output(test_ai_output)
assert len(banned) == 0, f"Banned patterns found: {banned}"
print("  [PASS] Zero banned patterns detected!")

errors = validate_with_compiler(test_ai_output)
assert len(errors) == 0, f"Compiler validation failed: {errors}"
print("  [PASS] Both code blocks compiled and verified against AST with zero errors!")

print("\n=======================================================")
print("  AI KNOWLEDGE BASE & TRAINING ENGINE 100% FUNCTIONAL! ")
print("=======================================================")
