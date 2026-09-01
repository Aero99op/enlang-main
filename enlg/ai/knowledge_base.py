"""Enlang AI Knowledge Base - MASTER FAIL-PROOF EDITION (v1.1.0).

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
    ("elif ", ".enlg: No 'elif' in core .enlg — use 'else:' with nested 'if'"),
    # .enlg — parenthesis-style function calls BANNED
    ("isPrime(", ".enlg: Use 'call isPrime with <args>'"),
    ("twoSum(", ".enlg: Use 'call twoSum with <args>'"),
    ("factorial(", ".enlg: Use 'call factorial with <args>'"),
    ("fibonacci(", ".enlg: Use 'call fibonacci with <args>'"),

    # ---- .enlgdb (Database) BANNED ----
    ("drop table", ".enlgdb: MUST end with 'confirmed' — e.g. 'drop table name confirmed'"),
    ("delete all", ".enlgdb: MUST end with 'confirmed' — e.g. 'delete all from name confirmed'"),
    ("truncate table", ".enlgdb: MUST end with 'confirmed' — e.g. 'truncate table name confirmed'"),

    # ---- .enlgf (Frontend Markup) BANNED ----
    ("<div", ".enlgf: No raw HTML tags — use indented keyword syntax: 'div class \"name\":'"),
    ("<html", ".enlgf: No raw HTML — use 'document enlgf:' as root element"),
    ("<body", ".enlgf: No raw HTML — use 'body:' inside 'document enlgf:'"),
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
type enlg

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
if call isPrime with number:
    print number + " is prime!"
else:
    print number + " is NOT prime!"
```""",

    "factorial": """```enlg
type enlg

# Factorial using Recursion
function factorial with n:
    if n <= 1:
        return 1
    return n * call factorial with (n - 1)

declare result = call factorial with 5
print "Factorial of 5 is: " + result
```""",

    "fibonacci": """```enlg
type enlg

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
type enlg

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

    "bubble_sort": """```enlg
type enlg

# Bubble Sort Algorithm
function bubbleSort with arr:
    declare n = call length with arr
    declare i = 0
    while i < n:
        declare j = 0
        while j < n - i - 1:
            declare next_j = j + 1
            if arr[j] > arr[next_j]:
                declare temp = arr[j]
                set arr[j] = arr[next_j]
                set arr[next_j] = temp
            set j = j + 1
        set i = i + 1
    return arr

declare nums = [64, 34, 25, 12, 22, 11, 90]
declare sorted = call bubbleSort with nums
print "Sorted list: " + sorted
```""",

    "binary_search": """```enlg
type enlg

# Binary Search in Sorted Array
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

declare dataset = [10, 20, 30, 40, 50, 60, 70]
declare idx = call binarySearch with dataset, 40
print "Index of 40: " + idx
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

    "frontend": """```enlgf
type enlgf

document enlgf:
    head:
        title "Esports Arena - Live Championship"
        connect design styles.enlgd
        connect script app.enlgs

    body class "dark-theme":
        header class "main-navbar":
            div class "brand-logo":
                heading 1 "ESPORTS ARENA"
            button "SIGN IN" id "btn-signin" class "btn-primary"

        main class "container" id "main-content":
            section class "hero":
                heading 2 "Live Tournament Standings"
                paragraph "Real-time results from all active brackets."
                button "JOIN QUEUE" id "btn-join" class "btn-cta"
```""",

    "design": """```enlgd
type enlgd

define color "primary" as "#6366f1"
define color "dark-bg" as "#090d16"

for "body" apply:
    background: "#090d16"
    color: "#f8fafc"
    font-family: "Outfit, sans-serif"
    margin: "0"
    padding: "0"
end

for ".main-navbar" apply:
    background: "rgba(15, 23, 42, 0.95)"
    padding: "16px 32px"
    display: "flex"
    justify-content: "space-between"
    align-items: "center"
end

when ".btn-cta" is hovered apply:
    transform: "scale(1.05)"
    opacity: 0.95
end
```""",

    "scripting": """```enlgs
type enlgs

in script:
    create score as 100
    create isLoggedIn as false

    when "btn-signin" is clicked:
        set isLoggedIn = true
        refresh "btn-signin" with "Welcome!"

    when "btn-join" is clicked:
        set score = score + 25
        refresh "main-content" with "Active Score: " + score
```""",

    "mobile": """```enlgm
type enlgm

in mobile:
    use flutter "material"
    use package "google_fonts"

    app "ArenaApp":
        theme dark
        accent color "#6366f1"
        home screen HomeScreen

    screen HomeScreen:
        app bar:
            title "Esports Mobile Arena"
        body:
            scroll:
                column centered:
                    text "Live Standings" size 24, bold
                    button "Join Tournament":
                        when tapped:
                            go to StandingsScreen
