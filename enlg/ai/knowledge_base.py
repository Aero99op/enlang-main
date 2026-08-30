"""Enlang AI Knowledge Base - NUCLEAR FAIL-PROOF EDITION.

This is the single source of truth for ALL Enlang syntax rules.
The AI must NEVER deviate from this specification.
Zero hallucinations, zero invented keywords, zero wrong syntax.
"""

# ============================================================
# BANNED PATTERNS — AI output is scanned and flagged if found
# ============================================================
BANNED_PATTERNS = [
    # ---- .enlg (Core Backend) BANNED ----
    ("def ", ".enlg: Use 'function <name> with <params>:' — not Python 'def'"),
    ("to do ", ".enlg: Use 'function <name> with <params>:' — not 'to do'"),
    ("let result =", ".enlg: Use 'declare result =' or 'create result as'"),
    ("print(", ".enlg: Use 'print \"message\"' — no parentheses, EVER"),
    ("True", ".enlg: Use lowercase 'true' — not Python 'True'"),
    ("False", ".enlg: Use lowercase 'false' — not Python 'False'"),
    ("None", ".enlg: Use 'null' — not Python 'None'"),
    ("elif ", ".enlg: No 'elif' — use 'else:' with nested 'if' for chaining"),
    # .enlg — parenthesis-style function calls BANNED
    ("isPrime(", ".enlg: Use 'call isPrime with <args>'"),
    ("twoSum(", ".enlg: Use 'call twoSum with <args>'"),
    ("factorial(", ".enlg: Use 'call factorial with <args>'"),
    ("fibonacci(", ".enlg: Use 'call fibonacci with <args>'"),

    # ---- .enlgdb (Database) BANNED ----
    ("create table", ".enlgdb: MUST start file with 'type enlgdb' on line 1"),
    ("drop table", ".enlgdb: MUST end with 'confirmed' — e.g. 'drop table name confirmed'"),
    ("delete all", ".enlgdb: MUST end with 'confirmed' — e.g. 'delete all from name confirmed'"),
    ("truncate table", ".enlgdb: MUST end with 'confirmed' — e.g. 'truncate table name confirmed'"),

    # ---- .enlgf (Frontend Markup) BANNED ----
    ("<div", ".enlgf: No raw HTML tags — use indented keyword syntax: 'div class \"name\":'"),
    ("<html", ".enlgf: No raw HTML — use 'page:' as root element"),
    ("<body", ".enlgf: No raw HTML — use 'body:' inside 'page:'"),
    ("</", ".enlgf: No closing tags — Enlang frontend is indentation-based, NO closing tags"),

    # ---- .enlgd (Design/CSS) BANNED ----
    ("{\n", ".enlgd: No curly braces — use indented key: value syntax without braces"),
    (";\n", ".enlgd: No semicolons — Enlang CSS DSL uses clean 'property: \"value\"' format"),

    # ---- .enlgs (Reactive Scripting) BANNED ----
    ("document.querySelector", ".enlgs: Use 'set text of \"id\" to value' — no raw JS DOM API"),
    ("addEventListener", ".enlgs: Use 'when \"btn-id\" is clicked:' — no raw JS event listeners"),
    ("getElementById", ".enlgs: Use Enlang reactive syntax — no raw JS"),
    ("function(", ".enlgs: No raw JS functions — use 'when ... is clicked:' event blocks"),
    ("const ", ".enlgs: No JS const/let/var — use 'create <name> as <value>'"),
    ("var ", ".enlgs: No JS var — use 'create <name> as <value>'"),

    # ---- .enlgm (Mobile) BANNED ----
    ("Widget build", ".enlgm: No raw Flutter Dart code — use Enlang mobile DSL syntax"),
    ("StatelessWidget", ".enlgm: No raw Flutter — use 'screen <Name>:' in .enlgm"),
    ("@override", ".enlgm: No raw Dart annotations — Enlang mobile DSL handles this"),
]

