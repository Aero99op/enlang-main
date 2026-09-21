"""Unit tests for enlg Hint Discovery Engine."""

import unittest
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from enlg.lexer.lexer import Lexer
from enlg.parser.discovery import IntentDiscoveryEngine
from enlg.diagnostics.diagnostics import UnknownHintError, IntentAmbiguityError

class TestDiscovery(unittest.TestCase):

    def test_valid_intent_lock(self):
        source = 'declare x to 20'
        tokens = Lexer(source).tokenize()
        
        # Strip newline/EOF for statement testing
        stmt_tokens = [t for t in tokens if t.type.name not in ("NEWLINE", "EOF")]
        
        locked = IntentDiscoveryEngine.process_statement(stmt_tokens)
        self.assertEqual(locked.intent_id, "DECLARE_VARIABLE")
        self.assertEqual(locked.hint_token.value, "declare")

    def test_alias_resolution(self):
        source = 'initialize y as 50'
        tokens = Lexer(source).tokenize()
        stmt_tokens = [t for t in tokens if t.type.name not in ("NEWLINE", "EOF")]
        
        locked = IntentDiscoveryEngine.process_statement(stmt_tokens)
        self.assertEqual(locked.intent_id, "DECLARE_VARIABLE")

    def test_unknown_hint(self):
        source = 'magically make x 10'
        tokens = Lexer(source).tokenize()
        stmt_tokens = [t for t in tokens if t.type.name not in ("NEWLINE", "EOF")]
        
        with self.assertRaises(UnknownHintError):
            IntentDiscoveryEngine.process_statement(stmt_tokens)

    def test_intent_ambiguity(self):
        # Valid nesting / identifier overlap: "declare flag = display"
        # 'display' is in expression position after '=', so it does not conflict with 'declare'.
        source = "declare flag = display"
        tokens = Lexer(source).tokenize()
        locked = IntentDiscoveryEngine.process_statement(tokens)
        self.assertEqual(locked.intent_id, "DECLARE_VARIABLE")
        self.assertEqual(locked.hint_token.value, "declare")

    def test_valid_nested_action_in_assignment(self):
        # 'set result to split text by comma'
        # 'split' is after 'to' in expression position, subordinate to 'set'.
        source = "set result to split text"
        tokens = Lexer(source).tokenize()
        stmt_tokens = [t for t in tokens if t.type.name not in ("NEWLINE", "EOF")]
        locked = IntentDiscoveryEngine.process_statement(stmt_tokens)
        self.assertEqual(locked.intent_id, "ASSIGN_VARIABLE")
        self.assertEqual(locked.hint_token.value, "set")

    def test_conflicting_root_intents_raises_ambiguity(self):
        # Competing root-level intents: 'train model and test dataset'
        source = "train model and test dataset"
        tokens = Lexer(source).tokenize()
        stmt_tokens = [t for t in tokens if t.type.name not in ("NEWLINE", "EOF")]
        with self.assertRaises(IntentAmbiguityError):
            IntentDiscoveryEngine.process_statement(stmt_tokens)

    def test_consecutive_conflicting_intents_raises_ambiguity(self):
        # Competing root-level keywords: 'train evaluate model'
        source = "train evaluate model"
        tokens = Lexer(source).tokenize()
        stmt_tokens = [t for t in tokens if t.type.name not in ("NEWLINE", "EOF")]
        with self.assertRaises(IntentAmbiguityError):
            IntentDiscoveryEngine.process_statement(stmt_tokens)

    def test_synonymous_root_hints_resolves_cleanly(self):
        # Synonymous aliases: 'declare and create x' both resolve to DECLARE_VARIABLE
        source = "declare and create x"
        tokens = Lexer(source).tokenize()
        stmt_tokens = [t for t in tokens if t.type.name not in ("NEWLINE", "EOF")]
        locked = IntentDiscoveryEngine.process_statement(stmt_tokens)
        self.assertEqual(locked.intent_id, "DECLARE_VARIABLE")

    def test_fallback_assignment(self):
        source = "x = 42"
        tokens = Lexer(source).tokenize()
        stmt_tokens = [t for t in tokens if t.type.name not in ("NEWLINE", "EOF")]
        locked = IntentDiscoveryEngine.process_statement(stmt_tokens)
        self.assertEqual(locked.intent_id, "ASSIGN_VARIABLE")

if __name__ == '__main__':
    unittest.main()
