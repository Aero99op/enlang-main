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
    """Invoked only when offline or Groq API network connection fails."""
    return f"""[OFFLINE / CONNECTION ERROR]
Enlang AI could not connect to the cloud inference engine.
Please check your internet connection or verify your GROQ_API_KEY.

Your query: "{prompt}"
"""



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


def extract_code_blocks(ai_output: str) -> list:
    """Extracts all code blocks and their domain languages from AI response.
    Returns list of (domain_tag, code_str) tuples.
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
            if not fence_lang:
                fence_lang = "enlg"
            in_block = True
            current_block = []
        elif stripped == "```" and in_block:
            if current_block:
                blocks.append((fence_lang, "\n".join(current_block)))
            in_block = False
            current_block = []
            fence_lang = ""
        elif in_block:
            current_block.append(line)

    return blocks


def validate_with_compiler(ai_output: str) -> list:
    """
    UNIVERSAL MULTI-DOMAIN COMPILER VALIDATOR:
    Dynamically tests code blocks against their REAL domain compiler AST:
    - .enlg   -> Backend Logic (Lexer -> BlockParser -> CIRGenerator)
    - .enlgdb -> Natural SQL Database (enlgdb.parser.Parser)
    - .enlgf  -> Frontend HTML Markup (enlgf.parser.ENLEGFPParser)
    - .enlgd  -> Design/CSS DSL (enlgd.parser.ENLGDParser)
    - .enlgs  -> Reactive Scripting (enlgs.parser.ENLGSParser)
    - .enlgm  -> Mobile Flutter DSL (enlgm.parser.ENLGMParser)
    
    ZERO HARDCODING: Tests whatever code the LLM dynamically generates.
    """
    compile_errors = []
    blocks = extract_code_blocks(ai_output)

    for i, (domain, block_code) in enumerate(blocks, 1):
        clean_code = block_code.strip()
        if not clean_code:
            continue

        block_label = f"Block #{i} [.{domain}]"

        try:
            if domain in ("enlg", ""):
                from enlg.lexer.lexer import Lexer
                from enlg.parser.block_parser import BlockParser
                from enlg.compiler.generator import CIRGenerator
                tokens = Lexer(clean_code).tokenize()
                ast = BlockParser.parse(tokens)
                CIRGenerator().generate(ast)

            elif domain == "enlgdb":
                from enlgdb.lexer import Lexer as DBLexer
                from enlgdb.parser import Parser as DBParser
                tokens = DBLexer(clean_code).tokenize()
                DBParser(tokens).parse()

            elif domain == "enlgf":
                from enlgf.lexer import ENLGFLexer
                from enlgf.parser import ENLEGFPParser
                tokens = ENLGFLexer(clean_code).tokenize()
                ENLEGFPParser(tokens).parse()

            elif domain == "enlgd":
                from enlgd.lexer import ENLGDLexer
                from enlgd.parser import ENLGDParser
                tokens = ENLGDLexer(clean_code).tokenize()
                ENLGDParser(tokens).parse()

            elif domain == "enlgs":
                from enlgs.lexer import ENLGSLexer
                from enlgs.parser import ENLGSParser
                tokens = ENLGSLexer(clean_code).tokenize()
                ENLGSParser(tokens).parse()

            elif domain == "enlgm":
                from enlgm.lexer import ENLGMLexer
                from enlgm.parser import ENLGMParser
                tokens = ENLGMLexer(clean_code).tokenize()
                ENLGMParser(tokens).parse()

        except Exception as e:
            compile_errors.append(f"[COMPILER ERROR] {block_label}: {str(e)}")

    return compile_errors