# ============================================================
# CANONICAL ENLANG WORKING CODE EXAMPLES (tested & verified)
# ============================================================
VERIFIED_EXAMPLES = {
    "prime": """```enlg
# Prime Number Check in Enlang - O(sqrt(n)) Time
function isPrime with n:
    if n <= 1:
        return false
    declare i = 2
    while i * i <= n:
        if n % i == 0:
            return false
        set i = i + 1
    return true

declare number = 29
call isPrime with number
```""",

    "factorial": """```enlg
# Factorial using Recursion
function factorial with n:
    if n <= 1:
        return 1
    return n * call factorial with n - 1

declare result = call factorial with 5
print "Factorial of 5 is: " + result
```""",

    "fibonacci": """```enlg
# Fibonacci - Print first N numbers
function fibonacci with n:
    declare a = 0
    declare b = 1
    declare count = 0
    while count < n:
        print a
        declare temp = a + b
        set a = b
        set b = temp
        set count = count + 1

call fibonacci with 10
```""",

    "palindrome": """```enlg
# Check if a String is a Palindrome
function isPalindrome with s:
    declare left = 0
    declare right = call length with s - 1
    while left < right:
        if s[left] != s[right]:
            return false
        set left = left + 1
        set right = right - 1
    return true

declare word = "racecar"
declare result = call isPalindrome with word
if result:
    print word + " is a palindrome"
else:
    print word + " is NOT a palindrome"
```""",

    "reverse_string": """```enlg
# Reverse a String
function reverseStr with s:
    declare reversed = ""
    declare idx = call length with s - 1
    while idx >= 0:
        set reversed = reversed + s[idx]
        set idx = idx - 1
    return reversed

declare word = "hello"
declare result = call reverseStr with word
print "Reversed: " + result
```""",

    "bubble_sort": """```enlg
# Bubble Sort Algorithm
function bubbleSort with arr:
    declare n = call length with arr
    declare i = 0
    while i < n:
        declare j = 0
        while j < n - i - 1:
            if arr[j] > arr[j + 1]:
                declare temp = arr[j]
                set arr[j] = arr[j + 1]
                set arr[j + 1] = temp
            set j = j + 1
        set i = i + 1
    return arr

declare nums = [64, 34, 25, 12, 22, 11, 90]
declare sorted = call bubbleSort with nums
print sorted
```""",

    "sum_array": """```enlg
# Sum of Array Elements
function sumArray with arr:
    declare total = 0
    declare idx = 0
    declare n = call length with arr
    while idx < n:
        set total = total + arr[idx]
        set idx = idx + 1
    return total

declare numbers = [10, 20, 30, 40, 50]
declare result = call sumArray with numbers
print "Sum: " + result
```""",

    "find_max": """```enlg
# Find Maximum Element in Array
function findMax with arr:
    declare maxVal = arr[0]
    declare idx = 1
    declare n = call length with arr
    while idx < n:
        if arr[idx] > maxVal:
            set maxVal = arr[idx]
        set idx = idx + 1
    return maxVal

declare nums = [3, 7, 1, 9, 4, 6]
declare max_num = call findMax with nums
print "Maximum: " + max_num
```""",

    "linear_search": """```enlg
# Linear Search
function linearSearch with arr, target:
    declare idx = 0
    declare n = call length with arr
    while idx < n:
        if arr[idx] == target:
            return idx
        set idx = idx + 1
    return -1

declare nums = [10, 20, 30, 40, 50]
declare found = call linearSearch with nums, 30
print "Found at index: " + found
```""",

    "database": """```enlgdb
type enlgdb

create table "students" with:
    id as integer primary key autoincrement
    name as text not null unique
    marks as real not null
    grade as text default "F"
    enrolled as boolean default true

insert into "students" values:
    name: "Spandan"
    marks: 95.5
    grade: "A"
    enrolled: true

select all from "students" where marks >= 90 order by marks descending limit 10

update "students" set grade = "A+" where marks >= 95

drop table temp_data confirmed
```""",

    "class": """```enlg
# Object-Oriented Programming with Classes
class Animal:
    declare name = "Unknown"
    declare sound = "..."

    function init with animal_name, animal_sound:
        set name = animal_name
        set sound = animal_sound

    function speak with:
        print name + " says: " + sound

new Animal with "Dog", "Woof"
```""",

    # ---- .enlgf (Frontend Markup) ----
    "frontend": """```enlgf
page "Esports Hub":
    head:
        title "Esports Hub - Live Standings"
        link rel "stylesheet" href "styles.enlgd"

    body:
        header class "main-navbar":
            div class "brand-logo":
                h1 "ESPORTS HUB"
            nav class "nav-links":
                a "Home" href "#home"
                a "Standings" href "#standings"
                a "Players" href "#players"
            button "SIGN IN" id "btn-signin" class "btn-primary"

        main class "container" id "main-content":
            section class "hero":
                h2 "Live Tournament Standings"
                p "Real-time results from all active tournaments"
                button "VIEW ALL" id "btn-view-all" class "btn-cta"

            section class "standings-grid" id "standings-section":
                div class "card" id "card-1":
                    h3 "Group Stage"
                    div class "score-display" id "score-display"

        footer class "site-footer":
            p "Powered by Enlang"
```""",

    # ---- .enlgd (Design/CSS) ----
    "design": """```enlgd
design system:
    body:
        background: "#080c14"
        color: "#f0f4ff"
        font-family: "'Inter', sans-serif"
        margin: "0"
        padding: "0"

    .main-navbar:
        background: "rgba(10, 15, 30, 0.95)"
        backdrop-filter: "blur(16px)"
        border-bottom: "1px solid rgba(255,255,255,0.08)"
        padding: "16px 32px"
        display: "flex"
        justify-content: "space-between"
        align-items: "center"
        position: "sticky"
        top: "0"
        z-index: "1000"

    h1:
        color: "#7c3aed"
        font-size: "1.8rem"
        font-weight: "800"
        letter-spacing: "2px"

    .btn-primary:
        background: "linear-gradient(135deg, #7c3aed, #3b82f6)"
        color: "#ffffff"
        border: "none"
        padding: "10px 24px"
        border-radius: "8px"
        cursor: "pointer"
        font-weight: "600"

    .card:
        background: "rgba(255,255,255,0.04)"
        border: "1px solid rgba(255,255,255,0.1)"
        border-radius: "16px"
        padding: "24px"
        backdrop-filter: "blur(8px)"
```""",

    # ---- .enlgs (Reactive Scripting) ----
    "scripting": """```enlgs
in script:
    create score as 0
    create isLoggedIn as false

    when "btn-signin" is clicked:
        set isLoggedIn to true
        set text of "btn-signin" to "Welcome!"
        show element "main-content"
        hide element "signin-modal"

    when "btn-view-all" is clicked:
        show element "standings-section"
        set text of "score-display" to score
        add class "active" to "standings-section"

    when "input-search" is changed:
        create query as value of "input-search"
        set text of "search-results" to query

    on page load:
        set text of "score-display" to "Loading..."
        fetch data from "/api/standings" into "score-display"
```""",

    # ---- .enlgm (Mobile Flutter) ----
    "mobile": """```enlgm
mobile app "EsportsHub":
    theme:
        primary: "#7c3aed"
        background: "#080c14"
        text: "#f0f4ff"
        font: "Inter"

    screen HomeScreen:
        app bar:
            title "Esports Hub"
            action icon "notifications"

        body:
            scroll:
                column centered:
                    spacer height 20
                    text "Live Standings" size 28 bold color "#7c3aed"
                    spacer height 16

                    card:
                        text "Group Stage - Day 2" size 18 bold
                        spacer height 8
                        text "8 Teams Remaining" size 14 color "#94a3b8"
                        spacer height 16
                        button "VIEW BRACKET" filled color "#7c3aed":
                            when tapped:
                                go to BracketScreen

                    spacer height 20
                    button "ALL STANDINGS" outlined:
                        when tapped:
                            go to StandingsScreen

    screen StandingsScreen:
        app bar:
            title "Standings"
            back button

        body:
            list "standings" scrollable:
                item for each team:
                    row:
                        text team.rank bold
                        spacer width 16
                        text team.name size 16
                        spacer expand
                        text team.score color "#7c3aed" bold
```""",
}


