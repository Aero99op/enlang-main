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
            
        # Check for direct implicit assignment (e.g. `x = 10`)
        if IntentDiscoveryEngine._is_implicit_assignment(statement_tokens):
            return LockedStatement("ASSIGN_VARIABLE", statement_tokens[0], statement_tokens)

        root_candidate_hints = []
        paren_depth = 0
        bracket_depth = 0
        in_expression_mode = False
        first_hint_seen = False

        for i, token in enumerate(statement_tokens):
            # Track structural nesting
            if token.type == TokenType.SYMBOL:
                if token.value in ("(", "{"):
                    paren_depth += 1
                elif token.value in (")", "}"):
                    if paren_depth > 0:
                        paren_depth -= 1
                elif token.value == "[":
                    bracket_depth += 1
                elif token.value == "]":
                    if bracket_depth > 0:
                        bracket_depth -= 1
                elif token.value == "=":
                    in_expression_mode = True
                    continue

            # Check for assignment connectors switching to expression mode
            if first_hint_seen and token.type == TokenType.IDENTIFIER:
                val_lower = token.value.lower()
                if val_lower in ("to", "as") and i > 1:
                    in_expression_mode = True
                    continue

            # Check candidate hint
            if token.type == TokenType.IDENTIFIER:
                val = token.value.lower()
                if val in INTENT_REGISTRY:
                    # Grammatical guard: only root clause identifiers are statement intent candidates.
                    # Tokens in nested brackets, parens, or on the RHS of assignments are expressions.
                    if paren_depth == 0 and bracket_depth == 0 and not in_expression_mode:
                        root_candidate_hints.append((val, token))
                        first_hint_seen = True

        if len(root_candidate_hints) == 0:
            raise UnknownHintError("No valid hint keyword found in statement.")

        # Detect conflicting root intents
        if len(root_candidate_hints) > 1:
            intents_found = {INTENT_REGISTRY[alias] for alias, _ in root_candidate_hints}
            if len(intents_found) > 1:
                conflicting = ", ".join(sorted(intents_found))
                raise IntentAmbiguityError(f"Found conflicting intents: {conflicting}. Refusing to guess.")

        primary_alias, primary_token = root_candidate_hints[0]
        canonical_intent = INTENT_REGISTRY[primary_alias]

        # Remaining tokens excludes the locked primary hint token
        remaining_tokens = [t for t in statement_tokens if t is not primary_token]

        return LockedStatement(canonical_intent, primary_token, remaining_tokens)

    @staticmethod
    def _is_implicit_assignment(tokens: List[Token]) -> bool:
        if len(tokens) >= 3:
            if tokens[0].type == TokenType.IDENTIFIER and tokens[1].type == TokenType.SYMBOL and tokens[1].value == "=":
                return True
        return False