```"""
}


def get_enlang_system_prompt() -> str:
    """Constructs the master system prompt with mandatory 'type <extension>' headers."""

    examples_text = "\n\n".join([
        f"## VERIFIED WORKING EXAMPLE: {k.upper()}\n{v}"
        for k, v in VERIFIED_EXAMPLES.items()
    ])

    return f'''You are the Official Enlang Master AI Compiler Specialist (v1.1.0).
You ONLY write Enlang code. You NEVER write Python, Java, or C syntax in Enlang code blocks.

================================================================================
ENLANG ABSOLUTE LAWS (NEVER BREAK THESE):
================================================================================

---LAW 0: MANDATORY 'type <extension>' LINE 1 SIGNATURE---
Every Enlang code file and markdown block MUST start with its exact dialect header on Line 1:
- .enlg   (Core Logic)      -> MUST START WITH: type enlg
- .enlgdb (Database SQL)    -> MUST START WITH: type enlgdb
- .enlgf  (Frontend HTML)   -> MUST START WITH: type enlgf
- .enlgd  (Design CSS)      -> MUST START WITH: type enlgd
- .enlgs  (Reactive Script) -> MUST START WITH: type enlgs
- .enlgm  (Flutter Mobile)  -> MUST START WITH: type enlgm

---LAW 1: FUNCTION DEFINITIONS---
CORRECT:
  function <name> with <param1>, <param2>:
      <body>

WRONG (BANNED):
  def <name>:               # Python BANNED
  to do <name>:             # BANNED
  func <name>:              # BANNED

---LAW 2: FUNCTION CALLS---
CORRECT:
  call <name> with <arg1>, <arg2>
  declare result = call <name> with <arg>

WRONG (BANNED):
  <name>(<arg>)             # Parenthesis calls are BANNED in statements

---LAW 3: VARIABLE DECLARATIONS & ASSIGNMENTS---
CORRECT:
  declare x = 10
  set x = x + 1
  set x = 20

WRONG (BANNED):
  let x = 10                # BANNED
  var x = 10                # BANNED
  int x = 10                # BANNED

---LAW 4: OUTPUT DISPLAY---
CORRECT:
  print "Hello World"
  print "Result: " + result

WRONG (BANNED):
  print("Hello World")      # Parentheses in print BANNED

---LAW 5: CONDITIONALS (IF / ELSE)---
CORRECT:
  if <condition>:
      <body>
  else:
      <body>

---LAW 6: LOOPS---
CORRECT:
  while <condition>:
      <body>
      set i = i + 1

---LAW 7: BOOLEAN LITERALS---
  true                      # lowercase (NOT True)
  false                     # lowercase (NOT False)
  null                      # lowercase (NOT None)

---LAW 8: DATABASE DESTRUCTIVE OPERATIONS---
MANDATORY: Destructive operations in .enlgdb MUST end with 'confirmed':
  drop table <name> confirmed
  delete all from <table> confirmed
  truncate table <name> confirmed

---LAW 9: UNIVERSAL ENGLISH CONNECTORS & THE 'is' COPULA BRIDGE---
- CONNECTORS SILENCING: Filler articles and prepositions (`a`, `an`, `the`, `of`, `to`, `into`, `as`) are natural glue:
    create a score of 78
    display the message "High"
- THE 'is' COPULA BRIDGE: `is` is preserved across all 6 domains:
    Comparisons: if score is greater than 50:, if count is at least 10:, if status is equal to "active":
    Design (.enlgd): when "#btn" is hovered apply:, when ".input" is focused apply:
    Scripting (.enlgs): when "btn-submit" is clicked:, when form is submitted:
    Mobile (.enlgm): when button is tapped:, when card is long pressed:
    Database (.enlgdb): where email is not null, where total is at least 1000
- QUOTED STRINGS BOUNDARY LAW: Any human sentence or data enclosed in `"..."` (`"the watch is good"`, `"Aero is a boy"`) is strictly raw text and 100% immune from syntax parsing.

================================================================================
100% VERIFIED WORKING CODE EXAMPLES (USE THESE AS REFERENCE):
================================================================================

{examples_text}

================================================================================
RESPONSE FORMAT RULES:
================================================================================
1. Start with a brief explanation in Hinglish (mix of Hindi + English).
2. Write the Enlang code block starting with 'type <extension>' on line 1.
3. Add step-by-step "How it works" explanation.
4. NEVER apologize for syntax errors — output verified code the first time.
'''


def synthesize_local_response(prompt: str) -> str:
    """Invoked only when offline or Groq API network connection fails."""
    return f"""[OFFLINE / CONNECTION ERROR]
Enlang AI could not connect to the cloud inference engine.
Please check your internet connection or verify your GROQ_API_KEY.

Your query: "{prompt}"
"""


def validate_enlg_output(code: str) -> list:
    """Scans AI output for banned patterns and returns list of warnings."""
    warnings = []
    for pattern, hint in BANNED_PATTERNS:
        in_code_block = False
        for line in code.split("\n"):
            if line.strip().startswith("```"):
                in_code_block = not in_code_block
            if in_code_block and pattern in line:
                if pattern in ("drop table", "delete all", "truncate table"):
                    if "confirmed" in line:
                        continue
                if pattern == "print(" and not line.strip().startswith("print"):
                    continue
                warnings.append(f"[BANNED SYNTAX] Line: '{line.strip()}' -> {hint}")
    return warnings


def extract_code_blocks(ai_output: str) -> list:
    """Extracts all code blocks and auto-detects domain from 'type <extension>' header."""
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
                block_str = "\n".join(current_block)
                # Auto-detect domain from 'type <extension>' on line 1
                first_line = current_block[0].strip().lower()
                if first_line.startswith("type "):
                    parts = first_line.split()
                    if len(parts) > 1:
                        hdr_domain = parts[1].rstrip(":")
                        if hdr_domain in ("enlg", "enlgdb", "enlgf", "enlgd", "enlgs", "enlgm"):
                            fence_lang = hdr_domain
                blocks.append((fence_lang, block_str))
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