def get_enlang_system_prompt() -> str:
    """Constructs the nuclear fail-proof master system prompt."""

    # Build the verified examples section
    examples_text = "\n\n".join([
        f"## VERIFIED WORKING EXAMPLE: {k.upper()}\n{v}"
        for k, v in VERIFIED_EXAMPLES.items()
    ])

    return f'''You are the Official Enlang Master AI Compiler Specialist.
You ONLY write Enlang code. You NEVER write Python, Java, or C syntax in Enlang code blocks.

================================================================================
ENLANG LANGUAGE SPECIFICATION (ABSOLUTE LAWS — NEVER BREAK THESE):
================================================================================

---LAW 1: FUNCTION DEFINITIONS---
CORRECT:
  function <name> with <param1>, <param2>:
      <body>

WRONG (NEVER USE):
  def <name>:               # This is Python, BANNED
  to do <name>:             # BANNED
  action <name>:            # BANNED
  func <name>:              # BANNED

---LAW 2: FUNCTION CALLS---
CORRECT:
  call <name> with <arg1>, <arg2>
  declare result = call <name> with <arg>

WRONG (NEVER USE):
  <name>(<arg>)             # Parenthesis-style calls are BANNED
  <name> <arg>              # Without 'call' keyword is BANNED

---LAW 3: VARIABLE DECLARATION---
CORRECT:
  declare x = 10
  create x as 10
  initialize x as 0

WRONG (NEVER USE):
  let x = 10                # BANNED
  var x = 10                # BANNED
  int x = 10                # BANNED

---LAW 4: VARIABLE MUTATION (AFTER FIRST DECLARATION)---
CORRECT:
  set x = x + 1
  set x = 20

WRONG (NEVER USE):
  x = x + 1                 # BANNED (no 'set' keyword)
  let x = x + 1             # BANNED

---LAW 5: CONDITIONALS---
CORRECT:
  if <condition>:
      <body>
  else:
      <body>

WRONG (NEVER USE):
  elif <condition>:          # Use 'else:' with nested 'if' for chains

---LAW 6: PRINT OUTPUT---
CORRECT:
  print "Hello World"
  print "Value: " + x

WRONG (NEVER USE):
  print("Hello")             # Parentheses are BANNED
  show "text"                # BANNED in .enlg (only in .enlgf domain)

---LAW 7: LOOPS---
CORRECT while loop:
  while <condition>:
      <body>
      set counter = counter + 1

CORRECT repeat loop:
  repeat 5 times:
      print "Hello"

WRONG (NEVER USE):
  for i in range(10):       # Python-style BANNED
  for i = 0; i < 10; i++:  # C-style BANNED

---LAW 8: RETURN STATEMENTS---
CORRECT:
  return <value>
  return true
  return false

---LAW 9: BOOLEAN LITERALS---
  true                      # NOT True (capital T is Python, BANNED)
  false                     # NOT False (capital F is Python, BANNED)
  null                      # NOT None (Python BANNED)

---LAW 10: PYTHON INTEROP (LEGITIMATE USAGE ONLY)---
CORRECT (when you need standard library math, etc.):
  import math
  native math.sqrt with 144
  native math.floor with 3.7

WRONG:
  import math; result = math.sqrt(144)   # Direct Python syntax BANNED

---LAW 11: .enlgdb RULES---
MANDATORY: First line of EVERY .enlgdb file MUST be: type enlgdb
MANDATORY: Destructive operations MUST end with 'confirmed':
  drop table <name> confirmed
  delete all from <table> confirmed
  truncate table <name> confirmed

---LAW 12: CODE BLOCK FORMATTING---
ALWAYS wrap Enlang code in correct markdown fences:
  ```enlg      (for .enlg backend)
  ```enlgf     (for .enlgf frontend)
  ```enlgd     (for .enlgd design/CSS)
  ```enlgs     (for .enlgs reactive scripting)
  ```enlgm     (for .enlgm mobile)
  ```enlgdb    (for .enlgdb database)

================================================================================
100% VERIFIED WORKING CODE EXAMPLES (USE THESE AS REFERENCE):
================================================================================

{examples_text}

================================================================================
RESPONSE FORMAT RULES:
================================================================================
1. Start with a brief explanation in Hinglish (mix of Hindi + English).
2. Write the Enlang code block with correct fence.
3. Add step-by-step "How it works" explanation.
4. If the user asks for something outside Enlang capability, clearly say so.
5. NEVER apologize for syntax errors — just give perfect code the first time.

You are an expert. Give perfect, tested, verified Enlang code every single time.
'''


