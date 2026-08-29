import sys
import os
sys.path.insert(0, os.path.abspath("."))
if sys.stdout.encoding and sys.stdout.encoding.lower() != "utf-8":
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

from enlgs.tokens import HINT_REGISTRY
from enlgs.ast_nodes import (
    ASTNode, VarAssignNode, DOMClassNode, DOMAddElementNode,
    ListAddNode, ListRemoveAtNode, ListInsertNode, RawJSNode
)
from enlgs.lexer import ENLGSLexer
from enlgs.parser import ENLGSParser
from enlgs.emitter import ENLGSEmitter

# Test full compilation of natural syntax
sample = """
in script:
    # Dictionary & array maps
    put { kills: 16 } into teamMap["Shadow Ninjas"]
    put 50 into scores[0]
    put 16 into record.kills
    put 99 at 2 in matrixRow
    put "Finals" at index 3 in tournamentRounds

    # Updates
    update teamMap["Aero"] to { kills: 8 }
    update scores[1] to 75
    update totalScore to 150
    update record.placePts to 12

    # Set property of target
    set kills of record to 16
    set prize of activeT to "₹50,000"

    # List add / push
    add teamItem to activeT.teams
    add "Shadow Ninjas" to squadList
    push 100 to pointsHistory

    # List remove / splice
    remove item at tIdx from activeT.teams
    remove at 2 from scoresList
    remove 0 from leaderboard

    # List insert
    insert newMatch at 0 in activeT.matches
    insert "Alpha" at index 1 in namesList
    insert bonusCard into rewardsDeck at 2

    # Existing DOM operations must remain 100% functional
    add class "active" to "nav-item"
    remove class "open" from "sidebar"
    add element "div" with class "card"
"""

lexer = ENLGSLexer(sample)
tokens = lexer.tokenize()
parser = ENLGSParser(tokens)
ast = parser.parse()
emitter = ENLGSEmitter(ast)
js_out = emitter.emit()

print("================ COMPILED JAVASCRIPT ================")
print(js_out)
print("======================================================")
