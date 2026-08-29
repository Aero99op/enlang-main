import sys
import os
sys.path.insert(0, os.path.abspath("."))

# Test prototype for token/parsing logic
from enlgs.tokens import HINT_REGISTRY

print("Existing hints count:", len(HINT_REGISTRY))
HINT_REGISTRY["put"] = "PUT_INTO"
HINT_REGISTRY["add"] = "LIST_ADD"
HINT_REGISTRY["push"] = "LIST_ADD"
HINT_REGISTRY["insert"] = "LIST_INSERT"
HINT_REGISTRY["remove"] = "LIST_REMOVE"

from enlgs.lexer import ENLGSLexer

sample_code = """
in script:
    put { kills: 16 } into teamMap["Shadow Ninjas"]
    put 50 into scores[0]
    put 16 into record.kills
    update teamMap[name] to data
    update scores[0] to 50
    update count to 20
    add teamItem to activeT.teams
    add "Shadow Ninjas" to squadList
    remove item at tIdx from activeT.teams
    remove at 2 from scoresList
    insert newMatch at 0 in activeT.matches
    insert "Alpha" at index 1 in namesList
    add class "active" to "nav-item"
    remove class "open" from "sidebar"
"""

lexer = ENLGSLexer(sample_code)
tokens = lexer.tokenize()
print(f"Tokenized {len(tokens)} tokens successfully!")
for t in tokens:
    if t.type.name in ("HINT", "INDENT", "DEDENT"):
        print(f"  {t.type.name}: {t.value}")