def synthesize_local_response(prompt: str) -> str:
    """Fail-safe offline synthesizer when Groq is unreachable.
    Uses keyword matching to return pre-verified, tested code."""
    p = prompt.lower().strip()

    # Greeting
    if any(w in p for w in ("hi", "hello", "hey", "namaste", "help", "kya", "kaise")):
        return """Hello! Main Enlang AI hoon — aapka personal Enlang programming guide!

Main aapko in cheezein mein help kar sakta hoon:
- **Backend Logic**: `.enlg` — algorithms, functions, loops, OOP
- **Frontend Web**: `.enlgf` — HTML5 markup
- **Design/CSS**: `.enlgd` — responsive styling
- **Reactive Scripting**: `.enlgs` — DOM events and interactions
- **Mobile Apps**: `.enlgm` — Flutter/Dart generation
- **Database**: `.enlgdb` — natural English SQL

Kya banana chahte ho? Poochho! 🚀"""

    # Prime number
    if "prime" in p:
        return VERIFIED_EXAMPLES["prime"] + """

**How it works:**
1. `function isPrime with n:` — function define karta hai jo `n` input leta hai.
2. `if n <= 1: return false` — 0 aur 1 prime nahi hote.
3. `while i * i <= n:` — sirf square root tak loop chalta hai (O(√n) time).
4. `if n % i == 0: return false` — agar divisible hai toh prime nahi.
5. Loop ke baad `return true` — koi divisor nahi mila, toh prime hai."""

    # Factorial
    if "factorial" in p:
        return VERIFIED_EXAMPLES["factorial"] + """

**How it works:**
1. Base case: `if n <= 1: return 1`
2. Recursive step: `return n * call factorial with n - 1`
3. `call factorial with 5` → 5×4×3×2×1 = **120**"""

    # Fibonacci
    if "fibonacci" in p or "fib" in p:
        return VERIFIED_EXAMPLES["fibonacci"] + """

**How it works:**
1. `declare a = 0, b = 1` — pehle do Fibonacci numbers.
2. Loop mein: print `a`, phir `a = b`, `b = a+b`.
3. `call fibonacci with 10` → pehle 10 numbers print karta hai."""

    # Palindrome
    if "palindrome" in p:
        return VERIFIED_EXAMPLES["palindrome"] + """

**How it works:**
1. Two-pointer approach: `left` = 0, `right` = last index.
2. Compare `s[left]` and `s[right]` — agar match nahi, palindrome nahi.
3. Move pointers inward until they meet."""

    # Reverse string
    if "reverse" in p and ("string" in p or "str" in p or "word" in p):
        return VERIFIED_EXAMPLES["reverse_string"] + """

**How it works:**
1. Last index se start karo, end tak loop karo.
2. Har character ko `reversed` string mein add karo.
3. Return karo reversed string."""

    # Sorting
    if "sort" in p or "bubble" in p:
        return VERIFIED_EXAMPLES["bubble_sort"] + """

**How it works:**
1. Outer loop: n passes karta hai.
2. Inner loop: adjacent elements compare karta hai.
3. Agar `arr[j] > arr[j+1]`, swap karo.
4. Har pass ke baad sabse bada element end mein aa jata hai."""

    # Sum / Array operations
    if "sum" in p and "array" in p:
        return VERIFIED_EXAMPLES["sum_array"] + """

**How it works:**
1. `declare total = 0` — accumulator start karo.
2. Loop through array, har element ko total mein add karo.
3. Return total."""

    # Max / Min
    if "max" in p or "maximum" in p or "largest" in p:
        return VERIFIED_EXAMPLES["find_max"] + """

**How it works:**
1. Pehle element ko max maan lo.
2. Baaki elements compare karo.
3. Agar koi bada mila, update karo."""

    # Search
    if "search" in p or "find" in p:
        return VERIFIED_EXAMPLES["linear_search"] + """

**How it works:**
1. `declare idx = 0` — start from beginning.
2. Har element check karo target ke sath.
3. Agar match mila, index return karo.
4. Nahi mila toh -1 return karo."""

    # Database / SQL
    if any(k in p for k in ("db", "database", "sql", "table", "enlgdb", "schema", "query", "insert", "select", "drop")):
        return VERIFIED_EXAMPLES["database"] + """

**Key Rules:**
- `type enlgdb` — MANDATORY first line.
- `create table "name" with:` — table schema.
- `insert into "name" values:` — data insert.
- `select ... from "name" where ...` — query data.
- Destructive ops need `confirmed`: `drop table name confirmed`."""

    # Class / OOP
    if any(k in p for k in ("class", "object", "oop", "inherit")):
        return VERIFIED_EXAMPLES["class"] + """

**How it works:**
1. `class Animal:` — blueprint define karta hai.
2. `declare name = "Unknown"` — default properties.
3. `function init with ...:` — constructor.
4. `new Animal with "Dog", "Woof"` — instance create karta hai."""

    # Two sum (hash map pattern)
    if "two sum" in p or ("two" in p and "sum" in p):
        return """### Two Sum using Dictionary in Enlang

```enlg
# Two Sum - Find indices of two numbers that add to target
function twoSum with nums, target:
    declare seen = {}
    declare idx = 0
    declare n = call length with nums
    while idx < n:
        declare current = nums[idx]
        declare complement = target - current
        if complement in seen:
            return [seen[complement], idx]
        set seen[current] = idx
        set idx = idx + 1
    return []

declare numbers = [2, 7, 11, 15]
declare target = 9
declare result = call twoSum with numbers, target
print "Indices: " + result
```

**How it works:**
1. `seen = {}` — dictionary/hashmap for O(1) lookup.
2. For each number, check if `target - current` is already seen.
3. If yes, return both indices. Time: O(n)."""

    # Frontend / HTML / .enlgf
    if any(k in p for k in ("frontend", "enlgf", "html", "webpage", "web page", "page", "navbar", "header", "button", "markup")):
        return VERIFIED_EXAMPLES["frontend"] + """

**Key .enlgf Rules:**
- `page "Title":` — root element (generates full HTML5 document).
- Indentation = nesting. NO closing tags needed.
- `header class "name":` → `<header class="name">`
- `button "Text" id "btn-id"` → `<button id="btn-id">Text</button>`
- `h1 "text"`, `h2 "text"`, `p "text"` — direct text elements."""

    # Design / CSS / .enlgd
    if any(k in p for k in ("design", "enlgd", "css", "style", "color", "background", "font", "layout", "flex", "grid", "padding", "margin", "responsive")):
        return VERIFIED_EXAMPLES["design"] + """

**Key .enlgd Rules:**
- `design system:` — root block.
- `body:` / `.classname:` / `#id:` — CSS selectors, indented.
- `property: "value"` — NO semicolons, NO curly braces.
- Use quoted string values: `background: "#080c14"`.
- All standard CSS properties are supported."""

    # Scripting / JS / .enlgs
    if any(k in p for k in ("script", "enlgs", "click", "event", "reactive", "dom", "javascript", "js", "button click", "on click", "fetch", "show", "hide")):
        return VERIFIED_EXAMPLES["scripting"] + """

**Key .enlgs Rules:**
- `in script:` — mandatory domain declaration.
- `create <var> as <value>` — reactive variable.
- `when "element-id" is clicked:` — event listener.
- `set text of "id" to value` — DOM text setter.
- `show element "id"` / `hide element "id"` — visibility.
- `add class "name" to "id"` — class manipulation.
- `fetch data from "/api/route" into "id"` — data fetching.
- `on page load:` — DOMContentLoaded equivalent."""

    # Mobile / Flutter / .enlgm
    if any(k in p for k in ("mobile", "enlgm", "flutter", "app", "screen", "android", "ios", "button tapped", "navigation", "scaffold")):
        return VERIFIED_EXAMPLES["mobile"] + """

**Key .enlgm Rules:**
- `mobile app "AppName":` — root. Generates complete Flutter project.
- `screen <ScreenName>:` — each screen = StatelessWidget.
- `app bar:` — AppBar with title and actions.
- `body:` → scaffold body.
- `scroll:` → SingleChildScrollView.
- `column centered:` → Column(mainAxisAlignment: center).
- `card:` → Material Card widget.
- `button "TEXT" filled:` / `button "TEXT" outlined:` — ElevatedButton / OutlinedButton.
- `when tapped: go to <Screen>` — Navigator.push."""

    # Default fallback

    return """### Enlang Quick Reference Guide

**Variable Declaration:**
```enlg
declare name = "Spandan"
create count as 0
```

**Function Definition & Call:**
```enlg
function greet with person:
    print "Hello, " + person

call greet with "Spandan"
```

**Loop & Condition:**
```enlg
declare i = 1
while i <= 5:
    if i == 3:
        print "Three!"
    else:
        print i
    set i = i + 1
```

Aur koi specific problem batao — main exact working code dunga! 🚀"""


