"""enlg Hint Discovery Engine.

Scans a sequence of tokens, identifies the hint keyword, and locks the
canonical semantic intent. Operates strictly under the 'fail-closed' principle.
"""

from typing import List
from dataclasses import dataclass
from enlg.lexer.tokens import Token, TokenType
from enlg.core.intents import INTENT_REGISTRY, CONNECTORS
from enlg.diagnostics.diagnostics import UnknownHintError, IntentAmbiguityError

@dataclass
class LockedStatement:
    intent_id: str
    hint_token: Token
    remaining_tokens: List[Token]


class IntentDiscoveryEngine:
    """Discovers and locks the semantic intent for a statement."""
    
    @staticmethod
    def process_statement(statement_tokens: List[Token]) -> LockedStatement:
        if not statement_tokens:
            raise UnknownHintError("Statement is empty.")
            
        candidate_hints = []
        remaining_tokens = []
        
        # Pass 1: Identify all potential hint identifiers
        for token in statement_tokens:
            if token.type == TokenType.IDENTIFIER:
                val = token.value.lower()
                if val in INTENT_REGISTRY:
                    if not candidate_hints:
                        candidate_hints.append((val, token))
                    else:
                        # Subsequent keywords (e.g., 'call', 'native') belong to the expression!
                        remaining_tokens.append(token)
                    continue
                elif val in CONNECTORS:
                    if val in ("from", "using"):
                        remaining_tokens.append(token)
                        continue
                    # Drop filler connectors from AST
                    continue
                    
            remaining_tokens.append(token)
            
        # Pass 2: Enforce fail-closed intent locking
        if len(candidate_hints) == 0:
            # Fallback checks (e.g., direct assignment `x = 10` without 'set')
            if IntentDiscoveryEngine._is_implicit_assignment(statement_tokens):
                return LockedStatement("ASSIGN_VARIABLE", statement_tokens[0], statement_tokens)
            raise UnknownHintError("No valid hint keyword found in statement.")
            
        if len(candidate_hints) > 1:
            # Check if all hints resolve to the same intent (e.g. "declare and create")
            intents_found = {INTENT_REGISTRY[alias] for alias, _ in candidate_hints}
            if len(intents_found) > 1:
                conflicting = ", ".join(sorted(intents_found))
                raise IntentAmbiguityError(f"Found conflicting intents: {conflicting}. Refusing to guess.")
            
            # If they map to the same intent, just take the first one
            
        alias, hint_token = candidate_hints[0]
        canonical_intent = INTENT_REGISTRY[alias]
        
        return LockedStatement(canonical_intent, hint_token, remaining_tokens)

    @staticmethod
    def _is_implicit_assignment(tokens: List[Token]) -> bool:
        if len(tokens) >= 3:
            if tokens[0].type == TokenType.IDENTIFIER and tokens[1].type == TokenType.SYMBOL and tokens[1].value == "=":
                return True
        return False