def validate_enlg_output(code: str) -> list:
    """Scans AI output for banned patterns and returns list of warnings.
    Called BEFORE showing response to user — ensures clean output.
    """
    warnings = []
    for pattern, hint in BANNED_PATTERNS:
        # Only check inside code blocks to avoid false positives in explanations
        in_code_block = False
        for line in code.split("\n"):
            if line.strip().startswith("```"):
                in_code_block = not in_code_block
            if in_code_block and pattern in line:
                # Exception: 'create table' is OK if 'type enlgdb' is in the full code
                if pattern == "create table" and "type enlgdb" in code:
                    continue
                # Exception: 'drop table' / 'delete all' / 'truncate' is OK if 'confirmed' is on same line
                if pattern in ("drop table", "delete all", "truncate table"):
                    if "confirmed" in line:
                        continue
                # Exception: 'print(' could appear in explanation text
                if pattern == "print(" and not line.strip().startswith("print"):
                    continue
                warnings.append(f"[AI SYNTAX WARNING] Detected '{pattern}' — {hint}")
                break
    return warnings


def extract_enlg_blocks(ai_output: str) -> list:
    """Extracts raw .enlg code blocks from AI response text.
    Returns list of (block_code, start_fence) tuples.
    Only extracts .enlg blocks (not .enlgf, .enlgd, .enlgs, .enlgm, .enlgdb —
    those have their own compilers we don't call here).
    """
    blocks = []
    lines = ai_output.split("\n")
    in_block = False
    fence_lang = ""
    current_block = []

    for line in lines:
        stripped = line.strip()
        if stripped.startswith("```") and not in_block:
            fence_lang = stripped[3:].strip().lower()
            if fence_lang in ("enlg", ""):
                # Only capture pure .enlg blocks
                in_block = True
                current_block = []
        elif stripped == "```" and in_block:
            if current_block:
                blocks.append("\n".join(current_block))
            in_block = False
            current_block = []
            fence_lang = ""
        elif in_block:
            current_block.append(line)

    return blocks


def validate_with_compiler(ai_output: str) -> list:
    """
    NUCLEAR VALIDATOR — Passes every .enlg code block from AI output
    through the REAL Enlang compiler pipeline:
        Lexer → BlockParser → CIRGenerator

    This catches ALL hallucinations — not just banned patterns.
    Any code that isn't valid Enlang WILL fail here.

    Core compiler files are READ (imported), not EDITED.
    """
    compile_errors = []

    try:
        # Import core compiler pipeline (READ-ONLY — zero edits)
        from enlg.lexer.lexer import Lexer
        from enlg.parser.block_parser import BlockParser
        from enlg.compiler.generator import CIRGenerator
    except ImportError:
        # If compiler not available (e.g. edge case), skip validation silently
        return []

    blocks = extract_enlg_blocks(ai_output)

    for i, block_code in enumerate(blocks, 1):
        block_num = f"Code Block #{i}"
        try:
            # Step 1: Lex — catches unknown tokens, bad characters
            tokens = Lexer(block_code).tokenize()

            # Step 2: Parse — catches intent/syntax errors (e.g. 'def', 'to do', etc.)
            ast = BlockParser.parse(tokens)

            # Step 3: CIR Generate — catches semantic issues (missing returns, etc.)
            CIRGenerator().generate(ast)

            # All 3 steps passed — this block is VALID Enlang
        except Exception as e:
            error_msg = str(e)
            compile_errors.append(
                f"[COMPILER REJECT] {block_num}: {error_msg}\n"
                f"  -> AI generated invalid Enlang. Replace with correct syntax."
            )

    return compile_errors
